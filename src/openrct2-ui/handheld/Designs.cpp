// SPDX-License-Identifier: GPL-3.0-only
#include "Designs.h"
#include "FileBrowsing.h"
#include "DesignAvailability.h"

#include "Currency.h"
#include "FlatRides.h"
#include "Mazes.h"

#include <filesystem>
#include <map>
#include <openrct2/OpenRCT2.h>
#include <openrct2/actions/track/SavedTrackDesignPlaceAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/Drawing.Sprite.h>
#include <openrct2/drawing/IDrawingEngine.h>
#include <openrct2/drawing/NewDrawing.h>
#include <openrct2/ride/ShopPlacement.h>
#include <openrct2/ride/TrackConstructionSupport.h>
#include <openrct2/ride/TrackDesignRepository.h>
#include <openrct2/ride/ted/TrackElemType.h>
#include <set>
namespace OpenRCT2::Ui::Handheld::Designs
{
    namespace GA = GameActions;
    struct Request
    {
        CoordsXYZD origin;
        bool operator==(const Request&) const = default;
    };
    static std::unique_ptr<TrackDesign> design;
    static std::string selection;
    static Direction direction{};
    static std::optional<int32_t> heightOffset;
    static std::unique_ptr<ShopPlacement::PreviewRide> preview;
    static std::optional<Request> requested, ghost, review;
    static std::string lastError;
    static uint32_t sceneryOmitted{}, reviewOmitted{}, placedOmitted{};
    static std::vector<int32_t> reviewContext;
    static money64 reviewCost{};
    static bool reviewNoMoney{};
    static uint32_t depth{};
    static uint64_t revision{};
    static bool busy{}, scenery{}, sceneryWarning{};
    static std::optional<RideManagement::Target> completed;
    static GA::Result Invalid(const char* message)
    {
        GA::Result result;
        result.error = GA::Status::invalidParameters;
        result.errorMessage = message;
        return result;
    }
    bool Eligible(const TrackDesign& td)
    {
        return GA::SavedTrackDesignPlaceAction::Eligible(td);
    }
    std::string UnavailableReason(const TrackDesign& td)
    {
        if(Eligible(td)) return {};
        const auto type=td.trackAndVehicle.rtdIndex;
        if(type!=RIDE_TYPE_MAZE && !TrackConstructionSupport::Type(type)) return "Ride family has no Pocket saved-design placement";
        const auto index=GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(td.trackAndVehicle.vehicleObject);
        const auto* entry=GetRideEntryByIndex(index);
        if(!entry) return "Required vehicle object is not loaded";
        if(std::find(std::begin(entry->ride_type),std::end(entry->ride_type),type)==std::end(entry->ride_type)) return "Vehicle and ride family are incompatible";
        if(FlatRidePlacement::StationObject()==kObjectEntryIndexNull) return "Required station object is not loaded";
        if(!getGameState().cheats.ignoreResearchStatus && (!RideTypeIsInvented(type) || !RideEntryIsInvented(index))) return "Ride type or vehicle is not researched";
        return "Native geometry is unsupported for placement";
    }
    struct PreviewImage
    {
        std::filesystem::file_time_type stamp{};
        uint64_t digest{};
        std::vector<Drawing::PaletteIndex> pixels;
    };
    static std::map<std::string, std::weak_ptr<PreviewImage>> images;
    static std::optional<FileBrowsing::Identity> selectedFile;
    static bool browserReturn{};
    bool BrowserReturn() { return browserReturn; }
    void SetBrowserReturn(bool value) { browserReturn=value; }
    std::optional<PickerEntry> FileEntry(const std::string& path, std::optional<ObjectEntryDescriptor> vehicle, int width, int height, Direction orientation)
    {
                const auto identity = FileBrowsing::Capture(std::filesystem::u8path(path));
                if (!identity) return {};
                auto imported = TrackDesignImport(path.c_str());
                if (imported && vehicle) {
                    const auto i=GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(*vehicle);
                    if (!DesignAvailability::Compatible(*imported,i)) return {};
                    imported->trackAndVehicle.vehicleObject=*vehicle;
                }
                if (!imported || !Eligible(*imported) || !FileBrowsing::Current(*identity))
                    return {};
                auto td = std::shared_ptr<TrackDesign>(std::move(imported));
                std::error_code error;
                const auto stamp = std::filesystem::last_write_time(path, error);
                const auto imageKey=path+":"+td->trackAndVehicle.vehicleObject.ToString();
                auto image = images[imageKey].lock();
                if (!image || error || image->stamp != stamp || image->digest != identity->digest)
                {
                    image = std::make_shared<PreviewImage>();
                    image->stamp = stamp; image->digest=identity->digest;
                    images[imageKey] = image;
                }
                const auto index = GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(
                    td->trackAndVehicle.vehicleObject);
                PickerEntry entry{ { ObjectType::ride, index, path }, std::filesystem::path(path).stem().string(), ZoomLevel{ 3 } };
                entry.drawPreview = [td, image, identity, width, height, orientation](Drawing::RenderTarget& rt) {
                    if (gLegacyScene != LegacyScene::playing || !Eligible(*td) || !FileBrowsing::Current(*identity))
                        return;
                    if (image->pixels.empty())
                    {
                        // The native renderer stashes/restores the map. In playing mode it
                        // does not load objects. Preserve the unused ride slot it borrows.
                        const auto id = GetNextFreeRideId();
                        if (id.IsNull())
                            return;
                        auto previous = std::move(getGameState().rides[id.ToUnderlying()]);
                        auto pixels = std::make_unique<TrackDesignPreviewBuffer>();
                        TrackDesignDrawPreview(*td, *pixels, false);
                        image->pixels.assign(pixels->begin(), pixels->end());
                        getGameState().rides[id.ToUnderlying()] = std::move(previous);
                    }
                    // Sample the real native four-orientation image into this viewport.
                    // Respect dirty render-target clipping; never write outside the target.
                    const int scale = std::min(width * 1000 / 370, height * 1000 / 217);
                    const int w = std::max(1, 370 * scale / 1000), h = std::max(1, 217 * scale / 1000);
                    const int left = (width - w) / 2, top = (height - h) / 2;
                    for (int y = std::max(top, rt.y); y < std::min(top + h, rt.y + rt.height); ++y)
                        for (int x = std::max(left, rt.x); x < std::min(left + w, rt.x + rt.width); ++x)
                        {
                            const auto colour = image->pixels[(orientation & 3) * kTrackPreviewImageSize
                                + (y - top) * 217 / h * 370 + (x - left) * 370 / w];
                            if (colour != static_cast<Drawing::PaletteIndex>(0))
                                rt.bits[(y - rt.y) * rt.LineStride() + x - rt.x] = colour;
                        }
                };
                return entry;
    }
    std::vector<PickerEntry> Entries()
    {
        std::vector<PickerEntry> entries;
        auto* repository = GetContext()->GetTrackDesignRepository();
        if (!repository)
            return entries;
        std::set<std::string> paths;
        auto collect = [&](ride_type_t type, const std::string& object) {
            for (const auto& file : repository->GetItemsForObjectEntry(type, object))
            {
                if (!paths.insert(file.path).second)
                    continue;
                if (auto entry = FileEntry(file.path)) entries.push_back(std::move(*entry));
            }
        };
        for (ride_type_t type = 0; type < RIDE_TYPE_COUNT; ++type)
            if (type == RIDE_TYPE_MAZE || TrackConstructionSupport::Type(type))
            {
                collect(type, "");
                for (ObjectEntryIndex i = 0; i < kMaxRideObjects; ++i)
                    if ((type == RIDE_TYPE_MAZE && Mazes::Eligible(i)) || TrackConstructionSupport::Available(i, type))
                        if (const auto* object = GetContext()->GetObjectManager().GetLoadedObject<RideObject>(i))
                            collect(type, std::string(object->GetObjectEntry().GetName()));
            }
        return entries;
    }
    void ClearPreview()
    {
        requested.reset();
        if (preview && ghost && design)
            if (auto* ride = GetRide(preview->id()))
                TrackDesignPreviewRemoveGhosts(*design, *ride, ghost->origin);
        ghost.reset();
        preview.reset();
    }
    void End()
    {
        lastError.clear(); sceneryOmitted=reviewOmitted=placedOmitted=0;
        browserReturn=false; selectedFile.reset();
        ClearPreview();
        design.reset();
        heightOffset.reset();
        review.reset();
        completed.reset();
        busy = false;
        ++revision;
    }
    bool Begin(const std::string& path, std::optional<ObjectEntryDescriptor> vehicle)
    {
        const auto file=FileBrowsing::Capture(std::filesystem::u8path(path));
        auto imported = TrackDesignImport(path.c_str());
        if (imported && vehicle) {
            const auto i=GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(*vehicle);
            if (!DesignAvailability::Compatible(*imported,i)) return false;
            imported->trackAndVehicle.vehicleObject=*vehicle;
        }
        if (!vehicle && design && selection==path && selectedFile==file && imported)
            imported->trackAndVehicle.vehicleObject=design->trackAndVehicle.vehicleObject;
        if (!file || !imported || !Eligible(*imported) || !FileBrowsing::Current(*file))
            return false;
        const bool same = selection == path && selectedFile == file;
        const auto previousHeight=heightOffset;
        End();
        design = std::move(imported);
        selectedFile=file;
        if(same) heightOffset=previousHeight;
        selection = path;
        if (!same)
            direction = 0;
        return true;
    }
    const TrackDesign* Draft() { return design.get(); }
    bool ChooseVehicle(const ObjectEntryDescriptor& vehicle)
    {
        if (!design || busy || review || !selectedFile || !FileBrowsing::Current(*selectedFile)) return false;
        const auto index=GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(vehicle);
        if (!DesignAvailability::Compatible(*design,index)) return false;
        ClearPreview(); design->trackAndVehicle.vehicleObject=vehicle; ++revision; return true;
    }
    std::string VehicleNotice()
    {
        if (!design) return "Vehicle unavailable";
        const auto* object=GetContext()->GetObjectManager().GetLoadedObject(design->trackAndVehicle.vehicleObject);
        return "Vehicle: " + (object?object->GetName():design->trackAndVehicle.vehicleObject.ToString());
    }
    std::string LastError() { return lastError; }
    uint32_t PlacementSceneryOmitted() { return placedOmitted; }
    std::vector<std::string> WarningLines()
    {
        if (!design) return {"Design unavailable"};
        std::vector<std::string> lines{VehicleNotice(), HeightNotice(), SceneryNotice()};
        if (!lastError.empty()) lines.insert(lines.begin(),"Rejected: "+lastError);
        if (scenery && sceneryOmitted) lines.push_back("Scenery omitted: "+std::to_string(sceneryOmitted));
        if (scenery && sceneryWarning) lines.push_back("This design will be placed without scenery");
        lines.push_back("Vehicle limits may change train settings");
        lines.push_back("Some scenery may remain if placement fails");
        return lines;
    }
    bool Active()
    {
        return design != nullptr;
    }
    bool Busy()
    {
        return busy;
    }
    const std::string& Selection()
    {
        return selection;
    }
    Direction Orientation()
    {
        return direction;
    }
    void Rotate()
    {
        if (design && !busy && !review)
        {
            ClearPreview();
            direction = (direction + 1) & 3;
        }
    }
    bool Height(int step)
    {
        if (!design || busy || review || !step)
            return false;
        const auto next = heightOffset.value_or(0) + (step > 0 ? 2 * kCoordsZStep : -2 * kCoordsZStep);
        if (next < -128 || next > 256)
            return false;
        ClearPreview();
        heightOffset = next;
        return true;
    }
    void AutomaticHeight()
    {
        if (!design || busy || review)
            return;
        ClearPreview();
        heightOffset.reset();
    }
    std::optional<int32_t> HeightOffset()
    {
        return heightOffset;
    }
    std::string HeightNotice()
    {
        return heightOffset ? "Ground offset " + std::string(*heightOffset >= 0 ? "+" : "") + std::to_string(*heightOffset / kCoordsZStep) + " levels"
                            : "Fits to ground";
    }
    void SetScenery(bool value)
    {
        if (!design || busy || review || scenery == value)
            return;
        ClearPreview();
        scenery = value;
    }
    bool IncludesScenery() { return scenery; }
    std::string SceneryNotice()
    {
        if (!design || design->sceneryElements.empty())
            return "No design scenery";
        if (!scenery)
            return "Scenery Off / R2: On";
        return sceneryWarning ? "All scenery omitted" : "Scenery On / L2: Off";
    }
    static GA::Result QueryAt(const Request& request)
    {
        if (!design || !Eligible(*design) || !selectedFile || !FileBrowsing::Current(*selectedFile))
            return Invalid("Design unavailable");
        sceneryWarning = false;
        if (scenery && !design->sceneryElements.empty())
        {
            std::unique_ptr<ShopPlacement::PreviewRide> borrowed;
            auto* ride = preview ? GetRide(preview->id()) : nullptr;
            if (!ride)
            {
                borrowed = std::make_unique<ShopPlacement::PreviewRide>(
                    GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(design->trackAndVehicle.vehicleObject),
                    design->trackAndVehicle.rtdIndex);
                ride = GetRide(borrowed->id());
            }
            if (ride)
            {
                TrackDesignPlace(*design, {}, true, *ride, request.origin);
                sceneryWarning = _trackDesignPlaceStateSceneryUnavailable;
            }
        }
        auto action = GA::SavedTrackDesignPlaceAction(
            request.origin, *design, Config::Get().general.defaultInspectionInterval, kMoney64Undefined, scenery);
        if (preview && !preview->id().IsNull())
        {
            action.UsePreviewRideForQuery(preview->id());
            auto result=GA::Query(&action, getGameState());
            sceneryOmitted=_trackDesignPlaceStateSceneryOmitted;
            return result;
        }
        const auto id = GetNextFreeRideId();
        if (id.IsNull())
            return GA::Query(&action, getGameState());
        auto previous = std::move(getGameState().rides[id.ToUnderlying()]);
        auto result = GA::Query(&action, getGameState());
        sceneryOmitted=_trackDesignPlaceStateSceneryOmitted;
        getGameState().rides[id.ToUnderlying()] = std::move(previous);
        return result;
    }
    static std::optional<Request> Resolve(std::optional<CoordsXY> tile)
    {
        if (!design || !Eligible(*design) || !selectedFile || !FileBrowsing::Current(*selectedFile) || !tile || !MapIsLocationValid(*tile) || tile->x % 32 || tile->y % 32)
            return {};
        // Same native terrain-fit calculation and bounded height search as desktop.
        auto* ride = preview ? GetRide(preview->id()) : nullptr;
        std::unique_ptr<ShopPlacement::PreviewRide> borrowed;
        if (!ride)
        {
            borrowed = std::make_unique<ShopPlacement::PreviewRide>(
                GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(design->trackAndVehicle.vehicleObject),
                design->trackAndVehicle.rtdIndex);
            ride = GetRide(borrowed->id());
        }
        if (!ride)
            return {};
        CoordsXYZD origin{ *tile, MapGetHighestZ(*tile), direction };
        origin.z += TrackDesignGetZPlacement(*design, *ride, origin);
        origin.z += heightOffset.value_or(0);
        // Release this borrowed slot before querying native RideCreate.
        return Request{ origin };
    }
    GA::Result Query(std::optional<CoordsXY> tile)
    {
        const auto request = Resolve(tile);
        if (!request)
            return Invalid("Choose land / design unavailable");
        auto origin = *request;
        GA::Result result;
        for (int i = 0; i < (heightOffset ? 1 : 7); ++i, origin.origin.z += kCoordsZStep)
        {
            result = QueryAt(origin);
            if (result.error == GA::Status::ok || result.error == GA::Status::insufficientFunds)
                break;
        }
        return result;
    }
    static std::optional<Request> Find(std::optional<CoordsXY> tile)
    {
        auto request = Resolve(tile);
        if (!request)
            return {};
        for (int i = 0; i < (heightOffset ? 1 : 7); ++i, request->origin.z += kCoordsZStep)
            if (QueryAt(*request).error == GA::Status::ok)
                return request;
        return {};
    }
    void UpdatePreview(std::optional<CoordsXY> tile)
    {
        if (!design || busy || review || depth)
            return;
        const auto next = Find(tile);
        if (!next)
        {
            ClearPreview();
            return;
        }
        if (ghost == next && preview)
            return;
        ClearPreview();
        requested = next;
        preview = std::make_unique<ShopPlacement::PreviewRide>(
            GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(design->trackAndVehicle.vehicleObject),
            design->trackAndVehicle.rtdIndex);
        if (preview->id().IsNull())
        {
            ClearPreview();
            return;
        }
        auto* ride = GetRide(preview->id());
        ride->entranceStyle = GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(
            design->appearance.stationObjectIdentifier);
        if (ride->entranceStyle == kObjectEntryIndexNull)
            ride->entranceStyle = getGameState().lastEntranceStyle;
        for (size_t i = 0; i < std::size(ride->trackColours); ++i)
            ride->trackColours[i] = design->appearance.trackColours[i];
        ghost = next; // Also clean up partial native placement on failure.
        auto result = TrackDesignPlace(
            *design, { GA::CommandFlag::apply, GA::CommandFlag::ghost, GA::CommandFlag::noSpend }, scenery && !sceneryWarning,
            *ride, next->origin);
        if (result.error != GA::Status::ok)
            ClearPreview();
    }
    void RemovePreviewTemporarily()
    {
        ++depth;
        const auto saved = requested;
        ClearPreview();
        requested = saved;
    }
    void RestorePreview()
    {
        if (!depth || --depth || !requested)
            return;
        UpdatePreview(requested->origin);
    }
    bool Review(std::optional<CoordsXY> tile)
    {
        if (busy || review)
            return false;
        ClearPreview();
        auto next = Find(tile);
        if (!next)
            return false;
        auto result = QueryAt(*next);
        if (result.error != GA::Status::ok)
            return false;
        lastError.clear(); reviewOmitted=sceneryOmitted;
        reviewContext=TrackDesignPlacementContext::Capture(*design);
        review = next;
        reviewCost = result.cost;
        reviewNoMoney = getGameState().park.flags.has(ParkFlag::noMoney);
        return true;
    }
    bool HasReview()
    {
        return review.has_value();
    }
    void CancelReview()
    {
        review.reset();
    }
    std::optional<CoordsXYZD> ReviewLocation()
    {
        return review ? std::optional<CoordsXYZD>{ review->origin } : std::nullopt;
    }
    money64 ReviewCost()
    {
        return reviewCost;
    }
    std::optional<RideManagement::Target> Completed()
    {
        return completed;
    }
    static GA::Result ConfirmReviewed()
    {
        if (!review || busy)
            return Invalid("No design review");
        const auto request = *review;
        review.reset();
        if (!design || reviewContext!=TrackDesignPlacementContext::Capture(*design))
            return Invalid("Design objects changed; review again");
        if (reviewNoMoney != getGameState().park.flags.has(ParkFlag::noMoney))
            return Invalid("Park money rules changed; review again");
        auto result = QueryAt(request);
        if (result.error != GA::Status::ok)
            return result;
        if (result.cost != reviewCost)
            return Invalid("Placement changed; review again");
        auto action = GA::SavedTrackDesignPlaceAction(
            request.origin, *design, Config::Get().general.defaultInspectionInterval, reviewCost, scenery);
        action.BindReview(reviewContext,*selectedFile,reviewOmitted);
        busy = true;
        const auto token = revision;
        action.SetCallback([token](const auto*, const GA::Result* finished) {
            if (token != revision)
                return;
            busy = false;
            if (finished->error != GA::Status::ok) { lastError=finished->getErrorMessage();return; }
            placedOmitted=_trackDesignPlaceStateSceneryOmitted;
            completed = RideManagement::Identify(finished->getData<RideId>());
        });
        result = GA::Execute(&action, getGameState());
        if (result.error != GA::Status::ok)
            busy = false;
        return result;
    }
    GA::Result Confirm()
    {
        auto result=ConfirmReviewed();
        if (result.error!=GA::Status::ok) lastError=result.getErrorMessage();
        return result;
    }
    void DrawPanel(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        using namespace Concept;
        Box(rt, kOrigin, { kRailWidth, 172 });
        Heading(rt, kOrigin + ScreenCoordsXY{ 10, 12 }, "Saved design");
        Section(rt, kOrigin + ScreenCoordsXY{ 10, 35 }, GetNameFromTrackPath(selection));
        Row(rt, kOrigin + ScreenCoordsXY{ 8, 54 }, "Place design", Icon::path, true);
        Rule(rt, kOrigin + ScreenCoordsXY{ 8, 99 }, 148);
        DrawModernEllipsised(rt, kOrigin + ScreenCoordsXY{ 12, 116 }, 140, "{MUTED}" + (!lastError.empty()?"Review rejected / X":completed && placedOmitted?"Omitted "+std::to_string(placedOmitted)+" scenery":SceneryNotice()));
        DrawModernEllipsised(rt, kOrigin + ScreenCoordsXY{ 12, 142 }, 140, "{MUTED}" + HeightNotice());
        DrawModernEllipsised(rt, kOrigin + ScreenCoordsXY{ 12, 158 }, 140, "{MUTED}X Design options");
        DrawFooter(
            rt, screen, { { "D-pad", "Move" }, { "A", "Review" }, { "Y", "Designs" }, { "SEL", "Turn" }, { "B", "Back" } });
    }
} // namespace OpenRCT2::Ui::Handheld::Designs

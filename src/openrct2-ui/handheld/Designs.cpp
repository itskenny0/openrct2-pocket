// SPDX-License-Identifier: GPL-3.0-only
#include "Designs.h"

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
    struct PreviewImage
    {
        std::filesystem::file_time_type stamp{};
        std::vector<Drawing::PaletteIndex> pixels;
    };
    static std::map<std::string, std::weak_ptr<PreviewImage>> images;
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
                auto imported = TrackDesignImport(file.path.c_str());
                if (!imported || !Eligible(*imported))
                    continue;
                auto td = std::shared_ptr<TrackDesign>(std::move(imported));
                std::error_code error;
                const auto stamp = std::filesystem::last_write_time(file.path, error);
                auto image = images[file.path].lock();
                if (!image || error || image->stamp != stamp)
                {
                    image = std::make_shared<PreviewImage>();
                    image->stamp = stamp;
                    images[file.path] = image;
                }
                const auto index = GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(
                    td->trackAndVehicle.vehicleObject);
                PickerEntry entry{ { ObjectType::ride, index, file.path }, file.name, ZoomLevel{ 3 } };
                entry.drawPreview = [td, image](Drawing::RenderTarget& rt) {
                    if (gLegacyScene != LegacyScene::playing || !Eligible(*td))
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
                        image->pixels.assign(pixels->begin(), pixels->begin() + kTrackPreviewImageSize);
                        getGameState().rides[id.ToUnderlying()] = std::move(previous);
                    }
                    G1Element sprite{};
                    sprite.offset = reinterpret_cast<uint8_t*>(image->pixels.data());
                    sprite.width = 370;
                    sprite.height = 217;
                    sprite.flags = { G1Flag::hasTransparency };
                    GfxSetG1Element(SPR_TEMP_TRACK_LIST, &sprite);
                    DrawingEngineInvalidateImage(SPR_TEMP_TRACK_LIST);
                    rt.zoom_level = ZoomLevel{ 3 };
                    GfxDrawSprite(rt, ImageId(SPR_TEMP_TRACK_LIST), { 48, 112 });
                };
                entries.push_back(std::move(entry));
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
        ClearPreview();
        design.reset();
        heightOffset.reset();
        review.reset();
        completed.reset();
        busy = false;
        ++revision;
    }
    bool Begin(const std::string& path)
    {
        auto imported = TrackDesignImport(path.c_str());
        if (!imported || !Eligible(*imported))
            return false;
        const bool same = selection == path;
        End();
        design = std::move(imported);
        selection = path;
        if (!same)
            direction = 0;
        return true;
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
        return heightOffset ? "Manual height " + std::string(*heightOffset >= 0 ? "+" : "") + std::to_string(*heightOffset / 8)
                            : "Automatic height";
    }
    void SetScenery(bool value)
    {
        if (!design || busy || review || scenery == value)
            return;
        ClearPreview();
        scenery = value;
    }
    std::string SceneryNotice()
    {
        if (!design || design->sceneryElements.empty())
            return "No design scenery";
        if (!scenery)
            return "Scenery Off / R2: On";
        return sceneryWarning ? "Scenery missing; omitted" : "Scenery On / L2: Off";
    }
    static GA::Result QueryAt(const Request& request)
    {
        if (!design || !Eligible(*design))
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
            return GA::Query(&action, getGameState());
        }
        const auto id = GetNextFreeRideId();
        if (id.IsNull())
            return GA::Query(&action, getGameState());
        auto previous = std::move(getGameState().rides[id.ToUnderlying()]);
        auto result = GA::Query(&action, getGameState());
        getGameState().rides[id.ToUnderlying()] = std::move(previous);
        return result;
    }
    static std::optional<Request> Resolve(std::optional<CoordsXY> tile)
    {
        if (!design || !Eligible(*design) || !tile || !MapIsLocationValid(*tile) || tile->x % 32 || tile->y % 32)
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
    GA::Result Confirm()
    {
        if (!review || busy)
            return Invalid("No design review");
        const auto request = *review;
        review.reset();
        if (reviewNoMoney != getGameState().park.flags.has(ParkFlag::noMoney))
            return Invalid("Park money rules changed; review again");
        auto result = QueryAt(request);
        if (result.error != GA::Status::ok)
            return result;
        if (result.cost != reviewCost)
            return Invalid("Placement changed; review again");
        auto action = GA::SavedTrackDesignPlaceAction(
            request.origin, *design, Config::Get().general.defaultInspectionInterval, reviewCost, scenery);
        busy = true;
        const auto token = revision;
        action.SetCallback([token](const auto*, const GA::Result* finished) {
            if (token != revision)
                return;
            busy = false;
            if (finished->error != GA::Status::ok)
                return;
            completed = RideManagement::Identify(finished->getData<RideId>());
        });
        result = GA::Execute(&action, getGameState());
        if (result.error != GA::Status::ok)
            busy = false;
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
        DrawModernEllipsised(rt, kOrigin + ScreenCoordsXY{ 12, 116 }, 140, "{MUTED}" + SceneryNotice());
        DrawModernEllipsised(rt, kOrigin + ScreenCoordsXY{ 12, 142 }, 140, "{MUTED}" + HeightNotice());
        DrawModernEllipsised(rt, kOrigin + ScreenCoordsXY{ 12, 158 }, 140, "{MUTED}X Design options");
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 8, y }, 38, "D-pad", "Move");
        DrawCommand(rt, { 115, y }, 17, "A", "Review");
        DrawCommand(rt, { 219, y }, 17, "Y", "Designs");
        DrawCommand(rt, { 329, y }, 30, "SEL", "Turn");
        DrawCommand(rt, { 440, y }, 17, "B", "Back");
    }
} // namespace OpenRCT2::Ui::Handheld::Designs

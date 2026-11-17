// SPDX-License-Identifier: GPL-3.0-only
#include "FlatRides.h"

#include "AccessPreview.h"
#include "AutoAccess.h"
#include "ConstructionPanel.h"
#include "RideAccess.h"
#include "TrackPreview.h"

#include <openrct2/actions/ride/FlatRideEntrancePlaceAction.h>
#include <openrct2/actions/ride/FlatRidePlaceAction.h>
#include <openrct2/actions/track/TrackPlaceAction.h>
namespace OpenRCT2::Ui::Handheld::FlatRides
{
    static Stage stage{};
    static ObjectEntryIndex selected = kObjectEntryIndexNull;
    static Direction direction{};
    static std::optional<RideManagement::Target> target;
    static bool busy{};
    static std::optional<RideAccess::ExistingPoint> editing;
    static StationIndex stationIndex{ StationIndex::FromUnderlying(0) };
    static uint64_t revision{};
    static std::string notice; // Why Auto could not place, until the stage changes.
    struct Request
    {
        CoordsXYZD origin;
        ObjectEntryIndex index;
        bool operator==(const Request&) const = default;
    };
    static std::optional<Request> requested, ghost;
    static TrackPreview preview;
    static uint32_t depth{};
    static GameActions::Result Invalid(const std::string& message)
    {
        GameActions::Result result;
        result.error = GameActions::Status::invalidParameters;
        result.errorMessage = message;
        return result;
    }
    bool Eligible(ObjectEntryIndex index)
    {
        return FlatRidePlacement::Eligible(index);
    }
    std::vector<PickerEntry> Entries()
    {
        std::vector<PickerEntry> entries;
        for (ObjectEntryIndex i = 0; i < kMaxRideObjects; ++i)
            if (Eligible(i))
                entries.push_back(
                    { { ObjectType::ride, i },
                      GetContext()->GetObjectManager().GetLoadedObject<RideObject>(i)->GetName(),
                      ZoomLevel{ 1 } });
        return entries;
    }
    std::string Name()
    {
        const auto* object = GetContext()->GetObjectManager().GetLoadedObject<RideObject>(selected);
        return object ? object->GetName() : "Ride unavailable";
    }
    Stage CurrentStage()
    {
        return stage;
    }
    ObjectEntryIndex Selection()
    {
        return selected;
    }
    Direction Orientation()
    {
        return direction;
    }
    bool Busy()
    {
        return busy;
    }
    std::optional<RideManagement::Target> Target()
    {
        return target;
    }
    void ClearPreview()
    {
        requested.reset();
        AccessPreview::Clear();
        preview.clear();
        ghost.reset();
    }
    void End()
    {
        notice.clear();
        ClearPreview();
        stage = Stage::inactive;
        editing.reset();
        busy = false;
        target.reset();
        ++revision;
    }
    bool Begin(ObjectEntryIndex index)
    {
        if (!Eligible(index))
            return false;
        const bool keepDirection = selected == index;
        End();
        selected = index;
        if (!keepDirection)
            direction = 0;
        stage = Stage::ride;
        return true;
    }
    bool CanResume(const RideManagement::Target& t)
    {
        const auto* ride = RideManagement::Resolve(t);
        if (!ride || !FlatRidePlacement::SupportsStation(*ride) || ride->status != RideStatus::closed
            || !ride->getRideTypeDescriptor().flags.has(RtdFlag::hasEntranceAndExit))
            return false;
        for (const auto& station : ride->getStations())
            if (!station.start.isNull() && (station.entrance.isNull() || station.exit.isNull()))
                return true;
        return false;
    }
    static void NextStation()
    {
        notice.clear();
        stage = Stage::inactive;
        const auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        if (!ride)
            return;
        for (size_t i = 0; i < ride->getStations().size(); ++i)
        {
            const auto& station = ride->getStations()[i];
            if (!station.start.isNull() && (station.entrance.isNull() || station.exit.isNull()))
            {
                stationIndex = StationIndex::FromUnderlying(i);
                stage = station.entrance.isNull() ? Stage::entrance : Stage::exit;
                return;
            }
        }
    }
    bool Resume(const RideManagement::Target& t)
    {
        if (!CanResume(t))
            return false;
        End();
        target = t;
        selected = t.object;
        NextStation();
        return true;
    }
    bool ResumeExisting(const RideAccess::ExistingPoint& point)
    {
        if (!RideAccess::Current(point) || RideManagement::Resolve(point.target)->status != RideStatus::closed)
            return false;
        End();
        editing = point;
        target = point.target;
        selected = point.target.object;
        stationIndex = point.station;
        stage = point.exit ? Stage::exit : Stage::entrance;
        return true;
    }
    void Rotate()
    {
        if (!busy && stage == Stage::ride)
        {
            ClearPreview();
            direction = (direction + 1) & 3;
        }
    }
    static std::optional<Request> Resolve(std::optional<CoordsXY> tile)
    {
        if (!tile || !MapIsLocationValid(*tile) || tile->x < 32 || tile->y < 32 || tile->x % 32 || tile->y % 32
            || !Eligible(selected))
            return {};
        const auto type = FlatRidePlacement::Type(selected);
        const auto& ted = TrackMetadata::GetTrackElementDescriptor(GetRideTypeDescriptor(type).StartTrackPiece);
        int32_t z = 0;
        for (uint8_t i = 0; i < ted.sequenceData.numSequences; ++i)
        {
            const auto& block = ted.sequenceData.sequences[i].clearance;
            const auto p = *tile + CoordsXY{ block.x, block.y }.rotate(direction);
            if (!MapIsLocationValid(p) || !MapGetSurfaceElementAt(p))
                return {};
            z = std::max(z, MapGetHighestZ(p) - block.z);
        }
        return Request{ { *tile, z, direction }, selected };
    }
    std::optional<CoordsXYZD> EntranceLocation(std::optional<CoordsXY> tile)
    {
        if (!tile || !target || !RideManagement::Resolve(*target))
            return {};
        return FlatRidePlacement::EntranceLocation(*GetRide(target->id), *tile, stationIndex);
    }
    static GameActions::Result Query(const Request& request)
    {
        if (ghost && ghost->index == request.index && !preview.id().IsNull())
        {
            const auto type = FlatRidePlacement::Type(request.index);
            auto action = GameActions::TrackPlaceAction(
                preview.id(), GetRideTypeDescriptor(type).StartTrackPiece, type, request.origin, 0, 0, 0, {}, false);
            return GameActions::Query(&action, getGameState());
        }
        auto action = GameActions::FlatRidePlaceAction(request.index, request.origin);
        return GameActions::Query(&action, getGameState());
    }
    GameActions::Result Action(std::optional<CoordsXY> tile, bool execute)
    {
        if (busy)
            return Invalid("Placement pending");
        if (execute)
            ClearPreview();
        if (stage == Stage::ride)
        {
            const auto request = Resolve(tile);
            if (!request)
            {
                ClearPreview();
                return Invalid("Ride unavailable / move onto land");
            }
            const auto result = Query(*request);
            if (result.error != GameActions::Status::ok)
                ClearPreview();
            if (!execute || result.error != GameActions::Status::ok)
                return result;
            auto action = GameActions::FlatRidePlaceAction(selected, request->origin);
            const auto token = revision;
            busy = true;
            action.SetCallback([token](const auto*, const GameActions::Result* completed) {
                if (token != revision)
                    return;
                busy = false;
                if (completed->error != GameActions::Status::ok)
                    return;
                target = RideManagement::Identify(completed->getData<RideId>());
                const auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
                stationIndex = StationIndex::FromUnderlying(0);
                stage = ride && ride->getRideTypeDescriptor().flags.has(RtdFlag::hasEntranceAndExit) ? Stage::entrance
                                                                                                     : Stage::inactive;
            });
            auto queued = GameActions::Execute(&action, getGameState());
            if (queued.error != GameActions::Status::ok)
                busy = false;
            return queued;
        }
        if (stage != Stage::entrance && stage != Stage::exit)
            return Invalid("No active construction");
        if (!target || !RideManagement::Resolve(*target))
            return Invalid("Ride no longer available");
        const bool isExit = stage == Stage::exit;
        const auto& station = GetRide(target->id)->getStation(stationIndex);
        if (!editing && !(isExit ? station.exit : station.entrance).isNull())
            return Invalid("Entrance/exit already placed; resume construction");
        const auto location = EntranceLocation(tile);
        if (!location)
            return Invalid("Choose a free tile beside the ride");
        if (editing)
        {
            std::string error;
            auto draft = RideAccess::Review(*editing, *location, error);
            if (!draft)
                return Invalid(error);
            const auto quote = GameActions::Query(&draft->action, getGameState());
            if (execute && quote.error == GameActions::Status::ok)
                Windows::HandheldAccessReviewOpen(*draft);
            return quote;
        }
        auto action = GameActions::FlatRideEntrancePlaceAction(target->id, *location, isExit, stationIndex);
        const auto result = AccessPreview::Bind(action, *location);
        if (!execute || result.error != GameActions::Status::ok)
        {
            if (execute && result.error != GameActions::Status::ok)
                ClearPreview();
            return result;
        }
        ClearPreview();
        const auto token = revision;
        busy = true;
        action.SetCallback([token](const auto*, const GameActions::Result* completed) {
            if (token != revision)
                return;
            busy = false;
            if (completed->error != GameActions::Status::ok)
                return;
            NextStation();
        });
        auto queued = GameActions::Execute(&action, getGameState());
        if (queued.error != GameActions::Status::ok)
            busy = false;
        return queued;
    }
    GameActions::Result AutoPlace(std::string& summary)
    {
        if (busy)
            return Invalid("Placement pending");
        if ((stage != Stage::entrance && stage != Stage::exit) || editing || !target || !RideManagement::Resolve(*target))
            return Invalid("Nothing to place");
        const auto plan = AutoAccess::Make(*target, stationIndex, summary);
        if (!plan)
        {
            notice = summary;
            return Invalid(summary);
        }
        ClearPreview();
        const auto token = revision;
        busy = true;
        auto result = AutoAccess::Execute(*plan, *target, stationIndex, [token] {
            if (token != revision)
                return;
            busy = false;
            NextStation();
        });
        summary = result.error == GameActions::Status::ok ? AutoAccess::Summary(*plan) : result.getErrorMessage();
        return result;
    }
    void UpdatePreview(std::optional<CoordsXY> tile)
    {
        if ((stage == Stage::entrance || stage == Stage::exit) && !busy && target)
        {
            preview.clear();
            ghost.reset();
            requested.reset();
            const auto loc = EntranceLocation(tile);
            if (editing && !RideAccess::Current(*editing))
            {
                AccessPreview::Clear();
                return;
            }
            if (loc)
                AccessPreview::Update(*target, *loc, stage == Stage::exit, stationIndex, editing.has_value());
            else
                AccessPreview::Clear();
            return;
        }
        if (stage != Stage::ride || busy)
        {
            ClearPreview();
            return;
        }
        const auto request = Resolve(tile);
        if (!request || Query(*request).error != GameActions::Status::ok)
        {
            ClearPreview();
            return;
        }
        requested = request;
        if (depth || (ghost == request && preview.exists()))
            return;
        preview.clear();
        ghost.reset();
        if (preview.place(selected, FlatRidePlacement::Type(selected), request->origin))
            ghost = request;
    }
    void RemovePreviewTemporarily()
    {
        ++depth;
        preview.clear();
        ghost.reset();
    }
    void RestorePreview()
    {
        if (!depth || --depth || !requested)
            return;
        UpdatePreview(requested->origin);
    }
    void DrawPanel(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        using namespace Concept;
        const bool placing = stage == Stage::ride;
        const auto p = kOrigin;
        Box(rt, p, { 216, 190 });
        Heading(rt, p + ScreenCoordsXY{ 10, 12 }, editing ? "Move ride access" : "Build ride");
        DrawWrappedLabel(rt, p + ScreenCoordsXY{ 12, 36 }, 192, Name(), 2);
        if (!editing)
            DrawPlacementSteps(rt, p + ScreenCoordsXY{ 12, 76 }, 192, placing ? 0 : stage == Stage::entrance ? 1 : 2);
        else
            DrawStrongText(
                rt, p + ScreenCoordsXY{ 12, 80 }, stage == Stage::entrance ? "{WHITE}Move entrance" : "{WHITE}Move exit", 192);
        if (const auto* object = GetContext()->GetObjectManager().GetLoadedObject(ObjectType::ride, selected))
        {
            Drawing::RenderTarget thumbnail;
            if (ClipRenderTarget(thumbnail, rt, p + ScreenCoordsXY{ 12, 110 }, 64, 64))
            {
                thumbnail.zoom_level = ZoomLevel{ 1 };
                object->DrawPreview(thumbnail, 128, 128);
            }
        }
        DrawStrongText(
            rt, p + ScreenCoordsXY{ 86, 116 },
            placing                        ? "{WHITE}Place ride"
                : stage == Stage::entrance ? "{WHITE}Place entrance"
                                           : "{WHITE}Place exit",
            118);
        if (!notice.empty() && !placing)
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 86, 140 }, 118, "{CAUTION}" + notice);
        else
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ 86, 140 }, 118,
                placing ? "{MUTED}Faces " + std::string(std::array{ "NW", "SW", "SE", "NE" }[direction & 3])
                        : "{MUTED}Station " + std::to_string(stationIndex.ToUnderlying() + 1));
        if (!editing)
            DrawCommand(rt, p + ScreenCoordsXY{ 86, 163 }, 32, placing ? "SELECT" : "Y", placing ? "Turn" : "Auto + paths");
        DrawActionStrip(rt, screen);

        std::vector<ControlHint> hints{ { "D-pad", "Move" },
                                        { "A", editing ? "Review" : "Place" },
                                        { "B", target ? "Ride" : "Cancel" },
                                        { "START", target ? "Ride" : "Park", HintPriority::guide } };
        if (!editing && placing)
        {
            hints.push_back({ "Y", "Rides" });
            hints.push_back({ "SELECT", "Turn", HintPriority::guide });
        }
        else if (!editing)
            hints.push_back({ "Y", "Auto" });
        DrawFooter(rt, screen, hints);
    }
} // namespace OpenRCT2::Ui::Handheld::FlatRides

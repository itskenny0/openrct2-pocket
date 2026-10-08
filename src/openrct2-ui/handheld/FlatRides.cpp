// SPDX-License-Identifier: GPL-3.0-only
#include "FlatRides.h"

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
    static StationIndex stationIndex{ StationIndex::FromUnderlying(0) };
    static uint64_t revision{};
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
        preview.clear();
        ghost.reset();
    }
    void End()
    {
        ClearPreview();
        stage = Stage::inactive;
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
        if (!(isExit ? station.exit : station.entrance).isNull())
            return Invalid("Entrance/exit already placed; resume construction");
        const auto location = EntranceLocation(tile);
        if (!location)
            return Invalid("Choose a free tile beside the ride");
        auto action = GameActions::FlatRideEntrancePlaceAction(target->id, *location, isExit, stationIndex);
        const auto result = GameActions::Query(&action, getGameState());
        if (!execute || result.error != GameActions::Status::ok)
            return result;
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
    void UpdatePreview(std::optional<CoordsXY> tile)
    {
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
        Box(rt, p, { kRailWidth, 146 });
        Heading(rt, p + ScreenCoordsXY{ 10, 12 }, "Build ride");
        Section(rt, p + ScreenCoordsXY{ 10, 35 }, Name());
        Row(rt, p + ScreenCoordsXY{ 8, 54 },
            placing                        ? "Place ride"
                : stage == Stage::entrance ? "Entrance"
                                           : "Exit",
            Icon::path, true);
        Rule(rt, p + ScreenCoordsXY{ 8, 99 }, 148);
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ 12, 116 }, 140,
            placing ? "{MUTED}Ground placement" : "{MUTED}Station " + std::to_string(stationIndex.ToUnderlying() + 1));
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 8, y }, 38, "D-pad", "Move");
        DrawCommand(rt, { 115, y }, 17, "A", "Place");
        if (placing)
        {
            DrawCommand(rt, { 207, y }, 17, "Y", "Rides");
            DrawCommand(rt, { 299, y }, 30, "SEL", "Turn");
        }
        DrawCommand(rt, { 420, y }, 17, "B", "Back");
    }
} // namespace OpenRCT2::Ui::Handheld::FlatRides

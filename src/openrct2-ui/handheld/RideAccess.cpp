// SPDX-License-Identifier: GPL-3.0-only
#include "RideAccess.h"

#include "Explorer.h"
#include "FlatRides.h"
#include "Paths.h"
#include "Currency.h"
#include <openrct2/ride/AccessPlacementContext.h>

#include <openrct2-ui/interface/Window.h>
#include <openrct2/Context.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/ride/FlatRidePlacement.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/tile_element/EntranceElement.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <set>
#include <tuple>
namespace OpenRCT2::Ui::Handheld::RideAccess
{
    static std::string Coordinates(CoordsXYZD p)
    {
        return "Tile " + std::to_string(p.x / 32) + "," + std::to_string(p.y / 32)
            + " / Z " + std::to_string(p.z) + " / dir " + std::to_string(p.direction);
    }
    std::string PointLabel(const ExistingPoint& p)
    {
        return "Station " + std::to_string(p.station.ToUnderlying() + 1) + (p.exit ? " / Exit" : " / Entrance");
    }
    std::vector<ExistingPoint> Existing(const RideManagement::Target& target)
    {
        std::vector<ExistingPoint> result;
        const auto* ride = RideManagement::Resolve(target);
        if (!ride || !Supported(target)) return result;
        for (size_t i = 0; i < ride->getStations().size(); ++i)
        {
            const auto& s = ride->getStations()[i];
            if (s.start.isNull()) continue;
            for (bool exit : { false, true })
            {
                const auto& source = exit ? s.exit : s.entrance;
                if (source.isNull()) continue;
                auto context = AccessPlacementContext::Capture(*ride, source.toCoordsXY());
                if (!context.empty()) result.push_back({target, StationIndex::FromUnderlying(i), exit, source.toCoordsXYZD(), std::move(context)});
            }
        }
        return result;
    }
    bool Current(const ExistingPoint& p)
    {
        const auto* ride = RideManagement::Resolve(p.target);
        if (!ride || p.station.ToUnderlying() >= ride->getStations().size()) return false;
        const auto& s = ride->getStation(p.station);
        const auto& source = p.exit ? s.exit : s.entrance;
        return !source.isNull() && source.toCoordsXYZD() == p.source && !p.context.empty()
            && p.context == AccessPlacementContext::Capture(*ride, p.source);
    }
    std::optional<EditDraft> Review(const ExistingPoint& point, std::optional<CoordsXYZD> destination, std::string& error)
    {
        if (!Current(point)) { error = "Access changed; select it again"; return {}; }
        auto action = GameActions::FlatRideEntrancePlaceAction(point.target.id, destination.value_or(point.source), point.exit, point.station);
        action.EditExisting(!destination);
        auto q = GameActions::Query(&action, getGameState());
        if (q.error != GameActions::Status::ok) { error = q.getErrorMessage(); if(error.empty()) error=q.getErrorTitle(); return {}; }
        action.GuardContext(AccessPlacementContext::Capture(*RideManagement::Resolve(point.target), destination.value_or(point.source)), q.cost);
        return EditDraft{point, destination.value_or(point.source), !destination, std::move(action), q.cost};
    }
    ManagementPresentation ReviewPresentation(const EditDraft& d, size_t selected, const std::string& feedback)
    {
        ManagementPresentation m;
        m.title = d.remove ? "Remove ride access?" : "Move ride access?";
        m.breadcrumb = "Rides / Entrances & exits";
        m.selected = selected; m.confirmation = true;
        m.rows = {"Apply native edit", "Cancel"};
        m.bodyLines = {PointLabel(d.point), "From: " + Coordinates(d.point.source)};
        if (!d.remove) m.bodyLines.push_back("To: " + Coordinates(d.destination));
        m.bodyLines.push_back("Native cost: " + FormatMoney(d.cost));
        m.bodyLines.push_back("Reconnect queue / exit path afterward.");
        m.bodyLines.push_back("Native maze hedges update with access.");
        if (!d.remove) m.bodyLines.push_back("Sequential move; failure may leave a gap.");
        m.status = feedback.empty() ? "Native edit clears testing / queue state" : feedback;
        return m;
    }
    bool BeginRelocation(const ExistingPoint& point)
    {
        if (!Current(point) || RideManagement::Resolve(point.target)->status != RideStatus::closed) return false;
        BeginTerrain(true);
        return FlatRides::ResumeExisting(point);
    }
    bool Supported(const RideManagement::Target& target)
    {
        const auto* ride = RideManagement::Resolve(target);
        return ride && FlatRidePlacement::SupportsStation(*ride)
            && ride->getRideTypeDescriptor().flags.has(RtdFlag::hasEntranceAndExit)
            && !ride->getStation(StationIndex::FromUnderlying(0)).start.isNull();
    }
    // Native path heights/directions and reciprocal edges, excluding provisional elements.
    static const PathElement* PathAt(CoordsXYZ p, Direction entering)
    {
        if (!MapIsLocationValid(p))
            return nullptr;
        for (auto* path : TileElementsView<PathElement>(p))
            if (!path->isGhost() && FootpathIsZAndDirectionValid(*path, p.z / kCoordsZStep, entering)
                && (path->getEdges() & (1 << DirectionReverse(entering))))
                return path;
        return nullptr;
    }
    static bool QueueReachesNormal(CoordsXYZ start, const PathElement* first, RideId ride, StationIndex station)
    {
        std::vector<std::pair<CoordsXY, const PathElement*>> pending{ { start, first } };
        std::set<std::tuple<int32_t, int32_t, int32_t>> visited;
        while (!pending.empty())
        {
            auto [xy, path] = pending.back();
            pending.pop_back();
            if (!visited.emplace(xy.x, xy.y, path->getBaseZ()).second)
                continue;
            if (!path->isQueue())
                return true;
            if (path->getRideIndex() != ride || path->getStationIndex() != station)
                continue;
            for (Direction d : kAllDirections)
            {
                if (!(path->getEdges() & (1 << d)))
                    continue;
                const auto z = path->getBaseZ() + (path->isSloped() && path->getSlopeDirection() == d ? kPathHeightStep : 0);
                const CoordsXYZ next{ xy + CoordsDirectionDelta[d], z };
                if (const auto* neighbour = PathAt(next, d))
                    pending.emplace_back(next, neighbour);
            }
        }
        return false;
    }
    // Read-only endpoint traversal, including native sloped edge heights. A normal
    // path attachment alone does not establish guest access to the park entrance.
    static bool ReachesParkEntrance(CoordsXYZ start, const PathElement* first, RideId ride, StationIndex station)
    {
        std::vector<std::pair<CoordsXY, const PathElement*>> pending{ { start, first } };
        std::set<std::tuple<int32_t, int32_t, int32_t>> seen;
        while (!pending.empty())
        {
            auto [xy, path] = pending.back();
            pending.pop_back();
            if (!seen.emplace(xy.x, xy.y, path->getBaseZ()).second)
                continue;
            if (path->isQueue() && (path->getRideIndex() != ride || path->getStationIndex() != station))
                continue;
            for (Direction d : kAllDirections)
            {
                if (!(path->getEdges() & (1 << d)))
                    continue;
                const auto z = path->getBaseZ() + (path->isSloped() && path->getSlopeDirection() == d ? kPathHeightStep : 0);
                const CoordsXYZ next{ xy + CoordsDirectionDelta[d], z };
                if (!MapIsLocationValid(next))
                    continue;
                for (auto* entrance : TileElementsView<EntranceElement>(next))
                    if (!entrance->isGhost() && entrance->getEntranceType() == EntranceType::parkEntrance
                        && entrance->getSequenceIndex() == ParkEntranceSequence::centre && entrance->getBaseZ() == z
                        && (entrance->getDirections() & (1 << ((DirectionReverse(d) - entrance->getDirection()) & 3))))
                        return true;
                if (const auto* neighbour = PathAt(next, d))
                    pending.emplace_back(next, neighbour);
            }
        }
        return false;
    }
    bool NormalPathReachesParkEntrance(CoordsXYZ position)
    {
        if (!MapIsLocationValid(position))
            return false;
        for (auto* path : TileElementsView<PathElement>(position))
            if (!path->isGhost() && !path->isQueue() && path->getBaseZ() == position.z)
                return ReachesParkEntrance(position, path, RideId::GetNull(), StationIndex::FromUnderlying(0));
        return false;
    }
    std::optional<State> Inspect(const RideManagement::Target& target)
    {
        if (!Supported(target))
            return {};
        auto point = [&](const TileCoordsXYZD& entrance, bool exit, StationIndex index) {
            auto outside = entrance.toCoordsXYZD();
            outside -= CoordsDirectionDelta[outside.direction];
            const auto* path = PathAt(outside, DirectionReverse(outside.direction));
            const bool attached = path
                && (exit ? !path->isQueue()
                         : path->isQueue() && path->getRideIndex() == target.id && path->getStationIndex() == index);
            return Point{ outside, attached, attached && (exit || QueueReachesNormal(outside, path, target.id, index)), index,
                          attached && ReachesParkEntrance(outside, path, target.id, index) };
        };
        std::optional<State> result;
        const auto* ride = RideManagement::Resolve(target);
        for (size_t i = 0; i < ride->getStations().size(); ++i)
        {
            const auto& station = ride->getStations()[i];
            if (station.start.isNull())
                continue;
            if (station.entrance.isNull() || station.exit.isNull())
                return {};
            const auto index = StationIndex::FromUnderlying(i);
            const auto entrance = point(station.entrance, false, index), exit = point(station.exit, true, index);
            if (!result)
                result = State{ entrance, exit };
            else
            {
                if (result->entrance.reachesParkEntrance && !entrance.reachesParkEntrance)
                    result->entrance = entrance;
                if (result->exit.reachesParkEntrance && !exit.reachesParkEntrance)
                    result->exit = exit;
            }
        }
        return result;
    }

    ManagementPresentation Presentation(const RideManagement::Target& target, size_t selected, const std::string& feedback)
    {
        ManagementPresentation model;
        model.breadcrumb = "Rides / Access";
        model.selected = selected;
        const auto* ride = RideManagement::Resolve(target);
        model.title = ride ? ride->getName() : "Ride unavailable";
        const auto state = Inspect(target);
        model.rows = { "Back to park" }; // Optional handoff never places a path or opens a ride by default.
        if (state)
        {
            model.stats = { { "Entrance " + std::to_string(state->entrance.station.ToUnderlying() + 1),
                              state->entrance.reachesParkEntrance     ? "Reaches park entrance"
                                  : state->entrance.reachesNormalPath ? "Needs park connection"
                                  : state->entrance.attached          ? "Extend queue to a path"
                                                                      : "Needs queue path" },
                            { "Exit " + std::to_string(state->exit.station.ToUnderlying() + 1),
                              state->exit.reachesParkEntrance ? "Reaches park entrance"
                                  : state->exit.attached      ? "Needs park connection"
                                                              : "Needs normal path" } };
            model.rows.insert(
                model.rows.end(),
                { state->entrance.reachesParkEntrance ? "Locate entrance / queue..." : "Connect entrance queue...",
                  state->exit.reachesParkEntrance ? "Locate exit / paths..." : "Connect exit path...", "Review opening...",
                  "Ride inspection...", "Connected entrance queue...", "Connected exit path..." });
            model.status = "Link both access points to park paths";
        }
        else if (FlatRides::CanResume(target))
        {
            model.rows = { "Place entrance / exit...", "Ride inspection...", "Back to park" };
            model.status = "Place missing access, then connect paths";
        }
        else
            model.status = "Missing access: close ride, then inspect again";
        if (!feedback.empty())
            model.status = feedback;
        return model;
    }
    bool PointValid(const RideManagement::Target& target, StationIndex index, bool exit, CoordsXYZD outside)
    {
        const auto* ride = RideManagement::Resolve(target);
        if (!ride || index.ToUnderlying() >= ride->getStations().size())
            return false;
        const auto& station = ride->getStations()[index.ToUnderlying()];
        const auto& entrance = exit ? station.exit : station.entrance;
        if (station.start.isNull() || entrance.isNull())
            return false;
        auto current = entrance.toCoordsXYZD();
        current -= CoordsDirectionDelta[current.direction];
        return current == outside && Supported(target);
    }
    bool BeginPaths(const RideManagement::Target& target, bool exit, bool connected)
    {
        const auto state = Inspect(target);
        auto* main = WindowGetMain();
        if (!state || !main || !main->viewport || ContextGetWidth() <= 0 || ContextGetHeight() <= 0
            || main->viewport->width <= 0 || main->viewport->height <= 0)
            return false;
        const auto point = exit ? state->exit : state->entrance;
        EndTerrain();
        if (connected)
        {
            if (!Paths::BeginConnectedAccess(target, point.station, exit, point.outside))
                return false;
        }
        else
            Paths::Begin(!exit);
        SetRidePathAccess(target, point.station, exit, point.outside);
        const auto* surface = MapGetSurfaceElementAt(point.outside);
        if (!surface)
        {
            EndTerrain();
            return false;
        }
        const bool elevated = point.outside.z != MapGetHighestZ(point.outside);
        Paths::UseHeightPlane(true, CoordsXY(point.outside) + CoordsDirectionDelta[point.outside.direction]);
        // Focus the native landing height; guided fixed-height targeting uses this plane.
        // Reuse the existing fixed-height Paths mode for the station landing tile.
        const CoordsXYZ focus{ point.outside.x + 16, point.outside.y + 16, point.outside.z };
        WindowScrollToLocation(*main, focus);
        // Snap to the native located view so a fresh A cannot act on the old camera target during scrolling.
        const auto cursor = ScreenCoordsXY{ ContextGetWidth() / 2, ContextGetHeight() / 2 } - main->viewport->pos;
        main->savedViewPos = Translate3DTo2DWithZ(main->viewport->rotation, focus)
            - ScreenCoordsXY{ main->viewport->zoom.ApplyTo(cursor.x), main->viewport->zoom.ApplyTo(cursor.y) };
        main->viewport->viewPos = main->savedViewPos;
        main->flags.unset(WindowFlag::scrollingToLocation);
        main->invalidate();
        if (elevated && !Paths::SetPlacementHeight(point.outside.z))
        {
            EndTerrain();
            return false;
        }
        Paths::UpdateTarget({ ContextGetWidth() / 2, ContextGetHeight() / 2 });
        return true;
    }
} // namespace OpenRCT2::Ui::Handheld::RideAccess

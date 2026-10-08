// SPDX-License-Identifier: GPL-3.0-only
#include "RideAccess.h"

#include "Explorer.h"
#include "Paths.h"

#include <openrct2-ui/interface/Window.h>
#include <openrct2/Context.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/ride/FlatRidePlacement.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <set>
#include <tuple>
namespace OpenRCT2::Ui::Handheld::RideAccess
{
    bool Supported(const RideManagement::Target& target)
    {
        const auto* ride = RideManagement::Resolve(target);
        return ride && FlatRidePlacement::Type(ride->subtype) == ride->type
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
    static bool QueueReachesNormal(CoordsXYZ start, const PathElement* first, RideId ride)
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
            if (path->getRideIndex() != ride || path->getStationIndex() != StationIndex::FromUnderlying(0))
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
    std::optional<State> Inspect(const RideManagement::Target& target)
    {
        if (!Supported(target))
            return {};
        const auto& station = RideManagement::Resolve(target)->getStation(StationIndex::FromUnderlying(0));
        if (station.entrance.isNull() || station.exit.isNull())
            return {};
        auto point = [&](const TileCoordsXYZD& entrance, bool exit) {
            auto outside = entrance.toCoordsXYZD();
            outside -= CoordsDirectionDelta[outside.direction];
            const auto* path = PathAt(outside, DirectionReverse(outside.direction));
            const bool attached = path
                && (exit ? !path->isQueue()
                         : path->isQueue() && path->getRideIndex() == target.id
                            && path->getStationIndex() == StationIndex::FromUnderlying(0));
            return Point{ outside, attached, attached && (exit || QueueReachesNormal(outside, path, target.id)) };
        };
        return State{ point(station.entrance, false), point(station.exit, true) };
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
            model.stats = { { "Entrance",
                              state->entrance.reachesNormalPath ? "Queue joins normal path"
                                  : state->entrance.attached    ? "Extend queue to a path"
                                                                : "Needs queue path" },
                            { "Exit", state->exit.attached ? "Normal path attached" : "Needs normal path" } };
            model.rows.insert(
                model.rows.end(),
                { state->entrance.reachesNormalPath ? "Locate entrance / queue..." : "Connect entrance queue...",
                  state->exit.attached ? "Locate exit / paths..." : "Connect exit path...", "Review opening...",
                  "Ride inspection..." });
            model.status = "Link both access points to park paths";
        }
        else
            model.status = "Finish entrances/exits from ride inspection";
        if (!feedback.empty())
            model.status = feedback;
        return model;
    }
    bool BeginPaths(const RideManagement::Target& target, bool exit)
    {
        const auto state = Inspect(target);
        auto* main = WindowGetMain();
        if (!state || !main || !main->viewport || ContextGetWidth() <= 0 || ContextGetHeight() <= 0
            || main->viewport->width <= 0 || main->viewport->height <= 0)
            return false;
        const auto point = exit ? state->exit : state->entrance;
        EndTerrain();
        Paths::Begin(!exit);
        SetRidePathAccess(target);
        const CoordsXYZ focus{ point.outside.x + 16, point.outside.y + 16, point.outside.z };
        WindowScrollToLocation(*main, focus);
        // Snap to the native located view so a fresh A cannot act on the old camera target during scrolling.
        const auto cursor = ScreenCoordsXY{ ContextGetWidth() / 2, ContextGetHeight() / 2 } - main->viewport->pos;
        main->savedViewPos = Translate3DTo2DWithZ(main->viewport->rotation, focus)
            - ScreenCoordsXY{ main->viewport->zoom.ApplyTo(cursor.x), main->viewport->zoom.ApplyTo(cursor.y) };
        main->viewport->viewPos = main->savedViewPos;
        main->flags.unset(WindowFlag::scrollingToLocation);
        main->invalidate();
        return true;
    }
} // namespace OpenRCT2::Ui::Handheld::RideAccess

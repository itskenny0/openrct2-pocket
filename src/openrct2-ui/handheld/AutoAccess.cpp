// SPDX-License-Identifier: GPL-3.0-only
#include "AutoAccess.h"

#include "../windows/Windows.h"

#include <algorithm>
#include <deque>
#include <map>
#include <memory>
#include <openrct2/GameState.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/ride/FlatRideEntrancePlaceAction.h>
#include <openrct2/ride/FlatRidePlacement.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <utility>

namespace OpenRCT2::Ui::Handheld::AutoAccess
{
    namespace
    {
        struct Side
        {
            CoordsXYZD location; // the entrance tile, facing the station
            CoordsXYZ outside;   // where its path starts
        };
        struct Walk
        {
            std::vector<CoordsXYZ> tiles;
            bool connects{};
        };

        constexpr int kSearchRadius = 16;
        constexpr size_t kMaxNodes = 900;

        struct Surface
        {
            ObjectEntryIndex type{ kObjectEntryIndexNull };
            PathConstructFlags flags{};
        };
        Surface PathSurface(bool queue)
        {
            Windows::WindowFootpathSelectDefault();
            Surface s{ queue ? gFootpathSelection.queueSurface : gFootpathSelection.normalSurface,
                       queue ? PathConstructFlag::IsQueue : PathConstructFlags{} };
            if (gFootpathSelection.legacyPath != kObjectEntryIndexNull)
            {
                s.type = gFootpathSelection.legacyPath;
                s.flags |= PathConstructFlag::IsLegacyPathObject;
            }
            return s;
        }
        GameActions::FootpathPlaceAction PathAction(CoordsXYZ at, bool queue)
        {
            const auto surface = PathSurface(queue);
            return GameActions::FootpathPlaceAction(
                at, FootpathSlope{}, surface.type, gFootpathSelection.railings, kInvalidDirection, surface.flags);
        }

        const PathElement* PathAt(CoordsXYZ at)
        {
            for (const auto* path : TileElementsView<PathElement>(at))
                if (!path->isGhost() && path->getBaseZ() == at.z)
                    return path;
            return nullptr;
        }

        bool Contains(const std::vector<CoordsXY>& tiles, CoordsXY tile)
        {
            return std::find(tiles.begin(), tiles.end(), tile) != tiles.end();
        }

        // Shortest level walk from start to a tile beside an existing (non-queue) path, through
        // tiles where a flat path could be built. Queries are cached per tile: the same tile is
        // asked about from up to four neighbours.
        std::optional<Walk> Route(CoordsXYZ start, const std::vector<CoordsXY>& blocked, bool queue, money64& cost)
        {
            if (Contains(blocked, start))
                return std::nullopt;
            auto& state = getGameState();
            std::map<std::pair<int32_t, int32_t>, std::optional<money64>> placeable;
            auto price = [&](CoordsXY tile) -> std::optional<money64> {
                const auto key = std::make_pair(tile.x, tile.y);
                if (const auto found = placeable.find(key); found != placeable.end())
                    return found->second;
                std::optional<money64> result;
                if (MapIsLocationValid(tile) && !PathAt({ tile, start.z }))
                {
                    auto action = PathAction({ tile, start.z }, queue);
                    const auto quote = GameActions::Query(&action, state);
                    if (quote.error == GameActions::Status::ok)
                        result = quote.cost;
                }
                placeable[key] = result;
                return result;
            };
            auto goal = [&](CoordsXY tile) {
                const auto* path = PathAt({ tile, start.z });
                return path && !path->isQueue() && !path->isSloped();
            };

            // Already standing on a path: nothing to build.
            if (goal(start))
                return Walk{ {}, true };
            if (!price(start))
                return std::nullopt;

            std::vector<CoordsXY> nodes{ start };
            std::vector<int> parent{ -1 };
            std::vector<int> depth{ 0 };
            std::deque<int> frontier{ 0 };
            while (!frontier.empty() && nodes.size() < kMaxNodes)
            {
                const int current = frontier.front();
                frontier.pop_front();
                const auto here = nodes[current];
                // Fixed neighbour order keeps the plan deterministic.
                for (Direction d = 0; d < kNumOrthogonalDirections; ++d)
                {
                    const CoordsXY next = here + CoordsDirectionDelta[d];
                    if (goal(next))
                    {
                        Walk walk{ {}, true };
                        for (int i = current; i >= 0; i = parent[i])
                            walk.tiles.insert(walk.tiles.begin(), { nodes[i], start.z });
                        for (const auto& tile : walk.tiles)
                            cost += price(tile).value_or(0);
                        return walk;
                    }
                }
                if (depth[current] + 1 >= kMaxWalk)
                    continue;
                for (Direction d = 0; d < kNumOrthogonalDirections; ++d)
                {
                    const CoordsXY next = here + CoordsDirectionDelta[d];
                    if (Contains(blocked, next) || Contains(nodes, next) || !price(next))
                        continue;
                    nodes.push_back(next);
                    parent.push_back(current);
                    depth.push_back(depth[current] + 1);
                    frontier.push_back(static_cast<int>(nodes.size() - 1));
                }
            }
            return std::nullopt;
        }

        std::vector<Side> Sides(const Ride& ride, StationIndex index, bool exit)
        {
            std::vector<Side> sides;
            const auto& station = ride.getStation(index);
            if (station.start.isNull())
                return sides;
            auto& state = getGameState();
            const TileCoordsXY middle(station.start);
            for (int dy = -kSearchRadius; dy <= kSearchRadius; ++dy)
                for (int dx = -kSearchRadius; dx <= kSearchRadius; ++dx)
                {
                    const auto tile = TileCoordsXY{ middle.x + dx, middle.y + dy }.toCoordsXY();
                    const auto location = FlatRidePlacement::EntranceLocation(ride, tile, index);
                    if (!location)
                        continue;
                    // An exit can only be checked once the entrance exists; until then the
                    // entrance's own check stands in for it (same tile, same clearance).
                    GameActions::FlatRideEntrancePlaceAction action(
                        ride.id, *location, exit && !station.entrance.isNull(), index);
                    if (GameActions::Query(&action, state).error != GameActions::Status::ok)
                        continue;
                    const CoordsXY outside = CoordsXY(*location) - CoordsDirectionDelta[location->direction];
                    sides.push_back({ *location, { outside, location->z } });
                }
            return sides;
        }

        money64 EntranceCost(const Ride& ride, StationIndex index, const Side& side, bool exit)
        {
            const bool asExit = exit && !ride.getStation(index).entrance.isNull();
            GameActions::FlatRideEntrancePlaceAction action(ride.id, side.location, asExit, index);
            return GameActions::Query(&action, getGameState()).cost;
        }

        // The best side for one end: the shortest connected walk wins; without any walk, the side
        // closest to the station's start (so entrance and exit at least end up somewhere tidy).
        struct Choice
        {
            Side side;
            Walk walk;
            money64 cost{};
        };
        std::optional<Choice> Choose(
            const std::vector<Side>& sides, const std::vector<CoordsXY>& blocked, bool queue, bool avoidNeighbours,
            const std::vector<CoordsXYZ>& otherWalk)
        {
            std::optional<Choice> best;
            size_t bestScore = SIZE_MAX;
            for (const auto& side : sides)
            {
                if (Contains(blocked, side.location) || Contains(blocked, side.outside))
                    continue;
                auto walkBlocked = blocked;
                if (avoidNeighbours)
                    for (const auto& tile : otherWalk)
                        for (Direction d = 0; d < kNumOrthogonalDirections; ++d)
                            walkBlocked.push_back(CoordsXY(tile) + CoordsDirectionDelta[d]);
                money64 cost = 0;
                const auto walk = Route(side.outside, walkBlocked, queue, cost);
                const size_t score = walk ? walk->tiles.size() : 1000;
                if (score < bestScore)
                {
                    bestScore = score;
                    best = Choice{ side, walk.value_or(Walk{}), cost };
                }
            }
            return best;
        }
    } // namespace

    std::vector<CoordsXY> Plan::Tiles() const
    {
        std::vector<CoordsXY> tiles;
        auto add = [&](CoordsXY t) {
            if (!Contains(tiles, t))
                tiles.push_back(t);
        };
        if (placeEntrance)
            add(entrance);
        if (placeExit)
            add(exit);
        for (const auto& t : queue)
            add(t);
        for (const auto& t : path)
            add(t);
        return tiles;
    }

    std::optional<Plan> Make(const RideManagement::Target& target, StationIndex index, std::string& error)
    {
        const auto* ride = RideManagement::Resolve(target);
        if (!ride || index.ToUnderlying() >= ride->getStations().size())
        {
            error = "Ride no longer available";
            return std::nullopt;
        }
        const auto& station = ride->getStation(index);
        Plan plan;
        plan.placeEntrance = station.entrance.isNull();
        plan.placeExit = station.exit.isNull();
        if (!plan.placeEntrance && !plan.placeExit)
        {
            error = "This station already has an entrance and exit";
            return std::nullopt;
        }

        std::vector<CoordsXY> blocked;
        if (plan.placeEntrance)
        {
            const auto choice = Choose(Sides(*ride, index, false), blocked, true, false, {});
            if (!choice)
            {
                error = "No free side for an entrance";
                return std::nullopt;
            }
            plan.entrance = choice->side.location;
            plan.queue = choice->walk.tiles;
            plan.queueConnects = choice->walk.connects;
            plan.cost += choice->cost + EntranceCost(*ride, index, choice->side, false);
            blocked.push_back(choice->side.location);
            blocked.push_back(choice->side.outside);
            for (const auto& t : plan.queue)
                blocked.push_back(t);
        }
        if (plan.placeExit)
        {
            const auto sides = Sides(*ride, index, true);
            // Keep the exit path off the queue's sides if possible, so it cannot join the queue.
            auto choice = Choose(sides, blocked, false, true, plan.queue);
            if (choice && !choice->walk.connects)
                if (auto loose = Choose(sides, blocked, false, false, plan.queue); loose && loose->walk.connects)
                    choice = loose;
            if (!choice)
            {
                error = "No free side for an exit";
                return std::nullopt;
            }
            plan.exit = choice->side.location;
            plan.path = choice->walk.tiles;
            plan.pathConnects = choice->walk.connects;
            plan.cost += choice->cost + EntranceCost(*ride, index, choice->side, true);
        }
        return plan;
    }

    namespace
    {
        // Queue a group of actions; after runs once every one of them has been processed (at once
        // if none could be queued). Costs and the first refusal are added to total.
        void QueueGroup(
            std::vector<std::unique_ptr<GameActions::GameAction>> actions, std::function<void()> after,
            GameActions::Result& total)
        {
            auto& state = getGameState();
            auto pending = std::make_shared<size_t>(0);
            auto finished = std::make_shared<std::function<void()>>(std::move(after));
            auto settle = [pending, finished] {
                if (*pending == 0 && *finished)
                    std::exchange(*finished, nullptr)();
            };
            for (auto& action : actions)
            {
                action->SetCallback([pending, settle](const auto*, const GameActions::Result*) {
                    --*pending;
                    settle();
                });
                ++*pending;
                const auto result = GameActions::Execute(action.get(), state);
                if (result.error == GameActions::Status::ok)
                    total.cost += result.cost;
                else
                {
                    --*pending;
                    if (total.error == GameActions::Status::ok)
                    {
                        total.error = result.error;
                        total.errorTitle = result.errorTitle;
                        total.errorMessage = result.errorMessage;
                    }
                    // An entrance that cannot be placed makes its path pointless.
                    if (dynamic_cast<GameActions::FlatRideEntrancePlaceAction*>(action.get()))
                        break;
                }
            }
            settle();
        }
    } // namespace

    GameActions::Result Execute(
        const Plan& plan, const RideManagement::Target& target, StationIndex index, std::function<void()> done)
    {
        GameActions::Result total;
        // The exit is placed in a second group, once the entrance exists: the entrance action
        // records the station as it was when it was made, so an exit made alongside it would be
        // refused as out of date. Within a group, each entrance comes before its path, so the
        // first path tile joins the entrance as it is laid.
        auto exitGroup = [plan, target, index, done]() {
            // No entrance after all (refused, or the ride changed meanwhile): stop here.
            const auto* ride = RideManagement::Resolve(target);
            if (!plan.placeExit || !ride || ride->getStation(index).entrance.isNull())
            {
                if (done)
                    done();
                return;
            }
            std::vector<std::unique_ptr<GameActions::GameAction>> actions;
            actions.push_back(std::make_unique<GameActions::FlatRideEntrancePlaceAction>(target.id, plan.exit, true, index));
            for (const auto& tile : plan.path)
                actions.push_back(std::make_unique<GameActions::FootpathPlaceAction>(PathAction(tile, false)));
            GameActions::Result ignored;
            QueueGroup(std::move(actions), done, ignored);
        };
        if (!plan.placeEntrance)
        {
            exitGroup();
            total.cost = plan.cost;
            return total;
        }
        std::vector<std::unique_ptr<GameActions::GameAction>> actions;
        actions.push_back(std::make_unique<GameActions::FlatRideEntrancePlaceAction>(target.id, plan.entrance, false, index));
        for (const auto& tile : plan.queue)
            actions.push_back(std::make_unique<GameActions::FootpathPlaceAction>(PathAction(tile, true)));
        QueueGroup(std::move(actions), exitGroup, total);
        return total;
    }

    std::string Summary(const Plan& plan)
    {
        std::string text = plan.placeEntrance && plan.placeExit ? "Entrance and exit placed"
            : plan.placeEntrance                                ? "Entrance placed"
                                                                : "Exit placed";
        const bool queueOk = !plan.placeEntrance || plan.queueConnects;
        const bool pathOk = !plan.placeExit || plan.pathConnects;
        if (queueOk && pathOk)
            return text + ", joined to the path";
        if (!queueOk && !pathOk)
            return text + "; no level route to a path";
        return text + (queueOk ? "; exit needs a path" : "; queue needs a path");
    }
} // namespace OpenRCT2::Ui::Handheld::AutoAccess

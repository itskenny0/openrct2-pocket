// SPDX-License-Identifier: GPL-3.0-only
#include "AccessPreview.h"

#include <openrct2/actions/ride/RideEntranceExitRemoveAction.h>
#include <openrct2/ride/AccessPlacementContext.h>
#include <openrct2/world/tile_element/EntranceElement.h>
namespace OpenRCT2::Ui::Handheld::AccessPreview
{
    struct Request
    {
        RideManagement::Target target;
        CoordsXYZD location;
        bool exit;
        StationIndex station;
        bool relocation{};
        std::vector<int64_t> context;
        money64 cost;
    };
    static std::optional<Request> requested, ghost;
    static uint32_t depth{};
    static void Remove()
    {
        if (ghost)
        {
            auto action = GameActions::RideEntranceExitRemoveAction(
                ghost->location, ghost->target.id, ghost->station, ghost->exit);
            action.SetPureGhost();
            action.SetFlags(
                { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
                  GameActions::CommandFlag::allowDuringPaused });
            // Ghost-only native cleanup also works after closure/identity changes.
            action.Execute(getGameState(), getGameState().park);
        }
        ghost.reset();
    }
    void Clear()
    {
        requested.reset();
        Remove();
    }
    void Update(const RideManagement::Target& target, CoordsXYZD location, bool exit, StationIndex station, bool relocation)
    {
        const auto* ride = RideManagement::Resolve(target);
        if (!ride)
        {
            Clear();
            return;
        }
        auto context = AccessPlacementContext::Capture(*ride, location);
        GameActions::FlatRideEntrancePlaceAction action(target.id, location, exit, station);
        if (relocation)
            action.EditExisting();
        auto q = GameActions::Query(&action, getGameState());
        if (q.error != GameActions::Status::ok)
        {
            Clear();
            return;
        }
        Request next{ target, location, exit, station, relocation, context, q.cost };
        requested.emplace(next);
        if (depth)
            return;
        if (ghost && ghost->location == location && ghost->target == target && ghost->exit == exit && ghost->station == station
            && ghost->context == context)
            return;
        Remove();
        action.GuardContext(context, q.cost);
        action.SetFlags(
            { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
              GameActions::CommandFlag::allowDuringPaused });
        if (GameActions::Execute(&action, getGameState()).error == GameActions::Status::ok)
            ghost.emplace(std::move(next));
    }
    void Suspend()
    {
        ++depth;
        Remove();
    }
    void Restore()
    {
        if (!depth || --depth || !requested)
            return;
        const auto old = *requested;
        const auto* ride = RideManagement::Resolve(old.target);
        if (!ride || old.context != AccessPlacementContext::Capture(*ride, old.location))
        {
            Clear();
            return;
        }
        Update(old.target, old.location, old.exit, old.station, old.relocation);
    }
    GameActions::Result Bind(GameActions::FlatRideEntrancePlaceAction& action, CoordsXYZD location)
    {
        if (requested && requested->location == location)
        {
            action.GuardContext(requested->context, requested->cost);
        }
        return GameActions::Query(&action, getGameState());
    }
} // namespace OpenRCT2::Ui::Handheld::AccessPreview

// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/GameState.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/terraform/WaterLowerAction.h>
#include <openrct2/actions/terraform/WaterRaiseAction.h>
#include <optional>

namespace OpenRCT2::Ui::Handheld::Water
{
    // The single target is shared with the terrain explorer. Native actions own
    // water rules, validation, pricing and queue execution.
    inline GameActions::Result Action(std::optional<CoordsXY> tile, bool raise, bool execute)
    {
        if (!tile)
        {
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = "Move onto land or water.";
            return result;
        }
        const MapRange range{ tile->x, tile->y, tile->x, tile->y };
        if (raise)
        {
            auto action = GameActions::WaterRaiseAction(range);
            return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
        }
        auto action = GameActions::WaterLowerAction(range);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
} // namespace OpenRCT2::Ui::Handheld::Water

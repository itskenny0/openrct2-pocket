// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"

#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/scenario/ScenarioObjective.h>

namespace OpenRCT2::Ui::Handheld::ParkOverview
{
    struct Draft
    {
        bool baselineOpen;
        bool open;
        bool noMoney;
        Scenario::Objective objective;
        money64 outcome;
    };
    bool StatusPermitted();
    std::optional<Draft> Begin();
    GameActions::Result Action(const Draft& draft, bool execute, GameActions::GameAction::Callback_t callback = {});
    std::string ObjectiveText();
    std::string Outcome();
    ManagementPresentation Presentation(
        size_t selected, const std::optional<Draft>& draft = {}, const std::string& feedback = {});
} // namespace OpenRCT2::Ui::Handheld::ParkOverview

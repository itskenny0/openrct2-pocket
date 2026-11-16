// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"

#include <openrct2/actions/GameActionRunner.h>

namespace OpenRCT2::Ui::Handheld::Finances
{
    enum class Field
    {
        loan,
        admission
    };
    enum class View
    {
        overview,
        editor,
        confirmation
    };
    struct Draft
    {
        Field field;
        money64 baseline;
        money64 value;
    };
    bool Available(Field field);
    Draft Begin(Field field);
    void Adjust(Draft& draft, int direction);
    GameActions::Result Action(const Draft& draft, bool execute, GameActions::GameAction::Callback_t callback = {});
    std::string Feedback(const GameActions::Result& result);
    std::pair<money64, money64> MonthlyTotals();
    ManagementPresentation Presentation(
        View view, size_t selected, const std::optional<Draft>& draft = {}, const std::string& status = {});
} // namespace OpenRCT2::Ui::Handheld::Finances

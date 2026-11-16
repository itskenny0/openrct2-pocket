// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"

#include <openrct2/actions/GameActionRunner.h>

namespace OpenRCT2::Ui::Handheld::ResearchManagement
{
    enum class View
    {
        overview,
        editor,
        confirmation
    };
    struct Draft
    {
        uint8_t baselineFunding{}, baselinePriorities{}, funding{}, priorities{};
    };
    bool FundingAvailable();
    uint8_t AvailableCategories();
    Draft Begin();
    void AdjustFunding(Draft& draft, int direction);
    void ToggleCategory(Draft& draft, size_t category);
    GameActions::Result Action(const Draft& draft, bool execute, GameActions::GameAction::Callback_t callback = {});
    ManagementPresentation Presentation(
        View view, size_t selected, const std::optional<Draft>& draft = {}, const std::string& status = {});
} // namespace OpenRCT2::Ui::Handheld::ResearchManagement

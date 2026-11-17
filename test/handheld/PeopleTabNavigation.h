// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/ManagementPanel.h>
#include <openrct2-ui/windows/Windows.h>
#include <stdexcept>
inline size_t FocusPersonAction(bool staff, const char* label)
{
    namespace W = OpenRCT2::Ui::Windows;
    for (int tab = 0; tab < 4; ++tab)
    {
        const auto model = staff ? W::HandheldStaffPresentation() : W::HandheldGuestsPresentation();
        const auto found = std::find(model.rows.begin(), model.rows.end(), label);
        if (found != model.rows.end())
        {
            const auto row = static_cast<size_t>(found - model.rows.begin());
            W::HandheldMenuMove(static_cast<int>(row) - static_cast<int>(model.selected));
            return row;
        }
        W::HandheldDashboardPage(1);
    }
    throw std::runtime_error(std::string("Missing person action: ") + label);
}

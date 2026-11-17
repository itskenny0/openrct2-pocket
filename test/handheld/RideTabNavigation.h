// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/windows/Windows.h>
#include <stdexcept>
#include <string>

// Test navigation goes through the production dispatcher and presentation, never
// directly edits UI state or simulation state. The tab driver separately tests axes.
inline size_t FocusRideTabAction(const std::string& label)
{
    namespace W = OpenRCT2::Ui::Windows;
    for (int n = 0; n < 4; ++n)
    {
        const auto model = W::HandheldRidesPresentation();
        const auto it = std::find(model.rows.begin(), model.rows.end(), label);
        if (it != model.rows.end())
        {
            const auto index = static_cast<size_t>(it - model.rows.begin());
            W::HandheldMenuMove(static_cast<int>(index) - static_cast<int>(model.selected));
            return index;
        }
        W::HandheldDashboardPage(1);
    }
    throw std::runtime_error("Ride tab action missing: " + label);
}

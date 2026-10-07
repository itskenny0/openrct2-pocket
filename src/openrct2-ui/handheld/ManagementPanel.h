// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Panel.h"

#include <optional>

namespace OpenRCT2::Ui::Handheld
{
    // Shared navigation/presentation shell; sections supply data and native actions.
    struct ManagementPresentation
    {
        std::string breadcrumb;
        std::string title;
        std::vector<std::pair<std::string, std::string>> stats;
        std::vector<std::string> rows;
        size_t selected{};
        std::optional<size_t> highlightedStat;
        std::string status;
        bool editing{};
        bool confirmation{};
    };
    inline void DrawManagementPanel(Drawing::RenderTarget& rt, ScreenSize screen, const ManagementPresentation& model)
    {
        using namespace Concept;
        constexpr int width = 244;
        const auto p = kOrigin;
        const int rowStart = 70 + static_cast<int>(model.stats.size()) * 24;
        const size_t capacity = std::max(1, (338 - rowStart - 30) / 30);
        const size_t first = model.selected / capacity * capacity;
        const size_t visible = std::min(capacity, model.rows.size() - std::min(first, model.rows.size()));
        const int height = std::min(338, rowStart + static_cast<int>(visible) * 30 + 30);
        Box(rt, p, { width, height });
        DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, 12 }, width - 20, "{MUTED}" + model.breadcrumb);
        DrawStrongText(rt, p + ScreenCoordsXY{ 10, 32 }, "{WHITE}" + model.title, width - 20);
        Rule(rt, p + ScreenCoordsXY{ 10, 54 }, width - 20);
        for (size_t i = 0; i < model.stats.size(); ++i)
        {
            const int y = 63 + static_cast<int>(i) * 24;
            const bool highlight = model.highlightedStat == i;
            if (highlight)
                Box(rt, p + ScreenCoordsXY{ 8, y - 3 }, { width - 16, 23 }, true);
            const auto colour = highlight ? "{BLACK}" : "{MUTED}";
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, y + 4 }, 104, std::string(colour) + model.stats[i].first);
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ 118, y + 4 }, width - 128,
                std::string(highlight ? "{BLACK}" : "{WHITE}") + model.stats[i].second);
        }
        for (size_t i = first; i < first + visible; ++i)
        {
            const int y = rowStart + static_cast<int>(i - first) * 30;
            const bool selected = i == model.selected;
            if (selected)
                Box(rt, p + ScreenCoordsXY{ 8, y }, { width - 16, 28 }, true);
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ 15, y + 9 }, width - 30, std::string(selected ? "{BLACK}" : "{WHITE}") + model.rows[i]);
        }
        const auto status = model.status.empty() && model.rows.size() > capacity ? "Page "
                + std::to_string(first / capacity + 1) + " / " + std::to_string((model.rows.size() + capacity - 1) / capacity)
                                                                                 : model.status;
        if (!status.empty())
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, height - 16 }, width - 20, "{MUTED}" + status);
        DrawActionStrip(rt, screen);
        const int y = screen.height - 21;
        DrawCommand(rt, { 12, y }, 38, "D-pad", model.editing ? "Adjust" : "Choose");
        DrawCommand(rt, { 190, y }, 17, "A", model.editing ? "Review" : model.confirmation ? "Confirm" : "Select");
        DrawCommand(rt, { 370, y }, 17, "B", "Back");
    }
} // namespace OpenRCT2::Ui::Handheld

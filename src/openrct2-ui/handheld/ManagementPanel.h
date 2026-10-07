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
        const int height = std::min(338, rowStart + static_cast<int>(model.rows.size()) * 30 + 30);
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
        for (size_t i = 0; i < model.rows.size(); ++i)
        {
            const int y = rowStart + static_cast<int>(i) * 30;
            const bool selected = i == model.selected;
            if (selected)
                Box(rt, p + ScreenCoordsXY{ 8, y }, { width - 16, 28 }, true);
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ 15, y + 9 }, width - 30, std::string(selected ? "{BLACK}" : "{WHITE}") + model.rows[i]);
        }
        if (!model.status.empty())
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, height - 16 }, width - 20, "{MUTED}" + model.status);
        DrawActionStrip(rt, screen);
        const int y = screen.height - 21;
        DrawCommand(rt, { 12, y }, 38, "D-pad", model.editing ? "Adjust" : "Choose");
        DrawCommand(rt, { 190, y }, 17, "A", model.editing ? "Review" : model.confirmation ? "Confirm" : "Select");
        DrawCommand(rt, { 370, y }, 17, "B", "Back");
    }
} // namespace OpenRCT2::Ui::Handheld

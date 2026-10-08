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
        std::string primaryAction;
        bool editing{};
        bool confirmation{};
        std::vector<std::string> bodyLines; // Optional native wrapped read-only detail; section controls paging.
        size_t columns = 1;                 // Controller text entry uses the same content shell.
    };
    inline void DrawManagementContent(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, const ManagementPresentation& model, bool focused = true)
    {
        using namespace Concept;
        const int width = size.width;
        const int valueX = 14 + (width - 30) * 45 / 100;
        const int rowStart = 70 + static_cast<int>(model.stats.size()) * 24 + static_cast<int>(model.bodyLines.size()) * 16;
        const auto columns = std::max<size_t>(1, model.columns);
        const int rowHeight = columns > 1 ? 24 : 30;
        const size_t capacity = std::max(1, (size.height - rowStart - 30) / rowHeight) * columns;
        const size_t first = model.selected / capacity * capacity;
        const size_t visible = std::min(capacity, model.rows.size() - std::min(first, model.rows.size()));
        const int height = std::min(
            size.height, rowStart + static_cast<int>((visible + columns - 1) / columns) * rowHeight + 30);
        Box(rt, p, { width, height });
        DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, 12 }, width - 20, "{MUTED}" + model.breadcrumb);
        DrawStrongText(
            rt, p + ScreenCoordsXY{ 10, 32 }, std::string(model.confirmation ? "{GOLD}" : "{WHITE}") + model.title, width - 20);
        Rule(rt, p + ScreenCoordsXY{ 10, 54 }, width - 20);
        for (size_t i = 0; i < model.stats.size(); ++i)
        {
            const int y = 63 + static_cast<int>(i) * 24;
            const bool highlight = focused && model.highlightedStat == i;
            if (highlight)
                Box(rt, p + ScreenCoordsXY{ 8, y - 3 }, { width - 16, 23 }, true);
            const auto colour = highlight ? "{BLACK}" : "{MUTED}";
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, y + 4 }, valueX - 20, std::string(colour) + model.stats[i].first);
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ valueX, y + 4 }, width - valueX - 10,
                std::string(highlight ? "{BLACK}" : "{WHITE}") + model.stats[i].second);
        }
        for (size_t i = 0; i < model.bodyLines.size(); ++i)
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ 10, 67 + static_cast<int>(model.stats.size()) * 24 + static_cast<int>(i) * 16 },
                width - 20, "{WHITE}" + model.bodyLines[i]);
        for (size_t i = first; i < first + visible; ++i)
        {
            const int y = rowStart + static_cast<int>((i - first) / columns) * rowHeight;
            const int cellWidth = (width - 16) / columns;
            const int x = 8 + static_cast<int>((i - first) % columns) * cellWidth;
            const bool selected = focused && i == model.selected;
            if (selected)
                Box(rt, p + ScreenCoordsXY{ x, y }, { cellWidth, rowHeight - 2 }, true);
            const auto& label = model.rows[i];
            const bool unavailable = label.find("unavailable") != std::string::npos
                || label.find("Unavailable") != std::string::npos;
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ x + (columns > 1 ? 3 : 7), y + (columns > 1 ? 6 : 9) },
                cellWidth - (columns > 1 ? 6 : 14),
                std::string(
                    selected          ? "{BLACK}"
                        : unavailable ? "{MUTED}"
                                      : "{WHITE}")
                    + label);
        }
        const auto status = model.status.empty() && model.rows.size() > capacity ? "Page "
                + std::to_string(first / capacity + 1) + " / " + std::to_string((model.rows.size() + capacity - 1) / capacity)
                                                                                 : model.status;
        if (!status.empty())
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, height - 16 }, width - 20, "{MUTED}" + status);
    }
    // Compatibility wrapper for construction-era captures; Dashboard uses the content region directly.
    inline void DrawManagementPanel(Drawing::RenderTarget& rt, ScreenSize screen, const ManagementPresentation& model)
    {
        DrawManagementContent(rt, Concept::kMenuOrigin, { 244, Layout::ContentHeight(screen) }, model);
        DrawActionStrip(rt, screen);
        const int y = screen.height - 21;
        DrawCommand(rt, { 12, y }, 38, "D-pad", model.editing ? "Adjust" : "Choose");
        DrawCommand(rt, { 190, y }, 17, "A", model.editing ? "Review" : model.confirmation ? "Confirm" : "Select");
        DrawCommand(rt, { 370, y }, 17, "B", "Back");
    }
} // namespace OpenRCT2::Ui::Handheld

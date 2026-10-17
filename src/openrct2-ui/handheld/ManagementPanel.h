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
        std::string contextualAction;
        bool editing{};
        bool confirmation{};
        std::vector<std::string> bodyLines; // Optional native wrapped read-only detail; section controls paging.
        std::vector<int32_t> graph{};
        std::string graphRange;
        // Presentation metadata never changes native action indices.
        bool summaryMetrics{};
        bool actionGrid{};
        bool rosterCards{};
        std::vector<std::string> rowDetails;
        std::vector<bool> rowEnabled;
        std::vector<std::string> tabs;
        size_t selectedTab{};
        std::string headerStatus;
        int metricHeight = 48;
        bool contentSized{};
        size_t columns = 1; // Controller text entry uses the same content shell.
    };
    struct ManagementLayout
    {
        int columns, rowHeight, rowStart, statColumns, statHeight;
        size_t capacity;
    };
    inline ManagementLayout MeasureManagement(ScreenSize size, const ManagementPresentation& model)
    {
        const int columns = model.actionGrid ? (size.width >= 360 ? 2 : 1) : static_cast<int>(std::max<size_t>(1, model.columns));
        const int statColumns = model.summaryMetrics && !model.stats.empty() ? (size.width >= 440 ? 3 : 2) : 1;
        const int statHeight = model.summaryMetrics ? model.metricHeight : 24;
        const int statRows = (static_cast<int>(model.stats.size()) + statColumns - 1) / statColumns;
        const int rowStart = 64 + (model.tabs.empty() ? 0 : 34) + statRows * statHeight + static_cast<int>(model.bodyLines.size()) * 16
            + (model.graph.empty() ? 0 : 110) + (model.rows.empty() ? 0 : 11);
        const int rowHeight = model.rosterCards ? 46 : model.actionGrid ? 30 : columns > 1 ? 24 : 30;
        const size_t capacity = std::max(1, (size.height - rowStart - 30) / rowHeight) * columns;
        return {columns, rowHeight, rowStart, statColumns, statHeight, capacity};
    }
    inline void DrawManagementContent(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, const ManagementPresentation& model, bool focused = true)
    {
        if (model.confirmation || model.contentSized)
        {
            const auto measured = MeasureManagement(size, model);
            const int rows = static_cast<int>((model.rows.size() + measured.columns - 1) / measured.columns);
            size.height = std::min(size.height, measured.rowStart + rows * measured.rowHeight + 34);
        }
        const int width = size.width;
        const auto layout = MeasureManagement(size, model);
        const auto [columns, rowHeight, rowStart, statColumns, statHeight, capacity] = layout;
        const size_t first = model.selected / capacity * capacity;
        const size_t visible = std::min(capacity, model.rows.size() - std::min(first, model.rows.size()));
        DrawSurface(rt, p, size);
        DrawSectionLabel(rt, p + ScreenCoordsXY{12, 13}, width - 24 - (model.headerStatus.empty() ? 0 : 138), model.breadcrumb);
        if (!model.headerStatus.empty()) DrawStrongText(rt,p+ScreenCoordsXY{width-138,13},"{WHITE}"+model.headerStatus,126);
        const int contentTop = model.tabs.empty() ? 64 : 98;
        if (!model.tabs.empty()) DrawTabs(rt,p+ScreenCoordsXY{10,62},width-20,model.tabs,model.selectedTab);
        DrawHeadingText(rt, p + ScreenCoordsXY{12, 29},
            std::string(model.confirmation ? "{GOLD}" : "{WHITE}") + model.title, width - 24);
        Fill(rt, p + ScreenCoordsXY{12, 55}, {width - 24, 1}, Theme::edge);
        const int valueX = 14 + (width - 30) * 45 / 100;
        for (size_t i = 0; i < model.stats.size(); ++i)
        {
            const int y = contentTop + static_cast<int>(i / statColumns) * statHeight;
            const bool highlight = focused && model.highlightedStat == i;
            if (model.summaryMetrics)
            {
                const int cellWidth = (width - 24) / statColumns;
                DrawMetric(rt, p + ScreenCoordsXY{12 + static_cast<int>(i % statColumns) * cellWidth, y},
                    {cellWidth - 4, statHeight - 5}, model.stats[i].first, model.stats[i].second, highlight);
            }
            else
            {
                if (highlight) Fill(rt, p + ScreenCoordsXY{8, y}, {width - 16, 23}, Theme::gold);
                DrawModernEllipsised(rt, p + ScreenCoordsXY{12, y + 6}, valueX - 24,
                    std::string(highlight ? "{BLACK}" : "{MUTED}") + model.stats[i].first);
                DrawModernEllipsised(rt, p + ScreenCoordsXY{valueX, y + 6}, width - valueX - 12,
                    std::string(highlight ? "{BLACK}" : "{WHITE}") + model.stats[i].second);
            }
        }
        const int statRows = (static_cast<int>(model.stats.size()) + statColumns - 1) / statColumns;
        for (size_t i = 0; i < model.bodyLines.size(); ++i)
            DrawModernEllipsised(rt, p + ScreenCoordsXY{12, contentTop + 4 + statRows * statHeight + static_cast<int>(i) * 16},
                width - 24, "{WHITE}" + model.bodyLines[i]);
        if (!model.graph.empty())
        {
            const auto [low, high] = std::minmax_element(model.graph.begin(), model.graph.end());
            const int range = std::max(1, *high - *low);
            const int plotBottom = rowStart - 35;
            const int plotWidth = width - 28;
            Fill(rt, p + ScreenCoordsXY{12, plotBottom}, {width - 24, 1}, Theme::edge);
            for (size_t i = 1; i < model.graph.size(); ++i)
            {
                const auto point = [&](size_t n) {
                    return p + ScreenCoordsXY{14 + static_cast<int>(n * plotWidth / (model.graph.size() - 1)),
                        plotBottom - (model.graph[n] - *low) * 76 / range};
                };
                GfxDrawLine(rt, {point(i - 1), point(i)}, Theme::gold);
            }
            DrawSectionLabel(rt, p + ScreenCoordsXY{12, plotBottom + 8}, width - 24, model.graphRange);
        }
        if (!model.rows.empty() && model.bodyLines.empty())
            DrawSectionLabel(rt, p + ScreenCoordsXY{12, rowStart - 12}, width - 24,
                model.confirmation ? "REVIEW & DECIDE" : model.rosterCards ? "CHOOSE A RIDE" : "ACTIONS");
        for (size_t i = first; i < first + visible; ++i)
        {
            const int y = rowStart + static_cast<int>((i - first) / columns) * rowHeight;
            const int cellWidth = (width - 20) / columns;
            const int x = 10 + static_cast<int>((i - first) % columns) * cellWidth;
            const bool enabled = i < model.rowEnabled.size() ? model.rowEnabled[i]
                : model.rows[i].find("unavailable") == std::string::npos && model.rows[i].find("Unavailable") == std::string::npos;
            if (columns > 2) // Native on-screen keyboard retains its compact cells.
            {
                if (focused && i == model.selected) Fill(rt, p + ScreenCoordsXY{x, y}, {cellWidth - 2, rowHeight - 2}, Theme::gold);
                DrawModernEllipsised(rt, p + ScreenCoordsXY{x + 3, y + 6}, cellWidth - 6,
                    std::string(focused && i == model.selected ? "{BLACK}" : "{WHITE}") + model.rows[i]);
            }
            else
                DrawActionRow(rt, p + ScreenCoordsXY{x, y}, {cellWidth - 4, rowHeight - 2}, model.rows[i],
                    i < model.rowDetails.size() ? model.rowDetails[i] : "", focused && i == model.selected, enabled);
        }
        // Feedback and overflow have independent lines, so failures never hide pagination.
        if (!model.status.empty())
            DrawModernEllipsised(rt, p + ScreenCoordsXY{12, size.height - 29}, width - 24, "{WHITE}" + model.status);
        DrawPagePosition(rt, p + ScreenCoordsXY{12, size.height - 13}, width - 24, model.selected, model.rows.size(), capacity);
        if (model.rows.size() > capacity)
        {
            const int trackHeight = std::max(8, size.height - rowStart - 40);
            Fill(rt, p + ScreenCoordsXY{width - 6, rowStart}, {2, trackHeight}, Theme::raised);
            const auto pages = (model.rows.size() + capacity - 1) / capacity;
            const int thumb = std::max(3, trackHeight / static_cast<int>(pages));
            Fill(rt, p + ScreenCoordsXY{width - 6, rowStart + static_cast<int>(model.selected / capacity) * (trackHeight - thumb) / static_cast<int>(pages - 1)},
                {2, thumb}, Theme::gold);
        }
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

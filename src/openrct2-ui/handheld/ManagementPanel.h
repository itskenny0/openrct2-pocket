// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Panel.h"
#include "ColourGrid.h"
#include "ListScroll.h"

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
        bool denseRoster{};
        size_t toolbarCount{};
        std::string listLabel;
        bool destructive{};
        std::string headerInk = "{WHITE}";
        std::vector<int> meterDirections; // +1 higher is better; -1 lower is better.
        std::vector<bool> meterAttention; // Native thought/attention predicates only.
        std::vector<std::string> statInk;
        std::vector<int> meters; // Native normalized values; -1 means no meter.
        std::vector<std::string> rowDetails;
        std::vector<bool> rowEnabled;
        std::vector<bool> rowReadOnly; // Informational rows keep focus for reading, without action chrome.
        std::vector<std::string> tabs;
        size_t selectedTab{};
        std::string headerStatus;
        int metricHeight = 48;
        int metricColumns{};
        int rowHeight{};
        int reservedHeight{}; // Space for specialized native charts, never synthetic graph data.
        std::string navigationHint;
        std::vector<std::string> rowValues; // Dense label/value reports; values are not actions.
        bool contentSized{};
        std::vector<uint16_t> colourValues; // Native palette entries; focus is a spatial swatch grid.
        uint16_t currentColour{};
        size_t columns = 1; // Controller text entry uses the same content shell.
        // The one action this screen is for, drawn filled in the section colour. Unset: the first row
        // of a review. Set to rows.size() to mark none.
        std::optional<size_t> primaryRow;
        // The first stat is the headline number: drawn in the heading font.
        bool heroStat{};
        // A small picture of recent numbers: paired bars on the left ("this month: in / out") and
        // a column per period on the right (negative columns hang below the line).
        struct Chart
        {
            std::string splitTitle;
            std::vector<std::tuple<std::string, int64_t, std::string>> split; // label, value, ink
            std::vector<std::string> splitValues;
            std::string columnsTitle;
            std::vector<int64_t> columns;
            std::vector<std::string> columnLabels;
        };
        std::optional<Chart> chart;
        // Before -> after rows for drafts and reviews ("Loan £2,000 > £3,000"), so a decision shows
        // what changes instead of a table of every related number.
        struct Change
        {
            std::string label, before, after;
            std::string afterInk = "{WHITE}";
            int meter = -1; // Optional 0-100 bar for the "after" value against a limit.
        };
        std::vector<Change> changes;
        std::optional<size_t> highlightedChange; // The value the D-pad is adjusting.
    };
    inline int ChangeHeight(const ManagementPresentation::Change& change)
    {
        return change.meter >= 0 ? 34 : 24;
    }
    inline constexpr int kChartHeight = 84;
    inline std::optional<size_t> PrimaryRow(const ManagementPresentation& model)
    {
        if (model.primaryRow)
            return *model.primaryRow < model.rows.size() ? model.primaryRow : std::nullopt;
        // Only a review has an obvious default; elsewhere each presenter marks its own primary, since
        // a grid of equal choices (campaign types, say) has none.
        if (model.confirmation && !model.rows.empty() && (model.rowEnabled.empty() || model.rowEnabled[0]))
            return size_t{ 0 };
        return std::nullopt;
    }
    // Header band: title (with optional status on the right), a rule, then tabs.
    inline constexpr int kHeaderHeight = 46, kTabsHeight = 32;
    struct ManagementLayout
    {
        int columns, rowHeight, rowStart, statColumns, statHeight;
        size_t capacity;
    };
    inline ManagementLayout MeasureManagement(ScreenSize size, const ManagementPresentation& model)
    {
        const int columns = model.actionGrid ? (size.width >= 360 ? 2 : 1) : static_cast<int>(std::max<size_t>(1, model.columns));
        const int statColumns = model.summaryMetrics && !model.stats.empty() ? (model.metricColumns ? model.metricColumns : size.width >= 440 ? 3 : 2) : 1;
        const int statHeight = model.summaryMetrics ? std::max(model.metricHeight, model.meterDirections.empty() ? 0 : 66) : 24;
        const int statRows = (static_cast<int>(model.stats.size()) + statColumns - 1) / statColumns;
        const int rowStart = kHeaderHeight + (model.tabs.empty() ? 0 : kTabsHeight) + statRows * statHeight + static_cast<int>(model.bodyLines.size()) * 16
            + model.reservedHeight + (model.graph.empty() ? 0 : 110) + (model.chart ? kChartHeight + 6 : 0)
            + [&] { int h = model.changes.empty() ? 0 : 8; for (const auto& c : model.changes) h += ChangeHeight(c); return h; }() + (model.rows.empty() ? 0 : 11) + (model.toolbarCount ? 36 : 0);
        const int rowHeight = model.rowHeight ? model.rowHeight : model.denseRoster ? 38 : model.rosterCards ? 46 : model.actionGrid ? 30 : columns > 1 ? 24 : 30;
        const size_t capacity = std::max(1, (size.height - rowStart - 34) / rowHeight) * columns;
        return {columns, rowHeight, rowStart, statColumns, statHeight, capacity};
    }
    inline void DrawManagementContent(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, const ManagementPresentation& model, bool focused = true, bool drawSurface = true)
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
        const size_t toolbar=std::min(model.toolbarCount,model.rows.size());
        const auto primary = PrimaryRow(model);
        const size_t count=model.rows.size()-toolbar;
        const size_t selectedItem=std::max(model.selected,toolbar)-toolbar;
        const size_t totalRows=(count+columns-1)/columns;
        const int listHeight=std::max(rowHeight,size.height-rowStart-34);
        const int offset=ListOffset(selectedItem/columns,totalRows,listHeight,rowHeight);
        const size_t first=toolbar+static_cast<size_t>(offset/rowHeight)*columns;
        const size_t end=std::min(model.rows.size(),toolbar+static_cast<size_t>((offset+listHeight+rowHeight-1)/rowHeight)*columns);
        if (drawSurface) DrawSurface(rt, p, size);
        // No breadcrumb: the rail and tabs already say where you are, and the title says what this is.
        const int statusWidth = model.headerStatus.empty() ? 0 : std::min(160, TextWidth(model.headerStatus, true) + 14);
        if (!model.headerStatus.empty())
        {
            const bool coloured = model.headerInk == "{POSITIVE}" || model.headerInk == "{CAUTION}" || model.headerInk == "{DANGER}";
            if (coloured)
                FillRound(
                    rt, p + ScreenCoordsXY{ width - 12 - statusWidth, 13 }, { 4, 14 },
                    static_cast<Drawing::PaletteIndex>(
                        model.headerInk == "{POSITIVE}"     ? PocketPalette::positive
                            : model.headerInk == "{DANGER}" ? PocketPalette::destructive
                                                            : PocketPalette::caution));
            DrawStrongText(rt, p + ScreenCoordsXY{ width - 12 - statusWidth + 9, 17 }, model.headerInk + model.headerStatus, statusWidth - 9);
        }
        const int contentTop = kHeaderHeight + (model.tabs.empty() ? 0 : kTabsHeight);
        DrawHeadingText(rt, p + ScreenCoordsXY{12, 11},
            std::string(model.destructive ? "{DANGER}" : model.confirmation ? "{GOLD}" : "{WHITE}") + model.title,
            width - 24 - statusWidth - (statusWidth ? 6 : 0));
        Fill(rt, p + ScreenCoordsXY{12, 37}, {width - 24, 1}, Theme::edge);
        if (model.destructive) Fill(rt,p+ScreenCoordsXY{12,37},{width-24,2},static_cast<Drawing::PaletteIndex>(PocketPalette::destructive));
        if (!model.tabs.empty()) DrawTabs(rt,p+ScreenCoordsXY{10,kHeaderHeight - 2},width-20,model.tabs,model.selectedTab,model.breadcrumb);
        if (!model.colourValues.empty())
        {
            DrawColourGrid(rt, p, size, model.colourValues, model.rows, model.selected, model.currentColour);
            return;
        }
        const int valueX = 14 + (width - 30) * 45 / 100;
        for (size_t i = 0; i < model.stats.size(); ++i)
        {
            const int y = contentTop + static_cast<int>(i / statColumns) * statHeight;
            const bool highlight = focused && model.highlightedStat == i;
            if (model.summaryMetrics)
            {
                const int cellWidth = (width - 24) / statColumns;
                DrawMetric(
                    rt, p + ScreenCoordsXY{ 12 + static_cast<int>(i % statColumns) * cellWidth, y },
                    { cellWidth - 4, statHeight - 5 }, model.stats[i].first, model.stats[i].second, highlight,
                    i < model.statInk.size() ? model.statInk[i] : "{WHITE}", model.heroStat && i == 0);
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
        for (size_t i=0;i<model.meters.size() && i<model.stats.size();++i)
        {
            if(model.meters[i]<0) continue;
            const int cellWidth=(width-24)/statColumns;
            const auto start=p+ScreenCoordsXY{12+static_cast<int>(i%statColumns)*cellWidth,contentTop+static_cast<int>(i/statColumns)*statHeight+statHeight-8};
            const bool directional = i < model.meterDirections.size();
            const bool attention = i < model.meterAttention.size() && model.meterAttention[i];
            const auto ink = attention                        ? static_cast<Drawing::PaletteIndex>(PocketPalette::caution)
                : directional && model.meterDirections[i] > 0 ? static_cast<Drawing::PaletteIndex>(PocketPalette::positive)
                                                              : Theme::gold;
            DrawMeter(rt, start + ScreenCoordsXY{ 8, -2 }, cellWidth - 22, model.meters[i], ink, 4);
            if (directional && attention)
                DrawModernText(rt, start + ScreenCoordsXY{ 0, -13 }, "{CAUTION}! Attention", {}, cellWidth - 6);
        }
        for(size_t i=0;i<model.toolbarCount && i<model.rows.size();++i)
        {
            const int cellWidth=(width-20)/static_cast<int>(model.toolbarCount);
            DrawActionRow(rt,p+ScreenCoordsXY{10+static_cast<int>(i)*cellWidth,rowStart-47},{cellWidth-4,28},model.rows[i],"",focused&&model.selected==i);
        }
        for (size_t i = 0; i < model.bodyLines.size(); ++i)
            DrawModernEllipsised(rt, p + ScreenCoordsXY{12, contentTop + 4 + statRows * statHeight + static_cast<int>(i) * 16},
                width - 24, model.bodyLines[i].starts_with("{") ? model.bodyLines[i] : "{WHITE}" + model.bodyLines[i]);
        if (!model.changes.empty())
        {
            int y = contentTop + statRows * statHeight + static_cast<int>(model.bodyLines.size()) * 16 + 2;
            for (size_t i = 0; i < model.changes.size(); ++i)
            {
                const auto& change = model.changes[i];
                const auto origin = p + ScreenCoordsXY{ 12, y };
                const int h = ChangeHeight(change) - 4;
                const bool adjusting = focused && model.highlightedChange == i;
                FillRound(rt, origin, { width - 24, h }, Theme::raised);
                DrawModernEllipsised(rt, origin + ScreenCoordsXY{ 8, 6 }, width / 3, "{MUTED}" + change.label);
                // Right-aligned "before  >  after": the eye lands on the new value.
                const int afterWidth = TextWidth(change.after, true);
                const int afterX = width - 24 - 10 - afterWidth;
                DrawStrongText(rt, origin + ScreenCoordsXY{ afterX, 6 }, change.afterInk + change.after);
                if (!change.before.empty() && change.before != change.after)
                {
                    const int arrowX = afterX - 12;
                    for (int r = 0; r < 4; ++r)
                        Fill(rt, origin + ScreenCoordsXY{ arrowX + r, 7 + r }, { 1, 9 - 2 * r }, Theme::text);
                    const int beforeWidth = TextWidth(change.before);
                    DrawModernText(rt, origin + ScreenCoordsXY{ arrowX - 8 - beforeWidth, 6 }, "{MUTED}" + change.before);
                }
                if (change.meter >= 0)
                    DrawMeter(rt, origin + ScreenCoordsXY{ 8, h - 9 }, width - 40, change.meter, Theme::gold, 4);
                if (adjusting)
                {
                    // Not a focus ring: A still acts on the focused button below. A bar and arrows
                    // say "left/right changes this".
                    FillRound(rt, origin, { 3, h }, Theme::gold);
                    DrawStrongText(rt, origin + ScreenCoordsXY{ width / 3 + 12, 6 }, "{GOLD}<  >");
                }
                y += ChangeHeight(change);
            }
        }
        if (model.chart)
        {
            const auto& chart = *model.chart;
            const auto origin = p + ScreenCoordsXY{ 12, contentTop + statRows * statHeight + static_cast<int>(model.bodyLines.size()) * 16 + 2 };
            const int chartWidth = width - 24;
            FillRound(rt, origin, { chartWidth, kChartHeight }, Theme::raised);
            const int half = chartWidth / 2;
            DrawModernEllipsised(rt, origin + ScreenCoordsXY{ 8, 6 }, half - 12, "{MUTED}" + chart.splitTitle);
            int64_t largest = 1;
            for (const auto& [label, value, ink] : chart.split)
                largest = std::max<int64_t>(largest, std::abs(value));
            for (size_t i = 0; i < chart.split.size(); ++i)
            {
                const auto& [label, value, ink] = chart.split[i];
                const int y = 24 + static_cast<int>(i) * 22;
                DrawModernText(rt, origin + ScreenCoordsXY{ 8, y + 1 }, "{WHITE}" + label);
                const int barRoom = half - 120;
                const int bar = std::max<int>(2, static_cast<int>(barRoom * std::abs(value) / largest));
                FillRound(rt, origin + ScreenCoordsXY{ 56, y + 1 }, { bar, 9 },
                    static_cast<Drawing::PaletteIndex>(ink == "{DANGER}" ? PocketPalette::destructive : PocketPalette::positive));
                if (i < chart.splitValues.size())
                    DrawStrongText(rt, origin + ScreenCoordsXY{ 60 + bar, y + 1 }, "{WHITE}" + chart.splitValues[i], half - 64 - bar);
            }
            Fill(rt, origin + ScreenCoordsXY{ half, 8 }, { 1, kChartHeight - 16 }, Theme::edge);
            DrawModernEllipsised(rt, origin + ScreenCoordsXY{ half + 8, 6 }, half - 16, "{MUTED}" + chart.columnsTitle);
            if (!chart.columns.empty())
            {
                int64_t high = 1, low = 0;
                for (const auto value : chart.columns)
                {
                    high = std::max(high, value);
                    low = std::min(low, value);
                }
                const int top = 22, bottom = kChartHeight - 16;
                const int zero = top + static_cast<int>((bottom - top) * high / (high - low));
                const int slot = (half - 16) / static_cast<int>(chart.columns.size());
                Fill(rt, origin + ScreenCoordsXY{ half + 8, zero }, { half - 16, 1 }, Theme::edge);
                for (size_t i = 0; i < chart.columns.size(); ++i)
                {
                    const auto value = chart.columns[i];
                    const int x = half + 8 + static_cast<int>(i) * slot + 2;
                    const int span = value >= 0 ? static_cast<int>((zero - top) * value / high)
                                                : static_cast<int>((bottom - zero) * -value / std::max<int64_t>(1, -low));
                    if (value >= 0)
                        FillRound(rt, origin + ScreenCoordsXY{ x, zero - span }, { slot - 4, std::max(span, 1) },
                            static_cast<Drawing::PaletteIndex>(PocketPalette::positive));
                    else
                        FillRound(rt, origin + ScreenCoordsXY{ x, zero + 1 }, { slot - 4, std::max(span, 1) },
                            static_cast<Drawing::PaletteIndex>(PocketPalette::destructive));
                    if (i < chart.columnLabels.size())
                        DrawModernText(rt, origin + ScreenCoordsXY{ x + (slot - 4) / 2 - 3, bottom + 4 }, "{MUTED}" + chart.columnLabels[i]);
                }
            }
        }
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
        // List labels keep only their first part: "GUESTS / CURRENT ACTIVITY / HAPPINESS" says no more
        // than "GUESTS" once the rows themselves show activity and happiness.
        if (!model.rows.empty() && model.bodyLines.empty() && !model.listLabel.empty())
            DrawSectionLabel(rt, p + ScreenCoordsXY{12, rowStart - 12}, width - 24,
                model.listLabel.substr(0, model.listLabel.find(" / ")));
        Drawing::RenderTarget listRt;
        if(ClipRenderTarget(listRt,rt,p+ScreenCoordsXY{0,rowStart},width-8,listHeight))
        {
        for (size_t i = first; i < end; ++i)
        {
            const int y = static_cast<int>((i - toolbar) / columns) * rowHeight-offset;
            const int cellWidth = (width - 20) / columns;
            const int x = 10 + static_cast<int>((i - toolbar) % columns) * cellWidth;
            const bool enabled = i < model.rowEnabled.size() ? model.rowEnabled[i]
                : model.rows[i].find("unavailable") == std::string::npos && model.rows[i].find("Unavailable") == std::string::npos;
            if (columns > 2) // Native on-screen keyboard retains its compact cells.
            {
                if (focused && i == model.selected) {Fill(listRt, ScreenCoordsXY{x, y}, {cellWidth - 2, rowHeight - 2}, Theme::gold);ReportSelected(listRt, {x, y}, {cellWidth - 2, rowHeight - 2});}
                DrawModernEllipsised(listRt, ScreenCoordsXY{x + 3, y + 6}, cellWidth - 6,
                    std::string(focused && i == model.selected ? "{BLACK}" : "{WHITE}") + model.rows[i]);
            }
            else if (i < model.rowReadOnly.size() && model.rowReadOnly[i])
            {
                const bool selected=focused && i==model.selected;
                Fill(listRt,{x,y},{cellWidth-4,rowHeight-2},selected?Theme::gold:Theme::ink);
                if(selected)ReportSelected(listRt,{x,y},{cellWidth-4,rowHeight-2});
                DrawModernEllipsised(listRt,{x+8,y+7},cellWidth-20,
                    std::string(selected?"{BLACK}":"{WHITE}")+model.rows[i]);
            }
            else if (i < model.rowValues.size())
            {
                const bool selected=focused&&i==model.selected;
                if(selected){Fill(listRt,ScreenCoordsXY{x,y},{cellWidth-4,rowHeight-2},Theme::gold);ReportSelected(listRt,{x,y},{cellWidth-4,rowHeight-2});}
                DrawModernEllipsised(listRt,ScreenCoordsXY{x+8,y+7},cellWidth-164,
                    std::string(selected?"{BLACK}":"{WHITE}")+model.rows[i]);
                DrawModernEllipsised(listRt,ScreenCoordsXY{x+cellWidth-150,y+7},140,
                    std::string(selected?"{BLACK}":"{WHITE}")+model.rowValues[i]);
            }
            else if (model.listLabel.starts_with("CURRENT THOUGHTS"))
            {
                const bool selected = focused && i == model.selected;
                Fill(listRt, ScreenCoordsXY{ x, y }, { cellWidth - 4, rowHeight - 2 }, selected ? Theme::gold : Theme::ink);
                if (selected) ReportSelected(listRt, { x, y }, { cellWidth - 4, rowHeight - 2 });
                DrawWrappedLabel(
                    listRt, ScreenCoordsXY{ x + 8, y + 7 }, cellWidth - 20,
                    std::string(selected ? "{BLACK}" : "{WHITE}") + model.rows[i], 2);
            }
            else
                DrawActionRow(
                    listRt, ScreenCoordsXY{ x, y }, { cellWidth - 4, rowHeight - 2 }, model.rows[i],
                    i < model.rowDetails.size() ? model.rowDetails[i] : "", focused && i == model.selected, enabled,
                    PanelIcon::none, model.destructive && i == 0, !model.rosterCards && !model.denseRoster,
                    primary == i);
        }
        }
        // Feedback and overflow have independent lines, so failures never hide pagination.
        if (!model.status.empty())
            DrawModernEllipsised(rt, p + ScreenCoordsXY{12, size.height - 29}, width - 24, "{WHITE}" + model.status);
        if(count>capacity)
        {
            DrawModernText(rt,p+ScreenCoordsXY{12,size.height-13},"{MUTED}"+std::to_string(std::min(selectedItem+1,count))+" / "+std::to_string(count));
            const int totalHeight=std::max(listHeight,static_cast<int>(totalRows)*rowHeight);
            const int thumb=std::max(8,listHeight*listHeight/totalHeight);
            Fill(rt,p+ScreenCoordsXY{width-6,rowStart},{2,listHeight},Theme::raised);
            Fill(rt,p+ScreenCoordsXY{width-6,rowStart+offset*(listHeight-thumb)/std::max(1,totalHeight-listHeight)},{2,thumb},Theme::gold);
        }
    }
    // Compatibility wrapper for construction-era captures; Dashboard uses the content region directly.
    inline void DrawManagementPanel(Drawing::RenderTarget& rt, ScreenSize screen, const ManagementPresentation& model)
    {
        DrawManagementContent(rt, Concept::kMenuOrigin, { 244, Layout::ContentHeight(screen) }, model);
        DrawFooter(
            rt, screen,
            { { "D-pad", model.editing ? "Adjust" : "Choose" },
              { "A",
                model.editing            ? "Review"
                    : model.confirmation ? "Confirm"
                                         : "Select" },
              { "B", "Back" } });
    }
} // namespace OpenRCT2::Ui::Handheld


// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <functional>
#include "Panel.h"
#include "ColourGrid.h"
#include "ListScroll.h"
#include "TextEntry.h"

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
        // Item pages lead with the thing itself: a picture, its state as a switch, and the two or
        // three numbers that matter most as meters.
        struct Hero
        {
            std::function<void(Drawing::RenderTarget&)> picture; // Draws into a 64 x 60 clip.
            std::vector<std::string> states;                     // e.g. Closed / Testing / Open.
            size_t state{};
            bool stateFocused{};
            struct Meter
            {
                std::string label, value;
                int percent{};
                std::string ink = "{WHITE}";
            };
            std::vector<Meter> meters;
            std::string caption; // What they are doing right now, under the meters.
        };
        std::optional<Hero> hero;
        // A text field and controller keyboard instead of rows. rows still holds TextEntry::Keys()
        // and selected is a logical key index, so screens confirm keys as before.
        std::optional<TextEntry> keyboard;
        std::string keyboardHeading; // "Rename ride"; the title is the field's placeholder.
        std::optional<size_t> heroRow; // A row that is the hero's switch: not drawn as a button.
        // Rows that are values you change in place with Left/Right (a price, say): drawn full width
        // as "label  < value >" above the buttons instead of as a button that opens an editor.
        std::vector<size_t> settingRows;
        std::vector<std::string> settingValues; // Parallel to settingRows.
        struct SettingLine
        {
            std::string label, value;
            bool focused{}, enabled{};
        };
        std::vector<SettingLine> settingLines; // Render-only, filled when the model is laid out.
    };
    inline constexpr int kHeroHeight = 72;
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
        const int rowStart = kHeaderHeight + (model.tabs.empty() ? 0 : kTabsHeight) + (model.hero ? kHeroHeight + 6 : 0) + statRows * statHeight + static_cast<int>(model.bodyLines.size()) * 16
            + model.reservedHeight + (model.graph.empty() ? 0 : 110) + (model.chart ? kChartHeight + 6 : 0)
            + [&] { int h = model.changes.empty() ? 0 : 8; for (const auto& c : model.changes) h += ChangeHeight(c); return h; }()
            + static_cast<int>(model.settingLines.size()) * 28 + (model.rows.empty() ? 0 : 11) + (model.toolbarCount ? 36 : 0);
        const int rowHeight = model.rowHeight ? model.rowHeight : model.denseRoster ? 38 : model.rosterCards ? 46 : model.actionGrid ? 30 : columns > 1 ? 24 : 30;
        const size_t capacity = std::max(1, (size.height - rowStart - 34) / rowHeight) * columns;
        return {columns, rowHeight, rowStart, statColumns, statHeight, capacity};
    }
    // "Back to ..." rows repeat what B does on every screen. They stay in the model (so native row
    // indices and handlers are untouched) but are not drawn, unless they are the only rows.
    inline bool IsBackRow(const std::string& label)
    {
        return label.starts_with("Back to ");
    }
    inline std::optional<ManagementPresentation> WithoutBackRows(const ManagementPresentation& model)
    {
        const auto setting = [&](size_t i) { return std::find(model.settingRows.begin(), model.settingRows.end(), i) != model.settingRows.end(); };
        const auto hidden = [&](size_t i) { return IsBackRow(model.rows[i]) || model.heroRow == i || setting(i); };
        size_t hiddenCount = 0;
        for (size_t i = 0; i < model.rows.size(); ++i)
            hiddenCount += hidden(i) ? 1 : 0;
        if (model.toolbarCount || !model.colourValues.empty() || hiddenCount == 0
            || (hiddenCount == model.rows.size() && !model.heroRow && model.settingRows.empty()))
            return std::nullopt;
        auto filtered = model;
        filtered.heroRow.reset();
        filtered.settingRows.clear();
        filtered.settingValues.clear();
        for (size_t n = 0; n < model.settingRows.size(); ++n)
        {
            const auto i = model.settingRows[n];
            if (i >= model.rows.size())
                continue;
            filtered.settingLines.push_back({ ButtonText(model.rows[i]), n < model.settingValues.size() ? model.settingValues[n] : "",
                                              model.selected == i, i >= model.rowEnabled.size() || model.rowEnabled[i] });
        }
        if (filtered.hero)
            filtered.hero->stateFocused = model.heroRow && model.selected == *model.heroRow;
        filtered.rows.clear(); filtered.rowDetails.clear(); filtered.rowEnabled.clear();
        filtered.rowReadOnly.clear(); filtered.rowValues.clear();
        filtered.selected = SIZE_MAX;
        filtered.primaryRow = model.primaryRow ? std::optional<size_t>{ model.rows.size() } : std::nullopt;
        for (size_t i = 0; i < model.rows.size(); ++i)
        {
            if (hidden(i))
                continue;
            if (i == model.selected) filtered.selected = filtered.rows.size();
            if (model.primaryRow == i) filtered.primaryRow = filtered.rows.size();
            filtered.rows.push_back(model.rows[i]);
            if (i < model.rowDetails.size()) filtered.rowDetails.resize(filtered.rows.size() - 1), filtered.rowDetails.push_back(model.rowDetails[i]);
            if (i < model.rowEnabled.size()) filtered.rowEnabled.resize(filtered.rows.size() - 1, true), filtered.rowEnabled.push_back(model.rowEnabled[i]);
            if (i < model.rowReadOnly.size()) filtered.rowReadOnly.resize(filtered.rows.size() - 1, false), filtered.rowReadOnly.push_back(model.rowReadOnly[i]);
            if (i < model.rowValues.size()) filtered.rowValues.resize(filtered.rows.size() - 1), filtered.rowValues.push_back(model.rowValues[i]);
        }
        if (filtered.selected == SIZE_MAX)
            filtered.selected = filtered.rows.size(); // Focus is on a hidden row: draw none focused.
        return filtered;
    }
    inline std::vector<ControlHint> KeyboardHints()
    {
        return { { "D-pad", "Move" }, { "A", "Type" }, { "Y", "Delete" }, { "X", "Space" }, { "START", "Done" }, { "B", "Cancel" } };
    }
    inline const char* KeyLabel(const TextEntry& entry, size_t key)
    {
        switch (key)
        {
            case TextEntry::kCase: return entry.Lower() ? "ABC" : "abc"; // What Shift will switch to.
            case TextEntry::kSpace: return "Space";
            case TextEntry::kDelete: return "Delete";
            case TextEntry::kClear: return "Clear";
            case TextEntry::kDone: return "Done";
            default: return nullptr;
        }
    }
    // A console-style keyboard: a real text field (text, caret, count) over QWERTY keycaps. The
    // focused key is the only ringed key; Done is the one filled key, since it is where you finish.
    inline void DrawKeyboardContent(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, const ManagementPresentation& model, bool focused, bool drawSurface)
    {
        const auto& entry = *model.keyboard;
        const int width = size.width;
        if (activeMenuFeel)
            activeMenuFeel->HidePointer();
        if (drawSurface)
            DrawSurface(rt, p, size);
        DrawHeadingText(rt, p + ScreenCoordsXY{ 12, 11 },
            "{WHITE}" + (model.keyboardHeading.empty() ? std::string("Enter a name") : model.keyboardHeading), width - 24);
        Fill(rt, p + ScreenCoordsXY{ 12, 37 }, { width - 24, 1 }, Theme::edge);

        const auto field = p + ScreenCoordsXY{ 12, 46 };
        const int fieldWidth = width - 24, fieldHeight = 28;
        FillRound(rt, field, { fieldWidth, fieldHeight }, Theme::edge);
        FillRound(rt, field + ScreenCoordsXY{ 1, 1 }, { fieldWidth - 2, fieldHeight - 2 }, Theme::raised);
        const auto counter = std::to_string(entry.Length()) + " / " + std::to_string(TextEntry::limit);
        const int counterWidth = TextWidth(counter);
        DrawModernText(rt, field + ScreenCoordsXY{ fieldWidth - 10 - counterWidth, CapCentre(fieldHeight) },
            (entry.Length() >= TextEntry::limit ? "{CAUTION}" : "{MUTED}") + counter);
        const int room = fieldWidth - 30 - counterWidth;
        int caret = 10;
        if (entry.text.empty())
            DrawModernEllipsised(rt, field + ScreenCoordsXY{ 14, CapCentre(fieldHeight) }, room - 4, "{MUTED}" + model.title);
        else
        {
            // Long names scroll: the end you are typing at stays visible.
            std::string shown = entry.text;
            while (!shown.empty() && TextWidth(shown, true) > room)
            {
                size_t cut = 1;
                while (cut < shown.size() && (static_cast<unsigned char>(shown[cut]) & 0xc0) == 0x80)
                    ++cut;
                shown.erase(0, cut);
            }
            DrawStrongText(rt, field + ScreenCoordsXY{ 10, CapCentre(fieldHeight) }, "{WHITE}" + shown, room + 4);
            caret = 10 + TextWidth(shown, true) + 1;
        }
        Fill(rt, field + ScreenCoordsXY{ caret, (fieldHeight - 14) / 2 }, { 2, 14 }, Theme::gold);

        const auto& layout = TextEntry::Layout();
        const auto keys = entry.Keys();
        constexpr int gap = 3;
        const int top = 46 + fieldHeight + 10;
        const int bottom = size.height - (model.status.empty() ? 10 : 24);
        const int rowCount = static_cast<int>(layout.size());
        const int keyHeight = std::clamp((bottom - top - (rowCount - 1) * gap) / rowCount, 18, 26);
        const int unit = (fieldWidth - 9 * gap) / 10;
        const int left = 12 + (fieldWidth - (unit * 10 + 9 * gap)) / 2;
        const size_t focusKey = model.selected == TextEntry::kBack ? TextEntry::kDone : model.selected;
        for (size_t r = 0; r < layout.size(); ++r)
            for (const auto& slot : layout[r])
            {
                const auto pos = p + ScreenCoordsXY{ left + slot.column * (unit + gap), top + static_cast<int>(r) * (keyHeight + gap) };
                const ScreenSize keySize{ slot.span * unit + (slot.span - 1) * gap, keyHeight };
                const bool done = slot.key == TextEntry::kDone;
                const bool isFocus = focused && slot.key == focusKey;
                DrawButtonFace(rt, pos, keySize, done ? ButtonKind::primary : ButtonKind::secondary, isFocus);
                if (const auto* label = KeyLabel(entry, slot.key))
                {
                    const int w = TextWidth(label, true);
                    DrawStrongText(rt, pos + ScreenCoordsXY{ (keySize.width - w) / 2, CapCentre(keyHeight - 2) },
                        std::string(done ? "{BLACK}" : "{WHITE}") + label);
                }
                else if (slot.key < keys.size())
                    DrawButtonLabel(rt, pos, { keySize.width, keyHeight - 2 }, keys[slot.key], false, true);
            }
        if (!model.status.empty())
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 12, size.height - 16 }, width - 24, "{MUTED}" + model.status);
    }
    inline void DrawManagementContent(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, const ManagementPresentation& model, bool focused = true, bool drawSurface = true)
    {
        if (model.keyboard)
            return DrawKeyboardContent(rt, p, size, model, focused, drawSurface);
        if (const auto filtered = WithoutBackRows(model))
            return DrawManagementContent(rt, p, size, *filtered, focused, drawSurface);
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
        const int heroTop = kHeaderHeight + (model.tabs.empty() ? 0 : kTabsHeight);
        const int contentTop = heroTop + (model.hero ? kHeroHeight + 6 : 0);
        DrawHeadingText(rt, p + ScreenCoordsXY{12, 11},
            std::string(model.destructive ? "{DANGER}" : model.confirmation ? "{GOLD}" : "{WHITE}") + model.title,
            width - 24 - statusWidth - (statusWidth ? 6 : 0));
        Fill(rt, p + ScreenCoordsXY{12, 37}, {width - 24, 1}, Theme::edge);
        if (model.destructive) Fill(rt,p+ScreenCoordsXY{12,37},{width-24,2},static_cast<Drawing::PaletteIndex>(PocketPalette::destructive));
        if (!model.tabs.empty()) DrawTabs(rt,p+ScreenCoordsXY{10,kHeaderHeight - 2},width-20,model.tabs,model.selectedTab,model.breadcrumb);
        if (model.hero)
        {
            const auto& hero = *model.hero;
            const auto origin = p + ScreenCoordsXY{ 12, heroTop };
            const int heroWidth = width - 24;
            FillRound(rt, origin, { heroWidth, kHeroHeight }, Theme::raised);
            int textLeft = 10;
            if (hero.picture)
            {
                FillRound(rt, origin + ScreenCoordsXY{ 6, 6 }, { 64, 60 }, Theme::ink);
                Drawing::RenderTarget picture;
                if (ClipRenderTarget(picture, rt, origin + ScreenCoordsXY{ 6, 6 }, 64, 60))
                    hero.picture(picture);
                textLeft = 80;
            }
            const int area = heroWidth - textLeft - 10;
            int y = 8;
            if (!hero.states.empty())
            {
                // The state switch: every option visible, the current one filled. Left/Right on it
                // changes the state in place when it has focus.
                DrawSegmented(rt, origin + ScreenCoordsXY{ textLeft, y }, { area, 22 }, hero.states, hero.state);
                if (hero.stateFocused && focused)
                    DrawFocusRing(rt, origin + ScreenCoordsXY{ textLeft, y }, { area, 22 });
                y += 30;
            }
            // A caption sits under the meters, or under the switch when there are none (a shop).
            if (!hero.caption.empty() && (hero.states.empty() || hero.meters.empty()))
                DrawStrongText(rt, origin + ScreenCoordsXY{ textLeft, kHeroHeight - 22 }, "{WHITE}" + hero.caption, area);
            if (!hero.meters.empty())
            {
                const int slot = area / static_cast<int>(hero.meters.size());
                const int meterTop = hero.states.empty() ? 12 : kHeroHeight - 30;
                for (size_t i = 0; i < hero.meters.size(); ++i)
                {
                    const auto& meter = hero.meters[i];
                    const auto at = origin + ScreenCoordsXY{ textLeft + static_cast<int>(i) * slot, meterTop };
                    const int vw = TextWidth(meter.value, true);
                    DrawModernEllipsised(rt, at, std::max(20, slot - vw - 16), "{MUTED}" + meter.label);
                    DrawStrongText(rt, at + ScreenCoordsXY{ slot - 10 - vw, 0 }, meter.ink + meter.value);
                    const auto ink = meter.ink == "{DANGER}" ? static_cast<Drawing::PaletteIndex>(PocketPalette::destructive)
                        : meter.ink == "{CAUTION}"           ? static_cast<Drawing::PaletteIndex>(PocketPalette::caution)
                        : meter.ink == "{POSITIVE}"          ? static_cast<Drawing::PaletteIndex>(PocketPalette::positive)
                                                             : Theme::gold;
                    DrawMeter(rt, at + ScreenCoordsXY{ 0, 16 }, slot - 10, meter.percent, ink, 5);
                }
            }
        }
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
                // Label left, value right-aligned in strong ink, each on its own soft card: a
                // review reads as a short receipt rather than a two-column table.
                const auto origin = p + ScreenCoordsXY{ 12, y };
                FillRound(rt, origin, { width - 24, 21 }, Theme::raised);
                DrawModernEllipsised(rt, origin + ScreenCoordsXY{ 8, 5 }, valueX - 24, "{MUTED}" + model.stats[i].first);
                const auto& value = model.stats[i].second;
                const int valueWidth = std::min(width - valueX - 20, TextWidth(value, true));
                DrawStrongText(rt, origin + ScreenCoordsXY{ width - 32 - valueWidth, 5 },
                    (i < model.statInk.size() ? model.statInk[i] : std::string("{WHITE}")) + value, width - valueX - 20);
                if (highlight)
                    DrawFocusRing(rt, origin, { width - 24, 21 });
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
            DrawActionRow(rt,p+ScreenCoordsXY{10+static_cast<int>(i)*cellWidth,rowStart-47},{cellWidth-4,28},model.rows[i],"",focused&&model.selected==i,true,PanelIcon::none,false,false,false,true);
        }
        for (size_t i = 0; i < model.bodyLines.size(); ++i)
            if (const auto tab = model.bodyLines[i].find('\t'); tab != std::string::npos)
            {
                // "Label\tvalue": a muted label and a strong value, like a stat card laid flat. Text
                // only honours a leading colour code, so the two parts are drawn separately.
                const auto label = model.bodyLines[i].substr(0, tab);
                const auto origin = p + ScreenCoordsXY{ 12, contentTop + 4 + statRows * statHeight + static_cast<int>(i) * 16 };
                DrawModernText(rt, origin, "{MUTED}" + label);
                const int x = TextWidth(label) + 6;
                DrawStrongText(rt, origin + ScreenCoordsXY{ x, 0 }, "{WHITE}" + model.bodyLines[i].substr(tab + 1), width - 24 - x);
            }
            else
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
            // A chart with no columns gives its bars the full width and a wider label column.
            const bool wide = chart.columns.empty() && chart.columnsTitle.empty();
            const int half = wide ? chartWidth : chartWidth / 2;
            const int labelWidth = wide ? std::min(220, chartWidth / 2) : 48;
            DrawModernEllipsised(rt, origin + ScreenCoordsXY{ 8, 6 }, half - 12, "{MUTED}" + chart.splitTitle);
            int64_t largest = 1;
            for (const auto& [label, value, ink] : chart.split)
                largest = std::max<int64_t>(largest, std::abs(value));
            for (size_t i = 0; i < chart.split.size(); ++i)
            {
                const auto& [label, value, ink] = chart.split[i];
                const int y = 24 + static_cast<int>(i) * 18;
                DrawModernEllipsised(rt, origin + ScreenCoordsXY{ 8, y + 1 }, labelWidth - 4, "{WHITE}" + label);
                const int barLeft = 8 + labelWidth;
                const int barRoom = half - barLeft - 60;
                const int bar = std::max<int>(2, static_cast<int>(barRoom * std::abs(value) / largest));
                FillRound(rt, origin + ScreenCoordsXY{ barLeft, y + 1 }, { bar, 9 },
                    static_cast<Drawing::PaletteIndex>(ink == "{DANGER}" ? PocketPalette::destructive : PocketPalette::positive));
                if (i < chart.splitValues.size())
                    DrawStrongText(rt, origin + ScreenCoordsXY{ barLeft + bar + 4, y + 1 }, "{WHITE}" + chart.splitValues[i], half - barLeft - bar - 8);
            }
            if (!wide)
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
        for (size_t i = 0; i < model.settingLines.size(); ++i)
        {
            const auto& line = model.settingLines[i];
            const auto origin = p + ScreenCoordsXY{ 12, rowStart - 11 - static_cast<int>(model.settingLines.size() - i) * 28 };
            const int lineWidth = width - 24;
            FillRound(rt, origin, { lineWidth, 24 }, Theme::raised);
            DrawStrongText(rt, origin + ScreenCoordsXY{ 10, 7 }, std::string(line.enabled ? "{WHITE}" : "{DISABLED}") + line.label, lineWidth / 2);
            const int vw = TextWidth(line.value, true);
            const int right = origin.x + lineWidth - 12;
            if (line.enabled)
            {
                // < value >  : the arrows say Left/Right changes it right here.
                const auto arrowInk = line.focused ? Theme::gold : Theme::text;
                for (int r = 0; r < 4; ++r)
                {
                    Fill(rt, { right - 3 + r, origin.y + 8 + r }, { 1, 9 - 2 * r }, arrowInk);      // >
                    Fill(rt, { right - vw - 17 - r, origin.y + 8 + r }, { 1, 9 - 2 * r }, arrowInk); // <
                }
                DrawStrongText(rt, { right - vw - 10, origin.y + 7 }, "{WHITE}" + line.value);
            }
            else
                DrawModernText(rt, { right - TextWidth(line.value), origin.y + 7 }, "{DISABLED}" + line.value);
            if (line.focused && focused)
                DrawFocusRing(rt, origin, { lineWidth, 24 });
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
                DrawListBackground(listRt,{x,y},{cellWidth-4,rowHeight-2},selected);
                DrawModernEllipsised(listRt,{x+8,y+7},cellWidth-20,"{WHITE}"+model.rows[i]);
            }
            else if (i < model.rowValues.size())
            {
                const bool selected=focused&&i==model.selected;
                DrawListBackground(listRt,{x,y},{cellWidth-4,rowHeight-2},selected);
                DrawModernEllipsised(listRt,ScreenCoordsXY{x+8,y+7},cellWidth-164,"{WHITE}"+model.rows[i]);
                const auto& value=model.rowValues[i];
                // Values right-aligned, losses in red: a ledger you can read down the column.
                const auto valueInk=value.starts_with("-")||value.find("-")==1?"{DANGER}":"{WHITE}";
                const int valueWidth=std::min(140,TextWidth(value,true));
                DrawStrongText(listRt,ScreenCoordsXY{x+cellWidth-12-valueWidth,y+7},valueInk+value,140);
            }
            else if (model.listLabel.starts_with("CURRENT THOUGHTS"))
            {
                const bool selected = focused && i == model.selected;
                DrawListBackground(listRt, { x, y }, { cellWidth - 4, rowHeight - 2 }, selected);
                DrawWrappedLabel(listRt, ScreenCoordsXY{ x + 8, y + 7 }, cellWidth - 20, "{WHITE}" + model.rows[i], 2);
            }
            else if (!model.actionGrid && !model.confirmation && !model.editing && columns == 1)
                DrawListRow(
                    listRt, ScreenCoordsXY{ x, y }, { cellWidth - 4, rowHeight - 2 }, model.rows[i],
                    i < model.rowDetails.size() ? model.rowDetails[i] : "", focused && i == model.selected, enabled,
                    !model.rosterCards && !model.denseRoster);
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
            // The pager counts items, so a trailing "Back to ..." row (kept for B-less navigation) is
            // not one of them: 20 employees read "1 / 20", not "1 / 21".
            const size_t items = count - (count > 0 && IsBackRow(model.rows.back()) ? 1 : 0);
            DrawModernText(rt,p+ScreenCoordsXY{12,size.height-13},"{MUTED}"+std::to_string(std::min(selectedItem+1,items))+" / "+std::to_string(items));
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


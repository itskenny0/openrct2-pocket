// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Panel.h"
#include <algorithm>
#include <vector>

namespace OpenRCT2::Ui::Handheld
{
    struct ChoiceCard
    {
        size_t action; // The existing menu action, not the visual position.
        std::string label, detail;
        PanelIcon icon = PanelIcon::none;
        uint32_t sprite = 0;
    };
    inline size_t MoveChoiceCards(const std::vector<ChoiceCard>& cards, size_t action, size_t columns, int x, int y)
    {
        if (cards.empty() || columns == 0) return action;
        const auto found = std::find_if(cards.begin(), cards.end(), [&](const auto& c) { return c.action == action; });
        size_t slot = found == cards.end() ? 0 : static_cast<size_t>(found - cards.begin());
        if (y)
        {
            const auto next = static_cast<int64_t>(slot) + (y > 0 ? 1 : -1) * static_cast<int64_t>(columns);
            if (next >= 0 && next < static_cast<int64_t>(cards.size())) slot = static_cast<size_t>(next);
            else if (y > 0 && (slot / columns + 1) * columns < cards.size()) slot = cards.size() - 1;
        }
        else if (x > 0 && slot % columns + 1 < columns && slot + 1 < cards.size()) ++slot;
        else if (x < 0 && slot % columns > 0) --slot;
        return cards[slot].action;
    }
    inline void DrawChoiceCard(Drawing::RenderTarget& rt, ScreenCoordsXY cell, ScreenSize size,
        const ChoiceCard& card, bool focused)
    {
        const int press = ButtonPressOffset(focused);
        cell.y += press;
        size.height -= press;
        DrawCard(rt, cell, size, focused, true);
        Drawing::RenderTarget inside;
        if (!ClipRenderTarget(inside, rt, cell + ScreenCoordsXY{ 3, 3 }, size.width - 6, size.height - 6)) return;
        // Reserve two complete 16px label lines, including glyph descenders.
        // Dense 4-row Build cards need less art/text gap than large categories.
        const bool twoLines = TextWidth(card.label, true) > size.width - 14;
        const int artSize = std::clamp(size.height - (twoLines ? 38 : 34), 20, 38);
        const auto icon = ScreenCoordsXY{ (size.width - artSize) / 2 - 3, 1 };
        if (card.sprite)
            DrawNativeMenuArt(inside, icon, { artSize, artSize }, { card.sprite }, focused);
        else DrawIcon(inside, icon + ScreenCoordsXY{ artSize / 2 - 8, artSize / 2 - 8 }, card.icon, focused);
        DrawWrappedLabel(inside, {4, artSize + 2}, size.width - 14,
            std::string(focused ? "{BLACK}" : "{WHITE}") + card.label, 2);
    }
    inline void DrawChoiceCards(Drawing::RenderTarget& rt, ScreenSize screen, const std::string& title,
        const std::vector<ChoiceCard>& cards, size_t selected, size_t columns, const std::string& back = "Back")
    {
        const ScreenCoordsXY p{8, 8};
        const int width = screen.width - 16;
        const int height = Layout::ContentHeight(screen);
        DrawSurface(rt, p, {width, height});
        DrawHeadingText(rt, p + ScreenCoordsXY{12, 13}, "{WHITE}" + title, width - 24);
        const int rows = static_cast<int>((cards.size() + columns - 1) / columns);
        const int pitch = std::min(96, (height - 88) / std::max(1, rows));
        const int cellWidth = (width - 18) / static_cast<int>(columns);
        for (size_t i = 0; i < cards.size(); ++i)
        {
            auto cell = p + ScreenCoordsXY{10 + static_cast<int>(i % columns) * cellWidth,
                40 + static_cast<int>(i / columns) * pitch};
            const auto& card = cards[i];
            const bool focused = selected == card.action;
            DrawChoiceCard(rt, cell, {cellWidth - 6, pitch - 6}, card, focused);
        }
        const auto found = std::find_if(cards.begin(), cards.end(), [&](const auto& c) {return c.action == selected;});
        if (found != cards.end())
            DrawWrappedLabel(rt, p + ScreenCoordsXY{14, 44 + rows * pitch}, width - 28, "{MUTED}" + found->detail, 2);
        DrawFooter(rt, screen, {{"D-pad", "Choose"}, {"A", "Select"}, {"B", back}});
    }
}

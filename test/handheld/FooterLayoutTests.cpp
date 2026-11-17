// SPDX-License-Identifier: GPL-3.0-only
#include <iostream>
#include <openrct2-ui/handheld/ChoiceCards.h>
#include <openrct2-ui/handheld/ColourGrid.h>
#include <openrct2-ui/handheld/Panel.h>
using namespace OpenRCT2::Ui::Handheld;
int main()
{
    for (size_t count : { 3u, 6u, 7u, 14u })
    {
        std::vector<ChoiceCard> cards;
        for (size_t i = 0; i < count; ++i)
            cards.push_back({ count - 1 - i, "Choice", "Detail" });
        const size_t columns = count == 14 ? 4 : 3;
        for (const auto& card : cards)
            for (const auto [x, y] : { std::pair{ -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } })
                if (MoveChoiceCards(cards, card.action, columns, x, y) >= count)
                    return 5;
        if (MoveChoiceCards(cards, cards.front().action, columns, -1, 0) != cards.front().action
            || MoveChoiceCards(cards, cards[columns - 1].action, columns, 1, 0) != cards[columns - 1].action
            || MoveChoiceCards(cards, cards.front().action, columns, 0, -1) != cards.front().action)
            return 6;
    }
    if (MoveColourGrid(0, 72, -1, 0) != 0 || MoveColourGrid(0, 72, 0, 1) != 8 || MoveColourGrid(7, 72, 1, 0) != 7
        || MoveColourGrid(71, 72, 0, 1) != 71 || MoveColourGrid(71, 74, 0, 1) != 73 || MoveColourGrid(73, 74, 1, 0) != 73
        || MoveColourGrid(0, 0, 0, 1) != 0)
        return 3;
    for (size_t count : { 1u, 72u, 74u })
        for (size_t i = 0; i < count; ++i)
            for (auto [x, y] : { std::pair{ -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } })
                if (MoveColourGrid(i, count, x, y) >= count)
                    return 4;
    std::vector<std::vector<ControlHint>> cases{
        { { "D-pad", "Navigate" }, { "A", "Select" }, { "B", "Park" }, { "L1/R1", "Section" } },
        { { "D-pad", "Choose" },
          { "A", "Select" },
          { "B", "Back" },
          { "L1/R1", "Tab" },
          { "X", "Access" },
          { "SELECT", "Sections", HintPriority::guide } },
        { { "A", "Build" }, { "B", "Undo" }, { "L1/R1", "Piece" }, { "X", "Speed +" }, { "Y", "Speed -" } },
        { { "D-pad", "Move" },
          { "A", "Place" },
          { "B", "Back" },
          { "X", "Remove" },
          { "Y", "Adjust" },
          { "SELECT", "Surface", HintPriority::guide },
          { "L1/R1", "Zoom", HintPriority::secondary } },
        { { "D-pad", "Move" },
          { "A", "Inspect" },
          { "B", "Back" },
          { "SELECT", "Build" },
          { "START", "Park" },
          { "L1/R1", "Zoom", HintPriority::secondary },
          { "X", "Rotate", HintPriority::guide } },
        { { "D-pad", "Choose" },
          { "A", "Select" },
          { "B", "Back" },
          { "L1/R1", "Page" },
          { "START", "Ride", HintPriority::guide },
          { "X", "Camera" } }
    };
    for (const auto& c : cases)
    {
        auto p = MeasureFooter({ 512, 384 }, c);
        std::sort(p.begin(), p.end(), [](auto a, auto b) { return a.x < b.x; });
        int right = 0;
        std::cout << "height=" << FooterHeight({ 512, 384 }, c) << " ";
        for (auto e : p)
        {
            std::cout << e.hint.key << ":" << e.x << ".." << e.x + e.width << " ";
            if (e.x < right || e.x + e.width > 504)
                return 1;
            right = e.x + e.width;
        }
        std::cout << "\n";
        if (FooterHeight({ 512, 384 }, c) != 28)
            return 2;
    }
}

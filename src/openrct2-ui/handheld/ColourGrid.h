// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Panel.h"
#include <openrct2/drawing/Colour.h>
#include <openrct2/drawing/ColourMap.h>

namespace OpenRCT2::Ui::Handheld
{
    inline constexpr size_t kColourGridColumns = 8;

    // Keep spatial movement inside the grid, including its final short row.
    inline size_t MoveColourGrid(size_t current, size_t count, int x, int y)
    {
        if (!count) return 0;
        current = std::min(current, count - 1);
        if (y < 0 && current >= kColourGridColumns) current -= kColourGridColumns;
        if (y > 0 && current / kColourGridColumns < (count - 1) / kColourGridColumns)
            current = std::min(current + kColourGridColumns, count - 1);
        if (x < 0 && current % kColourGridColumns) --current;
        if (x > 0 && current % kColourGridColumns < kColourGridColumns - 1 && current + 1 < count) ++current;
        return current;
    }

    inline void DrawColourGrid(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, const std::vector<uint16_t>& values,
        const std::vector<std::string>& labels, size_t selected, uint16_t current)
    {
        const int cellWidth = (size.width - 24) / kColourGridColumns;
        const int rows = (values.size() + kColourGridColumns - 1) / kColourGridColumns;
        const int pitch = std::min(24, (size.height - 134) / std::max(1, rows));
        DrawSectionLabel(rt, p + ScreenCoordsXY{12, 61}, size.width - 24, "CHOOSE A COLOUR");
        for (size_t i = 0; i < values.size(); ++i)
        {
            const auto cell = p + ScreenCoordsXY{12 + static_cast<int>(i % kColourGridColumns) * cellWidth,
                                               79 + static_cast<int>(i / kColourGridColumns) * pitch};
            const bool focused = i == selected;
            Fill(rt, cell, {cellWidth - 3, pitch - 3}, focused ? Theme::gold : Theme::edge);
            const auto shade = Drawing::getColourMap(static_cast<Drawing::Colour>(values[i]));
            Fill(rt, cell + ScreenCoordsXY{3, 3}, {cellWidth - 9, pitch - 9}, shade.midLight);
            Fill(rt, cell + ScreenCoordsXY{3, pitch - 6}, {cellWidth - 9, 2}, shade.dark);
            if (values[i] == current)
            {
                // A two-tone marker identifies the applied colour without relying on hue.
                Fill(rt, cell + ScreenCoordsXY{4, 4}, {7, 7}, Theme::ink);
                Fill(rt, cell + ScreenCoordsXY{6, 6}, {3, 3}, Theme::text);
            }
        }
        if (selected < labels.size())
            DrawStrongText(rt, p + ScreenCoordsXY{12, size.height - 41}, "{WHITE}" + labels[selected], size.width - 24);
        const auto applied = std::find(values.begin(), values.end(), current);
        if (applied != values.end() && static_cast<size_t>(applied - values.begin()) < labels.size())
            DrawModernEllipsised(rt, p + ScreenCoordsXY{12, size.height - 23}, size.width - 24,
                "{MUTED}Current: " + labels[applied - values.begin()]);
    }
}

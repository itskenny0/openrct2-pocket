// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ModernText.h"

#include <openrct2/drawing/Line.h>

namespace OpenRCT2::Ui::Handheld::Concept
{
    using Drawing::PaletteIndex;
    // The world palette remains untouched. Ordered mixtures of stable entries
    // approximate the reference's blue-navy surfaces in the indexed renderer.
    inline constexpr int kRailWidth = 164;
    inline constexpr int kRowHeight = 28;
    inline constexpr ScreenCoordsXY kOrigin{ 8, 8 };
    inline constexpr uint8_t kBayer[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };

    inline void Box(Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, bool gold = false)
    {
        for (int y = std::max(p.y, rt.y); y < std::min(p.y + size.height, rt.y + rt.height); ++y)
            for (int x = std::max(p.x, rt.x); x < std::min(p.x + size.width, rt.x + rt.width); ++x)
            {
                const int dx = std::min(x - p.x, p.x + size.width - 1 - x);
                const int dy = std::min(y - p.y, p.y + size.height - 1 - y);
                if ((dy == 0 && dx < 3) || (dy == 1 && dx < 1))
                    continue;
                const int d = kBayer[y & 3][x & 3];
                uint8_t colour;
                if (gold)
                    colour = dx == 0 || dy == 0 ? 55 : (y - p.y < 3 ? 54 : 53);
                else if (dx == 0 || dy == 0)
                    colour = d < 8 ? 14 : 121;
                else if (dx == 1 || dy == 1)
                    colour = d < 8 ? 12 : 119;
                else
                {
                    // Low-contrast blue/charcoal mixture avoids the coarse teal texture
                    // produced by alternating saturated palette ramps.
                    const int glow = 2 + (size.height - (y - p.y)) * 3 / size.height;
                    colour = d < 6 ? 118 : d < 6 + glow ? 11 : 10;
                }
                rt.bits[(y - rt.y) * rt.LineStride() + x - rt.x] = static_cast<PaletteIndex>(colour);
            }
    }

    inline void Rule(Drawing::RenderTarget& rt, ScreenCoordsXY p, int width)
    {
        GfxDrawLine(rt, { p, p + ScreenCoordsXY{ width - 1, 0 } }, static_cast<PaletteIndex>(12));
    }

    enum class Icon
    {
        path,
        queue,
        rise,
        lower,
        slope,
        height,
        options,
        remove,
        play,
        ground
    };
    inline void DrawIcon(Drawing::RenderTarget& rt, ScreenCoordsXY p, Icon icon, bool selected = false)
    {
        const auto light = static_cast<PaletteIndex>(selected ? 10 : 20);
        const auto shade = static_cast<PaletteIndex>(selected ? 52 : 14);
        const auto line = [&](int x1, int y1, int x2, int y2, PaletteIndex c) {
            GfxDrawLine(rt, { p + ScreenCoordsXY{ x1, y1 }, p + ScreenCoordsXY{ x2, y2 } }, c);
        };
        if (icon == Icon::path || icon == Icon::queue || icon == Icon::ground)
        {
            for (int y = 0; y < 9; ++y)
            {
                const int half = y <= 4 ? y * 2 : (8 - y) * 2;
                line(8 - half, 3 + y, 8 + half, 3 + y, y < 5 ? light : shade);
            }
            line(1, 9, 8, 13, shade);
            line(8, 13, 15, 9, shade);
            line(2, 7, 8, 10, shade);
            line(8, 4, 14, 7, shade);
            if (icon == Icon::queue)
                line(2, 8, 8, 11, static_cast<PaletteIndex>(selected ? 10 : 120));
        }
        else if (icon == Icon::rise || icon == Icon::lower || icon == Icon::height)
        {
            const bool down = icon == Icon::lower;
            for (int x = 7; x <= 9; ++x)
                line(x, 3, x, 14, light);
            for (int i = 0; i < 5; ++i)
                line(8 - i, down ? 14 - i : 3 + i, 8 + i, down ? 14 - i : 3 + i, light);
        }
        else if (icon == Icon::slope)
        {
            for (int x = 1; x < 16; ++x)
                line(x, 14 - x * 2 / 3, x, 14, shade);
            line(1, 14, 15, 4, light);
        }
        else if (icon == Icon::options)
        {
            for (int y = 4; y <= 12; y += 4)
            {
                line(1, y, 15, y, shade);
                const auto x = y == 8 ? 11 : 5;
                line(x, y - 2, x, y + 2, light);
                line(x + 1, y - 2, x + 1, y + 2, light);
            }
        }
        else if (icon == Icon::remove)
        {
            const auto colour = static_cast<PaletteIndex>(selected ? 10 : 173);
            for (int y = 7; y < 10; ++y)
                line(2, y, 14, y, colour);
        }
        else
            for (int x = 4; x <= 12; ++x)
                line(x, 2 + (x - 4) / 2, x, 14 - (x - 4) / 2, light);
    }

    inline void Heading(Drawing::RenderTarget& rt, ScreenCoordsXY p, const std::string& title)
    {
        DrawStrongText(rt, p, "{WHITE}" + title, 144);
    }
    inline void Row(Drawing::RenderTarget& rt, ScreenCoordsXY p, const std::string& label, Icon icon, bool selected)
    {
        if (selected)
            Box(rt, p, { 148, kRowHeight }, true);
        else
            Rule(rt, p + ScreenCoordsXY{ 2, kRowHeight - 1 }, 144);
        DrawIcon(rt, p + ScreenCoordsXY{ 7, 5 }, icon, selected);
        if (selected)
            DrawStrongText(rt, p + ScreenCoordsXY{ 31, 9 }, "{BLACK}" + label, 110);
        else
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 31, 9 }, 110, "{WHITE}" + label);
    }
} // namespace OpenRCT2::Ui::Handheld::Concept

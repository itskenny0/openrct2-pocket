// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "PocketSansData.h"

#include <algorithm>
#include <array>
#include <openrct2/SpriteIds.h>
#include <openrct2/drawing/Drawing.Sprite.h>
#include <openrct2/drawing/G1Element.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/drawing/Text.h>
#include <string>

namespace OpenRCT2::Ui::Handheld
{
    enum class Ink : uint8_t
    {
        light,
        dark,
        gold
    };

    inline Drawing::PaletteIndex BlendGlyph(Ink ink, Drawing::PaletteIndex background, uint8_t coverage)
    {
        struct Ramp
        {
            bool ready{};
            std::array<Drawing::PaletteIndex, 16> colours{};
        };
        static std::array<std::array<Ramp, 256>, 3> ramps;
        auto& ramp = ramps[static_cast<size_t>(ink)][static_cast<uint8_t>(background)];
        if (!ramp.ready)
        {
            const auto* palette = GfxGetG1Palette(SPR_GAME_DEFAULT_PALETTE);
            if (!palette || static_cast<uint8_t>(background) < palette->startIndex
                || static_cast<uint8_t>(background) >= palette->startIndex + palette->numColours)
                return background;
            const auto bg = palette->palette[static_cast<uint8_t>(background) - palette->startIndex];
            const std::array<int, 3> fg = ink == Ink::dark ? std::array<int, 3>{ 15, 24, 34 }
                : ink == Ink::gold                         ? std::array<int, 3>{ 255, 231, 95 }
                                                           : std::array<int, 3>{ 230, 240, 250 };
            for (int a = 0; a < 16; ++a)
            {
                const int red = (bg.red * (15 - a) + fg[0] * a) / 15;
                const int green = (bg.green * (15 - a) + fg[1] * a) / 15;
                const int blue = (bg.blue * (15 - a) + fg[2] * a) / 15;
                int distance = 3 * 256 * 256, nearest = static_cast<uint8_t>(background);
                for (int i = std::max<int>(10, palette->startIndex);
                     i < std::min<int>(230, palette->startIndex + palette->numColours); ++i)
                {
                    const auto colour = palette->palette[i - palette->startIndex];
                    const int r = red - colour.red, g = green - colour.green, b = blue - colour.blue;
                    const int candidate = r * r + g * g + b * b;
                    if (candidate < distance)
                    {
                        distance = candidate;
                        nearest = i;
                    }
                }
                ramp.colours[a] = static_cast<Drawing::PaletteIndex>(nearest);
            }
            ramp.ready = true;
        }
        return ramp.colours[coverage];
    }

    inline int TextWidth(const std::string& text)
    {
        int width = 0;
        for (const unsigned char c : text)
            if (c >= 32 && c < 127)
                width += PocketSans::kAdvance[c - 32];
        return width;
    }

    inline void DrawModernText(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, std::string text, TextPaint paint = {}, int maxWidth = 0)
    {
        Ink ink = paint.colour.colour == Drawing::Colour::yellow ? Ink::gold : Ink::light;
        for (const auto& prefix : { std::string("{BLACK}"), std::string("{WHITE}"), std::string("{YELLOW}") })
            if (text.starts_with(prefix))
            {
                ink = prefix == "{BLACK}" ? Ink::dark : prefix == "{YELLOW}" ? Ink::gold : Ink::light;
                text.erase(0, prefix.size());
                break;
            }
        // Localised names and native error formatting keep the complete engine fallback.
        if (std::any_of(text.begin(), text.end(), [](unsigned char c) { return c < 32 || c >= 127 || c == '{'; }))
        {
            text = (ink == Ink::dark ? "{BLACK}" : ink == Ink::gold ? "{YELLOW}" : "{WHITE}") + text;
            if (maxWidth > 0)
                drawTextEllipsised(rt, pos, maxWidth, text, paint);
            else
                drawText(rt, pos, text, paint);
            return;
        }
        if (maxWidth > 0 && TextWidth(text) > maxWidth)
        {
            while (!text.empty() && TextWidth(text + "...") > maxWidth)
                text.pop_back();
            text += "...";
        }
        const int width = TextWidth(text);
        if (paint.alignment == TextAlignment::centre)
            pos.x -= width / 2;
        else if (paint.alignment == TextAlignment::right)
            pos.x -= width;
        for (const unsigned char c : text)
        {
            const auto glyph = PocketSans::kCoverage[c - 32];
            for (int y = 0; y < PocketSans::kHeight; ++y)
                for (int x = 0; x < PocketSans::kWidth; ++x)
                {
                    const int px = pos.x + x, py = pos.y + y - 3;
                    if (px < rt.x || py < rt.y || px >= rt.x + rt.width || py >= rt.y + rt.height)
                        continue;
                    const char value = glyph[y * PocketSans::kWidth + x];
                    const uint8_t coverage = value <= '9' ? value - '0' : value - 'a' + 10;
                    if (!coverage)
                        continue;
                    auto& pixel = rt.bits[(py - rt.y) * rt.LineStride() + px - rt.x];
                    pixel = BlendGlyph(ink, pixel, coverage);
                }
            pos.x += PocketSans::kAdvance[c - 32];
        }
    }

    inline void DrawModernEllipsised(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, int width, const std::string& text, TextPaint paint = {})
    {
        DrawModernText(rt, pos, text, paint, width);
    }
} // namespace OpenRCT2::Ui::Handheld

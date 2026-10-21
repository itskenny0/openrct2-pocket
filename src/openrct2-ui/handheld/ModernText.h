// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "PocketPalette.h"
#include "PocketSansData.h"
#include "PocketSansStrongData.h"
#include "PocketSansHeadingData.h"

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
        gold,
        muted,
        positive,
        caution,
        destructive,
        disabled
    };

    inline Drawing::PaletteIndex BlendGlyph(Ink ink, Drawing::PaletteIndex background, uint8_t coverage)
    {
        struct Ramp
        {
            bool ready{};
            std::array<Drawing::PaletteIndex, 16> colours{};
        };
        static std::array<std::array<std::array<Ramp, 256>, 8>, 2> ramps;
        auto& ramp = ramps[PocketPalette::style == PocketPalette::Style::daylight ? 1 : 0][static_cast<size_t>(ink)][static_cast<uint8_t>(background)];
        if (!ramp.ready)
        {
            const auto* palette = GfxGetG1Palette(SPR_GAME_DEFAULT_PALETTE);
            if (!palette || static_cast<uint8_t>(background) < palette->startIndex
                || static_cast<uint8_t>(background) >= palette->startIndex + palette->numColours)
                return background;
            const auto bg = palette->palette[static_cast<uint8_t>(background) - palette->startIndex];
            const auto foreground = ink == Ink::caution ? PocketPalette::caution
                : ink == Ink::destructive ? PocketPalette::destructive
                : ink == Ink::disabled ? PocketPalette::disabled
                : ink == Ink::positive ? PocketPalette::positive
                : ink == Ink::muted                      ? PocketPalette::secondary
                : ink == Ink::dark                       ? PocketPalette::dark
                : ink == Ink::gold                       ? PocketPalette::accent
                                                         : PocketPalette::primary;
            const auto fg = palette->palette[foreground - palette->startIndex];
            for (int a = 0; a < 16; ++a)
            {
                const int red = (bg.red * (15 - a) + fg.red * a) / 15;
                const int green = (bg.green * (15 - a) + fg.green * a) / 15;
                const int blue = (bg.blue * (15 - a) + fg.blue * a) / 15;
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
            ramp.colours[0] = background;
            ramp.colours[15] = static_cast<Drawing::PaletteIndex>(foreground);
            ramp.ready = true;
        }
        return ramp.colours[coverage];
    }

    inline int TextWidth(const std::string& text, bool strong = false, bool heading = false)
    {
        int width = 0;
        for (const unsigned char c : text)
            if (c >= 32 && c < 127)
                width += heading ? PocketSansHeading::kAdvance[c - 32] : strong ? PocketSansStrong::kAdvance[c - 32] : PocketSans::kAdvance[c - 32];
        return width;
    }

    inline void DrawModernText(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, std::string text, TextPaint paint = {}, int maxWidth = 0,
        bool strong = false, bool heading = false)
    {
        Ink ink = paint.colour.colour == Drawing::Colour::yellow ? Ink::gold : Ink::light;
        for (const auto& prefix : { std::string("{BLACK}"), std::string("{WHITE}"), std::string("{YELLOW}"),
                                    std::string("{CAUTION}"), std::string("{DANGER}"), std::string("{DISABLED}"),
                                    std::string("{MUTED}"), std::string("{POSITIVE}"), std::string("{GOLD}") })
            if (text.starts_with(prefix))
            {
                ink = prefix == "{CAUTION}" ? Ink::caution
                    : prefix == "{DANGER}" ? Ink::destructive
                    : prefix == "{DISABLED}" ? Ink::disabled
                    : prefix == "{POSITIVE}"                     ? Ink::positive
                    : prefix == "{BLACK}"                        ? Ink::dark
                    : prefix == "{YELLOW}" || prefix == "{GOLD}" ? Ink::gold
                    : prefix == "{MUTED}"                        ? Ink::muted
                                                                 : Ink::light;
                text.erase(0, prefix.size());
                break;
            }
        // Localised names and native error formatting keep the complete engine fallback.
        if (std::any_of(text.begin(), text.end(), [](unsigned char c) { return c < 32 || c >= 127 || c == '{'; }))
        {
            // Localised currency and names use native glyphs too. Match the
            // light theme's ink polarity instead of forcing white on ivory.
            const char* nativeInk = PocketPalette::style == PocketPalette::Style::daylight
                ? (ink == Ink::dark ? "{WHITE}" : ink == Ink::destructive ? "{RED}"
                    : "{BLACK}")
                : (ink == Ink::dark ? "{BLACK}" : ink == Ink::gold ? "{YELLOW}" : "{WHITE}");
            text = nativeInk + text;
            if (maxWidth > 0)
                drawTextEllipsised(rt, pos, maxWidth, text, paint);
            else
                drawText(rt, pos, text, paint);
            return;
        }
        if (maxWidth > 0 && TextWidth(text, strong, heading) > maxWidth)
        {
            while (!text.empty() && TextWidth(text + "...", strong, heading) > maxWidth)
                text.pop_back();
            text += "...";
        }
        const int width = TextWidth(text, strong, heading);
        if (paint.alignment == TextAlignment::centre)
            pos.x -= width / 2;
        else if (paint.alignment == TextAlignment::right)
            pos.x -= width;
        for (const unsigned char c : text)
        {
            const auto glyph = heading ? PocketSansHeading::kCoverage[c - 32] : strong ? PocketSansStrong::kCoverage[c - 32] : PocketSans::kCoverage[c - 32];
            const auto glyphWidth = heading ? PocketSansHeading::kWidth : strong ? PocketSansStrong::kWidth : PocketSans::kWidth;
            const auto glyphHeight = heading ? PocketSansHeading::kHeight : PocketSans::kHeight;
            for (int y = 0; y < glyphHeight; ++y)
                for (int x = 0; x < glyphWidth; ++x)
                {
                    const int px = pos.x + x, py = pos.y + y - 3;
                    if (px < rt.x || py < rt.y || px >= rt.x + rt.width || py >= rt.y + rt.height)
                        continue;
                    const char value = glyph[y * glyphWidth + x];
                    const uint8_t coverage = value <= '9' ? value - '0' : value - 'a' + 10;
                    if (!coverage)
                        continue;
                    auto& pixel = rt.bits[(py - rt.y) * rt.LineStride() + px - rt.x];
                    pixel = BlendGlyph(ink, pixel, coverage);
                }
            pos.x += heading ? PocketSansHeading::kAdvance[c - 32] : strong ? PocketSansStrong::kAdvance[c - 32] : PocketSans::kAdvance[c - 32];
        }
    }

    inline void DrawHeadingText(Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& text, int maxWidth)
    {
        DrawModernText(rt, pos, text, {}, maxWidth, true, true);
    }

    inline void DrawStrongText(Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& text, int maxWidth = 0)
    {
        DrawModernText(rt, pos, text, {}, maxWidth, true);
    }

    inline void DrawModernEllipsised(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, int width, const std::string& text, TextPaint paint = {})
    {
        DrawModernText(rt, pos, text, paint, width);
    }

    // Button labels centre the visible ink, rather than the font advance/baseline.
    inline void DrawButtonLabel(Drawing::RenderTarget& rt, ScreenCoordsXY pos, ScreenSize size, const std::string& text)
    {
        int left = 10000, top = 10000, right = -1, bottom = -1, advance = 0;
        for (const unsigned char c : text)
        {
            if (c < 32 || c >= 127)
                return;
            const auto glyph = PocketSans::kCoverage[c - 32];
            for (int y = 0; y < PocketSans::kHeight; ++y)
                for (int x = 0; x < PocketSans::kWidth; ++x)
                    if (glyph[y * PocketSans::kWidth + x] != '0')
                    {
                        left = std::min(left, advance + x);
                        right = std::max(right, advance + x);
                        top = std::min(top, y);
                        bottom = std::max(bottom, y);
                    }
            advance += PocketSans::kAdvance[c - 32];
        }
        if (right < left)
            return;
        DrawModernText(
            rt,
            pos
                + ScreenCoordsXY{ (size.width - (right - left + 1)) / 2 - left,
                                  (size.height - (bottom - top + 1)) / 2 - top + 3 },
            "{BLACK}" + text);
    }
} // namespace OpenRCT2::Ui::Handheld

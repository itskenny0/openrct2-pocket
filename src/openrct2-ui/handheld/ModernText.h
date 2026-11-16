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
#include <optional>
#include <string>
#include <vector>
#include <string_view>
#include <utility>

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

    // Pocket Sans covers printable ASCII. Typographic punctuation in scenario and object names
    // (e.g. the right single quote in scenario names like Katie's Dreamland) would otherwise send the whole
    // string to the engine sprite font, so map it to the closest ASCII form first.
    inline std::string PlainPunctuation(std::string text)
    {
        static constexpr std::pair<std::string_view, std::string_view> kMap[] = {
            { "\xE2\x80\x98", "'" }, { "\xE2\x80\x99", "'" }, { "\xE2\x80\x9C", "\"" }, { "\xE2\x80\x9D", "\"" },
            { "\xE2\x80\x93", "-" }, { "\xE2\x80\x94", "-" }, { "\xE2\x80\xA6", "..." }, { "\xC2\xA0", " " },
        };
        if (std::all_of(text.begin(), text.end(), [](unsigned char c) { return c < 128; }))
            return text;
        for (const auto& [from, to] : kMap)
            for (auto at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size()))
                text.replace(at, from.size(), to);
        return text;
    }

    // Glyph index for a code point: 0-94 printable ASCII, then the extra glyphs (currency etc.).
    inline int GlyphIndex(char32_t code)
    {
        if (code >= 32 && code < 127)
            return static_cast<int>(code - 32);
        for (size_t i = 0; i < PocketSans::kExtraCodes.size(); ++i)
            if (PocketSans::kExtraCodes[i] == code)
                return 95 + static_cast<int>(i);
        return -1;
    }
    // Decodes UTF-8 into glyph indices; empty optional if any character has no Pocket glyph.
    inline std::optional<std::vector<int>> Glyphs(const std::string& text)
    {
        std::vector<int> glyphs;
        glyphs.reserve(text.size());
        for (size_t i = 0; i < text.size();)
        {
            const auto lead = static_cast<unsigned char>(text[i]);
            const int length = lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2 : (lead >> 4) == 0xE ? 3 : (lead >> 3) == 0x1E ? 4 : 0;
            if (length == 0 || i + length > text.size())
                return std::nullopt;
            char32_t code = length == 1 ? lead : lead & (0x7F >> length);
            for (int k = 1; k < length; ++k)
                code = (code << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3F);
            const int index = GlyphIndex(code);
            if (index < 0)
                return std::nullopt;
            glyphs.push_back(index);
            i += length;
        }
        return glyphs;
    }
    inline int GlyphAdvance(int index, bool strong, bool heading)
    {
        if (index < 95)
            return heading ? PocketSansHeading::kAdvance[index] : strong ? PocketSansStrong::kAdvance[index] : PocketSans::kAdvance[index];
        const auto extra = static_cast<size_t>(index - 95);
        return heading ? PocketSansHeading::kExtraAdvance[extra] : strong ? PocketSansStrong::kExtraAdvance[extra] : PocketSans::kExtraAdvance[extra];
    }
    inline const char* GlyphCoverage(int index, bool strong, bool heading)
    {
        if (index < 95)
            return heading ? PocketSansHeading::kCoverage[index] : strong ? PocketSansStrong::kCoverage[index] : PocketSans::kCoverage[index];
        const auto extra = static_cast<size_t>(index - 95);
        return heading ? PocketSansHeading::kExtraCoverage[extra] : strong ? PocketSansStrong::kExtraCoverage[extra] : PocketSans::kExtraCoverage[extra];
    }
    // Removes the last whole UTF-8 character.
    inline void PopCharacter(std::string& text)
    {
        while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80)
            text.pop_back();
        if (!text.empty())
            text.pop_back();
    }

    inline int TextWidth(const std::string& rawText, bool strong = false, bool heading = false)
    {
        const auto text = PlainPunctuation(rawText);
        int width = 0;
        if (const auto glyphs = Glyphs(text))
        {
            for (const int index : *glyphs)
                width += GlyphAdvance(index, strong, heading);
            return width;
        }
        for (const unsigned char c : text)
            if (c >= 32 && c < 127)
                width += GlyphAdvance(c - 32, strong, heading);
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
        text = PlainPunctuation(std::move(text));
        // Localised names and native error formatting keep the complete engine fallback; currency
        // symbols and the other extra glyphs stay in Pocket Sans.
        if (text.find('{') != std::string::npos || !Glyphs(text))
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
                PopCharacter(text);
            text += "...";
        }
        const int width = TextWidth(text, strong, heading);
        if (paint.alignment == TextAlignment::centre)
            pos.x -= width / 2;
        else if (paint.alignment == TextAlignment::right)
            pos.x -= width;
        for (const int index : Glyphs(text).value_or(std::vector<int>{}))
        {
            const auto glyph = GlyphCoverage(index, strong, heading);
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
            pos.x += GlyphAdvance(index, strong, heading);
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

// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/SpriteIds.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/Font.h>
#include <openrct2/drawing/G1Element.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/world/Location.hpp>
#include <string>

// The Pixel theme's text: RCT2's own bitmap font, read straight from g1.dat. The glyph sprites mark
// the letter body with palette index 1 (2 is an outline, 3 a drop shadow), so we paint the body in
// any ink and optionally a one-pixel shadow, at 1x for body text or 2x for big titles. Every pixel
// lands on the game's grid, which is what makes it read as a handheld game.
namespace OpenRCT2::Ui::Handheld::PixelFont
{
    // Set by the game (Explorer.cpp) to g1.dat's sprite lookup. Standalone layout tests link without
    // the engine, so the font measures with a fixed advance there instead of pulling it in.
    inline const OpenRCT2::G1Element* (*glyphSource)(uint32_t) = nullptr;
    inline const OpenRCT2::G1Element* Glyph(uint32_t sprite)
    {
        return glyphSource ? glyphSource(sprite) : nullptr;
    }
    enum class Style : uint8_t
    {
        regular, // RCT2 "small" (the normal window font)
        bold     // RCT2 "medium" (window titles)
    };
    inline uint32_t Base(Style style)
    {
        return SPR_CHAR_START + (style == Style::bold ? 1 : 0) * kSpriteFontGlyphCount;
    }
    // Decodes one UTF-8 character and maps it onto the RCT2 sprite font, which follows Latin-1 for
    // the common symbols (pound, yen, degrees) and has its own slot for the euro.
    inline int GlyphIndex(const std::string& text, size_t& at)
    {
        const auto c = static_cast<unsigned char>(text[at]);
        char32_t code = c;
        if (c >= 0xC0 && at + 1 < text.size())
        {
            const int extra = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1;
            code = c & (0x3F >> extra);
            for (int i = 0; i < extra && at + 1 < text.size(); ++i)
                code = (code << 6) | (static_cast<unsigned char>(text[++at]) & 0x3F);
        }
        ++at;
        if (code == 0x20AC)
            code = 0xB5; // euro
        else if (code == 0x2026)
            code = '.';
        else if (code == 0x2212)
            code = '-';
        if (code < 32 || code >= 32 + kSpriteFontGlyphCount)
            code = '?';
        return static_cast<int>(code - 32);
    }
    // RCT2's font has no '|', which the menus use as a separator ("Handyman | Walking"). The small
    // font's 'I' is a plain one-pixel bar, so '|' borrows it with a pixel of air each side.
    constexpr int kBarIndex = '|' - 32;
    constexpr int kCapIIndex = 'I' - 32;
    inline int Advance(Style style, int index)
    {
        if (index == kBarIndex)
            return Advance(Style::regular, kCapIIndex) + 2;
        if (index == 0)
            return style == Style::bold ? 5 : 4; // space
        const auto* glyph = Glyph(Base(style) + index);
        return glyph ? glyph->width - 1 + (style == Style::bold ? 1 : 0) : 4;
    }
    inline int Width(const std::string& text, Style style, int scale = 1)
    {
        int width = 0;
        for (size_t at = 0; at < text.size();)
            width += Advance(style, GlyphIndex(text, at));
        return width * scale;
    }
    inline int Height(int scale = 1)
    {
        return 10 * scale; // Both RCT2 sprite fonts are 10 px from cap line to descender.
    }
    inline void DrawGlyph(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, Style style, int index, Drawing::PaletteIndex ink, int scale)
    {
        const auto* glyph = Glyph(Base(style) + index);
        if (!glyph || !glyph->offset)
            return;
        for (int y = 0; y < glyph->height; ++y)
            for (int x = 0; x < glyph->width; ++x)
            {
                if (glyph->offset[y * glyph->width + x] != 1)
                    continue;
                for (int sy = 0; sy < scale; ++sy)
                    for (int sx = 0; sx < scale; ++sx)
                    {
                        const int px = pos.x + (x + glyph->xOffset) * scale + sx;
                        const int py = pos.y + (y + glyph->yOffset) * scale + sy;
                        if (px < rt.x || py < rt.y || px >= rt.x + rt.width || py >= rt.y + rt.height)
                            continue;
                        rt.bits[(py - rt.y) * rt.LineStride() + px - rt.x] = ink;
                    }
            }
    }
    inline void Draw(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& text, Style style, Drawing::PaletteIndex ink,
        int scale = 1)
    {
        for (size_t at = 0; at < text.size();)
        {
            const int index = GlyphIndex(text, at);
            if (index == kBarIndex)
                DrawGlyph(rt, pos + ScreenCoordsXY{ scale, 0 }, Style::regular, kCapIIndex, ink, scale);
            else if (index > 0)
                DrawGlyph(rt, pos, style, index, ink, scale);
            pos.x += Advance(style, index) * scale;
        }
    }
} // namespace OpenRCT2::Ui::Handheld::PixelFont

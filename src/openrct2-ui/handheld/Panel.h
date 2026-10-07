// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2-ui/interface/Window.h>
#include <openrct2/drawing/ColourMap.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/Rectangle.h>
#include <openrct2/drawing/Text.h>

namespace OpenRCT2::Ui::Handheld
{
    inline constexpr int32_t kPanelRowPitch = 32;

    inline void DrawSurface(Drawing::RenderTarget& rt, ScreenCoordsXY pos, ScreenSize size)
    {
        const auto blue = Drawing::getColourMap(Drawing::Colour::grey);
        Drawing::Rectangle::fill(rt, { pos, pos + ScreenCoordsXY{ size.width - 1, size.height - 1 } }, blue.midDark);
        Drawing::Rectangle::fill(
            rt, { pos + ScreenCoordsXY{ 1, 1 }, pos + ScreenCoordsXY{ size.width - 2, size.height - 2 } },
            Drawing::getColourMap(Drawing::Colour::grey).darkest);
    }

    inline void DrawPanelHeading(Drawing::RenderTarget& rt, const Window& window, const std::string& title)
    {
        DrawSurface(rt, window.windowPos, { window.width, window.height });
        const auto blue = Drawing::getColourMap(Drawing::Colour::grey);
        Drawing::Rectangle::fill(
            rt, { window.windowPos + ScreenCoordsXY{ 1, 1 }, window.windowPos + ScreenCoordsXY{ window.width - 2, 35 } },
            blue.darker);
        drawTextEllipsised(
            rt, window.windowPos + ScreenCoordsXY{ 16, 13 }, window.width - 32, title, { Drawing::Colour::white });
    }

    // Filled focus, inset rows and consistent clipping at the physical UI size.
    inline void DrawPanelRow(
        Drawing::RenderTarget& rt, const Window& window, int32_t y, const std::string& label, bool selected)
    {
        Drawing::Rectangle::fill(
            rt, { window.windowPos.x + 10, y - 8, window.windowPos.x + window.width - 11, y + 19 },
            selected ? Drawing::getColourMap(Drawing::Colour::yellow).light
                     : Drawing::getColourMap(Drawing::Colour::grey).darker);
        if (selected)
            drawText(rt, { window.windowPos.x + 17, y }, "{BLACK}>", { Drawing::Colour::black });
        drawTextEllipsised(
            rt, { window.windowPos.x + 30, y }, window.width - 46, (selected ? "{BLACK}" : "{WHITE}") + label,
            { selected ? Drawing::Colour::black : Drawing::Colour::white, FontStyle::medium, TextAlignment::left,
              selected ? TextDarkness::extraDark : TextDarkness::regular });
    }

    inline void DrawCommand(Drawing::RenderTarget& rt, ScreenCoordsXY pos, int32_t keyWidth, const char* key, const char* label)
    {
        const auto blue = Drawing::getColourMap(Drawing::Colour::grey);
        Drawing::Rectangle::fill(rt, { pos.x, pos.y - 3, pos.x + keyWidth - 1, pos.y + 12 }, blue.midDark);
        drawText(rt, { pos.x + keyWidth / 2, pos.y }, key, { Drawing::Colour::white, TextAlignment::centre });
        drawText(rt, { pos.x + keyWidth + 5, pos.y }, label, { Drawing::Colour::white });
    }

    inline void DrawPanelCommands(Drawing::RenderTarget& rt, const Window& window, const char* backLabel = "Back")
    {
        const auto pos = window.windowPos + ScreenCoordsXY{ 1, window.height - 29 };
        DrawSurface(rt, pos, { window.width - 2, 28 });
        const auto y = pos.y + 8;
        DrawCommand(rt, { pos.x + 12, y }, 38, "D-pad", "Choose");
        DrawCommand(rt, { pos.x + 139, y }, 17, "A", "Select");
        DrawCommand(rt, { pos.x + 238, y }, 17, "B", backLabel);
    }
} // namespace OpenRCT2::Ui::Handheld

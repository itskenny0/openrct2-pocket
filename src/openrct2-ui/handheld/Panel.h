// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2-ui/interface/Window.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/Rectangle.h>
#include <openrct2/drawing/Text.h>

namespace OpenRCT2::Ui::Handheld
{
    // Shared controller panel styling. All coordinates are logical UI pixels.
    inline void DrawPanelRow(
        Drawing::RenderTarget& rt, const Window& window, int32_t y, const std::string& label, bool selected)
    {
        if (selected)
            Drawing::Rectangle::fill(
                rt, { window.windowPos.x + 12, y - 5, window.windowPos.x + window.width - 13, y + 20 },
                Drawing::PaletteIndex::pi21);
        drawText(rt, { window.windowPos.x + 24, y }, label, { selected ? Drawing::Colour::black : Drawing::Colour::white });
    }
} // namespace OpenRCT2::Ui::Handheld

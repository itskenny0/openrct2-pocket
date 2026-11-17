// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Panel.h"
namespace OpenRCT2::Ui::Handheld
{
    // Presentation only: progress reflects the caller's actual construction stage.
    inline void DrawPlacementSteps(Drawing::RenderTarget& rt, ScreenCoordsXY p, int width, size_t active)
    {
        // The segmented track the mode switches use, with finished steps underlined in green: a
        // step indicator, read left to right.
        DrawSegmented(rt, p, { width - 3, 24 }, { "Ride", "Entry", "Exit" }, active);
        const int cell = (width - 7) / 3;
        for (size_t i = 0; i < active && i < 3; ++i)
            Fill(
                rt, p + ScreenCoordsXY{ 2 + static_cast<int>(i) * cell + 6, 19 }, { cell - 12, 2 },
                static_cast<Drawing::PaletteIndex>(PocketPalette::positive));
    }
    inline void DrawConstructionOverlay(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, const std::string& title, const std::string& object,
        const std::string& mode, const std::string& geometry, const std::string& height,
        Concept::Icon icon = Concept::Icon::tree)
    {
        using namespace Concept;
        Box(rt, p, { 164, 174 });
        Heading(rt, p + ScreenCoordsXY{ 12, 12 }, title);
        Row(rt, p + ScreenCoordsXY{ 8, 42 }, object, icon, true);
        DrawModernEllipsised(rt, p + ScreenCoordsXY{ 12, 80 }, 140, "{WHITE}" + mode);
        DrawModernEllipsised(rt, p + ScreenCoordsXY{ 12, 106 }, 140, "{MUTED}" + geometry);
        DrawModernEllipsised(rt, p + ScreenCoordsXY{ 12, 132 }, 140, "{MUTED}" + height);
    }
    inline void DrawConstructionFooter(
        Drawing::RenderTarget& rt, ScreenSize screen, const std::string& apply, const std::string& options)
    {
        DrawFooter(
            rt, screen,
            { { "D-pad", "Move" },
              { "A", apply },
              { "Y", options },
              { "L1/R1", "Zoom", HintPriority::guide },
              { "B", "Back" } });
    }
} // namespace OpenRCT2::Ui::Handheld

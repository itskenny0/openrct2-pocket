// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Panel.h"
namespace OpenRCT2::Ui::Handheld
{
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
        DrawFooter(rt, screen, { { "D-pad", "Move" }, { "A", apply }, { "Y", options }, { "L1/R1", "Zoom" }, { "B", "Back" } });
    }
} // namespace OpenRCT2::Ui::Handheld

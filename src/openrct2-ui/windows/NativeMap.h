// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/interface/ScreenCoords.hpp>
#include <openrct2/world/Location.hpp>
#include <optional>
#include <vector>
namespace OpenRCT2::Ui::Windows
{
    struct NativeMiniMapMarker
    {
        TileCoordsXY position;
        Drawing::PaletteIndex colour;
    };
    std::vector<NativeMiniMapMarker> NativeMiniMapPeepMarkers(bool staff, bool tracked, int role);
    // Shared with the desktop map; uses native colours, ownership and camera rotation.
    std::array<Drawing::PaletteIndex, 2> NativeMiniMapTileColour(CoordsXY, bool rides);
    TileCoordsXY NativeMiniMapCoords(CoordsXY);
    std::optional<std::array<ScreenCoordsXY, 2>> NativeMiniMapViewportBounds();
} // namespace OpenRCT2::Ui::Windows

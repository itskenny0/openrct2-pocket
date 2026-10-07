/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once
#include <openrct2/Identifiers.h>

namespace OpenRCT2::Drawing
{
    struct RenderTarget;
}

namespace OpenRCT2::Ui::Handheld
{
    class ControllerInput;
    void UpdateExplorer(ControllerInput& input);
    void EndTerrain();
    void BeginTerrain(bool raise);
    void DrawExplorer(Drawing::RenderTarget& rt);
} // namespace OpenRCT2::Ui::Handheld

namespace OpenRCT2::Ui::Windows
{
    void HandheldRideOpen(RideId ride);
    void HandheldPathOptionsOpen();
} // namespace OpenRCT2::Ui::Windows

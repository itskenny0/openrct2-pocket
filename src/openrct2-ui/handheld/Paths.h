// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/actions/GameActionResult.h>
#include <openrct2/interface/ScreenCoords.hpp>
#include <openrct2/world/Location.hpp>
#include <optional>
#include <string>
namespace OpenRCT2::Ui::Handheld::Paths
{
    void Begin(bool queue, bool remove = false);
    void End();
    bool IsActive();
    bool IsRemoving();
    bool IsQueue();
    void ToggleRemove();
    void CycleSurface();
    void UpdateTarget(ScreenCoordsXY cursor);
    std::optional<CoordsXY> TargetTile();
    GameActions::Result Action(bool execute);
    std::string SurfaceName();
} // namespace OpenRCT2::Ui::Handheld::Paths

// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/actions/GameActionResult.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/interface/ScreenCoords.hpp>
#include <openrct2/world/Location.hpp>
#include <optional>
#include <string>
namespace OpenRCT2
{
    struct InteractionInfo;
}
namespace OpenRCT2::Ui::Handheld::Paths
{
    void Begin(bool queue, bool remove = false);
    void BeginConnected(bool queue);
    bool IsConnected();
    bool ConnectedReady();
    bool TurnConnected();
    bool SlopeConnected(int delta);
    void End();
    bool IsActive();
    void UpdatePreview();
    void ClearPreview();
    void RemovePreviewTemporarily();
    void RestorePreview();
    bool IsRemoving();
    bool IsQueue();
    void ToggleRemove();
    void CycleSurface();
    void FollowGround();
    bool ChangeHeight(int direction);
    bool SetPlacementHeight(int32_t z);
    void UseHeightPlane(bool enabled, std::optional<CoordsXY> source = {});
    bool CycleSlope();
    bool RotateSlope();
    void DrawOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY position);
    std::string PlacementName();
    void UpdateTarget(ScreenCoordsXY cursor);
    void UpdateTargetFromInfo(const InteractionInfo& info);
    std::optional<CoordsXY> TargetTile();
    GameActions::Result Action(bool execute);
    std::string SurfaceName();
} // namespace OpenRCT2::Ui::Handheld::Paths

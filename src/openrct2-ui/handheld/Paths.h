// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "RideManagement.h"
#include "PathSelection.h"

#include <array>
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
    BuildMode Mode();
    bool CycleMode(int delta);
    bool CancelSelection();
    bool SelectionOwnsInput();
    bool BatchActive();
    std::string BatchMessage();
    void DrawBatchWorld(Drawing::RenderTarget& rt);
    void DrawFooter(Drawing::RenderTarget& rt, ScreenSize screen);
    void Begin(bool queue, bool remove = false);
    void BeginConnected(bool queue);
    bool BeginConnectedAccess(const RideManagement::Target&, StationIndex, bool exit, CoordsXYZD outside);
    struct UndoDraft
    {
        CoordsXYZ position;
        std::array<uint8_t, 16> baseline;
        uint64_t revision;
        money64 cost, cash;
    };
    std::optional<UndoDraft> BeginUndo();
    GameActions::Result Undo(const UndoDraft&, bool execute);
    bool IsConnected();
    bool ConnectedReady();
    bool TurnConnected(int delta = 1);
    bool LevelConnected();
    bool IsAdjusting();
    void SetAdjusting(bool enabled);
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
    bool RotateSlope(int delta = 1);
    void DrawOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY position);
    std::string PlacementName();
    void UpdateTarget(ScreenCoordsXY cursor);
    void UpdateTargetFromInfo(const InteractionInfo& info);
    std::optional<CoordsXY> TargetTile();
    GameActions::Result Action(bool execute);
    std::string SurfaceName();
} // namespace OpenRCT2::Ui::Handheld::Paths

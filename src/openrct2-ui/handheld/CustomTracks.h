// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"
#include "RideManagement.h"

#include <openrct2/ride/ted/TrackElemType.h>
namespace OpenRCT2::Ui::Handheld::CustomTracks
{
    bool Eligible(ObjectEntryIndex index);
    std::vector<PickerEntry> RideEntries();
    bool Begin(ObjectEntryIndex index);
    bool CanResume(const RideManagement::Target& target);
    bool Resume(const RideManagement::Target& target);
    void End();
    bool Active();
    bool Busy();
    bool StationPlacement();
    bool CameraMode();
    void ToggleCamera();
    void Rotate();
    bool Select(TrackElemType type);
    void Cycle(int direction);
    void Slope(int direction);
    void Bank();
    void Chain();
    std::vector<PickerEntry> Pieces();
    void DrawPiecePreview(Drawing::RenderTarget& rt, TrackElemType type);
    TrackElemType Selection();
    std::optional<CoordsXYZD> Endpoint();
    std::optional<RideManagement::Target> Target();
    std::optional<RideManagement::Target> Completed();
    GameActions::Result Action(bool execute);
    GameActions::Result Undo(bool execute = true);
    void Update(std::optional<CoordsXY> tile);
    void ClearPreview();
    void RemovePreviewTemporarily();
    void RestorePreview();
    void DrawPanel(Drawing::RenderTarget& rt, ScreenSize screen);
} // namespace OpenRCT2::Ui::Handheld::CustomTracks

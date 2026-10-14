// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"
#include "RideManagement.h"
namespace OpenRCT2::Ui::Handheld::Mazes
{
    bool Eligible(ObjectEntryIndex index);
    std::vector<PickerEntry> Entries();
    bool Begin(ObjectEntryIndex index);
    bool Resume(const RideManagement::Target& target);
    bool CanResume(const RideManagement::Target& target);
    void End();
    void ClearPreview();
    void RemovePreviewTemporarily();
    void RestorePreview();
    bool Active();
    bool Busy();
    bool Reviewing();
    bool CameraMode();
    void ToggleCamera();
    bool Picking();
    GameActions::Result SelectCell(bool execute);
    void Turn(int delta);
    void Height(int delta);
    void Update(std::optional<CoordsXY> tile);
    void Cancel();
    void ChooseRemoval();
    GameActions::Result Build(bool execute);
    GameActions::Result Move();
    GameActions::Result ReviewRemoval();
    GameActions::Result Confirm();
    std::optional<RideManagement::Target> Target();
    std::optional<CoordsXYZD> Cursor();
    void DrawPanel(Drawing::RenderTarget& rt, ScreenSize screen);
} // namespace OpenRCT2::Ui::Handheld::Mazes

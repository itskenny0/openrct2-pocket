// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"
#include "RideManagement.h"

#include <openrct2/ride/TrackDesign.h>
namespace OpenRCT2::Ui::Handheld::Designs
{
    bool Eligible(const TrackDesign& design);
    std::vector<PickerEntry> Entries();
    bool Begin(const std::string& path);
    void End();
    bool Active();
    bool Busy();
    const std::string& Selection();
    Direction Orientation();
    void Rotate();
    void SetScenery(bool value);
    std::string SceneryNotice();
    void ClearPreview();
    void UpdatePreview(std::optional<CoordsXY> tile);
    void RemovePreviewTemporarily();
    void RestorePreview();
    GameActions::Result Query(std::optional<CoordsXY> tile);
    bool Review(std::optional<CoordsXY> tile);
    bool HasReview();
    void CancelReview();
    GameActions::Result Confirm();
    money64 ReviewCost();
    std::optional<CoordsXYZD> ReviewLocation();
    std::optional<RideManagement::Target> Completed();
    void DrawPanel(Drawing::RenderTarget& rt, ScreenSize screen);
} // namespace OpenRCT2::Ui::Handheld::Designs

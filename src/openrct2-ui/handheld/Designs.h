// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"
#include "RideManagement.h"

#include <openrct2/ride/TrackDesign.h>
namespace OpenRCT2::Ui::Handheld::Designs
{
    bool Eligible(const TrackDesign& design);
    std::string UnavailableReason(const TrackDesign& design);
    std::vector<PickerEntry> Entries();
    std::optional<PickerEntry> FileEntry(const std::string& path);
    bool BrowserReturn();
    void SetBrowserReturn(bool value);
    bool Begin(const std::string& path);
    void End();
    bool Active();
    bool Busy();
    const std::string& Selection();
    Direction Orientation();
    void Rotate();
    bool Height(int direction);
    void AutomaticHeight();
    std::optional<int32_t> HeightOffset();
    std::string HeightNotice();
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

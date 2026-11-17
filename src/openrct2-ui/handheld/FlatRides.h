// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"
#include "RideManagement.h"

#include <openrct2/ride/FlatRidePlacement.h>
namespace OpenRCT2::Ui::Handheld::RideAccess
{
    struct ExistingPoint;
}
namespace OpenRCT2::Ui::Handheld::FlatRides
{
    enum class Stage
    {
        inactive,
        ride,
        entrance,
        exit
    };
    bool Eligible(ObjectEntryIndex index);
    std::vector<PickerEntry> Entries();
    std::string Name();
    bool Begin(ObjectEntryIndex index);
    bool Resume(const RideManagement::Target& target);
    bool ResumeExisting(const RideAccess::ExistingPoint& point);
    bool CanResume(const RideManagement::Target& target);
    void End();
    Stage CurrentStage();
    ObjectEntryIndex Selection();
    Direction Orientation();
    bool Busy();
    std::optional<RideManagement::Target> Target();
    std::optional<CoordsXYZD> EntranceLocation(std::optional<CoordsXY> tile);
    void Rotate();
    GameActions::Result Action(std::optional<CoordsXY> tile, bool execute);
    // Y while placing access: entrance, exit and their paths in one go (see AutoAccess). The
    // summary is what was built, or why nothing was.
    GameActions::Result AutoPlace(std::string& summary);
    void UpdatePreview(std::optional<CoordsXY> tile);
    void ClearPreview();
    void RemovePreviewTemporarily();
    void RestorePreview();
    void DrawPanel(Drawing::RenderTarget& rt, ScreenSize screen);
} // namespace OpenRCT2::Ui::Handheld::FlatRides

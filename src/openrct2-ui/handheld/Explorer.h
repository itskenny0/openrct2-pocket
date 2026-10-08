/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once
#include <openrct2/Identifiers.h>
#include <openrct2/object/ObjectTypes.h>
#include <openrct2/world/Location.hpp>

namespace OpenRCT2::Drawing
{
    struct RenderTarget;
}

namespace OpenRCT2::Ui::Handheld
{
    class ControllerInput;
    struct ManagementPresentation;
    struct Frame;
    bool HandleMenuInput(ControllerInput& input, const Frame& frame);
    void UpdateExplorer(ControllerInput& input);
    void EndTerrain();
    void BeginTerrain(bool raise);
    void BeginWater(bool raise = true);
    void BeginSurface();
    void BeginTrees();
    bool IsShopToolActive();
    ObjectEntryIndex ShopSelection();
    Direction ShopDirection();
    bool SetShopSelection(ObjectEntryIndex index);
    void RotateShop();
    bool IsTreeToolActive();
    ObjectEntryIndex TreeSelection();
    bool SetTreeSelection(ObjectEntryIndex index);
    void BeginSceneryRemoval();
    void DrawExplorer(Drawing::RenderTarget& rt);
} // namespace OpenRCT2::Ui::Handheld

namespace OpenRCT2::Ui::Windows
{
    void HandheldRideOpen(RideId ride);
    Handheld::ManagementPresentation HandheldDashboardPresentation(size_t selected);
    void HandheldDashboardOpen();
    void HandheldDashboardPause();
    bool HandheldDashboardIsOpen();
    bool HandheldDashboardRailFocused();
    void HandheldDashboardMove(int x, int y);
    void HandheldDashboardSwitchPane();
    bool HandheldRidesIsOpen();
    Handheld::ManagementPresentation HandheldRidesPresentation();
    bool HandheldGuestsIsOpen();
    Handheld::ManagementPresentation HandheldGuestsPresentation();
    bool HandheldStaffIsOpen();
    void HandheldStaffPause();
    void HandheldStaffRemoved(EntityId id);
    bool HandheldFinancesIsOpen();
    void HandheldFinancesMove(int x, int y);
    void HandheldFinancesPause();
    bool HandheldShopManagementIsOpen();
    void HandheldShopManagementMove(int x, int y);
    void HandheldShopManagementPause();
    void HandheldPathOptionsOpen();
    void HandheldTreePickerOpen();
    void HandheldShopPickerOpen();
    bool HandheldObjectPickerIsOpen();
    void HandheldObjectPickerMove(int x, int y);
    void HandheldObjectPickerPause();
    ObjectEntryIndex HandheldObjectPickerFocus();
    bool HandheldTreePickerIsOpen();
    void HandheldTreePickerMove(int x, int y);
    void HandheldTreePickerPause();
    ObjectEntryIndex HandheldTreePickerFocus();
} // namespace OpenRCT2::Ui::Windows

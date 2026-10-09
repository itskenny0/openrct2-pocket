/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once
#include "RideManagement.h"
#include "SceneryTools.h"
#include "StaffManagement.h"

#include <cstddef>
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
    void HandleExplorerInput(ControllerInput& input, const Frame& frame);
    void EndTerrain();
    void BeginTerrain(bool raise);
    void BeginWater(bool raise = true);
    void BeginSurface();
    void BeginTrees();
    void BeginScenery();
    void BeginAreaTools();
    void BeginRightsTools();
    bool IsSceneryToolActive();
    std::optional<SceneryTools::Target> SceneryEditTarget();
    bool BeginCustomTrack(ObjectEntryIndex index);
    bool ResumeCustomTrack(RideId id);
    bool BeginFlatRide(ObjectEntryIndex index);
    bool BeginSavedDesign(const std::string& path);
    bool ResumeFlatRide(RideId id);
    void SetRidePathAccess(const RideManagement::Target& target, StationIndex station, bool exit, CoordsXYZD outside);
    bool IsShopToolActive();
    ObjectEntryIndex ShopSelection();
    Direction ShopDirection();
    bool SetShopSelection(ObjectEntryIndex index);
    void RotateShop();
    bool IsTreeToolActive();
    ObjectEntryIndex TreeSelection();
    bool SetTreeSelection(ObjectEntryIndex index);
    void BeginSceneryRemoval();
    void BeginTrackColour(const RideManagement::Target& ride);
    void DrawExplorer(Drawing::RenderTarget& rt);
} // namespace OpenRCT2::Ui::Handheld

namespace OpenRCT2::Ui::Windows
{
    void HandheldRideOpen(RideId ride);
    void HandheldRideAccessOpen(const Handheld::RideManagement::Target& target);
    Handheld::ManagementPresentation HandheldDashboardPresentation(size_t selected);
    void HandheldDashboardOpen();
    void HandheldDashboardPause();
    bool HandheldDashboardIsOpen();
    bool HandheldDashboardRailFocused();
    void HandheldDashboardMove(int x, int y);
    void HandheldDashboardSwitchPane();
    size_t HandheldDashboardSectionIndex();
    bool HandheldOverviewIsOpen();
    Handheld::ManagementPresentation HandheldOverviewPresentation();
    bool HandheldRidesIsOpen();
    Handheld::ManagementPresentation HandheldRidesPresentation();
    bool HandheldGuestsIsOpen();
    Handheld::ManagementPresentation HandheldGuestsPresentation();
    void HandheldStaffOpen(std::optional<Handheld::StaffManagement::Target> target = {});
    Handheld::ManagementPresentation HandheldStaffPresentation();
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
    void HandheldSceneryOptionsOpen();
    void HandheldAreaOptionsOpen();
    void HandheldRightsOptionsOpen();
    void HandheldRightsReviewOpen(CoordsXY tile);
    bool HandheldConstructionMenuIsOpen();
    void HandheldConstructionMove(int x, int y);
    void HandheldShopPickerOpen();
    void HandheldRidePickerOpen();
    void HandheldDesignPickerOpen();
    void HandheldDesignReviewOpen();
    void HandheldDesignReviewPause();
    bool HandheldObjectPickerIsOpen();
    void HandheldObjectPickerMove(int x, int y);
    void HandheldObjectPickerPause();
    ObjectEntryIndex HandheldObjectPickerFocus();
    bool HandheldTreePickerIsOpen();
    void HandheldTreePickerMove(int x, int y);
    void HandheldTreePickerPause();
    ObjectEntryIndex HandheldTreePickerFocus();
} // namespace OpenRCT2::Ui::Windows

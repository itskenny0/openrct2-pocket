/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once
#include "Alerts.h"
#include "QuickWheel.h"
#include <openrct2/object/Object.h>
#include "PeepPlacement.h"
#include "RideManagement.h"
#include "RideNavigation.h"
#include "SceneryTools.h"
#include "StaffManagement.h"
#include <openrct2/ride/TrackConstructionSupport.h>

#include <cstddef>
#include <string_view>
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
    // Shows an upstream error as a short Pocket notice under the HUD (sound only while a menu
    // or build tool is open, since those already show the message in their own status line).
    void ShowToast(std::string_view title, std::string_view message);
    // Y in the park: moves the camera to the next alert and shows what usually fixes it. Returns
    // false when nothing needs attention.
    bool JumpToNextAlert();
    // L2 then Y: the park with nothing drawn over it, for screenshots.
    bool PhotoModeActive();
    // Glide to one alert and say what fixes it (the inbox's A).
    void JumpToAlert(const Alerts::Alert& alert);
    void UpdateExplorer(ControllerInput& input);
    bool QuickWheelOpen();
    void HandleExplorerInput(ControllerInput& input, const Frame& frame);
    void EndTerrain();
    std::optional<CoordsXY> CurrentTerrainTile();
    bool BeginStaffPatrol(const StaffManagement::Target& employee);
    bool BeginPeepPlacement(const PeepPlacement::Target&);
    void BeginTerrain(bool raise);
    void BeginWater(bool raise = true);
    void BeginSurface();
    void BeginTrees();
    void BeginScenery();
    void BeginSceneryClear();
    void BeginAreaTools();
    void BeginRightsTools();
    bool IsSceneryToolActive();
    std::optional<SceneryTools::Target> SceneryEditTarget();
    bool BeginMaze(ObjectEntryIndex index);
    bool ResumeMaze(RideId id);
    bool BeginCustomTrack(ObjectEntryIndex index, ride_type_t type = kRideTypeNull);
    bool ResumeCustomTrack(RideId id);
    bool ResumeCustomTrackFrom(
        const RideManagement::Target& target, CoordsXYZD origin, TrackElemType type, bool backward = false);
    bool ResumeCustomTrackReplacement(const RideManagement::Target& target,
        const TrackConstructionSupport::BlockSnapshot& removed, uint8_t colour);
    bool BeginFlatRide(ObjectEntryIndex index);
    bool BeginSavedDesign(const std::string& path, std::optional<ObjectEntryDescriptor> vehicle = {});
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
    void HandheldRideReturnNotice(std::string notice);
    void HandheldRideReturn(const Handheld::RideManagement::Target& target, const std::optional<Handheld::RideNavigation::Context>& context = {});
    bool HandheldRideQuickAccess();
    void HandheldRideAccessOpen(const Handheld::RideManagement::Target& target);
    Handheld::ManagementPresentation HandheldDashboardPresentation(size_t selected);
    void HandheldDashboardOpen();
    void HandheldQuickJump(Handheld::QuickWheel::Target target);
    void HandheldSignEditOpen();
    std::pair<std::string, uint32_t> HandheldLastBuild();
    void HandheldDashboardPause();
    bool HandheldDashboardIsOpen();
    bool HandheldDashboardRailFocused();
    void HandheldDashboardMove(int x, int y);
    void HandheldDashboardSwitchPane();
    void HandheldDashboardPage(int direction);
    size_t HandheldDashboardSectionIndex();
    bool HandheldOverviewIsOpen();
    Handheld::ManagementPresentation HandheldOverviewPresentation();
    Handheld::ManagementPresentation HandheldManagementPresentation();
    bool HandheldManagementContext();
    bool HandheldRidesIsOpen();
    Handheld::ManagementPresentation HandheldRidesPresentation();
    Handheld::ManagementPresentation HandheldShopAppearancePresentation();
    bool HandheldGuestsIsOpen();
    Handheld::ManagementPresentation HandheldGuestsPresentation();
    void HandheldGuestOpen(const Handheld::GuestManagement::Target&);
    bool HandheldObjectPickerGroup();
    bool HandheldTrackSelectionMove(int x,int y);
    bool HandheldToolSettingsPage(int direction);
    std::string HandheldNewsText(const std::string& text);
    void HandheldNewsOpen();
    void HandheldInboxOpen();
    // Y on the controls guide plays the intro; the intro can also be opened directly.
    bool HandheldControlsIntro();
    void HandheldPrimerOpen();
    void HandheldObjectivesOpen();
    bool HandheldDashboardNews();
    void HandheldPeepPlacementOptionsOpen();
    void HandheldStaffOpen(std::optional<Handheld::StaffManagement::Target> target = {});
    Handheld::ManagementPresentation HandheldStaffPresentation();
    bool HandheldStaffIsOpen();
    void HandheldStaffPause();
    void HandheldStaffPatrolOptionsOpen();
    void HandheldStaffRemoved(EntityId id);
    bool HandheldFinancesIsOpen();
    void HandheldFinancesMove(int x, int y);
    void HandheldFinancesPause();
    bool HandheldShopManagementIsOpen();
    void HandheldShopManagementMove(int x, int y);
    void HandheldShopManagementPause();
    void HandheldPathOptionsOpen();
    void HandheldPathSurfacePickerOpen();
    void HandheldTreePickerOpen();
    void HandheldSceneryOptionsOpen();
    void HandheldSceneryCatalogOpen();
    void HandheldTerrainMaterialPickerOpen(bool edge);
    bool HandheldSceneryPickerIsOpen();
    void HandheldScenerySearchOpen();
    void HandheldSceneryStyleOpen(const Handheld::SceneryTools::Target& target);
    void HandheldAreaOptionsOpen();
    void HandheldTerrainPaintReviewOpen(CoordsXY tile);
    void HandheldSceneryClearOptionsOpen();
    void HandheldSceneryClearReviewOpen(CoordsXY tile);
    void HandheldRightsOptionsOpen();
    void HandheldRightsReviewOpen(CoordsXY tile);
    Handheld::ManagementPresentation HandheldParkMapSubjectsPresentation();
    std::optional<CoordsXY> HandheldParkMapLocation();
    void HandheldParkMapSubjects();
    bool HandheldParkMapIsOpen();
    void HandheldParkMapControl(int control);
    bool HandheldConstructionMenuIsOpen();
    void HandheldConstructionMove(int x, int y);
    void HandheldShopPickerOpen();
    void HandheldRidePickerOpen();
    void HandheldDesignPickerOpen();
    void HandheldDesignFilesOpen();
    bool HandheldDesignPickerIsOpen();
    void HandheldDesignReviewOpen();
    void HandheldDesignOptionsOpen();
    void HandheldDesignReviewPause();
    bool HandheldObjectPickerIsOpen();
    bool HandheldObjectPickerCamera();
    void HandheldObjectPickerMove(int x, int y);
    void HandheldObjectPickerPause();
    ObjectEntryIndex HandheldObjectPickerFocus();
    bool HandheldTreePickerIsOpen();
    void HandheldTreePickerMove(int x, int y);
    void HandheldTreePickerPause();
    ObjectEntryIndex HandheldTreePickerFocus();
} // namespace OpenRCT2::Ui::Windows


/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "GuestFollow.h"
#include "Explorer.h"

#include "AreaTools.h"
#include "ConstructionPanel.h"
#include "ControllerInput.h"
#include "Currency.h"
#include "CustomTracks.h"
#include "Designs.h"
#include "DesignAuthoring.h"
#include "FlatRides.h"
#include "Hud.h"
#include "Mazes.h"
#include "Panel.h"
#include "Paths.h"
#include "RideAccess.h"
#include "RightsTools.h"
#include "SceneryRemoval.h"
#include "SceneryTools.h"
#include "SceneryClear.h"
#include "ShopManagement.h"
#include "Shops.h"
#include "StaffPatrol.h"
#include "Surface.h"
#include "Trees.h"
#include "Water.h"

#include <SDL_timer.h>
#include <iomanip>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/input/MouseInput.h>
#include <openrct2-ui/input/ShortcutIds.h>
#include <openrct2-ui/input/ShortcutManager.h>
#include <openrct2-ui/interface/Chat.h>
#include <openrct2-ui/interface/InGameConsole.h>
#include <openrct2-ui/interface/ViewportInteraction.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/terraform/LandLowerAction.h>
#include <openrct2/actions/terraform/LandRaiseAction.h>
#include <openrct2/drawing/Line.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/Rectangle.h>
#include <openrct2/drawing/Text.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <sstream>

namespace OpenRCT2::Ui::Handheld
{
    static bool _terrainActive{};
    static bool _waterTool{};
    static bool _surfaceTool{};
    static bool _treeTool{};
    static bool _shopTool{};
    static ObjectEntryIndex _shopIndex = kObjectEntryIndexNull;
    static Direction _shopDirection{};
    static ObjectEntryIndex _treeIndex = kObjectEntryIndexNull;
    static bool _sceneryRemovalTool{};
    static bool _sceneryTool{};
    static bool _sceneryClearTool{};
    static bool _areaTool{};
    static bool _rightsTool{};
    static std::optional<SceneryTools::Target> _sceneryEdit;
    static std::optional<RideManagement::Target> _colourRide;
    static std::optional<RideManagement::PaintTarget> _colourTarget;
    static uint8_t _colourScheme{};
    static std::optional<SceneryRemoval::Target> _sceneryTarget;
    static ObjectEntryIndex _surfaceIndex = kObjectEntryIndexNull;
    static bool _raiseLand = true;
    static bool _selectionOwned{};
    static std::optional<CoordsXY> _terrainTile;
    static std::string _terrainStatus;
    static bool _scatterFeedback{};
    static std::optional<RideManagement::Target> _ridePathAccess;
    static StationIndex _accessStation{};
    static bool _accessExit{};
    static CoordsXYZD _accessOutside{};

    static bool AccessTargetValid()
    {
        return _ridePathAccess && RideAccess::PointValid(*_ridePathAccess, _accessStation, _accessExit, _accessOutside);
    }

    void SetRidePathAccess(const RideManagement::Target& target, StationIndex station, bool exit, CoordsXYZD outside)
    {
        _ridePathAccess = target;
        _accessStation = station;
        _accessExit = exit;
        _accessOutside = outside;
    }

    void BeginTerrain(bool raise)
    {
        PeepPlacement::End();
        StaffPatrol::End();
        _ridePathAccess.reset();
        Paths::End();
        Trees::ClearPreview();
        SceneryTools::ClearPreview();
        Shops::ClearPreview();
        FlatRides::End();
        Designs::End();
        DesignAuthoring::End();
        CustomTracks::End();
        Mazes::End();
        _terrainActive = true;
        _waterTool = false;
        _surfaceTool = false;
        _treeTool = false;
        _shopTool = false;
        _shopIndex = kObjectEntryIndexNull;
        _treeIndex = kObjectEntryIndexNull;
        _colourRide.reset();
        _colourTarget.reset();
        _sceneryRemovalTool = false;
        _sceneryTool = false;
        _sceneryClearTool = false;
        _areaTool = false;
        _rightsTool = false;
        _sceneryEdit.reset();
        _sceneryTarget.reset();
        _surfaceIndex = kObjectEntryIndexNull;
        _raiseLand = raise;
        _terrainStatus.clear();
        _scatterFeedback = false;
    }

    void BeginWater(bool raise)
    {
        BeginTerrain(raise);
        _waterTool = true;
    }

    void BeginSurface()
    {
        BeginTerrain(true);
        _surfaceTool = true;
        _surfaceIndex = Surface::Next(kObjectEntryIndexNull);
    }

    void BeginRightsTools()
    {
        BeginTerrain(true);
        _rightsTool = true;
    }
    bool BeginPeepPlacement(const PeepPlacement::Target& person)
    {
        BeginTerrain(true);
        const auto result = PeepPlacement::Begin(person);
        if (result.error == GameActions::Status::ok)
            return true;
        EndTerrain();
        return false;
    }
    bool BeginStaffPatrol(const StaffManagement::Target& employee)
    {
        BeginTerrain(true);
        if (StaffPatrol::Begin(employee))
            return true;
        EndTerrain();
        return false;
    }
    void BeginAreaTools()
    {
        BeginTerrain(true);
        _areaTool = true;
    }
    void BeginSceneryClear() { BeginTerrain(true); _sceneryClearTool = true; }
    void BeginScenery()
    {
        BeginTerrain(true);
        _sceneryTool = true;
    }
    bool IsSceneryToolActive()
    {
        return _terrainActive && _sceneryTool;
    }

    std::optional<SceneryTools::Target> SceneryEditTarget()
    {
        return _sceneryEdit;
    }
    void BeginTrees()
    {
        BeginTerrain(true);
        _treeTool = true;
        _treeIndex = Trees::Next(kObjectEntryIndexNull);
    }

    bool BeginMaze(ObjectEntryIndex index)
    {
        if (!Mazes::Eligible(index))
            return false;
        BeginTerrain(true);
        return Mazes::Begin(index);
    }
    bool ResumeMaze(RideId id)
    {
        const auto target = RideManagement::Identify(id);
        if (!target || !Mazes::CanResume(*target))
            return false;
        BeginTerrain(true);
        return Mazes::Resume(*target);
    }
    bool BeginCustomTrack(ObjectEntryIndex index, ride_type_t type)
    {
        if (!CustomTracks::Eligible(index, type))
            return false;
        BeginTerrain(true);
        return CustomTracks::Begin(index, type);
    }
    bool ResumeCustomTrack(RideId id)
    {
        auto target = RideManagement::Identify(id);
        if (!target || !CustomTracks::CanResume(*target))
            return false;
        BeginTerrain(true);
        return CustomTracks::Resume(*target);
    }
    bool ResumeCustomTrackFrom(const RideManagement::Target& target, CoordsXYZD origin, TrackElemType type, bool backward)
    {
        if (!RideManagement::Resolve(target))
            return false;
        BeginTerrain(true);
        return CustomTracks::ResumeFrom(target, std::pair{ origin, type }, backward);
    }
    bool BeginFlatRide(ObjectEntryIndex index)
    {
        if (!FlatRides::Eligible(index))
            return false;
        BeginTerrain(true);
        return FlatRides::Begin(index);
    }
    bool BeginSavedDesign(const std::string& path)
    {
        auto td = TrackDesignImport(path.c_str());
        if (!td || !Designs::Eligible(*td))
            return false;
        BeginTerrain(true);
        return Designs::Begin(path);
    }
    bool ResumeFlatRide(RideId id)
    {
        auto target = RideManagement::Identify(id);
        if (!target || !FlatRides::CanResume(*target))
            return false;
        BeginTerrain(true);
        return FlatRides::Resume(*target);
    }
    bool IsShopToolActive()
    {
        return _shopTool;
    }
    ObjectEntryIndex ShopSelection()
    {
        return _shopIndex;
    }
    Direction ShopDirection()
    {
        return _shopDirection;
    }
    bool SetShopSelection(ObjectEntryIndex index)
    {
        if (!Shops::Eligible(index))
            return false;
        if (!_shopTool)
        {
            BeginTerrain(true);
            _shopTool = true;
            _shopDirection = 0;
        }
        Shops::ClearPreview();
        FlatRides::ClearPreview();
        Designs::ClearPreview();
        _shopIndex = index;
        _terrainStatus.clear();
        _scatterFeedback = false;
        return true;
    }
    void RotateShop()
    {
        if (!_shopTool)
            return;
        Shops::ClearPreview();
        FlatRides::ClearPreview();
        Designs::ClearPreview();
        _shopDirection = (_shopDirection + 1) & 3;
        _terrainStatus.clear();
        _scatterFeedback = false;
    }

    bool IsTreeToolActive()
    {
        return _treeTool;
    }
    ObjectEntryIndex TreeSelection()
    {
        return _treeIndex;
    }
    bool SetTreeSelection(ObjectEntryIndex index)
    {
        if (!_treeTool || !Trees::Eligible(index))
            return false;
        Trees::ClearPreview();
        SceneryTools::ClearPreview();
        Shops::ClearPreview();
        FlatRides::ClearPreview();
        Designs::ClearPreview();
        _treeIndex = index;
        _terrainStatus.clear();
        _scatterFeedback = false;
        return true;
    }

    void BeginTrackColour(const RideManagement::Target& ride)
    {
        BeginTerrain(true);
        _colourRide = ride;
        _colourScheme = 0;
    }
    void BeginSceneryRemoval()
    {
        BeginTerrain(true);
        _sceneryRemovalTool = true;
    }

    static void ClearTerrainSelection()
    {
        if (_selectionOwned)
        {
            MapSelection::invalidate();
            gMapSelectFlags.unset(MapSelectFlag::enable);
            gMapSelectFlags.unset(MapSelectFlag::enableArrow);
            MapSelection::invalidate();
            _selectionOwned = false;
        }
        _terrainTile.reset();
        _sceneryTarget.reset();
    }

    void EndTerrain()
    {
        PeepPlacement::End();
        StaffPatrol::End();
        _ridePathAccess.reset();
        Trees::ClearPreview();
        SceneryTools::ClearPreview();
        Shops::ClearPreview();
        FlatRides::End();
        Designs::End();
        CustomTracks::End();
        Mazes::End();
        _terrainActive = false;
        _waterTool = false;
        _surfaceTool = false;
        _treeTool = false;
        _shopTool = false;
        _shopIndex = kObjectEntryIndexNull;
        _treeIndex = kObjectEntryIndexNull;
        _colourRide.reset();
        _colourTarget.reset();
        _sceneryRemovalTool = false;
        _sceneryTool = false;
        _sceneryClearTool = false;
        _areaTool = false;
        _rightsTool = false;
        _sceneryEdit.reset();
        _sceneryTarget.reset();
        _surfaceIndex = kObjectEntryIndexNull;
        Paths::End();
        _terrainStatus.clear();
        _scatterFeedback = false;
        ClearTerrainSelection();
    }

    static ScreenCoordsXY Centre();
    static GameActions::Result TerrainAction(bool execute)
    {
        if (CustomTracks::Active())
            return CustomTracks::Action(execute);
        if (Designs::Active())
            return Designs::Query(_terrainTile);
        if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
            return FlatRides::Action(_terrainTile, execute);
        if (PeepPlacement::Active())
        {
            if (!_terrainTile)
                return PeepPlacement::Place({}, execute);
            const auto info = GetMapCoordinatesFromPos(
                Centre(), { ViewportInteractionItem::footpath, ViewportInteractionItem::terrain });
            return PeepPlacement::Place(
                info.Element ? std::optional<CoordsXYZ>{ CoordsXYZ{ info.Loc, info.Element->getBaseZ() } } : std::nullopt,
                execute);
        }
        if (StaffPatrol::Active())
            return StaffPatrol::Action(_terrainTile, execute);
        if (_rightsTool)
            return RightsTools::Query(_terrainTile, RightsTools::settings);
        if (_areaTool)
            return AreaTools::Action(_terrainTile, AreaTools::settings, execute);
        if (_sceneryClearTool) return SceneryClear::Query(_terrainTile, SceneryClear::settings);
        if (_sceneryTool)
        {
            const auto& settings = SceneryTools::Current();
            if (settings.mode == SceneryTools::Mode::place)
                return SceneryTools::Place(_terrainTile, settings, execute);
            if (_sceneryEdit)
            {
                if (settings.mode == SceneryTools::Mode::pick)
                    return SceneryTools::Pick(*_sceneryEdit, execute);
                return SceneryTools::Modify(*_sceneryEdit, settings.mode, settings.colours, execute);
            }
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = "Point at scenery";
            return result;
        }
        if (_colourRide)
        {
            if (_colourTarget)
                return RideManagement::Paint(*_colourTarget, _colourScheme, execute);
            GameActions::Result result;
            result.error = GameActions::Status::disallowed;
            result.errorMessage = "Point at track belonging to this ride";
            return result;
        }
        if (_sceneryRemovalTool)
            return SceneryRemoval::Action(_sceneryTarget, execute);
        if (_shopTool)
            return Shops::Action(_terrainTile, _shopIndex, _shopDirection, execute);
        if (_treeTool)
            return Trees::Action(_terrainTile, _treeIndex, execute);
        if (_surfaceTool)
            return Surface::Action(_terrainTile, _surfaceIndex, execute);
        if (_waterTool)
            return Water::Action(_terrainTile, _raiseLand, execute);
        const auto tile = *_terrainTile;
        const MapRange range{ tile.x, tile.y, tile.x, tile.y };
        const CoordsXY centre{ tile.x + 16, tile.y + 16 };
        if (_raiseLand)
        {
            auto action = GameActions::LandRaiseAction(centre, range, MapSelectType::full);
            return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
        }
        auto action = GameActions::LandLowerAction(centre, range, MapSelectType::full);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }

    static bool HasTextOrModalInput()
    {
        auto* wm = GetWindowManager();
        return gChatOpen || GetInGameConsole().IsOpen() || Windows::IsUsingWidgetTextBox()
            || wm->FindByClass(WindowClass::textinput) != nullptr || wm->FindByClass(WindowClass::loadsave) != nullptr
            || wm->FindByClass(WindowClass::loadsaveOverwritePrompt) != nullptr
            || wm->FindByClass(WindowClass::savePrompt) != nullptr || GetShortcutManager().isPendingShortcutChange();
    }

    static bool CanExplore()
    {
        if (gLegacyScene != LegacyScene::playing || WindowGetMain() == nullptr || HasTextOrModalInput()
            || gInputFlags.has(InputFlag::toolActive) || InputGetState() != InputState::normal
            || GetWindowManager()->FindByClass(WindowClass::error) != nullptr
            || GetWindowManager()->FindByClass(WindowClass::progressWindow) != nullptr
            || GetWindowManager()->FindByClass(WindowClass::dropdown) != nullptr)
            return false;
        for (const auto& window : gWindowList)
        {
            if (!window->flags.hasAny(WindowFlag::dead, WindowFlag::stickToBack, WindowFlag::stickToFront))
                return false;
        }
        return true;
    }

    static ScreenCoordsXY Centre()
    {
        return { ContextGetWidth() / 2, ContextGetHeight() / 2 };
    }

    static void Invoke(std::string_view id)
    {
        if (auto* shortcut = GetShortcutManager().getShortcut(id); shortcut != nullptr)
            shortcut->action();
    }

    class FocusWindow final : public Window
    {
    public:
        void onDraw(Drawing::RenderTarget& rt) override
        {
            DrawExplorer(rt);
        }
    };

    static void SyncFocusWindow()
    {
        auto* wm = GetWindowManager();
        auto* focus = wm->FindByClass(WindowClass::handheldFocus);
        if (!CanExplore())
        {
            if (focus != nullptr)
                wm->Close(*focus);
            wm->CloseByClass(WindowClass::handheldHud);
            return;
        }
        const auto position = Centre() - ScreenCoordsXY{ 9, 9 };
        if (focus == nullptr)
        {
            // An empty, input-transparent widget list lets map hit testing pass through.
            // A real overlay window also keeps viewport scroll blits from smearing the reticle.
            wm->Create<FocusWindow>(
                WindowClass::handheldFocus, position, { 19, 19 },
                { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
        }
        else if (focus->windowPos != position)
        {
            focus->invalidate();
            focus->windowPos = position;
            focus->invalidate();
        }
        auto* hud = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 0 });
        if (hud == nullptr)
        {
            hud = wm->Create<FocusWindow>(
                WindowClass::handheldHud, { 0, ContextGetHeight() - kActionStripHeight },
                { ContextGetWidth(), kActionStripHeight },
                { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
        }
        if (hud != nullptr)
        {
            hud->invalidate();
            hud->windowPos = { 0, ContextGetHeight() - kActionStripHeight };
            hud->width = ContextGetWidth();
            hud->invalidate();
        }
        auto* top = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 3 });
        if (top == nullptr)
        {
            top = wm->Create<FocusWindow>(
                WindowClass::handheldHud, { Layout::margin, Layout::hudTop },
                { ContextGetWidth() - 2 * Layout::margin, Layout::hudHeight },
                { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
            if (top != nullptr)
                top->number = 3;
        }
        if (top != nullptr)
        {
            top->invalidate();
            top->width = ContextGetWidth() - 2 * Layout::margin;
            top->invalidate();
        }
        // A separate small input-transparent window redraws only the tool panel,
        // rather than placing a full-screen overlay over the scrolling viewport.
        auto* tools = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 1 });
        if (_terrainActive || Paths::IsActive())
        {
            if (tools == nullptr)
            {
                tools = wm->Create<FocusWindow>(
                    WindowClass::handheldHud, Concept::kOrigin, kTerrainOverlaySize,
                    { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
                if (tools != nullptr)
                    tools->number = 1;
            }
            if (tools != nullptr)
                tools->invalidate();
        }
        else if (tools != nullptr)
            wm->Close(*tools);
        auto* cost = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 2 });
        if (_terrainActive || Paths::IsActive())
        {
            if (cost == nullptr)
            {
                cost = wm->Create<FocusWindow>(
                    WindowClass::handheldHud,
                    Layout::ContextAnchor({ ContextGetWidth(), ContextGetHeight() }, kContextSize, CustomTracks::Active()),
                    kContextSize,
                    { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
                if (cost != nullptr)
                    cost->number = 2;
            }
            if (cost != nullptr)
            {
                cost->invalidate();
                cost->windowPos = Layout::ContextAnchor(
                    { ContextGetWidth(), ContextGetHeight() }, kContextSize, CustomTracks::Active());
                cost->invalidate();
            }
        }
        else if (cost != nullptr)
            wm->Close(*cost);
    }

    bool HandleMenuInput(ControllerInput& input, const Frame& frame)
    {
        if (GetWindowManager()->FindByClass(WindowClass::handheldMenu) != nullptr)
        {
            if (Windows::HandheldDashboardIsOpen())
                Windows::HandheldDashboardMove(frame.navigateX, frame.navigateY);
            else if (Windows::HandheldObjectPickerIsOpen())
                Windows::HandheldObjectPickerMove(frame.navigateX, frame.navigateY);
            else if (Windows::HandheldFinancesIsOpen())
                Windows::HandheldFinancesMove(frame.navigateX, frame.navigateY);
            else if (Windows::HandheldShopManagementIsOpen())
                Windows::HandheldShopManagementMove(frame.navigateX, frame.navigateY);
            else if (Windows::HandheldConstructionMenuIsOpen())
                Windows::HandheldConstructionMove(frame.navigateX, frame.navigateY);
            else if (frame.navigateY != 0)
                Windows::HandheldMenuMove(frame.navigateY);
            for (const auto action : frame.actions)
            {
                if (Windows::HandheldParkMapIsOpen() && action == Action::openBuild)
                    Windows::HandheldParkMapSubjects();
                else if (Windows::HandheldParkMapIsOpen() && action == Action::contextPrimary)
                    Invoke(ShortcutId::kViewGeneralRotateClockwise);
                else if (Windows::HandheldParkMapIsOpen() && action == Action::contextSecondary)
                    Windows::HandheldParkMapControl(0);
                else if (Windows::HandheldParkMapIsOpen() && action == Action::previousOption)
                    Windows::HandheldParkMapControl(-1);
                else if (Windows::HandheldParkMapIsOpen() && action == Action::nextOption)
                    Windows::HandheldParkMapControl(1);
                else if (Windows::HandheldSceneryPickerIsOpen() && action == Action::contextPrimary)
                    Windows::HandheldScenerySearchOpen();
                else if (action == Action::confirm)
                    Windows::HandheldMenuConfirm();
                else if (action == Action::openBuild && Windows::HandheldDashboardIsOpen())
                    Windows::HandheldDashboardSwitchPane();
                else if (action == Action::openMenu && Windows::HandheldDashboardIsOpen())
                {
                    if (Windows::HandheldStaffIsOpen())
                        Windows::HandheldStaffPause();
                    else if (Windows::HandheldFinancesIsOpen())
                        Windows::HandheldFinancesPause();
                    else
                        Windows::HandheldDashboardPause();
                }
                else if (action == Action::openMenu && Designs::HasReview())
                    Windows::HandheldDesignReviewPause();
                else if (action == Action::openMenu && Windows::HandheldObjectPickerIsOpen())
                    Windows::HandheldObjectPickerPause();
                else if (action == Action::openMenu && Windows::HandheldShopManagementIsOpen())
                    Windows::HandheldShopManagementPause();
                else if (action == Action::openMenu && Windows::HandheldStaffIsOpen())
                    Windows::HandheldStaffPause();
                else if (action == Action::openMenu && Windows::HandheldFinancesIsOpen())
                    Windows::HandheldFinancesPause();
                else if (action == Action::back || action == Action::openMenu)
                    Windows::HandheldMenuBack();
                input.suspend();
                break;
            }
            return true;
        }
        return false;
    }

    void UpdateExplorer(ControllerInput& input)
    {
        GuestFollow::Update();
        if (gLegacyScene != LegacyScene::playing)
        {
            EndTerrain();
        }
        if (CustomTracks::Completed())
        {
            const auto target = *CustomTracks::Completed();
            EndTerrain();
            if (FlatRides::CanResume(target))
                ResumeFlatRide(target.id);
            else if (RideAccess::Inspect(target))
                Windows::HandheldRideAccessOpen(target);
            input.suspend();
        }
        if (Designs::Completed())
        {
            const auto target = *Designs::Completed();
            EndTerrain();
            if (FlatRides::CanResume(target))
                ResumeFlatRide(target.id);
            else if (RideAccess::Inspect(target))
                Windows::HandheldRideAccessOpen(target);
            input.suspend();
        }
        if (_terrainActive && FlatRides::CurrentStage() == FlatRides::Stage::inactive && FlatRides::Target())
        {
            const auto target = *FlatRides::Target();
            EndTerrain();
            if (RideAccess::Inspect(target))
                Windows::HandheldRideAccessOpen(target);
            input.suspend();
        }
        if (PeepPlacement::Active())
        {
            if (!PeepPlacement::Resolve(*PeepPlacement::target) || PeepPlacement::finished)
                EndTerrain();
            else
                PeepPlacement::Preview(CanExplore() && ContextHasFocus(), Centre());
        }
        if (StaffPatrol::Active())
        {
            if (!StaffManagement::Resolve(*StaffPatrol::target))
                EndTerrain();
            else if (CanExplore() && ContextHasFocus())
                StaffPatrol::ShowOverlay();
            else
                StaffPatrol::HideOverlay();
        }
        if (_ridePathAccess && (!Paths::IsActive() || !AccessTargetValid()))
        {
            EndTerrain();
            input.suspend();
        }
        ClearTerrainSelection();
        if ((_terrainActive || Paths::IsActive()) && CanExplore() && ContextHasFocus())
        {
            if (Paths::IsActive())
            {
                Paths::UpdateTarget(Centre());
                Paths::UpdatePreview();
            }
            auto coords = Paths::IsActive() ? Paths::TargetTile() : ScreenGetMapXY(Centre(), nullptr);
            if (_waterTool)
            {
                const auto info = GetMapCoordinatesFromPos(
                    Centre(), { ViewportInteractionItem::terrain, ViewportInteractionItem::water });
                coords = info.interactionType == ViewportInteractionItem::none ? std::nullopt
                                                                               : std::optional<CoordsXY>{ info.Loc };
            }
            if (_colourRide)
            {
                _colourTarget = RideManagement::CapturePaint(
                    *_colourRide, GetMapCoordinatesFromPos(Centre(), { ViewportInteractionItem::ride }));
                coords = _colourTarget ? std::optional<CoordsXY>{ _colourTarget->location } : std::nullopt;
            }
            if (_sceneryTool)
            {
                constexpr ViewportInteractionItems pickTypes = { ViewportInteractionItem::scenery,
                    ViewportInteractionItem::wall, ViewportInteractionItem::largeScenery,
                    ViewportInteractionItem::banner, ViewportInteractionItem::pathAddition };
                _sceneryEdit = SceneryTools::Capture(GetMapCoordinatesFromPos(Centre(),
                    SceneryTools::Current().mode == SceneryTools::Mode::pick ? pickTypes : kViewportInteractionItemAll));
            }
            if (_sceneryTool && SceneryTools::Current().mode != SceneryTools::Mode::place)
            {
                coords = _sceneryEdit ? std::optional<CoordsXY>{ _sceneryEdit->location } : std::nullopt;
            }
            if (_sceneryRemovalTool)
            {
                _sceneryTarget = SceneryRemoval::FromInfo(GetMapCoordinatesFromPos(Centre(), kViewportInteractionItemAll));
                coords = _sceneryTarget ? std::optional<CoordsXY>{ _sceneryTarget->location } : std::nullopt;
            }
            if (coords)
            {
                _terrainTile = CoordsXY{ coords->x & ~31, coords->y & ~31 };
                gMapSelectPositionA = gMapSelectPositionB = *_terrainTile;
                gMapSelectType = MapSelectType::full;
                if (_sceneryTool && SceneryTools::ScatterActive())
                {
                    const auto bounds=SceneryTools::ScatterBounds(*_terrainTile);
                    gMapSelectPositionA=bounds.first; gMapSelectPositionB=bounds.second;
                }
                if (StaffPatrol::Active())
                {
                    const auto range = StaffPatrol::Range(_terrainTile);
                    if (!range)
                        _terrainTile.reset();
                    else
                    {
                        gMapSelectPositionA = { range->getX1(), range->getY1() };
                        gMapSelectPositionB = { range->getX2(), range->getY2() };
                        gMapSelectType = MapSelectType::fullTerrainAndWater;
                    }
                }
                if (_rightsTool)
                {
                    const auto r = AreaTools::Range(_terrainTile, RightsTools::settings.width, RightsTools::settings.height);
                    if (!r)
                        _terrainTile.reset();
                    else
                    {
                        gMapSelectPositionA = { r->getX1(), r->getY1() };
                        gMapSelectPositionB = { r->getX2(), r->getY2() };
                    }
                }
                if (_sceneryClearTool) {
                    const auto range=AreaTools::Range(_terrainTile,SceneryClear::settings.width,SceneryClear::settings.height);
                    if(!range)_terrainTile.reset(); else {gMapSelectPositionA={range->getX1(),range->getY1()};gMapSelectPositionB={range->getX2(),range->getY2()};}
                }
                if (_areaTool)
                {
                    const auto range = AreaTools::Range(_terrainTile, AreaTools::settings.width, AreaTools::settings.height);
                    if (!range)
                    {
                        _terrainTile.reset();
                    }
                    else
                    {
                        gMapSelectPositionA = { range->getX1(), range->getY1() };
                        gMapSelectPositionB = { range->getX2(), range->getY2() };
                        gMapSelectType = AreaTools::Selection(AreaTools::settings);
                    }
                }
                gMapSelectFlags.set(MapSelectFlag::enable, _terrainTile.has_value());
                _selectionOwned = _terrainTile.has_value();
                MapSelection::invalidate();
            }
        }
        if (FlatRides::CurrentStage() != FlatRides::Stage::inactive && CanExplore() && ContextHasFocus())
        {
            FlatRides::UpdatePreview(_terrainTile);
            if (FlatRides::CurrentStage() != FlatRides::Stage::ride)
            {
                const auto location = FlatRides::EntranceLocation(_terrainTile);
                const auto result = FlatRides::Action(_terrainTile, false);
                if (location && result.error == GameActions::Status::ok)
                {
                    gMapSelectArrowPosition = *location;
                    gMapSelectArrowDirection = location->direction;
                    gMapSelectFlags.set(MapSelectFlag::enableArrow);
                    MapSelection::invalidate();
                }
                else
                {
                    gMapSelectFlags.unset(MapSelectFlag::enable);
                    _selectionOwned = false;
                }
            }
        }
        if (Mazes::Active() && CanExplore() && ContextHasFocus())
            Mazes::Update(_terrainTile);
        if (CustomTracks::Active() && CanExplore() && ContextHasFocus())
            CustomTracks::Update(_terrainTile);
        if (Designs::Active() && CanExplore() && ContextHasFocus())
            Designs::UpdatePreview(_terrainTile);
        if (_shopTool && CanExplore() && ContextHasFocus())
            Shops::UpdatePreview(_terrainTile, _shopIndex, _shopDirection);
        if (_treeTool && CanExplore() && ContextHasFocus())
            Trees::UpdatePreview(_terrainTile, _treeIndex);
        if (CanExplore() && ContextHasFocus() && _sceneryTool)
            SceneryTools::UpdatePreview(_terrainTile);
        if (!CanExplore() || !ContextHasFocus())
        {
            Paths::ClearPreview();
            Trees::ClearPreview();
            SceneryTools::ClearPreview();
            Shops::ClearPreview();
            FlatRides::ClearPreview();
            Designs::ClearPreview();
            CustomTracks::ClearPreview();
        }
        SyncFocusWindow();
        HandleExplorerInput(input, input.update(ContextHasFocus(), SDL_GetTicks()));
    }

    void HandleExplorerInput(ControllerInput& input, const Frame& frame)
    {
        auto* wm = GetWindowManager();
        for (const auto action : frame.actions)
        {
            if (action == Action::quit)
            {
                EndTerrain();
                ContextQuit();
                input.suspend();
                return;
            }
        }
        if (_ridePathAccess && !AccessTargetValid())
        {
            EndTerrain();
            input.suspend();
            return;
        }
        if (HandleMenuInput(input, frame))
            return;
        if (Mazes::Active() && CanExplore())
        {
            for (const auto action : frame.actions)
            {
                if (action == Action::openMenu)
                    Windows::HandheldDashboardOpen();
                else if (!Mazes::Busy())
                {
                    if (action == Action::confirm)
                    {
                        if (Mazes::Picking())
                            Mazes::SelectCell(true);
                        else
                            Mazes::Confirm();
                    }
                    else if (action == Action::back)
                    {
                        if (Mazes::Reviewing())
                            Mazes::Cancel();
                        else
                        {
                            const auto t = Mazes::Target();
                            EndTerrain();
                            if (t && RideManagement::Resolve(*t))
                                Windows::HandheldRideOpen(t->id);
                        }
                    }
                    else if (action == Action::previousOption || action == Action::nextOption)
                    {
                        if (Mazes::CameraMode())
                            Windows::MainWindowZoom(action == Action::nextOption, false);
                        else
                            Mazes::Turn(action == Action::nextOption ? 1 : -1);
                    }
                    else if (action == Action::slopeDown || action == Action::slopeUp)
                        Mazes::Height(action == Action::slopeUp ? 1 : -1);
                    else if (action == Action::contextPrimary)
                    {
                        if (Mazes::CameraMode())
                            Invoke(ShortcutId::kViewGeneralRotateClockwise);
                        else
                            Mazes::Move();
                    }
                    else if (action == Action::contextSecondary && !Mazes::Reviewing() && !Mazes::Picking())
                        Mazes::ReviewRemoval();
                    else if (action == Action::openBuild)
                        Mazes::ToggleCamera();
                }
                input.suspend();
                break;
            }
            if (frame.panX || frame.panY)
                InputScrollViewportSmooth({ frame.panX, frame.panY }, WindowGetMain());
            return;
        }
        if (CustomTracks::Active() && CanExplore())
        {
            for (const auto action : frame.actions)
            {
                if (action == Action::openMenu)
                    Windows::HandheldDashboardOpen();
                else if (action == Action::openBuild)
                    Windows::HandheldTrackPiecePickerOpen();
                else if (action == Action::cameraMode)
                    CustomTracks::ToggleCamera();
                else if (!CustomTracks::Busy())
                {
                    if (action == Action::confirm)
                        CustomTracks::Action(true);
                    else if (action == Action::back)
                    {
                        if (CustomTracks::StationPlacement())
                            EndTerrain();
                        else
                            CustomTracks::Undo();
                    }
                    else if (action == Action::contextPrimary)
                    {
                        if (CustomTracks::CameraMode())
                            Invoke(ShortcutId::kViewGeneralRotateClockwise);
                        else
                            CustomTracks::Bank();
                    }
                    else if (action == Action::contextSecondary)
                    {
                        if (CustomTracks::StationPlacement())
                            CustomTracks::Rotate();
                        else
                            CustomTracks::Chain();
                    }
                    else if (action == Action::previousOption || action == Action::nextOption)
                    {
                        if (CustomTracks::CameraMode())
                            Windows::MainWindowZoom(action == Action::nextOption, false);
                        else
                            CustomTracks::Cycle(action == Action::nextOption ? 1 : -1);
                    }
                    else if (action == Action::slopeDown || action == Action::slopeUp)
                        CustomTracks::Slope(action == Action::slopeUp ? 1 : -1);
                }
                input.suspend();
                break;
            }
            if (frame.panX || frame.panY)
                InputScrollViewportSmooth({ frame.panX, frame.panY }, WindowGetMain());
            return;
        }
        if (gLegacyScene == LegacyScene::titleSequence)
        {
            const bool scenarios = wm->FindByClass(WindowClass::scenarioSelect) != nullptr;
            bool modal = HasTextOrModalInput() || wm->FindByClass(WindowClass::dropdown) != nullptr
                || wm->FindByClass(WindowClass::changelog) != nullptr;
            for (const auto& window : gWindowList)
            {
                if (window->classification != WindowClass::scenarioSelect
                    && !window->flags.hasAny(WindowFlag::dead, WindowFlag::stickToBack, WindowFlag::stickToFront))
                    modal = true;
            }
            if (!modal)
            {
                if (scenarios && frame.navigateY != 0)
                    Windows::ScenarioSelectHandheldMove(frame.navigateY);
                else if (!scenarios && (frame.navigateX != 0 || frame.navigateY != 0))
                    Windows::TitleMenuHandheldMove(frame.navigateX != 0 ? frame.navigateX : frame.navigateY);
            }
            for (const auto action : frame.actions)
            {
                if (action == Action::back)
                {
                    if (wm->FindByClass(WindowClass::dropdown) != nullptr)
                        wm->CloseByClass(WindowClass::dropdown);
                    else if (modal || scenarios)
                        wm->CloseTop();
                    else
                        ContextQuit();
                }
                else if (!modal && scenarios)
                {
                    if (action == Action::confirm)
                        Windows::ScenarioSelectHandheldConfirm();
                    else if (action == Action::previousOption || action == Action::nextOption)
                        Windows::ScenarioSelectHandheldTab(action == Action::previousOption ? -1 : 1);
                }
                else if (!modal && action == Action::confirm)
                    Windows::TitleMenuHandheldConfirm();
                input.suspend();
                break;
            }
            return;
        }
        if (gLegacyScene != LegacyScene::playing || HasTextOrModalInput())
        {
            for (const auto action : frame.actions)
                if (action == Action::back)
                    wm->CloseTop();
            if (!frame.actions.empty() || gLegacyScene != LegacyScene::playing)
                input.suspend();
            return;
        }
        for (const auto action : frame.actions)
        {
            if (FlatRides::Busy() || Designs::Busy())
            {
                input.suspend();
                return;
            }
            if (action == Action::back)
            {
                if (wm->FindByClass(WindowClass::error) != nullptr || gInputFlags.has(InputFlag::toolActive))
                    Invoke(ShortcutId::kInterfaceCancelConstruction);
                else if (wm->FindByClass(WindowClass::dropdown) != nullptr)
                    wm->CloseByClass(WindowClass::dropdown);
                else if ((_terrainActive || Paths::IsActive()) && CanExplore())
                {
                    const auto access = _ridePathAccess;
                    const auto person = PeepPlacement::target;
                    const auto employee = StaffPatrol::target;
                    EndTerrain();
                    if (person && person->guest && GuestManagement::Resolve(*person->guest))
                        Windows::HandheldGuestOpen(*person->guest);
                    if (person && person->staff && StaffManagement::Resolve(*person->staff))
                        Windows::HandheldStaffOpen(*person->staff);
                    if (employee && StaffManagement::Resolve(*employee))
                        Windows::HandheldStaffOpen(employee);
                    if (access && RideManagement::Resolve(*access))
                        Windows::HandheldRideAccessOpen(*access);
                }
                else if (const auto followed = GuestFollow::Current(); followed && CanExplore())
                    Windows::HandheldGuestOpen(*followed);
                else
                    wm->CloseTop();
            }
            else if (action == Action::openMenu && !gInputFlags.has(InputFlag::toolActive))
            {
                Windows::HandheldDashboardOpen();
            }
            else if (action == Action::openBuild && CanExplore())
            {
                if (Designs::Active())
                    Designs::Rotate();
                else if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
                    FlatRides::Rotate();
                else if (PeepPlacement::Active())
                    Windows::HandheldPeepPlacementOptionsOpen();
                else if (StaffPatrol::Active())
                    Windows::HandheldStaffPatrolOptionsOpen();
                else if (_rightsTool)
                    Windows::HandheldRightsOptionsOpen();
                else if (_areaTool)
                    Windows::HandheldAreaOptionsOpen();
                else if (_sceneryClearTool)
                    Windows::HandheldSceneryClearOptionsOpen();
                else if (_sceneryTool)
                    Windows::HandheldSceneryOptionsOpen();
                else if (_shopTool)
                    RotateShop();
                else if (Paths::IsActive())
                    Windows::HandheldPathOptionsOpen();
                else
                {
                    EndTerrain();
                    Windows::HandheldBuildOpen();
                }
            }
            else if (CanExplore())
            {
                switch (action)
                {
                    case Action::confirm:
                        if (Paths::IsActive())
                        {
                            const auto result = Paths::Action(true);
                            _terrainStatus = result.error == GameActions::Status::ok ? "Path change requested."
                                                                                     : result.getErrorMessage();
                        }
                        else if (_sceneryClearTool && _terrainTile)
                            Windows::HandheldSceneryClearReviewOpen(*_terrainTile);
                        else if (_rightsTool && _terrainTile)
                            Windows::HandheldRightsReviewOpen(*_terrainTile);
                        else if (_terrainActive)
                        {
                            if (_terrainTile)
                            {
                                if (Designs::Active())
                                {
                                    if (Designs::Review(_terrainTile))
                                        Windows::HandheldDesignReviewOpen();
                                    else
                                        _terrainStatus = Designs::Query(_terrainTile).getErrorMessage();
                                    break;
                                }
                                if (_areaTool && AreaTools::settings.operation==AreaTools::Operation::paint)
                                {
                                    Windows::HandheldTerrainPaintReviewOpen(*_terrainTile);
                                    input.suspend();
                                    return;
                                }
                                const bool picking = _sceneryTool && SceneryTools::Current().mode == SceneryTools::Mode::pick;
                                const auto result = TerrainAction(true);
                                if (picking && result.error == GameActions::Status::ok)
                                {
                                    _terrainStatus = "Object selected / Move, then A Place";
                                    input.suspend(); // Selection cannot spill into placement on the same held press.
                                    return;
                                }
                                if (_sceneryTool && SceneryTools::ScatterActive())
                                {
                                    _scatterFeedback=result.error==GameActions::Status::ok;
                                    _terrainStatus=result.error==GameActions::Status::ok ? SceneryTools::ScatterStatus() : result.getErrorMessage();
                                    input.suspend(); // One review/apply transition per released press.
                                    return;
                                }
                                _terrainStatus = result.error == GameActions::Status::ok
                                    ? (FlatRides::CurrentStage() != FlatRides::Stage::inactive ? ""
                                           : _shopTool                                         ? "Shop placement requested."
                                           : _treeTool                                         ? "Tree placement requested."
                                           : _colourRide                                       ? "Track colour scheme applied."
                                           : _sceneryRemovalTool                               ? "Removal requested."
                                           : _surfaceTool                                      ? "Surface change requested."
                                           : _waterTool                                        ? "Water change requested."
                                                                                               : "Land changed.")
                                    : result.getErrorMessage();
                            }
                        }
                        else
                        {
                            const auto info = GetMapCoordinatesFromPos(Centre(), { ViewportInteractionItem::ride });
                            if (const auto shop = ShopManagement::FromInfo(info))
                                Windows::HandheldRideOpen(shop->id);
                            else if (info.Element && !info.Element->isGhost())
                            {
                                const auto* ride = GetRide(info.Element->getRideIndex());
                                if (ride && !ride->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility))
                                    Windows::HandheldRideOpen(ride->id);
                            }
                            else if (!info.Element || !info.Element->isGhost())
                                ViewportInteractionLeftClick(Centre());
                        }
                        break;
                    case Action::contextPrimary:
                        if (Designs::Active() && !Designs::Busy())
                        {
                            Windows::HandheldDesignOptionsOpen();
                            input.suspend();
                            return;
                        }
                        if (Paths::IsActive() && !Paths::IsConnected())
                        {
                            Paths::ToggleRemove();
                            _terrainStatus.clear();
                            _scatterFeedback = false;
                            break;
                        }
                        Invoke(ShortcutId::kViewGeneralRotateClockwise);
                        break;
                    case Action::previousOption:
                        Windows::MainWindowZoom(false, false);
                        break;
                    case Action::nextOption:
                        Windows::MainWindowZoom(true, false);
                        break;
                    case Action::contextSecondary:
                        if (Paths::IsActive())
                        {
                            if (Paths::IsConnected())
                                Paths::TurnConnected();
                            else
                                Paths::CycleSurface();
                            _terrainStatus.clear();
                            _scatterFeedback = false;
                            break;
                        }
                        if (_terrainActive)
                        {
                            if (Designs::Active() && !Designs::Busy())
                                Windows::HandheldDesignPickerOpen();
                            else if (FlatRides::CurrentStage() == FlatRides::Stage::ride && !FlatRides::Busy())
                                Windows::HandheldRidePickerOpen();
                            else if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
                                break;
                            else if (_shopTool)
                                Windows::HandheldShopPickerOpen();
                            else if (PeepPlacement::Active())
                                Windows::HandheldPeepPlacementOptionsOpen();
                            else if (StaffPatrol::Active())
                                StaffPatrol::Configure(1);
                            else if (_rightsTool)
                                Windows::HandheldRightsOptionsOpen();
                            else if (_areaTool)
                                Windows::HandheldAreaOptionsOpen();
                            else if (_sceneryClearTool)
                                Windows::HandheldSceneryClearOptionsOpen();
                            else if (_sceneryTool)
                                Windows::HandheldSceneryOptionsOpen();
                            else if (_treeTool)
                            {
                                Windows::HandheldTreePickerOpen();
                            }
                            else if (_colourRide)
                                _colourScheme = (_colourScheme + 1) % kNumRideColourSchemes;
                            else if (_surfaceTool)
                            {
                                AreaTools::settings={}; AreaTools::settings.operation=AreaTools::Operation::paint;
                                AreaTools::settings.surface=_surfaceIndex; AreaTools::ResetColours();
                                _surfaceTool=false; _areaTool=true;
                                Windows::HandheldAreaOptionsOpen();
                            }
                            else if (!_sceneryRemovalTool)
                                _raiseLand = !_raiseLand;
                            _terrainStatus.clear();
                            _scatterFeedback = false;
                        }
                        break;
                    case Action::back:
                    case Action::openMenu:
                    case Action::openBuild:
                    case Action::quit:
                    case Action::slopeDown:
                    case Action::slopeUp:
                        if (Designs::Active())
                            Designs::SetScenery(action == Action::slopeUp);
                        else if (Paths::IsConnected())
                            Paths::SlopeConnected(action == Action::slopeUp ? 1 : -1);
                        else if (_sceneryTool)
                            SceneryTools::Configure(action == Action::slopeUp ? 4 : 5);
                        break;
                    case Action::cameraMode:
                        break;
                }
            }
            // Never let a second queued action confirm/close a newly opened window.
            input.suspend();
            return;
        }
        if (CanExplore() && (frame.panX != 0 || frame.panY != 0))
        {
            auto* main = WindowGetMain();
            _terrainStatus.clear();
            _scatterFeedback = false;
            GuestFollow::Stop();
            WindowUnfollowSprite(*main);
            InputScrollViewportSmooth({ frame.panX, frame.panY }, main);
        }
    }

    void DrawExplorer(Drawing::RenderTarget& rt)
    {
        if (!IsEnabled() || !CanExplore())
            return;
        using namespace Drawing;
        const auto c = Centre();
        const auto height = ContextGetHeight();
        const auto context = _terrainActive || Paths::IsActive() ? Layout::Context::construction : Layout::Context::park;
        if (Layout::ShowsHud(context) && rt.y < Layout::hudTop + Layout::hudHeight && rt.y + rt.height > Layout::hudTop)
            DrawHud(rt, { ContextGetWidth(), height }, NativeHud());
        // Completion can arrive after UpdateExplorer but before this frame is drawn.
        // Never fall through to terrain controls while waiting for the next update to end the tool.
        if (Designs::Completed()
            || (_terrainActive && FlatRides::CurrentStage() == FlatRides::Stage::inactive && FlatRides::Target()))
        {
            DrawGameplayFooter(rt, { ContextGetWidth(), height }, "", false, false);
            DrawMapFocus(rt, c);
            return;
        }
        if (Mazes::Active())
        {
            Mazes::DrawPanel(rt, { ContextGetWidth(), height });
            return;
        }
        if (CustomTracks::Active())
        {
            CustomTracks::DrawPanel(rt, { ContextGetWidth(), height });
            DrawMapFocus(rt, c);
            return;
        }
        std::string status = GuestFollow::Current() ? "Follow / B Guest / D-pad Stop" : "L/R: Zoom   X: Rotate";
        std::string costAmount;
        if (_terrainActive)
        {
            status = PeepPlacement::Active()                              ? "PLACE PERSON"
                : Designs::Active()                                       ? "PLACE DESIGN"
                : FlatRides::CurrentStage() != FlatRides::Stage::inactive ? "PLACE RIDE"
                : _shopTool                                               ? "PLACE SHOP"
                : _treeTool                                               ? "PLACE TREE"
                : _sceneryTool                                            ? "SCENERY"
                : _colourRide                                             ? "TRACK SCHEME " + std::to_string(_colourScheme + 1)
                : _sceneryRemovalTool                                     ? "REMOVE SCENERY"
                : _surfaceTool                                            ? "PAINT SURFACE"
                : _waterTool                                              ? (_raiseLand ? "RAISE WATER" : "LOWER WATER")
                                                                          : (_raiseLand ? "RAISE LAND" : "LOWER LAND");
            if (_terrainTile)
            {
                const auto result = TerrainAction(false);
                if (result.error == GameActions::Status::ok)
                {
                    costAmount = FormatMoney(result.cost) + (_sceneryTool && SceneryTools::ScatterActive() ? " est." : "");
                }
                else
                    status += "  " + result.getErrorMessage();
            }
            else
                status += _sceneryTool && SceneryTools::Current().mode == SceneryTools::Mode::pick ? "  Point at placed scenery"
                    : _colourRide     ? "  Point at this ride track / Y scheme"
                    : _sceneryRemovalTool ? "  Point at small scenery"
                                          : "  Move onto land";
            if (!_terrainStatus.empty())
                status = _sceneryTool && SceneryTools::ScatterActive() && _scatterFeedback ? SceneryTools::ScatterStatus() : _terrainStatus;
        }
        if (Paths::IsActive())
        {
            const auto result = Paths::Action(false);
            status = Paths::IsRemoving() ? "REMOVE PATH" : "PLACE PATH";
            if (result.error == GameActions::Status::ok)
            {
                costAmount = FormatMoney(result.cost);
            }
            else
                status = result.getErrorMessage();
            if (!_terrainStatus.empty())
                status = _sceneryTool && SceneryTools::ScatterActive() && _scatterFeedback ? SceneryTools::ScatterStatus() : _terrainStatus;
            Paths::DrawOverlay(rt, Concept::kOrigin);
            if (Paths::IsConnected())
                DrawConstructionFooter(rt, { ContextGetWidth(), height }, Paths::ConnectedReady() ? "Extend" : "Start", "Turn");
            else
                DrawPathFooter(rt, { ContextGetWidth(), height }, status, Paths::IsRemoving());
        }
        else
        {
            if (_terrainActive)
            {
                if (Designs::Active())
                    Designs::DrawPanel(rt, { ContextGetWidth(), height });
                else if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
                    FlatRides::DrawPanel(rt, { ContextGetWidth(), height });
                else if (_shopTool)
                    DrawShopOverlay(rt, Concept::kOrigin, Shops::Name(_shopIndex), Shops::Eligible(_shopIndex), _shopDirection);
                else if (PeepPlacement::Active())
                    PeepPlacement::DrawOverlay(rt, Concept::kOrigin);
                else if (StaffPatrol::Active())
                    StaffPatrol::DrawOverlay(rt, Concept::kOrigin);
                else if (_rightsTool)
                    DrawConstructionOverlay(
                        rt, Concept::kOrigin, "Land / Rights", RightsTools::Kind(RightsTools::settings.construction),
                        "A: Purchase review", RightsTools::Summary(_terrainTile, RightsTools::settings),
                        std::to_string(RightsTools::settings.width) + " x " + std::to_string(RightsTools::settings.height)
                            + " tiles");
                else if (_areaTool)
                    AreaTools::DrawOverlay(rt,Concept::kOrigin);
                else if (_sceneryClearTool)
                    SceneryClear::DrawOverlay(rt, Concept::kOrigin);
                else if (_sceneryTool)
                    SceneryTools::DrawOverlay(rt, Concept::kOrigin, _sceneryEdit);
                else if (_treeTool)
                    DrawTreeOverlay(rt, Concept::kOrigin, Trees::Name(_treeIndex), Trees::Eligible(_treeIndex));
                else if (_colourRide)
                    DrawTrackColourOverlay(rt, Concept::kOrigin, _colourScheme, _colourTarget.has_value());
                else if (_sceneryRemovalTool)
                    DrawSceneryRemovalOverlay(
                        rt, Concept::kOrigin, SceneryRemoval::Name(_sceneryTarget), _sceneryTarget.has_value());
                else if (_surfaceTool)
                    DrawSurfaceOverlay(rt, Concept::kOrigin, Surface::Name(_surfaceIndex));
                else
                    DrawTerrainOverlay(rt, Concept::kOrigin, _raiseLand, _waterTool);
            }
            if (Designs::Active() || FlatRides::CurrentStage() != FlatRides::Stage::inactive)
            { /* The shared ride panel owns its action strip. */
            }
            else if (_shopTool)
                DrawShopFooter(rt, { ContextGetWidth(), height });
            else if (PeepPlacement::Active())
                DrawConstructionFooter(rt, { ContextGetWidth(), height }, "Place", "Options");
            else if (StaffPatrol::Active())
                DrawConstructionFooter(rt, { ContextGetWidth(), height }, "Apply", "Add/Remove");
            else if (_rightsTool)
                DrawConstructionFooter(rt, { ContextGetWidth(), height }, "Review", "Options");
            else if (_sceneryClearTool)
                DrawConstructionFooter(rt, {ContextGetWidth(), height}, "Review", "Options");
            else if (_areaTool)
                DrawConstructionFooter(rt, { ContextGetWidth(), height }, AreaTools::settings.operation==AreaTools::Operation::paint?"Review":"Apply", "Brush");
            else if (_sceneryTool)
                DrawConstructionFooter(rt, { ContextGetWidth(), height },
                    SceneryTools::Current().mode == SceneryTools::Mode::pick ? "Pick" : SceneryTools::ScatterActive() && !SceneryTools::ScatterReviewed() ? "Review" : "Apply", "Options");
            else if (_treeTool)
                DrawTreeFooter(rt, { ContextGetWidth(), height });
            else if (_colourRide)
                DrawTrackColourFooter(rt, { ContextGetWidth(), height });
            else if (_sceneryRemovalTool)
                DrawSceneryRemovalFooter(rt, { ContextGetWidth(), height });
            else
                DrawGameplayFooter(
                    rt, { ContextGetWidth(), height }, status, _terrainActive, _surfaceTool || _colourRide.has_value());
        }
        const bool feedback = !_terrainStatus.empty();
        if ((_terrainActive || Paths::IsActive()) && (feedback || costAmount.empty()))
            DrawContextMessage(rt, Layout::ContextAnchor({ ContextGetWidth(), height }, kContextSize), status);
        else if (!costAmount.empty())
            DrawCostBadge(rt, Layout::ContextAnchor({ ContextGetWidth(), height }, kContextSize), costAmount);
        if (GuestFollow::Current() && !_terrainActive && !Paths::IsActive())
            DrawContextMessage(rt, Layout::ContextAnchor({ ContextGetWidth(), height }, kContextSize), status);
        // High contrast, fixed map focus; this does not move the desktop pointer.
        DrawMapFocus(rt, c);
    }
} // namespace OpenRCT2::Ui::Handheld

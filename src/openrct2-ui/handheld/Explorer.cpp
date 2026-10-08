/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Explorer.h"

#include "ControllerInput.h"
#include "Currency.h"
#include "Designs.h"
#include "FlatRides.h"
#include "Panel.h"
#include "Paths.h"
#include "RideAccess.h"
#include "SceneryRemoval.h"
#include "ShopManagement.h"
#include "Shops.h"
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
    static std::optional<SceneryRemoval::Target> _sceneryTarget;
    static ObjectEntryIndex _surfaceIndex = kObjectEntryIndexNull;
    static bool _raiseLand = true;
    static bool _selectionOwned{};
    static std::optional<CoordsXY> _terrainTile;
    static std::string _terrainStatus;
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
        _ridePathAccess.reset();
        Paths::End();
        Trees::ClearPreview();
        Shops::ClearPreview();
        FlatRides::End();
        Designs::End();
        _terrainActive = true;
        _waterTool = false;
        _surfaceTool = false;
        _treeTool = false;
        _shopTool = false;
        _shopIndex = kObjectEntryIndexNull;
        _treeIndex = kObjectEntryIndexNull;
        _sceneryRemovalTool = false;
        _sceneryTarget.reset();
        _surfaceIndex = kObjectEntryIndexNull;
        _raiseLand = raise;
        _terrainStatus.clear();
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

    void BeginTrees()
    {
        BeginTerrain(true);
        _treeTool = true;
        _treeIndex = Trees::Next(kObjectEntryIndexNull);
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
        Shops::ClearPreview();
        FlatRides::ClearPreview();
        Designs::ClearPreview();
        _treeIndex = index;
        _terrainStatus.clear();
        return true;
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
        _ridePathAccess.reset();
        Trees::ClearPreview();
        Shops::ClearPreview();
        FlatRides::End();
        Designs::End();
        _terrainActive = false;
        _waterTool = false;
        _surfaceTool = false;
        _treeTool = false;
        _shopTool = false;
        _shopIndex = kObjectEntryIndexNull;
        _treeIndex = kObjectEntryIndexNull;
        _sceneryRemovalTool = false;
        _sceneryTarget.reset();
        _surfaceIndex = kObjectEntryIndexNull;
        Paths::End();
        _terrainStatus.clear();
        ClearTerrainSelection();
    }

    static GameActions::Result TerrainAction(bool execute)
    {
        if (Designs::Active())
            return Designs::Query(_terrainTile);
        if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
            return FlatRides::Action(_terrainTile, execute);
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
                    WindowClass::handheldHud, Centre() + ScreenCoordsXY{ 32, 32 }, kContextSize,
                    { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
                if (cost != nullptr)
                    cost->number = 2;
            }
            if (cost != nullptr)
            {
                cost->invalidate();
                cost->windowPos = Centre() + ScreenCoordsXY{ 32, 32 };
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
            else if (frame.navigateY != 0)
                Windows::HandheldMenuMove(frame.navigateY);
            for (const auto action : frame.actions)
            {
                if (action == Action::confirm)
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
        if (gLegacyScene != LegacyScene::playing)
        {
            EndTerrain();
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
                gMapSelectFlags.set(MapSelectFlag::enable);
                _selectionOwned = true;
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
        if (Designs::Active() && CanExplore() && ContextHasFocus())
            Designs::UpdatePreview(_terrainTile);
        if (_shopTool && CanExplore() && ContextHasFocus())
            Shops::UpdatePreview(_terrainTile, _shopIndex, _shopDirection);
        if (_treeTool && CanExplore() && ContextHasFocus())
            Trees::UpdatePreview(_terrainTile, _treeIndex);
        if (!CanExplore() || !ContextHasFocus())
        {
            Paths::ClearPreview();
            Trees::ClearPreview();
            Shops::ClearPreview();
            FlatRides::ClearPreview();
            Designs::ClearPreview();
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
                    EndTerrain();
                    if (access && RideManagement::Resolve(*access))
                        Windows::HandheldRideAccessOpen(*access);
                }
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
                                const auto result = TerrainAction(true);
                                _terrainStatus = result.error == GameActions::Status::ok
                                    ? (FlatRides::CurrentStage() != FlatRides::Stage::inactive ? ""
                                           : _shopTool                                         ? "Shop placement requested."
                                           : _treeTool                                         ? "Tree placement requested."
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
                        if (Paths::IsActive())
                        {
                            Paths::ToggleRemove();
                            _terrainStatus.clear();
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
                            Paths::CycleSurface();
                            _terrainStatus.clear();
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
                            else if (_treeTool)
                            {
                                Windows::HandheldTreePickerOpen();
                            }
                            else if (_surfaceTool)
                                _surfaceIndex = Surface::Next(_surfaceIndex);
                            else if (!_sceneryRemovalTool)
                                _raiseLand = !_raiseLand;
                            _terrainStatus.clear();
                        }
                        break;
                    case Action::back:
                    case Action::openMenu:
                    case Action::openBuild:
                    case Action::quit:
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
        // Completion can arrive after UpdateExplorer but before this frame is drawn.
        // Never fall through to terrain controls while waiting for the next update to end the tool.
        if (Designs::Completed()
            || (_terrainActive && FlatRides::CurrentStage() == FlatRides::Stage::inactive && FlatRides::Target()))
        {
            DrawGameplayFooter(rt, { ContextGetWidth(), height }, "", false, false);
            DrawMapFocus(rt, c);
            return;
        }
        std::string status = "L/R: Zoom   X: Rotate";
        std::string costAmount;
        if (_terrainActive)
        {
            status = Designs::Active()                                    ? "PLACE DESIGN"
                : FlatRides::CurrentStage() != FlatRides::Stage::inactive ? "PLACE RIDE"
                : _shopTool                                               ? "PLACE SHOP"
                : _treeTool                                               ? "PLACE TREE"
                : _sceneryRemovalTool                                     ? "REMOVE SCENERY"
                : _surfaceTool                                            ? "PAINT SURFACE"
                : _waterTool                                              ? (_raiseLand ? "RAISE WATER" : "LOWER WATER")
                                                                          : (_raiseLand ? "RAISE LAND" : "LOWER LAND");
            if (_terrainTile)
            {
                const auto result = TerrainAction(false);
                if (result.error == GameActions::Status::ok)
                {
                    costAmount = FormatMoney(result.cost);
                }
                else
                    status += "  " + result.getErrorMessage();
            }
            else
                status += _sceneryRemovalTool ? "  Point at small scenery" : "  Move onto land";
            if (!_terrainStatus.empty())
                status = _terrainStatus;
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
                status = _terrainStatus;
            DrawPathOverlay(
                rt, Concept::kOrigin, Paths::SurfaceName(), Paths::IsQueue(), Paths::IsRemoving(), Paths::PlacementName());
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
                else if (_treeTool)
                    DrawTreeOverlay(rt, Concept::kOrigin, Trees::Name(_treeIndex), Trees::Eligible(_treeIndex));
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
            else if (_treeTool)
                DrawTreeFooter(rt, { ContextGetWidth(), height });
            else if (_sceneryRemovalTool)
                DrawSceneryRemovalFooter(rt, { ContextGetWidth(), height });
            else
                DrawGameplayFooter(rt, { ContextGetWidth(), height }, status, _terrainActive, _surfaceTool);
        }
        const bool feedback = !_terrainStatus.empty();
        if ((_terrainActive || Paths::IsActive()) && (feedback || costAmount.empty()))
            DrawContextMessage(rt, c + ScreenCoordsXY{ 32, 32 }, status);
        else if (!costAmount.empty())
            DrawCostBadge(rt, c + ScreenCoordsXY{ 32, 32 }, costAmount);
        // High contrast, fixed map focus; this does not move the desktop pointer.
        DrawMapFocus(rt, c);
    }
} // namespace OpenRCT2::Ui::Handheld

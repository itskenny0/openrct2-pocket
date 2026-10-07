/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Explorer.h"

#include "ControllerInput.h"
#include "Panel.h"
#include "Paths.h"
#include "Surface.h"
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
    static ObjectEntryIndex _surfaceIndex = kObjectEntryIndexNull;
    static bool _raiseLand = true;
    static bool _selectionOwned{};
    static std::optional<CoordsXY> _terrainTile;
    static std::string _terrainStatus;

    void BeginTerrain(bool raise)
    {
        Paths::End();
        _terrainActive = true;
        _waterTool = false;
        _surfaceTool = false;
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

    static void ClearTerrainSelection()
    {
        if (_selectionOwned)
        {
            MapSelection::invalidate();
            gMapSelectFlags.unset(MapSelectFlag::enable);
            MapSelection::invalidate();
            _selectionOwned = false;
        }
        _terrainTile.reset();
    }

    void EndTerrain()
    {
        _terrainActive = false;
        _waterTool = false;
        _surfaceTool = false;
        _surfaceIndex = kObjectEntryIndexNull;
        Paths::End();
        _terrainStatus.clear();
        ClearTerrainSelection();
    }

    static GameActions::Result TerrainAction(bool execute)
    {
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

    void UpdateExplorer(ControllerInput& input)
    {
        if (gLegacyScene != LegacyScene::playing)
        {
            EndTerrain();
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
        if (!CanExplore() || !ContextHasFocus())
            Paths::ClearPreview();
        SyncFocusWindow();
        auto frame = input.update(ContextHasFocus(), SDL_GetTicks());
        auto* wm = GetWindowManager();
        for (const auto action : frame.actions)
        {
            if (action == Action::quit)
            {
                Paths::End();
                ContextQuit();
                input.suspend();
                return;
            }
        }
        if (wm->FindByClass(WindowClass::handheldMenu) != nullptr)
        {
            if (frame.navigateY != 0)
                Windows::HandheldMenuMove(frame.navigateY);
            for (const auto action : frame.actions)
            {
                if (action == Action::confirm)
                    Windows::HandheldMenuConfirm();
                else if (action == Action::back || action == Action::openMenu)
                    Windows::HandheldMenuBack();
                input.suspend();
                break;
            }
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
            if (action == Action::back)
            {
                if (wm->FindByClass(WindowClass::error) != nullptr || gInputFlags.has(InputFlag::toolActive))
                    Invoke(ShortcutId::kInterfaceCancelConstruction);
                else if (wm->FindByClass(WindowClass::dropdown) != nullptr)
                    wm->CloseByClass(WindowClass::dropdown);
                else if ((_terrainActive || Paths::IsActive()) && CanExplore())
                {
                    EndTerrain();
                }
                else
                    wm->CloseTop();
            }
            else if (action == Action::openMenu && !gInputFlags.has(InputFlag::toolActive))
            {
                Windows::HandheldMenuOpen();
            }
            else if (action == Action::openBuild && CanExplore())
            {
                if (Paths::IsActive())
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
                                const auto result = TerrainAction(true);
                                _terrainStatus = result.error == GameActions::Status::ok
                                    ? (_surfaceTool     ? "Surface change requested."
                                           : _waterTool ? "Water change requested."
                                                        : "Land changed.")
                                    : result.getErrorMessage();
                            }
                        }
                        else
                        {
                            const auto info = GetMapCoordinatesFromPos(Centre(), { ViewportInteractionItem::ride });
                            if (info.Element)
                                Windows::HandheldRideOpen(info.Element->getRideIndex());
                            else
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
                            if (_surfaceTool)
                                _surfaceIndex = Surface::Next(_surfaceIndex);
                            else
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
        std::string status = "L/R: Zoom   X: Rotate";
        std::string costAmount;
        if (_terrainActive)
        {
            status = _surfaceTool ? "PAINT SURFACE"
                : _waterTool      ? (_raiseLand ? "RAISE WATER" : "LOWER WATER")
                                  : (_raiseLand ? "RAISE LAND" : "LOWER LAND");
            if (_terrainTile)
            {
                const auto result = TerrainAction(false);
                if (result.error == GameActions::Status::ok)
                {
                    std::ostringstream cost;
                    cost << "$" << std::fixed << std::setprecision(2) << result.cost / 100.0;
                    costAmount = cost.str();
                }
                else
                    status += "  " + result.getErrorMessage();
            }
            else
                status += "  Move onto land";
            if (!_terrainStatus.empty())
                status = _terrainStatus;
        }
        if (Paths::IsActive())
        {
            const auto result = Paths::Action(false);
            status = Paths::IsRemoving() ? "REMOVE PATH" : "PLACE PATH";
            if (result.error == GameActions::Status::ok)
            {
                std::ostringstream cost;
                cost << "$" << std::fixed << std::setprecision(2) << result.cost / 100.0;
                costAmount = cost.str();
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
                if (_surfaceTool)
                    DrawSurfaceOverlay(rt, Concept::kOrigin, Surface::Name(_surfaceIndex));
                else
                    DrawTerrainOverlay(rt, Concept::kOrigin, _raiseLand, _waterTool);
            }
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

/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Explorer.h"

#include "ControllerInput.h"
#include "Panel.h"

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
#include <sstream>

namespace OpenRCT2::Ui::Handheld
{
    static bool _terrainActive{};
    static bool _raiseLand = true;
    static bool _selectionOwned{};
    static std::optional<CoordsXY> _terrainTile;
    static std::string _terrainStatus;

    void BeginTerrain(bool raise)
    {
        _terrainActive = true;
        _raiseLand = raise;
        _terrainStatus.clear();
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
        _terrainStatus.clear();
        ClearTerrainSelection();
    }

    static GameActions::Result TerrainAction(bool execute)
    {
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
                WindowClass::handheldHud, { 0, ContextGetHeight() - 34 }, { ContextGetWidth(), 34 },
                { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
        }
        if (hud != nullptr)
        {
            hud->invalidate();
            hud->windowPos = { 0, ContextGetHeight() - 34 };
            hud->width = ContextGetWidth();
            hud->invalidate();
        }
        // A separate small input-transparent window redraws only the tool panel,
        // rather than placing a full-screen overlay over the scrolling viewport.
        auto* tools = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 1 });
        if (_terrainActive)
        {
            if (tools == nullptr)
            {
                tools = wm->Create<FocusWindow>(
                    WindowClass::handheldHud, { 8, 52 }, kTerrainOverlaySize,
                    { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
                if (tools != nullptr)
                    tools->number = 1;
            }
            if (tools != nullptr)
                tools->invalidate();
        }
        else if (tools != nullptr)
            wm->Close(*tools);
    }

    void UpdateExplorer(ControllerInput& input)
    {
        if (gLegacyScene != LegacyScene::playing)
            _terrainActive = false;
        ClearTerrainSelection();
        if (_terrainActive && CanExplore())
        {
            auto coords = ScreenGetMapXY(Centre(), nullptr);
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
        SyncFocusWindow();
        auto frame = input.update(ContextHasFocus(), SDL_GetTicks());
        auto* wm = GetWindowManager();
        for (const auto action : frame.actions)
        {
            if (action == Action::quit)
            {
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
                else if (_terrainActive && CanExplore())
                {
                    _terrainActive = false;
                    ClearTerrainSelection();
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
                _terrainActive = false;
                ClearTerrainSelection();
                Windows::HandheldBuildOpen();
            }
            else if (CanExplore())
            {
                switch (action)
                {
                    case Action::confirm:
                        if (_terrainActive)
                        {
                            if (_terrainTile)
                            {
                                const auto result = TerrainAction(true);
                                _terrainStatus = result.error == GameActions::Status::ok ? "Land changed."
                                                                                         : result.getErrorMessage();
                            }
                        }
                        else
                            ViewportInteractionLeftClick(Centre());
                        break;
                    case Action::contextPrimary:
                        Invoke(ShortcutId::kViewGeneralRotateClockwise);
                        break;
                    case Action::previousOption:
                        Windows::MainWindowZoom(false, false);
                        break;
                    case Action::nextOption:
                        Windows::MainWindowZoom(true, false);
                        break;
                    case Action::contextSecondary:
                        if (_terrainActive)
                        {
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
        if (_terrainActive)
        {
            status = _raiseLand ? "RAISE LAND" : "LOWER LAND";
            if (_terrainTile)
            {
                const auto result = TerrainAction(false);
                if (result.error == GameActions::Status::ok)
                {
                    std::ostringstream cost;
                    cost << "  Cost: $" << std::fixed << std::setprecision(2) << result.cost / 100.0;
                    status += cost.str();
                }
                else
                    status += "  " + result.getErrorMessage();
            }
            else
                status += "  Move onto land";
            if (!_terrainStatus.empty())
                status = _terrainStatus;
        }
        if (_terrainActive)
            DrawTerrainOverlay(rt, { 8, 52 }, _raiseLand);
        DrawGameplayFooter(rt, { ContextGetWidth(), height }, status, _terrainActive);
        // High contrast, fixed map focus; this does not move the desktop pointer.
        DrawMapFocus(rt, c);
    }
} // namespace OpenRCT2::Ui::Handheld

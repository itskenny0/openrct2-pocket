/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Explorer.h"

#include "ControllerInput.h"

#include <SDL_timer.h>
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
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/drawing/Line.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/ui/WindowManager.h>

namespace OpenRCT2::Ui::Handheld
{
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
    }

    void UpdateExplorer(ControllerInput& input)
    {
        SyncFocusWindow();
        auto frame = input.update(ContextHasFocus(), SDL_GetTicks());
        if (gLegacyScene != LegacyScene::playing || HasTextOrModalInput())
        {
            input.suspend();
            return;
        }
        for (const auto action : frame.actions)
        {
            if (action == Action::back)
            {
                auto* wm = GetWindowManager();
                if (wm->FindByClass(WindowClass::error) != nullptr || gInputFlags.has(InputFlag::toolActive))
                    Invoke(ShortcutId::kInterfaceCancelConstruction);
                else if (wm->FindByClass(WindowClass::dropdown) != nullptr)
                    wm->CloseByClass(WindowClass::dropdown);
                else
                    wm->CloseTop();
            }
            else if (CanExplore())
            {
                switch (action)
                {
                    case Action::inspect:
                        ViewportInteractionLeftClick(Centre());
                        break;
                    case Action::rotate:
                        Invoke(ShortcutId::kViewGeneralRotateClockwise);
                        break;
                    case Action::zoomOut:
                        Windows::MainWindowZoom(false, false);
                        break;
                    case Action::zoomIn:
                        Windows::MainWindowZoom(true, false);
                        break;
                    case Action::pause:
                        Invoke(ShortcutId::kInterfacePause);
                        break;
                    case Action::back:
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
        // High contrast, fixed map focus; this does not move the desktop pointer.
        GfxDrawLine(rt, { c + ScreenCoordsXY{ -7, -1 }, c + ScreenCoordsXY{ 7, -1 } }, PaletteIndex::pi10);
        GfxDrawLine(rt, { c + ScreenCoordsXY{ -7, 1 }, c + ScreenCoordsXY{ 7, 1 } }, PaletteIndex::pi10);
        GfxDrawLine(rt, { c + ScreenCoordsXY{ -1, -7 }, c + ScreenCoordsXY{ -1, 7 } }, PaletteIndex::pi10);
        GfxDrawLine(rt, { c + ScreenCoordsXY{ 1, -7 }, c + ScreenCoordsXY{ 1, 7 } }, PaletteIndex::pi10);
        GfxDrawLine(rt, { c + ScreenCoordsXY{ -6, 0 }, c + ScreenCoordsXY{ 6, 0 } }, PaletteIndex::pi21);
        GfxDrawLine(rt, { c + ScreenCoordsXY{ 0, -6 }, c + ScreenCoordsXY{ 0, 6 } }, PaletteIndex::pi21);
    }
} // namespace OpenRCT2::Ui::Handheld

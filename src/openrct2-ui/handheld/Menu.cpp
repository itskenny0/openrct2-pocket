// SPDX-License-Identifier: GPL-3.0-only
#include "ListFocus.h"

#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/input/ShortcutIds.h>
#include <openrct2-ui/input/ShortcutManager.h>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/drawing/Line.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/Text.h>
#include <openrct2/localisation/StringIds.h>
#include <openrct2/ui/WindowManager.h>

namespace OpenRCT2::Ui::Windows
{
    using namespace Drawing;
    static constexpr ScreenSize kSize{ 300, 224 };
    static constexpr auto kWidgets = makeWidgets(makeWindowShim(STR_SHORTCUT_PAUSE, kSize));
    static constexpr const char* kRows[] = { "Resume Park", "Save Park", "Load Park", "Options", "Exit OpenRCT2" };

    class HandheldMenuWindow final : public Window
    {
        Handheld::ListFocus _focus;
        bool _pausedHere{};
        bool _exitPending{};

    public:
        void onOpen() override
        {
            setWidgets(kWidgets);
            colours[0] = ColourWithFlags{ Colour::darkBlue };
            colours[1] = ColourWithFlags{ Colour::darkBlue };
            _pausedHere = GameIsNotPaused();
            if (_pausedHere)
                PauseToggle();
        }

        void onClose() override
        {
            if (_pausedHere && GameIsPaused())
                PauseToggle();
        }

        void move(int32_t direction)
        {
            _focus.move(direction, std::size(kRows));
            _exitPending = false;
            invalidate();
        }

        void confirm()
        {
            const auto choice = _focus.index();
            if (choice == 4 && !_exitPending)
            {
                _exitPending = true;
                invalidate();
                return;
            }
            close();
            switch (choice)
            {
                case 1:
                    SaveGame();
                    break;
                case 2:
                    if (auto* shortcut = GetShortcutManager().getShortcut(ShortcutId::kInterfaceLoadGame))
                        shortcut->action();
                    break;
                case 3:
                    ContextOpenWindow(WindowClass::options);
                    break;
                case 4:
                    ContextQuit();
                    break;
                default:
                    break;
            }
        }

        void onMouseUp(WidgetIndex widget) override
        {
            if (widget == 2)
                close();
        }

        void onDraw(RenderTarget& rt) override
        {
            drawWidgets(rt);
            for (size_t i = 0; i < std::size(kRows); ++i)
            {
                const auto y = windowPos.y + 30 + static_cast<int32_t>(i) * 32;
                const auto selected = i == _focus.index();
                if (selected)
                {
                    GfxDrawLine(rt, { { windowPos.x + 12, y - 5 }, { windowPos.x + width - 13, y - 5 } }, PaletteIndex::pi21);
                    GfxDrawLine(rt, { { windowPos.x + 12, y + 18 }, { windowPos.x + width - 13, y + 18 } }, PaletteIndex::pi21);
                }
                drawText(rt, { windowPos.x + 24, y }, kRows[i], { selected ? Colour::yellow : Colour::white });
            }
            drawText(
                rt, windowPos + ScreenCoordsXY{ 16, 202 },
                _exitPending ? "A: Exit WITHOUT saving   B: Back" : "D-pad: Choose   A: Select   B: Back", { Colour::white });
        }
    };

    void HandheldMenuOpen()
    {
        auto* wm = GetWindowManager();
        if (wm->BringToFrontByClass(WindowClass::handheldMenu) == nullptr)
            wm->Create<HandheldMenuWindow>(WindowClass::handheldMenu, {}, kSize, { WindowFlag::centreScreen });
    }

    void HandheldMenuMove(int32_t direction)
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->move(direction);
    }

    void HandheldMenuConfirm()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->confirm();
    }
} // namespace OpenRCT2::Ui::Windows

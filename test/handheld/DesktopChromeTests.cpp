// SPDX-License-Identifier: GPL-3.0-only
// Native compositor/hit-test regression; synthetic colour windows need no proprietary sprites.
#include <SDL.h>
#include <algorithm>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/WindowVisibility.h>
#include <openrct2/Context.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/interface/WindowBase.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <stdexcept>
#include <vector>
using namespace OpenRCT2;
static void Require(bool b, const char* message)
{
    if (!b)
        throw std::runtime_error(message);
}
class ColourWindow final : public WindowBase
{
public:
    int draws{};
    Drawing::PaletteIndex ink{ static_cast<Drawing::PaletteIndex>(0) };
    void onDraw(Drawing::RenderTarget& rt) override
    {
        ++draws;
        for (int y = 0; y < rt.height; y++)
            std::fill_n(rt.bits + y * rt.LineStride(), rt.width, ink);
    }
};
int main(int argc, char** argv)
{
    try
    {
        if (argc != 4)
            return 2;
        const bool pocket = std::string_view(argv[3]) == "pocket";
        SDL_setenv("OPENRCT2_HANDHELD", pocket ? "1" : "0", 1);
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;
        auto env = CreatePlatformEnvironment();
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        Require(context->Initialise(), "Native context failed");
        auto* wm = Ui::GetWindowManager();
        wm->CloseAllExceptFlags({});
        WindowCullDead();
        constexpr auto baseInk = static_cast<Drawing::PaletteIndex>(19), chromeInk = static_cast<Drawing::PaletteIndex>(53);
        std::vector<Drawing::PaletteIndex> pixels(32 * 32);
        Drawing::RenderTarget rt;
        rt.bits = pixels.data();
        rt.width = 32;
        rt.height = 32;
        const WindowClass chrome[] = { WindowClass::topToolbar,    WindowClass::gameStatusBar, WindowClass::parkInfoPanel,
                                       WindowClass::dateInfoPanel, WindowClass::newsTicker,    WindowClass::titleLogo,
                                       WindowClass::titleExit,     WindowClass::titleOptions,  WindowClass::titleVersion };
        for (auto cls : chrome)
        {
            Require(Ui::Handheld::IsDesktopChrome(cls), "Chrome class omitted");
            for (bool transparent : { false, true })
            {
                auto* base = wm->Create<ColourWindow>(
                    WindowClass::mainWindow, ScreenCoordsXY{ 0, 0 }, ScreenSize{ 32, 32 }, WindowFlags{});
                base->ink = baseInk;
                WindowFlags flags{ WindowFlag::stickToFront };
                if (transparent)
                    flags.set(WindowFlag::transparent);
                auto* over = wm->Create<ColourWindow>(cls, ScreenCoordsXY{ 4, 4 }, ScreenSize{ 8, 8 }, flags);
                over->ink = chromeInk;
                Require(over->flags.has(WindowFlag::hidden) == pocket, "Mode-specific visibility incorrect");
                std::fill(pixels.begin(), pixels.end(), static_cast<Drawing::PaletteIndex>(0));
                WindowDrawAll(rt, 0, 0, 32, 32);
                Require(over->draws == (pocket ? 0 : 1), "Hidden chrome drawn or desktop chrome removed");
                Require(pixels[5 * 32 + 5] == (pocket ? baseInk : chromeInk), "Chrome occluded native background");
                Require(wm->FindFromPoint({ 5, 5 }) == (pocket ? base : over), "Hidden chrome intercepted hit test");
                wm->CloseAllExceptFlags({});
                WindowCullDead();
            }
        }
        for (auto cls : { WindowClass::handheldMenu, WindowClass::handheldHud, WindowClass::titleMenu, WindowClass::error })
        {
            Require(!Ui::Handheld::IsDesktopChrome(cls), "Pocket/error/menu classified as chrome");
            // Native error themes are transparent; retain the underlying viewport layer.
            auto* base = wm->Create<ColourWindow>(
                WindowClass::mainWindow, ScreenCoordsXY{ 0, 0 }, ScreenSize{ 32, 32 }, WindowFlags{});
            base->ink = baseInk;
            auto* w = wm->Create<ColourWindow>(cls, ScreenCoordsXY{ 0, 0 }, ScreenSize{ 32, 32 }, WindowFlags{});
            w->ink = chromeInk;
            Require(!w->flags.has(WindowFlag::hidden), "Native/Pocket recovery window hidden");
            std::fill(pixels.begin(), pixels.end(), static_cast<Drawing::PaletteIndex>(0));
            WindowDrawAll(rt, 0, 0, 32, 32);
            Require(w->draws == 1 && pixels[0] == chromeInk, "Recovery/menu window not rendered");
            wm->CloseAllExceptFlags({});
            WindowCullDead();
        }
        std::cout << "PASS native compositor: " << (pocket ? "Pocket suppression" : "desktop preservation")
                  << "; opaque/transparent chrome, pixels, hit tests, recreation and recovery windows\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

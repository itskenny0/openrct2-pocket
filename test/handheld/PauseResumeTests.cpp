// SPDX-License-Identifier: GPL-3.0-only
#include <SDL.h>
#include <array>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/interface/WindowBase.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <stdexcept>
using namespace OpenRCT2;
namespace W = Ui::Windows;
static void Check(bool b, const char* s)
{
    if (!b)
        throw std::runtime_error(s);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 3)
            return 2;
        SDL_setenv("OPENRCT2_HANDHELD", "1", 1);
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;
        auto env = CreatePlatformEnvironment();
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        Check(context->Initialise(), "Context failed");
        auto* wm = Ui::GetWindowManager();
        wm->CloseAllExceptFlags({});
        WindowCullDead();
        gameStateInitAll(getGameState(), { 16, 16 });
        gLegacyScene = LegacyScene::playing;
        // Arrange native pause-reason fixtures; production must use PauseToggle rather than direct flag writes.
        for (bool back : { false, true })
            for (int initial :
                 std::array<int, 5>{ 0, GAME_PAUSED_NORMAL, GAME_PAUSED_MODAL, GAME_PAUSED_NORMAL | GAME_PAUSED_MODAL,
                                     GAME_PAUSED_NORMAL | GAME_PAUSED_SAVING_TRACK })
            {
                gGamePaused = initial;
                W::HandheldMenuOpen();
                if (back)
                    W::HandheldMenuBack();
                else
                    W::HandheldMenuConfirm();
                Check(
                    gGamePaused == (back ? initial : (initial & ~GAME_PAUSED_NORMAL)),
                    "Back changed prior pause or Resume cleared an engine pause reason");
                WindowCullDead();
            }
        for (bool back : { false, true })
        {
            gGamePaused = 0;
            W::HandheldMenuOpen();
            W::HandheldMenuMove(7);
            W::HandheldMenuConfirm();
            if (back)
                W::HandheldMenuBack();
            else
                W::HandheldMenuConfirm();
            W::HandheldMenuConfirm(); // restored Controls focus reopens the guide
            W::HandheldMenuBack();
            W::HandheldMenuBack();
            WindowCullDead();
            Check(!(gGamePaused & GAME_PAUSED_NORMAL), "Controller guide trapped pause or failed Back return");
        }
        gGamePaused = 0;
        W::HandheldMenuOpen();
        Check(gGamePaused & GAME_PAUSED_NORMAL, "Menu did not acquire normal pause");
        gGamePaused = GAME_PAUSED_MODAL;
        wm->CloseByClass(WindowClass::handheldMenu);
        Check(gGamePaused == GAME_PAUSED_MODAL, "Owned-menu cleanup reintroduced normal pause");
        WindowCullDead();
        gGamePaused = GAME_PAUSED_NORMAL;
        W::HandheldPathOptionsOpen();
        wm->CloseByClass(WindowClass::handheldMenu);
        Check(gGamePaused == GAME_PAUSED_NORMAL, "Non-Resume close changed an existing pause");
        WindowCullDead();
        gGamePaused = 0;
        W::HandheldBuildOpen();
        W::HandheldMenuMove(2 - 8);
        W::HandheldMenuConfirm(); // Build opens on Rides (8); Paths is action 2.
        W::HandheldMenuMove(1);   // Queue mode; remember this focus on returning.
        W::HandheldMenuBack();
        W::HandheldMenuConfirm(); // Returns to Paths, not a different build tool.
        W::HandheldMenuConfirm();
        Check(
            Ui::Handheld::Paths::IsActive() && Ui::Handheld::Paths::IsQueue(),
            "Path parent/focus navigation lost the queue choice");
        Ui::Handheld::Paths::End();
        gGamePaused = GAME_PAUSED_NORMAL;
        const auto park = std::string(argv[2]) + "/paused-roundtrip.park";
        Check(ScenarioSave(getGameState(), park, {}) != 0, "Paused native save failed");
        Check(context->LoadParkFromFile(park), "Paused native reload failed");
        W::HandheldMenuOpen();
        W::HandheldMenuConfirm();
        Check(!(gGamePaused & GAME_PAUSED_NORMAL), "Loaded park could not resume");
        std::cout << "PASS paused native save/reload and explicit Resume\n";
        std::cout << "PASS Build/Paths back hierarchy and focused queue return\n";
        std::cout << "PASS Resume clears normal pause; Back preserves prior pause; both preserve engine reasons\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

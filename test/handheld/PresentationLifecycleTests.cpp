// SPDX-License-Identifier: GPL-3.0-only
// Real SDL/window lifecycle with public engine data; graphics-disabled native park fixture.
#include <SDL.h>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <stdexcept>

using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace W = Ui::Windows;
static void Require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
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
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Config::Get().general.windowScale = 1;
        Require(context->Initialise(), "Public-data UI fixture initialise failed");
        context->GetUiContext().CreateWindow();
        auto& state = getGameState();
        gameStateInitAll(state, { 16, 16 });
        gLegacyScene = LegacyScene::playing;
        Require(W::MainOpen(), "Fixture main viewport missing");
        InputSetState(InputState::normal);
        // Graphics-disabled context initialization leaves its native progress window pending a UI tick.
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        // UI metrics can use public G2 fonts without any original park/world sprite assets.
        gOpenRCT2NoGraphics = false;
        GfxLoadG2PalettesFontsTracks();
        FontSpriteInitialiseCharacters();
        auto* wm = Ui::GetWindowManager();
        H::ControllerInput input;
        auto top = [&] { return wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 3 }); };
        H::UpdateExplorer(input);
        Require(top(), "Exploration HUD window missing");
        Require(
            top()->windowPos == ScreenCoordsXY{ H::Layout::margin, H::Layout::hudTop } && top()->height == H::Layout::hudHeight,
            "HUD window/safe area disagrees");
        const auto cash = state.park.cash;
        const auto cashLabel = H::NativeHud().cash;
        H::BeginWater();
        H::UpdateExplorer(input);
        Require(top(), "Construction lost persistent HUD");
        const auto* tools = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 1 });
        Require(tools && tools->windowPos == H::Layout::toolOrigin, "Construction panel didn't use shared inset");
        W::HandheldDashboardOpen();
        H::UpdateExplorer(input);
        Require(!top(), "Dashboard retained colliding HUD window");
        W::HandheldMenuBack();
        H::UpdateExplorer(input);
        Require(top(), "Returning to park did not recreate HUD");
        W::HandheldBuildOpen();
        H::UpdateExplorer(input);
        Require(!top(), "Build menu retained HUD");
        W::HandheldMenuBack();
        H::UpdateExplorer(input);
        Require(!top(), "Build Back skipped its existing pause-menu level");
        W::HandheldMenuBack();
        H::UpdateExplorer(input);
        Require(top(), "Build cancellation lost HUD");
        H::EndTerrain();
        SaveFlags flags{};
        const auto save = std::string(argv[2]) + "/hud-lifecycle.park";
        Require(ScenarioSave(state, save, flags), "HUD fixture save failed");
        gOpenRCT2NoGraphics = true;
        Require(context->LoadParkFromFile(save), "HUD fixture reload failed");
        gOpenRCT2NoGraphics = false;
        wm->CloseByClass(WindowClass::progressWindow);
        InputSetState(InputState::normal);
        H::UpdateExplorer(input);
        Require(
            H::NativeHud().cash == cashLabel && state.park.cash == cash,
            "HUD/save mutated native accounting or cached stale cash");
        H::BeginTerrain(true);
        H::UpdateExplorer(input);
        context.reset(); // Exercise the confirmed pre-unload cleanup with UI/tool windows alive.
        std::cout << "Pocket SDL HUD windows / construction inset / Dashboard-Build yield-return / native save-reload / "
                     "shutdown passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

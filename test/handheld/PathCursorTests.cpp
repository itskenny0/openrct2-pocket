// SPDX-License-Identifier: GPL-3.0-only
// RC3 defect investigation: real screen-coordinate targeting on a native flat map.
// This is a desktop reproduction probe, not proof that the reported Brick defect is fixed.
#include <SDL.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/PathBatch.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <stdexcept>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace W = Ui::Windows;
static void Check(bool ok, const char* message)
{
    if (!ok)
        throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 4)
            return 2;
        SDL_setenv("OPENRCT2_HANDHELD", "1", 1);
        SDL_setenv("SDL_VIDEODRIVER", "offscreen", 0);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 0);
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = false;
        auto env = CreatePlatformEnvironment();
        env->SetBasePath(DirBase::rct2, argv[3]);
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        Config::Get().general.rct2Path = argv[3];
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Config::Get().general.windowScale = 1;
        Check(context->Initialise(), "Context failed");
        context->GetUiContext().CreateWindow();
        auto& state = getGameState();
        gameStateInitAll(state, { 64, 64 });
        gLegacyScene = LegacyScene::playing;
        Check(W::MainOpen(), "No viewport");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        auto* wm = Ui::GetWindowManager();
        wm->CloseByClass(WindowClass::progressWindow);
        state.park.cash = 10000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        for (int y = 32; y < 2016; y += 32)
            for (int x = 32; x < 2016; x += 32)
                MapGetSurfaceElementAt(CoordsXY{ x, y })->setOwnership({ OwnershipFlag::landOwned });
        auto& objects = context->GetObjectManager();
        for (const char* id :
             { "rct2.terrain_surface.grass", "rct2.terrain_edge.rock", "rct2.station.plain", "rct2.footpath_surface.tarmac",
               "rct2.footpath_surface.queue_blue", "rct2.footpath_railings.wood" })
            Check(objects.LoadObject(id), "Fixture object missing");
        namespace P = H::Paths;
        H::ControllerInput input;
        auto press = [&](H::Action action) {
            H::Frame frame;
            frame.actions = { action, action };
            H::HandleExplorerInput(input, frame);
        };
        auto begin = [&](P::BuildMode mode, bool queue = false) {
            wm->CloseByClass(WindowClass::error);
            WindowCullDead();
            P::End();
            P::Begin(queue);
            if (mode != P::BuildMode::single)
                P::CycleMode(mode == P::BuildMode::line ? 1 : -1);
        };
        auto path = [&](int x, int y) -> PathElement* {
            for (auto* p : TileElementsView<PathElement>(CoordsXY{ x, y }))
                if (!p->isGhost())
                    return p;
            return nullptr;
        };
        auto capture = [&](const char* name) {
            const auto* folder = std::getenv("POCKET_PATH_BATCH_CAPTURES");
            if (!folder)
                return;
            std::filesystem::create_directories(folder);
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, WindowGetMain()->viewport);
            if (auto* window = wm->FindByClass(WindowClass::handheldMenu))
                window->onDraw(rt);
            else
                H::DrawExplorer(rt);
            Check(H::FooterHeight({ 512, 384 }, H::LastControlHints) == 28, "Runtime footer expanded beyond one row");
            Image image;
            image.Width = 512;
            image.Height = 384;
            image.Depth = 8;
            image.Stride = rt.LineStride();
            image.Palette = Drawing::gPalette;
            const auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(folder) / name).string(), image, ImageFormat::png);
            engine.EndDraw();
        };
        // Actual per-frame targeting, without supplying InteractionInfo to the tool.
        auto aim = [&](int x, int y) {
            auto* main = WindowGetMain();
            auto* v = main->viewport;
            v->zoom = ZoomLevel{ 0 };
            const auto z = MapGetSurfaceElementAt(CoordsXY{ x, y })->getBaseZ();
            v->viewPos = Translate3DTo2DWithZ(v->rotation, { x + 16, y + 16, z }) - ScreenCoordsXY{ 256, 192 };
            main->savedViewPos = v->viewPos;
            H::UpdateExplorer(input);
            // Headless focus differs from a physical window; still use the actual native hit test.
            P::UpdateTarget({ 256, 192 });
            P::UpdatePreview();
            const auto t = P::TargetTile();
            Check(t.has_value(), "No real cursor target");
            std::cout << "aim " << x << "," << y << " -> " << t->x << "," << t->y << " " << P::PlacementName() << "\n";
        };
        begin(P::BuildMode::single);
        aim(512, 512);
        press(H::Action::contextSecondary);
        press(H::Action::nextOption);
        press(H::Action::back);
        Check(P::Mode() == P::BuildMode::line, "Actual Line entry failed");
        aim(512, 512);
        press(H::Action::confirm);
        aim(672, 512);
        capture("live-line-preview.png");
        const auto cost = P::Action(false).cost;
        const auto cash = state.park.cash;
        press(H::Action::confirm);
        press(H::Action::confirm);
        gameStateUpdateLogic();
        Check(state.park.cash == cash - cost, "Live Line charge mismatch");
        for (int x = 512; x <= 672; x += 32)
            Check(path(x, 512), "Live Line real path missing");
        capture("live-line-built.png");
        begin(P::BuildMode::fill);
        aim(960, 640);
        press(H::Action::confirm);
        aim(1056, 736);
        capture("live-fill-preview.png");
        const auto fillCost = P::Action(false).cost;
        const auto fillCash = state.park.cash;
        press(H::Action::confirm);
        press(H::Action::confirm);
        gameStateUpdateLogic();
        for (int y = 640; y <= 736; y += 32)
            for (int x = 960; x <= 1056; x += 32)
                Check(path(x, y), "Live Fill real path missing");
        Check(state.park.cash == fillCash - fillCost, "Live Fill charge mismatch");
        capture("live-fill-built.png");
        P::End();
        const auto savedCash = state.park.cash;
        SaveFlags flags{};
        flags.set(SaveFlag::exportObjects);
        const auto save = (std::filesystem::path(argv[2]) / "path-cursor.park").string();
        Check(ScenarioSave(state, save, flags) != 0 && context->LoadParkFromFile(save), "Cursor flow save/reload failed");
        for (int x = 512; x <= 672; x += 32)
            Check(path(x, 512), "Reload lost cursor Line");
        for (int y = 640; y <= 736; y += 32)
            for (int x = 960; x <= 1056; x += 32)
                Check(path(x, y), "Reload lost cursor Fill");
        Check(getGameState().park.cash == savedCash, "Cursor reload changed cash");
        std::cout << "PASS PathCursorTests\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
}

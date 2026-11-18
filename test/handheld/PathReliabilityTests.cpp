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
#include <openrct2-ui/handheld/SceneryTools.h>
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
            if (std::getenv("POCKET_OVERLAY_AUDIT"))
                if (const auto gap = H::OverlayAudit(); !gap.empty())
                    std::cerr << "AUDIT " << name << ": " << gap << "\n";
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

        auto target = [&](int x, int y) {
            InteractionInfo hit{};
            hit.Loc = { x, y };
            hit.interactionType = ViewportInteractionItem::terrain;
            hit.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(CoordsXY{ x, y }));
            P::UpdateTargetFromInfo(hit);
            P::UpdatePreview();
        };
        auto ground = [&](CoordsXY tile, int z, int slope = 0) {
            auto* s = MapGetSurfaceElementAt(tile);
            s->setBaseZ(z);
            s->setClearanceZ(z);
            s->setSlope(slope);
        };
        auto build = [&] {
            const auto q = P::Action(false);
            Check(q.error == GameActions::Status::ok, "Ground selection query failed");
            const auto cash = state.park.cash;
            press(H::Action::confirm);
            press(H::Action::confirm);
            gameStateUpdateLogic();
            Check(state.park.cash == cash - q.cost, "Actual path charges differ from quote");
        };
        const auto base = MapGetSurfaceElementAt(CoordsXY{ 512, 512 })->getBaseZ();
        // Repeated Line without restarting: a finished ground selection must not pin the next one to its height.
        begin(P::BuildMode::line);
        target(512, 512);
        press(H::Action::confirm);
        target(576, 512);
        build();
        for (int x = 512; x <= 576; x += 32)
            Check(path(x, 512) && path(x, 512)->getBaseZ() == base, "Flat ground path missing");
        for (int x = 512; x <= 576; x += 32)
            ground({ x, 640 }, base + 64);
        target(512, 640);
        press(H::Action::confirm);
        target(576, 640);
        build();
        for (int x = 512; x <= 576; x += 32)
            Check(path(x, 640) && path(x, 640)->getBaseZ() == base + 64, "Repeated ground Line built below the terrain");
        Check(P::PlacementName() == "Follows ground", "Completed Line retained a fixed elevation");
        // Cancel an anchor, then target different terrain in the same tool session.
        target(512, 512);
        press(H::Action::confirm);
        press(H::Action::back);
        for (int x = 704; x <= 768; x += 32)
            ground({ x, 640 }, base + 96);
        target(704, 640);
        press(H::Action::confirm);
        target(768, 640);
        build();
        Check(path(768, 640) && path(768, 640)->getBaseZ() == base + 96, "Cancelled anchor retained an old height");
        // Fill follows each tile's actual terrain; steep ledges keep native disconnected-edge rules.
        begin(P::BuildMode::fill);
        for (int y = 896; y <= 960; y += 32)
            for (int x = 896; x <= 960; x += 32)
                ground({ x, y }, base + (x == 928 ? 64 : 0));
        target(896, 896);
        press(H::Action::confirm);
        target(960, 960);
        build();
        for (int y = 896; y <= 960; y += 32)
            for (int x = 896; x <= 960; x += 32)
                Check(path(x, y) && path(x, y)->getBaseZ() == base + (x == 928 ? 64 : 0), "Fill did not follow tile terrain");
        // Supported terrain slopes use the native geometry, rather than flattening every purchased tile.
        begin(P::BuildMode::line);
        ground({ 1152, 896 }, base, 3);
        target(1152, 896);
        press(H::Action::confirm);
        target(1152, 896);
        build();
        Check(path(1152, 896) && path(1152, 896)->isSloped(), "Ground Line lost the native terrain slope");
        // Explicit level placement remains available and persists after an intentional height choice.
        begin(P::BuildMode::line);
        target(1280, 896);
        Check(P::SetPlacementHeight(base + 64), "No advanced elevation");
        press(H::Action::confirm);
        target(1344, 896);
        build();
        Check(path(1280, 896) && path(1280, 896)->getBaseZ() == base + 64, "Intentional elevated path was grounded");
        P::FollowGround();
        Check(P::PlacementName() == "Follows ground", "Follow ground did not restore automatic mode");
        // An elevated path selected as the anchor continues at that path's level.
        P::End();
        P::Begin(false);
        InteractionInfo selected{};
        selected.Loc = { 1280, 896 };
        selected.Element = reinterpret_cast<TileElement*>(path(1280, 896));
        selected.interactionType = ViewportInteractionItem::footpath;
        P::UpdateTargetFromInfo(selected);
        P::CycleMode(1);
        press(H::Action::confirm);
        target(1376, 896);
        build();
        Check(path(1376, 896) && path(1376, 896)->getBaseZ() == base + 64, "Selected elevated anchor lost its level");
        auto* viewport = WindowGetMain()->viewport;
        viewport->viewPos = Translate3DTo2DWithZ(viewport->rotation, { 1056, 928, base + 32 }) - ScreenCoordsXY{ 352, 192 };
        WindowGetMain()->savedViewPos = viewport->viewPos;
        capture("08-ground-and-elevated-paths.png");
        // Native lamp edge rules are retained, while a chosen stacked path resolves to its actual level.
        P::End();
        Check(objects.LoadObject("rct2.footpath_item.lamp1"), "Lamp object missing");
        namespace S = H::SceneryTools;
        S::SelectCategory(S::Category::additions);
        auto entries = S::Entries(S::Category::additions);
        Check(!entries.empty(), "Lamp not available");
        S::Settings lamp;
        lamp.category = S::Category::additions;
        lamp.object = entries.front().identity.index;
        auto* lower = path(896, 928);
        Check(lower, "Lower path missing");
        lower->setEdges(15);
        auto* upper = TileElementInsert<PathElement>(CoordsXYZ{ 896, 928, base + 64 }, 15);
        Check(upper, "Stacked path insertion failed");
        upper->setClearanceZ(base + 80);
        upper->setEdges(0);
        upper->setSurfaceEntryIndex(gFootpathSelection.normalSurface);
        upper->setRailingsEntryIndex(gFootpathSelection.railings);
        lamp.pathHeight = base;
        Check(S::Place(CoordsXY{ 896, 928 }, lamp, false).error != GameActions::Status::ok, "Interior path edge rule bypassed");
        lamp.pathHeight = base + 64;
        auto q = S::Place(CoordsXY{ 896, 928 }, lamp, false);
        Check(q.error == GameActions::Status::ok, "Selected elevated path could not take a lamp");
        const auto lampCash = state.park.cash;
        S::Place(CoordsXY{ 896, 928 }, lamp, true);
        GameActions::ProcessQueue(state);
        Check(
            upper->hasAddition() && !upper->additionIsGhost() && !lower->hasAddition(),
            "Lamp attached to the wrong path level");
        Check(state.park.cash == lampCash - q.cost, "Lamp charge incorrect");
        // A real connected line and its addition survive serialization; ghosts never become purchases.
        const auto cash = state.park.cash;
        const auto file = (std::filesystem::path(argv[2]) / "path-reliability.park").string();
        Check(ScenarioSave(state, file, {}) != 0 && context->LoadParkFromFile(file), "Reliability save/reload failed");
        Check(
            getGameState().park.cash == cash && path(512, 640)->getBaseZ() == base + 64, "Reload lost path elevation or cash");
        bool savedLamp = false;
        for (const auto* p : TileElementsView<PathElement>(CoordsXY{ 896, 928 }))
            if (!p->isGhost() && p->getBaseZ() == base + 64 && p->hasAddition() && !p->additionIsGhost())
                savedLamp = true;
        Check(savedLamp, "Reload lost the real elevated lamp");
        std::cout
            << "PASS PathReliabilityTests: repeated/cancelled ground anchors, varied terrain, slopes, deliberate elevation, "
               "selected elevated paths, stacked lamp attachment, native edge rejection, exact cash, persistence\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
}

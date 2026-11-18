// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
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
        auto target = [&](int x, int y) {
            InteractionInfo hit{};
            hit.Loc = { x, y };
            hit.interactionType = ViewportInteractionItem::terrain;
            hit.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(CoordsXY{ x, y }));
            P::UpdateTargetFromInfo(hit);
            P::UpdatePreview();
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
        auto focus = [&](CoordsXYZ point) {
            if (const auto tile = P::TargetTile())
                point = CoordsXYZ{ *tile + CoordsXY{ 16, 16 }, MapGetSurfaceElementAt(*tile)->getBaseZ() };
            auto* v = WindowGetMain()->viewport;
            v->zoom = ZoomLevel{ 0 };
            v->viewPos = Translate3DTo2DWithZ(v->rotation, point) - ScreenCoordsXY{ 256, 192 };
            WindowGetMain()->savedViewPos = v->viewPos;
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
        auto flush = [&]() {
            if (std::getenv("POCKET_PATH_LIVE_TICKS"))
                gameStateUpdateLogic();
            else
                GameActions::ProcessQueue(state);
        };
        // Run with POCKET_PATH_LIVE_TICKS=1 to include the real simulation lifecycle.
        // Single placement, query, ghost suspension and accounting remain native.
        begin(P::BuildMode::single);
        target(512, 512);
        const auto single = P::Action(false);
        const auto initialCash = state.park.cash;
        Check(single.error == GameActions::Status::ok, "Single query failed");
        press(H::Action::confirm);
        flush();
        Check(path(512, 512) && state.park.cash == initialCash - single.cost, "Single construction/accounting regressed");
        P::ClearPreview();
        // Horizontal line includes both ends and joins an existing path through native connection logic.
        begin(P::BuildMode::line);
        target(512, 512);
        press(H::Action::confirm);
        target(672, 512);
        const auto lineQuote = P::Action(false);
        Check(lineQuote.error == GameActions::Status::ok, "Line quote failed");
        focus({ 656, 528, 16 });
        capture("01-line-preview.png");
        const auto lineCash = state.park.cash;
        press(H::Action::confirm);
        Check(P::SelectionOwnsInput() && !path(544, 512), "Review purchased a tile");
        capture("02-line-review.png");
        press(H::Action::back);
        Check(!P::SelectionOwnsInput() && P::IsActive(), "Review Back left construction");
        press(H::Action::confirm);
        press(H::Action::confirm);
        Check(!path(544, 512), "Batch bypassed action queue");
        flush();
        Check(state.park.cash == lineCash - lineQuote.cost, "Line accounting differs from native quote");
        for (int x = 512; x <= 672; x += 32)
            Check(path(x, 512), "Line tile missing");
        Check(path(544, 512)->getEdges() != 0, "Native line connectivity missing");
        capture("03-line-built.png");
        begin(P::BuildMode::line);
        target(512, 512);
        press(H::Action::confirm);
        target(672, 512);
        focus({});
        capture("12-existing-path-preview.png");
        press(H::Action::back);
        // Repeated operation returns to start-selection, with no stale endpoint or automatic purchase.
        target(672, 544);
        press(H::Action::confirm);
        target(672, 640);
        const auto vertical = P::Action(false);
        Check(vertical.error == GameActions::Status::ok, "Vertical line quote failed");
        press(H::Action::confirm);
        press(H::Action::confirm);
        flush();
        for (int y = 544; y <= 640; y += 32)
            Check(path(672, y), "Vertical line missing");
        for (const auto& ends : std::array<std::pair<CoordsXY, CoordsXY>, 4>{ { { { 1280, 512 }, { 1376, 512 } },
                                                                                { { 1376, 576 }, { 1280, 576 } },
                                                                                { { 1440, 512 }, { 1440, 608 } },
                                                                                { { 1504, 608 }, { 1504, 512 } } } })
        {
            begin(P::BuildMode::line);
            target(ends.first.x, ends.first.y);
            press(H::Action::confirm);
            target(ends.second.x, ends.second.y);
            Check(P::Action(false).error == GameActions::Status::ok, "Cardinal line quote failed");
            press(H::Action::confirm);
            press(H::Action::confirm);
            flush();
            Check(path(ends.first.x, ends.first.y) && path(ends.second.x, ends.second.y), "Cardinal line endpoint missing");
        }
        // Diagonal is rejected rather than routed. Cancel never changes map/cash.
        target(800, 512);
        press(H::Action::confirm);
        target(864, 576);
        const auto cancelCash = state.park.cash;
        Check(P::Action(false).error != GameActions::Status::ok, "Diagonal silently routed");
        capture("04-line-alignment.png");
        press(H::Action::confirm);
        Check(!P::SelectionOwnsInput(), "Invalid diagonal opened review");
        press(H::Action::back);
        Check(P::IsActive() && state.park.cash == cancelCash && !path(800, 512), "Selection cancel changed park or exited");
        // Rectangle, all corners included; native flat paths at the start height.
        begin(P::BuildMode::fill);
        target(960, 640);
        press(H::Action::confirm);
        target(1056, 736);
        const auto fill = P::Action(false);
        Check(fill.error == GameActions::Status::ok, "Fill query failed");
        focus({ 1008, 704, 16 });
        capture("05-fill-preview.png");
        const auto fillCash = state.park.cash;
        press(H::Action::confirm);
        press(H::Action::confirm);
        flush();
        Check(state.park.cash == fillCash - fill.cost, "Fill accounting wrong");
        for (int y = 640; y <= 736; y += 32)
            for (int x = 960; x <= 1056; x += 32)
                Check(path(x, y) && !path(x, y)->isSloped(), "Rectangle tile/height wrong");
        capture("06-fill-built.png");
        target(1088, 640);
        press(H::Action::confirm);
        target(1120, 672);
        press(H::Action::confirm);
        press(H::Action::confirm);
        flush();
        Check(path(1088, 640) && path(1120, 672), "Repeated fill lost state");
        // A fresh quote that changes while review is open cannot purchase at an unreviewed price.
        begin(P::BuildMode::line);
        target(1184, 640);
        press(H::Action::confirm);
        target(1216, 640);
        press(H::Action::confirm);
        const auto reviewCash = state.park.cash;
        GameActions::FootpathPlaceAction external(
            { 1184, 640, MapGetSurfaceElementAt(CoordsXY{ 1184, 640 })->getBaseZ() }, { FootpathSlopeType::flat, 0 },
            gFootpathSelection.normalSurface, gFootpathSelection.railings, kInvalidDirection, 0);
        Check(GameActions::Execute(&external, state).error == GameActions::Status::ok, "External native placement failed");
        flush();
        press(H::Action::confirm);
        flush();
        Check(!path(1216, 640) && P::SelectionOwnsInput(), "Changed review quote purchased without renewed consent");
        Check(state.park.cash < reviewCash, "External action accounting missing");
        press(H::Action::back);
        press(H::Action::back);
        // Queue surface/type is preserved; the native engine decides queue connections, not this selection tool.
        begin(P::BuildMode::line, true);
        target(800, 896);
        press(H::Action::confirm);
        target(896, 896);
        Check(P::Action(false).error == GameActions::Status::ok, "Queue quote failed");
        focus({ 864, 912, 16 });
        capture("07-queue-preview.png");
        const auto queueSurface = gFootpathSelection.queueSurface;
        press(H::Action::confirm);
        press(H::Action::confirm);
        flush();
        for (int x = 800; x <= 896; x += 32)
            Check(
                path(x, 896) && path(x, 896)->isQueue() && path(x, 896)->getSurfaceEntryIndex() == queueSurface,
                "Queue selection changed");
        begin(P::BuildMode::fill, true);
        target(960, 896);
        press(H::Action::confirm);
        target(992, 928);
        Check(P::Action(false).error == GameActions::Status::ok, "Queue fill unavailable");
        press(H::Action::confirm);
        press(H::Action::confirm);
        flush();
        Check(path(992, 928)->isQueue(), "Queue fill lost queue type");
        // Invalid ownership blocks the whole selection before review, even with valid tiles either side.
        begin(P::BuildMode::line);
        target(640, 1088);
        press(H::Action::confirm);
        target(768, 1088);
        MapGetSurfaceElementAt(CoordsXY{ 704, 1088 })->setOwnership({});
        state.currentTicks += 8;
        P::UpdatePreview();
        const auto blockedCash = state.park.cash;
        Check(P::Action(false).error != GameActions::Status::ok, "Blocked selection accepted");
        focus({ 736, 1104, 16 });
        capture("08-blocked-preview.png");
        press(H::Action::confirm);
        flush();
        Check(
            !path(640, 1088) && !path(768, 1088) && state.park.cash == blockedCash, "Known invalid selection partially built");
        MapGetSurfaceElementAt(CoordsXY{ 704, 1088 })->setOwnership({ OwnershipFlag::landOwned });
        // Full-selection affordability, not affordability of just the first tile.
        begin(P::BuildMode::fill);
        target(960, 1088);
        press(H::Action::confirm);
        target(1056, 1184);
        const auto fullQuote = P::Action(false);
        state.park.cash = fullQuote.cost / 2;
        state.currentTicks += 8;
        Check(P::Action(false).error == GameActions::Status::insufficientFunds, "Whole-selection affordability missing");
        press(H::Action::confirm);
        flush();
        Check(!path(960, 1088), "Unaffordable fill partially built");
        state.park.cash = blockedCash;
        // An intentional level span still uses native clearance on incompatible terrain.
        begin(P::BuildMode::line);
        target(640, 1248);
        press(H::Action::confirm);
        target(768, 1248);
        auto* ridge = MapGetSurfaceElementAt(CoordsXY{ 704, 1248 });
        const auto oldBase = ridge->getBaseZ();
        Check(P::SetPlacementHeight(oldBase), "Intentional level span unavailable");
        ridge->setBaseZ(oldBase + 8);
        ridge->setClearanceZ(oldBase + 8);
        state.currentTicks += 8;
        Check(P::Action(false).error != GameActions::Status::ok, "Native uneven-terrain rejection bypassed");
        press(H::Action::confirm);
        Check(!path(640, 1248), "Uneven selection partially built");
        ridge->setBaseZ(oldBase);
        ridge->setClearanceZ(oldBase);
        // An obstruction is also native validated, independently of ownership/terrain.
        auto* obstruction = TileElementInsert<TrackElement>(CoordsXYZ{ 704, 1248, oldBase }, 15);
        Check(obstruction, "Obstruction fixture failed");
        obstruction->setClearanceZ(oldBase + 16);
        state.currentTicks += 8;
        Check(P::Action(false).error == GameActions::Status::noClearance, "Occupied selection not rejected");
        press(H::Action::back);
        TileElementRemove(reinterpret_cast<TileElement*>(obstruction));
        // Native runtime change AFTER explicit sequential confirmation: stop and report exact retained count.
        begin(P::BuildMode::line);
        target(640, 1408);
        press(H::Action::confirm);
        const auto failureTileCost = P::Action(false).cost;
        target(736, 1408);
        press(H::Action::confirm);
        const auto failureCash = state.park.cash;
        press(H::Action::confirm);
        MapGetSurfaceElementAt(CoordsXY{ 672, 1408 })->setOwnership({});
        flush();
        Check(path(640, 1408) && !path(672, 1408) && !path(704, 1408), "Sequential failure did not stop");
        Check(
            P::BatchMessage().find("Stopped: 1/4") != std::string::npos && state.park.cash == failureCash - failureTileCost,
            "Partial result not explicitly reported/accounted");
        focus({ 704, 1424, 16 });
        capture("09-stopped-result.png");
        if (wm->FindByClass(WindowClass::error))
            press(H::Action::back);
        Check(P::IsActive(), "Sequential failure exited construction");
        // Mode/Adjust controls do not purchase or pan; surface/options Back retains mode and anchor.
        begin(P::BuildMode::single);
        target(1280, 960);
        const auto navCash = state.park.cash;
        P::ChangeHeight(1);
        P::CycleSlope();
        const auto singleGeometry = P::PlacementName();
        press(H::Action::contextSecondary);
        Check(P::IsAdjusting(), "Y did not enter Adjust");
        press(H::Action::nextOption);
        Check(P::Mode() == P::BuildMode::line, "R1 did not choose Line");
        capture("10-mode-adjust.png");
        press(H::Action::nextOption);
        Check(P::Mode() == P::BuildMode::fill, "R1 did not choose Fill");
        press(H::Action::nextOption);
        Check(
            P::Mode() == P::BuildMode::single && P::PlacementName() == singleGeometry, "Single geometry was lost across modes");
        press(H::Action::previousOption);
        Check(P::Mode() == P::BuildMode::fill, "L1 did not choose Fill");
        press(H::Action::back);
        Check(P::IsActive() && !P::IsAdjusting(), "Adjust B exited tool");
        target(1280, 960);
        press(H::Action::confirm);
        target(1312, 992);
        press(H::Action::openBuild);
        Check(W::HandheldObjectPickerIsOpen(), "Surface picker unavailable");
        capture("11-surface-selection.png");
        press(H::Action::back);
        Check(P::Mode() == P::BuildMode::fill && P::IsActive(), "Surface Back lost batch mode");
        press(H::Action::contextSecondary);
        press(H::Action::contextPrimary);
        Check(wm->FindByClass(WindowClass::handheldMenu), "More unavailable");
        press(H::Action::back);
        Check(P::IsAdjusting() && P::Mode() == P::BuildMode::fill, "More Back lost Adjust/mode");
        press(H::Action::back);
        press(H::Action::back);
        Check(P::IsActive(), "Anchor B exited tool");
        press(H::Action::back);
        Check(!P::IsActive(), "Unanchored B failed to exit");
        Check(state.park.cash == navCash, "Navigation purchased paths");
        // Explicit height remains one consistent plane; water obstruction is native queried.
        begin(P::BuildMode::fill);
        for (int y = 1600; y <= 1696; y += 32)
            for (int x = 960; x <= 1056; x += 32)
                MapGetSurfaceElementAt(CoordsXY{ x, y })->setWaterHeight(oldBase + 16);
        Check(objects.LoadObject("rct2.water.wtrcyan"), "Fixture water object missing");
        target(960, 1600);
        press(H::Action::confirm);
        target(1056, 1696);
        state.currentTicks += 8;
        Check(P::Action(false).error != GameActions::Status::ok, "Underwater fill accepted");
        focus({});
        capture("13-water-blocked.png");
        Check(P::SetPlacementHeight(oldBase + 16), "Batch height adjustment failed");
        Check(P::Action(false).error == GameActions::Status::ok, "Native raised water-plane fill rejected");
        capture("14-elevated-fill-preview.png");
        press(H::Action::back);
        // Bounded selection and stable cache: no repeated validation when tick/target/material/cash are unchanged.
        P::BatchDraft draft;
        draft.mode = P::BuildMode::fill;
        draft.start = CoordsXYZ{ 1280, 1280, oldBase };
        const P::BatchMaterial settings{ gFootpathSelection.normalSurface, gFootpathSelection.railings, 0 };
        draft.update(CoordsXY{ 1504, 1504 }, settings);
        Check(draft.selection.tiles.size() == 64, "64-tile bound wrong");
        const auto validations = draft.validationCount;
        auto timer = std::chrono::steady_clock::now();
        for (int i = 0; i < 1000; ++i)
            draft.update(CoordsXY{ 1504, 1504 }, settings);
        Check(draft.validationCount == validations, "Unchanged previews repeatedly queried");
        std::cout << "1000 cached preview calls us="
                  << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - timer).count()
                  << "\n";
        draft.update(CoordsXY{ 1536, 1504 }, settings);
        Check(draft.quote.error != GameActions::Status::ok && draft.selection.tiles.empty(), "Oversize fill not bounded");
        Check(
            P::SelectTiles(P::BuildMode::line, { 32, 32, oldBase }, { 2048, 32 }).tiles.size() == 64,
            "64-tile line bound wrong");
        Check(P::SelectTiles(P::BuildMode::line, { 32, 32, oldBase }, { 2080, 32 }).tiles.empty(), "Oversize line accepted");
        begin(P::BuildMode::fill);
        target(1280, 1280);
        press(H::Action::confirm);
        target(1536, 1504);
        focus({});
        capture("15-selection-limit.png");
        target(1504, 1504);
        const auto largeQuote = P::Action(false);
        Check(largeQuote.error == GameActions::Status::ok, "64-tile quote failed");
        const auto largeCash = state.park.cash;
        press(H::Action::confirm);
        press(H::Action::confirm);
        timer = std::chrono::steady_clock::now();
        flush();
        std::cout << "64 tile native queued build us="
                  << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - timer).count()
                  << "\n";
        Check(state.park.cash == largeCash - largeQuote.cost && path(1504, 1504), "64-tile execution/accounting failed");
        P::End();
        const auto savedCash = state.park.cash;
        SaveFlags flags{};
        flags.set(SaveFlag::exportObjects);
        const auto save = (std::filesystem::path(argv[2]) / "path-batch.park").string();
        Check(ScenarioSave(state, save, flags) != 0 && context->LoadParkFromFile(save), "Batch save/reload failed");
        Check(
            path(672, 640) && path(1056, 736) && path(1504, 1504) && path(896, 896)->isQueue(), "Batch save/reload lost paths");
        Check(getGameState().park.cash == savedCash, "Save/reload changed accounting");
        std::cout << "PASS PathBatchTests: Single, lines, fill, queues, native validation/cost/connectivity, review/cancel, "
                     "failure reporting, controls, cache, bounds, save/reload\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
}

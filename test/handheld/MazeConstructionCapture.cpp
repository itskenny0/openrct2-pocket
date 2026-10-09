// SPDX-License-Identifier: GPL-3.0-only
// Native private scenario, production picker/tool/shell rendering; no fixture objects or research.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Designs.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2-ui/handheld/Panel.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRegistry.h>
#include <openrct2/actions/ResultWithMessage.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/footpath/FootpathRemoveAction.h>
#include <openrct2/actions/park/ParkSetLoanAction.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
#include <openrct2/actions/ride/MazeSetTrackAction.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/ride/RideEntranceExitRemoveAction.h>
#include <openrct2/actions/track/SequentialTrackPlaceAction.h>
#include <openrct2/actions/track/TrackPlaceAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/entity/EntityList.h>
#include <openrct2/entity/Guest.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/management/Research.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/rct2/T6Exporter.h>
#include <openrct2/ride/FlatRidePlacement.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/TrackData.h>
#include <openrct2/ride/TrackDesignRepository.h>
#include <openrct2/ride/TrackIteration.h>
#include <openrct2/ride/Vehicle.h>
#include <openrct2/ride/ted/TrackElementDescriptor.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <queue>
#include <set>
#include <stdexcept>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace F = H::FlatRides;
static void Check(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "FAIL: " << message << std::endl;
        throw std::runtime_error(message);
    }
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 6)
            return 2;
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[5];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = false;
        auto env = CreatePlatformEnvironment();
        env->SetBasePath(DirBase::rct2, argv[2]);
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        Config::Get().general.rct2Path = argv[2];
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Config::Get().general.windowScale = 1;
        Check(context->Initialise() && context->LoadParkFromFile(argv[3], false, true), "Native scenario failed");
        context->GetUiContext().CreateWindow();
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        Ui::GetWindowManager()->CloseByClass(WindowClass::error);
        Ui::GetWindowManager()->CloseByClass(WindowClass::parkInformation);
        gGamePaused = 0;
        auto& state = getGameState();
        std::filesystem::create_directories(argv[4]);
        namespace W = Ui::Windows;
        namespace C = H::CustomTracks;
        namespace D = H::Designs;
        namespace E = H::TrackEditing;
        GameActions::ParkSetLoanAction loan(state.park.maxBankLoan);
        Check(GameActions::Execute(&loan, state).error == GameActions::Status::ok, "Native loan unavailable");
        GameActions::ProcessQueue(state);
        namespace M = H::Mazes;
        const auto entries = M::Entries();
        Check(!entries.empty(), "Scenario has no researched maze");
        H::ControllerInput controller;
        H::Frame a;
        a.actions = { H::Action::confirm, H::Action::confirm };
        CoordsXYZ focus{ 800, 800, MapGetHighestZ({ 800, 800 }) };
        auto capture = [&](const char* name, bool modal = false) {
            if (M::Active() && M::Target() && GetRide(M::Target()->id)->mazeTiles && M::Cursor())
                M::Update(M::Cursor()->toTileStart());
            Viewport vp{};
            vp.width = 512;
            vp.height = 384;
            vp.zoom = ZoomLevel{ 0 };
            vp.viewPos = Translate3DTo2DWithZ(0, focus + CoordsXYZ{ 16, 16, 0 }) - ScreenCoordsXY{ 320, 190 };
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &vp);
            if (modal)
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
            else if (M::Active())
                M::DrawPanel(rt, { 512, 384 });
            else if (D::Active())
                D::DrawPanel(rt, { 512, 384 });
            else if (F::CurrentStage() != F::Stage::inactive)
                F::DrawPanel(rt, { 512, 384 });
            else
                H::DrawGameplayFooter(rt, { 512, 384 }, "", false, false);
            Image im;
            im.Width = 512;
            im.Height = 384;
            im.Depth = 8;
            im.Stride = rt.LineStride();
            im.Palette = Drawing::gPalette;
            const auto* bits = reinterpret_cast<const uint8_t*>(rt.bits);
            im.Pixels.assign(bits, bits + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(argv[4]) / name).string(), im, ImageFormat::png);
            engine.EndDraw();
        };
        W::HandheldBuildOpen();
        W::HandheldMenuMove(8);
        H::HandleMenuInput(controller, a);
        const auto row = F::Entries().size();
        for (size_t n = 0; n < row / 3; n++)
            W::HandheldObjectPickerMove(0, 1);
        for (size_t n = 0; n < row % 3; n++)
            W::HandheldObjectPickerMove(1, 0);
        capture("01-maze-object-picker.png", true);
        H::HandleMenuInput(controller, a);
        GameActions::ProcessQueue(state);
        Check(M::Active() && M::Target(), "Controller maze selection failed");
        std::optional<CoordsXY> site;
        for (int y = 96; y < state.mapSize.y * 32 - 128 && !site; y += 32)
            for (int x = 96; x < state.mapSize.x * 32 - 128 && !site; x += 32)
            {
                const CoordsXY tile{ x, y };
                bool owned = true;
                const int z = (MapGetHighestZ(tile) + 15) / 16 * 16;
                for (int dy = -32; dy <= 64; dy += 32)
                    for (int dx = -32; dx <= 64; dx += 32)
                        owned &= MapIsLocationOwned({ x + dx, y + dy, z }) && MapGetHighestZ({ x + dx, y + dy }) <= z;
                if (!owned)
                    continue;
                M::Update(tile);
                if (M::Build(false).error == GameActions::Status::ok)
                    site = tile;
            }
        Check(site.has_value(), "Native maze site unavailable");
        M::Update(*site);
        focus = *M::Cursor();
        capture("02-maze-initial-placement.png");
        M::Update(CoordsXY{ 0, 0 });
        Check(M::Build(false).error != GameActions::Status::ok, "Invalid native tile accepted");
        capture("03-maze-invalid-placement.png");
        M::Update(*site);
        const auto q = M::Build(false);
        const auto cash = state.park.cash;
        H::HandleExplorerInput(controller, a);
        GameActions::ProcessQueue(state);
        Check(
            state.park.cash == cash - q.cost && GetRide(M::Target()->id)->mazeTiles == 1,
            "Held/repeated initial action changed accounting");
        const auto target = *M::Target();
        M::Turn(2);
        for (int n = 0; n < 3; n++)
        {
            H::HandleExplorerInput(controller, a);
            GameActions::ProcessQueue(state);
        }
        M::Turn(-1);
        for (int n = 0; n < 3; n++)
        {
            H::HandleExplorerInput(controller, a);
            GameActions::ProcessQueue(state);
        }
        capture("04-maze-segment-wall-editing.png");
        M::Turn(2);
        Check(M::Move().error == GameActions::Status::ok, "Move failed");
        GameActions::ProcessQueue(state);
        const auto device = SDL_JoystickAttachVirtual(
            SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        Check(device >= 0, "Maze virtual controller unavailable");
        auto* joystick = SDL_JoystickOpen(device);
        auto pump = [&] {
            SDL_Event event;
            while (SDL_PollEvent(&event))
                controller.handleEvent(event);
        };
        pump();
        controller.update(true, 1000);
        pump();
        controller.update(true, 1010);
        M::ToggleCamera();
        Check(M::ReviewRemoval().error == GameActions::Status::ok && !M::CameraMode(), "Fill review camera handoff failed");
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 1);
        SDL_JoystickUpdate();
        pump();
        const auto held = controller.update(true, 1020);
        H::HandleExplorerInput(controller, held);
        Check(!M::Reviewing() && controller.update(true, 1200).actions.empty(), "Held A repeated removal cancellation");
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 0);
        SDL_JoystickUpdate();
        pump();
        controller.update(true, 1210);
        SDL_JoystickClose(joystick);
        SDL_JoystickDetachVirtual(device);
        Check(M::ReviewRemoval().error == GameActions::Status::ok, "Review after held A failed");
        capture("05-maze-cancel-first-removal.png");
        H::Frame option;
        option.actions = { H::Action::nextOption };
        H::HandleExplorerInput(controller, option);
        capture("06-maze-removal-review.png");
        Check(M::Confirm().error == GameActions::Status::ok, "Fill failed");
        GameActions::ProcessQueue(state);
        if (!M::CameraMode())
            M::ToggleCamera();
        capture("07-maze-dense-camera-controls.png");
        M::ToggleCamera();
        M::End();
        Check(M::Resume(target), "Existing maze recovery failed");
        capture("08-existing-maze-recovery.png");
        M::End();
        Check(F::Resume(target), "Maze access resume failed");
        for (int n = 0; n < 2; n++)
        {
            std::optional<CoordsXY> access;
            for (int y = site->y - 32; y <= site->y + 96 && !access; y += 32)
                for (int x = site->x - 32; x <= site->x + 96 && !access; x += 32)
                    if (F::Action(CoordsXY{ x, y }, false).error == GameActions::Status::ok)
                        access = CoordsXY{ x, y };
            Check(access.has_value(), "Maze entrance/exit site unavailable");
            F::UpdatePreview(access);
            capture(n == 0 ? "09-maze-entrance.png" : "10-maze-exit.png");
            Check(F::Action(access, true).error == GameActions::Status::ok, "Maze access execute failed");
            GameActions::ProcessQueue(state);
        }
        F::End();
        Check(
            GetRide(target.id)->getStation().entrance.isNull() == false
                && GetRide(target.id)->getStation().exit.isNull() == false,
            "Access incomplete");
        TrackDesign td;
        TrackDesignState tds{};
        Check(td.CreateTrackDesign(tds, *GetRide(target.id)).Successful, "Native maze design creation failed");
        const auto tracks = std::filesystem::path(
            context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::trackDesigns));
        std::filesystem::create_directories(tracks);
        const auto file = (tracks / "Pocket native maze.td6").string();
        Check(RCT2::T6Exporter(td).SaveTrack(file.c_str()), "Maze design export failed");
        TrackRepositoryScan();
        Check(D::Begin(file), "Maze design rejected");
        std::optional<CoordsXY> destination;
        for (int y = 128; y < state.mapSize.y * 32 - 128 && !destination; y += 32)
            for (int x = 256; x < state.mapSize.x * 32 - 128 && !destination; x += 32)
                if (D::Query(CoordsXY{ x, y }).error == GameActions::Status::ok)
                    destination = CoordsXY{ x, y };
        Check(destination.has_value(), "Native maze design site unavailable");
        focus = { *destination, MapGetHighestZ(*destination) };
        D::UpdatePreview(*destination);
        capture("11-maze-design-ghost.png");
        D::Rotate();
        D::UpdatePreview(*destination);
        capture("12-maze-design-rotation.png");
        D::ClearPreview();
        if (D::Query(*destination).error != GameActions::Status::ok)
        {
            destination.reset();
            for (int y = 128; y < state.mapSize.y * 32 - 128 && !destination; y += 32)
                for (int x = 256; x < state.mapSize.x * 32 - 128 && !destination; x += 32)
                    if (D::Query(CoordsXY{ x, y }).error == GameActions::Status::ok)
                        destination = CoordsXY{ x, y };
        }
        Check(destination.has_value(), "Rotated maze site unavailable");
        Check(D::Review(*destination), "Maze design review failed");
        W::HandheldDesignReviewOpen();
        capture("13-maze-design-cost-review.png", true);
        H::Frame back;
        back.actions = { H::Action::back };
        H::HandleMenuInput(controller, back);
        Check(!D::HasReview(), "Maze design review cancel failed");
        Check(D::Review(*destination), "Second maze design review failed");
        const auto designCash = state.park.cash, designCost = D::ReviewCost();
        Check(
            D::Confirm().error == GameActions::Status::ok && D::Confirm().error != GameActions::Status::ok,
            "Maze design repeat guard failed");
        GameActions::ProcessQueue(state);
        Check(D::Completed() && state.park.cash == designCash - designCost, "Maze design execute cost failed");
        const auto designTarget = *D::Completed();
        D::End();
        Check(M::CanResume(designTarget), "Design maze recovery failed");
        Check(
            !GetRide(designTarget.id)->getStation().entrance.isNull() && !GetRide(designTarget.id)->getStation().exit.isNull(),
            "Native design access missing");
        const auto save = std::string(argv[4]) + "/profile/maze-validation.park";
        SaveFlags flags{};
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Maze reload failed");
        Check(GetRide(target.id) && GetRide(target.id)->mazeTiles > 0, "Maze persistence lost");
        std::cout << "Asset-backed maze controller/native cells/walls/cancel/fill/access/recovery/save passed: "
                  << state.scenarioOptions.name << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

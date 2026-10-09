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
        const auto entries = C::RideEntries();
        auto chosen = entries.end();
        ride_type_t type = kRideTypeNull;
        for (auto it = entries.begin(); it != entries.end(); ++it)
        {
            if (!it->identity.key.starts_with("custom-type:"))
                continue;
            const auto candidate = static_cast<ride_type_t>(std::stoi(it->identity.key.substr(12)));
            if (GetRideTypeDescriptor(candidate).Category != RideCategory::rollerCoaster)
                continue;
            if (chosen == entries.end() || candidate == RIDE_TYPE_LOOPING_ROLLER_COASTER)
            {
                chosen = it;
                type = candidate;
            }
            if (candidate == RIDE_TYPE_LOOPING_ROLLER_COASTER)
                break;
        }
        Check(chosen != entries.end(), "Scenario has no eligible non-Wooden coaster");
        std::cout << "Asset-backed " << state.scenarioOptions.name << " / " << chosen->name << " type " << type << std::endl;
        H::ControllerInput controller;
        H::Frame a;
        a.actions = { H::Action::confirm, H::Action::confirm };
        H::Frame b;
        b.actions = { H::Action::back, H::Action::back };
        CoordsXYZ focus{ 800, 800, MapGetHighestZ({ 800, 800 }) };
        auto capture = [&](const char* name, bool modal = false) {
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
            else if (C::Active())
                C::DrawPanel(rt, { 512, 384 });
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
        const auto row = F::Entries().size() + H::Mazes::Entries().size() + std::distance(entries.begin(), chosen);
        for (size_t n = 0; n < row / 3; ++n)
            W::HandheldObjectPickerMove(0, 1);
        for (size_t n = 0; n < row % 3; ++n)
            W::HandheldObjectPickerMove(1, 0);
        capture("01-ride-picker.png", true);
        H::HandleMenuInput(controller, a);
        capture("02-build-choice.png", true);
        H::HandleMenuInput(controller, a);
        GameActions::ProcessQueue(state);
        Check(C::Active() && C::Target() && GetRide(C::Target()->id)->type == type, "Controller selected wrong ride family");
        const auto id = C::Target()->id;
        for (int n = 0; n < 6; ++n)
            C::Slope(1);
        // Find an owned location by actual native queries, without changing park terrain or ownership.
        std::optional<CoordsXY> site;
        for (int y = 15; y < state.mapSize.y - 15 && !site; ++y)
            for (int x = 20; x < state.mapSize.x - 15 && !site; ++x)
            {
                CoordsXY tile{ x * 32, y * 32 };
                C::Update(tile);
                if (C::Action(false).error != GameActions::Status::ok)
                    continue;
                auto cursor = *C::Endpoint();
                bool valid = true;
                for (int n = 0; n < 4; ++n)
                    for (int side : { -32, 32 })
                        if (!MapIsLocationOwned({ tile.x - n * 32, tile.y + side, cursor.z }))
                            valid = false;
                if (!valid)
                    continue;
                for (int n = 0; n < 22; ++n)
                    for (int side = -3; side <= 3; ++side)
                        if (!MapIsLocationOwned({ tile.x - n * 32, tile.y + side * 32, cursor.z })
                            || MapGetHighestZ({ tile.x - n * 32, tile.y + side * 32 }) > cursor.z)
                            valid = false;
                if (!valid)
                    continue;
                for (int n = 0; n < 10; ++n)
                {
                    const auto piece = n < 4 ? TrackElemType::endStation : n == 6 ? TrackElemType::brakes : TrackElemType::flat;
                    GameActions::TrackPlaceAction q(id, piece, type, cursor, 6, 0, 0, {}, false);
                    if (GameActions::Query(&q, state).error != GameActions::Status::ok)
                    {
                        valid = false;
                        break;
                    }
                    cursor += CoordsDirectionDelta[cursor.direction];
                }
                if (valid)
                    site = tile;
            }
        Check(site.has_value(), "No native construction site");
        C::Update(*site);
        focus = *C::Endpoint();
        capture("03-new-coaster-station.png");
        for (int n = 0; n < 9; ++n)
        {
            const auto piece = n < 4 ? TrackElemType::endStation : n == 6 ? TrackElemType::brakes : TrackElemType::flat;
            Check(C::Select(piece), "Native compatible piece absent");
            C::Update(n == 0 ? site : std::nullopt);
            if (n == 6)
            {
                H::Frame increase;
                increase.actions = { H::Action::contextPrimary };
                H::HandleExplorerInput(controller, increase);
                Check(C::PlacementSpeed() == 8, "Controller placement parameter failed");
                C::Update({});
                focus = *C::Endpoint();
                capture("17-placement-speed-parameter.png");
            }
            Check(C::Action(true).error == GameActions::Status::ok, "Native construction failed");
            GameActions::ProcessQueue(state);
        }
        focus = *C::Endpoint();
        C::Update({});
        capture("04-new-coaster-editor.png");
        W::HandheldTrackPiecePickerOpen();
        capture("05-special-piece-picker.png", true);
        H::HandleMenuInput(controller, b);
        const auto pieces = C::Pieces();
        const auto camera = std::find_if(
            pieces.begin(), pieces.end(), [](const auto& p) { return p.identity.key == "camera"; });
        Check(camera != pieces.end(), "Brick camera controls absent from piece picker");
        W::HandheldTrackPiecePickerOpen();
        const auto cameraRow = std::distance(pieces.begin(), camera);
        for (int n = 0; n < cameraRow / 3; ++n)
            W::HandheldObjectPickerMove(0, 1);
        for (int n = 0; n < cameraRow % 3; ++n)
            W::HandheldObjectPickerMove(1, 0);
        H::HandleMenuInput(controller, a);
        Check(C::CameraMode(), "Select-based Brick camera handoff failed");
        C::ToggleCamera();
        if (C::Select(TrackElemType::flatToUp25) && C::Action(false).error == GameActions::Status::ok)
        {
            Check(C::Action(true).error == GameActions::Status::ok, "Native slope failed");
            GameActions::ProcessQueue(state);
            if (C::Select(TrackElemType::leftVerticalLoop))
            {
                C::Update({});
                focus.z += 64;
                capture("06-inversion-preview.png");
                focus.z -= 64;
            }
            Check(C::Undo().error == GameActions::Status::ok, "Approach undo failed");
            GameActions::ProcessQueue(state);
        }
        C::ClearPreview();
        auto target = *H::RideManagement::Identify(id);
        C::End();
        H::EndTerrain();
        TrackDesign td;
        TrackDesignState tds{};
        Check(td.CreateTrackDesign(tds, *GetRide(id)).Successful, "Native export failed");
        const auto tracks = std::filesystem::path(
            context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::trackDesigns));
        std::filesystem::create_directories(tracks);
        const auto file = (tracks / "Pocket native coaster.td6").string();
        Check(RCT2::T6Exporter(td).SaveTrack(file.c_str()), "Native design save failed");
        TrackRepositoryScan();
        Check(D::Begin(file), "New family design rejected");
        std::optional<CoordsXY> destination;
        for (int y = 15; y < state.mapSize.y - 15 && !destination; ++y)
            for (int x = 20; x < state.mapSize.x - 15 && !destination; ++x)
                if (D::Query(CoordsXY{ x * 32, y * 32 }).error == GameActions::Status::ok)
                    destination = CoordsXY{ x * 32, y * 32 };
        Check(destination.has_value(), "No native saved design site");
        D::UpdatePreview(*destination);
        focus = { *destination, MapGetHighestZ(*destination) };
        capture("07-saved-design-preview.png");
        Check(D::Review(*destination), "Native design review failed");
        W::HandheldDesignReviewOpen();
        capture("08-saved-design-review.png", true);
        H::HandleMenuInput(controller, b);
        D::End();
        H::EndTerrain();
        W::HandheldRideOpen(id);
        W::HandheldMenuMove(3 + (H::RideAccess::Supported(target) ? 1 : 0) + (C::CanResume(target) ? 1 : 0));
        H::HandleMenuInput(controller, a);
        capture("09-existing-track-blocks.png", true);
        Check(W::HandheldRidesPresentation().title == "SELECT TRACK BLOCK", "Block selector not reachable");
        auto blocks = E::Blocks(target);
        const auto brakeIt = std::find_if(
            blocks.begin(), blocks.end(), [](const auto& block) { return block.type == TrackElemType::brakes; });
        Check(brakeIt != blocks.end(), "Brake block absent");
        const auto brake = *brakeIt;
        W::HandheldMenuMove(std::distance(blocks.begin(), brakeIt));
        H::HandleMenuInput(controller, a);
        W::HandheldMenuMove(4);
        H::HandleMenuInput(controller, a);
        W::HandheldMenuMove(1);
        H::HandleMenuInput(controller, a);
        Check(
            W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 0,
            "Speed review not Cancel-first");
        capture("10-brake-speed-review.png", true);
        H::HandleMenuInput(controller, a);
        Check(E::Valid(brake), "Cancel changed native speed");
        W::HandheldMenuMove(5);
        H::HandleMenuInput(controller, a);
        W::HandheldMenuMove(1);
        H::HandleMenuInput(controller, a);
        GameActions::ProcessQueue(state);
        Check(!E::Valid(brake), "Native speed confirmation failed");
        W::HandheldRideOpen(id);
        W::HandheldMenuMove(3 + (H::RideAccess::Supported(target) ? 1 : 0) + (C::CanResume(target) ? 1 : 0));
        H::HandleMenuInput(controller, a);
        H::HandleMenuInput(controller, a);
        W::HandheldMenuMove(1);
        H::HandleMenuInput(controller, a);
        Check(
            W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 0,
            "Removal review not Cancel-first");
        capture("11-remove-block-review.png", true);
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
        const auto device = SDL_JoystickAttachVirtual(
            SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        Check(device >= 0, "Editing virtual controller unavailable");
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
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 1);
        SDL_JoystickUpdate();
        pump();
        auto held = controller.update(true, 1020);
        Check(held.actions.size() == 1 && H::HandleMenuInput(controller, held), "Editing Cancel A missing");
        Check(controller.update(true, 1200).actions.empty(), "Held editing A repeated after cancellation");
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 0);
        SDL_JoystickUpdate();
        pump();
        controller.update(true, 1210);
        SDL_JoystickClose(joystick);
        SDL_JoystickDetachVirtual(device);

        Check(E::Blocks(target).size() == blocks.size(), "Cancel removed block");
        H::HandleMenuInput(controller, b);
        W::HandheldMenuMove(std::distance(blocks.begin(), brakeIt));
        H::HandleMenuInput(controller, a);
        capture("12-dense-block-operations.png", true);
        W::HandheldMenuMove(6);
        capture("18-continue-section-selection.png", true);
        H::HandleMenuInput(controller, a);
        Check(C::Active() && C::Endpoint(), "Selected section controller handoff failed");
        focus = *C::Endpoint();
        C::Update({});
        capture("16-selected-section-continuation.png");
        H::EndTerrain();
        Check(!C::Active(), "Selected section cleanup failed");
        Check(F::Resume(target), "Native station completion unavailable");
        for (int phase = 0; phase < 2; ++phase)
        {
            std::optional<CoordsXY> tile;
            std::map<std::string, int> entranceErrors;
            for (int y = site->y / 32 - 3; y <= site->y / 32 + 3 && !tile; ++y)
                for (int x = site->x / 32 - 10; x <= site->x / 32 + 3 && !tile; ++x)
                {
                    const auto q = F::Action(CoordsXY{ x * 32, y * 32 }, false);
                    if (q.error == GameActions::Status::ok)
                        tile = CoordsXY{ x * 32, y * 32 };
                    else
                        ++entranceErrors[q.getErrorMessage()];
                }
            if (!tile)
            {
                std::cout << "Access phase " << phase << " stage " << int(F::CurrentStage()) << std::endl;
                for (const auto& [error, count] : entranceErrors)
                    std::cout << count << " " << error << std::endl;
            }
            Check(tile.has_value(), "Native entrance/exit site absent");
            F::UpdatePreview(*tile);
            focus = *F::EntranceLocation(*tile);
            capture(phase ? "14-exit-completion.png" : "13-entrance-completion.png");
            Check(F::Action(*tile, true).error == GameActions::Status::ok, "Native station access failed");
            GameActions::ProcessQueue(state);
        }
        F::End();
        H::EndTerrain();
        Check(C::Begin(chosen->identity.index, type), "Invalid placement context failed");
        GameActions::ProcessQueue(state);
        C::Update(CoordsXY{ 0, 0 });
        focus = { 0, 0, 16 };
        Check(C::Action(false).error != GameActions::Status::ok, "Invalid native bounds accepted");
        capture("15-invalid-placement.png");
        C::End();
        H::EndTerrain();
        const auto save = std::string(argv[5]) + "/construction-capture.park";
        SaveFlags flags{};
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Asset-backed editing save/reload failed");
        Check(!E::Valid(brake), "Loaded park reused old editor identity");
        std::cout << "Asset-backed new-family picker/editor/design/block-speed-cancel/stations/save passed\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

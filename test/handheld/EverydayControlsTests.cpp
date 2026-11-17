// SPDX-License-Identifier: GPL-3.0-only
// RC3 defect investigation: real screen-coordinate targeting on a native flat map.
// This is a desktop reproduction probe, not proof that the reported Brick defect is fixed.
#include <SDL.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/MenuThemes.h>
#include <openrct2-ui/handheld/ModernText.h>
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
#include <openrct2-ui/handheld/SceneryTools.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
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
        if (argc != 4) return 2;
        SDL_setenv("OPENRCT2_HANDHELD","1",1);
        SDL_setenv("SDL_VIDEODRIVER","offscreen",0);
        SDL_setenv("SDL_AUDIODRIVER","dummy",0);
        gCustomOpenRCT2DataPath=argv[1];gCustomUserDataPath=argv[2];
        gOpenRCT2Headless=true;gOpenRCT2NoGraphics=false;
        auto env=CreatePlatformEnvironment();env->SetBasePath(DirBase::rct2,argv[3]);
        auto ui=Ui::CreateUiContext(*env);
        auto context=CreateContext(std::move(env),Audio::CreateDummyAudioContext(),std::move(ui));
        Config::SetDefaults();Config::Get().general.rct2Path=argv[3];
        Config::Get().general.windowWidth=512;Config::Get().general.windowHeight=384;Config::Get().general.windowScale=1;
        Check(context->Initialise(),"Context failed");context->GetUiContext().CreateWindow();
        auto& state=getGameState();gameStateInitAll(state,{64,64});gLegacyScene=LegacyScene::playing;
        Check(W::MainOpen(),"No viewport");WindowGetMain()->onResize();InputSetState(InputState::normal);
        auto* wm=Ui::GetWindowManager();wm->CloseByClass(WindowClass::progressWindow);
        state.park.cash=10000000;state.park.flags.unset(ParkFlag::noMoney,ParkFlag::noMoneyScenario);
        for(int y=32;y<2016;y+=32)for(int x=32;x<2016;x+=32)
            MapGetSurfaceElementAt(CoordsXY{x,y})->setOwnership({OwnershipFlag::landOwned});
        auto& objects=context->GetObjectManager();
        for(const char* id:{"rct2.terrain_surface.grass","rct2.terrain_edge.rock","rct2.station.plain",
            "rct2.footpath_surface.tarmac","rct2.footpath_surface.queue_blue","rct2.footpath_railings.wood"})
            Check(objects.LoadObject(id),"Fixture object missing");
        namespace P=H::Paths;
        H::ControllerInput input;
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        const auto device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        Check(device >= 0, "No virtual controller");
        auto* joystick = SDL_JoystickOpen(device);
        uint32_t now = 100;
        auto pump = [&] { SDL_JoystickUpdate(); SDL_Event event; while (SDL_PollEvent(&event)) input.handleEvent(event); };
        pump(); input.update(true, now); pump(); input.update(true, now += 20);
        auto button = [&](SDL_GameControllerButton key) {
            SDL_JoystickSetVirtualButton(joystick, key, 1); pump();
            const auto frame = input.update(true, now += 20); H::HandleExplorerInput(input, frame);
            SDL_JoystickSetVirtualButton(joystick, key, 0); pump(); input.update(true, now += 20);
            WindowCullDead();
        };
        auto press = [&](H::Action action) {
            button(action == H::Action::confirm ? SDL_CONTROLLER_BUTTON_A : action == H::Action::back ? SDL_CONTROLLER_BUTTON_B
                : action == H::Action::contextSecondary ? SDL_CONTROLLER_BUTTON_Y : action == H::Action::nextOption ? SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
                : SDL_CONTROLLER_BUTTON_START);
        };
        auto nav = [&](int delta) { for (int i = 0; i < std::abs(delta); ++i) button(delta > 0 ? SDL_CONTROLLER_BUTTON_DPAD_DOWN : SDL_CONTROLLER_BUTTON_DPAD_UP); };

        auto capture=[&](const char* name){const auto* folder=std::getenv("POCKET_PATH_BATCH_CAPTURES");if(!folder)return;
            std::filesystem::create_directories(folder);Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();
            auto& rt=*engine.getRT();ResetAllSpriteQuadrantPlacements();Drawing::UpdatePaletteEffects();ViewportRender(rt,WindowGetMain()->viewport);
            if(auto* window=wm->FindByClass(WindowClass::handheldMenu))window->onDraw(rt);else H::DrawExplorer(rt);
            Check(H::FooterHeight({ 512, 384 }, H::LastControlHints) == 28, "Runtime footer expanded beyond one row");
            Image image;image.Width=512;image.Height=384;image.Depth=8;image.Stride=rt.LineStride();image.Palette=Drawing::gPalette;
            const auto* bytes=reinterpret_cast<uint8_t*>(rt.bits);image.Pixels.assign(bytes,bytes+rt.LineStride()*rt.height);
            Imaging::WriteToFile((std::filesystem::path(folder)/name).string(),image,ImageFormat::png);engine.EndDraw();};

        Check(context->LoadParkFromFile((std::filesystem::path(argv[3]) / "Scenarios/Electric Fields.SC6").string(), false, true), "Capture scenario failed");
        // Scenario start presents the objective as a Pocket page, never the desktop park window.
        Check(!wm->FindByClass(WindowClass::parkInformation), "Desktop objective window opened at scenario start");
        Check(W::HandheldOverviewIsOpen(), "Scenario start did not open the Pocket objectives page");
        wm->CloseByClass(WindowClass::progressWindow); WindowCullDead();
        WindowGetMain()->onResize(); InputSetState(InputState::normal);
        press(H::Action::back);
        Check(!wm->FindByClass(WindowClass::handheldMenu), "B from scenario objectives did not return to the park");
        auto* viewport = WindowGetMain()->viewport;
        viewport->viewPos = Translate3DTo2DWithZ(viewport->rotation, {1504, 1504, 32}) - ScreenCoordsXY{256,192};
        WindowGetMain()->savedViewPos = viewport->viewPos;
        auto menu = [&] { return wm->FindByClass(WindowClass::handheldMenu); };
        auto noDesktop = [&] { Check(!wm->FindByClass(WindowClass::savePrompt), "Desktop quit prompt opened"); };
        // Currency symbols draw in Pocket Sans (extra glyphs). Amounts must
        // remain readable on ivory cards and teal selections in the light theme.
        {
            Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(80,24);engine.BeginDraw();
            auto& rt=*engine.getRT();Drawing::UpdatePaletteEffects();
            const auto original=H::PocketPalette::style;H::PocketPalette::SetStyle(H::PocketPalette::Style::daylight);
            for(bool selected:{false,true}) {
                std::fill_n(rt.bits,rt.LineStride()*rt.height,static_cast<Drawing::PaletteIndex>(selected?194:225));
                H::DrawModernText(rt,{2,6},std::string(selected?"{BLACK}":"{WHITE}")+"£0.00");
                int readablePixels=0;
                for(int y=0;y<rt.height;++y)for(int x=0;x<rt.width;++x) {
                    const auto colour=Drawing::gPalette[static_cast<uint8_t>(rt.bits[y*rt.LineStride()+x])];
                    const int brightness=colour.red+colour.green+colour.blue;
                    if(selected?brightness>600:brightness<240)++readablePixels;
                }
                Check(readablePixels>=8,"Native currency fallback lost text contrast");
            }
            H::PocketPalette::SetStyle(original);engine.EndDraw();
        }
        // Real virtual-controller theme selection retains focus, pause and cash.
        gGamePaused=0;const auto themeCash=state.park.cash;
        W::HandheldMenuOpen();nav(12);capture("00a-daylight-choice.png");
        const auto originalTheme=H::PocketPalette::style;
        press(H::Action::confirm);
        Check(H::PocketPalette::style!=originalTheme,"Theme action did not switch");
        capture("00b-slate-choice.png");
        const auto secondTheme=H::PocketPalette::style;
        press(H::Action::confirm); // same focused row moves on to the third theme
        Check(H::PocketPalette::style!=originalTheme&&H::PocketPalette::style!=secondTheme,"Theme cycle skipped a theme");
        capture("00b2-pixel-choice.png");
        press(H::Action::confirm); // and back round to where it started
        Check(H::PocketPalette::style==originalTheme,"Theme focus or round trip lost");
        capture("00c-daylight-restored.png");
        H::MenuThemes::Settings reloaded;
        reloaded.Load(context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user));
        Check(H::PocketPalette::style==originalTheme,"Theme preference did not survive reload");
        press(H::Action::back);
        Check(gGamePaused==0 && state.park.cash==themeCash,"Theme altered simulation or money");
        {
            // Quick wheel: hold L2, point with the D-pad, release (after the hold time) to choose.
            // A quick tap keeps it open for A / B instead.
            auto step=[&]{pump();H::HandleExplorerInput(input,input.update(true,now+=20));WindowCullDead();};
            auto trigger=[&](bool down){SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,down?32767:-32768);step();};
            auto dpad=[&](SDL_GameControllerButton key,bool down){SDL_JoystickSetVirtualButton(joystick,key,down);step();};
            trigger(false);step();
            trigger(true);Check(H::QuickWheelOpen(),"L2 did not open the quick wheel");
            dpad(SDL_CONTROLLER_BUTTON_DPAD_RIGHT,true);capture("00d-quick-wheel.png");
            dpad(SDL_CONTROLLER_BUTTON_DPAD_UP,true);dpad(SDL_CONTROLLER_BUTTON_DPAD_UP,false); // Leaving a diagonal keeps it.
            dpad(SDL_CONTROLLER_BUTTON_DPAD_RIGHT,false);
            dpad(SDL_CONTROLLER_BUTTON_DPAD_RIGHT,true);dpad(SDL_CONTROLLER_BUTTON_DPAD_RIGHT,false); // Back to Rides.
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            trigger(false);
            Check(!H::QuickWheelOpen()&&W::HandheldRidesIsOpen(),"Releasing L2 did not open Rides");
            capture("00e-wheel-rides.png");
            wm->CloseByClass(WindowClass::handheldMenu);step();step();
            trigger(true);trigger(false);Check(H::QuickWheelOpen(),"A tapped wheel did not stay open");
            press(H::Action::back);Check(!H::QuickWheelOpen()&&!menu(),"B did not close the wheel");
            trigger(true);trigger(false);dpad(SDL_CONTROLLER_BUTTON_DPAD_DOWN,true);dpad(SDL_CONTROLLER_BUTTON_DPAD_DOWN,false);
            press(H::Action::confirm);Check(!H::QuickWheelOpen()&&menu(),"A on a pointed slot did not open it");
            wm->CloseByClass(WindowClass::handheldMenu);step();step();
        }
        // B preserves the state before entry, including external native reasons.
        for (int initial : std::array<int, 4>{0, GAME_PAUSED_NORMAL, GAME_PAUSED_MODAL, GAME_PAUSED_NORMAL | GAME_PAUSED_MODAL}) {
            gGamePaused = initial; W::HandheldMenuOpen(); press(H::Action::back);
            Check(gGamePaused == initial, "Back changed pre-menu pause ownership");
        }
        for (int initial : std::array<int, 2>{0, GAME_PAUSED_NORMAL}) for (int choice = 0; choice < 4; ++choice) {
            gGamePaused = initial; W::HandheldMenuOpen(); nav(8 + choice);
            capture(choice == 0 ? "01-paused-choice.png" : choice == 1 ? "02-normal-speed.png" : choice == 2 ? "03-double-speed.png" : "04-quad-speed.png");
            press(H::Action::confirm); GameActions::ProcessQueue(state);
            Check(!menu(), "Speed selection did not return to park");
            Check(bool(gGamePaused & GAME_PAUSED_NORMAL) == (choice == 0), "Selected simulation pause incorrect");
            if (choice) Check(gGameSpeed == choice, "Wrong native speed (4x requires native speed 3)");
            W::HandheldMenuOpen();
            if (choice == 3) { nav(11); capture("04-current-4x.png"); }
            press(H::Action::back);
            Check(bool(gGamePaused & GAME_PAUSED_NORMAL) == (choice == 0), "Repeated menu entry changed simulation");
        }
        gGamePaused = GAME_PAUSED_MODAL; W::HandheldMenuOpen(); nav(9); press(H::Action::confirm);
        Check(gGamePaused == GAME_PAUSED_MODAL, "Speed choice cleared an external pause reason");
        gGamePaused = 0; W::HandheldMenuOpen(); nav(6); press(H::Action::confirm);
        capture("05-quit-options.png"); noDesktop();
        nav(-1); press(H::Action::confirm); capture("06-discard-confirmation.png");
        press(H::Action::back); press(H::Action::back); // restore caller's Quit focus
        press(H::Action::confirm); // opens Quit again, not Resume
        Check(menu(), "Quit caller focus was not restored");
        press(H::Action::back); press(H::Action::back);
        ContextQuit(); noDesktop(); Check(menu(), "Native quit did not route to Pocket");
        press(H::Action::back); Check(!menu(), "Native quit cancellation trapped the caller");
        ContextQuit(); nav(-2); press(H::Action::confirm); press(H::Action::back); press(H::Action::back);
        Check(!menu(), "Save cancellation trapped the game"); noDesktop();
        // Real filesystem failure must keep the Pocket menu running and cancellable.
        const auto user = context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user);
        context->GetPlatformEnvironment().SetBasePath(DirBase::user, "/proc/pocket-save-failure");
        ContextQuit(); nav(-2); press(H::Action::confirm); press(H::Action::confirm);
        Check(menu(), "Failed save closed the menu"); noDesktop();
        capture("07-save-failure.png"); press(H::Action::back); press(H::Action::back);
        Check(!menu(), "Failed save could not be cancelled");
        context->GetPlatformEnvironment().SetBasePath(DirBase::user, user);
        // Successful save is complete and reloadable before the final exit is requested.
        const auto saved = std::filesystem::path(context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::saves)) / "Pocket-3.park";
        std::filesystem::remove(saved);
        ContextQuit(); nav(-2); press(H::Action::confirm); nav(2); press(H::Action::confirm);
        Check(!menu() && std::filesystem::is_regular_file(saved), "Save and quit did not finish its save"); noDesktop();
        Check(context->LoadParkFromFile(saved.string()), "Quit save did not reload");
        wm->CloseByClass(WindowClass::parkInformation); wm->CloseByClass(WindowClass::progressWindow); WindowCullDead();
        ContextQuit(); nav(-1); press(H::Action::confirm); nav(-1); press(H::Action::confirm);
        Check(!menu(), "Confirmed discard did not close Pocket"); noDesktop();
        std::cout << "PASS EverydayControlsTests: mapped SDL virtual controller, pause ownership, 1x/2x/4x, native quit routing, cancel/focus, failed and successful saves, no desktop re-prompt\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << "\n"; return 1; }
}

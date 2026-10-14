// SPDX-License-Identifier: GPL-3.0-only
// Asset-backed scenario, native actions/progression, production Pocket window rendering.
#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FinanceInformation.h>
#include <openrct2-ui/handheld/ManagementNews.h>
#include <openrct2-ui/handheld/ResearchCatalog.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2-ui/handheld/FileBrowsing.h>
#include <openrct2-ui/handheld/SaveSlots.h>
#include <openrct2-ui/handheld/Designs.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2/rct2/T6Exporter.h>
#include <openrct2/actions/ResultWithMessage.h>
#include <openrct2/ride/TrackDesignRepository.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/world/Map.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/park/ParkMarketingAction.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
#include <openrct2/actions/park/ParkSetLoanAction.h>
#include <openrct2-ui/handheld/Currency.h>
#include <openrct2/actions/park/ParkSetResearchFundingAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Window.h>
#include <openrct2/management/Marketing.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace W = Ui::Windows;
static void Check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 6)
            return 2;
        SDL_setenv("OPENRCT2_HANDHELD", "1", 1);
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
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
        Check(context->Initialise(), "Context failed");
        context->GetUiContext().CreateWindow();
        Check(context->LoadParkFromFile(argv[3], false, true), "Scenario failed");
        Ui::GetWindowManager()->CloseByClass(WindowClass::parkInformation);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        Check(WindowGetMain() || W::MainOpen(), "Main viewport missing");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        auto& state = getGameState();
        gGamePaused = 0;
        namespace fs = std::filesystem;
        const auto saves = fs::u8path(context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user,DirId::saves));
        fs::create_directories(saves/"autosave"); fs::create_directories(saves/"empty");
        const auto parkName=state.park.name;
        Check(ScenarioSave(state,(saves/"Roundtrip.park").u8string(),{})!=0,"Native save failed");
        for(int i=0;i<20;++i) fs::copy_file(saves/"Roundtrip.park",saves/("Dense-"+std::to_string(i)+".park"));
        fs::copy_file(saves/"Roundtrip.park",saves/"autosave"/"autosave_native.park");
        std::ofstream(saves/"Invalid.park")<<"corrupt file";
        std::ofstream(saves/"hidden.park.pending.park")<<"incomplete";
        fs::create_directories(argv[4]); std::ofstream provenance(fs::path(argv[4])/"provenance.txt");
        H::ControllerInput input;
        auto press=[&](H::Action a){H::Frame f; f.actions={a};H::HandleMenuInput(input,f);};
        auto choose=[&](int row){W::HandheldMenuMove(row); press(H::Action::confirm);};
        auto capture = [&](const char* name) {
            if(!WindowGetMain()) W::MainOpen();
            if(auto* main=WindowGetMain()) {
                CoordsXY centre{704,704};
                for(const auto& ride:RideManager(state)) if(ride.type==RIDE_TYPE_WOODEN_ROLLER_COASTER && !ride.overallView.isNull()) {centre=ride.overallView+CoordsXY{16,16};break;}
                if(std::string(name)=="24-native-placement-handoff.png") centre={704,704};
                if(main->viewport) ViewportRemove(main->viewport);
                main->viewport=nullptr;
                ViewportCreate(*main,{0,0},512,384,Focus(CoordsXYZ{centre,TileElementHeight(centre)}));
            }
            const auto cash = state.park.cash;
            const auto ledger = state.park.expenditureTable;
            if (auto* native = dynamic_cast<Drawing::X8DrawingEngine*>(context->GetDrawingEngine()))
            {
                native->Drawing::X8DrawingEngine::Resize(512, 384);
                native->Drawing::X8DrawingEngine::BeginDraw();
                ViewportUpdatePosition(WindowGetMain());
                native->Drawing::X8DrawingEngine::EndDraw();
            }
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            Drawing::UpdatePaletteEffects();
            auto& rt = *engine.getRT();
            if(auto* main=WindowGetMain();main && main->viewport) {
                const auto& v=*main->viewport;
                provenance << name << " viewport="<<v.width<<","<<v.height<<" view="<<v.viewPos.x<<","<<v.viewPos.y<<" flags="<<v.flags.holder<<"\n";
            }
            if (auto* main = WindowGetMain(); main && main->viewport)
                ViewportRender(rt, main->viewport);
            auto* menu = Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu);
            Check(menu || H::Designs::Active(), "Capture menu/tool missing");
            if(menu) menu->onDraw(rt);
            else {
                H::Designs::DrawPanel(rt,{512,384});
                const auto query=H::Designs::Query(CoordsXY{704,704});
                Check(query.error==GameActions::Status::ok,"Native handoff quote unavailable");
                H::DrawCostBadge(rt,{340,276},H::FormatMoney(query.cost));
                provenance << "Native handoff cost="<<query.cost<<"\n";
            }
            Image image;
            image.Width = 512;
            image.Height = 384;
            image.Depth = 8;
            image.Stride = rt.LineStride();
            image.Palette = Drawing::gPalette;
            auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(argv[4]) / name).string(), image, ImageFormat::png);
            engine.EndDraw();
            Check(cash == state.park.cash && ledger == state.park.expenditureTable, "Rendering changed accounting");
            provenance << name << " ticks=" << state.currentTicks << " cash=" << cash << "\n";
        };
        W::HandheldLoadOpen(); choose(3);
        Check(W::HandheldConstructionMenuIsOpen(),"Browser not routed to controller");
        capture("01-park-browser.png");
        choose(2); capture("02-autosaves.png");
        choose(2); capture("15-park-native-metadata.png");
        press(H::Action::confirm); // explicit Load -> Cancel-first discard
        Check(state.park.name==parkName,"Selection loaded park");
        capture("03-discard-review.png");
        {
            SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
            Check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0,"SDL controller initialise failed");
            H::ControllerInput virtualInput;
            const auto device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
            Check(device>=0,"SDL virtual controller attach failed");
            auto* joystick=SDL_JoystickOpen(device);Check(joystick,"SDL joystick open failed");
            auto pump=[&]{SDL_Event e;while(SDL_PollEvent(&e))virtualInput.handleEvent(e);};
            pump();virtualInput.update(true,100);pump();virtualInput.update(true,110);
            Check(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1)==0,"SDL A failed");
            SDL_JoystickUpdate();pump();
            const auto frame=virtualInput.update(true,120);
            Check(frame.actions.size()==1 && frame.actions[0]==H::Action::confirm,"SDL A did not map to confirm");
            Check(H::HandleMenuInput(virtualInput,frame),"SDL frame not consumed by review");
            Check(virtualInput.update(true,130).actions.empty(),"Held SDL A repeated across browser transition");
            Check(state.park.name==parkName,"Held SDL cancel loaded park");
            SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);SDL_JoystickUpdate();pump();virtualInput.update(true,140);
            SDL_JoystickClose(joystick);SDL_JoystickDetachVirtual(device);
        }
        Check(W::HandheldConstructionMenuIsOpen(),"Discard cancel lost browser");
        capture("04-discard-cancel-return.png");
        press(H::Action::back); // load slots
        choose(3); choose(3); capture("05-empty-folder.png");
        fs::remove(saves/"empty");choose(1);capture("22-unreadable-folder.png");
        choose(0); // parent selects empty folder
        W::HandheldMenuMove(19); capture("06-dense-paging.png");
        press(H::Action::back); choose(3);
        H::FileBrowsing::State listing; listing.Open(saves,"*.park;*.sv6;*.sc6;*.sc4;*.sv4;*.sv7;*.sea");
        auto index=[&](const char* name){for(size_t i=0;i<listing.entries.size();++i)if(listing.entries[i].path.filename()==name)return static_cast<int>(i+2);throw std::runtime_error("Fixture missing");};
        choose(index("Invalid.park")); press(H::Action::confirm); W::HandheldMenuMove(-1); press(H::Action::confirm);
        Check(state.park.name==parkName && gLegacyScene==LegacyScene::playing,"Early importer failure lost park");
        Check(W::HandheldConstructionMenuIsOpen(),"Invalid importer failure lost recovery");
        capture("07-invalid-load-recovery.png");
        press(H::Action::back); choose(3); choose(index("Roundtrip.park")); press(H::Action::confirm);
        std::ofstream(saves/"Roundtrip.park",std::ios::app)<<"changed";
        W::HandheldMenuMove(-1); press(H::Action::confirm);
        Check(state.park.name==parkName,"Stale file loaded"); capture("08-stale-file-review.png");
        press(H::Action::back); press(H::Action::back); choose(3); choose(2); choose(2); press(H::Action::confirm);
        W::HandheldMenuMove(-1); press(H::Action::confirm);
        Check(state.park.name==parkName && gCurrentLoadedPath.find("autosave_native.park")!=std::string::npos,"Native roundtrip not loaded");
        Check(!H::Designs::Active() && !H::CustomTracks::Active(),"Load did not clear tools");
        // Native named Save As and unchanged quick-slot menu.
        Check(ScenarioSave(state,(saves/fs::u8path("Résumé.park")).u8string(),{})!=0,"UTF-8 fixture save failed");
        auto named=[&] { Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);W::HandheldMenuOpen();choose(3);choose(3); };
        named(); capture("09-save-destination.png");
        choose(0); capture("10-save-keyboard.png");
        choose(47); // keyboard Back must not save
        Check(gCurrentLoadedPath.find("autosave_native.park")!=std::string::npos,"Keyboard cancel changed save identity");
        choose(0);
        choose(46); capture("11-named-save-review.png");
        press(H::Action::confirm); // default Cancel
        Check(gCurrentLoadedPath.find("autosave_native.park")!=std::string::npos,"Cancel changed save identity");
        choose(46); W::HandheldMenuMove(-1); press(H::Action::confirm);
        Check(fs::exists(saves/"autosave_native.park") && gCurrentLoadedPath==(saves/"autosave_native.park").u8string(),"Named save or identity failed");
        Check(context->LoadParkFromFile(gCurrentLoadedPath),"Named save roundtrip failed");
        Check(state.park.name==parkName,"Named save changed native park");
        named(); H::FileBrowsing::State namedListing; namedListing.Open(saves,"*.park");
        auto namedIndex=[&](const std::string& name){for(size_t i=0;i<namedListing.entries.size();++i)if(namedListing.entries[i].path.filename().u8string()==name)return static_cast<int>(i+3);throw std::runtime_error("Named fixture missing");};
        choose(namedIndex("Résumé.park")); choose(46); capture("12-overwrite-review.png");
        std::ofstream(saves/fs::u8path("Résumé.park"),std::ios::app)<<"external change";
        const auto changed=H::FileBrowsing::Capture(saves/fs::u8path("Résumé.park"));
        W::HandheldMenuMove(-1);press(H::Action::confirm);
        Check(changed && H::FileBrowsing::Current(*changed),"Stale overwrite changed destination");capture("13-stale-save-recovery.png");
        named(); choose(0); choose(46);
        const auto destination=saves/"autosave_native.park";
        const auto before=H::FileBrowsing::Capture(destination);
        auto stage=destination;stage+=".pending.park";fs::create_directory(stage);
        W::HandheldMenuMove(-1);press(H::Action::confirm);
        Check(before && H::FileBrowsing::Current(*before),"Publication failure changed prior save");capture("14-save-failure.png");
        fs::remove(stage);press(H::Action::confirm);
        Check(Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu),"Save recovery lost park menu");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        Ui::GetWindowManager()->CloseByClass(WindowClass::error);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        Ui::GetWindowManager()->CloseByClass(WindowClass::parkInformation);
        InputSetState(InputState::normal);
        if(!WindowGetMain()) W::MainOpen();
        WindowGetMain()->onResize();
        const auto tracks=fs::u8path(context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user,DirId::trackDesigns));
        fs::create_directories(tracks/"folder");
        bool exported=false;
        for(const auto& ride:RideManager(state)) if(ride.type==RIDE_TYPE_WOODEN_ROLLER_COASTER) {
            TrackDesign td;TrackDesignState tds{};
            if(td.CreateTrackDesign(tds,ride).Successful) {
                Check(RCT2::T6Exporter(td).SaveTrack((tracks/"Native wooden.td6").u8string().c_str()),"Native design export failed");exported=true;break;
            }
        }
        Check(exported,"Native wooden source missing");
        fs::copy_file(tracks/"Native wooden.td6",tracks/"folder"/"Nested wooden.td6");
        std::ofstream(tracks/"Invalid.td6")<<"invalid design";
        TrackRepositoryScan();
        const auto cash=state.park.cash; const auto rides=RideManager(state).size();
        W::HandheldDesignPickerOpen();press(H::Action::contextPrimary);
        Check(W::HandheldConstructionMenuIsOpen(),"X did not open native design browser");capture("16-design-browser.png");
        choose(2); capture("17-design-folder.png");
        choose(2); capture("18-design-inspection.png");
        Check(state.park.cash==cash && RideManager(state).size()==rides && !H::Designs::Active(),"Design inspection mutated park");
        press(H::Action::confirm);Check(H::Designs::Active(),"Design inspection did not hand off");
        GameActions::ParkSetLoanAction loan(state.park.maxBankLoan);
        Check(GameActions::Execute(&loan,state).error==GameActions::Status::ok,"Native permitted loan failed");
        GameActions::ProcessQueue(state);
        H::Designs::UpdatePreview(CoordsXY{704,704});
        capture("24-native-placement-handoff.png");
        H::Designs::Height(1);H::Designs::Rotate();const auto height=H::Designs::HeightOffset();const auto rotation=H::Designs::Orientation();
        W::HandheldDesignPickerOpen();capture("19-design-placement-return.png");
        press(H::Action::confirm);press(H::Action::confirm);
        Check(H::Designs::Active() && H::Designs::HeightOffset()==height && H::Designs::Orientation()==rotation,"Design reinspection lost placement choices");
        H::Frame back;back.actions={H::Action::back};H::HandleExplorerInput(input,back);
        Check(!H::Designs::Active() && W::HandheldConstructionMenuIsOpen(),"Placement cancel did not return to browser");
        // Selected row retained. Reinspect, then invalidate the actual file.
        press(H::Action::confirm);std::ofstream(tracks/"folder"/"Nested wooden.td6",std::ios::app)<<"changed";
        press(H::Action::confirm);Check(!H::Designs::Active(),"Stale design handed off");capture("20-stale-design.png");
        press(H::Action::back); W::HandheldMenuMove(-2);press(H::Action::confirm); // parent
        W::HandheldMenuMove(1);press(H::Action::confirm); // invalid design
        capture("21-invalid-design.png");
        Check(!H::Designs::Active(),"Invalid design activated");
        // Actual native serialization into an unwritable procfs destination.
        const auto loadedIdentity=gCurrentLoadedPath;
        Check(!H::WriteSaveAtomically(fs::path("/proc/Pocket-native-save-failure.park"),[&](const fs::path& temporary) {
            return ScenarioSave(state,temporary.u8string(),{})!=0;
        }),"Unwritable native serialization unexpectedly succeeded");
        Check(gCurrentLoadedPath==loadedIdentity,"Failed serialization changed current save identity");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        W::HandheldMenuOpen();choose(4);choose(3);choose(0);choose(46);
        const auto exitDestination=fs::u8path(gScenarioSavePath);
        auto exitStage=exitDestination;exitStage+=".pending.park";fs::create_directory(exitStage);
        const auto modeBefore=gSavePromptMode;
        W::HandheldMenuMove(-1);press(H::Action::confirm);
        Check(gSavePromptMode==modeBefore && gLegacyScene==LegacyScene::playing,"Failed Save and exit initiated quit");
        capture("23-save-and-exit-failure.png");
        fs::remove(exitStage);
        Config::Get().general.confirmationPrompt=false;
        press(H::Action::confirm);
        Check(gSavePromptMode==PromptMode::quit && fs::is_regular_file(exitDestination),"Successful named Save and exit did not publish then request native quit");
        std::cout<<"Native browser/import/failure/roundtrip/controller and named Save As and design browsing PASS\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}

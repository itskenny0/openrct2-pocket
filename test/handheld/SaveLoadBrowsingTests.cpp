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
#include <openrct2-ui/handheld/Designs.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/park/ParkMarketingAction.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
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
            if (auto* main = WindowGetMain(); main && main->viewport)
                ViewportRender(rt, main->viewport);
            auto* menu = Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu);
            Check(menu, "Capture menu missing");
            menu->onDraw(rt);
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
        choose(2); // native autosave -> Cancel-first discard
        Check(state.park.name==parkName,"Selection loaded park");
        capture("03-discard-review.png");
        press(H::Action::confirm); // default Cancel
        Check(W::HandheldConstructionMenuIsOpen(),"Discard cancel lost browser");
        capture("04-discard-cancel-return.png");
        press(H::Action::back); // load slots
        choose(3); choose(3); capture("05-empty-folder.png");
        choose(0); // parent selects empty folder
        W::HandheldMenuMove(19); capture("06-dense-paging.png");
        press(H::Action::back); choose(3);
        H::FileBrowsing::State listing; listing.Open(saves,"*.park;*.sv6;*.sc6;*.sc4;*.sv4;*.sv7;*.sea");
        auto index=[&](const char* name){for(size_t i=0;i<listing.entries.size();++i)if(listing.entries[i].path.filename()==name)return static_cast<int>(i+2);throw std::runtime_error("Fixture missing");};
        choose(index("Invalid.park")); W::HandheldMenuMove(-1); press(H::Action::confirm);
        Check(state.park.name==parkName && gLegacyScene==LegacyScene::playing,"Early importer failure lost park");
        Check(W::HandheldConstructionMenuIsOpen(),"Invalid importer failure lost recovery");
        capture("07-invalid-load-recovery.png");
        press(H::Action::back); choose(3); choose(index("Roundtrip.park"));
        std::ofstream(saves/"Roundtrip.park",std::ios::app)<<"changed";
        W::HandheldMenuMove(-1); press(H::Action::confirm);
        Check(state.park.name==parkName,"Stale file loaded"); capture("08-stale-file-review.png");
        press(H::Action::back); press(H::Action::back); choose(3); choose(2); choose(2);
        W::HandheldMenuMove(-1); press(H::Action::confirm);
        Check(state.park.name==parkName && gCurrentLoadedPath.find("autosave_native.park")!=std::string::npos,"Native roundtrip not loaded");
        Check(!H::Designs::Active() && !H::CustomTracks::Active(),"Load did not clear tools");
        std::cout<<"Native browser/import/failure/roundtrip/controller PASS\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}

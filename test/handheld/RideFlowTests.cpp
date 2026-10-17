// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include "RideTabNavigation.h"
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2/management/Research.h>
#include <openrct2/interface/Viewport.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/ride/Vehicle.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace C = H::CustomTracks;
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
        if (argc != 3 && argc != 4)
            return 2;
        SDL_setenv("OPENRCT2_HANDHELD", "1", 1);
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 0);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 0);
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = argc != 4;
        auto env = CreatePlatformEnvironment();
        if (argc == 4)
            env->SetBasePath(DirBase::rct2, argv[3]);
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        if (argc == 4)
            Config::Get().general.rct2Path = argv[3];
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Config::Get().general.windowScale = 1;
        Check(context->Initialise(), "Context failed");
        context->GetUiContext().CreateWindow();
        auto& state = getGameState();
        gameStateInitAll(state, { 64, 64 });
        gLegacyScene = LegacyScene::playing;
        Check(W::MainOpen(), "Fixture viewport missing");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        state.park.cash = 10000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        for (int y = 32; y < 2016; y += 32)
            for (int x = 32; x < 2016; x += 32)
                MapGetSurfaceElementAt(CoordsXY{ x, y })->setOwnership({ OwnershipFlag::landOwned });
        auto& objects = context->GetObjectManager();
        Check(
            objects.LoadObject("rct2.terrain_surface.grass") && objects.LoadObject("rct2.terrain_edge.rock")
                && objects.LoadObject("rct2.station.plain"),
            "Fixture terrain/station missing");
        namespace F=H::FlatRides;namespace M=H::Mazes;
        auto* wm=Ui::GetWindowManager();H::ControllerInput input;
        auto press=[&](H::Action action){H::Frame frame;frame.actions={action,action};H::HandleExplorerInput(input,frame);};
        auto closeMenu=[&](){wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();};
        auto focus=[&](CoordsXYZ point){auto* v=WindowGetMain()->viewport;v->zoom=ZoomLevel{0};
            v->viewPos=Translate3DTo2DWithZ(v->rotation,point)-ScreenCoordsXY{256,192};WindowGetMain()->savedViewPos=v->viewPos;};
        auto capture=[&](const char* name){const auto* folder=std::getenv("POCKET_RIDE_FLOW_CAPTURES");if(!folder||argc!=4)return;
            std::filesystem::create_directories(folder);Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();
            auto& rt=*engine.getRT();ResetAllSpriteQuadrantPlacements();Drawing::UpdatePaletteEffects();ViewportRender(rt,WindowGetMain()->viewport);
            if(auto* window=wm->FindByClass(WindowClass::handheldMenu))window->onDraw(rt);else H::DrawExplorer(rt);
            Image image;image.Width=512;image.Height=384;image.Depth=8;image.Stride=rt.LineStride();image.Palette=Drawing::gPalette;
            const auto* bytes=reinterpret_cast<uint8_t*>(rt.bits);image.Pixels.assign(bytes,bytes+rt.LineStride()*rt.height);
            Imaging::WriteToFile((std::filesystem::path(folder)/name).string(),image,ImageFormat::png);engine.EndDraw();};
        auto pickTrackOption=[&](const char* key){
            const auto entries=C::Pieces();const auto found=std::find_if(entries.begin(),entries.end(),[&](const auto& e){return e.identity.key==key;});
            Check(found!=entries.end(),"Track option missing");const auto index=static_cast<size_t>(found-entries.begin());
            for(size_t i=0;i<entries.size();++i)W::HandheldObjectPickerMove(0,-1);
            W::HandheldObjectPickerMove(-1,0);W::HandheldObjectPickerMove(-1,0);
            for(size_t i=0;i<index/3;++i)W::HandheldObjectPickerMove(0,1);
            for(size_t i=0;i<index%3;++i)W::HandheldObjectPickerMove(1,0);
            press(H::Action::confirm);
        };
        auto load=[&](const char* name,ride_type_t type){auto* o=objects.LoadObject(name);Check(o,"Ride object missing");const auto i=objects.GetLoadedObjectEntryIndex(o);
            ResearchInsertRideEntry(i,true);RideEntrySetInvented(i);RideTypeSetInvented(type);return i;};
        auto flat=load("rct2.ride.mgr1",RIDE_TYPE_MERRY_GO_ROUND);
        Check(H::BeginFlatRide(flat),"Flat start failed");focus({656,656,48});F::UpdatePreview(CoordsXY{640,640});capture("01-flat-placement.png");
        const auto previewCash=state.park.cash;press(H::Action::back);Check(F::CurrentStage()==F::Stage::inactive&&state.park.cash==previewCash,"Unplaced flat cancellation spent cash");Check(H::BeginFlatRide(flat),"Flat restart failed");
        const auto quote=F::Action(CoordsXY{640,640},false);const auto flatCash=state.park.cash;
        Check(quote.error==GameActions::Status::ok&&F::Action(CoordsXY{640,640},true).error==GameActions::Status::ok,"Flat purchase failed");GameActions::ProcessQueue(state);
        const auto target=*F::Target();Check(state.park.cash==flatCash-quote.cost,"Flat native cost changed");
        press(H::Action::openMenu);Check(W::HandheldRidesPresentation().tabs.size()==4,"Construction Start lost modern workspace");
        closeMenu();H::EndTerrain();
        bool hit=false;const auto location=H::RideManagement::Location(target);Check(location.has_value(),"Ride has no native location");
        focus(*location);
        const auto centre=WindowGetMain()->viewport->viewPos;
        for(int y=-48;y<=48&&!hit;y+=4){WindowGetMain()->viewport->viewPos=centre+ScreenCoordsXY{0,y};
            const auto info=GetMapCoordinatesFromPos({256,192},{ViewportInteractionItem::ride});
            hit=info.Element&&info.Element->getRideIndex()==target.id;}
        Check(hit,"Native park-centre ride hit testing failed");const auto camera=WindowGetMain()->viewport->viewPos;
        press(H::Action::confirm);Check(W::HandheldRidesPresentation().tabs.size()==4,"Park A did not open Ride Overview");{const auto m=W::HandheldRidesPresentation();Check(H::MeasureManagement({496,H::Layout::ContentHeight({512,384})},m).capacity>=m.rows.size(),"Overview unnecessarily splits ordinary actions");}capture("02-park-ride-overview.png");
        press(H::Action::nextOption);capture("03-ride-operations.png");
        FocusRideTabAction("Operations...");press(H::Action::confirm);press(H::Action::back);
        Check(W::HandheldRidesPresentation().selectedTab==1,"Operations Back lost tab");
        press(H::Action::back);WindowCullDead();Check(!wm->FindByClass(WindowClass::handheldMenu)&&WindowGetMain()->viewport->viewPos==camera,"Park entry Back lost camera/context");
        W::HandheldRideOpen(target.id);FocusRideTabAction("Place entrance / exit...");const auto placementFocus=W::HandheldRidesPresentation().selected;
        press(H::Action::confirm);Check(F::CurrentStage()==F::Stage::entrance,"Placement handoff failed");capture("04-access-placement.png");
        press(H::Action::back);WindowCullDead();Check(W::HandheldRidesPresentation().tabs.size()==4&&W::HandheldRidesPresentation().selected==placementFocus,"Access cancellation lost focus");
        closeMenu();Check(H::ResumeFlatRide(target.id),"Resume access failed");
        for(int part=0;part<2;++part){bool done=false;for(int y=480;y<=800&&!done;y+=32)for(int x=480;x<=800&&!done;x+=32) {
            if(F::Action(CoordsXY{x,y},false).error==GameActions::Status::ok){Check(F::Action(CoordsXY{x,y},true).error==GameActions::Status::ok,"Access action failed");GameActions::ProcessQueue(state);done=true;} }
            Check(done,"No native access side");}
        H::UpdateExplorer(input);Check(W::HandheldRidesPresentation().tabs.size()==4,"Completion returned to legacy handoff");
        press(H::Action::contextPrimary);Check(W::HandheldRidesPresentation().title=="Entrances / exits","X Access did not open points");capture("05-existing-access.png");press(H::Action::back);
        FocusRideTabAction("Open...");press(H::Action::confirm);Check(W::HandheldRidesPresentation().confirmation,"Native Open unavailable after access placement");
        W::HandheldMenuMove(-1);press(H::Action::confirm);GameActions::ProcessQueue(state);Check(GetRide(target.id)->status==RideStatus::open,"Modern Open action failed");capture("05a-open-ride.png");
        FocusRideTabAction("Close...");press(H::Action::confirm);W::HandheldMenuMove(-1);press(H::Action::confirm);GameActions::ProcessQueue(state);Check(GetRide(target.id)->status==RideStatus::closed,"Modern Close action failed");
        FocusRideTabAction("Close...");press(H::Action::confirm);Check(W::HandheldRidesPresentation().confirmation&&W::HandheldRidesPresentation().selected==1,"Status review lost Cancel-first");
        press(H::Action::back);FocusRideTabAction("Demolish...");press(H::Action::confirm);capture("06-demolition-review.png");press(H::Action::back);Check(H::RideManagement::Resolve(target),"Cancel demolished ride");closeMenu();
        const auto wooden=load("rct2.ride.ptct1",RIDE_TYPE_WOODEN_ROLLER_COASTER);
        Check(H::BeginCustomTrack(wooden,RIDE_TYPE_WOODEN_ROLLER_COASTER),"Coaster start failed");GameActions::ProcessQueue(state);C::Update(CoordsXY{1408,640});focus({1424,656,64});
        for(int i=0;i<3;++i){Check(C::Select(TrackElemType::endStation)&&C::Action(true).error==GameActions::Status::ok,"Station action failed");GameActions::ProcessQueue(state);}
        Check(C::Select(TrackElemType::flat)&&C::Action(true).error==GameActions::Status::ok,"Track action failed");GameActions::ProcessQueue(state);
        const auto coaster=*C::Target();C::Update(CoordsXY{1408,640});capture("07-coaster-build.png");
        const auto beforeUndo=state.park.cash;press(H::Action::back);GameActions::ProcessQueue(state);Check(C::Active()&&state.park.cash>beforeUndo,"Native B Undo changed");
        press(H::Action::slopeUp);C::Update(CoordsXY{1408,640});capture("08-coaster-adjust.png");
        press(H::Action::openBuild);Check(W::HandheldObjectPickerIsOpen(),"Piece catalog missing");capture("09-track-pieces.png");
        press(H::Action::openMenu);Check(!C::Active()&&W::HandheldRidesPresentation().tabs.size()==4,"Start from piece catalog failed to return to ride");
        FocusRideTabAction("Continue track...");press(H::Action::confirm);press(H::Action::openBuild);press(H::Action::back);WindowCullDead();
        Check(C::Active(),"Piece Back ended builder");press(H::Action::openBuild);pickTrackOption("camera");Check(C::CameraMode(),"D-pad camera selection failed");press(H::Action::back);Check(C::Active()&&!C::CameraMode(),"Camera Back failed to return to building");
        press(H::Action::openMenu);Check(!C::Active()&&W::HandheldRidesPresentation().tabs.size()==4,"Coaster Start did not return to ride");
        const auto resume=FocusRideTabAction("Continue track...");press(H::Action::confirm);Check(C::Active(),"Continue track failed");press(H::Action::openMenu);
        Check(W::HandheldRidesPresentation().selectedTab==1&&W::HandheldRidesPresentation().selected==resume,"Coaster return lost Operations focus");
        press(H::Action::confirm);press(H::Action::openBuild);pickTrackOption("leave");
        Check(!C::Active()&&W::HandheldRidesPresentation().selectedTab==1,"Picker leave failed to return to ride");closeMenu();
        const auto mazeIndex=load("rct2.ride.hmaze",RIDE_TYPE_MAZE);Check(H::BeginMaze(mazeIndex),"Maze begin failed");GameActions::ProcessQueue(state);M::Update(CoordsXY{960,1280});focus({976,1296,40});
        Check(M::Build(true).error==GameActions::Status::ok,"Maze native cell failed");GameActions::ProcessQueue(state);const auto maze=*M::Target();capture("10-maze-build.png");
        const auto mazeCash=state.park.cash;M::ToggleCamera();press(H::Action::back);Check(M::Active()&&!M::CameraMode(),"Maze camera Back left builder");
        press(H::Action::back);Check(!M::Active()&&W::HandheldRidesPresentation().tabs.size()==4&&H::RideManagement::Resolve(maze)&&state.park.cash==mazeCash,"Maze leave changed native state");closeMenu();
        W::HandheldDashboardOpen();while(W::HandheldDashboardSectionIndex()!=1)W::HandheldDashboardMove(0,1);W::HandheldDashboardMove(1,0);
        const auto roster=W::HandheldRidesPresentation();const auto row=std::find(roster.rows.begin(),roster.rows.end(),GetRide(coaster.id)->getName());Check(row!=roster.rows.end(),"Coaster missing from roster");
        const auto rosterRow=static_cast<size_t>(row-roster.rows.begin());W::HandheldMenuMove(static_cast<int>(rosterRow)-static_cast<int>(roster.selected));press(H::Action::confirm);
        FocusRideTabAction("Continue track...");press(H::Action::confirm);press(H::Action::openMenu);press(H::Action::back);
        Check(W::HandheldRidesPresentation().selected==rosterRow&&W::HandheldRidesPresentation().tabs.empty(),"Roster return lost original row");capture("11-roster-return.png");
        std::cout<<"PASS native park selection / modern tabs / contextual access / camera preservation / safe status and demolition reviews\n";
        std::cout<<"PASS coaster Undo/piece/camera/return, flat access cancellation/completion, maze camera/leave, roster focus restoration\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}


// SPDX-License-Identifier: GPL-3.0-only
#include <SDL.h>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/PathSurfacePicker.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/interface/WindowBase.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/scenario/ScenarioCategory.h>
#include <openrct2/scenario/ScenarioRepository.h>
#include <openrct2/scenario/ScenarioSources.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/object/ObjectLimits.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
using namespace OpenRCT2;
namespace H=Ui::Handheld;namespace W=Ui::Windows;
static void Check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
#include <fstream>
#include <openrct2-ui/handheld/ConstructionPanel.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2/management/Research.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/drawing/Drawing.Sprite.h>
#include <openrct2/drawing/G1Element.h>
#include <openrct2/drawing/Palette.h>
int main(int argc,char** argv) { try {
 if(argc!=4)return 2;
 SDL_setenv("OPENRCT2_HANDHELD","1",1);gCustomOpenRCT2DataPath=argv[1];gCustomUserDataPath=argv[2];
 gOpenRCT2Headless=true;gOpenRCT2NoGraphics=true;
 auto env=CreatePlatformEnvironment();auto ui=Ui::CreateUiContext(*env);
 auto context=CreateContext(std::move(env),Audio::CreateDummyAudioContext(),std::move(ui));
 Config::SetDefaults();Config::Get().general.windowWidth=512;Config::Get().general.windowHeight=384;Config::Get().general.windowScale=1;
 Check(context->Initialise(),"Context failed");context->GetUiContext().CreateWindow();auto* wm=Ui::GetWindowManager();wm->CloseAllExceptFlags({});WindowCullDead();
 gameStateInitAll(getGameState(),{64,64});gLegacyScene=LegacyScene::playing;
 getGameState().park.cash=10000000;
 getGameState().park.flags.unset(ParkFlag::noMoney,ParkFlag::noMoneyScenario);
 for(int y=32;y<2016;y+=32)for(int x=32;x<2016;x+=32) MapGetSurfaceElementAt(CoordsXY{x,y})->setOwnership({OwnershipFlag::landOwned});
 auto& objects=context->GetObjectManager();
 Check(objects.LoadObject("rct2.footpath_surface.tarmac"),"Path missing");Check(objects.LoadObject("rct2.footpath_surface.queue_blue"),"Queue missing");
 const auto cash=getGameState().park.cash;
 std::filesystem::create_directories(argv[3]);
 gOpenRCT2NoGraphics=false;GfxLoadG2PalettesFontsTracks();Drawing::initColourMaps();FontSpriteInitialiseCharacters();
   auto capture=[&](const char* name,bool placement=false) {
    Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();
    auto& rt=*engine.getRT();GfxClear(rt,Drawing::PaletteIndex::pi10);
    if(placement) {
     H::Paths::DrawOverlay(rt,H::Concept::kOrigin);
     H::DrawConstructionFooter(rt,{512,384},H::Paths::ConnectedReady()?"Extend":"Start","Options");
    } else {
     auto* window=wm->FindByClass(WindowClass::handheldMenu);Check(window,"Capture window missing");window->onDraw(rt);
    }
    Drawing::GamePalette palette{};const auto* pal=GfxGetG1Palette(SPR_GAME_DEFAULT_PALETTE);
    for(int i=0;i<pal->numColours;i++)palette[pal->startIndex+i]={pal->palette[i].blue,pal->palette[i].green,pal->palette[i].red,255};
    std::ofstream out(std::filesystem::path(argv[3])/name,std::ios::binary);out<<"P6\n512 384\n255\n";
    for(int y=0;y<384;y++)for(int x=0;x<512;x++) {
     const auto c=palette[static_cast<unsigned>(rt.bits[y*rt.LineStride()+x])];
     const char rgb[]={static_cast<char>(c.red),static_cast<char>(c.green),static_cast<char>(c.blue)};out.write(rgb,3);
    }
    engine.EndDraw();
   };

 H::Paths::Begin(false);W::HandheldPathOptionsOpen();W::HandheldMenuMove(6);capture("path-settings.ppm");
 W::HandheldMenuConfirm();Check(W::HandheldObjectPickerIsOpen(),"Surface action missing");W::HandheldMenuBack();
 W::HandheldMenuConfirm();Check(W::HandheldObjectPickerIsOpen(),"Surface-picker Back lost options focus");W::HandheldMenuBack();
 W::HandheldMenuBack();Check(H::Paths::IsActive(),"Back ended path tool");H::Paths::End();
 H::Paths::BeginConnected(true);W::HandheldPathOptionsOpen();W::HandheldMenuMove(6);capture("connected-settings.ppm");
 W::HandheldMenuConfirm();Check(W::HandheldObjectPickerIsOpen(),"Connected surface action missing");W::HandheldMenuBack();
 W::HandheldMenuConfirm();Check(W::HandheldObjectPickerIsOpen(),"Connected surface-picker Back lost focus");W::HandheldMenuBack();
 W::HandheldMenuMove(1);W::HandheldMenuConfirm(); // No new section: native undo refuses and retains focus.
 W::HandheldMenuMove(-1);W::HandheldMenuConfirm();Check(W::HandheldObjectPickerIsOpen(),"Undo error lost surface neighbour/focus");W::HandheldMenuBack();
 W::HandheldMenuBack();Check(H::Paths::IsConnected()&&H::Paths::IsQueue(),"Back lost connected queue");H::Paths::End();
 Check(getGameState().park.cash==cash,"Settings spent cash");
 // Build native ride/access fixture, then exercise both point-specific handoffs.
 gOpenRCT2NoGraphics=true;
 for(auto name:{"rct2.terrain_surface.grass","rct2.terrain_edge.rock","rct2.station.plain","rct2.footpath_railings.wood"}) Check(objects.LoadObject(name),"Fixture object missing");
 auto* rideObject=objects.LoadObject("rct2.ride.mgr1");Check(rideObject,"Carousel missing");
 const auto index=objects.GetLoadedObjectEntryIndex(rideObject);ResearchInsertRideEntry(index,true);RideEntrySetInvented(index);RideTypeSetInvented(FlatRidePlacement::Type(index));
 Check(H::BeginFlatRide(index),"Ride start failed");Check(H::FlatRides::Action(CoordsXY{640,640},true).error==GameActions::Status::ok,"Ride placement failed");GameActions::ProcessQueue(getGameState());
 const auto target=*H::FlatRides::Target();
 for(int n=0;n<2;++n) {bool done=false;
  for(int y=480;y<=800&&!done;y+=32)for(int x=480;x<=800&&!done;x+=32)if(H::FlatRides::Action(CoordsXY{x,y},false).error==GameActions::Status::ok) {
   Check(H::FlatRides::Action(CoordsXY{x,y},true).error==GameActions::Status::ok,"Access placement failed");GameActions::ProcessQueue(getGameState());done=true;
  }Check(done,"No valid access side");
 }
 H::FlatRides::End();Check(W::MainOpen(),"Main viewport missing");
 const auto nativeCash=getGameState().park.cash;
 const auto points=H::RideAccess::Existing(target);Check(points.size()==2,"Native access missing");
 for(int i=0;i<2;++i) {
  const auto handoffCash=getGameState().park.cash;
  const auto point=H::RideAccess::Existing(target).at(i); // Refresh context after the preceding real landing changed nearby paths.
  W::HandheldRideOpen(target.id);Check(W::HandheldRideQuickAccess(),"Modern access shortcut missing");W::HandheldMenuMove(i);W::HandheldMenuConfirm();
  const auto model=W::HandheldRidesPresentation();Check(model.rows.size()==4&&model.rows[2]==(i?"Connect exit path...":"Connect entrance queue..."),"Point path handoff missing");
  W::HandheldMenuMove(2);
  if(i==0) {gOpenRCT2NoGraphics=false;capture("access-path-shortcut.ppm");gOpenRCT2NoGraphics=true;}
  if(i==0) {
   wm->CloseByClass(WindowClass::mainWindow);WindowCullDead();W::HandheldMenuConfirm();
   Check(W::HandheldRidesIsOpen()&&!H::Paths::IsActive(),"Failed path handoff closed inspection");
   Check(W::HandheldRidesPresentation().status.find("Cannot start path")!=std::string::npos,"Failed handoff feedback missing");
   Check(W::MainOpen(),"Restore viewport failed");
  }
  W::HandheldMenuConfirm();Check(H::Paths::IsConnected()&&H::Paths::IsQueue()==!i&&!W::HandheldRidesIsOpen(),"Point handoff lost connected path mode");
  Check(H::RideAccess::Current(point)&&getGameState().park.cash==handoffCash,"Path handoff mutated access/cash");
  if(i==0) {
   gOpenRCT2NoGraphics=false;capture("connected-queue-overlay.ppm",true);gOpenRCT2NoGraphics=true;
   const auto quote=H::Paths::Action(false);Check(quote.error==GameActions::Status::ok,"Guided native landing query failed");
   Check(H::Paths::Action(true).error==GameActions::Status::ok,"Guided native landing failed");GameActions::ProcessQueue(getGameState());
   const auto placedCash=getGameState().park.cash;Check(placedCash==nativeCash-quote.cost,"Guided native landing accounting changed");
   W::HandheldPathOptionsOpen();W::HandheldMenuMove(7);W::HandheldMenuConfirm();W::HandheldMenuBack();
   W::HandheldMenuMove(-1);W::HandheldMenuConfirm();Check(W::HandheldObjectPickerIsOpen(),"Undo review Back lost row focus");
   W::HandheldMenuBack();W::HandheldMenuBack();
   Check(getGameState().park.cash==placedCash,"Undo Cancel changed accounting");
  }
  H::EndTerrain();WindowCullDead();
 }
 std::cout<<"PASS native entrance/exit point shortcuts start connected queue/normal paths without changing access/cash\n";
 std::cout<<"PASS ordinary path settings, native surface grid and cancel preserve tool/mode/cash\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

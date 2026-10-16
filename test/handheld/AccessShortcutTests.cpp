// SPDX-License-Identifier: GPL-3.0-only
#include <SDL.h>
#include <iostream>
#include <array>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <openrct2/SpriteIds.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/drawing/Drawing.Sprite.h>
#include <openrct2/drawing/G1Element.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/management/Research.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <stdexcept>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/interface/WindowBase.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
using namespace OpenRCT2;
namespace W=Ui::Windows;
static void Check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(int argc,char** argv)
{
 try {
  if(argc!=3 && argc!=4)return 2;
  SDL_setenv("OPENRCT2_HANDHELD","1",1);gCustomOpenRCT2DataPath=argv[1];gCustomUserDataPath=argv[2];
  gOpenRCT2Headless=true;gOpenRCT2NoGraphics=true;
  auto env=CreatePlatformEnvironment();auto ui=Ui::CreateUiContext(*env);
  auto context=CreateContext(std::move(env),Audio::CreateDummyAudioContext(),std::move(ui));
  Config::SetDefaults();Config::Get().general.windowWidth=512;Config::Get().general.windowHeight=384;Config::Get().general.windowScale=1;
  Check(context->Initialise(),"Context failed");context->GetUiContext().CreateWindow();auto* wm=Ui::GetWindowManager();wm->CloseAllExceptFlags({});WindowCullDead();
  gameStateInitAll(getGameState(),{32,32});gLegacyScene=LegacyScene::playing;
  namespace H=Ui::Handheld;namespace F=H::FlatRides;namespace GA=GameActions;
  auto& state=getGameState();state.park.cash=10000000;
  for(int y=32;y<992;y+=32)for(int x=32;x<992;x+=32)
   MapGetSurfaceElementAt(CoordsXY{x,y})->setOwnership({OwnershipFlag::landOwned});
  auto& objects=context->GetObjectManager();
  Check(objects.LoadObject("rct2.terrain_surface.grass")&&objects.LoadObject("rct2.terrain_edge.rock")&&objects.LoadObject("rct2.station.plain"),"Terrain/station fixture missing");
  auto* object=objects.LoadObject("rct2.ride.mgr1");Check(object,"Carousel missing");
  const auto index=objects.GetLoadedObjectEntryIndex(object);
  ResearchInsertRideEntry(index,true);RideEntrySetInvented(index);RideTypeSetInvented(FlatRidePlacement::Type(index));
  Check(H::BeginFlatRide(index),"Native flat start failed");
  Check(F::Action(CoordsXY{640,640},true).error==GA::Status::ok,"Native footprint failed");GA::ProcessQueue(state);
  const auto target=*F::Target();F::End();
  auto snapshot=[&](){
   std::vector<uint8_t> bytes;
   for(int y=0;y<1024;y+=32)for(int x=0;x<1024;x+=32) {
    for(const auto* e:TileElementsView(CoordsXY{x,y}))
     bytes.insert(bytes.end(),reinterpret_cast<const uint8_t*>(e),reinterpret_cast<const uint8_t*>(e)+sizeof(TileElement));
   }
   return bytes;
  };
  const auto before=snapshot();const auto cash=state.park.cash;const auto station=GetRide(target.id)->getStation();
  Check(H::RideManagement::Presentation(H::RideManagement::View::inspection,0,{},target).rows.front()=="Place entrance / exit...","Placement buried in ride inspection");
  Check(H::RideAccess::Presentation(target,0).rows.front()=="Place entrance / exit...","Missing access path page has no placement handoff");
  W::HandheldRideOpen(target.id);W::HandheldMenuBack();WindowCullDead();
  Check(F::CurrentStage()==F::Stage::inactive,"Cancel began placement");
  W::HandheldRideOpen(target.id);W::HandheldMenuConfirm();WindowCullDead();
  Check(F::CurrentStage()==F::Stage::entrance && F::Target()==target,"Quick ride first action did not resume missing entrance");F::End();
  W::HandheldRideAccessOpen(target);W::HandheldMenuConfirm();WindowCullDead();
  Check(F::CurrentStage()==F::Stage::entrance,"Access page first action did not resume placement");F::End();
  W::HandheldRideAccessOpen(target);W::HandheldMenuMove(1);W::HandheldMenuConfirm();
  Check(W::HandheldRidesPresentation().rows.front()=="Place entrance / exit...","Inspection entry misplaced");
  W::HandheldMenuConfirm();WindowCullDead();
  Check(F::CurrentStage()==F::Stage::entrance,"Inspection first action did not resume placement");F::End();
  Check(snapshot()==before && state.park.cash==cash && std::memcmp(&station,&GetRide(target.id)->getStation(),sizeof(station))==0,"Navigation modified map/cash/station");
  // Moving past the new first action must still reach the original Close operation.
  W::HandheldRideOpen(target.id);W::HandheldMenuMove(3);W::HandheldMenuConfirm();GA::ProcessQueue(state);
  Check(GetRide(target.id)->status==RideStatus::closed && F::CurrentStage()==F::Stage::inactive,"Shortcut shifted Close into a placement action");
  wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
  if(argc==4) {
   std::filesystem::create_directories(argv[3]);
   gOpenRCT2NoGraphics=false;GfxLoadG2PalettesFontsTracks();Drawing::initColourMaps();FontSpriteInitialiseCharacters();
   auto capture=[&](const char* name) {
    Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();
    auto& rt=*engine.getRT();GfxClear(rt,Drawing::PaletteIndex::pi10);
    auto* window=wm->FindByClass(WindowClass::handheldMenu);Check(window,"Capture window missing");
    window->onDraw(rt);
    Drawing::GamePalette palette{};const auto* pal=GfxGetG1Palette(SPR_GAME_DEFAULT_PALETTE);
    for(int i=0;i<pal->numColours;i++)palette[pal->startIndex+i]={pal->palette[i].blue,pal->palette[i].green,pal->palette[i].red,255};
    std::ofstream out(std::filesystem::path(argv[3])/name,std::ios::binary);out<<"P6\n512 384\n255\n";
    for(int y=0;y<384;y++)for(int x=0;x<512;x++) {
     const auto c=palette[static_cast<unsigned>(rt.bits[y*rt.LineStride()+x])];
     const char rgb[]={static_cast<char>(c.red),static_cast<char>(c.green),static_cast<char>(c.blue)};out.write(rgb,3);
    }
    engine.EndDraw();
   };
   W::HandheldSceneryClearOptionsOpen();capture("clear-area.ppm");
   wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
   W::HandheldRideAccessOpen(target);W::HandheldMenuMove(1);W::HandheldMenuConfirm();capture("ride-access.ppm");
   wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
  }
  Check(F::Resume(target),"Native access recovery failed");
  bool placed=false;
  for(int y=544;y<800 && !placed;y+=32)for(int x=544;x<800 && !placed;x+=32) {
   if(F::Action(CoordsXY{x,y},false).error==GA::Status::ok) {
    Check(F::Action(CoordsXY{x,y},true).error==GA::Status::ok,"Native entrance execute failed");GA::ProcessQueue(state);placed=true;
   }
  }
  Check(placed && !GetRide(target.id)->getStation().entrance.isNull(),"Native entrance fixture missing");F::End();
  const auto entrance=GetRide(target.id)->getStation().entrance;const auto afterCash=state.park.cash;
  W::HandheldRideOpen(target.id);W::HandheldMenuConfirm();WindowCullDead();
  Check(F::CurrentStage()==F::Stage::exit && GetRide(target.id)->getStation().entrance==entrance && state.park.cash==afterCash,"Shortcut did not recover missing exit without changing the entrance/cash");F::End();
  std::cout<<"PASS native entrance commit then first-action missing-exit recovery\n";
  H::RideManagement::Invalidate(target.id);
  Check(!F::CanResume(target),"Stale allocation accepted");
  std::cout<<"PASS first-action quick ride / inspection / access handoffs; cancel and navigation preserve exact map/cash/station; Close routing and stale identity guard\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

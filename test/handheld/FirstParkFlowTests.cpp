// SPDX-License-Identifier: GPL-3.0-only
// Asset-backed title -> scenario -> paths -> shops -> flat ride interaction smoke.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <openrct2-ui/UiContext.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/Shops.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/ShopAppearance.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/ride/RideManager.hpp>
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
#include <openrct2/paint/Paint.h>
#include <openrct2/scenes/SceneManager.h>
#include <openrct2/scenes/title/TitleScene.h>
#include <openrct2/scenes/title/TitleSequencePlayer.h>
#include <openrct2/scenes/title/TitleSequenceManager.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
using namespace OpenRCT2;
namespace H=Ui::Handheld; namespace W=Ui::Windows;
static void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv)
{
 try {
  if(argc!=5)return 2;
  SDL_setenv("OPENRCT2_HANDHELD","1",1);SDL_setenv("SDL_VIDEODRIVER","offscreen",1);SDL_setenv("SDL_AUDIODRIVER","dummy",1);
  gCustomOpenRCT2DataPath=argv[1];gCustomUserDataPath=argv[2];gOpenRCT2Headless=true;gOpenRCT2NoGraphics=false;
  auto env=CreatePlatformEnvironment();env->SetBasePath(DirBase::rct2,argv[3]);auto ui=Ui::CreateUiContext(*env);
  auto context=CreateContext(std::move(env),Audio::CreateDummyAudioContext(),std::move(ui));Config::SetDefaults();
  Config::Get().general.rct2Path=argv[3];Config::Get().general.windowWidth=512;Config::Get().general.windowHeight=384;Config::Get().general.windowScale=1;
  Check(context->Initialise(),"Context initialization failed");context->GetUiContext().CreateWindow();
  auto* wm=Ui::GetWindowManager();const auto out=std::filesystem::path(argv[4]);std::filesystem::create_directories(out);
  auto capture=[&](const char* file){
   WindowCullDead();
   Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();auto& rt=*engine.getRT();
   if(auto* main=WindowGetMain())main->viewport->viewPos=main->savedViewPos;
   ResetAllSpriteQuadrantPlacements();Drawing::UpdatePaletteEffects();WindowDrawAll(rt,0,0,512,384);H::DrawExplorer(rt);

   Image image;image.Width=512;image.Height=384;image.Depth=8;image.Stride=rt.LineStride();image.Palette=Drawing::gPalette;
   const auto* bytes=reinterpret_cast<uint8_t*>(rt.bits);image.Pixels.assign(bytes,bytes+rt.LineStride()*rt.height);
   Imaging::WriteToFile((out/file).string(),image,ImageFormat::png);engine.EndDraw();
  };
  auto lookAt=[&](CoordsXYZ location){
   auto* main=WindowGetMain();main->savedViewPos=Translate3DTo2DWithZ(main->viewport->rotation,location)
      -ScreenCoordsXY{main->viewport->zoom.ApplyTo(410),main->viewport->zoom.ApplyTo(194)};
  };
  H::ControllerInput input;
  auto press=[&](H::Action a){H::Frame f;f.actions={a};H::HandleExplorerInput(input,f);WindowCullDead();};
  auto nav=[&](int x,int y){H::Frame f;f.navigateX=x;f.navigateY=y;H::HandleExplorerInput(input,f);};
  gOpenRCT2Headless=false;gOpenRCT2ShowChangelog=false;
  TitleSequenceManager::Scan();Check(TitleSequenceManager::GetCount()>0,"Title sequences not installed");
  auto* title=static_cast<TitleScene*>(context->GetSceneManager()->getTitleScene());title->Load();gOpenRCT2Headless=true;
  for(int i=0;i<5;++i)Check(title->GetSequencePlayer()->Update(),"Native title sequence failed to play");
  auto* menu=wm->FindByClass(WindowClass::titleMenu);Check(menu,"Title menu missing");
  Check(menu->windowPos==ScreenCoordsXY{0,0}&&menu->width==512&&menu->height==384,"Title menu outside logical screen after resize");
  Check(gLegacyScene==LegacyScene::titleSequence,"Title loaded as gameplay");
  capture("01-title.png");
  Check(H::LastControlHints.size()==3&&H::LastControlHints.back().label=="Exit","Title footer contains park controls");
  // A real software compositor redraws the title in clipped dirty rectangles.
  Drawing::X8DrawingEngine dirty(context->GetUiContext());dirty.Resize(512,384);dirty.BeginDraw();auto& rt=*dirty.getRT();
  WindowDrawAll(rt,0,0,512,384);std::vector<Drawing::PaletteIndex> full(rt.bits,rt.bits+rt.LineStride()*rt.height);
  for(int y=0;y<384;y+=32)for(int x=0;x<512;x+=64)WindowDrawAll(rt,x,y,std::min(x+64,512),std::min(y+32,384));
  Check(std::equal(full.begin(),full.end(),rt.bits),"Title dirty-region drawing differs from full redraw");dirty.EndDraw();
  press(H::Action::confirm);Check(wm->FindByClass(WindowClass::scenarioSelect),"New park did not open scenario selection");capture("02-new-park.png");
  press(H::Action::back);Check(!wm->FindByClass(WindowClass::scenarioSelect),"Scenario Back did not return home");
  press(H::Action::confirm);
  // Move through the native visible source tabs to the original RCT2 scenarios.
  for(int i=0;i<3;++i)press(H::Action::nextOption);
  capture("02b-rct2-scenarios.png");press(H::Action::confirm);
  Check(gLegacyScene==LegacyScene::playing&&WindowGetMain(),"Scenario confirmation did not start park");
  std::cout<<"Started native scenario: "<<getGameState().park.name<<'\n';
  wm->CloseByClass(WindowClass::parkInformation);wm->CloseByClass(WindowClass::error);WindowCullDead();InputSetState(InputState::normal);gGamePaused=0;
  capture("03-started-park.png");
  // Enter the production Build -> Paths menus without shortcuts or a mouse.
  W::HandheldBuildOpen();capture("04-build-menu.png");W::HandheldMenuMove(2);press(H::Action::confirm);capture("04b-path-mode-menu.png");press(H::Action::confirm);
  Check(H::Paths::IsActive()&&!wm->FindByClass(WindowClass::handheldMenu),"Build/Paths did not enter construction");
  auto& state=getGameState();const auto startingCash=state.park.cash;
  std::vector<CoordsXY> pathTiles;
  for(int y=32;y<state.mapSize.y*32-32&&pathTiles.empty();y+=32)for(int x=32;x<state.mapSize.x*32-96&&pathTiles.empty();x+=32){
   bool good=true;
   for(int i=0;i<3;++i){CoordsXY tile{x+i*32,y};auto* surface=MapGetSurfaceElementAt(tile);if(!surface){good=false;break;}
    InteractionInfo info;info.Loc=tile;info.Element=reinterpret_cast<TileElement*>(surface);info.interactionType=ViewportInteractionItem::terrain;
    H::Paths::UpdateTargetFromInfo(info);if(H::Paths::Action(false).error!=GameActions::Status::ok){good=false;break;}}
   if(good)for(int i=0;i<3;++i)pathTiles.push_back({x+i*32,y});
  }
  Check(pathTiles.size()==3,"No legal three-tile path location in scenario");
  money64 quoted=0;
  for(const auto tile:pathTiles){auto* surface=MapGetSurfaceElementAt(tile);InteractionInfo info;info.Loc=tile;info.Element=reinterpret_cast<TileElement*>(surface);info.interactionType=ViewportInteractionItem::terrain;H::Paths::UpdateTargetFromInfo(info);
   auto q=H::Paths::Action(false);Check(q.error==GameActions::Status::ok,"Path query failed");quoted+=q.cost;Check(H::Paths::Action(true).error==GameActions::Status::ok,"Path construction failed");GameActions::ProcessQueue(state);}
  Check(state.park.cash==startingCash-quoted,"Path charges differ from native quotes");
  const auto* pathSurface=MapGetSurfaceElementAt(pathTiles.front());lookAt({pathTiles.front().x+16,pathTiles.front().y+16,pathSurface->getBaseZ()});
  capture("05-paths.png");H::Paths::End();
  // Existing catalog/research eligibility supplies all objects; do not unlock objects.
  auto shops=H::Shops::Entries();std::stable_sort(shops.begin(),shops.end(),[](const auto& a,const auto& b){
   auto colour=[](const auto& e){const auto* entry=GetRideEntryByIndex(e.identity.index);return entry && !entry->flags.has(RideEntryFlag::disableColourTab) && GetRideTypeDescriptor(ShopPlacement::Type(e.identity.index)).flags.has(RtdFlag::hasTrackColourMain);};
   return colour(a)&&!colour(b);
  });Check(shops.size()>=2,"Scenario has too few researched shops");
  std::vector<RideId> vendors;
  for(size_t n=0;n<2;++n){
   const auto index=shops[n].identity.index;Check(H::SetShopSelection(index),"Shop catalog selection failed");
   std::optional<CoordsXY> place;
   for(int y=32;y<state.mapSize.y*32-32&&!place;y+=32)for(int x=32;x<state.mapSize.x*32-32&&!place;x+=32)
    if(H::Shops::Action(CoordsXY{x,y},index,0,false).error==GameActions::Status::ok)place=CoordsXY{x,y};
   Check(place.has_value(),"No legal shop position");std::vector<RideId> before;for(const auto& ride:RideManager(state))before.push_back(ride.id);
   Check(H::Shops::Action(place,index,0,true).error==GameActions::Status::ok,"Shop construction failed");GameActions::ProcessQueue(state);
   for(const auto& ride:RideManager(state))if(std::find(before.begin(),before.end(),ride.id)==before.end())vendors.push_back(ride.id);
   Check(vendors.size()==n+1,"Shop action did not create vendor");
  }
  Check(vendors.size()==2,"Placed vendors missing from management");
  const auto shopTarget=H::ShopManagement::Capture(vendors.front());Check(shopTarget.has_value(),"Shop inspection target missing");lookAt(shopTarget->location);
  W::HandheldRideOpen(vendors.front());capture("06-shop.png");W::HandheldMenuMove(4);press(H::Action::confirm);capture("07-shop-appearance.png");
  auto appearance=W::HandheldShopAppearancePresentation();
  Check(appearance.rows.size()>1,"Chosen shop has no appearance channels");{press(H::Action::confirm);auto colours=W::HandheldShopAppearancePresentation();
   Check(!colours.colourValues.empty(),"Shop colour swatches missing");{
    const auto original=colours.selected;nav(0,1);auto moved=W::HandheldShopAppearancePresentation();Check(moved.selected==H::MoveColourGrid(original,moved.rows.size(),0,1),"Shop D-pad did not follow swatch grid");capture("08-shop-colours.png");
    press(H::Action::confirm);capture("09-shop-colour-review.png");Check(W::HandheldShopAppearancePresentation().confirmation,"Colour review absent");press(H::Action::back);press(H::Action::back);
   }
  }
  press(H::Action::back);press(H::Action::back);
  auto flats=H::FlatRides::Entries();Check(!flats.empty(),"Scenario has no researched flat rides");
  Check(H::BeginFlatRide(flats.front().identity.index),"Flat ride catalog choice failed");
  std::optional<CoordsXY> location;
  for(int y=32;y<state.mapSize.y*32-32&&!location;y+=32)for(int x=32;x<state.mapSize.x*32-32&&!location;x+=32)
   if(H::FlatRides::Action(CoordsXY{x,y},false).error==GameActions::Status::ok)location=CoordsXY{x,y};
  Check(location.has_value(),"No legal flat ride position");Check(H::FlatRides::Action(location,true).error==GameActions::Status::ok,"Flat ride placement failed");GameActions::ProcessQueue(state);
  Check(H::FlatRides::CurrentStage()==H::FlatRides::Stage::entrance,"Flat ride did not advance to entrance placement");
  if(const auto flat=H::FlatRides::Target())if(const auto rideView=H::RideManagement::Location(*flat))lookAt(*rideView);
  capture("10-flat-entrance.png");
  const auto target=H::FlatRides::Target();Check(target.has_value(),"Flat ride target lost");
  for(int access=0;access<2;++access){
   std::optional<CoordsXY> point;
   for(int y=32;y<state.mapSize.y*32-32&&!point;y+=32)for(int x=32;x<state.mapSize.x*32-32&&!point;x+=32)
    if(H::FlatRides::Action(CoordsXY{x,y},false).error==GameActions::Status::ok)point=CoordsXY{x,y};
   Check(point.has_value(),"No legal ride access position");Check(H::FlatRides::Action(point,true).error==GameActions::Status::ok,"Ride access placement failed");GameActions::ProcessQueue(state);
   if(access==0){Check(H::FlatRides::CurrentStage()==H::FlatRides::Stage::exit,"Entrance did not advance to exit");capture("10b-flat-exit.png");}
  }
  Check(H::FlatRides::CurrentStage()==H::FlatRides::Stage::inactive,"Completed ride access left tool active");H::EndTerrain();W::HandheldRideOpen(target->id);capture("11-flat-management.png");
  for(int tab=0;tab<3;++tab)press(H::Action::nextOption);
  auto rideModel=W::HandheldRidesPresentation();auto appearanceRow=std::find(rideModel.rows.begin(),rideModel.rows.end(),"Appearance...");
  Check(appearanceRow!=rideModel.rows.end(),"Ride Settings lost appearance action");
  W::HandheldMenuMove(static_cast<int>(appearanceRow-rideModel.rows.begin())-static_cast<int>(rideModel.selected));press(H::Action::confirm);
  const auto settings=H::RideManagement::AppearanceSettings(*H::RideManagement::Resolve(*target));
  const auto channel=std::find_if(settings.begin(),settings.end(),[](const auto& setting){return H::RideManagement::IsColourSetting(setting);});
  Check(channel!=settings.end(),"Flat ride has no native colour channel");W::HandheldMenuMove(static_cast<int>(channel-settings.begin()));press(H::Action::confirm);
  rideModel=W::HandheldRidesPresentation();Check(rideModel.colourValues.size()==Drawing::kColourNumNormal && rideModel.rows==channel->labels,"Ride colour editor still uses numeric adjustment");
  const auto originalColour=channel->current;nav(1,0);capture("12-ride-colours.png");press(H::Action::back);
  const auto afterColours=H::RideManagement::AppearanceSettings(*H::RideManagement::Resolve(*target));
  Check(afterColours[channel-settings.begin()].current==originalColour,"Back applied ride colour draft");
  const auto park=(out/"first-park-flow.park").string();Check(ScenarioSave(state,park,{})!=0,"Native save failed");Check(context->LoadParkFromFile(park),"Native reload failed");
  gOpenRCT2Headless=false;title->Load();gOpenRCT2Headless=true;
  Check(gLegacyScene==LegacyScene::titleSequence,"Return to title retained park scene");capture("13-title-after-park.png");
  Check(!wm->FindByClass(WindowClass::handheldMenu) && H::LastControlHints.size()==3
      && H::LastControlHints.back().label=="Exit","Return to title retained gameplay controls");
  std::cout<<"PASS native title/back/new scenario, dirty redraw, paths and accounting, two vendors, shop palette review/cancel, flat placement/entrance/exit/management return, save/reload\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}

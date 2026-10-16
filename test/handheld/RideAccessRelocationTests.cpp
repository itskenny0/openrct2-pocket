// SPDX-License-Identifier: GPL-3.0-only
#include <SDL.h>
#include <iostream>
#include <array>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/handheld/AccessPreview.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2/actions/ride/RideEntranceExitPlaceAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/core/DataSerialiser.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/ride/AccessPlacementContext.h>
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
  gameStateInitAll(getGameState(),{64,64});gLegacyScene=LegacyScene::playing;
  namespace H=Ui::Handheld;namespace F=H::FlatRides;namespace GA=GameActions;namespace RA=H::RideAccess;
  auto& state=getGameState();state.park.cash=10000000;
  for(int y=32;y<2016;y+=32)for(int x=32;x<2016;x+=32)
   MapGetSurfaceElementAt(CoordsXY{x,y})->setOwnership({OwnershipFlag::landOwned});
  auto& objects=context->GetObjectManager();
  Check(objects.LoadObject("rct2.terrain_surface.grass")&&objects.LoadObject("rct2.terrain_edge.rock")&&objects.LoadObject("rct2.station.plain"),"Terrain/station missing");
  auto load=[&](const char* name,ride_type_t type){auto* o=objects.LoadObject(name);Check(o,"Ride object missing");auto i=objects.GetLoadedObjectEntryIndex(o);ResearchInsertRideEntry(i,true);RideEntrySetInvented(i);RideTypeSetInvented(type);return i;};
  auto snapshot=[&](){std::vector<uint8_t> bytes;
   for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32){for(const auto* e:TileElementsView(CoordsXY{x,y}))if(!e->isGhost()){
    auto copy=*e;copy.setLastForTile(false);auto* p=reinterpret_cast<const uint8_t*>(&copy);bytes.insert(bytes.end(),p,p+sizeof(copy));}}
   return bytes;};
  auto ghosts=[&](){size_t n=0;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32)for(const auto* e:TileElementsView(CoordsXY{x,y}))n+=e->isGhost();return n;};
  auto sides=[&](const H::RideManagement::Target& t,StationIndex station){std::vector<CoordsXYZD> result;
   const auto* ride=H::RideManagement::Resolve(t);Check(ride,"Stale fixture");
   auto centre=ride->getStation(station).start;
   // Native maze station.start is the {0,0} sentinel, not its world location.
   if(ride->type==RIDE_TYPE_MAZE){bool found=false;for(int y=32;y<2016 && !found;y+=32)for(int x=32;x<2016 && !found;x+=32)
    for(const auto* e:TileElementsView<TrackElement>(CoordsXY{x,y}))if(!e->isGhost()&&e->getRideIndex()==t.id){centre={x,y};found=true;break;}}
   for(int y=centre.y-160;y<=centre.y+160;y+=32)for(int x=centre.x-160;x<=centre.x+160;x+=32){
    auto loc=FlatRidePlacement::EntranceLocation(*ride,{x,y},station);if(!loc)continue;
    GA::RideEntranceExitPlaceAction q(*loc,loc->direction,t.id,station,false);q.SetPureGhost();q.SetFlags({GA::CommandFlag::ghost,GA::CommandFlag::noSpend});
    if(GA::Query(&q,state).error==GA::Status::ok)result.push_back(*loc);
   }return result;};
  auto* fixturePath=objects.LoadObject("rct2.footpath_surface.tarmac");auto* fixtureRail=objects.LoadObject("rct2.footpath_railings.wood");Check(fixturePath&&fixtureRail,"Path metadata missing");
  auto complete=[&](const H::RideManagement::Target& t){Check(F::Resume(t),"Missing access return failed");
   for(int n=0;n<2;n++){bool done=false;for(auto loc:sides(t,StationIndex::FromUnderlying(0))){if(F::Action(loc,false).error==GA::Status::ok){Check(F::Action(loc,true).error==GA::Status::ok,"Native access failed");GA::ProcessQueue(state);done=true;break;}}Check(done,"No native access side");}F::End();
   for(const auto& point:RA::Existing(t)) {
    auto outside=CoordsXY(point.source)-CoordsDirectionDelta[point.source.direction];
    GA::FootpathPlaceAction path({outside,point.source.z},{},objects.GetLoadedObjectEntryIndex(fixturePath),objects.GetLoadedObjectEntryIndex(fixtureRail),kInvalidDirection,point.exit ? 0 : PathConstructFlag::IsQueue);
    Check(GA::Execute(&path,state).error==GA::Status::ok,"Source queue/path fixture failed");GA::ProcessQueue(state);
   }
  };
  auto carousel=load("rct2.ride.mgr1",RIDE_TYPE_MERRY_GO_ROUND);Check(H::BeginFlatRide(carousel),"Flat start failed");
  Check(F::Action(CoordsXY{640,640},true).error==GA::Status::ok,"Flat footprint failed");GA::ProcessQueue(state);auto target=*F::Target();complete(target);
  auto exercise=[&](H::RideManagement::Target t,bool isExit){
   auto points=RA::Existing(t);Check(points.size()==2,"Existing access inspection incomplete");auto point=points[isExit ? 1 : 0];
   auto before=snapshot();auto cash=state.park.cash;auto station=GetRide(t.id)->getStation(point.station);auto mazeCount=GetRide(t.id)->mazeTiles;
   auto destinations=sides(t,point.station);Check(!destinations.empty(),"Relocation side missing");auto dest=destinations.back();std::string error;
   auto draft=RA::Review(point,dest,error);Check(draft.has_value(),error.c_str());
   Check(snapshot()==before && cash==state.park.cash && std::memcmp(&station,&GetRide(t.id)->getStation(point.station),sizeof(station))==0,"Inspection/query mutated native state");
   Check(RA::BeginRelocation(point),"Relocation start failed");F::UpdatePreview(dest);
   Check(ghosts()==1 && snapshot()==before && cash==state.park.cash && std::memcmp(&station,&GetRide(t.id)->getStation(point.station),sizeof(station))==0,"Relocation ghost changed source/station/path/maze/cash");
   Ui::ProvisionalElementsRemove();Ui::ProvisionalElementsRemove();Check(ghosts()==0,"Nested suspension retained ghost");Ui::ProvisionalElementsRestore();Check(ghosts()==0,"Nested restoration early");Ui::ProvisionalElementsRestore();Check(ghosts()==1,"Nested restoration missing");
   SaveFlags flags;flags.set(SaveFlag::automatic);auto save=std::filesystem::path(argv[2])/("preview-"+std::to_string(t.id.ToUnderlying())+".park");
   Check(ScenarioSave(state,save.string(),flags)!=0 && snapshot()==before && ghosts()==1,"Autosave leaked/failed preview restoration");
   Check(F::Action(dest,true).error==GA::Status::ok,"Move review missing");auto m=W::HandheldRidesPresentation();Check(m.selected==1 && m.rows[1]=="Cancel","Move review not Cancel-first");
   SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");Check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0,"SDL init failed");
   H::ControllerInput controller;controller.update(true,1000);int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);Check(device>=0,"Virtual controller missing");
   auto* joystick=SDL_JoystickOpen(device);Check(joystick,"Virtual joystick missing");auto pump=[&](){SDL_Event e;while(SDL_PollEvent(&e))controller.handleEvent(e);};pump();controller.update(true,1010);pump();controller.update(true,1020);
   Check(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1)==0,"Held A setup failed");SDL_JoystickUpdate();pump();H::HandleExplorerInput(controller,controller.update(true,1030));WindowCullDead();
   Check(controller.update(true,1800).actions.empty(),"Held A repeated");
   SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);SDL_JoystickUpdate();pump();controller.update(true,1810);SDL_JoystickClose(joystick);SDL_JoystickDetachVirtual(device);
   Check(snapshot()==before && cash==state.park.cash && !W::HandheldRidesIsOpen(),"Held A cancellation changed state or retained review");
   F::ClearPreview();
   DataSerialiser encoded(true);draft->action.Serialise(encoded);encoded.getStream().SetPosition(0);GA::FlatRideEntrancePlaceAction decoded;DataSerialiser input(false,encoded.getStream());decoded.Serialise(input);
   Check(decoded.Query(state,state.park).error==GA::Status::ok,"Serialized edit guard changed");
   auto* object=objects.GetLoadedObject<RideObject>(GetRide(t.id)->subtype);auto version=object->GetVersion();auto changed=version;++std::get<2>(changed);object->SetVersion(changed);
   Check(decoded.Execute(state,state.park).error!=GA::Status::ok && snapshot()==before,"Changed descriptor executed");object->SetVersion(version);
   auto badQuote=draft->action;badQuote.GuardContext(AccessPlacementContext::Capture(*GetRide(t.id),dest),draft->cost+1);
   Check(badQuote.Execute(state,state.park).error!=GA::Status::ok && snapshot()==before,"Changed quote executed");
   // Actual UI Apply, followed by repeated Confirm while pending; one native edit only.
   W::HandheldAccessReviewOpen(*draft);W::HandheldMenuMove(-1);W::HandheldMenuConfirm();W::HandheldMenuConfirm();GA::ProcessQueue(state);
   Check((isExit ? GetRide(t.id)->getStation(point.station).exit : GetRide(t.id)->getStation(point.station).entrance).toCoordsXYZD()==dest && state.park.cash==cash-draft->cost && GetRide(t.id)->mazeTiles==mazeCount,"Native relocation accounting/reference/count differs");
   const auto outside=CoordsXY(point.source)-CoordsDirectionDelta[point.source.direction];
   bool detached=false;
   for(const auto* path:TileElementsView<PathElement>(outside))if(!path->isGhost() && path->getBaseZ()==point.source.z)
    detached=!(path->getEdges() & (1 << point.source.direction));
   Check(detached,"Native move retained old source path edge");
   wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();H::EndTerrain();
   auto moved=RA::Existing(t)[isExit ? 1 : 0];auto removal=RA::Review(moved,{},error);Check(removal.has_value(),"Removal quote missing");
   before=snapshot();cash=state.park.cash;W::HandheldAccessReviewOpen(*removal);W::HandheldMenuConfirm();WindowCullDead();Check(snapshot()==before && state.park.cash==cash,"Removal Cancel changed map/cash");
   W::HandheldAccessReviewOpen(*removal);W::HandheldMenuMove(-1);W::HandheldMenuConfirm();W::HandheldMenuConfirm();GA::ProcessQueue(state);
   Check((isExit ? GetRide(t.id)->getStation(point.station).exit : GetRide(t.id)->getStation(point.station).entrance).isNull() && !(isExit ? GetRide(t.id)->getStation(point.station).entrance : GetRide(t.id)->getStation(point.station).exit).isNull() && state.park.cash==cash-removal->cost,"Native explicit removal differs");
   wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();Check(F::Resume(t),"Native gap recovery failed");
   bool restored=false;for(auto loc:sides(t,point.station))if(F::Action(loc,false).error==GA::Status::ok){Check(F::Action(loc,true).error==GA::Status::ok,"Native gap rebuild failed");GA::ProcessQueue(state);restored=true;break;}Check(restored,"No rebuild side");F::End();
   std::cout<<"PASS family "<<GetRide(t.id)->type<<(isExit ? " exit" : " entrance")<<" inspection/ghost/nested autosave/cancel/serialized stale descriptor/quote/one Apply/native move/remove/accounting/gap recovery\n";
  };
  exercise(target,false);exercise(target,true);
  auto mazeIndex=load("rct2.ride.hmaze",RIDE_TYPE_MAZE);Check(H::BeginMaze(mazeIndex),"Maze begin failed");GA::ProcessQueue(state);H::Mazes::Update(CoordsXY{960,960});Check(H::Mazes::Build(true).error==GA::Status::ok,"Maze build failed");GA::ProcessQueue(state);auto maze=*H::Mazes::Target();H::Mazes::End();complete(maze);exercise(maze,false);exercise(maze,true);
  auto mono=load("rct2.ride.mono1",RIDE_TYPE_MONORAIL);Check(H::BeginCustomTrack(mono,RIDE_TYPE_MONORAIL),"Monorail begin failed");GA::ProcessQueue(state);H::CustomTracks::Update(CoordsXY{1440,1440});
  for(int i=0;i<3;i++){Check(H::CustomTracks::Select(TrackElemType::endStation)&&H::CustomTracks::Action(true).error==GA::Status::ok,"Monorail station failed");GA::ProcessQueue(state);}auto conventional=*H::CustomTracks::Target();H::CustomTracks::End();complete(conventional);exercise(conventional,false);exercise(conventional,true);
  // Real open-ride native eligibility rejection; no implicit close.
  auto point=RA::Existing(target).front();std::string blockedError;auto blocked=RA::Review(point,{},blockedError);Check(blocked.has_value(),"Closed removal missing");
  GA::RideSetStatusAction open(target.id,RideStatus::open);Check(GA::Execute(&open,state).error==GA::Status::ok,"Carousel Open failed");GA::ProcessQueue(state);
  auto unchanged=snapshot();auto cash=state.park.cash;Check(blocked->action.Execute(state,state.park).error!=GA::Status::ok && GetRide(target.id)->status==RideStatus::open && snapshot()==unchanged && state.park.cash==cash,"Open ride edited/closed implicitly");
  GA::RideSetStatusAction closeRide(target.id,RideStatus::closed);Check(GA::Execute(&closeRide,state).error==GA::Status::ok,"Close failed");GA::ProcessQueue(state);
  std::string error;point=RA::Existing(target).front();auto captureDraft=RA::Review(point,sides(target,point.station).back(),error);Check(captureDraft.has_value(),"Capture draft missing");
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
   W::HandheldRideOpen(target.id);W::HandheldMenuConfirm();capture("access-list.ppm");W::HandheldMenuConfirm();capture("access-inspection.ppm");
   wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
   W::HandheldAccessReviewOpen(*captureDraft);capture("move-review.ppm");
   wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
   auto removeCapture=RA::Review(point,{},error);Check(removeCapture.has_value(),"Removal capture missing");
   W::HandheldAccessReviewOpen(*removeCapture);capture("remove-review.ppm");
   wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
   auto staleCapture=*captureDraft;staleCapture.action.GuardContext(AccessPlacementContext::Capture(*GetRide(target.id),staleCapture.destination),staleCapture.cost+1);
   W::HandheldAccessReviewOpen(staleCapture);W::HandheldMenuMove(-1);W::HandheldMenuConfirm();capture("stale-review.ppm");
   wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
  }
  H::EndTerrain();
  // Demonstrate audited native remove-before-build failure versus guarded execution.
  point=RA::Existing(target).front();auto destination=sides(target,point.station).back();auto guarded=RA::Review(point,destination,error);Check(guarded.has_value(),"Failure fixture quote missing");
  auto* pathObject=objects.LoadObject("rct2.footpath_surface.tarmac");auto* rail=objects.LoadObject("rct2.footpath_railings.wood");Check(pathObject&&rail,"Path objects missing");
  GA::FootpathPlaceAction obstruct({CoordsXY(destination),destination.z},{},objects.GetLoadedObjectEntryIndex(pathObject),objects.GetLoadedObjectEntryIndex(rail));Check(GA::Execute(&obstruct,state).error==GA::Status::ok,"Native target obstruction failed");GA::ProcessQueue(state);
  auto obstructed=snapshot();auto obstructionCash=state.park.cash;
  Check(guarded->action.Execute(state,state.park).error!=GA::Status::ok && snapshot()==obstructed && state.park.cash==obstructionCash && GetRide(target.id)->getStation().entrance.toCoordsXYZD()==point.source,"Changed target bypassed guard or removed source");
  W::HandheldAccessReviewOpen(*guarded);W::HandheldMenuMove(-1);W::HandheldMenuConfirm();
  Check(snapshot()==obstructed && !W::HandheldRidesPresentation().status.empty(),"Stale UI review lost source or failure feedback");
  W::HandheldMenuBack();WindowCullDead();
  GA::RideEntranceExitPlaceAction unguarded(destination,destination.direction,target.id,point.station,point.exit);
  Check(unguarded.Execute(state,state.park).error!=GA::Status::ok && GetRide(target.id)->getStation().entrance.isNull(),"Native late-failure ordering differs from audit");
  Check(F::Resume(target),"Native late gap was not recoverable");bool rebuilt=false;
  for(auto loc:sides(target,point.station)) { if(F::Action(loc,false).error==GA::Status::ok){Check(F::Action(loc,true).error==GA::Status::ok,"Late gap native recovery failed");GA::ProcessQueue(state);rebuilt=true;break;} }
  Check(rebuilt,"Late gap has no native side");F::End();
  std::cout<<"PASS changed target preserves source through guarded action; direct native Execute demonstrates sequential gap and native recovery\n";
  point=RA::Existing(target).front();auto pendingDraft=RA::Review(point,sides(target,point.station).back(),error);Check(pendingDraft.has_value(),"Pending fixture missing");
  const auto beforePending=snapshot();W::HandheldAccessReviewOpen(*pendingDraft);W::HandheldMenuMove(-1);W::HandheldMenuConfirm();
  Check(snapshot()==beforePending,"UI action did not use native pending queue");
  wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
  // Deliberately collide request numbers in a new allocation of the menu window.
  W::HandheldAccessReviewOpen(*pendingDraft);W::HandheldAccessReviewOpen(*pendingDraft);
  if(gGamePaused & GAME_PAUSED_NORMAL) PauseToggle(); // The new inspection menu acquired a fresh pause.
  GA::ProcessQueue(state);
  Check(W::HandheldRidesPresentation().rows.front()=="Apply native edit","Old completion callback changed a new review window");
  Check(GetRide(target.id)->getStation().entrance.toCoordsXYZD()==pendingDraft->destination,"Authorized pending edit did not complete");
  wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
  point=RA::Existing(target).front();auto loadDestination=sides(target,point.station).back();
  Check(RA::BeginRelocation(point),"Load-failure relocation start failed");F::UpdatePreview(loadDestination);Check(ghosts()==1,"Load-failure preview missing");
  auto earlyMap=snapshot();auto earlyCash=state.park.cash;H::EndTerrain();
  Check(!context->LoadParkFromFile((std::filesystem::path(argv[2])/"absent.park").string()) && ghosts()==0 && snapshot()==earlyMap && state.park.cash==earlyCash,"Early failed native load changed real access or retained preview");
  wm->CloseByClass(WindowClass::error);WindowCullDead();
  const auto finalMap=snapshot();const auto finalCash=state.park.cash;auto finalSave=std::filesystem::path(argv[2])/"edited-access.park";
  Check(ScenarioSave(state,finalSave.string(),{})!=0,"Final save failed");
  auto queuedAfterLoad=RA::Review(point,loadDestination,error);Check(queuedAfterLoad.has_value(),"Load pending guard missing");GA::Enqueue(&queuedAfterLoad->action,state.currentTicks);
  Check(context->LoadParkFromFile(finalSave.string()),"Native reload failed");GA::ProcessQueue(state);
  Check(ghosts()==0 && snapshot()==finalMap && state.park.cash==finalCash && F::CurrentStage()==F::Stage::inactive,"Native reload retained ghost/draft or changed real map/cash");
  Check(!RA::Current(point),"Reload accepted old allocation identity");
  std::cout<<"PASS native Open eligibility, final .park map/cash/access round-trip and stale-load identity\n";
  auto currentTarget=H::RideManagement::Identify(target.id);Check(currentTarget.has_value(),"Reload target missing");
  auto unloadPoint=RA::Existing(*currentTarget).front();Check(RA::BeginRelocation(unloadPoint),"Unload recovery start failed");F::UpdatePreview(sides(*currentTarget,unloadPoint.station).back());
  context.reset();Check(F::CurrentStage()==F::Stage::inactive,"Pre-unload retained relocation draft");
  std::cout<<"PASS old-window completion isolation, early load preservation, successful-load pending queue cleanup and pre-unload\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

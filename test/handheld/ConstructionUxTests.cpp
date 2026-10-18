// SPDX-License-Identifier: GPL-3.0-only
#include <SDL.h>
#include <openrct2/Input.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include "RideTabNavigation.h"
#include <openrct2-ui/handheld/RideTabs.h>
#include <openrct2-ui/handheld/ConstructionAdjust.h>
#include <openrct2-ui/handheld/TerrainMaterials.h>
#include <openrct2/world/Scenery.h>
#include <openrct2-ui/interface/Window.h>
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
#include <openrct2-ui/handheld/Dashboard.h>
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
     H::DrawExplorer(rt);
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

 Check(W::MainOpen(),"Main viewport missing");WindowGetMain()->onResize();InputSetState(InputState::normal);
 for(auto name:{"rct2.terrain_surface.grass","rct2.terrain_surface.sand","rct2.terrain_edge.rock","rct2.terrain_edge.wood_red"})
  Check(objects.LoadObject(name),"Terrain material missing");
 auto* wall=objects.LoadObject("rct2.scenery_wall.wbrg");Check(wall,"Wall missing");
 const auto wallIndex=objects.GetLoadedObjectEntryIndex(wall);ScenerySetInvented({SCENERY_TYPE_WALL,wallIndex});
 auto* roof=objects.LoadObject("rct2.scenery_small.corroof");Check(roof,"Roof missing");
 const auto roofIndex=objects.GetLoadedObjectEntryIndex(roof);ScenerySetInvented({SCENERY_TYPE_SMALL,roofIndex});
 namespace S=H::SceneryTools;
 H::ControllerInput input;
 auto press=[&](H::Action action){H::Frame f;f.actions={action,action};H::HandleExplorerInput(input,f);};
 auto move=[&](int x,int y){H::Frame f;f.navigateX=x;f.navigateY=y;f.panX=x*8;f.panY=y*8;H::HandleExplorerInput(input,f);};
 S::SelectCategory(S::Category::walls);Check(S::Select(wallIndex),"Wall select failed");H::BeginScenery();
 W::HandheldSceneryCatalogOpen();capture("01-scenery-catalog.ppm");press(H::Action::back);WindowCullDead();
 Check(!wm->FindByClass(WindowClass::handheldMenu)&&H::IsSceneryToolActive(),"Catalog cancel entered nested settings");
 const auto oldDirection=S::Current().direction;press(H::Action::openBuild);
 Check(S::Current().direction==(oldDirection+1)%4,"Live Select failed to rotate once");
 capture("02-scenery-build.ppm",true);
 const auto q=S::Place(CoordsXY{640,640},S::Current(),false);Check(q.error==GameActions::Status::ok,"Native wall query failed");
 Check(S::Place(CoordsXY{640,640},S::Current(),true).error==GameActions::Status::ok,"Native wall action failed");GameActions::ProcessQueue(getGameState());
 Check(getGameState().park.cash==cash-q.cost,"Native wall cost changed");const auto navigationCash=getGameState().park.cash;
 const auto view=WindowGetMain()->viewport->viewPos;
 press(H::Action::contextSecondary);move(1,0);
 Check(S::Current().direction==(oldDirection+2)%4&&WindowGetMain()->viewport->viewPos.x==view.x,"Adjust direction panned or failed");capture("03a-scenery-orientation.ppm",true);
 press(H::Action::nextOption);
 const auto colours=S::Current().colours;move(1,0);
 Check(S::Current().colours[0]!=colours[0],"Colour shortcut failed");capture("03-scenery-colour.ppm",true);
 const auto beforePicker=S::Current().colours[0];
 press(H::Action::confirm);move(0,1);capture("03b-scenery-palette.ppm",true);
 Check(S::Current().colours[0]==beforePicker,"Palette navigation applied scenery colour");
 press(H::Action::back);Check(S::Current().colours[0]==beforePicker,"Palette cancel changed scenery colour");
 press(H::Action::confirm);move(0,1);press(H::Action::confirm);
 const auto wantedIndex=H::MoveColourGrid(H::ConstructionAdjust::ColourIndex(beforePicker),72,0,1);
 Check(S::Current().colours[0]==W::ColourDropDownIndexToColour(wantedIndex),"Palette applied wrong native scenery colour");
 for (int colour=0;colour<72;++colour) {
  const auto index=H::ConstructionAdjust::ColourIndex(S::Current().colours[0]);
  S::Configure(7,1);Check(S::Current().colours[0]==W::ColourDropDownIndexToColour((index+1)%72),"Normal palette cycle omitted or included a colour");
 }
 Check(getGameState().park.cash==navigationCash&&WindowGetMain()->viewport->viewPos.x==view.x,"Palette changed park or camera");
 press(H::Action::contextPrimary);Check(wm->FindByClass(WindowClass::handheldMenu),"More missing");press(H::Action::back);WindowCullDead();
 move(1,0);Check(S::Current().colours[0]!=colours[0],"More lost Colour group");
 press(H::Action::openBuild);capture("04-catalog-return.ppm");press(H::Action::back);WindowCullDead();
 press(H::Action::nextOption);move(-1,0);capture("04a-scenery-mode.ppm",true);Check(S::Current().mode==S::Mode::pick,"Mode shortcut did not enter eyedropper");
 press(H::Action::confirm);capture("05-eyedropper.ppm",true);
 Check(getGameState().park.cash==navigationCash,"Adjust A bought scenery");
 press(H::Action::contextSecondary);press(H::Action::previousOption);press(H::Action::previousOption);
 move(-1,0); // Object direction changes; independent group focus remains bounded.
 press(H::Action::back);H::EndTerrain();
 S::SelectCategory(S::Category::small);Check(S::Select(roofIndex),"Small scenery select failed");H::BeginScenery();
 press(H::Action::contextSecondary);press(H::Action::previousOption);move(1,0);
 Check(S::Current().scatter,"Scatter shortcut failed");capture("06-scatter-adjust.ppm",true);
 move(0,1);move(1,0);Check(S::Current().scatterSize==2,"Scatter size shortcut failed");
 press(H::Action::back);H::EndTerrain();
 H::AreaTools::settings={};H::BeginAreaTools();
 press(H::Action::contextSecondary);move(0,1);move(1,0);move(0,1);move(1,0);
 Check(H::AreaTools::settings.width==2&&H::AreaTools::settings.height==2,"Direct brush dimensions failed");
 capture("07-terrain-brush.ppm",true);
 press(H::Action::nextOption);press(H::Action::nextOption);
 press(H::Action::confirm);Check(W::HandheldObjectPickerIsOpen(),"Surface picker missing");move(1,0);capture("08-terrain-surfaces.ppm");
 const auto selectedMaterial=W::HandheldObjectPickerFocus();
 H::AreaTools::settings.edgeColour=Drawing::Colour::yellow;
 const auto preservedEdgeColour=H::AreaTools::settings.edgeColour;press(H::Action::confirm);WindowCullDead();
 Check(H::AreaTools::settings.surface==selectedMaterial&&H::AreaTools::settings.operation==H::AreaTools::Operation::paint,"Material did not select native paint draft");
 Check(H::AreaTools::settings.edgeColour==preservedEdgeColour,"Surface selection reset the other material colour");
 move(0,1);press(H::Action::confirm);Check(W::HandheldObjectPickerIsOpen(),"Edge picker missing");move(1,0);capture("09-terrain-edges.ppm");
 const auto oldEdge=H::AreaTools::settings.edge;press(H::Action::back);WindowCullDead();Check(H::AreaTools::settings.edge==oldEdge,"Edge cancel changed selection");
 press(H::Action::confirm);move(1,0);const auto edgeChoice=W::HandheldObjectPickerFocus();press(H::Action::confirm);WindowCullDead();
 Check(H::AreaTools::settings.edge==edgeChoice&&edgeChoice!=kObjectEntryIndexNull,"Native edge selection failed");
 Check(!H::TerrainMaterials::Select(false,{ObjectType::terrainSurface,selectedMaterial,"stale"}),"Changed material identity accepted");capture("09a-terrain-paint-adjust.ppm",true);
 const auto preservedEdge=H::AreaTools::settings.edge;
 const auto preservedColour=H::AreaTools::settings.edgeColour;
 auto* colouredEdge=objects.LoadObject("official.terrain_edge.stucco");Check(colouredEdge,"Colourable edge fixture missing");
 H::AreaTools::settings.edge=objects.GetLoadedObjectEntryIndex(colouredEdge);
 Check(H::AreaTools::EdgeColourSupported(H::AreaTools::settings.edge),"Fixture edge has no colour channel");
 move(0,1);move(0,1);const auto terrainColour=H::AreaTools::settings.edgeColour;
 press(H::Action::confirm);move(0,1);capture("09b-terrain-palette.ppm",true);
 Check(H::AreaTools::settings.edgeColour==terrainColour,"Terrain palette navigation changed tool colour");
 press(H::Action::back);Check(H::AreaTools::settings.edgeColour==terrainColour,"Terrain palette cancel changed colour");
 press(H::Action::confirm);move(0,1);press(H::Action::confirm);
 const auto edgeIndex=H::MoveColourGrid(H::ConstructionAdjust::ColourIndex(terrainColour),72,0,1);
 Check(H::AreaTools::settings.edgeColour==W::ColourDropDownIndexToColour(edgeIndex),"Terrain palette applied wrong native colour");
 Check(getGameState().park.cash==navigationCash,"Terrain palette changed accounting");
 H::AreaTools::settings.edge=preservedEdge;H::AreaTools::settings.edgeColour=preservedColour;move(0,-1);move(0,-1);
 press(H::Action::contextPrimary);press(H::Action::back);WindowCullDead();
 press(H::Action::back);capture("10-terrain-build.ppm",true);
 press(H::Action::slopeUp);Check(H::AreaTools::settings.operation==H::AreaTools::Operation::raise,"Direct operation switch failed");
 Check(getGameState().park.cash==navigationCash,"Navigation changed native accounting");
 const auto terrainBefore=MapGetSurfaceElementAt(CoordsXY{960,960})->getBaseZ();
 const auto terrainQuote=H::AreaTools::Action(CoordsXY{960,960},H::AreaTools::settings,false);
 Check(terrainQuote.error==GameActions::Status::ok,"Native brush query failed");
 Check(H::AreaTools::Action(CoordsXY{960,960},H::AreaTools::settings,true).error==GameActions::Status::ok,"Native brush action failed");GameActions::ProcessQueue(getGameState());
 Check(getGameState().park.cash==navigationCash-terrainQuote.cost&&MapGetSurfaceElementAt(CoordsXY{960,960})->getBaseZ()>terrainBefore,"Native terrain/cost not preserved");
 press(H::Action::back);Check(!wm->FindByClass(WindowClass::handheldMenu),"Build Back opened an unexpected menu");
 H::BeginWater(true);press(H::Action::slopeDown);press(H::Action::contextSecondary);
 Check(H::AreaTools::settings.operation==H::AreaTools::Operation::waterLower,"Quick water trigger/promotion lost current operation");
 press(H::Action::back);H::EndTerrain();
 H::BeginSurface();press(H::Action::openBuild);
 Check(W::HandheldObjectPickerIsOpen(),"Quick surface Select did not reach material picker");press(H::Action::back);WindowCullDead();H::EndTerrain();
 // A real mapped controller must release Y before another transition is armed.
 H::BeginScenery();
 SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
 Check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0,"SDL controller init failed");
 {
  H::ControllerInput mapped;mapped.update(true,9000);
  const auto device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
  auto* joystick=SDL_JoystickOpen(device);Check(joystick,"Virtual controller missing");
  auto pump=[&](){SDL_Event event;while(SDL_PollEvent(&event))mapped.handleEvent(event);};
  pump();mapped.update(true,9010);pump();mapped.update(true,9020);
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_Y,true);SDL_JoystickUpdate();pump();
  auto frame=mapped.update(true,9030);Check(frame.actions.size()==1,"Mapped Y missing");H::HandleExplorerInput(mapped,frame);
  Check(mapped.update(true,9040).actions.empty()&&mapped.update(true,10040).actions.empty(),"Held Y escaped Adjust");
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_Y,false);SDL_JoystickUpdate();pump();mapped.update(true,10050);
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_B,true);SDL_JoystickUpdate();pump();
  H::HandleExplorerInput(mapped,mapped.update(true,10060));Check(H::IsSceneryToolActive(),"Mapped Adjust B ended tool");
  Check(mapped.update(true,10100).actions.empty()&&H::IsSceneryToolActive(),"Held B ended tool after return");
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_B,false);SDL_JoystickUpdate();pump();mapped.update(true,10110);
  SDL_JoystickClose(joystick);SDL_JoystickDetachVirtual(device);
 }
 H::EndTerrain();
 std::cout<<"PASS scenery direct rotation/colours/modes/scatter; catalog/More return; no Adjust placement or viewport pan\n";
 std::cout<<"PASS terrain brush/material/edge/operation controls; cancel; native wall and area action exact costs\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#include <openrct2-ui/handheld/AccessPreview.h>
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2/actions/ride/MazeSetTrackAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/actions/general/PauseToggleAction.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2/ride/AccessPlacementContext.h>
#include <openrct2/world/tile_element/EntranceElement.h>
#include <openrct2/actions/ride/FlatRideEntrancePlaceAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
// SPDX-License-Identifier: GPL-3.0-only
#include <openrct2-ui/handheld/ShopAppearance.h>
#include <openrct2-ui/handheld/Shops.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2/core/DataSerialiser.h>
#include <openrct2/object/RideObject.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2/interface/Window.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/actions/cheats/CheatSetAction.h>
#include <openrct2/GameState.h>
#include <openrct2/scenes/SceneManager.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
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

        CoordsXYZ captureFocus{656,656,112};
        auto capture=[&](const char* name) {
            const auto* output=std::getenv("POCKET_MAZE_ACCESS_CAPTURES");
            if(!output || argc!=4)return;
            std::filesystem::create_directories(output);
            Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();
            auto& rt=*engine.getRT();Viewport viewport{};viewport.width=512;viewport.height=384;viewport.zoom=ZoomLevel{0};
            viewport.viewPos=Translate3DTo2DWithZ(GetCurrentRotation(),captureFocus)-ScreenCoordsXY{330,190};
            ResetAllSpriteQuadrantPlacements();Drawing::UpdatePaletteEffects();ViewportRender(rt,&viewport);
            if(auto* window=Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu))window->onDraw(rt);
            else {H::DrawHud(rt,{512,384},H::NativeHud());
                if(H::Mazes::Active())H::Mazes::DrawPanel(rt,{512,384});
                else if(H::FlatRides::CurrentStage()!=H::FlatRides::Stage::inactive)H::FlatRides::DrawPanel(rt,{512,384});
            }
            if(auto* error=Ui::GetWindowManager()->FindByClass(WindowClass::error))error->onDraw(rt);
            Image image;image.Width=512;image.Height=384;image.Depth=8;image.Stride=rt.LineStride();image.Palette=Drawing::gPalette;
            auto* bytes=reinterpret_cast<uint8_t*>(rt.bits);image.Pixels.assign(bytes,bytes+rt.LineStride()*rt.height);
            Imaging::WriteToFile((std::filesystem::path(output)/name).string(),image,ImageFormat::png);engine.EndDraw();
        };
        gGamePaused=0;namespace F=H::FlatRides;namespace GA=GameActions;
        auto map=[&](){std::vector<uint8_t> result;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32){
            for(const auto* e:TileElementsView(CoordsXY{x,y}))if(!e->isGhost()){auto copy=*e;copy.setLastForTile(false);const auto* p=reinterpret_cast<const uint8_t*>(&copy);result.insert(result.end(),p,p+sizeof(copy));}}return result;};
        auto ghosts=[&](){int n=0;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32)for(const auto* e:TileElementsView<EntranceElement>(CoordsXY{x,y}))n+=e->isGhost();return n;};
        auto* object=objects.LoadObject("rct2.ride.mgr1");Check(object,"Carousel fixture missing");auto index=objects.GetLoadedObjectEntryIndex(object);
        ResearchInsertRideEntry(index,true);RideEntrySetInvented(index);RideTypeSetInvented(FlatRidePlacement::Type(index));
        Check(H::BeginFlatRide(index),"Native flat start failed");Check(F::Action(CoordsXY{640,640},true).error==GA::Status::ok,"Native footprint failed");GA::ProcessQueue(state);
        auto target=*F::Target();auto* ride=GetRide(target.id);Check(ride && F::CurrentStage()==F::Stage::entrance,"Native access stage missing");
        std::vector<CoordsXYZD> candidates;
        for(int y=544;y<800;y+=32)for(int x=544;x<800;x+=32){auto loc=FlatRidePlacement::EntranceLocation(*ride,{x,y});if(loc && F::Action(CoordsXY{x,y},false).error==GA::Status::ok)candidates.push_back(*loc);}
        Check(candidates.size()>2,"Native station-side fixture missing");const auto loc=candidates.front();
        auto* surface=MapGetSurfaceElementAt(loc);const auto pathLoc=CoordsXY(loc)-CoordsDirectionDelta[loc.direction];
        auto* pathObject=objects.LoadObject("rct2.footpath_surface.tarmac");auto* rail=objects.LoadObject("rct2.footpath_railings.wood");Check(pathObject && rail,"Path fixture missing");
        GA::FootpathPlaceAction path({pathLoc,surface->getBaseZ()},{},objects.GetLoadedObjectEntryIndex(pathObject),objects.GetLoadedObjectEntryIndex(rail));
        Check(GA::Execute(&path,state).error==GA::Status::ok,"Adjacent native path failed");GA::ProcessQueue(state);
        const auto before=map();const auto cash=state.park.cash;const auto stationBefore=ride->getStation();const auto contextBefore=AccessPlacementContext::Capture(*ride,loc);
        auto q=F::Action(loc,false);Check(q.error==GA::Status::ok && map()==before && state.park.cash==cash,"Native access query changed map/cash");
        F::UpdatePreview(loc);Check(ghosts()==1,"Native access ghost missing");capture("01-flat-entrance-preview.png");
        Check(map()==before && state.park.cash==cash && std::memcmp(&stationBefore,&ride->getStation(),sizeof(stationBefore))==0 && contextBefore==AccessPlacementContext::Capture(*ride,loc),"Access ghost modified real station/path/map/cash");
        GA::FlatRideEntrancePlaceAction stale(target.id,loc,false);stale.GuardContext(contextBefore,q.cost);
        DataSerialiser serialised(true);stale.Serialise(serialised);serialised.getStream().SetPosition(0);GA::FlatRideEntrancePlaceAction decoded;DataSerialiser input(false,serialised.getStream());decoded.Serialise(input);Check(decoded.Query(state,state.park).error==GA::Status::ok,"Access serialized review differs");
        Ui::ProvisionalElementsRemove();Ui::ProvisionalElementsRemove();Check(ghosts()==0 && map()==before,"Nested access suspension mutated real map");Ui::ProvisionalElementsRestore();Check(ghosts()==0,"Access restored early");Ui::ProvisionalElementsRestore();Check(ghosts()==1,"Access restoration missing");
        GA::CheatSetAction pauseBuild(CheatType::buildInPauseMode,1);Check(GA::Execute(&pauseBuild,state).error==GA::Status::ok,"Native paused-build fixture failed");GA::ProcessQueue(state);
        gGamePaused=1;Ui::GetWindowManager()->CloseAll();WindowCullDead();InputSetState(InputState::normal);
        auto* main=WindowGetMain();main->viewport->zoom=ZoomLevel{0};main->savedViewPos=Translate3DTo2DWithZ(GetCurrentRotation(),CoordsXYZ{loc.x+16,loc.y+16,loc.z})-ScreenCoordsXY{256,192};main->viewport->viewPos=main->savedViewPos;
        F::UpdatePreview(loc);Check(ghosts()==1,"Valid paused access preview missing");capture("02-paused-access-preview.png");gameStateTick();
        Check(ghosts()==(ContextHasFocus()?1:0) && map()==before && state.park.cash==cash,"Paused native access frame changed real map/cash or stranded ghost");
        gGamePaused=0;GA::CheatSetAction unpauseBuild(CheatType::buildInPauseMode,0);Check(GA::Execute(&unpauseBuild,state).error==GA::Status::ok,"Native pause setting restoration failed");GA::ProcessQueue(state);
        F::UpdatePreview({});Check(ghosts()==0 && map()==before,"Invalid target retained ghost/mutated map");F::UpdatePreview(loc);F::UpdatePreview(candidates.back());Check(ghosts()==1 && map()==before,"Changed native side duplicated/mutated ghost");F::ClearPreview();Check(ghosts()==0 && map()==before,"Clear retained native ghost");capture("03-canceled-access-unchanged.png");
        auto originalVersion=object->GetVersion();auto changed=originalVersion;++std::get<2>(changed);object->SetVersion(changed);Check(decoded.Query(state,state.park).error!=GA::Status::ok,"Changed object version accepted");object->SetVersion(originalVersion);
        F::UpdatePreview(loc);SaveFlags flags;flags.set(SaveFlag::automatic);const auto save=std::filesystem::path(argv[2])/"access-preview.park";
        Check(ScenarioSave(state,save.string(),flags)!=0 && ghosts()==1 && map()==before,"Native access autosave failed restoration");capture("04-access-autosave-restoration.png");
        Check(F::Action(loc,true).error==GA::Status::ok,"Native access apply failed");Check(F::Action(loc,true).error!=GA::Status::ok,"Pending access repeated");GA::ProcessQueue(state);
        Check(ghosts()==0 && !ride->getStation().entrance.isNull() && state.park.cash==cash-q.cost && F::CurrentStage()==F::Stage::exit,"Native access accounting/stage failed");capture("05-native-entrance-exit-stage.png");
        Check(decoded.Execute(state,state.park).error!=GA::Status::ok,"Changed station accepted stale action");
        Check(context->LoadParkFromFile(save.string()) && ghosts()==0 && GetRide(target.id)->getStation().entrance.isNull() && F::CurrentStage()==F::Stage::inactive,"Native autosave included access ghost/station or retained draft");
        gGamePaused=0;
        const auto nativeIndex=GetRide(target.id)->subtype;
        GA::FlatRideEntrancePlaceAction reused(target.id,loc,false);
        GA::RideDemolishAction demolish(target.id,GA::RideModifyType::demolish);
        Check(GA::Execute(&demolish,state).error==GA::Status::ok,"Native reuse demolition failed");GA::ProcessQueue(state);
        Check(F::Begin(nativeIndex),"Native slot reuse object selection failed");
        const auto replacementResult=F::Action(CoordsXY{640,640},true);
        if(replacementResult.error!=GA::Status::ok)std::cerr<<"Reuse result "<<replacementResult.getErrorMessage()<<" index "<<index<<"\n";
        Check(replacementResult.error==GA::Status::ok,"Native slot reuse recreation failed");GA::ProcessQueue(state);
        Check(F::Target()->id==target.id && reused.Execute(state,state.park).error!=GA::Status::ok,"Native same-slot replacement accepted old access action");F::End();

        namespace M=H::Mazes;
        auto* mazeObject=objects.LoadObject("rct2.ride.hmaze");Check(mazeObject,"Native maze object missing");auto mazeIndex=objects.GetLoadedObjectEntryIndex(mazeObject);
        ResearchInsertRideEntry(mazeIndex,true);RideEntrySetInvented(mazeIndex);RideTypeSetInvented(RIDE_TYPE_MAZE);
        Check(H::BeginMaze(mazeIndex),"Native maze begin failed");GA::ProcessQueue(state);auto mazeTarget=*M::Target();auto* mazeRide=GetRide(mazeTarget.id);
        auto mazeGhosts=[&](){int n=0;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32)for(const auto* e:TileElementsView<TrackElement>(CoordsXY{x,y}))n+=e->isGhost() && e->getRideIndex()==mazeTarget.id;return n;};
        auto cells=[&](bool ghost){std::vector<std::tuple<int,int,int,uint16_t>> result;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32){
            for(const auto* e:TileElementsView<TrackElement>(CoordsXY{x,y}))if(e->isGhost()==ghost && e->getRideIndex()==mazeTarget.id && e->getTrackType()==TrackElemType::maze)result.emplace_back(x,y,e->getBaseZ(),e->getMazeEntry());}return result;};
        const auto emptyMap=map();const auto emptyCash=state.park.cash;const auto emptyStation=mazeRide->getStation();
        captureFocus={976,976,112};M::Update(CoordsXY{960,960});capture("06-maze-initial-preview.png");auto initialQuote=M::Build(false);const auto initialProposal=cells(true);
        Check(initialQuote.error==GA::Status::ok && mazeGhosts()==1 && mazeRide->mazeTiles==0 && map()==emptyMap && state.park.cash==emptyCash && std::memcmp(&emptyStation,&mazeRide->getStation(),sizeof(emptyStation))==0,"Initial native maze ghost changed real map/station/cash/count");
        GA::MazeSetTrackAction quoted(*M::Cursor(),true,mazeTarget.id,GA::MazeBuildMode::build);
        const auto initialContext=AccessPlacementContext::Capture(*mazeRide,*M::Cursor());
        quoted.GuardContext(initialContext,initialQuote.cost+1);Check(quoted.Query(state,state.park).error!=GA::Status::ok,"Changed maze quote accepted");
        quoted.GuardContext(initialContext,initialQuote.cost);DataSerialiser mazeEncoded(true);quoted.Serialise(mazeEncoded);mazeEncoded.getStream().SetPosition(0);
        GA::MazeSetTrackAction mazeDecoded;DataSerialiser mazeDecode(false,mazeEncoded.getStream());mazeDecoded.Serialise(mazeDecode);
        Check(mazeDecoded.Query(state,state.park).error==GA::Status::ok && map()==emptyMap,"Maze serialized guard/query purity failed");
        GA::CheatSetAction noMoney(CheatType::noMoney,1);Check(GA::Execute(&noMoney,state).error==GA::Status::ok,"Native money-mode fixture failed");GA::ProcessQueue(state);
        Check(mazeDecoded.Query(state,state.park).error!=GA::Status::ok,"Changed money mode accepted old maze quote");
        GA::CheatSetAction restoreMoneyMode(CheatType::noMoney,0);Check(GA::Execute(&restoreMoneyMode,state).error==GA::Status::ok,"Native money-mode restoration failed");GA::ProcessQueue(state);
        Ui::ProvisionalElementsRemove();Ui::ProvisionalElementsRemove();Check(mazeGhosts()==0 && map()==emptyMap,"Nested maze suspension changed real map");Ui::ProvisionalElementsRestore();Check(mazeGhosts()==0,"Maze restored early");Ui::ProvisionalElementsRestore();Check(cells(true)==initialProposal,"Native maze proposal restoration differs");
        M::Update({});Check(mazeGhosts()==0 && map()==emptyMap,"Invalid initial maze target retained ghost");M::Update(CoordsXY{960,960});
        GA::CheatSetAction zeroCash(CheatType::setMoney,0);Check(GA::Execute(&zeroCash,state).error==GA::Status::ok,"Native funds fixture failed");GA::ProcessQueue(state);M::Update(CoordsXY{960,960});
        Check(M::Build(true).error==GA::Status::insufficientFunds && mazeGhosts()==0 && map()==emptyMap && mazeRide->mazeTiles==0,"Failed maze purchase changed map/count or retained ghost");capture("07-insufficient-funds-recovery.png");
        GA::CheatSetAction restoreCash(CheatType::setMoney,emptyCash);Check(GA::Execute(&restoreCash,state).error==GA::Status::ok,"Native funds restoration failed");GA::ProcessQueue(state);M::Update(CoordsXY{960,960});

        M::Height(1);Check(mazeGhosts()==1 && M::Cursor()->z==128 && map()==emptyMap,"Native maze height preview failed");captureFocus.z=128;capture("08-native-height-preview.png");M::Height(-1);captureFocus.z=112;
        GA::CheatSetAction mazePause(CheatType::buildInPauseMode,1);Check(GA::Execute(&mazePause,state).error==GA::Status::ok,"Maze paused setting failed");GA::ProcessQueue(state);gGamePaused=1;
        Ui::GetWindowManager()->CloseAll();WindowCullDead();InputSetState(InputState::normal);auto* mazeMain=WindowGetMain();
        mazeMain->viewport->zoom=ZoomLevel{0};mazeMain->savedViewPos=Translate3DTo2DWithZ(GetCurrentRotation(),captureFocus)-ScreenCoordsXY{256,192};mazeMain->viewport->viewPos=mazeMain->savedViewPos;
        M::Update(CoordsXY{960,960});Check(mazeGhosts()==1,"Paused maze preview absent");capture("26-paused-native-maze-preview.png");gameStateTick();
        Check(map()==emptyMap && state.park.cash==emptyCash && mazeRide->mazeTiles==0 && mazeGhosts()==(ContextHasFocus()?1:0),"Paused native maze frame changed real state or stranded preview");
        gGamePaused=0;GA::CheatSetAction mazeUnpause(CheatType::buildInPauseMode,0);Check(GA::Execute(&mazeUnpause,state).error==GA::Status::ok,"Maze pause restoration failed");GA::ProcessQueue(state);M::Update(CoordsXY{960,960});
        Check(M::Build(true).error==GA::Status::ok && M::Build(true).error!=GA::Status::ok,"Native initial maze apply/repeat failed");GA::ProcessQueue(state);mazeTarget=*M::Target();
        Check(cells(false)==initialProposal && mazeRide->mazeTiles==1 && state.park.cash==emptyCash-initialQuote.cost,"Native initial result differs from ghost/accounting");capture("09-native-initial-result.png");
        M::Turn(2);auto firstMap=map();const auto firstCash=state.park.cash;const auto secondProposal=cells(true);
        Check(mazeGhosts()==1 && map()==firstMap && mazeRide->mazeTiles==1,"Native within-tile proposal changed real maze");capture("10-within-tile-wall-preview.png");
        auto secondQuote=M::Build(false);Check(M::Build(true).error==GA::Status::ok,"Second half-cell native apply failed");GA::ProcessQueue(state);
        Check(cells(false)==secondProposal && state.park.cash==firstCash-secondQuote.cost,"Native wall algorithm differs between ghost and real half-cell");
        M::Update(CoordsXY{960,960});const auto multiProposal=cells(true);const auto multiMap=map();auto thirdQuote=M::Build(false);
        Check(mazeGhosts()==2 && mazeRide->mazeTiles==1 && map()==multiMap,"New-tile preview changed native count/walls");capture("11-neighbor-wall-preview.png");
        Check(M::Build(true).error==GA::Status::ok,"Native multi-tile build failed");GA::ProcessQueue(state);
        Check(cells(false)==multiProposal && mazeRide->mazeTiles==2 && state.park.cash==firstCash-secondQuote.cost-thirdQuote.cost,"Native neighbor wall result differs from preview");capture("12-native-neighbor-result.png");
        M::Turn(2);const auto moveMap=map();const auto moveCash=state.park.cash;Check(M::Move().error==GA::Status::ok,"Native cursor move failed");GA::ProcessQueue(state);
        Check(map()==moveMap && state.park.cash==moveCash,"Native move changed real geometry/accounting");
        Check(M::ReviewRemoval().error==GA::Status::ok,"Native fill preview review missing");const auto fillProposal=cells(true);capture("13-cancel-first-fill-review.png");
        Check(map()==moveMap && M::Confirm().error==GA::Status::ok && !M::Reviewing() && map()==moveMap,"Cancel-first fill changed real maze");capture("14-canceled-fill-unchanged.png");
        Check(M::ReviewRemoval().error==GA::Status::ok,"Second native fill review failed");M::ChooseRemoval();capture("15-selected-fill-review.png");Check(M::Confirm().error==GA::Status::ok,"Native fill apply failed");GA::ProcessQueue(state);
        Check(cells(false)==fillProposal && state.park.cash==moveCash,"Native fill ghost/result/cash differs");capture("16-native-fill-result.png");
        M::Update(CoordsXY{960,960});const auto staleMap=map();const auto staleCash=state.park.cash;auto version=mazeObject->GetVersion();auto changedMaze=version;++std::get<2>(changedMaze);mazeObject->SetVersion(changedMaze);
        Check(M::Build(true).error!=GA::Status::ok && mazeGhosts()==0 && map()==staleMap && state.park.cash==staleCash,"Changed descriptor executed or retained invalid maze ghost");capture("17-stale-definition-recovery.png");mazeObject->SetVersion(version);
        M::Update(CoordsXY{960,960});const auto beforeSave=map();const auto savedTiles=mazeRide->mazeTiles;const auto mazeSave=std::filesystem::path(argv[2])/"maze-preview-autosave.park";
        Check(ScenarioSave(state,mazeSave.string(),flags)!=0 && mazeGhosts()>0 && map()==beforeSave,"Native maze autosave failed preview restoration");capture("18-maze-autosave-restoration.png");
        M::End();Check(mazeGhosts()==0,"Maze end stranded ghosts");
        Check(context->LoadParkFromFile(mazeSave.string()) && mazeGhosts()==0 && GetRide(mazeTarget.id)->mazeTiles==savedTiles && !M::Active() && map()==beforeSave,"Native save included maze ghosts or changed real walls");
        Ui::GetWindowManager()->CloseAll();WindowCullDead();InputSetState(InputState::normal);
        auto resumed=H::RideManagement::Identify(mazeTarget.id);Check(resumed && H::ResumeMaze(mazeTarget.id),"Reloaded maze construction return failed");
        M::Update(CoordsXY{960,960});capture("19-reloaded-maze-selection.png");
        // Actual SDL held A remains one edge; the unchanged review stays on Cancel.
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");Check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0,"SDL init failed");
        H::ControllerInput inputController;inputController.update(true,1000);
        int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);Check(device>=0,"Virtual controller missing");
        auto* joystick=SDL_JoystickOpen(device);Check(joystick,"Virtual joystick missing");
        auto pump=[&](){SDL_Event event;while(SDL_PollEvent(&event))inputController.handleEvent(event);};pump();inputController.update(true,1010);pump();inputController.update(true,1020);
        M::Turn(2);Check(M::ReviewRemoval().error==GA::Status::ok,"Reloaded maze fill review failed");const auto heldMap=map();
        Check(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1)==0,"Held A fixture failed");SDL_JoystickUpdate();pump();
        H::HandleExplorerInput(inputController,inputController.update(true,1030));Check(!M::Reviewing() && map()==heldMap,"Held A did not cancel review unchanged");
        Check(inputController.update(true,1800).actions.empty(),"Held A repeated native placement");capture("20-held-a-cancel-return.png");
        SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);SDL_JoystickUpdate();pump();inputController.update(true,1810);SDL_JoystickClose(joystick);SDL_JoystickDetachVirtual(device);
        M::Update(CoordsXY{960,960});W::HandheldDashboardOpen();Check(mazeGhosts()==0,"Menu stranded maze preview");capture("21-menu-preview-cleanup.png");Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);WindowCullDead();
        M::End();Check(F::Resume(*resumed),"Maze access handoff failed");
        for(int n=0;n<2;n++){
            std::optional<CoordsXY> side;
            for(int y=896;y<=1056 && !side;y+=32)for(int x=896;x<=1056 && !side;x+=32)if(F::Action(CoordsXY{x,y},false).error==GA::Status::ok)side=CoordsXY{x,y};
            Check(side.has_value(),"Native maze access side missing");const auto stableMap=map();const auto stableCash=state.park.cash;const auto actualQuote=F::Action(side,false);
            F::UpdatePreview(side);Check(ghosts()==1 && map()==stableMap && state.park.cash==stableCash,"Maze access preview changed real state");
            capture(n==0?"22-maze-entrance-preview.png":"23-maze-exit-preview.png");
            Check(F::Action(side,true).error==GA::Status::ok,"Native maze access placement failed");GA::ProcessQueue(state);
            Check(ghosts()==0 && state.park.cash==stableCash-actualQuote.cost,"Maze access accounting differs");
        }
        F::End();GA::RideSetStatusAction opening(mazeTarget.id,RideStatus::open);
        Check(GA::Query(&opening,state).error==GA::Status::ok && GA::Execute(&opening,state).error==GA::Status::ok,"Native maze Open failed");GA::ProcessQueue(state);
        Check(GetRide(mazeTarget.id)->status==RideStatus::open,"Native maze did not open");W::HandheldRideOpen(mazeTarget.id);capture("24-native-maze-open-management.png");
        const auto completed=std::filesystem::path(argv[2])/"completed-maze-access.park";Check(ScenarioSave(state,completed.string(),{})!=0,"Completed native save failed");
        Check(context->LoadParkFromFile(completed.string()) && GetRide(mazeTarget.id)->status==RideStatus::open && !GetRide(mazeTarget.id)->getStation().entrance.isNull() && !GetRide(mazeTarget.id)->getStation().exit.isNull() && ghosts()==0 && mazeGhosts()==0,"Completed native access round-trip failed");
        W::HandheldRideOpen(mazeTarget.id);capture("25-save-reload-access-preservation.png");
        Ui::GetWindowManager()->CloseAll();WindowCullDead();InputSetState(InputState::normal);
        auto* monorailObject=objects.LoadObject("rct2.ride.mono1");Check(monorailObject,"Conventional original object missing");const auto monorailIndex=objects.GetLoadedObjectEntryIndex(monorailObject);
        ResearchInsertRideEntry(monorailIndex,true);RideEntrySetInvented(monorailIndex);RideTypeSetInvented(RIDE_TYPE_MONORAIL);
        Check(H::BeginCustomTrack(monorailIndex,RIDE_TYPE_MONORAIL),"Conventional station start failed");GA::ProcessQueue(state);C::Update(CoordsXY{1440,1440});
        for(int i=0;i<3;i++){Check(C::Select(TrackElemType::endStation) && C::Action(true).error==GA::Status::ok,"Conventional native station failed");GA::ProcessQueue(state);}
        const auto conventional=*C::Target();C::End();Check(F::Resume(conventional),"Conventional access return failed");captureFocus={1424,1456,112};
        for(int n=0;n<2;n++){
            std::optional<CoordsXY> side;for(int y=1312;y<=1536 && !side;y+=32)for(int x=1280;x<=1536 && !side;x+=32)if(F::Action(CoordsXY{x,y},false).error==GA::Status::ok)side=CoordsXY{x,y};
            Check(side.has_value(),"Conventional native side missing");const auto originalStation=GetRide(conventional.id)->getStation();const auto originalMap=map();const auto originalCash=state.park.cash;
            F::UpdatePreview(side);Check(ghosts()==1 && map()==originalMap && state.park.cash==originalCash && std::memcmp(&originalStation,&GetRide(conventional.id)->getStation(),sizeof(originalStation))==0,"Conventional preview changed real station/map/cash");
            capture(n==0?"27-conventional-entrance-preview.png":"28-conventional-exit-preview.png");
            Check(F::Action(side,true).error==GA::Status::ok,"Conventional native access apply failed");GA::ProcessQueue(state);
        }
        F::End();Check(!GetRide(conventional.id)->getStation().entrance.isNull() && !GetRide(conventional.id)->getStation().exit.isNull(),"Conventional native access result missing");
        W::HandheldRideAccessOpen(conventional);capture("29-conventional-access-return.png");
        Ui::GetWindowManager()->CloseAll();WindowCullDead();InputSetState(InputState::normal);
        // Native queued actions must not survive import into a different allocation epoch.
        Check(H::ResumeMaze(mazeTarget.id)==false,"Open maze unexpectedly editable");GA::RideSetStatusAction closeMaze(mazeTarget.id,RideStatus::closed);Check(GA::Execute(&closeMaze,state).error==GA::Status::ok,"Native maze close failed");GA::ProcessQueue(state);
        Check(H::ResumeMaze(mazeTarget.id),"Closed maze recovery failed");M::Update(CoordsXY{960,960});const auto pendingCursor=*M::Cursor();GA::MazeSetTrackAction pending(pendingCursor,false,mazeTarget.id,GA::MazeBuildMode::build);
        pending.GuardContext(AccessPlacementContext::Capture(*GetRide(mazeTarget.id),pendingCursor),pending.Query(state,state.park).cost);GA::Enqueue(&pending,state.currentTicks);
        Check(context->LoadParkFromFile(completed.string()),"Pending maze import failed");GA::ProcessQueue(state);
        Check(!M::Active() && ghosts()==0 && mazeGhosts()==0 && GetRide(mazeTarget.id)->status==RideStatus::open,"Load retained preview or pending maze action");
        Ui::GetWindowManager()->CloseAll();WindowCullDead();InputSetState(InputState::normal);
        if(gGamePaused & GAME_PAUSED_NORMAL){GA::PauseToggleAction unpause;Check(GA::Execute(&unpause,state).error==GA::Status::ok,"Native recovery unpause failed");GA::ProcessQueue(state);}
        GA::RideSetStatusAction closeForFailure(mazeTarget.id,RideStatus::closed);Check(GA::Execute(&closeForFailure,state).error==GA::Status::ok,"Failure fixture native close failed");GA::ProcessQueue(state);
        Check(H::ResumeMaze(mazeTarget.id),"Failure fixture native return failed");M::Update(CoordsXY{960,960});Check(mazeGhosts()>0,"Native failure fixture lacks valid ghost");const auto failureMap=map();const auto failureCash=state.park.cash;
        H::EndTerrain(); // Pocket Menu performs this before the native importer.
        Check(!context->LoadParkFromFile((std::filesystem::path(argv[2])/"absent-native-load.park").string()),"Missing native file loaded");H::UpdateExplorer(inputController);
        Check(mazeGhosts()==0 && ghosts()==0 && map()==failureMap && state.park.cash==failureCash,"Early native failure/error-modal cleanup changed real park");captureFocus={976,976,112};capture("30-native-load-error-cleanup.png");
        Ui::GetWindowManager()->CloseAll();WindowCullDead();InputSetState(InputState::normal);Check(H::ResumeMaze(mazeTarget.id),"Native error return failed");M::Update(CoordsXY{960,960});
        Check(mazeGhosts()>0,"Failure recovery did not restore valid proposal");M::ToggleCamera();Check(mazeGhosts()==0,"Camera transition stranded maze preview");M::ToggleCamera();M::Update(CoordsXY{960,960});
        context->GetSceneManager()->setActiveScene(context->GetSceneManager()->getTitleScene());Check(!M::Active() && ghosts()==0 && mazeGhosts()==0,"Title transition retained maze/access preview");
        Check(context->LoadParkFromFile(completed.string()),"Title recovery native load failed");Ui::GetWindowManager()->CloseAll();WindowCullDead();InputSetState(InputState::normal);
        GA::RideSetStatusAction closeForExit(mazeTarget.id,RideStatus::closed);Check(GA::Execute(&closeForExit,state).error==GA::Status::ok,"Exit fixture native close failed");GA::ProcessQueue(state);Check(H::ResumeMaze(mazeTarget.id),"Exit fixture native return failed");M::Update(CoordsXY{960,960});
        context.reset();Check(!M::Active() && F::CurrentStage()==F::Stage::inactive,"Pre-unload retained maze/access draft");
        std::cout<<"MazeAccessPreviewTests access/maze native/SDL PASS\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

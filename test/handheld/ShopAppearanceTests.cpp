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
        auto capture=[&](const char* name,bool menu=true) {
            const auto* output=std::getenv("POCKET_SHOP_APPEARANCE_CAPTURES");
            if (!output || argc!=4) return;
            std::filesystem::create_directories(output);
            Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();
            auto& rt=*engine.getRT();Viewport viewport{};viewport.width=512;viewport.height=384;viewport.zoom=ZoomLevel{1};
            viewport.viewPos=Translate3DTo2DWithZ(0,captureFocus)-ScreenCoordsXY{viewport.zoom.ApplyTo(410),viewport.zoom.ApplyTo(190)};
            ResetAllSpriteQuadrantPlacements();Drawing::UpdatePaletteEffects();ViewportRender(rt,&viewport);
            if (menu) {
                if (auto* window=Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)) window->onDraw(rt);
            } else {
                H::DrawHud(rt,{512,384},H::NativeHud());
                if(H::IsShopToolActive()) {
                    H::DrawShopOverlay(rt,H::Concept::kOrigin,H::Shops::Name(H::ShopSelection()),H::Shops::Eligible(H::ShopSelection()),H::ShopDirection());
                    H::DrawShopFooter(rt,{512,384});
                }
            }
            if (auto* error=Ui::GetWindowManager()->FindByClass(WindowClass::error)) error->onDraw(rt);
            Image image;image.Width=512;image.Height=384;image.Depth=8;image.Stride=rt.LineStride();image.Palette=Drawing::gPalette;
            auto* bytes=reinterpret_cast<uint8_t*>(rt.bits);image.Pixels.assign(bytes,bytes+rt.LineStride()*rt.height);
            Imaging::WriteToFile((std::filesystem::path(output)/name).string(),image,ImageFormat::png);engine.EndDraw();
        };
        H::ControllerInput controller;
        auto press=[&](H::Action action){H::Frame f;f.actions={action};Check(H::HandleMenuInput(controller,f),"Controller menu input not consumed");};
        auto choose=[&](int row){W::HandheldMenuMove(row);press(H::Action::confirm);};

        namespace S=H::Shops;namespace M=H::ShopManagement;namespace A=H::ShopAppearance;
        using Channel=GameActions::RideSetAppearanceType;
        auto mapBytes=[&](){std::vector<uint8_t> result;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32) {
            for(const auto* e:TileElementsView(CoordsXY{x,y})){auto* p=reinterpret_cast<const uint8_t*>(e);result.insert(result.end(),p,p+sizeof(TileElement));}}return result;};
        auto place=[&](const char* objectName,CoordsXY tile){
            auto* object=objects.LoadObject(std::string("rct2.ride.")+objectName);Check(object,"Shop fixture object missing");
            auto index=objects.GetLoadedObjectEntryIndex(object);ResearchInsertRideEntry(index,true);RideEntrySetInvented(index);RideTypeSetInvented(ShopPlacement::Type(index));
            auto q=S::Action(tile,index,0,false);if(q.error!=GameActions::Status::ok)std::cerr<<q.getErrorMessage()<<std::endl;
            Check(q.error==GameActions::Status::ok,"Shop placement query failed");Check(S::Action(tile,index,0,true).error==GameActions::Status::ok,"Shop placement failed");GameActions::ProcessQueue(state);
            auto id=(*TileElementsView<TrackElement>(tile).begin())->getRideIndex();auto target=M::Capture(id);Check(target.has_value(),"Placed shop inspection missing");return *target;
        };
        std::vector<M::Target> targets;
        for(auto [name,p]:{std::pair{"burgb",CoordsXY{640,640}}, {"balln",CoordsXY{704,640}}, {"tlt1",CoordsXY{768,640}}, {"atm1",CoordsXY{832,640}}, {"infok",CoordsXY{896,640}}})targets.push_back(place(name,p));
        gGamePaused=1;
        const auto cash=state.park.cash;const auto map=mapBytes();const auto common=state.park.samePriceThroughoutPark;
        bool haveItem=false;size_t channels=0;
        for(size_t n=0;n<targets.size();++n){
            auto target=targets[n];const auto* ride=M::Find(target);Check(ride,"Native shop vanished");
            const auto prices=ride->price;const auto status=ride->status;const auto name=ride->getName();
            auto settings=A::Settings(target);channels+=settings.size();
            std::cout<<name<<" channels "<<settings.size()<<"\n";
            W::HandheldRideOpen(target.id);choose(4);captureFocus={target.location.x+16,target.location.y+16,target.location.z};
            capture(("01-capability-"+std::to_string(n)+".png").c_str());
            for(size_t c=0;c<settings.size();++c){
                auto draft=A::Begin(target,c);Check(draft.has_value(),"Supported channel has no editor");
                const auto original=draft->setting.current;
                Check(A::Action(*draft,false).getErrorMessage()=="Choose a different native value","Unchanged selection misreported as stale shop");
                draft->value=draft->setting.values[original==draft->setting.values.front()?1:0];
                auto q=A::Action(*draft,false);Check(q.error==GameActions::Status::ok && q.cost==0,"Native appearance quote failed or cost differs");
                Check(state.park.cash==cash && mapBytes()==map && ShopAppearanceContext::Value(*ride,*draft->setting.appearance)==original,"Query changed park");
                auto invalid=*draft;invalid.value=UINT16_MAX;Check(A::Action(invalid,false).error!=GameActions::Status::ok,"Invalid palette value accepted");
                invalid=*draft;invalid.setting.appearance=Channel::entranceStyle;Check(A::Action(invalid,false).error!=GameActions::Status::ok,"Shop station appearance invented");
                GameActions::RideSetAppearanceAction bound(target.id,*draft->setting.appearance,draft->value,0);bound.GuardShopContext(draft->context);
                DataSerialiser encoded(true);bound.Serialise(encoded);encoded.getStream().SetPosition(0);GameActions::RideSetAppearanceAction decoded;
                DataSerialiser decode(false,encoded.getStream());decoded.Serialise(decode);
                Check(decoded.Query(state,state.park).error==GameActions::Status::ok,"Appearance guard serialization differs");
                Check(A::Action(*draft,true).error==GameActions::Status::ok,"Native appearance enqueue failed");GameActions::ProcessQueue(state);
                Check(ShopAppearanceContext::Value(*ride,*draft->setting.appearance)==draft->value,"Native channel did not change");
                Check(decoded.Execute(state,state.park).error!=GameActions::Status::ok,"Stale baseline executed after serialization");
                Check(A::Action(*draft,false).error!=GameActions::Status::ok,"Stale appearance baseline accepted");
                haveItem|=*draft->setting.appearance==Channel::sellingItemColourIsRandom;
            }
            capture(("09-result-"+std::to_string(n)+".png").c_str());
            W::HandheldMenuBack();capture(("10-management-"+std::to_string(n)+".png").c_str());
            Check(ride->price==prices && ride->status==status && ride->getName()==name && state.park.samePriceThroughoutPark==common && state.park.cash==cash && mapBytes()==map,"Appearance changed unrelated native state");
            W::HandheldMenuBack();
        }
        Check(channels>3 && haveItem,"Differing channel/merchandise coverage absent");
        auto target=targets[1];auto ride=M::Find(target);auto draft=A::Begin(target,0);Check(draft.has_value(),"Building channel absent");
        draft->value=draft->setting.values[draft->setting.current==draft->setting.values.front()?1:0];
        // Native object definition mutation is isolated to this fixture, then restored.
        auto* object=objects.GetLoadedObject<RideObject>(target.object);auto& entry=const_cast<RideObjectEntry&>(object->GetEntry());
        entry.flags.set(RideEntryFlag::disableColourTab);Check(A::Settings(target).empty() && A::Action(*draft,false).error!=GameActions::Status::ok,"Changed object capabilities accepted");entry.flags.unset(RideEntryFlag::disableColourTab);
        const auto originalVersion=object->GetVersion();auto changedVersion=originalVersion;++std::get<2>(changedVersion);object->SetVersion(changedVersion);
        Check(A::Action(*draft,false).error!=GameActions::Status::ok,"Changed native object version accepted");object->SetVersion(originalVersion);
        W::HandheldRideOpen(target.id);choose(4);choose(0);captureFocus={720,656,112};capture("02-native-palette.png");
        W::HandheldMenuMove(24);capture("11-paged-native-palette.png");W::HandheldMenuMove(-24);
        auto model=W::HandheldShopAppearancePresentation();Check(model.rows.size()==Drawing::kColourNumNormal,"Native palette list differs");
        const auto cancelled=ShopAppearanceContext::Capture(*ride);
        W::HandheldMenuMove(1);press(H::Action::back);
        Check(!W::HandheldShopAppearancePresentation().confirmation && ShopAppearanceContext::Capture(*ride)==cancelled,"Palette cancel applied appearance");capture("04-unchanged-cancellation.png");
        press(H::Action::confirm);W::HandheldMenuMove(1);H::Frame duplicate;duplicate.actions={H::Action::confirm,H::Action::confirm};
        Check(H::HandleMenuInput(controller,duplicate),"Appearance apply input not consumed");GameActions::ProcessQueue(state);
        Check(!W::HandheldShopAppearancePresentation().confirmation && W::HandheldShopAppearancePresentation().status=="Appearance updated","Direct Apply did not return to selected channel");capture("05-native-applied.png");
        // Real SDL virtual held input must not cross a Cancel-first review.
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");Check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0,"SDL controller init failed");
        {
            H::ControllerInput input;input.update(true,1000);const auto device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);Check(device>=0,"Virtual controller absent");auto* joystick=SDL_JoystickOpen(device);Check(joystick,"Virtual joystick absent");
            auto pump=[&](){SDL_Event event;while(SDL_PollEvent(&event))input.handleEvent(event);};pump();input.update(true,1010);pump();input.update(true,1020);
            choose(0);W::HandheldMenuMove(1);Check(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1)==0,"Virtual A failed");SDL_JoystickUpdate();pump();auto frame=input.update(true,1030);Check(H::HandleMenuInput(input,frame),"Virtual A not consumed");
            GameActions::ProcessQueue(state);
            Check(!W::HandheldShopAppearancePresentation().confirmation && input.update(true,1800).actions.empty(),"Held A repeated an edit");
            capture("06-held-a-review.png");SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);SDL_JoystickUpdate();pump();input.update(true,1810);SDL_JoystickClose(joystick);SDL_JoystickDetachVirtual(device);
        }
        // Bind review to exact native allocation, even a same-location/same-object replacement.
        auto stale=A::Begin(target,0);stale->value=stale->setting.values[stale->setting.current==stale->setting.values.front()?1:0];
        GameActions::RideSetAppearanceAction queued(target.id,*stale->setting.appearance,stale->value,0);queued.GuardShopContext(stale->context);
        W::HandheldMenuBack();gGamePaused=0;Check(M::Demolish(target,true).error==GameActions::Status::ok,"Native shop demolition failed");GameActions::ProcessQueue(state);
        auto replacement=place("balln",{704,640});Check(replacement.id==target.id,"Native slot reuse fixture failed");
        Check(queued.Execute(state,state.park).error!=GameActions::Status::ok && A::Action(*stale,true).error!=GameActions::Status::ok,"Reused shop accepted old appearance review");
        targets[1]=replacement;
        W::HandheldRideOpen(replacement.id);choose(4);choose(0);W::HandheldMenuMove(1);
        auto nativeChange=GameActions::RideSetAppearanceAction(replacement.id,Channel::trackColourMain,static_cast<uint16_t>(Drawing::Colour::brightRed),0);
        Check(GameActions::Execute(&nativeChange,state).error==GameActions::Status::ok,"Concurrent native action failed");GameActions::ProcessQueue(state);press(H::Action::confirm);
        Check(W::HandheldShopAppearancePresentation().status.find("changed")!=std::string::npos,"Stale UI review lacks recoverable error");capture("07-stale-review-recovery.png");
        W::HandheldMenuBack();W::HandheldMenuBack();W::HandheldMenuBack();
        const auto save=std::filesystem::path(argv[2])/"shop-appearance.park";
        std::vector<std::array<uint16_t,4>> values;for(auto t:targets){auto* r=M::Find(t);values.push_back({ShopAppearanceContext::Value(*r,Channel::trackColourMain),ShopAppearanceContext::Value(*r,Channel::trackColourAdditional),ShopAppearanceContext::Value(*r,Channel::trackColourSupports),ShopAppearanceContext::Value(*r,Channel::sellingItemColourIsRandom)});}
        const auto savedCash=state.park.cash;
        Check(ScenarioSave(state,save.string(),{})!=0 && context->LoadParkFromFile(save.string()),"Native appearance roundtrip failed");
        for(size_t i=0;i<targets.size();++i){auto* r=GetRide(targets[i].id);Check(r,"Reloaded shop missing");Check(values[i]==std::array<uint16_t,4>{ShopAppearanceContext::Value(*r,Channel::trackColourMain),ShopAppearanceContext::Value(*r,Channel::trackColourAdditional),ShopAppearanceContext::Value(*r,Channel::trackColourSupports),ShopAppearanceContext::Value(*r,Channel::sellingItemColourIsRandom)},"Reloaded appearance differs");}
        Check(state.park.cash==savedCash && !W::HandheldShopManagementIsOpen(),"Native load retained menu or changed cash");
        W::HandheldRideOpen(targets[1].id);choose(4);captureFocus={720,656,112};capture("08-reloaded-merchandise.png");
        choose(1);capture("15-native-merchandise-choices.png");
        W::HandheldMenuBack();W::HandheldMenuBack();
        W::HandheldMenuBack();
        Check(!Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu),"Preview fixture still has a modal menu");
        Ui::GetWindowManager()->CloseAll();
        Ui::GetWindowManager()->CloseByClass(WindowClass::error);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        // Native window updates cull closed windows before the next ordinary frame.
        WindowCullDead();
        auto* balloon=GetRide(targets[1].id);const auto selection=balloon->subtype;
        Check(H::SetShopSelection(selection),"Preview selection failed");
        const CoordsXY previewTile{1024,640};captureFocus={1040,656,112};
        auto* mainWindow=WindowGetMain();Check(mainWindow && mainWindow->viewport,"Native camera missing");
        mainWindow->viewport->zoom=ZoomLevel{0};
        mainWindow->viewport->viewPos=Translate3DTo2DWithZ(GetCurrentRotation(),captureFocus)-ScreenCoordsXY{256,192};
        mainWindow->savedViewPos=mainWindow->viewport->viewPos;ResetAllSpriteQuadrantPlacements();
        const auto nativeTarget=ScreenGetMapXY({256,192},nullptr);
        Check(nativeTarget && CoordsXY{nativeTarget->x&~31,nativeTarget->y&~31}==previewTile,"Native camera does not target preview fixture");
        const auto previewCash=state.park.cash;const auto previewMap=mapBytes();
        auto ghosts=[&](){int count=0;for(const auto* e:TileElementsView<TrackElement>(previewTile))count+=e->isGhost();return count;};
        GameActions::CheatSetAction allowPaused(CheatType::buildInPauseMode,1);
        Check(GameActions::Execute(&allowPaused,state).error==GameActions::Status::ok,"Native paused-build setting failed");GameActions::ProcessQueue(state);gGamePaused=1;
        S::UpdatePreview(previewTile,selection,0);Check(ghosts()==1 && state.park.cash==previewCash,"Paused native shop preview missing");
        capture("12-paused-native-preview.png",false);
        Ui::ProvisionalElementsRemove();Ui::ProvisionalElementsRemove();Check(ghosts()==0 && mapBytes()==previewMap,"Nested preview suspension mutated real map");
        Ui::ProvisionalElementsRestore();Check(ghosts()==0,"Nested preview restored early");Ui::ProvisionalElementsRestore();Check(ghosts()==1,"Nested preview failed restoration");
        const auto provisional=std::filesystem::path(argv[2])/"shop-preview-autosave.park";SaveFlags automatic;automatic.set(SaveFlag::automatic);
        Check(ScenarioSave(state,provisional.string(),automatic)!=0 && ghosts()==1 && state.park.cash==previewCash,"Native autosave did not restore paused preview");
        Check(InputGetState()==InputState::reset,"Native load/reset fixture assumption changed");
        gameStateTick();
        Check(ghosts()==0 && mapBytes()==previewMap && state.park.cash==previewCash,"Reset-state frame retained preview or changed real park");
        // Complete the native input reset as the ordinary mouse/UI frame does.
        InputSetState(InputState::normal);WindowCullDead();S::UpdatePreview(previewTile,selection,0);
        Check(ghosts()==1,"Normal-input paused preview failed restoration");
        gameStateTick();const bool focusCleanup=!ContextHasFocus();
        Check(ghosts()==(focusCleanup?0:1) && state.park.cash==previewCash,"Paused full shell frame disagrees with observed focus");
        W::HandheldRideOpen(targets[1].id);Check(ghosts()==0,"Inspection left preview ghost");capture("13-preview-management-return.png");
        W::HandheldMenuBack();H::EndTerrain();
        Check(context->LoadParkFromFile(provisional.string()) && ghosts()==0 && !H::IsShopToolActive(),"Autosave persisted borrowed shop/ghost/tool");
        size_t realRides=0;for(const auto& r:state.rides)realRides+=!r.id.IsNull();Check(realRides==targets.size(),"Temporary preview ride persisted in autosave");
        W::HandheldRideOpen(targets[1].id);choose(4);captureFocus={720,656,112};capture("14-autosave-reload-preservation.png");
        choose(0);W::HandheldMenuMove(1);press(H::Action::confirm);
        Check(!W::HandheldShopAppearancePresentation().confirmation,"Direct appearance unexpectedly entered review");
        auto pending=A::Begin(*M::Capture(targets[1].id),0);pending->value=pending->setting.values[pending->setting.current==pending->setting.values.front()?1:0];
        const auto reloadValue=pending->setting.current;
        GameActions::RideSetAppearanceAction pendingAction(pending->target.id,*pending->setting.appearance,pending->value,0);
        pendingAction.GuardShopContext(pending->context);GameActions::Enqueue(&pendingAction,state.currentTicks);
        Check(ShopAppearanceContext::Value(*GetRide(targets[1].id),*pending->setting.appearance)==reloadValue,"Native enqueue executed prematurely");
        Check(context->LoadParkFromFile(provisional.string()),"Pending-review native load failed");GameActions::ProcessQueue(state);
        Check(!W::HandheldShopManagementIsOpen() && ShopAppearanceContext::Value(*GetRide(targets[1].id),*pending->setting.appearance)==reloadValue,"Load retained review/queued appearance");
        W::HandheldRideOpen(targets[1].id);choose(4);capture("16-pending-review-load-recovery.png");

        std::cout<<"ShopAppearanceTests native/SDL PASS\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}

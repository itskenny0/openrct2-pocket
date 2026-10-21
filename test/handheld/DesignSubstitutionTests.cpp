#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/ride/TrackDesignRepository.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/core/DataSerialiser.h>
#include <openrct2/actions/ResultWithMessage.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2-ui/handheld/Designs.h>
#include <openrct2-ui/handheld/FileBrowsing.h>
#include <openrct2/actions/track/SavedTrackDesignPlaceAction.h>
#include <openrct2/rct2/T6Exporter.h>
#include <openrct2/world/tile_element/SmallSceneryElement.h>
#include <fstream>
#include <openrct2-ui/handheld/DesignAvailability.h>
// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
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
            const auto* output=std::getenv("POCKET_DESIGN_SUBSTITUTION_CAPTURES");
            if (!output || argc!=4) return;
            std::filesystem::create_directories(output);
            Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();
            auto& rt=*engine.getRT();Viewport viewport{};viewport.width=512;viewport.height=384;viewport.zoom=ZoomLevel{1};
            viewport.viewPos=Translate3DTo2DWithZ(0,captureFocus)-ScreenCoordsXY{viewport.zoom.ApplyTo(310),viewport.zoom.ApplyTo(190)};
            ResetAllSpriteQuadrantPlacements();Drawing::UpdatePaletteEffects();ViewportRender(rt,&viewport);
            if (menu) {
                if (auto* window=Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)) window->onDraw(rt);
            } else {
                if (H::Designs::Active())H::Designs::DrawPanel(rt,{512,384});
                else if (H::Paths::IsActive())H::Paths::DrawOverlay(rt,{8,8});
                else if(H::FlatRides::CurrentStage()!=H::FlatRides::Stage::inactive)H::FlatRides::DrawPanel(rt,{512,384});
                H::DrawHud(rt,{512,384},H::NativeHud());
            }
            if (auto* error=Ui::GetWindowManager()->FindByClass(WindowClass::error)) error->onDraw(rt);
            Image image;image.Width=512;image.Height=384;image.Depth=8;image.Stride=rt.LineStride();image.Palette=Drawing::gPalette;
            auto* bytes=reinterpret_cast<uint8_t*>(rt.bits);image.Pixels.assign(bytes,bytes+rt.LineStride()*rt.height);
            Imaging::WriteToFile((std::filesystem::path(output)/name).string(),image,ImageFormat::png);engine.EndDraw();
        };
        H::ControllerInput controller;
        auto press=[&](H::Action action){H::Frame f;f.actions={action};Check(H::HandleMenuInput(controller,f),"Controller menu input not consumed");};
        auto choose=[&](int row){W::HandheldMenuMove(-1000);W::HandheldMenuMove(row);press(H::Action::confirm);};
        namespace A = H::DesignAvailability;
        auto* vehicle=objects.LoadObject("rct2.ride.mono1");
        Check(vehicle,"Native vehicle fixture missing");
        auto index=objects.GetLoadedObjectEntryIndex(vehicle);
        TrackDesign td{};
        td.trackAndVehicle.rtdIndex=RIDE_TYPE_MONORAIL;
        td.trackAndVehicle.vehicleObject=vehicle->GetDescriptor();
        td.trackElements.push_back({TrackElemType::endStation});
        RideTypeSetInvented(RIDE_TYPE_MONORAIL);
        RideEntrySetInvented(index);
        Check(A::Compatible(td,index),"Native compatible vehicle rejected");
        const auto cash=state.park.cash;
        const auto free=GetNextFreeRideId();
        const auto seed=state.scenarioRand;
        auto lines=A::Lines(td);
        Check(lines.size()>=6,"Availability details incomplete");
        auto absent=td;
        absent.trackAndVehicle.vehicleObject=ObjectEntryDescriptor("pocket.fixture.missing_vehicle");
        Check(A::State(absent.trackAndVehicle.vehicleObject)=="not loaded","Missing required vehicle misreported");
        Check(A::Vehicles(absent).size()==1,"Native alternative discovery depends on missing original");
        SetEveryRideEntryNotInvented();
        Check(A::Vehicles(td).empty() && A::State(td.trackAndVehicle.vehicleObject)=="not researched","Research bypassed");
        RideEntrySetInvented(index);
        auto bad=td; bad.trackAndVehicle.rtdIndex=RIDE_TYPE_WOODEN_ROLLER_COASTER;
        Check(!A::Compatible(bad,index),"Different native family accepted");
        TrackDesignSceneryElement optional{};
        optional.sceneryObject=ObjectEntryDescriptor("pocket.fixture.missing_scenery");
        td.sceneryElements.push_back(optional);
        const auto flag=_trackDesignPlaceStateSceneryUnavailable;
        Check(!TrackDesignResolveScenery(optional),"Missing optional scenery resolved");
        lines=A::Lines(td);
        Check(std::any_of(lines.begin(),lines.end(),[](const auto& line){return line.find("ALL design scenery")!=std::string::npos;}),"Native all-scenery fallback not explicit");
        Check(_trackDesignPlaceStateSceneryUnavailable==flag,"Read-only resolver changed native placement flag");
        Check(state.park.cash==cash && GetNextFreeRideId()==free && state.scenarioRand==seed
            && RideEntryIsInvented(index),"Inspection changed real park/accounting/research/RNG");
        auto* alternate=objects.LoadObject("rct2.ride.mono2");Check(alternate,"Alternate fixture missing");
        auto alternateIndex=objects.GetLoadedObjectEntryIndex(alternate);RideEntrySetInvented(alternateIndex);
        Check(H::BeginCustomTrack(index,RIDE_TYPE_MONORAIL),"Native circuit start failed");
        GameActions::ProcessQueue(state);C::Update(CoordsXY{640,640});
        auto place=[&](TrackElemType type){Check(C::Select(type),"Native piece absent");auto r=C::Action(true);if(r.error!=GameActions::Status::ok)std::cerr<<"piece "<<static_cast<int>(type)<<" "<<r.getErrorMessage()<<std::endl;Check(r.error==GameActions::Status::ok,"Native circuit build failed");GameActions::ProcessQueue(state);};
        for(int n=0;n<4;++n)place(TrackElemType::endStation);
        for(int side=0;side<4;++side){for(int n=0;n<(side==0?2:side==2?6:4);++n)place(TrackElemType::flat);place(TrackElemType::leftQuarterTurn3Tiles);}
        Check(C::Completed().has_value(),"Native design fixture circuit incomplete");
        auto original=C::Completed()->id;H::EndTerrain();
        TrackDesign exported;TrackDesignState tds{};Check(exported.CreateTrackDesign(tds,*GetRide(original)).Successful,"Native design export failed");
        auto missingEntry=vehicle->GetObjectEntry();missingEntry.SetName("NOSUCH");
        exported.trackAndVehicle.vehicleObject=ObjectEntryDescriptor(missingEntry);
        auto missingScenery=objects.LoadObject("rct2.scenery_small.corroof");Check(missingScenery,"Scenery fixture absent");
        auto missingSceneryEntry=missingScenery->GetObjectEntry();missingSceneryEntry.SetName("NOSUCH");
        TrackDesignSceneryElement missingOptional{};missingOptional.sceneryObject=ObjectEntryDescriptor(missingSceneryEntry);missingOptional.loc={192,192,0};
        exported.sceneryElements={missingOptional};
        const auto tracks=std::filesystem::u8path(context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user,DirId::trackDesigns));
        auto file=tracks/"Folder"/"Missing original.td6";std::filesystem::create_directories(file.parent_path());
        Check(RCT2::T6Exporter(exported).SaveTrack(file.string().c_str()),"Native missing vehicle design export failed");
        const auto source=H::FileBrowsing::Capture(file);Check(source.has_value(),"Source identity absent");
        Check(!H::Designs::Begin(file.string()),"Missing required vehicle silently substituted");
        for(int n=0;n<18;++n)std::filesystem::copy_file(file,tracks/("Dense native design "+std::to_string(n)+".td6"),std::filesystem::copy_options::overwrite_existing);
        std::filesystem::create_directories(tracks/"Empty");
        TrackRepositoryScan();
        W::HandheldDesignFilesOpen();W::HandheldMenuMove(14);capture("01-dense-native-design-browser.png");
        W::HandheldDesignFilesOpen();choose(2);capture("02-empty-folder.png");press(H::Action::back);
        W::HandheldDesignFilesOpen();choose(3);capture("03-native-folder.png");choose(2);capture("04-missing-original-inspection.png");
        W::HandheldMenuMove(3);capture("05-required-object-detail.png");W::HandheldMenuMove(-1);press(H::Action::confirm);capture("06-eligible-native-vehicles.png");
        choose(1);capture("07-explicit-vehicle-native-preview.png");W::HandheldMenuMove(-2);press(H::Action::confirm);
        Check(H::Designs::Active() && H::Designs::BrowserReturn(),"Browser explicit substitute handoff failed");
        Check(H::Designs::Selection()==file.string(),"Browser selected file identity lost");
        Check(H::Designs::Draft()->trackAndVehicle.vehicleObject==alternate->GetDescriptor(),"Explicit vehicle not retained");
        Check(H::Designs::Height(1),"Manual height unavailable");H::Designs::Rotate();
        const auto orientation=H::Designs::Orientation();const auto height=H::Designs::HeightOffset();
        Check(H::Designs::ChooseVehicle(vehicle->GetDescriptor()),"Explicit original compatible choice failed");
        Check(H::Designs::Orientation()==orientation && H::Designs::HeightOffset()==height,"Vehicle selection reset placement");
        Check(H::Designs::ChooseVehicle(alternate->GetDescriptor()),"Alternate reselection failed");
        H::Designs::AutomaticHeight();
        // B/Y browser return keeps the selected file and explicit same-file choice.
        W::HandheldDesignPickerOpen();capture("08-selected-file-browser-return.png");
        press(H::Action::confirm);choose(2);capture("09-choice-cancel-return.png");press(H::Action::back);
        choose(2);choose(1);W::HandheldMenuMove(-2);press(H::Action::confirm);Check(H::Designs::Active(),"Same-file explicit choice not recovered");
        const CoordsXY site{1408,1408};captureFocus={1424,1424,112};
        auto count=[&](bool ghost){size_t n=0;for(int y=32;y<2016;y+=32)for(int x=32;x<2016;x+=32)for(auto* e:TileElementsView<>(CoordsXY{x,y}))if(e->isGhost()==ghost)++n;return n;};
        const auto beforeCash=state.park.cash;const auto beforeElements=count(false);const auto beforeRides=RideManager(state).size();
        H::Designs::SetScenery(true);
        const auto designQuote=H::Designs::Query(site);if(designQuote.error!=GameActions::Status::ok)std::cerr<<designQuote.getErrorMessage()<<std::endl;
        Check(designQuote.error==GameActions::Status::ok,"Native substituted query failed");
        Check(H::Designs::SceneryNotice().find("omitted")!=std::string::npos,"Missing optional scenery warning absent");
        Check(state.park.cash==beforeCash && count(false)==beforeElements && RideManager(state).size()==beforeRides,"Native query changed real park");
        H::Designs::UpdatePreview(site);capture("10-native-substitute-ghost.png",false);Check(count(true)>0 && state.park.cash==beforeCash,"Native substituted preview missing or spent");
        const auto provisional=std::filesystem::path(argv[2])/"substitution-autosave.park";
        Check(ScenarioSave(state,provisional.string(),{SaveFlag::automatic})!=0,"Alternative preview autosave failed");
        Check(count(true)>0 && count(false)==beforeElements && state.park.cash==beforeCash
            && H::Designs::Draft()->trackAndVehicle.vehicleObject==alternate->GetDescriptor(),"Autosave changed real state or selected alternative");
        H::Designs::ClearPreview();Check(count(true)==0,"Native substituted ghost cleanup failed");
        // Put the actual controller cursor over the same native quoted site.
        auto* main=WindowGetMain();main->viewport->zoom=ZoomLevel{0};
        main->viewport->viewPos=Translate3DTo2DWithZ(main->viewport->rotation,{site.x+16,site.y+16,MapGetHighestZ(site)})-ScreenCoordsXY{256,192};
        main->savedViewPos=main->viewport->viewPos;H::UpdateExplorer(controller);
        W::HandheldDesignOptionsOpen();capture("11-native-placement-options.png");
        const auto initialFacing=H::Designs::Orientation();const auto optionCash=state.park.cash;
        choose(1);Check(H::Designs::HeightOffset().has_value(),"Controller height did not become manual");
        choose(2);Check(!H::Designs::HeightOffset(),"Controller automatic-height toggle failed");
        choose(3);Check(H::Designs::Orientation()==((initialFacing+1)&3),"Controller orientation did not rotate design");
        capture("25-design-rotated.png");H::Frame left;left.navigateX=-1;Check(H::HandleMenuInput(controller,left),"Design adjustment input not consumed");
        Check(H::Designs::Orientation()==initialFacing,"Controller reverse rotation failed");
        choose(4);Check(!H::Designs::IncludesScenery(),"Scenery toggle did not turn off");choose(4);
        Check(H::Designs::IncludesScenery() && state.park.cash==optionCash,"Design option edits spent cash or lost scenery");
        choose(5);capture("26-design-vehicles.png");press(H::Action::back);
        capture("24-design-workspace.png");choose(6);capture("12-native-warning-list.png");press(H::Action::back);press(H::Action::back);
        Check(H::Designs::Review(site),"Native substitute review failed");
        Check(!H::Designs::ChooseVehicle(vehicle->GetDescriptor()),"Reviewed vehicle mutated");
        W::HandheldDesignReviewOpen();capture("13-cancel-first-native-quote.png");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
        Check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0,"SDL init failed");
        H::ControllerInput virtualInput;
        const auto device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
        Check(device>=0,"Virtual controller missing");auto* joystick=SDL_JoystickOpen(device);Check(joystick,"Virtual controller open failed");
        auto pump=[&]{SDL_Event e;while(SDL_PollEvent(&e))virtualInput.handleEvent(e);};
        pump();virtualInput.update(true,100);pump();virtualInput.update(true,110);
        Check(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1)==0,"Virtual A failed");
        SDL_JoystickUpdate();pump();auto frame=virtualInput.update(true,120);
        Check(H::HandleMenuInput(virtualInput,frame),"SDL review input not consumed");
        Check(!H::Designs::HasReview() && virtualInput.update(true,130).actions.empty()
            && count(false)==beforeElements && state.park.cash==beforeCash,"Held A did not cancel unchanged");
        SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);SDL_JoystickUpdate();pump();virtualInput.update(true,140);
        SDL_JoystickClose(joystick);SDL_JoystickDetachVirtual(device);
        Check(state.park.cash==beforeCash && count(false)==beforeElements,"Cancelled substitute mutated park");capture("14-cancelled-unchanged.png",false);
        Check(H::Designs::Review(site),"Descriptor review failed");
        auto version=alternate->GetVersion();alternate->SetVersion({99,0,0});
        W::HandheldDesignReviewOpen();choose(1);
        Check(!H::Designs::Completed() && state.park.cash==beforeCash && !H::Designs::HasReview(),"Changed descriptor accepted");
        capture("15-stale-descriptor-recovery.png",false);Ui::GetWindowManager()->CloseByClass(WindowClass::error);
        Check(!H::Designs::LastError().empty(),"Rejected native review detail lost");
        W::HandheldDesignOptionsOpen();choose(6);capture("21-stale-native-error-detail.png");press(H::Action::back);press(H::Action::back);
        alternate->SetVersion(version);
        Check(H::Designs::Review(site),"File review failed");
        GameActions::SavedTrackDesignPlaceAction bound(*H::Designs::ReviewLocation(),*H::Designs::Draft(),Config::Get().general.defaultInspectionInterval,H::Designs::ReviewCost(),true);
        bound.BindReview(TrackDesignPlacementContext::Capture(*H::Designs::Draft()),*H::FileBrowsing::Capture(file));
        DataSerialiser serialised(true);bound.Serialise(serialised);serialised.getStream().SetPosition(0);
        GameActions::SavedTrackDesignPlaceAction decoded;DataSerialiser decode(false,serialised.getStream());decoded.Serialise(decode);
        Check(GameActions::Query(&decoded,state).error==GameActions::Status::ok,"Bound action roundtrip rejected valid review");
        {std::ofstream append(file,std::ios::binary|std::ios::app);append<<"changed";}
        Check(GameActions::Query(&decoded,state).error!=GameActions::Status::ok
            && decoded.Execute(state,state.park).error!=GameActions::Status::ok,"Queued/serialised source identity bypassed");
        Check(H::Designs::Confirm().error!=GameActions::Status::ok && state.park.cash==beforeCash,"Changed file accepted");
        Check(RCT2::T6Exporter(exported).SaveTrack(file.string().c_str()),"Fixture source replacement failed");
        Check(H::BeginSavedDesign(file.string(),alternate->GetDescriptor()),"Reinspect source recovery failed");H::Designs::SetScenery(true);
        Check(H::Designs::Review(site),"Failure review failed");
        GameActions::CheatSetAction noFunds(CheatType::setMoney,0);Check(GameActions::Execute(&noFunds,state).error==GameActions::Status::ok,"Native funds fixture failed");GameActions::ProcessQueue(state);
        Check(H::Designs::Confirm().error!=GameActions::Status::ok && state.park.cash==0 && count(false)==beforeElements,"Native funds failure mutated map");
        W::HandheldDesignOptionsOpen();choose(6);capture("22-native-funds-failure-detail.png");press(H::Action::back);press(H::Action::back);
        GameActions::CheatSetAction funds(CheatType::setMoney,beforeCash);Check(GameActions::Execute(&funds,state).error==GameActions::Status::ok,"Native funds restore failed");GameActions::ProcessQueue(state);
        Check(H::Designs::Review(site),"Placement recovery review failed");const auto cost=H::Designs::ReviewCost();
        auto identity=H::FileBrowsing::Capture(file);Check(identity.has_value(),"Roundtrip identity missing");
        Check(H::Designs::Confirm().error==GameActions::Status::ok,"Native explicit placement failed");GameActions::ProcessQueue(state);
        Check(H::Designs::Completed().has_value(),"Native explicit placement missing");
        const auto id=H::Designs::Completed()->id;auto* placed=GetRide(id);
        Check(placed && placed->subtype==alternateIndex && state.park.cash==beforeCash-cost,"Actual native vehicle or accounting differs");
        Check(count(true)==0 && H::FileBrowsing::Current(*identity),"Placement changed source bytes or left ghost");
        capture("16-native-substitute-result.png",false);
        Check(H::Designs::Confirm().error!=GameActions::Status::ok && state.park.cash==beforeCash-cost,"Repeated placement executed");
        auto target=*H::Designs::Completed();H::ControllerInput explorer;H::UpdateExplorer(explorer);
        Check(H::FlatRides::CurrentStage()==H::FlatRides::Stage::entrance,"Design completion did not recover missing entrance");
        capture("17-native-entrance-handoff.png",false);
        for(int access=0;access<2;++access) {
            std::optional<CoordsXY> tile;
            for(int y=32;y<2016 && !tile;y+=32)for(int x=32;x<2016 && !tile;x+=32)
                if(H::FlatRides::Action(CoordsXY{x,y},false).error==GameActions::Status::ok)tile=CoordsXY{x,y};
            Check(tile.has_value(),"Native access location unavailable");const auto costBefore=state.park.cash;auto q=H::FlatRides::Action(tile,false);
            Check(H::FlatRides::Action(tile,true).error==GameActions::Status::ok,"Native access placement failed");GameActions::ProcessQueue(state);
            Check(state.park.cash==costBefore-q.cost,"Native entrance/exit accounting differs");
        }
        H::EndTerrain();
        for(const auto* name:{"rct2.footpath_surface.tarmac","rct2.footpath_surface.queue_blue","rct2.footpath_railings.wood"})Check(objects.LoadObject(name),"Native path fixture missing");
        auto access=H::RideAccess::Inspect(target);Check(access.has_value(),"Native station access inspection failed");
        for(bool exit:{false,true}) {
            Check(H::RideAccess::BeginPaths(target,exit,true),"Native access path handoff failed");
            auto q=H::Paths::Action(false);Check(q.error==GameActions::Status::ok && H::Paths::Action(true).error==GameActions::Status::ok,"Native access path failed");GameActions::ProcessQueue(state);H::EndTerrain();
        }
        access=H::RideAccess::Inspect(target);Check(access && access->entrance.attached && access->exit.attached,"Native access not attached");
        W::HandheldRideAccessOpen(target);capture("18-native-access-return.png");press(H::Action::back);Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        GameActions::RideSetStatusAction open(id,RideStatus::open);Check(GameActions::Execute(&open,state).error==GameActions::Status::ok,"Native substituted circuit cannot open");GameActions::ProcessQueue(state);
        Check(GetRide(id)->status==RideStatus::open,"Native Open not retained");W::HandheldRideOpen(id);capture("19-native-open-substituted-ride.png");Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        H::EndTerrain();
        // Native placement ignores a failed optional small-scenery subaction.
        // The report comes from those actual native actions, not a UI estimator.
        TrackDesign partial=exported;partial.trackAndVehicle.vehicleObject=alternate->GetDescriptor();
        auto availableOptional=missingOptional;availableOptional.sceneryObject=missingScenery->GetDescriptor();
        const auto sceneryIndex=objects.GetLoadedObjectEntryIndex(missingScenery);
        ScenerySetInvented({SCENERY_TYPE_SMALL,sceneryIndex});
        auto obstructedOptional=availableOptional;obstructedOptional.loc={-2048,0,0};
        partial.sceneryElements={availableOptional,obstructedOptional};
        const auto partialFile=tracks/"Partial native scenery.td6";
        Check(RCT2::T6Exporter(partial).SaveTrack(partialFile.string().c_str()),"Native optional scenery export failed");
        auto partialIdentity=H::FileBrowsing::Capture(partialFile);Check(partialIdentity.has_value(),"Partial source identity absent");
        Check(H::BeginSavedDesign(partialFile.string()),"Available original design rejected");H::Designs::SetScenery(true);
        const CoordsXY partialSite{640,1408};captureFocus={656,1424,112};
        const auto partialQuote=H::Designs::Query(partialSite);
        Check(partialQuote.error==GameActions::Status::ok && _trackDesignPlaceStateSceneryOmitted==1,"Native ignored-scenery query not reported");
        GameActions::SavedTrackDesignPlaceAction wrongOmissions({partialSite,112,0},*H::Designs::Draft(),Config::Get().general.defaultInspectionInterval,partialQuote.cost,true);
        wrongOmissions.BindReview(TrackDesignPlacementContext::Capture(*H::Designs::Draft()),*partialIdentity,0);
        Check(GameActions::Query(&wrongOmissions,state).error!=GameActions::Status::ok,"Changed native omission quote accepted");
        Check(H::Designs::Review(partialSite),"Partial scenery review unavailable");
        W::HandheldDesignReviewOpen();capture("23-native-partial-scenery-quote.png");press(H::Action::confirm);
        Check(H::Designs::Review(partialSite),"Partial scenery apply review unavailable");const auto partialCash=state.park.cash;
        Check(H::Designs::Confirm().error==GameActions::Status::ok,"Native partial result failed");GameActions::ProcessQueue(state);
        Check(H::Designs::Completed() && H::Designs::PlacementSceneryOmitted()==1 && state.park.cash==partialCash-partialQuote.cost,
            "Actual native omission/accounting differs");
        size_t actualScenery=0;for(int y=32;y<2016;y+=32)for(int x=32;x<2016;x+=32)
            for(auto* element:TileElementsView<SmallSceneryElement>(CoordsXY{x,y}))
                if(!element->isGhost() && element->getEntryIndex()==sceneryIndex)++actualScenery;
        Check(actualScenery==1 && H::FileBrowsing::Current(*partialIdentity),"Native partial scenery/source result differs");
        capture("24-native-partial-scenery-result.png",false);
        H::EndTerrain();captureFocus={1424,1424,112};const auto save=std::filesystem::path(argv[2])/"substitution.park";const auto savedCash=state.park.cash;
        Check(ScenarioSave(state,save.string(),{})!=0 && context->LoadParkFromFile(save.string()),"Substitute native roundtrip failed");
        Check(GetRide(id) && GetRide(id)->subtype==alternateIndex && state.park.cash==savedCash && !H::Designs::Active(),"Substitute save/load state differs");
        Check(GetRide(id)->status==RideStatus::open && H::FileBrowsing::Current(*identity),"Open/source state lost at reload");
        W::HandheldRideOpen(id);capture("20-native-reloaded-vehicle.png");
        Check(context->LoadParkFromFile(provisional.string()) && !GetRide(id) && count(true)==0 && count(false)==beforeElements
            && !H::Designs::Active() && !H::Designs::HasReview() && H::Designs::LastError().empty(),"Provisional autosave included draft/ghost or retained transient state");
        Check(context->LoadParkFromFile(save.string()) && GetRide(id)->subtype==alternateIndex && GetRide(id)->status==RideStatus::open,"Final park recovery after provisional load failed");
        std::cout<<"DesignSubstitutionTests Phase 1/2/3 PASS\n";
        return 0;
    }
    catch(const std::exception& e) { std::cerr<<e.what()<<std::endl;return 1; }
}

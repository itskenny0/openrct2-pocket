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
        Check(lines.back().find("ALL scenery")!=std::string::npos,"Native all-scenery fallback not explicit");
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
        auto file=std::filesystem::path(argv[2])/"missing-vehicle.td6";std::filesystem::create_directories(file.parent_path());
        Check(RCT2::T6Exporter(exported).SaveTrack(file.string().c_str()),"Native missing vehicle design export failed");
        const auto source=H::FileBrowsing::Capture(file);Check(source.has_value(),"Source identity absent");
        Check(!H::Designs::Begin(file.string()),"Missing required vehicle silently substituted");
        Check(H::BeginSavedDesign(file.string(),alternate->GetDescriptor()),"Explicit compatible substitute rejected");
        Check(H::Designs::Draft()->trackAndVehicle.vehicleObject==alternate->GetDescriptor(),"Explicit vehicle not retained");
        Check(H::Designs::Height(1),"Manual height unavailable");H::Designs::Rotate();
        const auto orientation=H::Designs::Orientation();const auto height=H::Designs::HeightOffset();
        Check(H::Designs::ChooseVehicle(vehicle->GetDescriptor()),"Explicit original compatible choice failed");
        Check(H::Designs::Orientation()==orientation && H::Designs::HeightOffset()==height,"Vehicle selection reset placement");
        Check(H::Designs::ChooseVehicle(alternate->GetDescriptor()),"Alternate reselection failed");
        H::Designs::AutomaticHeight();
        const CoordsXY site{1408,1408};
        auto count=[&](bool ghost){size_t n=0;for(int y=32;y<2016;y+=32)for(int x=32;x<2016;x+=32)for(auto* e:TileElementsView<>(CoordsXY{x,y}))if(e->isGhost()==ghost)++n;return n;};
        const auto beforeCash=state.park.cash;const auto beforeElements=count(false);const auto beforeRides=RideManager(state).size();
        H::Designs::SetScenery(true);
        const auto q=H::Designs::Query(site);if(q.error!=GameActions::Status::ok)std::cerr<<q.getErrorMessage()<<std::endl;
        Check(q.error==GameActions::Status::ok,"Native substituted query failed");
        Check(H::Designs::SceneryNotice().find("omitted")!=std::string::npos,"Missing optional scenery warning absent");
        Check(state.park.cash==beforeCash && count(false)==beforeElements && RideManager(state).size()==beforeRides,"Native query changed real park");
        H::Designs::UpdatePreview(site);Check(count(true)>0 && state.park.cash==beforeCash,"Native substituted preview missing or spent");
        H::Designs::ClearPreview();Check(count(true)==0,"Native substituted ghost cleanup failed");
        Check(H::Designs::Review(site),"Native substitute review failed");
        Check(!H::Designs::ChooseVehicle(vehicle->GetDescriptor()),"Reviewed vehicle mutated");
        W::HandheldDesignReviewOpen();
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
        Check(state.park.cash==beforeCash && count(false)==beforeElements,"Cancelled substitute mutated park");
        Check(H::Designs::Review(site),"Descriptor review failed");
        auto version=alternate->GetVersion();alternate->SetVersion({99,0,0});
        Check(H::Designs::Confirm().error!=GameActions::Status::ok && state.park.cash==beforeCash,"Changed descriptor accepted");
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
        GameActions::CheatSetAction funds(CheatType::setMoney,beforeCash);Check(GameActions::Execute(&funds,state).error==GameActions::Status::ok,"Native funds restore failed");GameActions::ProcessQueue(state);
        Check(H::Designs::Review(site),"Placement recovery review failed");const auto cost=H::Designs::ReviewCost();
        auto identity=H::FileBrowsing::Capture(file);Check(identity.has_value(),"Roundtrip identity missing");
        Check(H::Designs::Confirm().error==GameActions::Status::ok,"Native explicit placement failed");GameActions::ProcessQueue(state);
        Check(H::Designs::Completed().has_value(),"Native explicit placement missing");
        const auto id=H::Designs::Completed()->id;auto* placed=GetRide(id);
        Check(placed && placed->subtype==alternateIndex && state.park.cash==beforeCash-cost,"Actual native vehicle or accounting differs");
        Check(count(true)==0 && H::FileBrowsing::Current(*identity),"Placement changed source bytes or left ghost");
        Check(H::Designs::Confirm().error!=GameActions::Status::ok && state.park.cash==beforeCash-cost,"Repeated placement executed");
        H::EndTerrain();const auto save=std::filesystem::path(argv[2])/"substitution.park";const auto savedCash=state.park.cash;
        Check(ScenarioSave(state,save.string(),{})!=0 && context->LoadParkFromFile(save.string()),"Substitute native roundtrip failed");
        Check(GetRide(id) && GetRide(id)->subtype==alternateIndex && state.park.cash==savedCash && !H::Designs::Active(),"Substitute save/load state differs");
        std::cout<<"DesignSubstitutionTests Phase 1/2 PASS\n";
        return 0;
    }
    catch(const std::exception& e) { std::cerr<<e.what()<<std::endl;return 1; }
}

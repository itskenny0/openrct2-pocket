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
        std::cout<<"DesignSubstitutionTests Phase 1 PASS\n";
        return 0;
    }
    catch(const std::exception& e) { std::cerr<<e.what()<<std::endl;return 1; }
}

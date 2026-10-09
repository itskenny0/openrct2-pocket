// SPDX-License-Identifier: GPL-3.0-only
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/actions/ride/MazeSetTrackAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/ride/FlatRidePlacement.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <stdexcept>
using namespace OpenRCT2;
namespace M = Ui::Handheld::Mazes;
namespace GA = GameActions;
static void Check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}
static uint16_t Walls(CoordsXYZ loc, RideId id)
{
    for (auto* e : TileElementsView<TrackElement>(loc))
        if (!e->isGhost() && e->getRideIndex() == id && e->getBaseZ() == loc.z && e->getTrackType() == TrackElemType::maze)
            return e->getMazeEntry();
    return 0xFFFF;
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 3)
            return 2;
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = gOpenRCT2NoGraphics = true;
        auto context = CreateContext();
        Config::SetDefaults();
        Check(context->Initialise(), "Context failed");
        auto& state = getGameState();
        gameStateInitAll(state, { 64, 64 });
        gLegacyScene = LegacyScene::playing;
        state.park.cash = 10000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        // Scenario initialisation only. The border and tile (640,640) remain unowned.
        for (int y = 32; y < 2016; y += 32)
            for (int x = 32; x < 2016; x += 32)
                if (x != 640 || y != 640)
                    MapGetSurfaceElementAt(CoordsXY{ x, y })->setOwnership({ OwnershipFlag::landOwned });
        auto& objects = context->GetObjectManager();
        Check(objects.LoadObject("rct2.station.plain"), "Station missing");
        auto* object = objects.LoadObject("rct2.ride.hmaze");
        Check(object, "Maze missing");
        const auto index = objects.GetLoadedObjectEntryIndex(object);
        Check(!M::Eligible(index) && M::Entries().empty(), "Unresearched maze offered");
        RideTypeSetInvented(RIDE_TYPE_MAZE);
        RideEntrySetInvented(index);
        Check(M::Eligible(index) && M::Entries().size() == 1 && M::Begin(index), "Maze selection/create failed");
        GA::ProcessQueue(state);
        Check(M::Target().has_value(), "Missing target");
        auto t = *M::Target();
        M::Update(CoordsXY{ 320, 320 });
        const auto q = M::Build(false);
        const auto cash = state.park.cash;
        Check(q.error == GA::Status::ok && M::Build(true).error == GA::Status::ok, "Initial maze failed");
        Check(M::Build(true).error != GA::Status::ok, "Queued repeat accepted");
        GA::ProcessQueue(state);
        t = *M::Target();
        const auto first = *M::Cursor();
        const auto walls = Walls(first, t.id);
        Check(walls != 0xFFFF && state.park.cash == cash - q.cost && GetRide(t.id)->mazeTiles == 1, "Native walls/cost failed");
        M::Turn(2);
        Check(M::Build(true).error == GA::Status::ok, "Half cell failed");
        GA::ProcessQueue(state);
        Check(M::Cursor()->x == first.x + 16 && Walls(first, t.id) != walls, "Half tile/native segment unchanged");
        Check(M::Build(true).error == GA::Status::ok, "Second tile failed");
        GA::ProcessQueue(state);
        Check(GetRide(t.id)->mazeTiles == 2, "Native multi cell count wrong");
        M::Turn(2);
        Check(M::Move().error == GA::Status::ok, "Native move failed");
        GA::ProcessQueue(state);
        Check(M::ReviewRemoval().error == GA::Status::ok && M::Reviewing(), "Removal review failed");
        const auto before = Walls(first, t.id);
        Check(
            M::Confirm().error == GA::Status::ok && !M::Reviewing() && Walls(first, t.id) == before,
            "Cancel-first modified maze");
        Check(M::ReviewRemoval().error == GA::Status::ok, "Second review failed");
        M::ChooseRemoval();
        Check(M::Confirm().error == GA::Status::ok && M::Confirm().error != GA::Status::ok, "Remove repeat accepted");
        GA::ProcessQueue(state);
        Check(Walls(first, t.id) != before, "Native fill did not edit walls");
        M::Turn(2);
        // Revalidate a changed native geometry baseline after an external action.
        Check(M::ReviewRemoval().error == GA::Status::ok, "Stale review setup failed");
        M::ChooseRemoval();
        GA::MazeSetTrackAction external({ first.x + 16, first.y, first.z, 0 }, true, t.id, GA::MazeBuildMode::build);
        Check(GA::Execute(&external, state).error == GA::Status::ok, "External native action failed");
        GA::ProcessQueue(state);
        Check(M::Confirm().error != GA::Status::ok && !M::Reviewing(), "Stale geometry accepted");
        GA::MazeSetTrackAction missing({ 320, 320, first.z, 0 }, true, RideId::GetNull(), GA::MazeBuildMode::build);
        GA::MazeSetTrackAction odd({ 321, 320, first.z, 0 }, true, t.id, GA::MazeBuildMode::build);
        GA::MazeSetTrackAction bounds({ -16, 320, first.z, 0 }, true, t.id, GA::MazeBuildMode::build);
        GA::MazeSetTrackAction unowned({ 640, 640, first.z, 0 }, true, t.id, GA::MazeBuildMode::build);
        Check(
            GA::Query(&missing, state).error != GA::Status::ok && GA::Query(&odd, state).error != GA::Status::ok
                && GA::Query(&bounds, state).error != GA::Status::ok && GA::Query(&unowned, state).error != GA::Status::ok,
            "Native invalid guards failed");
        auto stale = t;
        stale.generation++;
        Check(!M::CanResume(stale), "Reused target accepted");
        M::End();
        Check(!M::Active() && M::Resume(t), "Resume failed");
        Check(FlatRidePlacement::SupportsStation(*GetRide(t.id)), "Maze access unsupported");
        bool access = false;
        for (Direction d = 0; d < 4; d++)
            access |= FlatRidePlacement::EntranceLocation(*GetRide(t.id), CoordsXY{ first } + CoordsDirectionDelta[d])
                          .has_value();
        Check(access, "Maze native adjacency unavailable");
        M::End();
        const auto save = std::string(argv[2]) + "/maze.park";
        SaveFlags flags{};
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Maze save/reload failed");
        Check(GetRide(t.id) && GetRide(t.id)->mazeTiles > 0 && Walls(first, t.id) != 0xFFFF, "Maze persistence failed");
        std::cout << "Native maze "
                     "availability/create/half-cells/multi-tiles/move/fill/cancel/stale/identity/bounds/ownership/cost/access/"
                     "save passed\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

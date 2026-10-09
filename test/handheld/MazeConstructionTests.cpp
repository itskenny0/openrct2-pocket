// SPDX-License-Identifier: GPL-3.0-only
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/Designs.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/ResultWithMessage.h>
#include <openrct2/actions/ride/MazeSetTrackAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/rct2/T6Exporter.h>
#include <openrct2/ride/FlatRidePlacement.h>
#include <openrct2/ride/TrackDesignRepository.h>
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
static size_t Ghosts()
{
    size_t count = 0;
    for (int y = 32; y < 2016; y += 32)
        for (int x = 32; x < 2016; x += 32)
            for (auto* e : TileElementsView<>(CoordsXY{ x, y }))
                count += e->isGhost();
    return count;
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
        namespace D = Ui::Handheld::Designs;
        GA::MazeSetTrackAction disconnected({ 800, 800, first.z, 0 }, false, t.id, GA::MazeBuildMode::build);
        Check(GA::Execute(&disconnected, state).error == GA::Status::ok, "Native disconnected fixture failed");
        GA::ProcessQueue(state);
        Check(M::Resume(t), "Resume for disconnected selection failed");
        M::ToggleCamera();
        M::Update(CoordsXY{ 640, 640 });
        Check(M::Picking() && M::SelectCell(false).error != GA::Status::ok, "Wrong maze pick accepted");
        M::Update(CoordsXY{ 800, 800 });
        M::Height(1);
        Check(
            M::SelectCell(false).error == GA::Status::ok && M::SelectCell(true).error == GA::Status::ok && M::Cursor()->x == 816
                && !M::Picking(),
            "Disconnected quarter selection failed");
        M::End();
        namespace F = Ui::Handheld::FlatRides;
        Check(F::Resume(t), "Maze access recovery failed");
        for (int n = 0; n < 2; n++)
        {
            std::optional<CoordsXY> accessTile;
            for (int y = 288; y <= 384 && !accessTile; y += 32)
                for (int x = 288; x <= 384 && !accessTile; x += 32)
                    if (F::Action(CoordsXY{ x, y }, false).error == GA::Status::ok)
                        accessTile = CoordsXY{ x, y };
            Check(accessTile && F::Action(accessTile, true).error == GA::Status::ok, "Maze access execute failed");
            GA::ProcessQueue(state);
        }
        F::End();
        TrackDesign td;
        TrackDesignState tds{};
        Check(
            td.CreateTrackDesign(tds, *GetRide(t.id)).Successful && !td.mazeElements.empty() && D::Eligible(td),
            "Native maze export/eligibility failed");
        auto invalid = td;
        invalid.mazeElements.clear();
        Check(!D::Eligible(invalid), "Empty maze design accepted");
        auto missingObject = td;
        missingObject.trackAndVehicle.vehicleObject = ObjectEntryDescriptor("rct2.ride.nonexistent");
        Check(!D::Eligible(missingObject), "Missing maze design object accepted");
        const auto tracks = std::filesystem::path(
            context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::trackDesigns));
        std::filesystem::create_directories(tracks);
        const auto file = (tracks / "Native maze regression.td6").string();
        Check(RCT2::T6Exporter(td).SaveTrack(file.c_str()), "Maze export failed");
        TrackRepositoryScan();
        Check(!D::Entries().empty() && D::Begin(file), "Native maze design picker/import failed");
        const CoordsXY designTile{ 960, 960 };
        for (int turn = 0; turn < 4; turn++)
        {
            Check(D::Query(designTile).error == GA::Status::ok, "Maze rotation/native height failed");
            D::UpdatePreview(designTile);
            Check(Ghosts() > 0, "Maze ghost not created");
            D::ClearPreview();
            Check(Ghosts() == 0, "Maze ghosts survived cleanup");
            D::Rotate();
        }
        D::SetScenery(false);
        Check(D::Review(designTile), "Maze review failed");
        const auto designCost = D::ReviewCost();
        const auto designCash = state.park.cash;
        D::CancelReview();
        Check(state.park.cash == designCash && !D::HasReview(), "Maze review cancellation spent money");
        Check(
            D::Review(designTile) && D::Confirm().error == GA::Status::ok && D::Confirm().error != GA::Status::ok,
            "Maze design confirm/repeat failed");
        GA::ProcessQueue(state);
        Check(
            D::Completed() && GetRide(D::Completed()->id)->mazeTiles > 0 && state.park.cash == designCash - designCost,
            "Native maze design accounting failed");
        const auto designTarget = *D::Completed();
        D::End();
        Check(M::CanResume(designTarget), "Design maze recovery unavailable");
        const auto save = std::string(argv[2]) + "/maze.park";
        SaveFlags flags{};
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Maze save/reload failed");
        Check(GetRide(t.id) && GetRide(t.id)->mazeTiles > 0 && Walls(first, t.id) != 0xFFFF, "Maze persistence failed");
        // A separate scenario fixture verifies native funds rejection without patching a live park.
        gameStateInitAll(state, { 64, 64 });
        gLegacyScene = LegacyScene::playing;
        state.park.cash = 0;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        MapGetSurfaceElementAt(CoordsXY{ 320, 320 })->setOwnership({ OwnershipFlag::landOwned });
        RideTypeSetInvented(RIDE_TYPE_MAZE);
        RideEntrySetInvented(index);
        Check(M::Begin(index), "Zero funds maze create failed");
        GA::ProcessQueue(state);
        M::Update(CoordsXY{ 320, 320 });
        Check(
            M::Build(false).error != GA::Status::ok && M::Build(true).error != GA::Status::ok && state.park.cash == 0
                && GetRide(M::Target()->id)->mazeTiles == 0,
            "Native maze insufficient funds bypassed");
        M::End();
        GA::ProcessQueue(state);
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

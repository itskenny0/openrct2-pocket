// SPDX-License-Identifier: GPL-3.0-only
// Public native scenario metadata. Construction is performed only by GameActions.
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Designs.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/ResultWithMessage.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/rct2/T6Exporter.h>
#include <openrct2/ride/TrackConstructionSupport.h>
#include <openrct2/ride/TrackDesignRepository.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/world/Scenery.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SmallSceneryElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <stdexcept>
using namespace OpenRCT2;
namespace C = Ui::Handheld::CustomTracks;
static void Check(bool ok, const char* text)
{
    if (!ok)
        throw std::runtime_error(text);
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
        for (int y = 32; y < 2016; y += 32)
            for (int x = 32; x < 2016; x += 32)
                MapGetSurfaceElementAt(CoordsXY{ x, y })->setOwnership({ OwnershipFlag::landOwned });
        auto& objects = context->GetObjectManager();
        Check(objects.LoadObject("rct2.station.plain"), "Station missing");
        auto* object = objects.LoadObject("rct2.ride.golf1");
        Check(object, "Mini Golf missing");
        const auto index = objects.GetLoadedObjectEntryIndex(object);
        RideTypeSetInvented(RIDE_TYPE_MINI_GOLF);
        RideEntrySetInvented(index);
        Check(C::Eligible(index, RIDE_TYPE_MINI_GOLF), "Mini Golf adapter eligibility failed");
        namespace E = Ui::Handheld::TrackEditing;
        for (int rotation = 0; rotation < 4; rotation++)
        {
            Check(C::Begin(index, RIDE_TYPE_MINI_GOLF), "Mini Golf creation failed");
            GameActions::ProcessQueue(state);
            for (int n = 0; n < rotation; n++)
                C::Rotate();
            C::Update(CoordsXY{ 320 + rotation * 384, 960 });
            Check(C::Action(true).error == GameActions::Status::ok, "Mini Golf station failed");
            GameActions::ProcessQueue(state);
            for (auto hole : { TrackElemType::minigolfHoleA, TrackElemType::minigolfHoleB, TrackElemType::minigolfHoleC,
                               TrackElemType::minigolfHoleD, TrackElemType::minigolfHoleE })
            {
                Check(C::Select(hole), "Native golf hole absent from picker");
                C::Update({});
                const auto q = C::Action(false);
                const auto cash = state.park.cash;
                Check(
                    q.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
                    "Golf hole placement failed");
                GameActions::ProcessQueue(state);
                Check(state.park.cash == cash - q.cost, "Golf hole cost incorrect");
                const auto t = *C::Target();
                const auto blocks = E::Blocks(t);
                Check(
                    std::count_if(blocks.begin(), blocks.end(), [hole](const auto& b) { return b.type == hole; }) == 1,
                    "Multi-tile golf block identity wrong");
                Check(C::Undo(false).error == GameActions::Status::ok, "Golf undo query failed");
                const auto refund = C::Undo(false).cost;
                Check(C::Undo().error == GameActions::Status::ok, "Golf hole removal failed");
                GameActions::ProcessQueue(state);
                Check(
                    state.park.cash == cash - q.cost - refund && C::CanResume(*C::Target()), "Golf refund/recovery incorrect");
            }
            Check(!C::Select(TrackElemType::leftVerticalLoop), "Mini Golf accepted coaster inversion");
            C::End();
        }
        Check(C::Begin(index, RIDE_TYPE_MINI_GOLF), "Golf circuit begin failed");
        GameActions::ProcessQueue(state);
        C::Update(CoordsXY{ 960, 320 });
        auto circuit = *C::Target();
        for (auto piece : { TrackElemType::endStation, TrackElemType::minigolfHoleA, TrackElemType::leftQuarterTurn1Tile,
                            TrackElemType::flat, TrackElemType::leftQuarterTurn1Tile, TrackElemType::flat, TrackElemType::flat,
                            TrackElemType::flat, TrackElemType::leftQuarterTurn1Tile, TrackElemType::flat,
                            TrackElemType::leftQuarterTurn1Tile })
        {
            Check(C::Select(piece), "Golf circuit piece absent");
            const auto q = C::Action(false);
            const auto before = state.park.cash;
            if (q.error != GameActions::Status::ok)
                std::cerr << q.getErrorMessage() << '\n';
            Check(
                q.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
                "Golf circuit action failed");
            GameActions::ProcessQueue(state);
            Check(state.park.cash == before - q.cost, "Golf circuit native cost differs");
        }
        Check(C::Completed().has_value(), "Golf circuit did not close");
        circuit = *C::Completed();
        C::End();
        namespace F = Ui::Handheld::FlatRides;
        Check(F::Resume(circuit), "Golf circuit access unavailable");
        for (int n = 0; n < 2; ++n)
        {
            std::optional<CoordsXY> tile;
            for (int y = 256; y <= 384 && !tile; y += 32)
                for (int x = 896; x <= 1024 && !tile; x += 32)
                    if (F::Action(CoordsXY{ x, y }, false).error == GameActions::Status::ok)
                        tile = CoordsXY{ x, y };
            Check(tile && F::Action(tile, true).error == GameActions::Status::ok, "Golf circuit access failed");
            GameActions::ProcessQueue(state);
        }
        F::End();
        GameActions::RideSetStatusAction open(circuit.id, RideStatus::open);
        auto opening = GameActions::Query(&open, state);
        if (opening.error != GameActions::Status::ok)
            std::cerr << opening.getErrorMessage() << '\n';
        Check(
            opening.error == GameActions::Status::ok && GameActions::Execute(&open, state).error == GameActions::Status::ok,
            "Native golf opening failed");
        GameActions::ProcessQueue(state);
        Check(GetRide(circuit.id)->status == RideStatus::open, "Golf native status not open");
        const auto save = std::string(argv[2]) + "/mini-golf.park";
        SaveFlags flags{};
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Golf station persistence failed");
        Check(GetRide(circuit.id) && GetRide(circuit.id)->status == RideStatus::open, "Open golf circuit did not persist");
        std::cout << "Native Mini Golf all five holes / four orientations / multi-tile identity / costs / undo refunds / "
                     "recovery / native closed circuit-access-open / save passed\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

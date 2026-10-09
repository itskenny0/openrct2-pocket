// SPDX-License-Identifier: GPL-3.0-only
// Public native scenario metadata. Construction is performed only by GameActions.
#include <iostream>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/config/Config.h>
#include <openrct2/ride/TrackConstructionSupport.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/world/TileElementsView.h>
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
static size_t Ghosts()
{
    size_t n = 0;
    for (int y = 32; y < 2016; y += 32)
        for (int x = 32; x < 2016; x += 32)
            for (auto* e : TileElementsView<>(CoordsXY{ x, y }))
                n += e->isGhost();
    return n;
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
        const char* ids[] = { "rct2.ride.ptct1", "rct2.ride.scht1", "rct2.ride.arrt1", "rct2.ride.bmsd", "rct2.ride.mono1" };
        int family = 0;
        for (const auto* id : ids)
        {
            auto* object = objects.LoadObject(id);
            Check(object, "Ride metadata missing");
            const auto index = objects.GetLoadedObjectEntryIndex(object);
            const auto* entry = GetRideEntryByIndex(index);
            ride_type_t type = kRideTypeNull;
            for (auto t : entry->ride_type)
                if (TrackConstructionSupport::Type(t))
                {
                    type = t;
                    break;
                }
            Check(type != kRideTypeNull, "Conventional descriptor missing");
            RideTypeSetInvented(type);
            RideEntrySetInvented(index);
            Check(C::Eligible(index, type) && !C::Eligible(index, RIDE_TYPE_MAZE), "Eligibility type mismatch");
            Check(C::Begin(index, type), "Begin failed");
            GameActions::ProcessQueue(state);
            const CoordsXY tile{ 320, 320 + family * 256 };
            C::Update(tile);
            auto target = C::Target();
            Check(target && GetRide(target->id)->type == type, "Wrong created type");
            const auto q = C::Action(false);
            const auto cash = state.park.cash;
            Check(q.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok, "Station failed");
            Check(C::Action(true).error != GameActions::Status::ok, "Pending repeat accepted");
            GameActions::ProcessQueue(state);
            Check(state.park.cash == cash - q.cost, "Station cost differs");
            Check(C::Select(TrackElemType::flat), "Straight absent");
            C::Update({});
            Check(C::Action(true).error == GameActions::Status::ok, "Straight failed");
            GameActions::ProcessQueue(state);
            if (GetRide(target->id)->getRideTypeDescriptor().SupportsTrackGroup(TrackGroup::verticalLoop)
                && RideEntryGetSupportedTrackPieces(*entry)[static_cast<size_t>(TrackGroup::verticalLoop)])
            {
                Check(C::Select(TrackElemType::flatToUp25), "Inversion approach absent");
                Check(C::Action(true).error == GameActions::Status::ok, "Inversion approach failed");
                GameActions::ProcessQueue(state);
                Check(C::Select(TrackElemType::leftVerticalLoop), "Native inversion picker absent");
                Check(C::Action(true).error == GameActions::Status::ok, "Native inversion placement failed");
                GameActions::ProcessQueue(state);
                Check(C::Undo(true).error == GameActions::Status::ok, "Native inversion undo failed");
                GameActions::ProcessQueue(state);
                Check(C::Undo(true).error == GameActions::Status::ok, "Inversion approach undo failed");
                GameActions::ProcessQueue(state);
            }
            Check(C::Select(TrackElemType::leftQuarterTurn3Tiles), "Turn absent");
            const auto turn = C::Action(false);
            Check(turn.error == GameActions::Status::ok, "Native turn query failed");
            Check(C::Action(true).error == GameActions::Status::ok, "Turn failed");
            GameActions::ProcessQueue(state);
            const auto refund = C::Undo(false);
            const auto before = state.park.cash;
            Check(
                refund.error == GameActions::Status::ok && C::Undo(true).error == GameActions::Status::ok, "Turn undo failed");
            GameActions::ProcessQueue(state);
            Check(state.park.cash == before - refund.cost, "Native refund differs");
            Check(!C::Select(TrackElemType::maze), "Distinct maze geometry offered");
            target = C::Target();
            C::End();
            Check(!Ghosts() && C::Resume(*target), "Cleanup/resume failed");
            C::End();
            ++family;
        }
        const auto save = std::string(argv[2]) + "/ride-breadth.park";
        SaveFlags flags{};
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Breadth save/reload failed");
        std::cout << "Native five-family construction / native stations-straights-multitile-turns / query-repeat-cost-refund / "
                     "unsupported geometry / ghosts-resume-save passed\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

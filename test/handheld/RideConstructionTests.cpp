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
        auto* sceneryObject = objects.LoadObject("rct2.scenery_small.corroof");
        Check(sceneryObject, "Design scenery fixture missing");
        const auto sceneryIndex = objects.GetLoadedObjectEntryIndex(sceneryObject);
        ScenerySetInvented({ SCENERY_TYPE_SMALL, sceneryIndex });
        const auto tracks = std::filesystem::path(
            context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::trackDesigns));
        std::filesystem::create_directories(tracks);
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
            TrackDesign td;
            TrackDesignState tdState{};
            Check(td.CreateTrackDesign(tdState, *GetRide(target->id)).Successful, "Native design export failed");
            td.sceneryElements.push_back({ sceneryObject->GetDescriptor(), { 64, 64, 0 } });
            const auto file = (tracks / ("Native family " + std::to_string(family) + ".td6")).string();
            Check(RCT2::T6Exporter(td).SaveTrack(file.c_str()), "Native design write failed");
            namespace D = Ui::Handheld::Designs;
            Check(D::Begin(file), "Non-Wooden/scenery design unavailable");
            D::SetScenery(false);
            const CoordsXY destination{ 960, tile.y };
            Check(D::Review(destination), "Automatic design height review failed");
            const auto automaticLocation = *D::ReviewLocation();
            D::CancelReview();
            Check(D::Height(1) && D::HeightOffset() == 16 && D::Review(destination), "Manual design elevation failed");
            Check(D::ReviewLocation()->z == automaticLocation.z + 16 && !D::Height(1), "Manual/review height changed");
            D::AutomaticHeight();
            Check(D::HeightOffset() == 16, "Reviewed height reset");
            D::CancelReview();
            D::AutomaticHeight();
            Check(!D::HeightOffset(), "Automatic height reset failed");
            for (int n = 0; n < 16; ++n)
                Check(D::Height(1), "Manual height upper range failed");
            Check(!D::Height(1) && D::HeightOffset() == 256, "Manual height upper bound failed");
            for (int n = 0; n < 24; ++n)
                Check(D::Height(-1), "Manual height lower range failed");
            Check(!D::Height(-1) && D::HeightOffset() == -128, "Manual height lower bound failed");
            Check(D::Query(destination).error != GameActions::Status::ok, "Invalid manual native height accepted");
            D::AutomaticHeight();
            Check(D::Height(1), "Manual execution draft failed");
            const auto without = D::Query(destination);
            D::SetScenery(true);
            const auto with = D::Query(destination);
            Check(
                without.error == GameActions::Status::ok && with.error == GameActions::Status::ok && with.cost > without.cost,
                "Native scenery query cost differs");
            D::UpdatePreview(destination);
            Check(Ghosts(), "Scenery design ghost missing");
            D::ClearPreview();
            Check(!Ghosts(), "Scenery design ghost cleanup failed");
            Check(D::Review(destination) && D::SceneryNotice().find("On") != std::string::npos, "Scenery design review failed");
            D::SetScenery(false);
            Check(D::SceneryNotice().find("On") != std::string::npos, "Review scenery mutated");
            const auto designCash = state.park.cash;
            Check(
                D::Confirm().error == GameActions::Status::ok && D::Confirm().error != GameActions::Status::ok,
                "Design confirm/repeat failed");
            GameActions::ProcessQueue(state);
            Check(D::Completed() && state.park.cash == designCash - with.cost, "Design type/cost execute differs");
            Check(GetRide(D::Completed()->id)->type == type, "Design silently substituted native type");
            size_t sceneryCount = 0;
            for (int sy = 32; sy < 2016; sy += 32)
                for (int sx = 32; sx < 2016; sx += 32)
                    for (auto* e : TileElementsView<SmallSceneryElement>(CoordsXY{ sx, sy }))
                        sceneryCount += !e->isGhost() && e->getEntryIndex() == sceneryIndex;
            Check(sceneryCount == static_cast<size_t>(family + 1), "Native design scenery not placed");
            D::End();
            ++family;
        }
        TrackRepositoryScan();
        Check(Ui::Handheld::Designs::Entries().size() == 5, "Native repository omitted supported family designs");
        namespace E = Ui::Handheld::TrackEditing;
        auto original = Ui::Handheld::RideManagement::Identify(RideId::FromUnderlying(0));
        Check(original && C::Resume(*original), "Editing fixture resume failed");
        Check(C::Select(TrackElemType::brakes), "Native brake selection failed");
        Check(
            !C::SetPlacementSpeed(1) && !C::SetPlacementSpeed(31) && C::SetPlacementSpeed(12), "Placement speed bounds failed");
        Check(C::Action(true).error == GameActions::Status::ok, "Native brake build failed");
        GameActions::ProcessQueue(state);
        original = C::Target();
        C::End();
        auto blocks = E::Blocks(*original);
        auto findBrake = [&]() {
            return *std::find_if(blocks.begin(), blocks.end(), [](const auto& b) { return b.type == TrackElemType::brakes; });
        };
        auto brake = findBrake();
        Check(brake.speed == 12, "Native placement speed not persisted");
        const auto baseline = state.park.cash;
        Check(
            E::Change(brake, 8).error == GameActions::Status::ok && E::Change(brake, 31).error != GameActions::Status::ok,
            "Native speed bounds failed");
        Check(E::Change(brake, 8, 0, true).error == GameActions::Status::ok, "Native speed execute failed");
        GameActions::ProcessQueue(state);
        Check(!E::Valid(brake) && state.park.cash == baseline, "Speed stale baseline/accounting failed");
        blocks = E::Blocks(*original);
        brake = findBrake();
        Check(brake.speed == 8, "Native brake value not changed");
        // Remove a middle block with a following brake; endpoint-only undo must reject it.
        const auto middle = *std::find_if(
            blocks.begin(), blocks.end(), [](const auto& b) { return b.type == TrackElemType::flat; });
        GameActions::SequentialTrackPlaceAction endpoint(
            middle.ride.id, middle.type, middle.origin, middle.chain, false, middle.origin, middle.type);
        endpoint.UseRemoval();
        Check(GameActions::Query(&endpoint, state).error != GameActions::Status::ok, "Endpoint guard lost");
        const auto removal = E::Change(middle, {});
        Check(
            removal.error == GameActions::Status::ok
                && E::Change(middle, {}, removal.cost + 1, true).error != GameActions::Status::ok,
            "Selected removal cost guard failed");
        Check(E::Change(middle, {}, removal.cost, true).error == GameActions::Status::ok, "Selected middle removal failed");
        GameActions::ProcessQueue(state);
        Check(
            !E::Valid(middle) && E::Change(middle, {}, removal.cost, true).error != GameActions::Status::ok
                && state.park.cash == baseline - removal.cost && C::CanResume(*original),
            "Selected refund/repeat/gap recovery failed");
        Check(C::ResumeFrom(brake.ride, std::pair{ brake.origin, brake.type }), "Disconnected section resume failed");
        Check(C::Endpoint() && C::Select(TrackElemType::flat), "Disconnected native endpoint missing");
        const auto sectionQuote = C::Action(false);
        Check(
            sectionQuote.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
            "Disconnected extension failed");
        GameActions::ProcessQueue(state);
        Check(C::Undo().error == GameActions::Status::ok, "Disconnected extension undo failed");
        GameActions::ProcessQueue(state);
        Check(C::Active() && C::Endpoint(), "Selected section anchor lost after undo");
        Check(C::Undo().error == GameActions::Status::ok, "Selected anchor removal failed");
        GameActions::ProcessQueue(state);
        Check(!C::Active() && !Ghosts(), "Removed anchor jumped to unrelated section");
        Check(!C::ResumeFrom(brake.ride, std::pair{ brake.origin, brake.type }), "Missing anchor resumed");
        auto stale = brake;
        stale.ride.generation++;
        Check(!E::Valid(stale) && E::Change(stale, 8).error != GameActions::Status::ok, "Reused ride identity accepted");
        const auto save = std::string(argv[2]) + "/ride-breadth.park";
        SaveFlags flags{};
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Breadth save/reload failed");
        std::cout << "Native five-family construction / native stations-straights-multitile-turns / query-repeat-cost-refund / "
                     "unsupported geometry / selected-block-refund-gap / brake-baseline-bounds / ghosts-resume-save passed\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

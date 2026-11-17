#include "PeopleTabNavigation.h"
// SPDX-License-Identifier: GPL-3.0-only
// Native map fixture, real SDL controller routing and production map rendering.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/GuestFollow.h>
#include <openrct2-ui/handheld/GuestManagement.h>
#include <openrct2-ui/handheld/ManagementNews.h>
#include <openrct2-ui/handheld/MapSubjects.h>
#include <openrct2-ui/handheld/ParkInformation.h>
#include <openrct2-ui/handheld/ParkMap.h>
#include <openrct2-ui/handheld/ParkOverview.h>
#include <openrct2-ui/handheld/PeepPlacement.h>
#include <openrct2-ui/handheld/ResearchCatalog.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/windows/NativeMap.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/general/MapChangeSizeAction.h>
#include <openrct2/actions/park/ParkMarketingAction.h>
#include <openrct2/actions/park/ParkSetResearchFundingAction.h>
#include <openrct2/actions/peep/StaffFireAction.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/track/TrackPlaceAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Window.h>
#include <openrct2/management/Marketing.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/Track.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/Park.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
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
        gameStateInitAll(state, { 64, 48 });
        gLegacyScene = LegacyScene::playing;
        Check(W::MainOpen(), "Main viewport missing");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        namespace G = H::GuestManagement;
        state.park.cash = 100000.00_GBP;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        for (int y = 32; y < 47 * 32; y += 32)
            for (int x = 32; x < 63 * 32; x += 32)
                MapGetSurfaceElementAt(CoordsXY{ x, y })
                    ->setOwnership({ OwnershipFlag::landOwned }); // initial scenario metadata only
        Check(context->GetObjectManager().LoadObject("rct2.peep_animations.guest"), "Guest metadata missing");
        auto* person = Guest::generate({ 640, 640, 16 });
        Check(person, "Guest generation failed");
        person->outsideOfPark = false;
        person->setParkEntryTime(state.currentTicks);
        const auto target = *G::Identify(person->id);
        Check(G::RideHistory(target).empty(), "Empty history fabricated");
        Check(
            G::SetName(target, person->getName(), "Guest Filter Fixture", true).error == GameActions::Status::ok,
            "Native filter fixture name failed");
        GameActions::ProcessQueue(state);
        auto* object = context->GetObjectManager().LoadObject("rct2.ride.ptct1");
        Check(object && context->GetObjectManager().LoadObject("rct2.station.plain"), "Ride metadata missing");
        const auto index = context->GetObjectManager().GetLoadedObjectEntryIndex(object);
        RideTypeSetInvented(RIDE_TYPE_WOODEN_ROLLER_COASTER);
        RideEntrySetInvented(index);
        for (int i = 0; i < 3; ++i)
        {
            GameActions::RideCreateAction create(
                RIDE_TYPE_WOODEN_ROLLER_COASTER, index, 0, 0, 0, RideInspection::every30Minutes);
            Check(GameActions::Execute(&create, state).error == GameActions::Status::ok, "Ride create failed");
            GameActions::ProcessQueue(state);
            const auto rides = H::RideManagement::Roster();
            auto id = rides.back().id;
            CoordsXYZD origin{ 320, 320 + i * 256, MapGetSurfaceElementAt(CoordsXY{ 320, 320 + i * 256 })->getBaseZ(), 0 };
            GameActions::TrackPlaceAction track(
                id, TrackElemType::endStation, RIDE_TYPE_WOODEN_ROLLER_COASTER, origin, 0, 0, 0, {}, false);
            Check(GameActions::Execute(&track, state).error == GameActions::Status::ok, "Native fixture track failed");
            GameActions::ProcessQueue(state);
        }
        const auto rides = H::RideManagement::Roster();
        Check(rides.size() == 3, "Native ride fixture missing");
        person->setHasRidden(*GetRide(rides[0].id));
        person->setHasRidden(*GetRide(rides[2].id)); // native history API fixture membership, not invented dates
        const auto cash = state.park.cash;
        const auto accounting = state.park.expenditureTable;
        const auto history = G::RideHistory(target);
        Check(
            history.size() == 2 && history[0] == rides[0] && history[1] == rides[2], "Native history membership/order differs");
        H::ControllerInput input;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm };
        H::Frame cancel;
        cancel.actions = { H::Action::back };
        W::HandheldGuestOpen(target);
        FocusPersonAction(false, "Ride history...");
        H::HandleMenuInput(input, confirm);
        Check(W::HandheldGuestsPresentation().breadcrumb == "Guests / Ride history", "Controller history entry failed");
        H::HandleMenuInput(input, confirm);
        Check(W::HandheldRidesIsOpen(), "History inspection failed");
        H::HandleMenuInput(input, cancel);
        Check(W::HandheldGuestsPresentation().breadcrumb == "Guests / Ride history", "History return failed");
        H::HandleMenuInput(input, cancel);
        // Native GuestList fixture: each state uses the desktop action/thought formatter contract.
        const auto savedState = person->state;
        const auto savedRide = person->currentRide;
        const auto savedThought = person->thoughts[0];
        G::Filter rideFilter;
        rideFilter.ride = G::RideFilter{ rides[0], G::RideFilterKind::riders };
        person->state = PeepState::onRide;
        person->currentRide = rides[0].id;
        Check(G::Roster(rideFilter) == std::vector<G::Target>{ target }, "Rider native membership failed");
        person->currentRide = rides[1].id;
        Check(G::Roster(rideFilter).empty(), "Other ride matched rider filter");
        person->currentRide = rides[0].id;
        person->state = PeepState::queuing;
        Check(G::Roster(rideFilter).empty(), "Queue matched rider filter");
        rideFilter.ride->kind = G::RideFilterKind::queue;
        Check(G::Roster(rideFilter) == std::vector<G::Target>{ target }, "Queue native membership failed");
        person->state = PeepState::queuingFront;
        Check(G::Roster(rideFilter).size() == 1, "Queue front excluded");
        person->state = savedState;
        person->currentRide = savedRide;
        rideFilter.ride->kind = G::RideFilterKind::thoughts;
        person->thoughts[0].type = PeepThoughtType::wasGreat;
        person->thoughts[0].rideId = rides[0].id;
        person->thoughts[0].freshness = 5;
        Check(G::Roster(rideFilter) == std::vector<G::Target>{ target }, "Native thought wildcard membership failed");
        person->thoughts[0].freshness = 6;
        Check(G::Roster(rideFilter).empty(), "Stale thought matched filter");
        person->thoughts[0].freshness = 1;
        person->thoughts[0].rideId = rides[1].id;
        Check(G::Roster(rideFilter).empty(), "Other ride thought matched filter");
        person->thoughts[0].rideId = rides[0].id;
        rideFilter.tracked = true;
        Check(G::Roster(rideFilter).empty(), "Untracked guest matched intersection");
        const auto originalFlags = person->peepFlags;
        person->peepFlags.set(PeepFlag::tracking);
        Check(G::Roster(rideFilter).size() == 1, "Tracked ride intersection failed");
        rideFilter.tracked = false;
        const auto thoughtGroups = G::Groups(G::GroupMode::thoughts);
        Check(!thoughtGroups.empty(), "Thought group fixture absent");
        rideFilter.group = thoughtGroups.front();
        Check(G::Roster(rideFilter).size() == 1, "Group ride intersection failed");
        rideFilter.group.reset();
        rideFilter.name = "gUeSt";
        Check(G::Roster(rideFilter).size() == 1, "Case-insensitive native name intersection failed");
        rideFilter.name = "no such guest name";
        Check(G::Roster(rideFilter).empty(), "Nonmatching name admitted guest");
        rideFilter.name.clear();
        const auto subjects = H::MapSubjects::Roster(2, 0, rideFilter);
        Check(subjects.size() == 1 && std::get<G::Target>(subjects.front()) == target, "Filtered map differs from roster");
        const auto markers = W::NativeMiniMapPeepMarkers(false, false, -1);
        Check(
            std::any_of(markers.begin(), markers.end(), [&](const auto& m) { return m.id == target.id; }),
            "Native map marker identity missing");
        auto choose = [&](const H::ManagementPresentation& model, const std::string& row) {
            const auto it = std::find(model.rows.begin(), model.rows.end(), row);
            Check(it != model.rows.end(), "Controller row missing");
            W::HandheldMenuMove(static_cast<int>(it - model.rows.begin()) - static_cast<int>(model.selected));
            H::HandleMenuInput(input, confirm);
        };
        W::HandheldGuestOpen(target);
        FocusPersonAction(false, "Ride history...");
        H::HandleMenuInput(input, confirm);
        H::HandleMenuInput(input, confirm);
        W::HandheldDashboardPage(2);
        choose(W::HandheldRidesPresentation(), "Customers...");
        choose(W::HandheldRidesPresentation(), "Guests thinking about ride...");
        Check(W::HandheldGuestsPresentation().rows[3] == person->getName(), "Ride filter controller roster failed");
        choose(W::HandheldGuestsPresentation(), "Name filter...");
        choose(W::HandheldGuestsPresentation(), "G");
        choose(W::HandheldGuestsPresentation(), "Done");
        Check(W::HandheldGuestsPresentation().rows[3] == person->getName(), "Controller name filter apply failed");
        choose(W::HandheldGuestsPresentation(), "Name filter...");
        choose(W::HandheldGuestsPresentation(), "Z");
        H::HandleMenuInput(input, cancel);
        Check(W::HandheldGuestsPresentation().rows[3] == person->getName(), "Cancelled name draft changed membership");
        choose(W::HandheldGuestsPresentation(), "Tracked only");
        Check(W::HandheldGuestsPresentation().rows[3] == person->getName(), "Controller tracked intersection failed");
        choose(W::HandheldGuestsPresentation(), "Map roster");
        Check(W::HandheldParkMapIsOpen(), "Filtered roster map entry failed");
        W::HandheldParkMapSubjects();
        Check(W::HandheldParkMapSubjectsPresentation().rows.front() == person->getName(), "Map subject filter lost");
        H::HandleMenuInput(input, confirm);
        choose(W::HandheldParkMapSubjectsPresentation(), "Inspect subject");
        Check(W::HandheldGuestsIsOpen(), "Filtered map inspection failed");
        H::HandleMenuInput(input, cancel);
        Check(W::HandheldParkMapSubjectsPresentation().title == person->getName(), "Map inspection return context lost");
        H::HandleMenuInput(input, cancel);
        H::HandleMenuInput(input, cancel);
        Check(W::HandheldParkMapIsOpen(), "Map subject return failed");
        H::HandleMenuInput(input, cancel);
        Check(W::HandheldGuestsPresentation().title.find("Thinking / ") == 0, "Map return lost roster filter");
        choose(W::HandheldGuestsPresentation(), "Name filter...");
        choose(W::HandheldGuestsPresentation(), "Clear");
        choose(W::HandheldGuestsPresentation(), "Done");
        H::HandleMenuInput(input, cancel);
        Check(W::HandheldRidesPresentation().breadcrumb == "Rides / Customers", "Filtered roster return lost ride context");
        person->thoughts[0] = savedThought;
        person->peepFlags = originalFlags;
        W::HandheldGuestOpen(target);
        namespace F = H::GuestFollow;
        const auto personPosition = G::Resolve(target)->getLocation();
        FocusPersonAction(false, "Follow guest");
        H::HandleMenuInput(input, confirm);
        Check(
            F::Current() == target && WindowGetMain()->viewportSmartFollowSprite == target.id,
            "Controller follow start failed");
        H::HandleExplorerInput(input, cancel);
        Check(W::HandheldGuestsIsOpen() && F::Current() == target, "Follow inspector recovery failed");
        FocusPersonAction(false, "Stop following");
        H::HandleMenuInput(input, confirm);
        Check(!F::Current() && WindowGetMain()->viewportSmartFollowSprite.IsNull(), "Explicit follow stop failed");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        Check(F::Start(target), "Native follow restart failed");
        if (argc == 4)
        {
            auto* native = dynamic_cast<Drawing::X8DrawingEngine*>(context->GetDrawingEngine());
            Check(native, "Native drawing context missing");
            native->Drawing::X8DrawingEngine::Resize(512, 384);
            native->Drawing::X8DrawingEngine::BeginDraw();
            ViewportUpdatePosition(WindowGetMain());
            W::MainWindowZoom(true, false);
            ViewportRotateSingle(WindowGetMain(), 1);
            ViewportUpdatePosition(WindowGetMain());
            native->Drawing::X8DrawingEngine::EndDraw();
            Check(
                F::Current() == target && G::Resolve(target)->getLocation() == personPosition,
                "Native follow zoom/rotation changed person or follow");
        }
        H::Frame pan;
        pan.panX = 1;
        H::HandleExplorerInput(input, pan);
        Check(!F::Current(), "Controller pan did not stop follow");
        Check(H::PeepPlacement::Begin({ target, {} }).error == GameActions::Status::ok, "Follow pickup fixture failed");
        GameActions::ProcessQueue(state);
        Check(!F::Start(target), "Picked guest follow accepted");
        H::PeepPlacement::End();
        Check(F::Start(target), "Follow before load failed");
        SaveFlags flags{};
        const auto save = std::string(argv[2]) + "/guest-history.park";
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "History save/load failed");
        Check(
            G::RideHistory(target).size() == 2 && !Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu),
            "History persistence/teardown failed");
        Check(!F::Current(), "Load retained native follow guard");
        Check(state.park.cash == cash && state.park.expenditureTable == accounting, "History reads changed accounting");
        const auto old = G::RideHistory(target).front();
        GameActions::RideDemolishAction remove(old.id, GameActions::RideModifyType::demolish);
        Check(GameActions::Execute(&remove, state).error == GameActions::Status::ok, "Ride removal failed");
        GameActions::ProcessQueue(state);
        rideFilter.ride = G::RideFilter{ old, G::RideFilterKind::riders };
        Check(G::Roster(rideFilter).empty(), "Removed ride filter retained members");
        Check(!G::HistoryRideCurrent(target, old) && G::RideHistory(target).size() == 1, "Removed history ride retained");
        GameActions::RideCreateAction replacement(
            RIDE_TYPE_WOODEN_ROLLER_COASTER, index, 0, 0, 0, RideInspection::every30Minutes);
        Check(GameActions::Execute(&replacement, state).error == GameActions::Status::ok, "Replacement failed");
        GameActions::ProcessQueue(state);
        Check(!G::HistoryRideCurrent(target, old), "Reused ride revived old history");
        Check(G::Roster(rideFilter).empty(), "Reused ride revived stale filter");
        Check(F::Start(target), "Follow before departure failed");
        auto* departing = state.entities.tryGetEntity<Guest>(target.id);
        departing->remove();
        Check(
            !F::Current() && !F::Start(target) && WindowGetMain()->viewportSmartFollowSprite.IsNull(),
            "Departed native guest retained follow");
        auto* reused = Guest::generate(personPosition);
        Check(reused && reused->id == target.id, "Native peep slot was not reused");
        reused->outsideOfPark = false;
        reused->peepId = target.peepId;
        reused->setParkEntryTime(target.entryTime); // Replacement fixture deliberately matches native baseline fields.
        Check(!F::Start(target), "Reused peep with matching fields revived stale follow");
        const auto replacementPerson = *G::Identify(reused->id);
        Check(F::Start(replacementPerson), "Fresh reused peep identity rejected");
        Ui::GetWindowManager()->Cleanup();
        Check(!F::Current(), "Native window-manager shutdown retained follow");
        Check(G::RideHistory(target).empty(), "Stale guest history accepted");
        std::cout << "Native guest history membership/order/empty/removal-reuse/controller-return/save passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

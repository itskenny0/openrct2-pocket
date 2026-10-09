// SPDX-License-Identifier: GPL-3.0-only
// Native private scenario, production picker/tool/shell rendering; no fixture objects or research.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Designs.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRegistry.h>
#include <openrct2/actions/ResultWithMessage.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/park/ParkSetLoanAction.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/ride/RideEntranceExitRemoveAction.h>
#include <openrct2/actions/track/SavedTrackDesignPlaceAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/entity/EntityList.h>
#include <openrct2/entity/Guest.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/rct2/T6Exporter.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/TrackDesignRepository.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <queue>
#include <set>
#include <stdexcept>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace F = H::FlatRides;
static void Check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 6)
            return 2;
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[5];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = false;
        auto env = CreatePlatformEnvironment();
        env->SetBasePath(DirBase::rct2, argv[2]);
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        Config::Get().general.rct2Path = argv[2];
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Config::Get().general.windowScale = 1;
        Check(context->Initialise() && context->LoadParkFromFile(argv[3], false, true), "Scenario load failed");
        context->GetUiContext().CreateWindow();
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        Ui::GetWindowManager()->CloseByClass(WindowClass::error);
        gGamePaused = 0;
        auto& state = getGameState();
        std::filesystem::create_directories(argv[4]);
        const auto tracks = std::filesystem::path(
            context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::trackDesigns));
        std::filesystem::create_directories(tracks);
        std::cout << "Scenario " << state.scenarioOptions.name << " cash " << state.park.cash << " map " << state.mapSize.x
                  << "x" << state.mapSize.y << std::endl;
        for (auto& ride : RideManager(state))
        {
            std::cout << "Ride " << ride.id.ToUnderlying() << " " << ride.getName() << " type " << ride.type << std::endl;
            if (ride.type != RIDE_TYPE_WOODEN_ROLLER_COASTER)
                continue;
            TrackDesign td;
            TrackDesignState tds{};
            const auto result = td.CreateTrackDesign(tds, ride);
            std::cout << "Export result " << result.Successful << " pieces " << td.trackElements.size() << " entrances "
                      << td.entranceElements.size() << " eligible " << H::Designs::Eligible(td) << std::endl;
            if (result.Successful)
                Check(
                    RCT2::T6Exporter(td).SaveTrack((tracks / "Native scenario wooden coaster.td6").string().c_str()),
                    "Export failed");
        }
        TrackRepositoryScan();
        auto entries = H::Designs::Entries();
        const auto exported = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {
            return entry.identity.key == (tracks / "Native scenario wooden coaster.td6").string();
        });
        Check(exported != entries.end(), "Exported Wooden fixture absent from broader native list");
        std::rotate(entries.begin(), exported, exported + 1);
        std::cout << "Designs " << entries.size() << std::endl;
        Check(!entries.empty(), "No exported eligible wooden designs");
        GameActions::ParkSetLoanAction loan(state.park.maxBankLoan);
        Check(GameActions::Execute(&loan, state).error == GameActions::Status::ok, "Native loan failed");
        GameActions::ProcessQueue(state);
        std::cout << "Native permitted loan " << state.park.bankLoan << " cash " << state.park.cash << std::endl;
        Check(H::BeginSavedDesign(entries.front().identity.key), "Begin failed");
        std::optional<CoordsXY> site;
        size_t attempts = 0;
        std::map<std::string, size_t> errors;
        for (int y = 128; !site && y < (state.mapSize.y - 4) * 32; y += 64)
            for (int x = 128; !site && x < (state.mapSize.x - 4) * 32; x += 64)
            {
                const auto result = H::Designs::Query(CoordsXY{ x, y });
                ++attempts;
                if (result.error == GameActions::Status::ok)
                {
                    std::cout << "Valid site " << x << "," << y << " cost " << result.cost << std::endl;
                    site = CoordsXY{ x, y };
                }
                ++errors[result.getErrorMessage()];
            }
        for (const auto& [error, count] : errors)
            std::cout << count << " " << error << std::endl;
        Check(site.has_value(), "No valid native site");
        const auto tile = *site;
        const auto baselineCash = state.park.cash;
        const auto baselineRandom = ScenarioRandState();
        auto elements = [&](bool ghost) {
            size_t count = 0;
            for (int y = 32; y < (state.mapSize.y - 1) * 32; y += 32)
                for (int x = 32; x < (state.mapSize.x - 1) * 32; x += 32)
                    for (auto* e : TileElementsView(CoordsXY{ x, y }))
                        if (e->isGhost() == ghost
                            && (e->getType() == TileElementType::track || e->getType() == TileElementType::entrance))
                            ++count;
            return count;
        };
        const auto baselineElements = elements(false);
        auto nativeDesign = TrackDesignImport(entries.front().identity.key.c_str());
        Check(nativeDesign && H::Designs::Eligible(*nativeDesign), "Native design unavailable");
        std::vector<ObjectEntryIndex> researched;
        for (ObjectEntryIndex i = 0; i < kMaxRideObjects; ++i)
            if (RideEntryIsInvented(i))
                researched.push_back(i);
        SetEveryRideEntryNotInvented();
        Check(!H::Designs::Eligible(*nativeDesign) && H::Designs::Entries().empty(), "Research filtering bypass");
        for (const auto i : researched)
            RideEntrySetInvented(i);
        H::ObjectPicker picker;
        picker.setEntries(entries);
        // File-backed identities remain distinct even when sharing a vehicle object.
        auto copies = entries;
        for (int i = 1; i < 9; ++i)
        {
            auto item = entries.front();
            item.identity.key += std::to_string(i);
            copies.push_back(item);
        }
        H::ObjectPicker navigation;
        navigation.setEntries(copies);
        navigation.move(0, 1);
        navigation.move(0, 1);
        Check(navigation.focus() == 6 && navigation.page() == 1, "Design picker paging failed");
        navigation.setEntries(copies, copies.back().identity);
        Check(navigation.focus() == copies.size() - 1, "File identity selection failed");
        const CoordsXYZ focus{ tile.x + 16, tile.y + 16, MapGetHighestZ(tile) };
        auto viewFocus = focus;
        auto capture = [&](const char* name, int mode, money64 cost = 0,
                           std::optional<H::RideManagement::Target> identity = {}) {
            Viewport viewport{};
            viewport.width = 512;
            viewport.height = 384;
            viewport.zoom = ZoomLevel{ static_cast<int8_t>(mode == 5 ? 2 : 1) };
            const auto centre = Translate3DTo2DWithZ(0, viewFocus);
            viewport.viewPos = centre - ScreenCoordsXY{ viewport.zoom.ApplyTo(300), viewport.zoom.ApplyTo(180) };
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &viewport);
            if (mode == 6)
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
            else if (mode == 0)
                H::DrawObjectPicker(rt, { 512, 384 }, picker, "Wooden designs");
            else if (mode == 1)
            {
                H::Designs::DrawPanel(rt, { 512, 384 });
                H::DrawCostBadge(rt, { 340, 276 }, H::FormatMoney(cost));
            }
            else if (mode == 2)
            {
                H::MenuPresentation model;
                model.title = "PLACE DESIGN?";
                model.rows = { "Cancel", "Place design" };
                model.status = "Cost " + H::FormatMoney(cost);
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
            }
            else if (mode == 3)
                F::DrawPanel(rt, { 512, 384 });
            else if (mode == 4)
            {
                H::Dashboard::State dashboard;
                dashboard.section = H::Dashboard::Section::rides;
                dashboard.focus = H::Dashboard::Focus::content;
                H::Dashboard::Draw(
                    rt, { 512, 384 }, dashboard,
                    H::RideManagement::Presentation(H::RideManagement::View::inspection, 0, {}, identity), true);
            }
            else
                H::DrawGameplayFooter(rt, { 512, 384 }, "", false, false);
            Image image;
            image.Width = 512;
            image.Height = 384;
            image.Depth = 8;
            image.Stride = rt.LineStride();
            image.Palette = Drawing::gPalette;
            const auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(argv[4]) / name).string(), image, ImageFormat::png);
            engine.EndDraw();
        };
        capture("design-picker.png", 0);
        Check(
            elements(false) == baselineElements && state.park.cash == baselineCash
                && (ScenarioRandState().s0 == baselineRandom.s0 && ScenarioRandState().s1 == baselineRandom.s1),
            "Native picker preview altered park/cash/random");
        H::Designs::UpdatePreview(tile);
        const auto ghostElements = elements(true);
        std::cout << "Ghost elements " << ghostElements << std::endl;
        Check(
            ghostElements > 0 && state.park.cash == baselineCash
                && (ScenarioRandState().s0 == baselineRandom.s0 && ScenarioRandState().s1 == baselineRandom.s1),
            "Ghost cash/random failed");
        H::Designs::UpdatePreview(tile);
        Check(elements(true) == ghostElements, "Unchanged target duplicated ghost");
        const auto cost = H::Designs::Query(tile).cost;
        capture("design-ghost.png", 1, cost);
        // Native empty ride creation fills the remaining slots, without fabricating
        // tracks. The provisional ride owns the last slot needed by real placement.
        std::vector<RideId> capacityFixtures;
        while (!GetNextFreeRideId().IsNull())
        {
            std::optional<RideId> created;
            const auto index = context->GetObjectManager().GetLoadedObjectEntryIndex(
                nativeDesign->trackAndVehicle.vehicleObject);
            GameActions::RideCreateAction action(
                RIDE_TYPE_WOODEN_ROLLER_COASTER, index, 0, 0, FlatRidePlacement::StationObject(),
                RideInspection::every30Minutes);
            action.SetCallback([&](const auto*, const GameActions::Result* result) {
                if (result->error == GameActions::Status::ok)
                    created = result->getData<RideId>();
            });
            Check(
                GameActions::Execute(&action, state).error == GameActions::Status::ok, "Native capacity fixture create failed");
            GameActions::ProcessQueue(state);
            Check(created.has_value(), "Native capacity fixture missing");
            capacityFixtures.push_back(*created);
        }
        H::Designs::UpdatePreview(tile);
        Check(
            H::Designs::Query(tile).error == GameActions::Status::ok && elements(true) == ghostElements,
            "Last-slot preview disappeared or used an extra ride record");
        for (const auto id : capacityFixtures)
        {
            GameActions::RideDemolishAction action(id, GameActions::RideModifyType::demolish);
            Check(
                GameActions::Execute(&action, state).error == GameActions::Status::ok,
                "Native capacity fixture cleanup failed");
            GameActions::ProcessQueue(state);
        }
        Check(
            state.park.cash == baselineCash && elements(false) == baselineElements, "Capacity fixtures altered cash or tracks");
        Ui::ProvisionalElementsRemove();
        Ui::ProvisionalElementsRemove();
        Check(elements(true) == 0 && elements(false) == baselineElements, "Nested ghost suspension failed");
        Ui::ProvisionalElementsRestore();
        Check(elements(true) == 0, "Nested ghost restored early");
        Ui::ProvisionalElementsRestore();
        Check(elements(true) == ghostElements, "Ghost restore failed");
        H::Designs::UpdatePreview(CoordsXY{ 0, 0 });
        Check(elements(true) == 0, "Invalid target retained ghost");
        H::Designs::UpdatePreview(tile);
        H::Designs::Rotate();
        Check(elements(true) == 0 && H::Designs::Orientation() == 1, "Rotation cleanup failed");
        for (int i = 0; i < 3; ++i)
            H::Designs::Rotate();
        H::Designs::UpdatePreview(tile);
        Check(elements(true) == ghostElements, "Rotation return ghost failed");
        SaveFlags flags{};
        flags.set(SaveFlag::exportObjects);
        const auto ghostSave = (std::filesystem::path(argv[4]) / "preview-safety.park").string();
        Check(ScenarioSave(state, ghostSave, flags) != 0 && elements(true) == ghostElements, "Ghost save/restore failed");
        Check(context->LoadParkFromFile(ghostSave), "Ghost save reload failed");
        Check(
            elements(true) == 0 && elements(false) == baselineElements && state.park.cash == baselineCash
                && !H::Designs::Active(),
            "Preview survived save/reload");
        Ui::GetWindowManager()->CloseByClass(WindowClass::error);
        gGamePaused = 0;
        InputSetState(InputState::normal); // The native event loop normally leaves load/reset before gameplay input.
        Check(H::BeginSavedDesign(entries.front().identity.key), "Reenter failed");
        H::Designs::UpdatePreview(tile);
        Ui::Windows::HandheldDesignPickerOpen();
        Check(elements(true) == 0, "Picker retained ghost");
        Ui::Windows::HandheldMenuBack();
        Check(H::Designs::Active(), "Picker cancellation ended placement");
        Ui::Windows::HandheldDesignPickerOpen();
        H::ControllerInput controller;
        H::Frame selecting;
        selecting.actions = { H::Action::confirm, H::Action::confirm };
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
        const auto device = SDL_JoystickAttachVirtual(
            SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        Check(device >= 0, "Design virtual controller unavailable");
        auto* joystick = SDL_JoystickOpen(device);
        auto pump = [&] {
            SDL_Event event;
            while (SDL_PollEvent(&event))
                controller.handleEvent(event);
        };
        pump();
        controller.update(true, 1000);
        pump();
        controller.update(true, 1010);
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 1);
        SDL_JoystickUpdate();
        pump();
        const auto heldSelection = controller.update(true, 1020);
        Check(
            heldSelection.actions.size() == 1 && heldSelection.actions.front() == H::Action::confirm,
            "Design mapped A missing");
        H::HandleMenuInput(controller, heldSelection);
        pump();
        Check(controller.update(true, 1070).actions.empty(), "Held picker A spilled into placement");
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 0);
        SDL_JoystickUpdate();
        pump();
        controller.update(true, 1080);
        SDL_JoystickClose(joystick);
        SDL_JoystickDetachVirtual(device);
        Check(
            H::Designs::Active() && elements(false) == baselineElements && state.park.cash == baselineCash,
            "Picker A spilled into placement");
        auto* main = WindowGetMain();
        const auto camera = main->viewport->rotation;
        const auto orientation = H::Designs::Orientation();
        H::Frame rotateCamera;
        rotateCamera.actions = { H::Action::contextPrimary };
        H::HandleExplorerInput(controller, rotateCamera);
        Check(
            main->viewport->rotation != camera && H::Designs::Orientation() == orientation,
            "Design tool stole camera rotation");
        for (int i = 0; i < 3; ++i)
            H::HandleExplorerInput(controller, rotateCamera);
        const auto zoom = main->viewport->zoom;
        H::Frame zoomOut;
        zoomOut.actions = { H::Action::previousOption };
        H::HandleExplorerInput(controller, zoomOut);
        H::Frame zoomIn;
        zoomIn.actions = { H::Action::nextOption };
        H::HandleExplorerInput(controller, zoomIn);
        Check(main->viewport->zoom == zoom, "Design tool changed camera zoom mappings");
        Check(H::Designs::Review(tile), "Review failed");
        Ui::Windows::HandheldDesignReviewOpen();
        capture("design-confirm.png", 2, H::Designs::ReviewCost());
        H::HandleMenuInput(controller, selecting);
        Check(
            !H::Designs::HasReview() && elements(false) == baselineElements && state.park.cash == baselineCash,
            "Cancel-first review placed ride");
        Check(H::Designs::Review(tile), "Second review failed");
        // Native funds and pause restrictions must still apply at confirmation.
        const auto cash = state.park.cash;
        state.park.cash = 0;
        Check(H::Designs::Confirm().error == GameActions::Status::insufficientFunds, "Confirmation bypassed funds");
        state.park.cash = cash;
        Check(H::Designs::Review(tile), "Pause review failed");
        gGamePaused = 1;
        Check(H::Designs::Confirm().error != GameActions::Status::ok, "Confirmation bypassed pause");
        gGamePaused = 0;
        Check(H::Designs::Review(tile), "Stale research review failed");
        GameActions::SavedTrackDesignPlaceAction guarded(
            *H::Designs::ReviewLocation(), *nativeDesign, Config::Get().general.defaultInspectionInterval,
            H::Designs::ReviewCost());
        DataSerialiser serialised(true);
        guarded.Serialise(serialised);
        serialised.getStream().SetPosition(0);
        const auto factory = GameActions::getFactory(GameCommand::placeSavedTrackDesign);
        Check(factory.has_value(), "Saved design action factory missing");
        std::unique_ptr<GameActions::GameAction> decoded((*factory)());
        DataSerialiser decode(false, serialised.getStream());
        decoded->Serialise(decode);
        Check(GameActions::Query(decoded.get(), state).error == GameActions::Status::ok, "Serialised design query failed");
        GameActions::SavedTrackDesignPlaceAction staleQuote(
            *H::Designs::ReviewLocation(), *nativeDesign, Config::Get().general.defaultInspectionInterval,
            H::Designs::ReviewCost() + 1);
        Check(GameActions::Query(&staleQuote, state).error != GameActions::Status::ok, "Changed native quote accepted");
        SetEveryRideEntryNotInvented();
        Check(
            GameActions::Query(decoded.get(), state).error != GameActions::Status::ok
                && decoded->Execute(state, state.park).error != GameActions::Status::ok,
            "Queued/serialised design used research fallback");
        Check(H::Designs::Confirm().error != GameActions::Status::ok && state.park.cash == cash, "Stale research accepted");
        for (const auto i : researched)
            RideEntrySetInvented(i);
        Check(H::Designs::Review(tile), "Stale money rules review failed");
        state.park.flags.set(ParkFlag::noMoney);
        Check(H::Designs::Confirm().error != GameActions::Status::ok, "Stale money rules accepted");
        state.park.cash = 0;
        Check(H::Designs::Query(tile).error == GameActions::Status::ok, "Native no-money placement query rejected");
        state.park.flags.unset(ParkFlag::noMoney);
        state.park.cash = cash;
        Check(H::Designs::Review(tile), "Pause UI review failed");
        Ui::Windows::HandheldDesignReviewOpen();
        H::Frame pause;
        pause.actions = { H::Action::openMenu };
        H::HandleMenuInput(controller, pause);
        Check(!H::Designs::HasReview() && state.park.cash == cash, "Start retained review or placed ride");
        Ui::Windows::HandheldMenuBack();
        Check(H::Designs::Review(tile), "Final review failed");
        const auto finalCost = H::Designs::ReviewCost();
        Ui::Windows::HandheldDesignReviewOpen();
        Ui::Windows::HandheldMenuMove(1);
        H::HandleMenuInput(controller, selecting);
        GameActions::ProcessQueue(state);
        auto placed = H::Designs::Completed();
        Check(
            placed && state.park.cash == baselineCash - finalCost && elements(true) == 0,
            "Single native placement/cash failed");
        Check(
            H::Designs::Confirm().error != GameActions::Status::ok && state.park.cash == baselineCash - finalCost,
            "Duplicate confirmation placed again");
        const auto id = placed->id;
        std::cout << "Placed native coaster " << id.ToUnderlying() << " cost " << finalCost << std::endl;
        const auto roster = H::RideManagement::Roster();
        Check(std::find(roster.begin(), roster.end(), *placed) != roster.end(), "Placed ride missing from Dashboard");
        int minX = INT32_MAX, minY = INT32_MAX, minZ = INT32_MAX, maxX = 0, maxY = 0, maxZ = 0;
        for (int y = 32; y < (state.mapSize.y - 1) * 32; y += 32)
            for (int x = 32; x < (state.mapSize.x - 1) * 32; x += 32)
                for (auto* track : TileElementsView<TrackElement>(CoordsXY{ x, y }))
                    if (!track->isGhost() && track->getRideIndex() == id)
                    {
                        minX = std::min(minX, x);
                        maxX = std::max(maxX, x);
                        minY = std::min(minY, y);
                        maxY = std::max(maxY, y);
                        minZ = std::min(minZ, track->getBaseZ());
                        maxZ = std::max(maxZ, track->getBaseZ());
                    }
        viewFocus = { (minX + maxX) / 2, (minY + maxY) / 2, (minZ + maxZ) / 2 };
        capture("design-completed.png", 5);
        viewFocus = focus;
        capture("design-dashboard.png", 4, 0, placed);
        // A complete design offers the existing optional access page without placing paths.
        H::UpdateExplorer(controller);
        Check(
            Ui::Windows::HandheldRidesIsOpen() && Ui::Windows::HandheldRidesPresentation().selected == 0,
            "Design completion did not offer optional access");
        H::HandleMenuInput(controller, selecting);
        Check(!H::Paths::IsActive() && !H::Designs::Active(), "Completion A spilled into Paths");
        // Remove access points using native actions to exercise recovery on both real stations.
        H::EndTerrain();
        struct Access
        {
            CoordsXY tile;
            bool exit;
            StationIndex station;
        };
        std::vector<Access> removed;
        for (size_t i = 0; i < GetRide(id)->getStations().size(); ++i)
        {
            const auto& station = GetRide(id)->getStations()[i];
            if (station.start.isNull())
                continue;
            Check(!station.entrance.isNull() && !station.exit.isNull(), "Design station incomplete");
            removed.push_back({ station.entrance.toCoordsXYZD(), false, StationIndex::FromUnderlying(i) });
            removed.push_back({ station.exit.toCoordsXYZD(), true, StationIndex::FromUnderlying(i) });
        }
        for (const auto& point : removed)
        {
            GameActions::RideEntranceExitRemoveAction action(point.tile, id, point.station, point.exit);
            Check(GameActions::Execute(&action, state).error == GameActions::Status::ok, "Native access removal failed");
            GameActions::ProcessQueue(state);
        }
        Ui::Windows::HandheldDashboardOpen();
        Ui::Windows::HandheldDashboardMove(0, 1);
        H::HandleMenuInput(controller, selecting);
        const auto recoveryRoster = H::RideManagement::Roster();
        const auto recoveryRow = std::find(recoveryRoster.begin(), recoveryRoster.end(), *placed);
        Check(recoveryRow != recoveryRoster.end(), "Recovery ride missing from roster");
        for (auto i = recoveryRoster.begin(); i != recoveryRow; ++i)
            Ui::Windows::HandheldMenuMove(1);
        H::HandleMenuInput(controller, selecting);
        Check(Ui::Windows::HandheldRidesPresentation().rows.size() > 7, "Missing recoverable inspection entry");
        for (int i = 0; i < 7; ++i)
            Ui::Windows::HandheldMenuMove(1);
        const auto recoveryCash = state.park.cash;
        H::HandleMenuInput(controller, selecting);
        Check(
            F::CurrentStage() == F::Stage::entrance && state.park.cash == recoveryCash,
            "Inspection recovery A spilled into station placement");
        for (const auto& point : removed)
        {
            const auto query = F::Action(point.tile, false);
            const auto before = state.park.cash;
            Check(query.error == GameActions::Status::ok, "Native station-side recovery invalid");
            const auto location = F::EntranceLocation(point.tile);
            Check(location.has_value(), "Station targeting unavailable");
            viewFocus = { point.tile.x + 16, point.tile.y + 16, location->z };
            gMapSelectPositionA = gMapSelectPositionB = point.tile;
            gMapSelectType = MapSelectType::full;
            gMapSelectArrowPosition = *location;
            gMapSelectArrowDirection = location->direction;
            gMapSelectFlags.set(MapSelectFlag::enable, MapSelectFlag::enableArrow);
            MapSelection::invalidate();
            capture(point.exit ? "design-exit.png" : "design-entrance.png", 3);
            gMapSelectFlags.unset(MapSelectFlag::enable, MapSelectFlag::enableArrow);
            MapSelection::invalidate();
            viewFocus = focus;
            Check(F::Action(point.tile, true).error == GameActions::Status::ok, "Station recovery execution failed");
            GameActions::ProcessQueue(state);
            Check(state.park.cash == before - query.cost, "Station recovery accounting failed");
            Check(F::Action(point.tile, true).error != GameActions::Status::ok, "Duplicate station placement succeeded");
        }
        auto identity = H::RideManagement::Identify(id);
        Check(identity && H::RideAccess::Inspect(*identity), "Recovered design access unavailable");
        auto connectLanding = [&](bool exit) {
            const auto access = *H::RideAccess::Inspect(*identity);
            const auto point = exit ? access.exit : access.entrance;
            Check(H::RideAccess::BeginPaths(*identity, exit), "Design path handoff failed");
            H::Paths::UpdateTarget({ ContextGetWidth() / 2, ContextGetHeight() / 2 });
            Check(H::Paths::TargetTile() == CoordsXY(point.outside), "Elevated design handoff missed native target");
            const auto query = H::Paths::Action(false);
            Check(query.error == GameActions::Status::ok, "Native landing path invalid");
            const auto before = state.park.cash;
            H::Paths::UpdatePreview();
            Check(state.park.cash == before, "Landing ghost charged cash");
            Check(H::Paths::Action(true).error == GameActions::Status::ok, "Native landing placement failed");
            GameActions::ProcessQueue(state);
            Check(state.park.cash == before - query.cost, "Landing path exact cost failed");
            bool found = false;
            for (auto* path : TileElementsView<PathElement>(point.outside))
                if (!path->isGhost() && path->getBaseZ() == point.outside.z && path->isQueue() == !exit)
                {
                    found = true;
                    if (!exit)
                        Check(
                            path->getRideIndex() == id && path->getStationIndex() == point.station,
                            "Native queue station association failed");
                }
            Check(found, "Native landing path missing");
            H::EndTerrain();
        };
        connectLanding(false);
        connectLanding(true);
        auto test = H::RideManagement::Begin(*identity, RideStatus::testing);
        Check(
            test && H::RideManagement::Action(*test, false).error == GameActions::Status::ok, "Native design testing invalid");
        Ui::Windows::HandheldDashboardOpen();
        Ui::Windows::HandheldDashboardMove(0, 1);
        H::HandleMenuInput(controller, selecting);
        const auto testingRoster = H::RideManagement::Roster();
        const auto testingRow = std::find(testingRoster.begin(), testingRoster.end(), *identity);
        Check(testingRow != testingRoster.end(), "Testing ride missing from roster");
        for (auto i = testingRoster.begin(); i != testingRow; ++i)
            Ui::Windows::HandheldMenuMove(1);
        H::HandleMenuInput(controller, selecting);
        for (int i = 0; i < 4; ++i)
            Ui::Windows::HandheldMenuMove(1);
        H::HandleMenuInput(controller, selecting);
        Check(
            Ui::Windows::HandheldRidesPresentation().confirmation && Ui::Windows::HandheldRidesPresentation().selected == 1,
            "Design testing review was not Cancel-first");
        capture("design-testing-review.png", 6);
        const auto testingCash = state.park.cash;
        H::HandleMenuInput(controller, selecting);
        Check(
            GetRide(id)->status == RideStatus::closed && state.park.cash == testingCash,
            "Cancel-first testing review changed ride/cash");
        for (int i = 0; i < 4; ++i)
            Ui::Windows::HandheldMenuMove(1);
        H::HandleMenuInput(controller, selecting);
        Ui::Windows::HandheldMenuMove(-1);
        H::HandleMenuInput(controller, selecting);
        GameActions::ProcessQueue(state);
        Check(
            GetRide(id)->status == RideStatus::testing && state.park.cash == testingCash,
            "Dashboard design testing confirmation failed");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        const auto savedCash = state.park.cash;
        const auto save = (std::filesystem::path(argv[4]) / "constructed-design.park").string();
        Check(
            ScenarioSave(state, save, flags) != 0 && context->LoadParkFromFile(save), "Constructed design save/reload failed");
        identity = H::RideManagement::Identify(id);
        std::cout << "Reload identity " << bool(identity) << " old resolves " << bool(H::RideManagement::Resolve(*placed))
                  << " access " << bool(identity && H::RideAccess::Inspect(*identity)) << " status " << int(GetRide(id)->status)
                  << " cash " << state.park.cash << "/" << savedCash << " ghost " << elements(true) << std::endl;
        Check(
            identity && H::RideAccess::Inspect(*identity) && GetRide(id)->status == RideStatus::testing
                && state.park.cash == savedCash && elements(true) == 0,
            "Design save identity/status/access/cash failed");
        std::cout << "Native design "
                     "picker/research/paging/ghost/rotation/cash/cancel/input/stations/access/testing/save-reload passed"
                  << std::endl;
        H::EndTerrain();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

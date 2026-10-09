// SPDX-License-Identifier: GPL-3.0-only
// Native private scenario, production picker/tool/shell rendering; no fixture objects or research.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Designs.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Panel.h>
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
#include <openrct2/actions/footpath/FootpathRemoveAction.h>
#include <openrct2/actions/park/ParkSetLoanAction.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/ride/RideEntranceExitRemoveAction.h>
#include <openrct2/actions/track/SequentialTrackPlaceAction.h>
#include <openrct2/actions/track/TrackPlaceAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/entity/EntityList.h>
#include <openrct2/entity/Guest.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/management/Research.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/rct2/T6Exporter.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/TrackData.h>
#include <openrct2/ride/TrackDesignRepository.h>
#include <openrct2/ride/TrackIteration.h>
#include <openrct2/ride/Vehicle.h>
#include <openrct2/ride/ted/TrackElementDescriptor.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <queue>
#include <set>
#include <stdexcept>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace F = H::FlatRides;
static void Check(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "FAIL: " << message << std::endl;
        throw std::runtime_error(message);
    }
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
        // Dismiss the native scenario objective page before controller gameplay.
        Ui::GetWindowManager()->CloseByClass(WindowClass::parkInformation);
        gGamePaused = 0;
        auto& state = getGameState();
        std::filesystem::create_directories(argv[4]);
        ScenarioRandSeed(0x1234567F, 0x789FABCD);
        namespace C = H::CustomTracks;
        GameActions::ParkSetLoanAction loan(state.park.maxBankLoan);
        Check(GameActions::Execute(&loan, state).error == GameActions::Status::ok, "Native loan setup failed");
        GameActions::ProcessQueue(state);
        const auto objects = C::RideEntries();
        const auto wooden = std::find_if(
            objects.begin(), objects.end(), [](const auto& item) { return item.identity.key.starts_with("custom:"); });
        Check(wooden != objects.end(), "No researched loaded wooden coaster");
        Ui::Windows::HandheldBuildOpen();
        Ui::Windows::HandheldMenuMove(8);
        Ui::Windows::HandheldMenuConfirm();
        const auto flat = F::Entries().size() + std::distance(objects.begin(), wooden);
        for (size_t n = 0; n < flat / 3; ++n)
            Ui::Windows::HandheldObjectPickerMove(0, 1);
        for (size_t n = 0; n < flat % 3; ++n)
            Ui::Windows::HandheldObjectPickerMove(1, 0);
        Ui::Windows::HandheldMenuConfirm();
        // The mode choice is rendered below before Custom build confirmation.
        std::optional<H::RideManagement::Target> target;
        RideId id = RideId::GetNull();
        H::RideAccess::State access{};
        StationIndex station{ StationIndex::FromUnderlying(0) };
        CoordsXYZD entry{}, exit{};
        H::ControllerInput controller;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm, H::Action::confirm };
        H::Frame back;
        back.actions = { H::Action::back, H::Action::back };
        auto focus = [&](CoordsXYZ p) {
            auto* main = WindowGetMain();
            const CoordsXYZ centre{ p.x + 16, p.y + 16, p.z };
            auto cursor = ScreenCoordsXY{ 256, 192 } - main->viewport->pos;
            main->savedViewPos = Translate3DTo2DWithZ(main->viewport->rotation, centre)
                - ScreenCoordsXY{ main->viewport->zoom.ApplyTo(cursor.x), main->viewport->zoom.ApplyTo(cursor.y) };
            main->viewport->viewPos = main->savedViewPos;
            main->flags.unset(WindowFlag::scrollingToLocation);
            main->invalidate();
        };
        auto capture = [&](const char* name, CoordsXYZ p, bool panel = false, bool modal = false, int rotation = 0,
                           int zoom = 0) {
            Viewport vp{};
            vp.width = 512;
            vp.height = 384;
            vp.zoom = ZoomLevel{ static_cast<int8_t>(zoom) };
            vp.rotation = rotation;
            vp.viewPos = Translate3DTo2DWithZ(rotation, CoordsXYZ{ p.x + 16, p.y + 16, p.z })
                - ScreenCoordsXY{ vp.zoom.ApplyTo(320), vp.zoom.ApplyTo(190) };
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &vp);
            if (modal)
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
            else if (C::Active())
                C::DrawPanel(rt, { 512, 384 });
            else if (F::CurrentStage() != F::Stage::inactive)
                F::DrawPanel(rt, { 512, 384 });
            else if (panel)
            {
                H::Dashboard::State ds;
                ds.section = H::Dashboard::Section::rides;
                ds.focus = H::Dashboard::Focus::content;
                H::Dashboard::Draw(rt, { 512, 384 }, ds, H::RideAccess::Presentation(*target, 0), true);
            }
            else if (H::Paths::IsActive())
            {
                H::DrawPathFooter(rt, { 512, 384 }, "", H::Paths::IsRemoving());
                H::DrawPathOverlay(
                    rt, H::Concept::kOrigin, H::Paths::SurfaceName(), H::Paths::IsQueue(), H::Paths::IsRemoving(),
                    H::Paths::PlacementName());
                auto q = H::Paths::Action(false);
                if (q.error == GameActions::Status::ok)
                    H::DrawCostBadge(rt, { 370, 300 }, H::FormatMoney(q.cost));
            }
            else
                H::DrawGameplayFooter(rt, { 512, 384 }, "", false, false);
            Image im;
            im.Width = 512;
            im.Height = 384;
            im.Depth = 8;
            im.Stride = rt.LineStride();
            im.Palette = Drawing::gPalette;
            auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            im.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(argv[4]) / name).string(), im, ImageFormat::png);
            engine.EndDraw();
        };
        capture("custom-build-choice.png", { 800, 800, MapGetHighestZ({ 800, 800 }) }, false, true);
        H::HandleMenuInput(controller, confirm);
        GameActions::ProcessQueue(state);
        Check(C::Active() && C::Target() && !C::Busy(), "Custom build entry failed");
        target = C::Target();
        id = target->id;
        using T = TrackElemType;
        std::vector<T> recipe;
        auto add = [&](T t, int count = 1) {
            for (int n = 0; n < count; ++n)
                recipe.push_back(t);
        };
        add(T::endStation, 5);
        add(T::flat, 2);
        add(T::flatToUp25);
        add(T::up25, 4);
        add(T::up25ToFlat);
        add(T::flatToLeftBank);
        add(T::bankedLeftQuarterTurn5Tiles);
        add(T::leftBankToFlat);
        add(T::flat, 2);
        add(T::flatToDown25);
        add(T::down25, 4);
        add(T::down25ToFlat);
        add(T::flatToLeftBank);
        add(T::bankedLeftQuarterTurn5Tiles);
        add(T::leftBankToFlat);
        add(T::flat, 13);
        add(T::flatToLeftBank);
        add(T::bankedLeftQuarterTurn5Tiles);
        add(T::leftBankToFlat);
        add(T::flat, 8);
        add(T::flatToLeftBank);
        add(T::bankedLeftQuarterTurn5Tiles);
        add(T::leftBankToFlat);
        // Locate a genuinely owned, native-query-valid site for this test itinerary.
        // Geometry below is read from native descriptors; production contains no recipe/site.
        std::optional<CoordsXYZD> site;
        for (int y = 20; y < 60 && !site; ++y)
            for (int x = 20; x < 65 && !site; ++x)
                for (Direction direction = 0; direction < 4 && !site; ++direction)
                {
                    const int z = MapGetHighestZ({ x * 32, y * 32 }) + 96;
                    if (z % 16)
                        continue;
                    CoordsXYZD cursor{ x * 32, y * 32, z, direction };
                    bool valid = true;
                    for (auto t : recipe)
                    {
                        const auto& ted = TrackMetadata::GetTrackElementDescriptor(t);
                        auto origin = cursor;
                        origin.z -= ted.coordinates.zBegin;
                        origin.direction = (cursor.direction - ted.coordinates.rotationBegin) & 3;
                        SelectedLiftAndInverted flags{};
                        if (t == T::flatToUp25 || t == T::up25 || t == T::up25ToFlat)
                            flags.set(LiftHillAndInverted::liftHill);
                        GameActions::TrackPlaceAction a(id, t, RIDE_TYPE_WOODEN_ROLLER_COASTER, origin, 6, 0, 0, flags, false);
                        if (GameActions::Query(&a, state).error != GameActions::Status::ok)
                        {
                            valid = false;
                            break;
                        }
                        cursor = CoordsXYZD{
                            CoordsXY(origin) + CoordsXY{ ted.coordinates.x, ted.coordinates.y }.rotate(origin.direction),
                            origin.z + ted.coordinates.zEnd,
                            static_cast<Direction>(
                                ((origin.direction + ted.coordinates.rotationEnd) & 3) | (ted.coordinates.rotationEnd & 4))
                        };
                        if (!(cursor.direction & 4))
                            cursor += CoordsDirectionDelta[cursor.direction];
                    }
                    if (valid && cursor == CoordsXYZD{ x * 32, y * 32, z, direction })
                        site = CoordsXYZD{ x * 32, y * 32, z, direction };
                }
        Check(site.has_value(), "No legal native query-valid custom coaster site");
        std::cout << "Custom station site " << site->x << "," << site->y << "," << site->z << ":" << int(site->direction)
                  << std::endl;
        focus(*site);
        C::Update(CoordsXY(*site));
        for (int n = 0; n < site->direction; ++n)
        {
            H::Frame turn;
            turn.actions = { H::Action::contextSecondary };
            H::HandleExplorerInput(controller, turn);
        }
        for (int n = 0; n < 12; ++n)
        {
            H::Frame height;
            height.actions = { H::Action::slopeUp };
            H::HandleExplorerInput(controller, height);
        }
        C::Update(CoordsXY(*site));
        Check(C::Endpoint() == site, "Station target/height/orientation mismatch");

        auto tracks = [&](bool ghost) {
            size_t count = 0;
            for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
                for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
                    for (auto* e : TileElementsView<TrackElement>(CoordsXY{ x, y }))
                        count += e->isGhost() == ghost;
            return count;
        };
        const auto previewCash = state.park.cash;
        const auto realTracks = tracks(false);
        const auto ghostTracks = tracks(true);
        const auto previewRandom = ScenarioRandState();

        Check(ghostTracks == 1, "Missing native station ghost");
        C::Update(CoordsXY(*site));
        Check(
            tracks(true) == ghostTracks && tracks(false) == realTracks && state.park.cash == previewCash,
            "Unchanged preview duplicated ghosts/changed park");
        std::cout << "Lifecycle suspension" << std::endl;
        Ui::ProvisionalElementsRemove();
        Ui::ProvisionalElementsRemove();
        Check(tracks(true) == 0, "Nested custom ghost suspension failed");
        Ui::ProvisionalElementsRestore();
        C::Update(CoordsXY(*site));
        Check(tracks(true) == 0, "Nested ghost restored too early");
        Ui::ProvisionalElementsRestore();
        C::Update(CoordsXY(*site));
        Check(tracks(true) == ghostTracks, "Custom ghost did not restore after provisional handling");
        std::cout << "Lifecycle invalid target" << std::endl;
        C::Update(CoordsXY{ 0, 0 });
        Check(tracks(true) == 0 && C::Action(false).error != GameActions::Status::ok, "Invalid target retained ghost");
        C::Update(CoordsXY(*site));
        std::cout << "Lifecycle quote" << std::endl;
        const auto stationQuote = C::Action(false);
        Check(stationQuote.error == GameActions::Status::ok, "Station query unavailable");
        GameActions::SequentialTrackPlaceAction guarded(id, T::endStation, *site, false, true, {}, T::flat, stationQuote.cost);
        std::cout << "Lifecycle serialize" << std::endl;
        DataSerialiser serialised(true);
        guarded.Serialise(serialised);
        serialised.getStream().SetPosition(0);
        const auto factory = GameActions::getFactory(GameCommand::placeSequentialTrack);
        Check(factory.has_value(), "Sequential action factory missing");
        std::unique_ptr<GameActions::GameAction> decoded((*factory)());
        DataSerialiser decode(false, serialised.getStream());
        decoded->Serialise(decode);
        Check(GameActions::Query(decoded.get(), state).error == GameActions::Status::ok, "Serialised track query failed");
        GameActions::SequentialTrackPlaceAction wrongCost(
            id, T::endStation, *site, false, true, {}, T::flat, stationQuote.cost + 1);
        Check(GameActions::Query(&wrongCost, state).error != GameActions::Status::ok, "Stale quoted price accepted");
        // Reversible restriction fixtures: restored before gameplay/captures; never bypass a placement rule.
        std::cout << "Lifecycle money" << std::endl;
        state.park.cash = 0;
        Check(C::Action(false).error == GameActions::Status::insufficientFunds, "Custom placement bypassed funds");
        state.park.flags.set(ParkFlag::noMoney);
        Check(
            C::Action(false).error == GameActions::Status::ok && C::Action(false).cost == stationQuote.cost,
            "Native no-money placement unavailable");
        Check(GameActions::Query(decoded.get(), state).error != GameActions::Status::ok, "Stale money mode accepted");
        H::HandleExplorerInput(controller, confirm);
        GameActions::ProcessQueue(state);
        Check(state.park.cash == 0 && tracks(false) == realTracks + 1, "No-money station placement charged cash");
        H::HandleExplorerInput(controller, back);
        GameActions::ProcessQueue(state);
        Check(
            state.park.cash == 0 && tracks(false) == realTracks && C::StationPlacement(),
            "No-money native undo charged/refunded cash or lost station mode");

        state.park.flags.unset(ParkFlag::noMoney);
        state.park.cash = previewCash;
        gGamePaused = 1;
        Check(C::Action(false).error != GameActions::Status::ok, "Custom action bypassed pause");
        C::Update(CoordsXY(*site));
        Check(tracks(true) == 0, "Paused invalid placement retained ghost");
        gGamePaused = 0;
        std::vector<ObjectEntryIndex> invented;
        for (ObjectEntryIndex object = 0; object < kMaxRideObjects; ++object)
            if (RideEntryIsInvented(object))
                invented.push_back(object);
        C::ClearPreview();
        SetEveryRideEntryNotInvented();
        Check(
            C::RideEntries().empty() && GameActions::Query(decoded.get(), state).error != GameActions::Status::ok
                && decoded->Execute(state, state.park).error != GameActions::Status::ok,
            "Unavailable research accepted by picker/serialised execution");
        for (const auto object : invented)
            RideEntrySetInvented(object);
        C::Update(CoordsXY(*site));
        Check(
            tracks(true) == ghostTracks && tracks(false) == realTracks && state.park.cash == previewCash,
            "Restriction fixtures changed native tracks/cash");
        const auto realIdentity = H::RideManagement::Identify(id);
        H::Frame pause;
        pause.actions = { H::Action::openMenu };
        H::HandleExplorerInput(controller, pause);
        Check(tracks(true) == 0 && C::Active(), "Dashboard transition retained ghost or discarded unfinished station");
        H::HandleMenuInput(controller, back);
        Check(
            C::Active() && C::Endpoint() == site && state.park.cash == previewCash,
            "Dashboard back changed construction state");
        C::Update(CoordsXY(*site));
        auto* nativeWindow = static_cast<SDL_Window*>(context->GetUiContext().GetWindow());
        SDL_HideWindow(nativeWindow);
        context->GetUiContext().ProcessMessages();
        Check(!ContextHasFocus(), "Native window did not relinquish focus");
        H::UpdateExplorer(controller);
        Check(tracks(true) == 0, "Focus loss retained custom ghost");
        SDL_ShowWindow(nativeWindow);
        context->GetUiContext().ProcessMessages();
        C::Update(CoordsXY(*site));
        Check(
            H::RideManagement::Identify(id) == realIdentity && ScenarioRandState().s0 == previewRandom.s0
                && ScenarioRandState().s1 == previewRandom.s1,
            "Preview altered real ride identity/random");
        capture("custom-station-ghost.png", *site);
        bool chainOn = false;
        for (size_t i = 0; i < recipe.size(); ++i)
        {
            const auto wanted = recipe[i];
            if (wanted == T::flatToUp25 || wanted == T::up25ToFlat)
            {
                H::Frame slope;
                slope.actions = { wanted == T::flatToUp25 ? H::Action::slopeUp : H::Action::slopeDown };
                H::HandleExplorerInput(controller, slope);
                Check(C::Selection() == wanted, "Semantic slope transition selection failed");
            }
            if (wanted == T::flatToLeftBank)
            {
                H::Frame bank;
                bank.actions = { H::Action::contextPrimary };
                H::HandleExplorerInput(controller, bank);
                Check(C::Selection() == wanted, "Semantic banking transition selection failed");
            }
            for (int n = 0; n < 350 && C::Selection() != wanted; ++n)
            {
                H::Frame cycle;
                cycle.actions = { H::Action::nextOption };
                H::HandleExplorerInput(controller, cycle);
            }
            Check(C::Selection() == wanted, "Compatible native piece missing");
            const bool lift = wanted == T::flatToUp25 || wanted == T::up25 || wanted == T::up25ToFlat;
            if (lift != chainOn)
            {
                H::Frame chain;
                chain.actions = { H::Action::contextSecondary };
                H::HandleExplorerInput(controller, chain);
                chainOn = lift;
            }
            const auto endpoint = *C::Endpoint();
            C::Update(i == 0 ? std::optional<CoordsXY>{ CoordsXY(*site) } : std::nullopt);
            auto q = C::Action(false);
            const auto cash = state.park.cash;
            if (q.error != GameActions::Status::ok)
            {
                std::cerr << "Custom piece " << i << " type " << int(wanted) << " at " << endpoint.x << "," << endpoint.y << ","
                          << endpoint.z << " error " << q.getErrorMessage() << std::endl;
            }
            Check(q.error == GameActions::Status::ok, "Custom track query rejected");
            Check(state.park.cash == cash, "Custom preview spent money");
            if (i == 6)
                capture("custom-sequential.png", endpoint);
            if (wanted == T::up25)
                capture("custom-chain-slope.png", endpoint);
            if (wanted == T::bankedLeftQuarterTurn5Tiles)
                capture("custom-banked-turn.png", endpoint);
            if (i == 8)
            {
                H::Frame picker;
                picker.actions = { H::Action::openBuild };
                H::HandleExplorerInput(controller, picker);
                // Capture the actual picker independently of the construction panel.
                C::ClearPreview();
                auto* window = Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu);
                Check(window != nullptr, "Track picker missing");
                // A dedicated native viewport render below uses the production picker.
                capture("custom-track-picker.png", endpoint, false, true);
                for (size_t page = 0; page < C::Pieces().size(); ++page)
                    Ui::Windows::HandheldObjectPickerMove(0, 1);
                capture("custom-track-picker-last.png", endpoint, false, true);
                H::HandleMenuInput(controller, back);
                Check(C::Endpoint() == endpoint && state.park.cash == cash, "Picker cancel changed endpoint/cash");
                Check(tracks(true) == 0, "Picker cancellation retained ghost before revalidation");
                H::HandleExplorerInput(controller, picker);
                SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
                SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
                const auto device = SDL_JoystickAttachVirtual(
                    SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
                Check(device >= 0, "Custom virtual controller unavailable");
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
                const auto selecting = controller.update(true, 1020);
                Check(
                    selecting.actions.size() == 1 && selecting.actions.front() == H::Action::confirm,
                    "Picker mapped A missing");
                H::HandleMenuInput(controller, selecting);
                Check(
                    C::Endpoint() == endpoint && tracks(false) == realTracks + i && state.park.cash == cash,
                    "Picker A placed track instead of selecting");
                Check(C::Selection() == wanted, "Picker did not preserve focused native piece");
                pump();
                Check(controller.update(true, 1070).actions.empty(), "Held picker A spilled into construction");
                SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 0);
                SDL_JoystickUpdate();
                pump();
                controller.update(true, 1080);
                SDL_JoystickClose(joystick);
                SDL_JoystickDetachVirtual(device);
            }
            H::HandleExplorerInput(controller, confirm);
            GameActions::ProcessQueue(state);
            Check(state.park.cash == cash - q.cost, "Custom A did not charge exactly once");
            const auto& descriptor = TrackMetadata::GetTrackElementDescriptor(wanted);
            const auto& sequence = descriptor.sequenceData.sequences[0].clearance;
            const auto originXY = CoordsXY(endpoint) + CoordsXY{ sequence.x, sequence.y }.rotate(site->direction);
            size_t placedBlocks = 0;
            for (auto* element : TileElementsView<TrackElement>(originXY))
                if (!element->isGhost() && element->getRideIndex() == id && element->getSequenceIndex() == 0
                    && element->getTrackType() == wanted
                    && element->getBaseZ() == endpoint.z - descriptor.coordinates.zBegin + sequence.z)
                {
                    ++placedBlocks;
                    Check(element->hasChain() == lift, "Real native track lost selected chain state");
                }
            Check(placedBlocks == 1, "A did not produce exactly one selected native block");

            if (i == 0)
                Check(
                    GameActions::Query(decoded.get(), state).error != GameActions::Status::ok
                        && decoded->Execute(state, state.park).error != GameActions::Status::ok
                        && state.park.cash == cash - q.cost,
                    "Stale first-station action placed duplicate track");
            std::cout << "Built " << i << " type " << int(wanted) << " cost " << q.cost << std::endl;
            if (i == 10 || i == 14)
            {
                for (int undo = 0; undo < (i == 10 ? 2 : 1); ++undo)
                {
                    // Exact native removal refund is asserted by the actual prior-piece query.
                    const auto refund = C::Undo(false);
                    const auto before = state.park.cash;
                    H::HandleExplorerInput(controller, back);
                    GameActions::ProcessQueue(state);
                    Check(
                        refund.error == GameActions::Status::ok && state.park.cash == before - refund.cost && !C::Busy(),
                        "Native undo failed/exact refund lost");
                }
                for (int redo = 0; redo < (i == 10 ? 2 : 1); ++redo)
                {
                    const auto replay = i == 10 ? T::up25 : wanted;
                    for (size_t attempt = 0; C::Selection() != replay && attempt < C::Pieces().size(); ++attempt)
                    {
                        H::Frame cycle;
                        cycle.actions = { H::Action::nextOption };
                        H::HandleExplorerInput(controller, cycle);
                    }
                    Check(C::Selection() == replay, "Semantic redo piece selection failed");
                    C::Update({});
                    auto r = C::Action(false);
                    const auto before = state.park.cash;
                    H::HandleExplorerInput(controller, confirm);
                    GameActions::ProcessQueue(state);
                    Check(state.park.cash == before - r.cost, "Redo cost failed");
                }
                C::Update({});
                const auto mid = (std::filesystem::path(argv[4]) / "custom-unfinished.park").string();
                SaveFlags flags{};
                flags.set(SaveFlag::exportObjects);
                Check(
                    ScenarioSave(state, mid, flags) && context->LoadParkFromFile(mid), "Custom unfinished save/reload failed");
                Check(tracks(true) == 0, "Unfinished save persisted a ghost");
                target = H::RideManagement::Identify(id);
                Check(target && C::CanResume(*target), "Native endpoint recovery missing");
                InputSetState(InputState::normal);
                Ui::GetWindowManager()->CloseByClass(WindowClass::parkInformation);
                const auto resumeCash = state.park.cash;
                Ui::Windows::HandheldRideOpen(id);
                Ui::Windows::HandheldMenuMove(H::RideAccess::Supported(*target) ? 4 : 3);
                H::HandleMenuInput(controller, confirm);
                Check(C::Active() && state.park.cash == resumeCash, "Inspection recovery/input handoff failed");
                chainOn = false;
            }
        }
        Check(C::Completed() && C::Target(), "Custom circuit did not close natively");
        target = C::Target();
        capture("custom-completed.png", { site->x, site->y + 5 * 32, site->z }, false, false, 0, 2);
        H::UpdateExplorer(controller);
        Check(F::CurrentStage() == F::Stage::entrance, "Custom completion did not hand off to native entrance");
        // Select genuinely valid native station-adjacent entrance/exit positions.
        for (int phase = 0; phase < 2; ++phase)
        {
            std::optional<CoordsXY> tile;
            for (int y = site->y - 6 * 32; y <= site->y + 6 * 32 && !tile; y += 32)
                for (int x = site->x - 6 * 32; x <= site->x + 6 * 32 && !tile; x += 32)
                    if (F::Action(CoordsXY{ x, y }, false).error == GameActions::Status::ok)
                        tile = CoordsXY{ x, y };
            Check(tile.has_value(), "No native entrance/exit site");
            const auto q = F::Action(tile, false);
            const auto cash = state.park.cash;
            const auto location = F::EntranceLocation(tile);
            Check(location.has_value(), "Missing native entrance target");
            focus(*location);
            F::UpdatePreview(tile);
            capture(phase ? "custom-exit.png" : "custom-entrance.png", *location);
            Check(F::Action(tile, true).error == GameActions::Status::ok, "Native custom entrance/exit failed");
            GameActions::ProcessQueue(state);
            Check(state.park.cash == cash - q.cost, "Entrance/exit accounting mismatch");
        }
        H::EndTerrain();
        target = H::RideManagement::Identify(id);
        Check(target && H::RideAccess::Inspect(*target), "Custom access unavailable");
        access = *H::RideAccess::Inspect(*target);
        entry = access.entrance.outside;
        exit = access.exit.outside;
        station = access.entrance.station;
        size_t elevatedPieces = 0, slopedPieces = 0;
        std::optional<CoordsXYZ> bridge;
        auto ghostCount = [&]() {
            size_t n = 0;
            for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
                for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
                    for (auto* e : TileElementsView<PathElement>(CoordsXY{ x, y }))
                        n += e->isGhost();
            return n;
        };
        struct Piece
        {
            CoordsXYZ p;
            FootpathSlope slope;
        };
        auto route = [&](CoordsXYZD start, bool queue) {
            H::Paths::Begin(queue);
            std::vector<CoordsXYZ> goals;
            for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
                for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
                    for (auto* p : TileElementsView<PathElement>(CoordsXY{ x, y }))
                        if (!p->isGhost() && !p->isQueue()
                            && H::RideAccess::NormalPathReachesParkEntrance({ x, y, p->getBaseZ() }))
                            goals.emplace_back(x, y, p->getBaseZ());
            Check(!goals.empty(), "No native park-connected paths");
            auto heuristic = [&](CoordsXYZ p) {
                int best = INT32_MAX;
                for (auto g : goals)
                    best = std::min(best, (std::abs(p.x - g.x) + std::abs(p.y - g.y)) / 32 + std::abs(p.z - g.z) / 16);
                return best;
            };
            struct Node
            {
                CoordsXYZ pos;
                Direction incoming;
                int parent, g;
                Piece piece;
            };
            using Key = std::tuple<int, int, int, int>;
            std::vector<Node> nodes;
            std::map<Key, int> visited;
            std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>>
                pending;
            nodes.push_back({ start, DirectionReverse(start.direction), -1, 0, {} });
            pending.emplace(heuristic(start), 0);
            std::optional<int> goal;
            int queries = 0;
            while (!pending.empty() && !goal && queries < 100000)
            {
                auto index = pending.top().second;
                pending.pop();
                const auto node = nodes[index];
                const Key key{ node.pos.x, node.pos.y, node.pos.z, node.incoming };
                if (visited.contains(key))
                    continue;
                visited[key] = index;
                for (auto* p : TileElementsView<PathElement>(node.pos))
                    if (!p->isGhost() && !p->isQueue() && FootpathIsZAndDirectionValid(*p, node.pos.z / 8, node.incoming)
                        && H::RideAccess::NormalPathReachesParkEntrance({ node.pos.x, node.pos.y, p->getBaseZ() }))
                        goal = index;
                if (goal)
                    break;
                for (Direction d : kAllDirections)
                    for (int dz : { 0, 16, -16 })
                    {
                        if (index == 0 && dz)
                            continue;
                        if (dz && d != node.incoming)
                            continue;
                        if (d == DirectionReverse(node.incoming) && index)
                            continue;
                        const auto base = node.pos.z + std::min(dz, 0);
                        if (base < kFootpathMinHeight || base > kFootpathMaxHeight)
                            continue;
                        FootpathSlope slope{ dz ? FootpathSlopeType::sloped : FootpathSlopeType::flat,
                                             dz > 0 ? d : DirectionReverse(d) };
                        auto type = gFootpathSelection.getSelectedSurface();
                        PathConstructFlags flags = queue ? PathConstructFlag::IsQueue : 0;
                        if (gFootpathSelection.legacyPath != kObjectEntryIndexNull)
                        {
                            type = gFootpathSelection.legacyPath;
                            flags |= PathConstructFlag::IsLegacyPathObject;
                        }
                        // Prefer an unobstructed corridor. Native queries permit some
                        // overlapping path forms which are not a usable walk route.
                        bool usable = true;
                        for (auto* existing : TileElementsView<PathElement>(node.pos))
                            if (!existing->isGhost() && std::abs(existing->getBaseZ() - base) < 32)
                            {
                                if (queue || existing->isQueue() || existing->getBaseZ() != base
                                    || existing->isSloped() != (slope.type == FootpathSlopeType::sloped)
                                    || (existing->isSloped() && existing->getSlopeDirection() != slope.direction))
                                    usable = false;
                            }
                        if (!usable)
                            continue;
                        GameActions::FootpathPlaceAction action(
                            { node.pos.x, node.pos.y, base }, slope, type, gFootpathSelection.railings, node.incoming, flags);
                        ++queries;
                        if (GameActions::Query(&action, state).error != GameActions::Status::ok)
                            continue;
                        const CoordsXYZ next{ CoordsXY(node.pos) + CoordsDirectionDelta[d], node.pos.z + dz };
                        if (!MapIsLocationValid(next))
                            continue;
                        nodes.push_back({ next, d, index, node.g + 1, { { node.pos.x, node.pos.y, base }, slope } });
                        pending.emplace(node.g + 1 + heuristic(next), nodes.size() - 1);
                    }
            }
            std::cout << (queue ? "Queue" : "Exit") << " search " << queries << " queries; goal " << bool(goal) << std::endl;
            Check(goal.has_value(), "No legal native elevated route");
            std::vector<Piece> result;
            for (int i = *goal; nodes[i].parent >= 0; i = nodes[i].parent)
                result.push_back(nodes[i].piece);
            std::reverse(result.begin(), result.end());
            H::Paths::End();
            return result;
        };
        auto build = [&](CoordsXYZD start, bool queue) {
            const auto pieces = route(start, queue);
            Check(!pieces.empty(), "Route empty");
            std::cout << (queue ? "Queue" : "Exit") << " pieces " << pieces.size() << std::endl;
            Check(H::RideAccess::BeginPaths(*target, !queue), "Native access handoff failed");
            for (size_t i = 0; i < pieces.size(); ++i)
            {
                const auto& part = pieces[i];
                Check(H::Paths::SetPlacementHeight(part.p.z), "Native height rejected");
                focus(part.p);
                H::Paths::UpdateTarget({ 256, 192 });
                Check(H::Paths::TargetTile() == CoordsXY(part.p), "Height-plane targeting missed real tile");
                if (part.slope.type == FootpathSlopeType::sloped)
                {
                    H::Paths::CycleSlope();
                    for (int n = 0; n < 4
                         && H::Paths::PlacementName().find(
                                std::array<const char*, 4>{ "NW", "NE", "SE", "SW" }[part.slope.direction])
                             == std::string::npos;
                         ++n)
                        H::Paths::RotateSlope();
                }
                const auto q = H::Paths::Action(false);
                const auto cash = state.park.cash;
                Check(q.error == GameActions::Status::ok, "Route native query rejected");
                H::Paths::UpdatePreview();
                Check(state.park.cash == cash, "Route ghost spent cash");
                if (i == 0 || part.slope.type == FootpathSlopeType::sloped)
                    capture(queue ? "custom-access-queue-ghost.png" : "custom-access-exit-ghost.png", part.p);
                H::HandleExplorerInput(controller, confirm);
                GameActions::ProcessQueue(state);
                if (state.park.cash != cash - q.cost)
                {
                    std::cerr << "Route tile " << i << " " << part.p.x << "," << part.p.y << "," << part.p.z << " query "
                              << q.cost << " charge " << cash - state.park.cash << " input " << int(InputGetState())
                              << " paused " << int(gGamePaused) << std::endl;
                    for (auto& window : gWindowList)
                        std::cerr << "Window " << int(window->classification) << " dead " << window->flags.has(WindowFlag::dead)
                                  << std::endl;
                }
                Check(state.park.cash == cash - q.cost, "Route A charged incorrectly");
                if (part.p.z > MapGetHighestZ(part.p))
                {
                    ++elevatedPieces;
                    bridge = part.p;
                }
                if (part.slope.type == FootpathSlopeType::sloped)
                    ++slopedPieces;
                for (auto* path : TileElementsView<PathElement>(part.p))
                    if (!path->isGhost() && path->getBaseZ() == part.p.z)
                        std::cout << "PLACED " << queue << " " << part.p.x << "," << part.p.y << "," << part.p.z << " slope "
                                  << int(part.slope.type) << ":" << int(part.slope.direction) << " edges "
                                  << int(path->getEdges()) << " ride " << path->getRideIndex().ToUnderlying() << " station "
                                  << int(path->getStationIndex().ToUnderlying()) << std::endl;
                if (i == pieces.size() / 2)
                {
                    H::HandleExplorerInput(controller, back);
                    Check(
                        !H::Paths::IsActive() && Ui::Windows::HandheldRidesIsOpen(),
                        "Unfinished route B did not return to access");
                    // Leave access completely, save the unfinished route, then resume via ordinary inspection.
                    H::HandleMenuInput(controller, confirm);
                    Check(!Ui::Windows::HandheldRidesIsOpen(), "Access Back did not return to park");
                    const auto savedCash = state.park.cash;
                    if (!queue)
                    {
                        const auto unfinished = (std::filesystem::path(argv[4]) / "unfinished-access.park").string();
                        SaveFlags flags{};
                        flags.set(SaveFlag::exportObjects);
                        Check(
                            ScenarioSave(state, unfinished, flags) && context->LoadParkFromFile(unfinished),
                            "Unfinished route save/reload failed");
                        target = H::RideManagement::Identify(id);
                        Check(target && state.park.cash == savedCash, "Unfinished route identity/cash lost");
                        InputSetState(InputState::normal);
                        Ui::GetWindowManager()->CloseByClass(WindowClass::parkInformation);
                    }
                    Ui::Windows::HandheldRideOpen(id);
                    Ui::Windows::HandheldMenuMove(3);
                    H::HandleMenuInput(controller, confirm);
                    Check(Ui::Windows::HandheldRidesIsOpen(), "Later inspection access unavailable");
                    Ui::Windows::HandheldMenuMove(queue ? 1 : 2);
                    H::HandleMenuInput(controller, confirm);
                    Check(H::Paths::IsActive() && state.park.cash == savedCash, "Inspection route resume placed/spent");
                }
            }
            H::EndTerrain();
        };
        build(exit, false);
        build(entry, true);
        Check(elevatedPieces > 0 && slopedPieces > 0 && bridge, "Scenario route did not exercise elevation and slope");
        capture("custom-access-connected-bridge.png", *bridge);
        access = *H::RideAccess::Inspect(*target);
        std::cout << "Final entry station " << int(access.entrance.station.ToUnderlying()) << " attached "
                  << access.entrance.attached << " normal " << access.entrance.reachesNormalPath << " park "
                  << access.entrance.reachesParkEntrance << " exit station " << int(access.exit.station.ToUnderlying())
                  << " attached " << access.exit.attached << " park " << access.exit.reachesParkEntrance << std::endl;
        Check(
            access.entrance.reachesParkEntrance && access.exit.reachesParkEntrance, "Native complete park connection missing");
        capture("custom-access-connected-review.png", entry, true);
        capture("custom-access-connected-park.png", entry);
        Ui::Windows::HandheldDashboardOpen();
        Ui::Windows::HandheldDashboardMove(0, 1);
        H::HandleMenuInput(controller, confirm);
        const auto roster = H::RideManagement::Roster();
        const auto row = std::find(roster.begin(), roster.end(), *target);
        Check(row != roster.end(), "Custom ride missing from Dashboard roster");
        for (auto item = roster.begin(); item != row; ++item)
            Ui::Windows::HandheldMenuMove(1);
        capture("custom-dashboard-roster.png", entry, false, true);
        H::HandleMenuInput(controller, confirm);
        Ui::Windows::HandheldMenuMove(4);
        H::HandleMenuInput(controller, confirm);
        Check(
            Ui::Windows::HandheldRidesPresentation().confirmation && Ui::Windows::HandheldRidesPresentation().selected == 1,
            "Testing review not Cancel-first");
        capture("custom-dashboard-test.png", entry, false, true);
        const auto testingCash = state.park.cash;
        H::HandleMenuInput(controller, confirm);
        Check(GetRide(id)->status == RideStatus::closed && state.park.cash == testingCash, "Testing cancel mutated park");
        Ui::Windows::HandheldMenuMove(4);
        H::HandleMenuInput(controller, confirm);
        Ui::Windows::HandheldMenuMove(-1);
        H::HandleMenuInput(controller, confirm);
        GameActions::ProcessQueue(state);
        Check(GetRide(id)->status == RideStatus::testing && state.park.cash == testingCash, "Native Dashboard test failed");
        auto closeDraft = H::RideManagement::Begin(*target, RideStatus::closed);
        Check(
            closeDraft && H::RideManagement::Action(*closeDraft, true).error == GameActions::Status::ok,
            "Native close after testing failed");
        GameActions::ProcessQueue(state);
        Ui::Windows::HandheldRideAccessOpen(*target);
        for (int n = 0; n < 3; ++n)
            Ui::Windows::HandheldMenuMove(1);
        H::HandleMenuInput(controller, confirm);
        Check(
            Ui::Windows::HandheldRidesPresentation().confirmation && Ui::Windows::HandheldRidesPresentation().selected == 1,
            "Opening not Cancel-first");
        capture("custom-access-opening-confirmation.png", entry, false, true);
        const auto cash = state.park.cash;
        H::HandleMenuInput(controller, confirm);
        Check(GetRide(id)->status == RideStatus::closed && state.park.cash == cash, "Opening Cancel mutated park");
        Ui::Windows::HandheldRideAccessOpen(*target);
        for (int n = 0; n < 3; ++n)
            Ui::Windows::HandheldMenuMove(1);
        H::HandleMenuInput(controller, confirm);
        Ui::Windows::HandheldMenuMove(-1);
        H::HandleMenuInput(controller, confirm);
        GameActions::ProcessQueue(state);
        Check(GetRide(id)->status == RideStatus::open && state.park.cash == cash, "Opening confirmation/accounting failed");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        gGamePaused = 0;
        if (!state.park.flags.has(ParkFlag::parkOpen))
        {
            GameActions::ParkSetParameterAction a(GameActions::ParkParameter::open);
            Check(GameActions::Execute(&a, state).error == GameActions::Status::ok, "Native park opening failed");
            GameActions::ProcessQueue(state);
        }
        std::set<uint32_t> queued, riding;
        bool done = false, capturedMovingVehicle = false;
        for (int tick = 0; tick < 160000 && !done; ++tick)
        {
            gameStateUpdateLogic();
            for (auto* guest : EntityList<Guest>())
            {
                if (guest->currentRide == id && guest->currentRideStation == station
                    && (guest->state == PeepState::queuing || guest->state == PeepState::queuingFront))
                {
                    if (queued.empty())
                        capture("custom-access-guests-queue.png", CoordsXYZ{ guest->x, guest->y, guest->z }, false, false, 2);
                    queued.insert(guest->peepId);
                }
                if (guest->currentRide == id && guest->state == PeepState::onRide && queued.contains(guest->peepId))
                {
                    if (!capturedMovingVehicle)
                    {
                        auto* train = state.entities.getEntity<Vehicle>(GetRide(id)->vehicles[guest->currentTrain]);
                        auto* car = train ? train->GetCar(guest->currentCar) : nullptr;
                        Check(car && MapIsLocationValid(CoordsXY{ car->x, car->y }), "Boarded guest has no native car");
                        // Boarded guests have null map coordinates; follow their actual native vehicle.
                        if (car->status == Vehicle::Status::travelling
                            && std::abs(car->x - entry.x) + std::abs(car->y - entry.y) > 192)
                        {
                            capture("custom-access-guests-riding.png", CoordsXYZ{ car->x, car->y, car->z }, false, false, 2);
                            capturedMovingVehicle = true;
                        }
                    }
                    riding.insert(guest->peepId);
                }
                if (capturedMovingVehicle && guest->state == PeepState::walking && guest->hasRidden(*GetRide(id))
                    && riding.contains(guest->peepId))
                {
                    capture("custom-access-guests-exited.png", CoordsXYZ{ guest->x, guest->y, guest->z }, false, false, 2);
                    done = true;
                    break;
                }
            }
            if (tick % 10000 == 0)
                std::cout << "Tick " << tick << " queued " << queued.size() << " riding " << riding.size() << " customers "
                          << GetRide(id)->totalCustomers << std::endl;
        }
        Check(done && !queued.empty() && !riding.empty(), "Natural native queue/ride/exit not observed");
        const auto finalCash = state.park.cash;
        SaveFlags flags{};
        flags.set(SaveFlag::exportObjects);
        auto save = (std::filesystem::path(argv[4]) / "custom-access-access.park").string();
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Native save/reload failed");
        target = H::RideManagement::Identify(id);
        access = *H::RideAccess::Inspect(*target);
        Check(
            access.entrance.reachesParkEntrance && access.exit.reachesParkEntrance && state.park.cash == finalCash
                && GetRide(id)->status == RideStatus::open,
            "Saved access/status/cash lost");
        // A native external entrance move/removal invalidates the old guided landing before A.
        const auto savedRide = *target;
        access = *H::RideAccess::Inspect(savedRide);
        Check(H::RideAccess::BeginPaths(savedRide, false), "Stale target setup failed");
        const auto point = access.entrance;
        auto* currentRide = H::RideManagement::Resolve(savedRide);
        auto close = H::RideManagement::Begin(savedRide, RideStatus::closed);
        Check(close && H::RideManagement::Action(*close, true).error == GameActions::Status::ok, "Stale test close failed");
        GameActions::ProcessQueue(state);
        const auto entrancePosition = currentRide->getStations()[point.station.ToUnderlying()].entrance.toCoordsXYZD();
        GameActions::RideEntranceExitRemoveAction remove(entrancePosition, id, point.station, false);
        Check(GameActions::Execute(&remove, state).error == GameActions::Status::ok, "Stale native entrance removal failed");
        GameActions::ProcessQueue(state);
        const auto staleCash = state.park.cash;
        H::HandleExplorerInput(controller, confirm);
        Check(
            !H::Paths::IsActive() && ghostCount() == 0 && state.park.cash == staleCash,
            "Stale access target accepted A or retained ghost");
        Check(context->LoadParkFromFile(save) && state.park.cash == finalCash, "Stale test restore failed");
        const auto unfinished = (std::filesystem::path(argv[4]) / "custom-unfinished.park").string();
        Check(context->LoadParkFromFile(unfinished), "Shutdown fixture reload failed");
        target = H::RideManagement::Identify(id);
        Check(target && H::ResumeCustomTrack(id), "Shutdown unfinished resume failed");
        C::Update({});
        Check(tracks(true) > 0, "Shutdown fixture lacked a native ghost");
        const auto staleTrackTarget = *target;
        GameActions::RideDemolishAction removeRide(id, GameActions::RideModifyType::demolish);
        Check(
            GameActions::Execute(&removeRide, state).error == GameActions::Status::ok,
            "Native stale-ride fixture removal failed");
        GameActions::ProcessQueue(state);
        C::Update({});
        const auto removedCash = state.park.cash;
        H::HandleExplorerInput(controller, confirm);
        GameActions::ProcessQueue(state);
        Check(
            !C::Active() && tracks(true) == 0 && state.park.cash == removedCash,
            "Removed custom ride accepted A or retained ghost");
        Check(context->LoadParkFromFile(unfinished), "Shutdown fixture restore failed");
        target = H::RideManagement::Identify(id);
        Check(
            target && !H::RideManagement::Resolve(staleTrackTarget) && !C::Resume(staleTrackTarget),
            "Reused ride identity accepted stale construction target");
        Check(H::ResumeCustomTrack(id), "Shutdown restored resume failed");
        C::Update({});
        Check(tracks(true) > 0, "Shutdown restored fixture lacked ghost");
        context.reset();
        Check(!C::Active(), "Context shutdown retained custom construction state");
        std::cout << "Native custom coaster / elevated access / accounting / cancellation-resume / opening / natural guests / "
                     "save-reload passed"
                  << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

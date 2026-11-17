// SPDX-License-Identifier: GPL-3.0-only
// Native map fixture, real SDL controller routing and production map rendering.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/MapSubjects.h>
#include <openrct2-ui/handheld/ParkInformation.h>
#include <openrct2-ui/handheld/ParkMap.h>
#include <openrct2-ui/handheld/ParkOverview.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/general/MapChangeSizeAction.h>
#include <openrct2/actions/peep/StaffFireAction.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Window.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Park.h>
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
        Check(context->GetObjectManager().LoadObject("rct2.peep_animations.guest"), "Fixture guest object missing");
        for (int i = 0; i < 2; ++i)
        {
            auto* guest = Guest::generate({ 640 + i * 32, 640, 16 });
            Check(guest, "Fixture guest generation failed");
            guest->outsideOfPark = false;
            guest->setParkEntryTime(state.currentTicks);
        }
        state.park.entrances.push_back({ 64, 64, 16, 0 }); // Initial native scenario placement source.
        Check(context->GetObjectManager().LoadObject("rct2.peep_animations.handyman"), "Fixture staff object missing");
        auto* wooden = context->GetObjectManager().LoadObject("rct2.ride.ptct1");
        Check(
            wooden && context->GetObjectManager().LoadObject("rct2.station.plain")
                && context->GetObjectManager().LoadObject("rct2.terrain_surface.grass")
                && context->GetObjectManager().LoadObject("rct2.terrain_edge.rock"),
            "Fixture ride/terrain metadata missing");
        const auto woodenIndex = context->GetObjectManager().GetLoadedObjectEntryIndex(wooden);
        RideTypeSetInvented(RIDE_TYPE_WOODEN_ROLLER_COASTER);
        RideEntrySetInvented(woodenIndex); // Initial scenario research fixture.
        auto capture = [&](const char* name) {
            const auto* output = std::getenv("POCKET_MAP_CAPTURES");
            if (!output || argc != 4)
                return;
            std::filesystem::create_directories(output);
            // SDL dummy has no hardware texture. Activate the existing native software context
            // to update its camera; capture pixels with the production X8 raster renderer.
            if (auto* native = dynamic_cast<Drawing::X8DrawingEngine*>(context->GetDrawingEngine()))
            {
                native->Drawing::X8DrawingEngine::Resize(512, 384);
                native->Drawing::X8DrawingEngine::BeginDraw();
                if (auto* main = WindowGetMain())
                    ViewportUpdatePosition(main);
                native->Drawing::X8DrawingEngine::EndDraw();
            }
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            Drawing::UpdatePaletteEffects();
            auto& rt = *engine.getRT();
            // Genuine native scenario viewport with the production Pocket page drawn by its real window.
            if (auto* main = WindowGetMain(); main && main->viewport)
            {
                ViewportRender(rt, main->viewport);
            }
            Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
            Image image;
            image.Width = 512;
            image.Height = 384;
            image.Depth = 8;
            image.Stride = rt.LineStride();
            image.Palette = Drawing::gPalette;
            const auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(output) / name).string(), image, ImageFormat::png);
            engine.EndDraw();
        };
        H::ParkMap::State map;
        map.Begin();
        for (int i = 0; i < 4; ++i)
            map.Update();
        Check(map.ready && map.extent == 132, "Rectangular map cache incomplete");
        for (int y = 32; y < 47 * 32; y += 32)
            for (int x = 32; x < 63 * 32; x += 32)
            {
                const auto p = W::NativeMiniMapCoords({ x, y });
                const auto c = W::NativeMiniMapTileColour({ x, y }, false);
                Check(p.x > 0 && p.y >= 0 && p.x < map.extent && p.y < map.extent, "Native coordinate outside raster");
                Check(map.pixels[p.y * map.extent + p.x] == c[1], "Cache colour differs from desktop map");
            }
        map.focus = { -10000, 100000 };
        map.Clamp();
        Check(map.focus == CoordsXY{ 32, 46 * 32 }, "Focus escaped map bounds");
        map.Mode();
        Check(map.rotation == 255, "Mode retained stale raster");
        map.Update();
        H::ControllerInput input;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm };
        auto openOverview = [&] {
            W::HandheldDashboardOpen();
            W::HandheldDashboardMove(0, -static_cast<int>(W::HandheldDashboardSectionIndex()));
            H::HandleMenuInput(input, confirm);
            Check(W::HandheldOverviewIsOpen(), "Overview entry failed");
        };
        auto parkTab = [&](size_t tab) {
            for (size_t i = 0; i < 3 && W::HandheldManagementPresentation().selectedTab != tab; ++i)
            {
                H::Frame next;
                next.actions = { H::Action::nextOption };
                H::HandleMenuInput(input, next);
            }
            Check(W::HandheldManagementPresentation().selectedTab == tab, "Park tab navigation failed");
        };
        auto selectParkAction = [&](const char* label) {
            const auto m = W::HandheldManagementPresentation();
            const auto it = std::find(m.rows.begin(), m.rows.end(), label);
            Check(it != m.rows.end(), "Named park action missing");
            W::HandheldMenuMove(static_cast<int>(it - m.rows.begin()) - static_cast<int>(m.selected));
        };
        auto openRecord = [&](const char* label) {
            openOverview();
            parkTab(2);
            selectParkAction(label);
            H::HandleMenuInput(input, confirm);
        };
        auto openMap = [&] {
            openOverview();
            parkTab(0);
            selectParkAction("Park map...");
            H::HandleMenuInput(input, confirm);
            Check(W::HandheldParkMapIsOpen(), "Pocket map entry failed");
        };
        openMap();
        for (int rotation = 0; rotation < 4; ++rotation)
        {
            H::Frame frame;
            frame.actions = { H::Action::contextPrimary };
            H::HandleMenuInput(input, frame);
            map.Update();
            Check(map.rotation == GetCurrentRotation(), "Rotation cache stale");
            map.focus = { 32 * 32, 24 * 32 };
            const auto before = W::NativeMiniMapCoords(map.focus);
            map.Move(1, 0);
            const auto after = W::NativeMiniMapCoords(map.focus);
            Check(after.x == before.x + 1 && after.y == before.y + 1, "Native camera-relative tile navigation differs");
        }
        H::Frame move;
        move.navigateX = 1;
        move.navigateY = 1;
        H::HandleMenuInput(input, move);
        for (auto a : { H::Action::contextSecondary, H::Action::previousOption, H::Action::nextOption })
        {
            H::Frame f;
            f.actions = { a };
            H::HandleMenuInput(input, f);
        }
        W::HandheldMenuBack();
        Check(W::HandheldOverviewIsOpen(), "Map Back failed");
        selectParkAction("Park map...");
        H::HandleMenuInput(input, confirm);
        H::HandleMenuInput(input, confirm);
        Check(!W::HandheldParkMapIsOpen(), "Locate failed to return to park");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        GameActions::MapChangeSizeAction resize({ 80, 56 });
        Check(GameActions::Query(&resize, state).error == GameActions::Status::ok, "Native map resize query failed");
        Check(GameActions::Execute(&resize, state).error == GameActions::Status::ok, "Native map resize failed");
        GameActions::ProcessQueue(state);
        map.Update();
        Check(map.size == state.mapSize && map.extent == 164, "Changed dimensions retained cache");
        namespace I = H::ParkInformation;
        Check(
            I::PlainText("{RED}Río{NEWLINE}Second", true) == "Río Second"
                && I::PlainText("{RED}Río{NEWLINE}Second") == "Río\nSecond",
            "Native formatting escaped into list/detail presentation");
        Check(I::NewsList().empty(), "Native initial news should be empty");
        Check(I::NewsPresentation({}, 0).status == "No park news yet", "Empty news state missing");
        const uint32_t assoc = 640u | (640u << 16);
        News::AddItemToQueue(state.park.newsItems, News::ItemType::blank, "Native fixture location message", assoc);
        auto news = I::NewsList();
        Check(news.size() == 1 && !news[0].archived && I::Current(news[0]), "Native news ordering/membership failed");
        const auto location = I::Location(news[0]);
        Check(location && location->x == 640 && location->y == 640, "Native news location failed");
        auto changed = news[0];
        changed.item.text += " changed";
        Check(!I::Current(changed) && !I::Location(changed), "Stale news text accepted");
        News::CloseCurrentItem(state.park.newsItems);
        Check(!I::Current(news[0]), "Moved news retained recent baseline");
        news = I::NewsList();
        Check(news.size() == 1 && news[0].archived && I::Current(news[0]), "Archived news missing");
        News::RemoveItem(state.park.newsItems, News::ItemHistoryStart);
        Check(!I::Current(news[0]) && !I::Location(news[0]), "Expired archived message accepted");
        Check(I::Awards(0).status == "No active awards", "Empty awards missing");
        AwardGrant(AwardType::bestStaff);
        Check(
            I::Awards(0).rows.size() == 2 && I::Awards(0).rows[0].starts_with("Positive:"), "Native award information missing");
        Check(state.park.ratingHistory[0] == kParkRatingHistoryUndefined, "Fixture history unexpectedly defined");
        Park::UpdateHistories(state.park);
        Check(
            state.park.ratingHistory[0] == state.park.rating && state.park.ratingHistory[1] == kParkRatingHistoryUndefined,
            "Native history order/undefined samples changed");
        const auto history = I::History(false, 0);
        Check(history.stats[0].second == std::to_string(state.park.rating), "History invented current rating");
        openRecord("Park news...");
        Check(W::HandheldParkMapSubjectsPresentation().title == "Park news", "News entry failed");
        W::HandheldMenuBack();
        Check(
            W::HandheldOverviewIsOpen() && W::HandheldManagementPresentation().selectedTab == 2, "News Back lost Records tab");
        openRecord("Park awards...");
        W::HandheldMenuBack();
        Check(
            W::HandheldOverviewIsOpen() && W::HandheldManagementPresentation().selectedTab == 2,
            "Awards Back lost Records tab");
        openRecord("Park history...");
        H::HandleMenuInput(input, confirm);
        W::HandheldMenuBack();
        Check(
            W::HandheldOverviewIsOpen() && W::HandheldManagementPresentation().selectedTab == 2,
            "History Back lost Records tab");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        namespace M = H::MapSubjects;
        const auto guests = M::Roster(2, 0);
        Check(guests.size() == 2 && M::Roster(2, 1).empty(), "Map guest filter membership differs");
        const auto guest = std::get<H::GuestManagement::Target>(guests.front());
        Check(
            H::GuestManagement::SetTracking(guest, H::GuestManagement::Resolve(guest)->peepFlags, true, true).error
                == GameActions::Status::ok,
            "Native tracking action failed");
        GameActions::ProcessQueue(state);
        Check(
            M::Roster(2, 1).size() == 1 && W::NativeMiniMapPeepMarkers(false, true, -1).size() == 1,
            "Tracked map filter differs");
        auto stale = guest;
        ++stale.generation;
        Check(!M::Current(stale) && !M::Location(stale), "Reused map target accepted");
        const auto markers = W::NativeMiniMapPeepMarkers(false, false, -1);
        Check(
            markers.size() == 2 && markers.front().position == W::NativeMiniMapCoords({ 640, 640 }),
            "Native peep overlay coordinate differs");
        Check(
            H::StaffManagement::Hire(StaffType::handyman, true).error == GameActions::Status::ok,
            "Native hire for map roster failed");
        GameActions::ProcessQueue(state);
        const auto staff = M::Roster(3, 0);
        Check(staff.size() == 1 && M::Roster(3, 1).size() == 1 && M::Roster(3, 2).empty(), "Map role filters differ");
        const auto employee = std::get<H::StaffManagement::Target>(staff.front());
        openMap();
        W::HandheldParkMapControl(0);
        W::HandheldParkMapControl(0);
        H::Frame subjects;
        subjects.actions = { H::Action::openBuild };
        H::HandleMenuInput(input, subjects);
        Check(W::HandheldParkMapSubjectsPresentation().rows[0] == M::Name(guests.front()), "Map guest roster entry failed");
        H::HandleMenuInput(input, confirm);
        H::HandleMenuInput(input, confirm);
        Check(
            W::HandheldParkMapIsOpen() && W::HandheldParkMapLocation() == std::optional<CoordsXY>{ { 640, 640 } },
            "Show subject on map failed");
        const auto focus = W::HandheldParkMapLocation();
        H::HandleMenuInput(input, subjects);
        H::HandleMenuInput(input, confirm);
        W::HandheldMenuMove(1);
        H::HandleMenuInput(input, confirm);
        Check(W::HandheldGuestsIsOpen(), "Map guest inspection handoff failed");
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        Check(W::HandheldParkMapIsOpen() && W::HandheldParkMapLocation() == focus, "Inspection lost map context");
        W::HandheldParkMapControl(0);
        H::HandleMenuInput(input, subjects);
        H::HandleMenuInput(input, confirm);
        W::HandheldMenuMove(1);
        H::HandleMenuInput(input, confirm);
        Check(W::HandheldStaffIsOpen(), "Map staff inspection handoff failed");
        W::HandheldMenuBack();
        GameActions::StaffFireAction fire(employee.id);
        Check(GameActions::Execute(&fire, state).error == GameActions::Status::ok, "Native map stale target setup failed");
        GameActions::ProcessQueue(state);
        Check(!M::Current(employee) && !M::Location(employee), "Departed staff map target accepted");
        W::HandheldMenuMove(1);
        H::HandleMenuInput(input, confirm);
        Check(!W::HandheldStaffIsOpen(), "Departed map subject inspected");
        capture("fixture-14-stale-subject.png");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        openMap();
        W::HandheldParkMapControl(0);
        W::HandheldParkMapControl(0);
        H::HandleMenuInput(input, subjects);
        W::HandheldMenuMove(2);
        H::HandleMenuInput(input, confirm);
        Check(
            W::HandheldParkMapSubjectsPresentation().title == "Tracked guests"
                && W::HandheldParkMapSubjectsPresentation().stats[0].second == "1",
            "Controller tracked filter differs");
        Check(
            H::GuestManagement::SetTracking(guest, H::GuestManagement::Resolve(guest)->peepFlags, false, true).error
                == GameActions::Status::ok,
            "Native filter membership change failed");
        GameActions::ProcessQueue(state);
        Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onUpdate();
        Check(W::HandheldParkMapSubjectsPresentation().stats[0].second == "0", "Live map filter retained departed membership");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        News::AddItemToQueue(state.park.newsItems, News::ItemType::peep, "Native subject fixture", guest.id.ToUnderlying());
        auto subjectNews = I::NewsList();
        const auto found = std::find_if(
            subjectNews.begin(), subjectNews.end(), [&](const auto& item) { return item.guest && item.guest->id == guest.id; });
        Check(found != subjectNews.end() && I::SubjectCurrent(*found), "Native news subject identity missing");
        const auto reviewed = *found;
        openRecord("Park news...");
        W::HandheldMenuMove(static_cast<int>(found - subjectNews.begin()));
        H::HandleMenuInput(input, confirm);
        H::HandleMenuInput(input, confirm);
        Check(W::HandheldGuestsIsOpen(), "News inspection handoff failed");
        W::HandheldMenuBack();
        Check(W::HandheldParkMapSubjectsPresentation().breadcrumb == "Overview / Park news", "News inspection recovery failed");
        News::DisableNewsItems(state.park.newsItems, News::ItemType::peep, guest.id.ToUnderlying());
        Check(!I::SubjectCurrent(reviewed) && !I::Location(reviewed), "Disabled/stale native subject accepted");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        GameActions::RideCreateAction create(
            RIDE_TYPE_WOODEN_ROLLER_COASTER, woodenIndex, 0, 0, 0, RideInspection::every30Minutes);
        const auto created = GameActions::Execute(&create, state);
        Check(created.error == GameActions::Status::ok, "Native ride creation for map membership failed");
        GameActions::ProcessQueue(state);
        Check(M::Roster(1, 0).size() == 1, "Native ride map roster differs");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        openMap();
        W::HandheldParkMapControl(0);
        H::HandleMenuInput(input, subjects);
        H::HandleMenuInput(input, confirm);
        W::HandheldMenuMove(1);
        H::HandleMenuInput(input, confirm);
        Check(W::HandheldRidesIsOpen(), "Map ride inspection handoff failed");
        W::HandheldMenuBack();
        Check(
            W::HandheldParkMapSubjectsPresentation().title == M::Name(M::Roster(1, 0).front()),
            "Ride inspection return failed");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        openOverview();
        parkTab(0);
        selectParkAction("Park map...");
        Check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "Virtual controller init failed");
        const auto device = SDL_JoystickAttachVirtual(
            SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        Check(device >= 0, "Virtual controller missing");
        auto* joystick = SDL_JoystickOpen(device);
        Check(joystick, "Virtual joystick missing");
        auto pump = [&] {
            SDL_Event e;
            while (SDL_PollEvent(&e))
                input.handleEvent(e);
        };
        pump();
        input.update(true, 100);
        pump();
        input.update(true, 110);
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 1);
        SDL_JoystickUpdate();
        pump();
        const auto pressed = input.update(true, 120);
        Check(!pressed.actions.empty(), "Mapped A missing");
        H::HandleMenuInput(input, pressed);
        Check(W::HandheldParkMapIsOpen(), "Held-A entry failed");
        const auto held = input.update(true, 130);
        Check(held.actions.empty(), "Held A repeated into map");
        H::HandleMenuInput(input, held);
        Check(W::HandheldParkMapIsOpen(), "Held A automatically located tile");
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 0);
        SDL_JoystickUpdate();
        pump();
        input.update(true, 140);
        SDL_JoystickClose(joystick);
        SDL_JoystickDetachVirtual(device);
        pump();
        input.suspend();
        GameActions::MapChangeSizeAction large({ 1001, 1001 });
        Check(GameActions::Execute(&large, state).error == GameActions::Status::ok, "Native maximum map resize failed");
        GameActions::ProcessQueue(state);
        map.Update();
        Check(map.extent == 2006 && map.next == 2048 && !map.ready, "Large map did not respect incremental budget");
        map.focus = { 100000, 100000 };
        map.Clamp();
        Check(map.focus == CoordsXY{ 999 * 32, 999 * 32 }, "Large map focus escaped bounds");
        GameActions::MapChangeSizeAction shrink({ 80, 56 });
        Check(GameActions::Execute(&shrink, state).error == GameActions::Status::ok, "Native map shrink failed");
        GameActions::ProcessQueue(state);
        map.Update();
        Check(map.extent == 164 && map.focus == CoordsXY{ 78 * 32, 54 * 32 }, "Shrunk map focus/cache not clamped");
        SaveFlags flags{};
        const auto save = std::string(argv[2]) + "/map.park";
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Map save/reload failed");
        Check(!W::HandheldParkMapIsOpen(), "Load retained transient map menu");
        map.Update();
        Check(map.size == state.mapSize, "Map dimensions not recovered");
        if (const auto* scenario = std::getenv("POCKET_MAP_SCENARIO"))
        {
            Check(context->LoadParkFromFile(scenario, false, true), "Capture scenario failed");
            if (!state.park.flags.has(ParkFlag::parkOpen))
            {
                auto status = H::ParkOverview::Begin();
                if (status)
                {
                    status->open = true;
                    Check(
                        H::ParkOverview::Action(*status, true).error == GameActions::Status::ok,
                        "Native capture park opening failed");
                    GameActions::ProcessQueue(state);
                }
            }
            // Observe the supplied scenario through native simulation; never populate capture metrics manually.
            for (int i = 0; i < 16384; ++i)
                gameStateUpdateLogic();
            std::cout << "Capture native scenario guests=" << state.park.numGuestsInPark << " news=" << I::NewsList().size()
                      << " awards=" << state.park.currentAwards.size() << "\n";
        }
        if (const auto* output = std::getenv("POCKET_MAP_CAPTURES"); output && argc == 4)
        {
            std::filesystem::create_directories(output);

            Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
            openMap();
            auto* menu = Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu);
            for (int i = 0; i < 1000; ++i)
                menu->onUpdate();
            capture("map-01-terrain.png");
            W::HandheldParkMapControl(0);
            for (int i = 0; i < 1000; ++i)
                menu->onUpdate();
            capture("map-02-rides.png");
            W::HandheldParkMapControl(2);
            W::HandheldConstructionMove(4, 2);
            capture("map-03-zoom.png");
            W::HandheldParkMapControl(0);
            menu->onUpdate();
            capture("map-09-guests.png");
            H::Frame subjectFrame;
            subjectFrame.actions = { H::Action::openBuild };
            H::HandleMenuInput(input, subjectFrame);
            capture("map-10-subject-roster.png");
            if (!M::Roster(2, 0).empty())
            {
                H::HandleMenuInput(input, confirm);
                capture("map-11-subject-detail.png");
                H::HandleMenuInput(input, confirm);
                menu->onUpdate();
                capture("map-15-located-subject.png");
                H::HandleMenuInput(input, subjectFrame);
                H::HandleMenuInput(input, confirm);
                W::HandheldMenuMove(1);
                H::HandleMenuInput(input, confirm);
                capture("map-12-inspection.png");
                W::HandheldMenuBack();
                W::HandheldMenuBack();
            }
            W::HandheldMenuBack();
            W::HandheldParkMapControl(0);
            menu->onUpdate();
            capture("map-13-staff.png");
            H::HandleMenuInput(input, subjectFrame);
            capture("map-16-staff-roster.png");
            W::HandheldMenuBack();
            W::HandheldMenuBack();
            W::HandheldMenuMove(3);
            H::HandleMenuInput(input, confirm);
            capture("information-04-news.png");
            const auto nativeNews = I::NewsList();
            if (!nativeNews.empty())
            {
                H::HandleMenuInput(input, confirm);
                capture("information-05-news-details.png");
                W::HandheldMenuBack();
            }
            W::HandheldMenuBack();
            W::HandheldMenuMove(4);
            H::HandleMenuInput(input, confirm);
            capture("information-06-awards.png");
            W::HandheldMenuBack();
            W::HandheldMenuMove(5);
            H::HandleMenuInput(input, confirm);
            capture("information-07-rating-history.png");
            H::HandleMenuInput(input, confirm);
            capture("information-08-guest-history.png");
        }
        H::EndTerrain();
        std::cout << "Native map raster/bounds/rotation/resize/save and SDL navigation/locate passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}

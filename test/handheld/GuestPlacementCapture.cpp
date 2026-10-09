// SPDX-License-Identifier: GPL-3.0-only
// Real asset-backed park, production Pocket components and native peep actions. No fixture visitors.
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/GuestManagement.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/handheld/PeepPlacement.h>
#include <openrct2-ui/handheld/StaffManagement.h>
#include <openrct2-ui/handheld/TextEntry.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/park/ParkMarketingAction.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/management/Marketing.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace D = H::Dashboard;
namespace G = H::GuestManagement;
namespace S = H::StaffManagement;
namespace P = H::PeepPlacement;
int main(int argc, char** argv)
{
    if (argc != 6)
        return 2;
    gCustomOpenRCT2DataPath = argv[1];
    gCustomUserDataPath = argv[5];
    gOpenRCT2Headless = true;
    gOpenRCT2NoGraphics = false;
    auto env = CreatePlatformEnvironment();
    env->SetBasePath(DirBase::rct2, argv[2]);
    auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), Ui::CreateDummyUiContext());
    Config::SetDefaults();
    Config::Get().general.rct2Path = argv[2];
    if (!context->Initialise() || !context->LoadParkFromFile(argv[3], false, true))
        return 3;
    auto& state = getGameState();
    gGamePaused = 0;
    GameActions::ParkSetParameterAction open(GameActions::ParkParameter::open);
    if (GameActions::Query(&open, state).error != GameActions::Status::ok)
        return 7;
    GameActions::Execute(&open, state);
    GameActions::ProcessQueue(state);
    for (const auto& ride : RideManager(state))
        if (ride.isRide() && ride.status == RideStatus::closed)
        {
            GameActions::RideSetStatusAction action(ride.id, RideStatus::open);
            if (GameActions::Query(&action, state).error == GameActions::Status::ok)
            {
                GameActions::Execute(&action, state);
                GameActions::ProcessQueue(state);
            }
        }
    GameActions::ParkMarketingAction advertise(ADVERTISING_CAMPAIGN_PARK, 0, 2);
    if (GameActions::Query(&advertise, state).error == GameActions::Status::ok)
    {
        GameActions::Execute(&advertise, state);
        GameActions::ProcessQueue(state);
    }
    // Run the native scenario naturally if it starts without visitors; no guest fixtures or need edits.
    std::cout << "Initial guests: " << G::Roster().size() << std::endl;
    for (int ticks = 0; G::Roster().size() < 20 && ticks < 30000; ++ticks)
        gameStateUpdateLogic();
    for (int ticks = 0; G::Groups(G::GroupMode::thoughts).empty() && ticks < 8192; ++ticks)
        gameStateUpdateLogic();
    gGamePaused = 1;
    const auto roster = G::Roster();
    if (roster.empty())
        return 4;
    const auto selected = std::find_if(roster.begin(), roster.end(), [](const auto& person) {
        const auto* peep = G::Resolve(person);
        return peep && peep->canBePickedUp();
    });
    if (selected == roster.end())
        return 4;
    const auto target = *selected;
    auto require = [](bool ok) {
        if (!ok)
            throw std::runtime_error("Capture native action failed");
    };
    const auto* guest = G::Resolve(target);
    require(G::SetName(target, guest->getName(), "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456", true).error == GameActions::Status::ok);
    GameActions::ProcessQueue(state);
    require(G::SetTracking(target, guest->peepFlags, true, true).error == GameActions::Status::ok);
    GameActions::ProcessQueue(state);
    require(S::Hire(StaffType::handyman, true).error == GameActions::Status::ok);
    GameActions::ProcessQueue(state);
    const auto employee = S::Roster(StaffType::handyman).back();
    const auto cash = state.park.cash;
    const auto ledger = state.park.expenditureTable;
    // Choose a busy native park region, without adding construction or altering the scenario.
    CoordsXY tile{};
    int best = -1;
    for (int y = 64; y < (state.mapSize.y - 2) * 32; y += 64)
        for (int x = 64; x < (state.mapSize.x - 2) * 32; x += 64)
        {
            int score = 0;
            for (int dy = -96; dy <= 96; dy += 32)
                for (int dx = -96; dx <= 96; dx += 32)
                    if (MapIsLocationValid({ x + dx, y + dy }))
                        for (const auto* e : TileElementsView(CoordsXY{ x + dx, y + dy }))
                            score += e->getType() == TileElementType::track ? 4 : e->getType() == TileElementType::path ? 1 : 0;
            if (score > best)
            {
                best = score;
                tile = { x, y };
            }
        }
    std::filesystem::create_directories(argv[4]);
    const char* names[] = { "guest-action-groups.png",     "guest-thought-groups.png", "guest-filtered-roster.png",
                            "guest-inspection.png",        "guest-name-keyboard.png",  "guest-name-review.png",
                            "guest-tracking-review.png",   "guest-preferences.png",    "guest-placement.png",
                            "guest-invalid-placement.png", "guest-cancel-review.png",  "staff-placement.png",
                            "guest-dense-inspection.png" };
    CoordsXYZ valid{};
    bool found = false;
    require(P::Begin({ target, {} }).error == GameActions::Status::ok);
    GameActions::ProcessQueue(state);
    for (int y = 32; !found && y < (state.mapSize.y - 1) * 32; y += 32)
        for (int x = 32; !found && x < (state.mapSize.x - 1) * 32; x += 32)
            for (const auto* e : TileElementsView(CoordsXY{ x, y }))
                if (e->getType() == TileElementType::path)
                {
                    CoordsXYZ position{ x, y, e->getBaseZ() };
                    if (P::Place(position, false).error == GameActions::Status::ok)
                    {
                        valid = position;
                        found = true;
                        break;
                    }
                }
    require(found);
    P::End();
    const CoordsXYZ invalid{ 0, 0, MapGetSurfaceElementAt(CoordsXY{ 0, 0 })->getBaseZ() };
    for (int mode = 0; mode < 13; ++mode)
    {
        const bool placing = mode == 8 || mode == 9 || mode == 11;
        if (placing || mode == 10)
        {
            require(
                P::Begin(mode == 11 ? P::Target{ {}, employee } : P::Target{ target, {} }).error == GameActions::Status::ok);
            GameActions::ProcessQueue(state);
        }
        Viewport viewport{};
        viewport.width = 512;
        viewport.height = 384;
        const CoordsXYZ location = placing ? (mode == 9 ? invalid : valid)
                                           : CoordsXYZ{ tile.x + 16, tile.y + 16, MapGetSurfaceElementAt(tile)->getBaseZ() };
        const auto focus = Translate3DTo2DWithZ(0, location);
        viewport.viewPos = { focus.x - (placing ? 256 : 328), focus.y - 192 };
        Drawing::X8DrawingEngine engine(context->GetUiContext());
        engine.Resize(512, 384);
        engine.BeginDraw();
        auto& rt = *engine.getRT();
        ResetAllSpriteQuadrantPlacements();
        Drawing::UpdatePaletteEffects();
        ViewportRender(rt, &viewport);
        D::State dashboard;
        dashboard.section = D::Section::guests;
        dashboard.focus = D::Focus::content;
        H::ManagementPresentation model;
        H::TextEntry entry{ guest->getName() };
        switch (mode)
        {
            case 0:
            case 1:
                model = G::GroupPresentation(
                    mode == 0 ? G::GroupMode::actions : G::GroupMode::thoughts,
                    G::Groups(mode == 0 ? G::GroupMode::actions : G::GroupMode::thoughts), 0);
                break;
            case 2:
            {
                const auto groups = G::Groups(G::GroupMode::actions);
                require(!groups.empty());
                model = G::Presentation(G::View::roster, 0, G::Roster({ groups.front(), false }));
                model.title = "Filtered guests";
                model.rows.back() = "Back to groups";
                model.status = std::to_string(G::Roster({ groups.front(), false }).size()) + " guests / "
                    + groups.front().label;
                break;
            }
            case 3:
                model = G::Presentation(G::View::inspection, 0, roster, target);
                break;
            case 4:
            case 5:
                if (mode == 5)
                {
                    entry.Press(43); // Valid draft, not a no-op review of the existing name.
                    entry.Press(25);
                    require(G::SetName(target, guest->getName(), entry.text, false).error == GameActions::Status::ok);
                }
                model = S::NamePresentation(entry, mode == 4 ? 46 : 1, mode == 5);
                model.breadcrumb = mode == 4 ? "Guests / Name keyboard" : "Guests / Review name";
                break;
            case 6:
                model = G::TrackingPresentation(target, false, 1);
                break;
            case 7:
                model = G::Presentation(G::View::preferences, 0, roster, target);
                break;
            case 10:
                model = P::CancelPresentation(1);
                break;
            case 12:
                model = G::Presentation(G::View::inspection, 9, roster, target);
                break;
        }
        if (placing)
        {
            H::DrawHud(rt, { 512, 384 }, H::NativeHud());
            P::Preview(true, { 256, 192 });
            Drawing::pickupPeepDraw(rt);
            P::DrawOverlay(rt, H::Concept::kOrigin);
            H::DrawConstructionFooter(rt, { 512, 384 }, "Place", "Options");
            const auto query = P::Place(location, false);
            require((mode == 9) == (query.error != GameActions::Status::ok));
            const auto anchor = H::Layout::ContextAnchor({ 512, 384 }, H::kContextSize);
            if (query.error == GameActions::Status::ok)
                H::DrawCostBadge(rt, anchor, H::FormatMoney(query.cost));
            else
                H::DrawContextMessage(rt, anchor, "PLACE PERSON  " + query.getErrorMessage());
            H::DrawMapFocus(rt, { 256, 192 });
        }
        else if (mode == 10)
            H::DrawManagementPanel(rt, { 512, 384 }, model);
        else
            D::Draw(rt, { 512, 384 }, dashboard, model, mode < 4 || mode > 6);
        Image image;
        image.Width = 512;
        image.Height = 384;
        image.Depth = 8;
        image.Stride = rt.LineStride();
        image.Palette = Drawing::gPalette;
        auto* pixels = reinterpret_cast<uint8_t*>(rt.bits);
        image.Pixels.assign(pixels, pixels + rt.LineStride() * rt.height);
        Imaging::WriteToFile((std::filesystem::path(argv[4]) / names[mode]).string(), image, ImageFormat::png);
        engine.EndDraw();
        if (mode == 8 || mode == 11)
        {
            require(P::Place(valid, true).error == GameActions::Status::ok);
            GameActions::ProcessQueue(state);
            require(P::finished);
        }
        if (placing || mode == 10)
            P::End();
        require(state.park.cash == cash && state.park.expenditureTable == ledger);
    }
    SaveFlags saveFlags{};
    const auto save = (std::filesystem::path(argv[5]) / "guest-placement-live.park").string();
    require(ScenarioSave(state, save, saveFlags) && context->LoadParkFromFile(save));
    require(
        G::Resolve(target) && G::Resolve(target)->getName() == "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456"
        && G::Resolve(target)->peepFlags.has(PeepFlag::tracking) && S::Resolve(employee)
        && S::Resolve(employee)->getLocation().x == valid.x + 16 && state.park.cash == cash
        && state.park.expenditureTable == ledger);
    std::cout << "Native scenario " << state.park.name << "; guests " << roster.size()
              << "; 13 asset-backed native Pocket guest/placement captures at 512x384\n";
    return 0;
}

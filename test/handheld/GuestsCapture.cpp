// SPDX-License-Identifier: GPL-3.0-only
// Real private scenario, production Guests presentation and Dashboard; no fixture guests or information.
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/GuestManagement.h>
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
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace D = H::Dashboard;
namespace G = H::GuestManagement;
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
    gGamePaused = 1;
    const auto roster = G::Roster();
    if (roster.empty())
        return 4;
    const auto target = roster.front();
    const auto cash = state.park.cash, loan = state.park.bankLoan, fee = state.park.entranceFee;
    const auto ledger = state.park.expenditureTable;
    const auto* guest = G::Resolve(target);
    const auto guestCash = guest->cashInPocket, guestSpent = guest->cashSpent;
    const auto items = G::Inventory(*guest), thoughts = G::Thoughts(*guest);
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
    for (int mode = 0; mode < 5; ++mode)
    {
        Viewport viewport{};
        viewport.width = 512;
        viewport.height = 384;
        const auto focus = Translate3DTo2DWithZ(0, { tile.x + 16, tile.y + 16, MapGetSurfaceElementAt(tile)->getBaseZ() });
        viewport.viewPos = { focus.x - 328, focus.y - 192 };
        Drawing::X8DrawingEngine engine(context->GetUiContext());
        engine.Resize(512, 384);
        engine.BeginDraw();
        auto& rt = *engine.getRT();
        ResetAllSpriteQuadrantPlacements();
        Drawing::UpdatePaletteEffects();
        ViewportRender(rt, &viewport);
        D::State dashboard;
        dashboard.section = D::Section::guests;
        dashboard.focus = mode == 0 ? D::Focus::rail : D::Focus::content;
        const G::View views[] = { G::View::overview, G::View::roster, G::View::inspection, G::View::needs, G::View::thoughts };
        auto model = G::Presentation(views[mode], 0, roster, target);
        D::Draw(rt, { 512, 384 }, dashboard, model, true);
        const char* names[] = { "guests-overview.png", "guests-roster.png", "guests-inspection.png", "guests-needs.png",
                                "guests-thoughts.png" };
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
        if (state.park.cash != cash || state.park.bankLoan != loan || state.park.entranceFee != fee
            || state.park.expenditureTable != ledger || G::Roster() != roster || guest->cashInPocket != guestCash
            || guest->cashSpent != guestSpent || G::Inventory(*guest) != items || G::Thoughts(*guest) != thoughts)
            return 6;
    }
    std::cout << "Native scenario " << state.park.name << "; guests " << roster.size() << "; guest " << guest->getName()
              << "; native Guests captures at 512x384\n";
    return 0;
}

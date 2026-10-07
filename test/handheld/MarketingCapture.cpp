// SPDX-License-Identifier: GPL-3.0-only
// Real private scenario, production Marketing presentation and Dashboard; no fixture research values.
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Finances.h>
#include <openrct2-ui/handheld/MarketingManagement.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
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
namespace F = H::Finances;
namespace D = H::Dashboard;
namespace M = H::MarketingManagement;
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
    gGamePaused = 1;
    const auto cash = state.park.cash, loan = state.park.bankLoan, fee = state.park.entranceFee;
    // Actual native operation on an existing scenario ride, never fixture status writes.
    if (!M::Eligible(ADVERTISING_CAMPAIGN_RIDE))
        for (const auto& ride : RideManager(state))
            if (ride.isRide() && ride.status == RideStatus::closed)
            {
                auto open = GameActions::RideSetStatusAction(ride.id, RideStatus::open);
                if (GameActions::Query(&open, state).error == GameActions::Status::ok)
                {
                    GameActions::Execute(&open, state);
                    GameActions::ProcessQueue(state);
                    if (M::Eligible(ADVERTISING_CAMPAIGN_RIDE))
                        break;
                }
            }
    M::Draft active{ ADVERTISING_CAMPAIGN_PARK, 2, {} };
    const auto initialCash = state.park.cash;
    const auto quote = M::Action(active, false);
    if (quote.error != GameActions::Status::ok || M::Action(active, true).error != GameActions::Status::ok)
        return 4;
    GameActions::ProcessQueue(state);
    if (!MarketingGetCampaign(active.type) || state.park.cash != initialCash - quote.cost)
        return 5;
    const auto captureCash = state.park.cash;
    const auto types = M::Types();
    if (types.empty())
        return 6;
    M::Draft draft{ M::Eligible(ADVERTISING_CAMPAIGN_RIDE) ? ADVERTISING_CAMPAIGN_RIDE : types.front(), 3, {} };
    const auto targets = M::Targets(draft.type);
    if (M::NeedsTarget(draft.type) && (targets.empty() || !M::SelectTarget(draft, targets.front())))
        return 7;
    if (M::Action(draft, false).error != GameActions::Status::ok)
        return 8;
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
    for (int mode = 0; mode < 4; ++mode)
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
        dashboard.section = D::Section::marketing;
        dashboard.focus = mode == 0 ? D::Focus::rail : D::Focus::content;
        auto model = M::Presentation(
            mode < 2        ? M::View::overview
                : mode == 2 ? M::View::editor
                            : M::View::confirmation,
            mode == 3 ? 1 : 0, mode < 2 ? std::nullopt : std::optional<M::Draft>{ draft }, -1, M::ActiveTypes());
        D::Draw(rt, { 512, 384 }, dashboard, model, mode < 2);
        const char* names[] = { "marketing-overview-rail.png", "marketing-overview-content.png", "marketing-draft.png",
                                "marketing-confirmation.png" };
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
        if (state.park.cash != captureCash || state.park.bankLoan != loan || state.park.entranceFee != fee
            || !MarketingGetCampaign(active.type) || MarketingGetCampaign(draft.type))
            return 6;
    }
    std::cout << "Native scenario " << state.park.name << "; cash " << H::FormatMoney(cash) << "; loan " << H::FormatMoney(loan)
              << "; admission " << (F::Available(F::Field::admission) ? "editable" : "locked")
              << "; native Marketing captures at 512x384\n";
    return 0;
}

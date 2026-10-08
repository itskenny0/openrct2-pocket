// SPDX-License-Identifier: GPL-3.0-only
// Real private scenario, production Rides presentation and Dashboard; no fixture rides or information.
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/RideManagement.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Viewport.h>
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
namespace R = H::RideManagement;
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
    const auto roster = R::Roster();
    if (roster.empty())
        return 4;
    std::optional<R::Target> chosen;
    for (const auto& entry : roster)
        if (R::PricePermitted(*R::Resolve(entry)))
        {
            chosen = entry;
            break;
        }
    const auto target = chosen.value_or(roster.front());
    const auto* ride = R::Resolve(target);
    auto draft = R::Begin(target);
    if (draft)
        R::AdjustPrice(*draft, 1);
    else
        draft = R::Begin(target, ride->status == RideStatus::closed ? RideStatus::open : RideStatus::closed);
    if (!draft)
        return 5;
    const auto cash = state.park.cash, loan = state.park.bankLoan, fee = state.park.entranceFee;
    const auto ledger = state.park.expenditureTable;
    const auto price = ride->price[0];
    const auto status = ride->status;
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
        dashboard.section = D::Section::rides;
        dashboard.focus = mode == 0 ? D::Focus::rail : D::Focus::content;
        const R::View views[] = { R::View::roster, R::View::inspection, R::View::statistics, R::View::confirmation };
        auto model = R::Presentation(views[mode], mode == 3 ? 1 : 0, roster, target, draft);
        D::Draw(rt, { 512, 384 }, dashboard, model, mode < 3);
        const char* names[] = { "rides-roster.png", "rides-inspection.png", "rides-statistics.png", "rides-confirmation.png" };
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
            || state.park.expenditureTable != ledger || R::Roster() != roster || ride->price[0] != price
            || ride->status != status)
            return 6;
    }
    std::cout << "Native scenario " << state.park.name << "; rides " << roster.size() << "; inspected " << ride->getName()
              << "; pricing " << R::PricePermitted(*ride) << "; native Rides captures at 512x384\n";
    // Additional native-scenario integration: exercise permitted status changes on actual built rides,
    // not fixture records. Each query/execute must agree and preserve exact cash and ledger.
    size_t opened = 0, tested = 0;
    for (const auto& entry : roster)
    {
        for (auto requested : { RideStatus::open, RideStatus::closed, RideStatus::testing, RideStatus::closed })
        {
            auto change = R::Begin(entry, requested);
            if (!change || R::Resolve(entry)->status == requested)
                continue;
            const auto query = R::Action(*change, false);
            if (query.error != GameActions::Status::ok)
                continue;
            if (R::Action(*change, true).error != GameActions::Status::ok)
                return 8;
            GameActions::ProcessQueue(state);
            if (!R::Resolve(entry) || R::Resolve(entry)->status != requested || state.park.cash != cash
                || state.park.expenditureTable != ledger)
                return 9;
            opened += requested == RideStatus::open;
            tested += requested == RideStatus::testing;
        }
    }
    if (!opened || !tested)
        return 10;
    const auto save = (std::filesystem::path(argv[5]) / "native-rides.park").string();
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    if (!ScenarioSave(state, save, flags) || !context->LoadParkFromFile(save, false, true))
        return 11;
    if (state.park.cash != cash || state.park.expenditureTable != ledger || R::Roster().size() != roster.size())
        return 12;
    for (const auto& entry : R::Roster())
        if (R::Resolve(entry)->status != RideStatus::closed)
            return 13;
    std::cout << "Native scenario status integration: opened " << opened << ", tested " << tested
              << "; zero action charge; real ride save/reload passed\n";
    return 0;
}

// SPDX-License-Identifier: GPL-3.0-only
// Real private scenario and production Overview/Finances presentation; no fixture park data.
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Finances.h>
#include <openrct2-ui/handheld/ParkOverview.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace D = H::Dashboard;
namespace O = H::ParkOverview;
namespace F = H::Finances;
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
    const auto draft = O::Begin();
    if (!draft)
        return 4;
    const auto cash = state.park.cash, loan = state.park.bankLoan, fee = state.park.entranceFee;
    const auto ledger = state.park.expenditureTable;
    const auto objective = O::ObjectiveText();
    const auto outcome = O::Outcome();
    const auto baselineOpen = state.park.flags.has(ParkFlag::parkOpen);
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
        dashboard.section = mode == 3 ? D::Section::finances : D::Section::overview;
        dashboard.focus = mode == 0 ? D::Focus::rail : D::Focus::content;
        if (mode == 4)
        {
            if (O::Action(*draft, false).error != GameActions::Status::ok
                || O::Action(*draft, true).error != GameActions::Status::ok)
                return 8;
            GameActions::ProcessQueue(state);
            if (state.park.flags.has(ParkFlag::parkOpen) != draft->open)
                return 9;
        }
        auto model = mode == 3 ? F::Presentation(F::View::overview, 1)
            : mode == 2        ? O::Presentation(1, draft)
                               : O::Presentation(0);
        // The summary objective and both actions must fit without hiding native objective text.
        if (mode != 2 && mode != 3
            && (70 + model.stats.size() * 24 + model.bodyLines.size() * 16 + model.rows.size() * 30 + 30 > 338))
            return 10;
        D::Draw(rt, { 512, 384 }, dashboard, model, mode != 2);
        const char* names[] = { "overview-rail.png", "overview-summary.png", "overview-confirmation.png",
                                "overview-finances.png", "overview-updated.png" };
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
            || state.park.expenditureTable != ledger || O::ObjectiveText() != objective || O::Outcome() != outcome
            || (mode < 4 && state.park.flags.has(ParkFlag::parkOpen) != baselineOpen))
            return 6;
    }
    const auto save = (std::filesystem::path(argv[5]) / "native-overview.park").string();
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    if (!ScenarioSave(state, save, flags) || !context->LoadParkFromFile(save, false, true))
        return 11;
    if (state.park.cash != cash || state.park.expenditureTable != ledger || O::ObjectiveText() != objective
        || O::Outcome() != outcome || state.park.flags.has(ParkFlag::parkOpen) != draft->open)
        return 12;
    auto restored = O::Begin();
    if (!restored || O::Action(*restored, true).error != GameActions::Status::ok)
        return 13;
    GameActions::ProcessQueue(state);
    if (state.park.cash != cash || state.park.expenditureTable != ledger
        || state.park.flags.has(ParkFlag::parkOpen) != baselineOpen)
        return 14;
    std::cout << "Native scenario " << state.park.name
              << "; Overview captures at 512x384; native open/close zero charge and objective/status save/reload passed\n";
    return 0;
}

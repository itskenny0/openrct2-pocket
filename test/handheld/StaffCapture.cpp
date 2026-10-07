// SPDX-License-Identifier: GPL-3.0-only
// Real private scenario, production staff renderer and native automatic hiring; no fixture employees.
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/StaffManagement.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/peep/StaffHireNewAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace S = H::StaffManagement;
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
    std::optional<S::Target> employee;
    const auto before = S::Roster().size();
    const auto result = S::Hire(
        StaffType::handyman, true, [&](const GameActions::GameAction*, const GameActions::Result* response) {
            if (response->error == GameActions::Status::ok)
                employee = S::Identify(response->getData<GameActions::StaffHireNewActionResult>().StaffEntityId);
        });
    if (result.error != GameActions::Status::ok)
        return 4;
    GameActions::ProcessQueue(state);
    if (!employee || S::Roster().size() != before + 1 || state.park.cash != cash)
        return 5;
    // Hire native employees to demonstrate both roster pages; no synthetic entity/stat fixtures.
    while (S::Roster().size() < 10)
    {
        const auto role = S::Roster().size() < 5 ? StaffType::handyman : StaffType::mechanic;
        if (S::Hire(role, true).error != GameActions::Status::ok)
            return 7;
        GameActions::ProcessQueue(state);
    }
    const auto roster = S::Roster();
    const auto orders = S::Resolve(*employee)->staffOrders ^ STAFF_ORDERS_MOWING;
    // Choose a busy native park region, without adding construction or replacing world graphics.
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
    for (int mode = 0; mode < 7; ++mode)
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
        const auto model = S::Presentation(
            mode == 0 || mode == 6 ? S::View::roster
                : mode == 1        ? S::View::inspection
                : mode == 2        ? S::View::orders
                : mode == 3        ? S::View::hire
                : mode == 4        ? S::View::dismissal
                                   : S::View::statistics,
            mode == 3 || mode == 4 ? 1
                : mode == 6        ? roster.size() + 1
                                   : 0,
            roster, employee, StaffType::mechanic, orders);
        H::DrawManagementPanel(rt, { 512, 384 }, model);
        const char* names[] = { "staff-roster.png",    "staff-inspection.png", "staff-orders.png",     "staff-hire.png",
                                "staff-dismissal.png", "staff-statistics.png", "staff-roster-last.png" };
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
        if (state.park.cash != cash || state.park.bankLoan != loan || state.park.entranceFee != fee)
            return 6;
    }
    std::cout << "Native scenario " << state.park.name << "; " << roster.size()
              << " employees; native auto-hires, no upfront charge; captures at 512x384\n";
    return 0;
}

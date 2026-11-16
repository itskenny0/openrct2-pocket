// SPDX-License-Identifier: GPL-3.0-only
// Real private scenario, production two-pane shell; native finances and auto-hired employee, no fixture values.
#include <filesystem>
#include <iostream>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Finances.h>
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
namespace F = H::Finances;
namespace S = H::StaffManagement;
namespace D = H::Dashboard;
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
    if (!F::Available(F::Field::loan))
        return 4;
    auto draft = F::Begin(F::Field::loan);
    F::Adjust(draft, 1);
    if (draft.value == draft.baseline)
        F::Adjust(draft, -1);
    if (F::Action(draft, false).error != GameActions::Status::ok)
        return 5;
    std::optional<S::Target> employee;
    const auto before = S::Roster().size();
    if (S::Hire(
            StaffType::handyman, true,
            [&](const GameActions::GameAction*, const GameActions::Result* result) {
                if (result->error == GameActions::Status::ok)
                    employee = S::Identify(result->getData<GameActions::StaffHireNewActionResult>().StaffEntityId);
            })
            .error
        != GameActions::Status::ok)
        return 7;
    GameActions::ProcessQueue(state);
    if (!employee || S::Roster().size() != before + 1 || state.park.cash != cash)
        return 8;
    const auto roster = S::Roster();
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
        D::State dashboard;
        dashboard.focus = mode == 0 || mode == 2 || mode == 6 ? D::Focus::rail : D::Focus::content;
        H::ManagementPresentation model;
        if (mode == 2 || mode == 3 || mode == 4)
        {
            dashboard.section = D::Section::staff;
            model = S::Presentation(
                mode == 4 ? S::View::inspection : S::View::roster,
                mode == 3       ? 2
                    : mode == 4 ? 1
                                : 0,
                roster, employee);
        }
        else if (mode == 6)
        {
            dashboard.section = D::Section::research;
            model = D::Unavailable(dashboard.section);
        }
        else
            model = F::Presentation(
                mode == 5 ? F::View::confirmation : F::View::overview, mode == 5 ? 1 : 0,
                mode == 5 ? std::optional<F::Draft>{ draft } : std::nullopt);
        D::Draw(rt, { 512, 384 }, dashboard, model, mode != 5);
        const char* names[] = { "dashboard-finances-rail.png", "dashboard-finances-content.png",
                                "dashboard-staff-rail.png",    "dashboard-staff-content.png",
                                "dashboard-staff-detail.png",  "dashboard-loan-confirmation.png",
                                "dashboard-unavailable.png" };
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
    std::cout << "Native scenario " << state.park.name << "; cash " << H::FormatMoney(cash) << "; loan " << H::FormatMoney(loan)
              << "; admission " << (F::Available(F::Field::admission) ? "editable" : "locked")
              << "; two-pane Dashboard captures at 512x384\n";
    return 0;
}

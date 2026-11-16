// SPDX-License-Identifier: GPL-3.0-only
// Native private scenario, production picker/tool/shell rendering; no fixture objects or research.
#include <filesystem>
#include <iostream>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
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
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), Ui::CreateDummyUiContext());
        Config::SetDefaults();
        Config::Get().general.rct2Path = argv[2];
        Check(context->Initialise() && context->LoadParkFromFile(argv[3], false, true), "Scenario load failed");
        gGamePaused = 0;
        auto& state = getGameState();
        const auto initialCash = state.park.cash;
        const auto entries = F::Entries();
        Check(!entries.empty(), "No eligible native flat rides");
        for (const auto& e : entries)
            std::cout << "Eligible: " << e.name << "\n";
        auto selected = entries.front().identity.index;
        for (const auto& e : entries)
            if (FlatRidePlacement::Type(e.identity.index) == RIDE_TYPE_MERRY_GO_ROUND)
                selected = e.identity.index;
        Check(H::BeginFlatRide(selected), "Begin flat ride failed");
        CoordsXY tile{};
        int best = -1;
        const auto& ted = TrackMetadata::GetTrackElementDescriptor(
            GetRideTypeDescriptor(FlatRidePlacement::Type(selected)).StartTrackPiece);
        for (int y = 128; y < (state.mapSize.y - 4) * 32; y += 32)
            for (int x = 128; x < (state.mapSize.x - 4) * 32; x += 32)
            {
                const CoordsXY p{ x, y };
                bool clear = true;
                const auto base = MapGetSurfaceElementAt(p)->getBaseZ();
                // Keep all native footprint cells clear; construction may not clear scenery for this capture.
                for (uint8_t i = 0; i < ted.sequenceData.numSequences; ++i)
                {
                    const auto& b = ted.sequenceData.sequences[i].clearance;
                    const auto xy = p + CoordsXY{ b.x, b.y };
                    auto* s = MapGetSurfaceElementAt(xy);
                    if (!s || !s->getOwnership().has(OwnershipFlag::landOwned) || s->getWaterHeight() || s->getSlope()
                        || s->getBaseZ() != base)
                    {
                        clear = false;
                        break;
                    }
                    for (auto* e : TileElementsView(xy))
                        if (e->getType() != TileElementType::surface)
                            clear = false;
                }
                if (!clear || F::Action(p, false).error != GameActions::Status::ok)
                    continue;
                int score = 0;
                for (int dy = -160; dy <= 160; dy += 32)
                    for (int dx = -160; dx <= 160; dx += 32)
                        if (MapIsLocationValid(p + CoordsXY{ dx, dy }))
                            for (auto* e : TileElementsView(p + CoordsXY{ dx, dy }))
                                score += e->getType() == TileElementType::path ? 4
                                    : e->getType() == TileElementType::track   ? 1
                                                                               : 0;
                if (score > best)
                {
                    best = score;
                    tile = p;
                }
            }
        Check(best >= 0, "No native owned flat ride footprint");
        std::filesystem::create_directories(argv[4]);
        H::ObjectPicker picker;
        picker.setEntries(entries, H::PickerIdentity{ ObjectType::ride, selected });
        auto capture = [&](const char* name, CoordsXYZ focus, int mode, money64 cost = 0) {
            Viewport viewport{};
            viewport.width = 512;
            viewport.height = 384;
            const auto centre = Translate3DTo2DWithZ(0, focus);
            viewport.viewPos = { centre.x - 300, centre.y - 180 };
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &viewport);
            if (mode == 0)
                H::DrawObjectPicker(rt, { 512, 384 }, picker, "Flat rides");
            else if (mode == 5)
            {
                auto roster = H::RideManagement::Roster();
                auto target = F::Target();
                size_t row = 0;
                if (target)
                    for (size_t i = 0; i < roster.size(); ++i)
                        if (roster[i].id == target->id)
                            row = i;
                H::Dashboard::State dashboard;
                dashboard.section = H::Dashboard::Section::rides;
                dashboard.focus = H::Dashboard::Focus::content;
                H::Dashboard::Draw(
                    rt, { 512, 384 }, dashboard, H::RideManagement::Presentation(H::RideManagement::View::roster, row, roster),
                    true);
            }
            else if (mode < 4)
            {
                F::DrawPanel(rt, { 512, 384 });
                H::DrawCostBadge(rt, { 332, 272 }, H::FormatMoney(cost));
            }
            else
            {
                H::DrawGameplayFooter(rt, { 512, 384 }, "", false, false);
            }
            Image image;
            image.Width = 512;
            image.Height = 384;
            image.Depth = 8;
            image.Stride = rt.LineStride();
            image.Palette = Drawing::gPalette;
            auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(argv[4]) / name).string(), image, ImageFormat::png);
            engine.EndDraw();
        };
        const auto z = MapGetHighestZ(tile);
        const CoordsXYZ focus{ tile.x + 32, tile.y + 32, z };
        capture("flat-picker.png", focus, 0);
        const auto rideCost = F::Action(tile, false).cost;
        F::UpdatePreview(tile);
        capture("flat-ghost.png", focus, 1, rideCost);
        Check(state.park.cash == initialCash, "Preview spent cash");
        Check(F::Action(tile, true).error == GameActions::Status::ok, "Native flat ride placement failed");
        GameActions::ProcessQueue(state);
        Check(
            F::CurrentStage() == F::Stage::entrance && state.park.cash == initialCash - rideCost,
            "Flat ride cash/stage failed");
        auto target = F::Target();
        Check(target.has_value(), "New native ride identity missing");
        std::vector<CoordsXY> locations;
        for (int y = tile.y - 160; y <= tile.y + 160; y += 32)
            for (int x = tile.x - 160; x <= tile.x + 160; x += 32)
                if (F::EntranceLocation(CoordsXY{ x, y })
                    && F::Action(CoordsXY{ x, y }, false).error == GameActions::Status::ok)
                    locations.push_back({ x, y });
        std::sort(
            locations.begin(), locations.end(), [](const CoordsXY& a, const CoordsXY& b) { return a.x + a.y > b.x + b.y; });
        Check(locations.size() >= 2, "No valid native entrance/exit sides");
        auto indicate = [&](CoordsXY xy) {
            auto loc = *F::EntranceLocation(xy);
            gMapSelectPositionA = gMapSelectPositionB = xy;
            gMapSelectType = MapSelectType::full;
            gMapSelectArrowPosition = loc;
            gMapSelectArrowDirection = loc.direction;
            gMapSelectFlags.set(MapSelectFlag::enable, MapSelectFlag::enableArrow);
            MapSelection::invalidate();
        };
        indicate(locations[0]);
        const auto entranceCost = F::Action(locations[0], false).cost;
        capture("flat-entrance.png", focus, 2, entranceCost);
        Check(F::Action(locations[0], true).error == GameActions::Status::ok, "Entrance failed");
        GameActions::ProcessQueue(state);
        Check(F::CurrentStage() == F::Stage::exit, "Entrance handoff failed");
        indicate(locations[1]);
        const auto exitCost = F::Action(locations[1], false).cost;
        capture("flat-exit.png", focus, 3, exitCost);
        Check(F::Action(locations[1], true).error == GameActions::Status::ok, "Exit failed");
        GameActions::ProcessQueue(state);
        gMapSelectFlags.unset(MapSelectFlag::enable, MapSelectFlag::enableArrow);
        MapSelection::invalidate();
        Check(state.park.cash == initialCash - rideCost - entranceCost - exitCost, "Exact native construction cash failed");
        auto draft = H::RideManagement::Begin(*target, RideStatus::open);
        Check(
            draft && H::RideManagement::Action(*draft, true).error == GameActions::Status::ok,
            "Completed ride native opening failed");
        GameActions::ProcessQueue(state);
        Check(GetRide(target->id)->status == RideStatus::open, "Native ride did not open");
        capture("flat-completed.png", focus, 4);
        capture("flat-dashboard.png", focus, 5);
        const auto finalCash = state.park.cash;
        H::EndTerrain();
        SaveFlags flags{};
        flags.set(SaveFlag::exportObjects);
        const auto save = (std::filesystem::path(argv[5]) / "flat-capture.park").string();
        Check(
            ScenarioSave(state, save, flags) != 0 && context->LoadParkFromFile(save), "Native construction save/reload failed");
        auto* ride = GetRide(target->id);
        Check(ride && ride->status == RideStatus::open && state.park.cash == finalCash, "Saved flat ride/cash missing");
        Check(
            !ride->getStation(StationIndex::FromUnderlying(0)).entrance.isNull()
                && !ride->getStation(StationIndex::FromUnderlying(0)).exit.isNull(),
            "Saved entrance/exit missing");
        std::cout << entries.size() << " eligible flat rides; " << F::Name() << " placed at " << tile.x << "," << tile.y
                  << "; exact cash and entrance/exit native opening/save/reload passed; 6 captures at 512x384\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}

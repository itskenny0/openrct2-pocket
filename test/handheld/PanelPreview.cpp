// SPDX-License-Identifier: GPL-3.0-only
// Native 512x384 overlay capture; optional backdrop is an explicitly labelled layout composite.
#include <fstream>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/handheld/ObjectPicker.h>
#include <openrct2-ui/handheld/Panel.h>
#include <openrct2-ui/handheld/RideManagement.h>
#include <openrct2-ui/handheld/TextEntry.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/actions/track/TrackPlaceAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/Drawing.Sprite.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/G1Element.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/tile_element/SurfaceElement.h>

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;
int main(int argc, char** argv)
{
    if (argc != 4 && argc != 5)
        return 2;
    gCustomOpenRCT2DataPath = argv[1];
    auto context = CreateContext();
    Config::SetDefaults();
    if (std::string(argv[3]).starts_with("ride-depth"))
    {
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;
        if (!context->Initialise())
            return 7;
        gameStateInitAll(getGameState(), { 16, 16 });
        gOpenRCT2NoGraphics = false;
    }
    GfxLoadG2PalettesFontsTracks();
    initColourMaps();
    FontSpriteInitialiseCharacters();
    X8DrawingEngine engine(context->GetUiContext());
    engine.Resize(512, 384);
    engine.BeginDraw();
    auto& rt = *engine.getRT();
    GfxClear(rt, PaletteIndex::pi10);
    GamePalette palette{};
    const auto* pal = GfxGetG1Palette(SPR_GAME_DEFAULT_PALETTE);
    for (int i = 0; i < pal->numColours; i++)
        palette[pal->startIndex + i] = { pal->palette[i].blue, pal->palette[i].green, pal->palette[i].red, 255 };
    if (argc == 5)
    {
        std::ifstream background(argv[4], std::ios::binary);
        std::string magic;
        int width{}, height{}, maxValue{};
        background >> magic >> width >> height >> maxValue;
        background.get();
        if (magic != "P6" || width != 512 || height != 384 || maxValue != 255)
            return 3;
        for (int y = 0; y < 384; ++y)
            for (int x = 0; x < 512; ++x)
            {
                unsigned char rgb[3];
                if (!background.read(reinterpret_cast<char*>(rgb), 3))
                    return 3;
                int nearest = 10, distance = 3 * 256 * 256;
                for (int index = 10; index < 230; ++index)
                {
                    const auto& colour = palette[index];
                    const auto r = rgb[0] - colour.red, g = rgb[1] - colour.green, b = rgb[2] - colour.blue;
                    const auto candidate = r * r + g * g + b * b;
                    if (candidate < distance)
                    {
                        distance = candidate;
                        nearest = index;
                    }
                }
                rt.bits[y * rt.LineStride() + x] = static_cast<PaletteIndex>(nearest);
            }
    }
    Ui::Window w;
    w.windowPos = { 8, 37 };
    const auto size = Ui::Handheld::OverlaySize(7);
    w.width = size.width;
    w.height = size.height;
    Ui::Handheld::MenuPresentation model;
    model.title = "PARK / PAUSE";
    model.rows = { "Resume park", "Build", "Park dashboard", "Save park", "Save and exit", "Load park", "Exit without saving" };
    model.icons = { Ui::Handheld::PanelIcon::play, Ui::Handheld::PanelIcon::terrain, Ui::Handheld::PanelIcon::save,
                    Ui::Handheld::PanelIcon::exit, Ui::Handheld::PanelIcon::folder,  Ui::Handheld::PanelIcon::exit };
    model.selected = 1;
    namespace H = Ui::Handheld;
    const std::string page = argv[3];
    // Explicit component fixtures: no scenario, object unlock or guest simulation is represented.
    if (page == "exploration" || page.starts_with("paths") || page.starts_with("terrain") || page == "construction"
        || page == "hud-stress")
    {
        std::vector<PaletteIndex> before(rt.bits, rt.bits + rt.LineStride() * rt.height);
        H::DrawHud(
            rt, { 512, 384 },
            page == "hud-stress" ? H::HudPresentation{ "-$1,234,567.80", "31 Oct Y8192", "100000", true, true, false, 4 }
                                 : H::HudPresentation{ "$12,456.00", "15 Mar Y3", "812", false, false, false, 1 });
        for (int y = 0; y < rt.height; ++y)
            for (int x = 0; x < rt.width; ++x)
                if ((y < H::Layout::hudTop || y >= H::Layout::hudTop + H::Layout::hudHeight || x < H::Layout::margin
                     || x >= rt.width - H::Layout::margin)
                    && rt.bits[y * rt.LineStride() + x] != before[y * rt.LineStride() + x])
                    return 6; // HUD text/icons must not escape the reserved native rectangle.
    }
    if (std::string(argv[3]) == "terrain" || std::string(argv[3]) == "terrain-lower")
    {
        Ui::Handheld::DrawTerrainOverlay(rt, Ui::Handheld::Concept::kOrigin, std::string(argv[3]) != "terrain-lower");
        Ui::Handheld::DrawGameplayFooter(rt, { 512, 384 }, "RAISE LAND", true);
        Ui::Handheld::DrawCostBadge(rt, { 288, 224 }, "$20.00");
        Ui::Handheld::DrawMapFocus(rt, { 256, 192 });
    }
    else if (std::string(argv[3]).starts_with("paths"))
    {
        Ui::Handheld::DrawPathOverlay(
            rt, Ui::Handheld::Concept::kOrigin, std::string(argv[3]) == "paths-queue" ? "Blue queue" : "Tarmac",
            std::string(argv[3]) == "paths-queue", std::string(argv[3]) == "paths-remove",
            std::string(argv[3]) == "paths-ground" ? "Ground / auto slope" : "Height 6 / Rise NE");
        Ui::Handheld::DrawPathFooter(rt, { 512, 384 }, "PLACE PATH", std::string(argv[3]) == "paths-remove");
        Ui::Handheld::DrawCostBadge(rt, { 288, 224 }, "$12.00");
        if (std::string(argv[3]) == "paths-error")
            Ui::Handheld::DrawContextMessage(rt, { 288, 224 }, "Land not owned by park.");
        Ui::Handheld::DrawMapFocus(rt, { 256, 192 });
    }
    else if (std::string(argv[3]).starts_with("path-options"))
    {
        Ui::Handheld::DrawPathOptions(
            rt, { 512, 384 },
            { "Resume", "Follow ground", "Raise height", "Lower height", "Flat / slope", "Rotate slope", "Change surface" },
            std::string(argv[3]) == "path-options-last" ? 6 : 2, "Height 6 / Rise NE");
    }
    else if (page == "exploration" || page == "hud-stress")
    {
        H::DrawGameplayFooter(rt, { 512, 384 }, "", false);
        H::DrawMapFocus(rt, { 256, 192 });
    }
    else if (page == "construction")
    {
        H::CustomTracks::DrawPanel(rt, { 512, 384 });
        H::DrawMapFocus(rt, { 256, 192 });
    }
    else if (page == "picker")
    {
        H::ObjectPicker picker;
        std::vector<H::PickerEntry> entries;
        const std::vector<std::pair<TrackElemType, std::string>> pieces{ { TrackElemType::flat, "Straight" },
                                                                         { TrackElemType::endStation, "Station" },
                                                                         { TrackElemType::leftQuarterTurn5Tiles, "Left turn" },
                                                                         { TrackElemType::rightQuarterTurn5Tiles,
                                                                           "Right turn" },
                                                                         { TrackElemType::flatToUp25, "Level to climb" },
                                                                         { TrackElemType::up25, "Climb" },
                                                                         { TrackElemType::up25ToFlat, "Climb to level" } };
        for (const auto& [type, name] : pieces)
            entries.push_back(
                { { ObjectType::ride, kObjectEntryIndexNull, name }, name, ZoomLevel{ 0 }, [type](RenderTarget& target) {
                     H::CustomTracks::DrawPiecePreview(target, type);
                 } });
        picker.setEntries(std::move(entries));
        H::DrawObjectPicker(rt, { 512, 384 }, picker, "Track pieces");
    }
    else if (page.starts_with("ride-depth"))
    {
        namespace R = H::RideManagement;
        // Native public object metadata, trackless component fixture; no live park or simulated guests.
        gOpenRCT2NoGraphics = true;
        ride_type_t fixtureType = RIDE_TYPE_WOODEN_ROLLER_COASTER;
        const char* fixtureObject = "rct2.ride.ptct1";
        if (page == "ride-depth-mode-duration" || page == "ride-depth-mode-draft" || page == "ride-depth-mode-confirm")
        {
            fixtureType = RIDE_TYPE_DODGEMS;
            fixtureObject = "rct2.ride.dodg1";
        }
        else if (page == "ride-depth-mode-rotations")
        {
            fixtureType = RIDE_TYPE_MERRY_GO_ROUND;
            fixtureObject = "rct2.ride.mgr1";
        }
        else if (page == "ride-depth-mode-laps")
        {
            fixtureType = RIDE_TYPE_GO_KARTS;
            fixtureObject = "rct2.ride.kart1";
        }
        else if (page == "ride-depth-mode-launch")
        {
            fixtureType = RIDE_TYPE_CORKSCREW_ROLLER_COASTER;
            fixtureObject = "rct2.ride.rckc";
        }
        auto* object = context->GetObjectManager().LoadObject(fixtureObject);
        if (!object)
            return 8;
        gOpenRCT2NoGraphics = false;
        const auto objectIndex = context->GetObjectManager().GetLoadedObjectEntryIndex(object);
        GameActions::RideCreateAction create(
            fixtureType, objectIndex, 0, 0, kObjectEntryIndexNull, RideInspection::every30Minutes);
        const auto created = GameActions::Execute(&create, getGameState());
        if (created.error != GameActions::Status::ok)
            return 9;
        GameActions::ProcessQueue(getGameState());
        auto* ride = GetRide(created.getData<RideId>());
        if (!ride)
            return 9;
        ride->customName = page.starts_with("ride-depth-mode") ? ride->getName() + " / component fixture"
                                                               : "Wooden Coaster / component fixture";
        if (page == "ride-depth-mode-launch")
        {
            GameActions::RideSetSettingAction launch(
                ride->id, GameActions::RideSetSetting::mode, static_cast<uint8_t>(RideMode::poweredLaunch));
            if (GameActions::Execute(&launch, getGameState()).error != GameActions::Status::ok)
                return 12;
            GameActions::ProcessQueue(getGameState());
        }
        ride->status = RideStatus::closed;
        ride->inspectionInterval = RideInspection::every30Minutes;
        ride->liftHillSpeed = ride->getRideTypeDescriptor().LiftData.minimum_speed;
        if (page.starts_with("ride-depth-vehicle"))
        {
            // Real native station piece in an owned component map; not a complete operating coaster.
            auto* surface = MapGetSurfaceElementAt(CoordsXY{ 256, 256 });
            surface->setOwnership({ OwnershipFlag::landOwned });
            GameActions::TrackPlaceAction station(
                ride->id, TrackElemType::endStation, ride->type, CoordsXYZD{ 256, 256, surface->getBaseZ(), 0 }, 0, 0, 0, {},
                false);
            if (GameActions::Execute(&station, getGameState()).error != GameActions::Status::ok)
                return 10;
            GameActions::ProcessQueue(getGameState());
        }
        const auto target = R::Identify(ride->id);
        std::optional<R::Draft> draft;
        auto view = page == "ride-depth-summary"     ? R::View::inspection
            : page == "ride-depth-maintenance"       ? R::View::maintenance
            : page.starts_with("ride-depth-vehicle") ? R::View::vehicles
            : page == "ride-depth-appearance"        ? R::View::appearance
            : page == "ride-depth-music"             ? R::View::music
            : page == "ride-depth-customers"         ? R::View::customers
            : page.starts_with("ride-depth-graph")   ? R::View::graphs
            : page == "ride-depth-stations"          ? R::View::stations
            : page == "ride-depth-finances"          ? R::View::earnings
            : page == "ride-depth-dispatch"          ? R::View::dispatch
                                                     : R::View::operations;
        size_t selected = page == "ride-depth-dense" ? R::Settings(*ride).size() - 1 : 0;
        if (page == "ride-depth-demolition")
        {
            view = R::View::confirmation;
            draft = R::BeginDemolition(*target);
            selected = 1;
        }
        if (page == "ride-depth-draft")
        {
            view = R::View::price;
            draft = R::BeginSetting(*target, R::Settings(*ride).front());
            R::AdjustPrice(*draft, 1);
        }
        if (page == "ride-depth-vehicle-draft" || page == "ride-depth-vehicle-confirm")
        {
            const auto settings = R::VehicleSettings(*ride);
            if (settings.empty())
                return 11;
            draft = R::BeginSetting(*target, settings.back());
            R::AdjustPrice(*draft, -1);
            view = page == "ride-depth-vehicle-draft" ? R::View::price : R::View::confirmation;
            selected = view == R::View::confirmation ? 1 : 0;
        }
        if (page.starts_with("ride-depth-mode"))
        {
            const auto settings = R::Settings(*ride);
            const auto key = page == "ride-depth-mode-circuits" ? GameActions::RideSetSetting::numCircuits
                                                                : GameActions::RideSetSetting::operation;
            const auto found = std::find_if(settings.begin(), settings.end(), [key](const auto& s) { return s.key == key; });
            if (found == settings.end())
                return 13;
            selected = found - settings.begin();
            if (page == "ride-depth-mode-draft" || page == "ride-depth-mode-confirm")
            {
                draft = R::BeginSetting(*target, *found);
                R::AdjustPrice(*draft, 1);
                view = page == "ride-depth-mode-draft" ? R::View::price : R::View::confirmation;
                selected = view == R::View::confirmation ? 1 : 0;
            }
        }
        if (page == "ride-depth-graph-fixture")
        {
            // Explicitly synthetic render fixture for plotting bounds, not native ride telemetry.
            ride->measurement = std::make_unique<RideMeasurement>();
            ride->measurement->flags.set(RideMeasurementFlag::running);
            ride->measurement->num_items = 240;
            for (size_t i = 0; i < 240; ++i)
                ride->measurement->velocity[i] = i < 120 ? i : 240 - i;
            ride->customName = "Synthetic graph rendering fixture";
        }
        auto content = R::Presentation(view, selected, {}, target, draft);
        if (page == "ride-depth-keyboard")
        {
            H::TextEntry keyboard{ "Pocket Coaster" };
            content.breadcrumb = "Name / Keyboard";
            content.title = keyboard.text;
            content.stats.clear();
            content.rows = keyboard.Keys();
            content.columns = H::TextEntry::columns;
            content.selected = 46;
            content.status = "14 / 32 characters";
        }
        H::Dashboard::State dashboard;
        dashboard.section = H::Dashboard::Section::rides;
        dashboard.focus = H::Dashboard::Focus::content;
        H::Dashboard::Draw(
            rt, { 512, 384 }, dashboard, content,
            page != "ride-depth-keyboard" && view != R::View::confirmation && view != R::View::price);
    }
    else if (page == "track-paint")
    {
        H::DrawTrackColourOverlay(rt, H::Concept::kOrigin, 2, true);
        H::DrawTrackColourFooter(rt, { 512, 384 });
    }
    else if (page == "dashboard" || page == "inspection" || page == "confirmation")
    {
        H::Dashboard::State dashboard;
        dashboard.section = page == "dashboard" ? H::Dashboard::Section::finances : H::Dashboard::Section::rides;
        dashboard.focus = H::Dashboard::Focus::content;
        H::ManagementPresentation content;
        content.breadcrumb = "Park dashboard / " + std::string(page == "dashboard" ? "Finances" : "Rides");
        content.title = page == "dashboard" ? "Finances" : "The incredibly long wooden roller coaster name";
        if (page == "dashboard")
        {
            content.stats = { { "Cash", "$12,456.00" },
                              { "Bank loan", "$10,000.00" },
                              { "Income", "$5,432.00" },
                              { "Expenditure", "$971.30" },
                              { "Current profit", "$3,210.00" } };
            content.rows = { "Loan...", "Admission...", "Back to dashboard" };
        }
        else if (page == "inspection")
        {
            content.stats = { { "Status", "Closed" }, { "Admission", "$2.00" } };
            content.rows = { "Locate",
                             "Statistics...",
                             "Open...",
                             "Close...",
                             "Test...",
                             "Admission price...",
                             "Back to roster",
                             "Finish entrances/exits...",
                             "Access / paths...",
                             "Continue track..." };
            content.selected = 8;
        }
        else
        {
            content.confirmation = true;
            content.title = "Review ride opening";
            content.stats = { { "Current status", "Closed" }, { "Requested", "Open" }, { "Action cost", "$0.00" } };
            content.rows = { "Confirm", "Cancel" };
            content.selected = 1;
            content.primaryAction = "Cancel";
        }
        H::Dashboard::Draw(rt, { 512, 384 }, dashboard, content, page != "confirmation");
    }
    else if (std::string(argv[3]) == "finances")
    {
        w.width = 330;
        w.height = 310;
        w.windowPos = { 8, 37 };
        model.title = "FINANCES";
        model.detailTitle = "Evergreen Gardens";
        model.stats = { { "Cash", "$12,456.00" },
                        { "Bank loan", "$10,000.00" },
                        { "Current profit", "$3,210.00" },
                        { "Expenditure", "$971.30" },
                        { "Park value", "$98,724.00" } };
        model.rows = { "Back to dashboard" };
        model.selected = 0;
        Ui::Handheld::DrawInformationOverlay(rt, w, model);
    }
    else if (std::string(argv[3]) == "build")
    {
        model.title = "BUILD / TERRAIN";
        model.rows = { "Raise land", "Lower land", "Paths", "Back to park" };
        model.icons = { Ui::Handheld::PanelIcon::raise, Ui::Handheld::PanelIcon::lower, Ui::Handheld::PanelIcon::back };
        model.selected = 0;
        model.status = "Choose a tool, then edit in the park.";
        const auto buildSize = Ui::Handheld::OverlaySize(4);
        w.height = buildSize.height;
        w.windowPos.y = (384 - w.height) / 2;
        Ui::Handheld::DrawOverlayShell(rt, w, model);
    }
    else
        Ui::Handheld::DrawOverlayShell(rt, w, model);
    std::ofstream out(argv[2], std::ios::binary);
    out << "P6\n512 384\n255\n";
    for (int y = 0; y < 384; y++)
        for (int x = 0; x < 512; x++)
        {
            auto c = palette[static_cast<unsigned>(rt.bits[y * rt.LineStride() + x])];
            char rgb[] = { static_cast<char>(c.red), static_cast<char>(c.green), static_cast<char>(c.blue) };
            out.write(rgb, 3);
        }
}

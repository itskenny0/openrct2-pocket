/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Explorer.h"

#include "Alerts.h"
#include "AreaTools.h"
#include "ConstructionAdjust.h"
#include "ConstructionPanel.h"
#include "ControllerInput.h"
#include "Currency.h"
#include "CustomTracks.h"
#include "DesignAuthoring.h"
#include "Designs.h"
#include "FlatRides.h"
#include "GuestFollow.h"
#include "Hud.h"
#include "Landscape.h"
#include "Mazes.h"
#include "MenuThemes.h"
#include "Panel.h"
#include "Paths.h"
#include "RideAccess.h"
#include "RideFollow.h"
#include "RightsTools.h"
#include "SceneryClear.h"
#include "SceneryRemoval.h"
#include "SceneryTools.h"
#include "ShopManagement.h"
#include "Shops.h"
#include "StaffPatrol.h"
#include "Surface.h"
#include "Trees.h"
#include "Undo.h"
#include "Water.h"

#include <SDL_timer.h>
#include <bit>
#include <iomanip>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/input/MouseInput.h>
#include <openrct2-ui/input/ShortcutIds.h>
#include <openrct2-ui/input/ShortcutManager.h>
#include <openrct2-ui/interface/Chat.h>
#include <openrct2-ui/interface/InGameConsole.h>
#include <openrct2-ui/interface/ViewportInteraction.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/terraform/LandLowerAction.h>
#include <openrct2/actions/terraform/LandRaiseAction.h>
#include <openrct2/audio/Audio.h>
#include <openrct2/core/Path.hpp>
#include <openrct2/drawing/Line.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/Rectangle.h>
#include <openrct2/drawing/Text.h>
#include <openrct2/entity/Peep.h>
#include <openrct2/interface/Screenshot.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/ride/Vehicle.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/tile_element/EntranceElement.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <sstream>

namespace OpenRCT2::Ui::Handheld
{
    // The Pixel theme's bitmap font reads glyphs from g1.dat through this hook.
    static const bool kPixelFontReady
        = (PixelFont::glyphSource = [](uint32_t sprite) { return GfxGetG1Element(sprite); }, true);
    static ConstructionAdjust::State _constructionAdjust;
    static bool _terrainActive{};
    static bool _waterTool{};
    static bool _surfaceTool{};
    static bool _treeTool{};
    static bool _shopTool{};
    static ObjectEntryIndex _shopIndex = kObjectEntryIndexNull;
    static Direction _shopDirection{};
    static ObjectEntryIndex _treeIndex = kObjectEntryIndexNull;
    static bool _sceneryRemovalTool{};
    static bool _sceneryTool{};
    static bool _sceneryClearTool{};
    static bool _areaTool{};
    static bool _rightsTool{};
    static std::optional<SceneryTools::Target> _sceneryEdit;
    static std::optional<RideManagement::Target> _colourRide;
    static std::optional<RideManagement::PaintTarget> _colourTarget;
    static uint8_t _colourScheme{};
    static std::optional<SceneryRemoval::Target> _sceneryTarget;
    static ObjectEntryIndex _surfaceIndex = kObjectEntryIndexNull;
    static bool _raiseLand = true;
    static bool _selectionOwned{};
    static std::optional<CoordsXY> _terrainTile;
    static std::string _terrainStatus;
    static bool _scatterFeedback{};
    static std::optional<RideManagement::Target> _ridePathAccess;
    static StationIndex _accessStation{};
    static bool _accessExit{};
    static CoordsXYZD _accessOutside{};

    static bool SimpleTerrain()
    {
        return _terrainActive && !_areaTool && !_surfaceTool && !_treeTool && !_shopTool && !_sceneryRemovalTool
            && !_sceneryTool && !_sceneryClearTool && !_rightsTool && !_colourRide && !PeepPlacement::Active()
            && !StaffPatrol::Active() && !Designs::Active() && !CustomTracks::Active() && !Mazes::Active()
            && FlatRides::CurrentStage() == FlatRides::Stage::inactive;
    }

    static std::optional<RideManagement::Target> ConstructionRide()
    {
        if (CustomTracks::Active())
            return CustomTracks::Target();
        if (Mazes::Active())
            return Mazes::Target();
        if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
            return FlatRides::Target();
        if (Paths::IsActive() && _ridePathAccess)
            return _ridePathAccess;
        return {};
    }
    static void ReturnFromRideTool(const std::optional<RideManagement::Target>& target)
    {
        const auto context = RideNavigation::Take();
        EndTerrain();
        if (target && RideManagement::Resolve(*target))
            Windows::HandheldRideReturn(*target, context);
    }

    static bool AccessTargetValid()
    {
        return _ridePathAccess && RideAccess::PointValid(*_ridePathAccess, _accessStation, _accessExit, _accessOutside);
    }

    void SetRidePathAccess(const RideManagement::Target& target, StationIndex station, bool exit, CoordsXYZD outside)
    {
        _ridePathAccess = target;
        _accessStation = station;
        _accessExit = exit;
        _accessOutside = outside;
    }

    void BeginTerrain(bool raise)
    {
        RideNavigation::construction.reset();
        _constructionAdjust.reset();
        PeepPlacement::End();
        StaffPatrol::End();
        _ridePathAccess.reset();
        Paths::End();
        Trees::ClearPreview();
        SceneryTools::ClearPreview();
        Shops::ClearPreview();
        FlatRides::End();
        Designs::End();
        DesignAuthoring::End();
        CustomTracks::End();
        Mazes::End();
        _terrainActive = true;
        _waterTool = false;
        _surfaceTool = false;
        _treeTool = false;
        _shopTool = false;
        _shopIndex = kObjectEntryIndexNull;
        _treeIndex = kObjectEntryIndexNull;
        _colourRide.reset();
        _colourTarget.reset();
        _sceneryRemovalTool = false;
        _sceneryTool = false;
        _sceneryClearTool = false;
        _areaTool = false;
        _rightsTool = false;
        _sceneryEdit.reset();
        _sceneryTarget.reset();
        _surfaceIndex = kObjectEntryIndexNull;
        _raiseLand = raise;
        Landscape::shape = raise ? Landscape::Shape::hill : Landscape::Shape::dig;
        _terrainStatus.clear();
        _scatterFeedback = false;
    }

    // Water is a pond you dig, not a column you raise: the quick tool fills what it digs, so the
    // water always sits inside its banks. Raw water levels remain in Build > Landscape.
    void BeginWater(bool raise)
    {
        BeginTerrain(raise);
        Landscape::shape = Landscape::Shape::pond;
    }

    void BeginSurface()
    {
        BeginTerrain(true);
        _surfaceTool = true;
        _surfaceIndex = Surface::Next(kObjectEntryIndexNull);
    }

    void BeginRightsTools()
    {
        BeginTerrain(true);
        _rightsTool = true;
    }
    bool BeginPeepPlacement(const PeepPlacement::Target& person)
    {
        BeginTerrain(true);
        const auto result = PeepPlacement::Begin(person);
        if (result.error == GameActions::Status::ok)
            return true;
        EndTerrain();
        return false;
    }
    bool BeginStaffPatrol(const StaffManagement::Target& employee)
    {
        BeginTerrain(true);
        if (StaffPatrol::Begin(employee))
            return true;
        EndTerrain();
        return false;
    }
    void BeginAreaTools()
    {
        BeginTerrain(true);
        _areaTool = true;
    }
    void BeginSceneryClear()
    {
        BeginTerrain(true);
        _sceneryClearTool = true;
    }
    void BeginScenery()
    {
        BeginTerrain(true);
        _sceneryTool = true;
    }
    bool IsSceneryToolActive()
    {
        return _terrainActive && _sceneryTool;
    }

    std::optional<SceneryTools::Target> SceneryEditTarget()
    {
        return _sceneryEdit;
    }
    void BeginTrees()
    {
        BeginTerrain(true);
        _treeTool = true;
        _treeIndex = Trees::Next(kObjectEntryIndexNull);
    }

    bool BeginMaze(ObjectEntryIndex index)
    {
        if (!Mazes::Eligible(index))
            return false;
        BeginTerrain(true);
        return Mazes::Begin(index);
    }
    bool ResumeMaze(RideId id)
    {
        const auto target = RideManagement::Identify(id);
        if (!target || !Mazes::CanResume(*target))
            return false;
        BeginTerrain(true);
        return Mazes::Resume(*target);
    }
    bool BeginCustomTrack(ObjectEntryIndex index, ride_type_t type)
    {
        if (!CustomTracks::Eligible(index, type))
            return false;
        BeginTerrain(true);
        return CustomTracks::Begin(index, type);
    }
    bool ResumeCustomTrack(RideId id)
    {
        auto target = RideManagement::Identify(id);
        if (!target || !CustomTracks::CanResume(*target))
            return false;
        BeginTerrain(true);
        return CustomTracks::Resume(*target);
    }
    bool ResumeCustomTrackFrom(const RideManagement::Target& target, CoordsXYZD origin, TrackElemType type, bool backward)
    {
        if (!RideManagement::Resolve(target))
            return false;
        BeginTerrain(true);
        return CustomTracks::ResumeFrom(target, std::pair{ origin, type }, backward);
    }
    bool ResumeCustomTrackReplacement(
        const RideManagement::Target& target, const TrackConstructionSupport::BlockSnapshot& removed, uint8_t colour)
    {
        BeginTerrain(true);
        return CustomTracks::ResumeReplacement(target, removed, colour);
    }
    bool BeginFlatRide(ObjectEntryIndex index)
    {
        if (!FlatRides::Eligible(index))
            return false;
        BeginTerrain(true);
        return FlatRides::Begin(index);
    }
    bool BeginSavedDesign(const std::string& path, std::optional<ObjectEntryDescriptor> vehicle)
    {
        auto td = TrackDesignImport(path.c_str());
        if (td && vehicle)
            td->trackAndVehicle.vehicleObject = *vehicle;
        if (td && !vehicle && Designs::Active() && Designs::Selection() == path && Designs::Draft())
            td->trackAndVehicle.vehicleObject = Designs::Draft()->trackAndVehicle.vehicleObject;
        if (!td || !Designs::Eligible(*td))
            return false;
        if (Designs::Active() && Designs::Selection() == path)
            return Designs::Begin(path, vehicle);
        BeginTerrain(true);
        return Designs::Begin(path, vehicle);
    }
    bool ResumeFlatRide(RideId id)
    {
        auto target = RideManagement::Identify(id);
        if (!target || !FlatRides::CanResume(*target))
            return false;
        BeginTerrain(true);
        return FlatRides::Resume(*target);
    }
    bool IsShopToolActive()
    {
        return _shopTool;
    }
    ObjectEntryIndex ShopSelection()
    {
        return _shopIndex;
    }
    Direction ShopDirection()
    {
        return _shopDirection;
    }
    bool SetShopSelection(ObjectEntryIndex index)
    {
        if (!Shops::Eligible(index))
            return false;
        if (!_shopTool)
        {
            BeginTerrain(true);
            _shopTool = true;
            _shopDirection = 0;
        }
        Shops::ClearPreview();
        FlatRides::ClearPreview();
        Designs::ClearPreview();
        _shopIndex = index;
        _terrainStatus.clear();
        _scatterFeedback = false;
        return true;
    }
    void RotateShop()
    {
        if (!_shopTool)
            return;
        Shops::ClearPreview();
        FlatRides::ClearPreview();
        Designs::ClearPreview();
        _shopDirection = (_shopDirection + 1) & 3;
        _terrainStatus.clear();
        _scatterFeedback = false;
    }

    bool IsTreeToolActive()
    {
        return _treeTool;
    }
    ObjectEntryIndex TreeSelection()
    {
        return _treeIndex;
    }
    bool SetTreeSelection(ObjectEntryIndex index)
    {
        if (!_treeTool || !Trees::Eligible(index))
            return false;
        Trees::ClearPreview();
        SceneryTools::ClearPreview();
        Shops::ClearPreview();
        FlatRides::ClearPreview();
        Designs::ClearPreview();
        _treeIndex = index;
        _terrainStatus.clear();
        _scatterFeedback = false;
        return true;
    }

    void BeginTrackColour(const RideManagement::Target& ride)
    {
        BeginTerrain(true);
        _colourRide = ride;
        _colourScheme = 0;
    }
    void BeginSceneryRemoval()
    {
        BeginTerrain(true);
        _sceneryRemovalTool = true;
    }

    static void ClearTerrainSelection()
    {
        if (_selectionOwned)
        {
            MapSelection::invalidate();
            gMapSelectFlags.unset(MapSelectFlag::enable);
            gMapSelectFlags.unset(MapSelectFlag::enableArrow);
            MapSelection::invalidate();
            _selectionOwned = false;
        }
        _terrainTile.reset();
        _sceneryTarget.reset();
    }

    std::optional<CoordsXY> CurrentTerrainTile()
    {
        return _terrainTile;
    }
    void EndTerrain()
    {
        Undo::Clear(); // Undo belongs to the tool session it came from.
        RideNavigation::construction.reset();
        _constructionAdjust.reset();
        PeepPlacement::End();
        StaffPatrol::End();
        _ridePathAccess.reset();
        Trees::ClearPreview();
        SceneryTools::ClearPreview();
        Shops::ClearPreview();
        FlatRides::End();
        Designs::End();
        CustomTracks::End();
        Mazes::End();
        _terrainActive = false;
        _waterTool = false;
        _surfaceTool = false;
        _treeTool = false;
        _shopTool = false;
        _shopIndex = kObjectEntryIndexNull;
        _treeIndex = kObjectEntryIndexNull;
        _colourRide.reset();
        _colourTarget.reset();
        _sceneryRemovalTool = false;
        _sceneryTool = false;
        _sceneryClearTool = false;
        _areaTool = false;
        _rightsTool = false;
        _sceneryEdit.reset();
        _sceneryTarget.reset();
        _surfaceIndex = kObjectEntryIndexNull;
        Paths::End();
        _terrainStatus.clear();
        _scatterFeedback = false;
        ClearTerrainSelection();
    }

    static ScreenCoordsXY Centre();
    static GameActions::Result TerrainAction(bool execute)
    {
        if (CustomTracks::Active())
            return CustomTracks::Action(execute);
        if (Designs::Active())
            return Designs::Query(_terrainTile);
        if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
            return FlatRides::Action(_terrainTile, execute);
        if (PeepPlacement::Active())
        {
            if (!_terrainTile)
                return PeepPlacement::Place({}, execute);
            const auto info = GetMapCoordinatesFromPos(
                Centre(), { ViewportInteractionItem::footpath, ViewportInteractionItem::terrain });
            return PeepPlacement::Place(
                info.Element ? std::optional<CoordsXYZ>{ CoordsXYZ{ info.Loc, info.Element->getBaseZ() } } : std::nullopt,
                execute);
        }
        if (StaffPatrol::Active())
            return StaffPatrol::Action(_terrainTile, execute);
        if (_rightsTool)
            return RightsTools::Query(_terrainTile, RightsTools::settings);
        if (_areaTool)
            return AreaTools::Action(_terrainTile, AreaTools::settings, execute);
        if (_sceneryClearTool)
            return SceneryClear::Query(_terrainTile, SceneryClear::settings);
        if (_sceneryTool)
        {
            const auto& settings = SceneryTools::Current();
            if (settings.mode == SceneryTools::Mode::place)
                return SceneryTools::Place(_terrainTile, settings, execute);
            if (_sceneryEdit)
            {
                if (settings.mode == SceneryTools::Mode::pick)
                    return SceneryTools::Pick(*_sceneryEdit, execute);
                return SceneryTools::Modify(*_sceneryEdit, settings.mode, settings.colours, execute);
            }
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = "Point at scenery";
            return result;
        }
        if (_colourRide)
        {
            if (_colourTarget)
                return RideManagement::Paint(*_colourTarget, _colourScheme, execute);
            GameActions::Result result;
            result.error = GameActions::Status::disallowed;
            result.errorMessage = "Point at track belonging to this ride";
            return result;
        }
        if (_sceneryRemovalTool)
            return SceneryRemoval::Action(_sceneryTarget, execute);
        if (_shopTool)
            return Shops::Action(_terrainTile, _shopIndex, _shopDirection, execute);
        if (_treeTool)
            return Trees::Action(_terrainTile, _treeIndex, execute);
        if (_surfaceTool)
            return Surface::Action(_terrainTile, _surfaceIndex, execute);
        if (_waterTool)
            return Water::Action(_terrainTile, _raiseLand, execute);
        return Landscape::Action(_terrainTile, execute);
    }

    // Before A changes the park, remember enough to put it back (hold B). Land shaping covers a
    // generous square, since smoothing reaches past the brush; placements cover the brush or the
    // object's footprint. Painting, removal, colours and ride building have their own flows.
    static bool PrepareUndo()
    {
        if (!_terrainTile || CustomTracks::Active() || Designs::Active()
            || FlatRides::CurrentStage() != FlatRides::Stage::inactive || PeepPlacement::Active() || StaffPatrol::Active()
            || _rightsTool || _sceneryClearTool || _colourRide || _sceneryRemovalTool || _surfaceTool)
            return false;
        const auto tile = *_terrainTile;
        if (_sceneryTool)
        {
            if (SceneryTools::Current().mode != SceneryTools::Mode::place)
                return false;
            Undo::Prepare(Undo::Kind::scenery, tile, 5, "scenery");
        }
        else if (_treeTool)
            Undo::Prepare(Undo::Kind::trees, tile, 2, "tree");
        else if (_shopTool)
            Undo::Prepare(Undo::Kind::shop, tile, 1, "shop");
        else if (_areaTool && AreaTools::settings.operation == AreaTools::Operation::paint)
            return false;
        else
            Undo::Prepare(Undo::Kind::land, tile, 8, _waterTool ? "water change" : "land change");
        return true;
    }

    static bool HasTextOrModalInput()
    {
        auto* wm = GetWindowManager();
        return gChatOpen || GetInGameConsole().IsOpen() || Windows::IsUsingWidgetTextBox()
            || wm->FindByClass(WindowClass::textinput) != nullptr || wm->FindByClass(WindowClass::loadsave) != nullptr
            || wm->FindByClass(WindowClass::loadsaveOverwritePrompt) != nullptr
            || wm->FindByClass(WindowClass::savePrompt) != nullptr || GetShortcutManager().isPendingShortcutChange();
    }

    static bool CanExplore()
    {
        if (gLegacyScene != LegacyScene::playing || WindowGetMain() == nullptr || HasTextOrModalInput()
            || gInputFlags.has(InputFlag::toolActive) || InputGetState() != InputState::normal
            || GetWindowManager()->FindByClass(WindowClass::error) != nullptr
            || GetWindowManager()->FindByClass(WindowClass::progressWindow) != nullptr
            || GetWindowManager()->FindByClass(WindowClass::dropdown) != nullptr)
            return false;
        for (const auto& window : gWindowList)
        {
            if (!window->flags.hasAny(WindowFlag::dead, WindowFlag::stickToBack, WindowFlag::stickToFront))
                return false;
        }
        return true;
    }

    static ScreenCoordsXY Centre()
    {
        return { ContextGetWidth() / 2, ContextGetHeight() / 2 };
    }

    static void Invoke(std::string_view id)
    {
        if (auto* shortcut = GetShortcutManager().getShortcut(id); shortcut != nullptr)
            shortcut->action();
    }

    // What A would inspect at the fixed cursor, refreshed each update so the footer verb, the
    // cursor colour and the name chip preview the press instead of leaving it to guesswork.
    enum class InspectKind : uint8_t
    {
        none,
        ride,
        shop,
        guest,
        staff,
        park,
        path, // A extends it with the connected path tool.
        sign, // A edits its text.
    };
    // The footer verb is one word (the chip beside the cursor names the target), so the park's
    // footer stays on one row.
    struct InspectTarget
    {
        std::string verb = "Inspect";
        std::string name;
        InspectKind kind = InspectKind::none;
    };
    static InspectTarget _inspect;
    static InspectKind _drawnInspectKind = InspectKind::none;

    static Drawing::PaletteIndex InspectColour(InspectKind kind)
    {
        switch (kind)
        {
            case InspectKind::ride:
                return NearestPaletteIndex(70, 200, 110);
            case InspectKind::shop:
                return NearestPaletteIndex(245, 150, 40);
            case InspectKind::guest:
                return NearestPaletteIndex(250, 215, 60);
            case InspectKind::staff:
                return NearestPaletteIndex(90, 160, 245);
            case InspectKind::park:
                return NearestPaletteIndex(225, 110, 220);
            case InspectKind::path:
                return NearestPaletteIndex(170, 200, 230);
            case InspectKind::sign:
                return NearestPaletteIndex(200, 170, 255);
            default:
                return NearestPaletteIndex(240, 240, 240);
        }
    }
    static constexpr int kCursorWindow = 41;
    // Brackets breathe 0-3 px on a ~0.8 s cycle; still with reduced motion.
    static int BracketPulse()
    {
        if (MenuFeel::reduceMotion)
            return 1;
        static constexpr int kSteps[] = { 0, 1, 2, 3, 2, 1 };
        return kSteps[(MenuFeel::Clock() / 140) % 6];
    }
    static ScreenSize InspectChipSize()
    {
        return { std::min(ContextGetWidth() / 2, TextWidth(_inspect.name, true) + 24), 20 };
    }
    static ScreenCoordsXY InspectChipOrigin()
    {
        const auto size = InspectChipSize();
        auto origin = Centre() + ScreenCoordsXY{ 16, 12 };
        origin.x = std::min(origin.x, ContextGetWidth() - size.width - Layout::margin);
        return origin;
    }

    struct Toast
    {
        std::string title, message;
        uint64_t shown{};
        bool notice{}; // An alert the player asked to see (Y), not an error: calmer colour and sound.
    };
    static Toast _toast;
    static constexpr uint64_t kToastMs = 2600, kToastSlideMs = 150;
    static bool ToastVisible(uint64_t now = MenuFeel::Clock())
    {
        return _toast.shown != 0 && now - _toast.shown < kToastMs;
    }
    static ScreenSize ToastSize()
    {
        const int text = std::max(TextWidth(_toast.title, true), TextWidth(_toast.message));
        return { std::min(ContextGetWidth() - 2 * Layout::margin, text + 34), _toast.message.empty() ? 24 : 40 };
    }
    static ScreenCoordsXY ToastOrigin()
    {
        const auto size = ToastSize();
        const int y = Layout::hudTop + Layout::hudHeight + 6;
        const int width = ContextGetWidth();
        // Tool panels dock on the left under the HUD. Centre the toast in the space to their right
        // so it never covers the panel the player is working in.
        int left = 0;
        for (const auto& window : gWindowList)
        {
            if (window->flags.has(WindowFlag::dead) || window->classification == WindowClass::mainWindow
                || (window->classification == WindowClass::handheldHud
                    && (window->number == 5 || window->number == 3 || window->number == 0 || window->number >= 6))
                || window->classification == WindowClass::handheldFocus)
                continue;
            const bool docked = window->windowPos.x <= Layout::margin + 2;
            const bool overlaps = window->windowPos.y < y + size.height && window->windowPos.y + window->height > y;
            if (docked && overlaps)
                left = std::max(left, window->windowPos.x + window->width + Layout::gutter);
        }
        const int room = width - Layout::margin - left;
        const int x = left == 0  ? (width - size.width) / 2
            : room >= size.width ? left + (room - size.width) / 2
                                 : std::max(Layout::margin, width - Layout::margin - size.width);
        return { x, y };
    }
    // Slides down from under the HUD, then back up before it disappears.
    static int ToastOffset(uint64_t now = MenuFeel::Clock())
    {
        if (MenuFeel::reduceMotion || !ToastVisible(now))
            return 0;
        const auto age = now - _toast.shown;
        const auto height = ToastSize().height + 8;
        if (age < kToastSlideMs)
            return -static_cast<int>(height * (kToastSlideMs - age) / kToastSlideMs);
        if (age > kToastMs - kToastSlideMs)
            return -static_cast<int>(height * (age - (kToastMs - kToastSlideMs)) / kToastSlideMs);
        return 0;
    }

    class FocusWindow final : public Window
    {
    public:
        void onDraw(Drawing::RenderTarget& rt) override
        {
            DrawExplorer(rt);
        }
    };

    static QuickWheel::State _wheel;
    struct Photo
    {
        bool active{};
        bool pending{};    // A pressed: waiting for a clean frame.
        int cleanFrames{}; // Frames drawn with nothing on top since A.
        bool pausedHere{};
        uint64_t hintUntil{}; // The hint shows on entry and after each button, then fades away.
        std::string note;
    };
    static Photo _photo;
    static constexpr uint64_t kPhotoHintMs = 2500;
    // Handheld zoom stops one step short of the game's furthest zoom-out. That last step draws four
    // times as much of the map as the one before, which the Brick's Cortex-A53 cannot keep at
    // frame rate in a busy or wooded park; the park map covers the overview instead.
    static constexpr ZoomLevel kPocketMaxZoom{ 2 };
    static void PocketZoom(bool zoomIn)
    {
        const auto* main = WindowGetMain();
        if (!zoomIn && main != nullptr && main->viewport != nullptr && main->viewport->zoom >= kPocketMaxZoom)
            return;
        Windows::MainWindowZoom(zoomIn, false);
    }

    // Overlay repaints. Every overlay window is transparent, so repainting one also re-renders the
    // park underneath it, which is the expensive part on the device (much more so zoomed out).
    // Overlays are repainted when input arrives, when the HUD's numbers are animating, and at
    // most four times a second otherwise, rather than on every frame.
    static bool _overlayDirty = true;
    static uint64_t _overlayRefreshed{};
    static constexpr uint64_t kOverlayRefreshMs = 250;
    static void MarkOverlayDirty()
    {
        _overlayDirty = true;
    }
    static bool TakeOverlayRefresh()
    {
        const auto now = MenuFeel::Clock();
        if (!_overlayDirty && now - _overlayRefreshed < kOverlayRefreshMs)
            return false;
        _overlayDirty = false;
        _overlayRefreshed = now;
        return true;
    }

    // Keeps one overlay window in step with something Pocket draws. Anything drawn over the park
    // needs a window under it: when the park scrolls, the game shifts the screen's pixels and
    // repaints only the newly exposed strips and the areas windows cover, so an element without a
    // window is dragged along with the map and only partly redrawn.
    static WindowBase* SyncOverlay(WindowNumber number, bool visible, ScreenCoordsXY origin, ScreenSize size, bool repaint)
    {
        auto* wm = GetWindowManager();
        auto* window = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ number });
        if (!visible)
        {
            if (window != nullptr)
                wm->Close(*window);
            return nullptr;
        }
        if (window == nullptr)
        {
            window = wm->Create<FocusWindow>(
                WindowClass::handheldHud, origin, size,
                { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
            if (window != nullptr)
                window->number = number;
            return window;
        }
        if (window->windowPos != origin || window->width != size.width || window->height != size.height)
        {
            window->invalidate();
            window->windowPos = origin;
            window->width = size.width;
            window->height = size.height;
            window->invalidate();
        }
        else if (repaint)
            window->invalidate();
        return window;
    }

    static void SyncFocusWindow()
    {
        auto* wm = GetWindowManager();
        auto* focus = wm->FindByClass(WindowClass::handheldFocus);
        if (!CanExplore())
        {
            if (focus != nullptr)
                wm->Close(*focus);
            wm->CloseByClass(WindowClass::handheldHud);
            return;
        }
        const bool refresh = TakeOverlayRefresh() || cashTicker.Trend() != 0;
        const auto position = Centre() - ScreenCoordsXY{ kCursorWindow / 2, kCursorWindow / 2 };
        if (focus == nullptr)
        {
            // An empty, input-transparent widget list lets map hit testing pass through.
            // A real overlay window also keeps viewport scroll blits from smearing the reticle.
            // Sized for the pulsing target brackets, not just the cross.
            wm->Create<FocusWindow>(
                WindowClass::handheldFocus, position, { kCursorWindow, kCursorWindow },
                { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
        }
        else if (focus->windowPos != position)
        {
            focus->invalidate();
            focus->windowPos = position;
            focus->invalidate();
        }
        else if (_inspect.kind != _drawnInspectKind || (_inspect.kind != InspectKind::none && !MenuFeel::reduceMotion))
            focus->invalidate(); // Target colour changed, or the brackets are pulsing.
        _drawnInspectKind = _inspect.kind;
        // Name chip beside the cursor, sized to the name; absent when nothing is under it.
        auto* chip = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 4 });
        if (_inspect.name.empty() || _terrainActive || Paths::IsActive())
        {
            if (chip != nullptr)
                wm->Close(*chip);
        }
        else
        {
            const auto origin = InspectChipOrigin();
            const auto size = InspectChipSize();
            if (chip == nullptr)
            {
                chip = wm->Create<FocusWindow>(
                    WindowClass::handheldHud, origin, size,
                    { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
                if (chip != nullptr)
                    chip->number = 4;
            }
            else if (chip->windowPos != origin || chip->width != size.width)
            {
                chip->invalidate();
                chip->windowPos = origin;
                chip->width = size.width;
                chip->height = size.height;
            }
            if (chip != nullptr && refresh)
                chip->invalidate();
        }
        auto* toast = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 5 });
        if (!ToastVisible())
        {
            if (toast != nullptr)
                wm->Close(*toast);
        }
        else
        {
            const auto size = ToastSize();
            if (toast == nullptr)
            {
                toast = wm->Create<FocusWindow>(
                    WindowClass::handheldHud, ToastOrigin(), size,
                    { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
                if (toast != nullptr)
                    toast->number = 5;
            }
            else
            {
                toast->invalidate();
                toast->windowPos = ToastOrigin();
                toast->width = size.width;
                toast->height = size.height;
            }
            if (toast != nullptr)
                toast->invalidate();
        }
        auto* hud = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 0 });
        if (hud == nullptr)
        {
            hud = wm->Create<FocusWindow>(
                WindowClass::handheldHud, { 0, ContextGetHeight() - kActionStripHeight },
                { ContextGetWidth(), kActionStripHeight },
                { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
        }
        if (hud != nullptr && refresh)
        {
            hud->invalidate();
            hud->windowPos = { 0, ContextGetHeight() - kActionStripHeight };
            hud->width = ContextGetWidth();
            hud->invalidate();
        }
        auto* top = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 3 });
        if (top == nullptr)
        {
            top = wm->Create<FocusWindow>(
                WindowClass::handheldHud, { Layout::margin, Layout::hudTop },
                { ContextGetWidth() - 2 * Layout::margin, Layout::hudHeight },
                { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
            if (top != nullptr)
                top->number = 3;
        }
        if (top != nullptr && refresh)
        {
            top->invalidate();
            top->width = ContextGetWidth() - 2 * Layout::margin;
            top->invalidate();
        }
        // A separate small input-transparent window redraws only the tool panel,
        // rather than placing a full-screen overlay over the scrolling viewport.
        auto* tools = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 1 });
        if (_terrainActive || Paths::IsActive())
        {
            if (tools == nullptr)
            {
                tools = wm->Create<FocusWindow>(
                    WindowClass::handheldHud, Concept::kOrigin, kTerrainOverlaySize,
                    { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
                if (tools != nullptr)
                    tools->number = 1;
            }
            if (tools != nullptr && refresh)
                tools->invalidate();
        }
        else if (tools != nullptr)
            wm->Close(*tools);
        auto* cost = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 2 });
        if (_terrainActive || Paths::IsActive())
        {
            if (cost == nullptr)
            {
                cost = wm->Create<FocusWindow>(
                    WindowClass::handheldHud,
                    Layout::ContextAnchor({ ContextGetWidth(), ContextGetHeight() }, kContextSize, CustomTracks::Active()),
                    kContextSize,
                    { WindowFlag::stickToFront, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
                if (cost != nullptr)
                    cost->number = 2;
            }
            if (cost != nullptr && refresh)
            {
                cost->invalidate();
                cost->windowPos = Layout::ContextAnchor(
                    { ContextGetWidth(), ContextGetHeight() }, kContextSize, CustomTracks::Active());
                cost->invalidate();
            }
        }
        else if (cost != nullptr)
            wm->Close(*cost);

        // The goal card, the quick menu and the photo-mode hint, which had no windows of their own.
        const ScreenSize screen{ ContextGetWidth(), ContextGetHeight() };
        const bool park = !_terrainActive && !Paths::IsActive();
        const bool goal = park && preferences.hudGoal && !_wheel.open && !ToastVisible() && !_photo.active
            && Goal::Current().has_value();
        SyncOverlay(6, goal, GoalOrigin(screen), { kGoalWidth, kGoalHeight }, refresh);
        SyncOverlay(7, _wheel.open, { 0, 0 }, screen, refresh);
        SyncOverlay(
            8, _photo.active && MenuFeel::Clock() < _photo.hintUntil, { 0, Layout::hudTop }, { screen.width, 40 }, refresh);
    }

    bool HandleMenuInput(ControllerInput& input, const Frame& frame)
    {
        if (GetWindowManager()->FindByClass(WindowClass::handheldMenu) != nullptr)
        {
            if (Windows::HandheldTrackSelectionMove(frame.navigateX, frame.navigateY))
            {
            }
            else if (Windows::HandheldDashboardIsOpen())
                Windows::HandheldDashboardMove(frame.navigateX, frame.navigateY);
            else if (Windows::HandheldObjectPickerIsOpen())
                Windows::HandheldObjectPickerMove(frame.navigateX, frame.navigateY);
            else if (Windows::HandheldFinancesIsOpen())
                Windows::HandheldFinancesMove(frame.navigateX, frame.navigateY);
            else if (Windows::HandheldShopManagementIsOpen())
                Windows::HandheldShopManagementMove(frame.navigateX, frame.navigateY);
            else if (Windows::HandheldConstructionMenuIsOpen())
                Windows::HandheldConstructionMove(frame.navigateX, frame.navigateY);
            else if (frame.navigateY != 0)
                Windows::HandheldMenuMove(frame.navigateY);
            for (const auto action : frame.actions)
            {
                if (Windows::HandheldTextEntryKey(static_cast<int>(action)))
                {
                }
                else if (
                    (action == Action::previousOption || action == Action::nextOption)
                    && Windows::HandheldToolSettingsPage(action == Action::previousOption ? -1 : 1))
                {
                }
                else if (
                    (action == Action::previousOption || action == Action::nextOption)
                    && Windows::HandheldTrackSelectionMove(action == Action::previousOption ? -1 : 1, 0))
                {
                }
                else if (Windows::HandheldParkMapIsOpen() && action == Action::openBuild)
                    Windows::HandheldParkMapSubjects();
                else if (Windows::HandheldParkMapIsOpen() && action == Action::contextPrimary)
                    Invoke(ShortcutId::kViewGeneralRotateClockwise);
                else if (Windows::HandheldParkMapIsOpen() && action == Action::contextSecondary)
                    Windows::HandheldParkMapControl(4);
                else if (Windows::HandheldParkMapIsOpen() && action == Action::slopeUp)
                    Windows::HandheldParkMapControl(2);
                else if (Windows::HandheldParkMapIsOpen() && action == Action::previousOption)
                    Windows::HandheldParkMapControl(3);
                else if (Windows::HandheldParkMapIsOpen() && action == Action::nextOption)
                    Windows::HandheldParkMapControl(0);
                else if (action == Action::contextPrimary && Windows::HandheldObjectPickerCamera())
                { /* Accessible without a stick-click; camera preserves the construction draft. */
                }
                else if (action == Action::contextSecondary && Windows::HandheldObjectPickerGroup())
                {
                }
                else if (action == Action::contextSecondary && Windows::HandheldDashboardNews())
                {
                }
                else if (action == Action::contextSecondary && Windows::HandheldControlsIntro())
                {
                }
                else if (
                    Windows::HandheldObjectPickerIsOpen() && (action == Action::previousOption || action == Action::nextOption))
                    Windows::HandheldObjectPickerMove(0, action == Action::nextOption ? 2 : -2);
                else if (Windows::HandheldSceneryPickerIsOpen() && action == Action::contextPrimary)
                    Windows::HandheldScenerySearchOpen();
                else if (Windows::HandheldDesignPickerIsOpen() && action == Action::contextPrimary)
                    Windows::HandheldDesignFilesOpen();
                else if (
                    Windows::HandheldDashboardIsOpen() && (action == Action::previousOption || action == Action::nextOption))
                    Windows::HandheldDashboardPage(action == Action::previousOption ? -1 : 1);
                else if (action == Action::contextPrimary && Windows::HandheldManagementContext())
                { /* Draft-only research funding shortcut. */
                }
                else if (action == Action::contextPrimary && Windows::HandheldRideQuickAccess())
                { /* Contextual access shortcut at the tab root only. */
                }
                else if (action == Action::confirm)
                    Windows::HandheldMenuConfirm();
                else if (action == Action::openBuild && Windows::HandheldDashboardIsOpen())
                    Windows::HandheldDashboardSwitchPane();
                else if (action == Action::openMenu && Windows::HandheldDashboardIsOpen())
                {
                    if (Windows::HandheldStaffIsOpen())
                        Windows::HandheldStaffPause();
                    else if (Windows::HandheldFinancesIsOpen())
                        Windows::HandheldFinancesPause();
                    else
                        Windows::HandheldDashboardPause();
                }
                else if (action == Action::openMenu && Designs::HasReview())
                    Windows::HandheldDesignReviewPause();
                else if (action == Action::openMenu && Windows::HandheldObjectPickerIsOpen())
                    Windows::HandheldObjectPickerPause();
                else if (action == Action::openMenu && Windows::HandheldShopManagementIsOpen())
                    Windows::HandheldShopManagementPause();
                else if (action == Action::openMenu && Windows::HandheldStaffIsOpen())
                    Windows::HandheldStaffPause();
                else if (action == Action::openMenu && Windows::HandheldFinancesIsOpen())
                    Windows::HandheldFinancesPause();
                else if (action == Action::back || action == Action::openMenu)
                    Windows::HandheldMenuBack();
                input.suspend();
                break;
            }
            return true;
        }
        return false;
    }

    // What A would inspect at the fixed cursor, refreshed each update so the footer verb and the
    // name chip preview the press instead of leaving it to guesswork.
    static InspectTarget ResolveInspectTarget(std::optional<ScreenCoordsXY> at = {})
    {
        const auto info = GetMapCoordinatesFromPos(
            at.value_or(Centre()),
            { ViewportInteractionItem::entity, ViewportInteractionItem::ride, ViewportInteractionItem::parkEntrance });
        const auto rideTarget = [](const Ride* ride) {
            if (ride == nullptr)
                return InspectTarget{};
            const bool shop = ride->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility);
            return InspectTarget{ "Open", ride->getName(), shop ? InspectKind::shop : InspectKind::ride };
        };
        switch (info.interactionType)
        {
            case ViewportInteractionItem::entity:
                if (info.Entity == nullptr)
                    break;
                if (const auto* peep = info.Entity->as<Peep>(); peep != nullptr)
                {
                    const bool staff = info.Entity->type == EntityType::staff;
                    return { "Inspect", peep->getName(), staff ? InspectKind::staff : InspectKind::guest };
                }
                if (const auto* vehicle = info.Entity->as<Vehicle>(); vehicle != nullptr && !vehicle->IsCableLift())
                    return rideTarget(GetRide(vehicle->ride));
                break;
            case ViewportInteractionItem::ride:
                if (info.Element == nullptr)
                    break;
                if (info.Element->getType() == TileElementType::track)
                    return rideTarget(GetRide(info.Element->asTrack()->getRideIndex()));
                if (info.Element->getType() == TileElementType::entrance)
                    return rideTarget(GetRide(info.Element->asEntrance()->getRideIndex()));
                if (info.Element->getType() == TileElementType::path)
                    return rideTarget(GetRide(info.Element->asPath()->getRideIndex()));
                break;
            case ViewportInteractionItem::parkEntrance:
                return { "Open", getGameState().park.name, InspectKind::park };
            default:
                break;
        }
        // Nothing to inspect: a sign can be edited, a plain path or queue extended.
        const auto sign = SceneryTools::Capture(GetMapCoordinatesFromPos(
            at.value_or(Centre()),
            { ViewportInteractionItem::banner, ViewportInteractionItem::wall, ViewportInteractionItem::largeScenery }));
        if (sign && sign->banner != BannerIndex::GetNull())
            return { "Edit", sign->text.empty() ? std::string("Sign") : sign->text, InspectKind::sign };
        const auto path = GetMapCoordinatesFromPos(at.value_or(Centre()), { ViewportInteractionItem::footpath });
        // Only path ends (at most one connected edge): every path tile would bracket most of a park.
        if (path.Element && path.Element->getType() == TileElementType::path && !path.Element->isGhost()
            && std::popcount(static_cast<unsigned>(path.Element->asPath()->getEdges() & 0x0F)) <= 1)
            return { "Extend", path.Element->asPath()->isQueue() ? "Queue end" : "Path end", InspectKind::path };
        return {};
    }

    static void UpdatePhoto();
    // Riding along with more than one train: A moves to the next.
    static bool RideAlongCycles()
    {
        const auto riding = RideFollow::Current();
        const auto* ride = riding ? GetRide(riding->ride) : nullptr;
        return ride && RideFollow::Trains(*ride) > 1 && !_terrainActive && !Paths::IsActive();
    }

    void UpdateExplorer(ControllerInput& input)
    {
        GuestFollow::Update();
        RideFollow::Update();
        UpdatePhoto();
        _inspect = !_terrainActive && !Paths::IsActive() && CanExplore() && ContextHasFocus() ? ResolveInspectTarget()
                                                                                              : InspectTarget{};
        if (gLegacyScene != LegacyScene::playing)
        {
            EndTerrain();
        }
        if (CustomTracks::Completed())
        {
            const auto target = *CustomTracks::Completed();
            const auto context = RideNavigation::Take();
            EndTerrain();
            if (FlatRides::CanResume(target))
            {
                if (ResumeFlatRide(target.id))
                    RideNavigation::construction = context;
            }
            else
                Windows::HandheldRideReturn(target, context);
            input.suspend();
        }
        if (Designs::Completed())
        {
            const auto target = *Designs::Completed();
            const auto context = RideNavigation::Take();
            EndTerrain();
            if (FlatRides::CanResume(target))
            {
                if (ResumeFlatRide(target.id))
                    RideNavigation::construction = context;
            }
            else
                Windows::HandheldRideReturn(target, context);
            input.suspend();
        }
        if (_terrainActive && FlatRides::CurrentStage() == FlatRides::Stage::inactive && FlatRides::Target())
        {
            const auto target = FlatRides::Target();
            ReturnFromRideTool(target);
            input.suspend();
        }
        if (PeepPlacement::Active())
        {
            if (!PeepPlacement::Resolve(*PeepPlacement::target) || PeepPlacement::finished)
                EndTerrain();
            else
                PeepPlacement::Preview(CanExplore() && ContextHasFocus(), Centre());
        }
        if (StaffPatrol::Active())
        {
            if (!StaffManagement::Resolve(*StaffPatrol::target))
                EndTerrain();
            else if (CanExplore() && ContextHasFocus())
                StaffPatrol::ShowOverlay();
            else
                StaffPatrol::HideOverlay();
        }
        if (_ridePathAccess && (!Paths::IsActive() || !AccessTargetValid()))
        {
            EndTerrain();
            input.suspend();
        }
        ClearTerrainSelection();
        if ((_terrainActive || Paths::IsActive()) && CanExplore() && ContextHasFocus())
        {
            if (Paths::IsActive())
            {
                Paths::UpdateTarget(Centre());
                Paths::UpdatePreview();
            }
            auto coords = Paths::IsActive() ? Paths::TargetTile() : ScreenGetMapXY(Centre(), nullptr);
            if (_waterTool || (SimpleTerrain() && Landscape::shape == Landscape::Shape::pond))
            {
                const auto info = GetMapCoordinatesFromPos(
                    Centre(), { ViewportInteractionItem::terrain, ViewportInteractionItem::water });
                coords = info.interactionType == ViewportInteractionItem::none ? std::nullopt
                                                                               : std::optional<CoordsXY>{ info.Loc };
            }
            if (_colourRide)
            {
                _colourTarget = RideManagement::CapturePaint(
                    *_colourRide, GetMapCoordinatesFromPos(Centre(), { ViewportInteractionItem::ride }));
                coords = _colourTarget ? std::optional<CoordsXY>{ _colourTarget->location } : std::nullopt;
            }
            if (_sceneryTool)
            {
                const auto pathHit = GetMapCoordinatesFromPos(
                    Centre(), { ViewportInteractionItem::footpath, ViewportInteractionItem::terrain });
                const auto* path = pathHit.Element ? pathHit.Element->asPath() : nullptr;
                SceneryTools::SetPathTarget(
                    path && !path->isGhost() ? std::optional<CoordsXYZ>{ CoordsXYZ{ pathHit.Loc, path->getBaseZ() } }
                                             : std::nullopt);
                if (path && !path->isGhost()
                    && (SceneryTools::Current().category == SceneryTools::Category::additions
                        || SceneryTools::Current().category == SceneryTools::Category::banners))
                    coords = pathHit.Loc;
                constexpr ViewportInteractionItems pickTypes = {
                    ViewportInteractionItem::scenery, ViewportInteractionItem::wall, ViewportInteractionItem::largeScenery,
                    ViewportInteractionItem::banner, ViewportInteractionItem::pathAddition
                };
                _sceneryEdit = SceneryTools::Capture(GetMapCoordinatesFromPos(
                    Centre(),
                    SceneryTools::Current().mode == SceneryTools::Mode::pick ? pickTypes : kViewportInteractionItemAll));
            }
            if (_sceneryTool && SceneryTools::Current().mode != SceneryTools::Mode::place)
            {
                coords = _sceneryEdit ? std::optional<CoordsXY>{ _sceneryEdit->location } : std::nullopt;
            }
            if (_sceneryRemovalTool)
            {
                _sceneryTarget = SceneryRemoval::FromInfo(GetMapCoordinatesFromPos(Centre(), kViewportInteractionItemAll));
                coords = _sceneryTarget ? std::optional<CoordsXY>{ _sceneryTarget->location } : std::nullopt;
            }
            if (coords)
            {
                _terrainTile = CoordsXY{ coords->x & ~31, coords->y & ~31 };
                gMapSelectPositionA = gMapSelectPositionB = *_terrainTile;
                gMapSelectType = MapSelectType::full;
                if (_sceneryTool && SceneryTools::ScatterActive())
                {
                    const auto bounds = SceneryTools::ScatterBounds(*_terrainTile);
                    gMapSelectPositionA = bounds.first;
                    gMapSelectPositionB = bounds.second;
                }
                if (StaffPatrol::Active())
                {
                    const auto range = StaffPatrol::Range(_terrainTile);
                    if (!range)
                        _terrainTile.reset();
                    else
                    {
                        gMapSelectPositionA = { range->getX1(), range->getY1() };
                        gMapSelectPositionB = { range->getX2(), range->getY2() };
                        gMapSelectType = MapSelectType::fullTerrainAndWater;
                    }
                }
                if (_rightsTool)
                {
                    const auto r = AreaTools::Range(_terrainTile, RightsTools::settings.width, RightsTools::settings.height);
                    if (!r)
                        _terrainTile.reset();
                    else
                    {
                        gMapSelectPositionA = { r->getX1(), r->getY1() };
                        gMapSelectPositionB = { r->getX2(), r->getY2() };
                    }
                }
                if (_sceneryClearTool)
                {
                    const auto range = AreaTools::Range(
                        _terrainTile, SceneryClear::settings.width, SceneryClear::settings.height);
                    if (!range)
                        _terrainTile.reset();
                    else
                    {
                        gMapSelectPositionA = { range->getX1(), range->getY1() };
                        gMapSelectPositionB = { range->getX2(), range->getY2() };
                    }
                }
                if (SimpleTerrain())
                {
                    // Preview the whole brush so the player sees exactly what A will shape.
                    if (const auto range = Landscape::Range(_terrainTile))
                    {
                        gMapSelectPositionA = { range->getX1(), range->getY1() };
                        gMapSelectPositionB = { range->getX2(), range->getY2() };
                        gMapSelectType = Landscape::shape == Landscape::Shape::pond ? MapSelectType::fullWater
                                                                                    : MapSelectType::full;
                    }
                    else
                        _terrainTile.reset();
                }
                if (_areaTool)
                {
                    const auto range = AreaTools::Range(_terrainTile, AreaTools::settings.width, AreaTools::settings.height);
                    if (!range)
                    {
                        _terrainTile.reset();
                    }
                    else
                    {
                        gMapSelectPositionA = { range->getX1(), range->getY1() };
                        gMapSelectPositionB = { range->getX2(), range->getY2() };
                        gMapSelectType = AreaTools::Selection(AreaTools::settings);
                    }
                }
                gMapSelectFlags.set(MapSelectFlag::enable, _terrainTile.has_value());
                _selectionOwned = _terrainTile.has_value();
                MapSelection::invalidate();
            }
        }
        if (FlatRides::CurrentStage() != FlatRides::Stage::inactive && CanExplore() && ContextHasFocus())
        {
            FlatRides::UpdatePreview(_terrainTile);
            if (FlatRides::CurrentStage() != FlatRides::Stage::ride)
            {
                const auto location = FlatRides::EntranceLocation(_terrainTile);
                const auto result = FlatRides::Action(_terrainTile, false);
                if (location && result.error == GameActions::Status::ok)
                {
                    gMapSelectArrowPosition = *location;
                    gMapSelectArrowDirection = location->direction;
                    gMapSelectFlags.set(MapSelectFlag::enableArrow);
                    MapSelection::invalidate();
                }
                else
                {
                    gMapSelectFlags.unset(MapSelectFlag::enable);
                    _selectionOwned = false;
                }
            }
        }
        if (Mazes::Active() && CanExplore() && ContextHasFocus())
            Mazes::Update(_terrainTile);
        if (CustomTracks::Active() && CanExplore() && ContextHasFocus())
            CustomTracks::Update(_terrainTile);
        if (Designs::Active() && CanExplore() && ContextHasFocus())
            Designs::UpdatePreview(_terrainTile);
        if (_shopTool && CanExplore() && ContextHasFocus())
            Shops::UpdatePreview(_terrainTile, _shopIndex, _shopDirection);
        if (_treeTool && CanExplore() && ContextHasFocus())
            Trees::UpdatePreview(_terrainTile, _treeIndex);
        if (CanExplore() && ContextHasFocus() && _sceneryTool)
            SceneryTools::UpdatePreview(_terrainTile);
        if (!CanExplore() || !ContextHasFocus())
        {
            Paths::ClearPreview();
            Trees::ClearPreview();
            SceneryTools::ClearPreview();
            Shops::ClearPreview();
            FlatRides::ClearPreview();
            Mazes::ClearPreview();
            Designs::ClearPreview();
            CustomTracks::ClearPreview();
        }
        SyncFocusWindow();
        HandleExplorerInput(input, input.update(ContextHasFocus(), SDL_GetTicks()));
    }

    // Smart cursor (Settings, off by default). Three small aids, no hard snapping:
    // - friction: panning slows to under half speed over a ride, stall, guest or staff member you
    //   are arriving at (not the one you set off from), so it is easy to stop on;
    // - acceleration: after most of a second of holding, empty ground pans faster;
    // - magnet: let go just short of something and the camera eases the last few pixels onto it.
    static struct
    {
        float x{}, y{};
        uint64_t started{};
        bool panning{};
        std::string leaving;
        ScreenCoordsXY magnet{};
    } _smart;
    static ScreenCoordsXY SmartCursor(const Frame& frame, ScreenCoordsXY pan)
    {
        const bool held = frame.heldX != 0 || frame.heldY != 0;
        const auto now = MenuFeel::Clock();
        if (held)
        {
            _smart.magnet = {};
            if (!_smart.panning)
            {
                _smart.panning = true;
                _smart.started = now;
                _smart.leaving = _inspect.name;
            }
            const bool arriving = !_inspect.name.empty() && _inspect.name != _smart.leaving;
            if (_inspect.name.empty())
                _smart.leaving.clear(); // Once clear of where you started, every target counts.
            const float factor = arriving ? 0.4f : now - _smart.started > 800 ? 1.8f : 1.0f;
            _smart.x += pan.x * factor;
            _smart.y += pan.y * factor;
            const ScreenCoordsXY step{ static_cast<int>(_smart.x), static_cast<int>(_smart.y) };
            _smart.x -= step.x;
            _smart.y -= step.y;
            return step;
        }
        if (_smart.panning)
        {
            _smart.panning = false;
            _smart.x = _smart.y = 0;
            if (_inspect.kind == InspectKind::none)
            {
                // Nearest target in rings of 8 / 16 / 24 px around the cursor.
                const auto centre = Centre();
                for (int radius = 8; radius <= 24 && _smart.magnet == ScreenCoordsXY{}; radius += 8)
                    for (const auto& [dx, dy] : std::array<std::pair<int, int>, 8>{
                             { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }, { 1, -1 }, { 1, 1 }, { -1, 1 }, { -1, -1 } } })
                    {
                        const ScreenCoordsXY offset{ dx * radius, dy * radius };
                        if (ResolveInspectTarget(centre + offset).kind != InspectKind::none)
                        {
                            _smart.magnet = offset;
                            break;
                        }
                    }
            }
        }
        // Ease towards the magnet target: a third of the way each frame, at least a pixel.
        if (_smart.magnet != ScreenCoordsXY{})
        {
            auto ease = [](int remaining) {
                return remaining == 0 ? 0 : remaining / 3 != 0 ? remaining / 3 : (remaining > 0 ? 1 : -1);
            };
            const ScreenCoordsXY step{ ease(_smart.magnet.x), ease(_smart.magnet.y) };
            _smart.magnet -= step;
            return step;
        }
        return pan;
    }
    // Leaving a tool returns to wherever it was opened from (a ride, a guest, a staff member,
    // the design browser), as B always has.
    static void LeaveTool()
    {
        const bool designBrowser = Designs::Active() && Designs::BrowserReturn();
        const auto access = _ridePathAccess;
        const auto ride = ConstructionRide();
        const auto context = RideNavigation::Take();
        const auto person = PeepPlacement::target;
        const auto employee = StaffPatrol::target;
        EndTerrain();
        if (designBrowser)
            Windows::HandheldDesignFilesOpen();
        if (person && person->guest && GuestManagement::Resolve(*person->guest))
            Windows::HandheldGuestOpen(*person->guest);
        if (person && person->staff && StaffManagement::Resolve(*person->staff))
            Windows::HandheldStaffOpen(*person->staff);
        if (employee && StaffManagement::Resolve(*employee))
            Windows::HandheldStaffOpen(employee);
        if (ride && RideManagement::Resolve(*ride))
            Windows::HandheldRideReturn(*ride, context);
        else if (access && RideManagement::Resolve(*access))
            Windows::HandheldRideAccessOpen(*access);
    }
    // Hold B in a tool to undo its last change; a tap still leaves the tool.
    static constexpr uint64_t kUndoHoldMs = 450;
    static uint64_t _backHold{};
    static void HandleBackHold(ControllerInput& input, const Frame& frame)
    {
        const auto elapsed = MenuFeel::Clock() - _backHold;
        if (frame.backHeld && elapsed < kUndoHoldMs)
            return; // Still deciding.
        _backHold = 0;
        if (!frame.backHeld)
        {
            LeaveTool();
            input.suspend();
            return;
        }
        if (Undo::Available())
        {
            const auto result = Undo::Perform();
            _terrainStatus = result.getErrorMessage();
        }
        else if (Paths::IsActive())
        {
            if (const auto draft = Paths::BeginUndo())
            {
                const auto result = Paths::Undo(*draft, true);
                _terrainStatus = result.error == GameActions::Status::ok ? "Undone: last path section"
                                                                         : result.getErrorMessage();
            }
        }
        Audio::Play(Audio::SoundId::click1, -400, Audio::kAudioPlayAtCentre);
        input.suspend(); // The held B must not also leave the tool on release.
    }
    // Y in the park with alerts: a tap glides to the next one, a hold opens the inbox.
    static constexpr uint64_t kInboxHoldMs = 400;
    static uint64_t _alertHold{};
    static void HandleAlertHold(ControllerInput& input, const Frame& frame)
    {
        if (frame.secondaryHeld && MenuFeel::Clock() - _alertHold < kInboxHoldMs)
            return;
        _alertHold = 0;
        if (frame.secondaryHeld)
            Windows::HandheldInboxOpen();
        else if (!JumpToNextAlert())
            Windows::HandheldNewsOpen();
        input.suspend();
    }
    static void ChooseWheel(ControllerInput& input, size_t slot)
    {
        const auto target = QuickWheel::kSlots[slot].target;
        _wheel.Close();
        Audio::Play(Audio::SoundId::click1, -400, Audio::kAudioPlayAtCentre);
        if (target == QuickWheel::Target::pause)
            PauseToggle();
        else
            Windows::HandheldQuickJump(target);
        input.suspend(); // The releasing press must not also act on what just opened.
    }
    // While the wheel is open it owns the controller: the D-pad points, releasing L2 (after a
    // hold) or A takes the highlighted slot, B or a second L2 closes it.
    // Photo mode: everything Pocket draws goes away, leaving the park itself. The D-pad, zoom and
    // rotation work as usual; A saves a screenshot, Y freezes time, B (or START) leaves. The
    // shot is taken one drawn frame after A, so the frame on screen when A was pressed (which may
    // still carry the hint) is never the one saved.
    static void BeginPhoto()
    {
        EndTerrain();
        GuestFollow::Stop();
        _photo = {};
        _photo.active = true;
        _photo.hintUntil = MenuFeel::Clock() + kPhotoHintMs;
    }
    static void EndPhoto()
    {
        if (_photo.pausedHere && GameIsPaused())
            PauseToggle();
        _photo = {};
    }
    bool PhotoModeActive()
    {
        return _photo.active;
    }
    // Called each update: take the pending shot once a clean frame has been drawn.
    static void UpdatePhoto()
    {
        if (!_photo.active || !_photo.pending || _photo.cleanFrames < 1)
            return;
        _photo.pending = false;
        const auto path = ScreenshotDump();
        Audio::Play(Audio::SoundId::windowOpen, 0, Audio::kAudioPlayAtCentre);
        _photo.note = path.empty() ? "Could not save the photo" : "Saved " + Path::GetFileName(path);
        _photo.hintUntil = MenuFeel::Clock() + kPhotoHintMs;
    }
    // True when photo mode handled the frame.
    static bool HandlePhoto(ControllerInput& input, const Frame& frame)
    {
        if (!_photo.active)
            return false;
        for (const auto action : frame.actions)
        {
            switch (action)
            {
                case Action::confirm:
                    _photo.pending = true;
                    _photo.cleanFrames = 0;
                    _photo.hintUntil = 0;
                    input.suspend();
                    return true;
                case Action::back:
                case Action::openMenu:
                    EndPhoto();
                    input.suspend();
                    return true;
                case Action::contextSecondary:
                    if (GameIsPaused() && _photo.pausedHere)
                    {
                        PauseToggle();
                        _photo.pausedHere = false;
                    }
                    else if (!GameIsPaused())
                    {
                        PauseToggle();
                        _photo.pausedHere = true;
                    }
                    _photo.note = GameIsPaused() ? "Time frozen" : "Time running";
                    _photo.hintUntil = MenuFeel::Clock() + kPhotoHintMs;
                    input.suspend();
                    return true;
                case Action::openBuild:
                case Action::slopeDown:
                case Action::slopeUp:
                    // Build and the quick menu wait until photo mode ends.
                    input.suspend();
                    return true;
                default:
                    break; // Zoom and rotate fall through to the park's own handling.
            }
        }
        return false;
    }
    static void DrawPhoto(Drawing::RenderTarget& rt)
    {
        if (_photo.pending)
        {
            ++_photo.cleanFrames;
            return;
        }
        if (MenuFeel::Clock() >= _photo.hintUntil)
            return;
        const ScreenSize screen{ ContextGetWidth(), ContextGetHeight() };
        const std::string text = _photo.note.empty() ? "Photo mode" : _photo.note;
        const int width = std::max(TextWidth(text, true), 200) + 24;
        const ScreenCoordsXY at{ (screen.width - width) / 2, Layout::hudTop };
        DrawCard(rt, at, { width, 40 });
        DrawStrongText(rt, at + ScreenCoordsXY{ 12, 7 }, "{WHITE}" + text, width - 24);
        DrawModernText(
            rt, at + ScreenCoordsXY{ 12, 23 },
            std::string("{MUTED}") + ActionKey(Action::confirm).data() + " photo  " + ActionKey(Action::contextSecondary).data()
                + (GameIsPaused() ? " resume  " : " freeze  ") + ActionKey(Action::back).data() + " done");
    }

    static void HandleWheel(ControllerInput& input, const Frame& frame)
    {
        if (!CanExplore())
        {
            _wheel.Close();
            return;
        }
        _wheel.Point(frame.heldX, frame.heldY);
        for (const auto action : frame.actions)
        {
            if (action == Action::confirm && _wheel.selected)
                return ChooseWheel(input, *_wheel.selected);
            if (action == Action::contextSecondary)
            {
                _wheel.Close();
                BeginPhoto();
                input.suspend();
                return;
            }
            if (action == Action::back || (action == Action::slopeDown && !_wheel.selected))
            {
                _wheel.Close();
                input.suspend();
                return;
            }
            if (action == Action::slopeDown && _wheel.selected)
                return ChooseWheel(input, *_wheel.selected);
        }
        if (!frame.leftTrigger && !_wheel.tapped)
        {
            if (MenuFeel::Clock() - _wheel.opened < QuickWheel::kHoldMs)
                _wheel.tapped = true; // A quick tap: stay open for A / B.
            else if (_wheel.selected)
                return ChooseWheel(input, *_wheel.selected);
            else
                _wheel.Close();
        }
    }
    bool QuickWheelOpen()
    {
        return _wheel.open;
    }

    void HandleExplorerInput(ControllerInput& input, const Frame& frame)
    {
        if (!frame.actions.empty() || frame.panX || frame.panY || frame.navigateX || frame.navigateY || frame.leftTrigger)
            MarkOverlayDirty();
        auto* wm = GetWindowManager();
        for (const auto action : frame.actions)
        {
            if (action == Action::quit)
            {
                ContextQuit();
                input.suspend();
                return;
            }
        }
        if (_wheel.open)
            return HandleWheel(input, frame);
        if (HandlePhoto(input, frame))
            return;
        if (_backHold)
            return HandleBackHold(input, frame);
        if (_alertHold)
            return HandleAlertHold(input, frame);
        if (_ridePathAccess && !AccessTargetValid())
        {
            EndTerrain();
            input.suspend();
            return;
        }
        const auto constructionRide = ConstructionRide();
        if (constructionRide && std::find(frame.actions.begin(), frame.actions.end(), Action::openMenu) != frame.actions.end()
            && !CustomTracks::Busy() && !FlatRides::Busy() && !Mazes::Busy() && !HasTextOrModalInput()
            && (CanExplore() || wm->FindByClass(WindowClass::handheldMenu)))
        {
            const auto context = RideNavigation::Take();
            wm->CloseByClass(WindowClass::handheldMenu);
            RideNavigation::construction = context;
            ReturnFromRideTool(constructionRide);
            input.suspend();
            return;
        }
        if (HandleMenuInput(input, frame))
            return;
        if (_constructionAdjust.active && (_sceneryTool || _areaTool) && CanExplore())
        {
            auto indices = _sceneryTool ? ConstructionAdjust::SceneryRows(_constructionAdjust.focus.tab)
                                        : ConstructionAdjust::TerrainRows(_constructionAdjust.focus.tab);
            _constructionAdjust.move(0, indices.size());
            if (_constructionAdjust.palette && !ConstructionAdjust::PickerAction(_constructionAdjust, _sceneryTool))
                _constructionAdjust.palette = false;
            if (_constructionAdjust.palette)
            {
                _constructionAdjust.colourChoice = MoveColourGrid(
                    _constructionAdjust.colourChoice, 72, frame.navigateX, frame.navigateY);
                for (const auto action : frame.actions)
                {
                    if (action == Action::back || action == Action::contextSecondary)
                        _constructionAdjust.palette = false;
                    else if (action == Action::confirm)
                    {
                        const auto colourRow = indices[_constructionAdjust.row];
                        const auto currentColour = ConstructionAdjust::CurrentColour(_sceneryTool, colourRow);
                        const auto delta = static_cast<int>(_constructionAdjust.colourChoice)
                            - static_cast<int>(ConstructionAdjust::ColourIndex(currentColour));
                        if (_sceneryTool)
                            SceneryTools::Configure(colourRow, delta);
                        else
                            AreaTools::Configure(colourRow, delta);
                        _constructionAdjust.palette = false;
                    }
                    input.suspend();
                    break;
                }
                return; // Palette navigation cannot pan, place, or recolour the park.
            }
            _constructionAdjust.move(frame.navigateY, indices.size());
            const auto row = indices[_constructionAdjust.row];
            const bool enabled = _sceneryTool ? ConstructionAdjust::SceneryEnabled(row)
                                              : ConstructionAdjust::TerrainEnabled(row);
            if (frame.navigateX)
            {
                if (!enabled)
                    _terrainStatus = "Unavailable for this object or operation";
                else
                {
                    if (_sceneryTool)
                        SceneryTools::Configure(row == 6 ? (frame.navigateX > 0 ? 4 : 5) : row, frame.navigateX);
                    else
                        AreaTools::Configure(row, frame.navigateX);
                    _terrainStatus.clear();
                    _scatterFeedback = false;
                }
            }
            for (const auto action : frame.actions)
            {
                if (action == Action::back || action == Action::contextSecondary)
                    _constructionAdjust.active = false;
                else if (action == Action::confirm)
                {
                    if (enabled && _sceneryTool && row == 6)
                    {
                        SceneryTools::Configure(6);
                        _terrainStatus.clear();
                    }
                    else if (enabled && ConstructionAdjust::ColourRow(_sceneryTool, row))
                    {
                        _constructionAdjust.colourChoice = ConstructionAdjust::ColourIndex(
                            ConstructionAdjust::CurrentColour(_sceneryTool, row));
                        _constructionAdjust.palette = true;
                    }
                    else if (_areaTool && _constructionAdjust.focus.tab == 2 && row <= 6)
                        Windows::HandheldTerrainMaterialPickerOpen(row == 6);
                    else
                        _constructionAdjust.active = false; // Return to building, never place on this press.
                }
                else if (action == Action::contextPrimary)
                {
                    if (_sceneryTool)
                        Windows::HandheldSceneryOptionsOpen();
                    else
                        Windows::HandheldAreaOptionsOpen();
                }
                else if (action == Action::openBuild)
                {
                    if (_sceneryTool)
                        Windows::HandheldSceneryCatalogOpen();
                    else
                        Windows::HandheldTerrainMaterialPickerOpen(_constructionAdjust.focus.tab == 2 && row == 6);
                }
                else if (action == Action::previousOption || action == Action::nextOption)
                {
                    _constructionAdjust.tab(action == Action::nextOption ? 1 : -1, _sceneryTool ? 4 : 3);
                    _terrainStatus.clear();
                }
                else if (action == Action::slopeDown || action == Action::slopeUp)
                {
                    if (_sceneryTool)
                        SceneryTools::Configure(action == Action::slopeUp ? 4 : 5);
                    else
                        AreaTools::Configure(1, action == Action::slopeUp ? 1 : -1);
                    _terrainStatus.clear();
                    _scatterFeedback = false;
                }
                else if (action == Action::openMenu)
                    Windows::HandheldDashboardOpen();
                input.suspend();
                break;
            }
            return; // Adjustment owns the D-pad; never leaks into viewport pan or purchase.
        }
        if (Paths::SelectionOwnsInput() && CanExplore())
        {
            for (const auto action : frame.actions)
            {
                if (action == Action::back)
                    Paths::CancelSelection();
                else if (action == Action::confirm)
                {
                    const auto result = Paths::Action(true);
                    _terrainStatus = result.error == GameActions::Status::ok ? "" : result.getErrorMessage();
                }
                input.suspend();
                break;
            }
            return;
        }
        if (Paths::IsAdjusting() && CanExplore())
        {
            bool changed = true;
            if (frame.navigateX)
                changed = Paths::BatchActive() ? true
                    : Paths::IsConnected()     ? Paths::TurnConnected(frame.navigateX)
                                               : Paths::RotateSlope(frame.navigateX);
            if (frame.navigateY)
                changed = Paths::IsConnected() ? Paths::SlopeConnected(-frame.navigateY)
                                               : Paths::ChangeHeight(-frame.navigateY);
            if (!changed)
                _terrainStatus = "Choose a target or check the height limit";
            else if (frame.navigateX || frame.navigateY)
                _terrainStatus.clear();
            for (const auto action : frame.actions)
            {
                if (action == Action::back || action == Action::contextSecondary)
                    Paths::SetAdjusting(false);
                else if (action == Action::confirm)
                {
                    if (Paths::BatchActive())
                        Paths::SetAdjusting(false);
                    else if (Paths::IsConnected())
                        Paths::LevelConnected();
                    else
                        Paths::CycleSlope();
                }
                else if (action == Action::contextPrimary)
                    Windows::HandheldPathOptionsOpen();
                else if (action == Action::openBuild)
                    Windows::HandheldPathSurfacePickerOpen();
                else if (action == Action::openMenu)
                    Windows::HandheldDashboardOpen();
                else if (action == Action::previousOption || action == Action::nextOption)
                {
                    if (!Paths::IsConnected())
                        Paths::CycleMode(action == Action::nextOption ? 1 : -1);
                    else
                        PocketZoom(action == Action::nextOption);
                    _terrainStatus.clear();
                }
                else if (action == Action::slopeDown || action == Action::slopeUp)
                {
                    if (Paths::IsConnected())
                        Paths::SlopeConnected(action == Action::slopeUp ? 1 : -1);
                    else
                        Paths::ChangeHeight(action == Action::slopeUp ? 1 : -1);
                }
                input.suspend();
                break;
            }
            return; // Adjusting geometry never pans or places a path.
        }
        if (Mazes::Active() && CanExplore())
        {
            for (const auto action : frame.actions)
            {
                if (action == Action::openMenu)
                    Windows::HandheldDashboardOpen();
                else if (!Mazes::Busy())
                {
                    if (action == Action::confirm)
                    {
                        if (Mazes::CameraMode())
                            Mazes::ToggleCamera();
                        else if (Mazes::Picking())
                            Mazes::SelectCell(true);
                        else
                            Mazes::Confirm();
                    }
                    else if (action == Action::back)
                    {
                        if (Mazes::Reviewing())
                            Mazes::Cancel();
                        else if (Mazes::CameraMode())
                            Mazes::ToggleCamera();
                        else
                            ReturnFromRideTool(Mazes::Target());
                    }
                    else if (action == Action::previousOption || action == Action::nextOption)
                    {
                        if (Mazes::CameraMode())
                            PocketZoom(action == Action::nextOption);
                        else
                            Mazes::Turn(action == Action::nextOption ? 1 : -1);
                    }
                    else if (action == Action::slopeDown || action == Action::slopeUp)
                    {
                        if (!Mazes::CameraMode())
                            Mazes::Height(action == Action::slopeUp ? 1 : -1);
                    }
                    else if (action == Action::contextPrimary)
                    {
                        if (Mazes::CameraMode())
                            Invoke(ShortcutId::kViewGeneralRotateClockwise);
                        else
                            Mazes::Move();
                    }
                    else if (
                        action == Action::contextSecondary && !Mazes::CameraMode() && !Mazes::Reviewing() && !Mazes::Picking())
                        Mazes::ReviewRemoval();
                    else if (action == Action::openBuild)
                        Mazes::ToggleCamera();
                }
                input.suspend();
                break;
            }
            if (frame.panX || frame.panY)
                InputScrollViewportSmooth({ frame.panX, frame.panY }, WindowGetMain());
            return;
        }
        if (CustomTracks::Active() && CanExplore())
        {
            for (const auto action : frame.actions)
            {
                if (action == Action::openMenu)
                    Windows::HandheldDashboardOpen();
                else if (action == Action::openBuild)
                {
                    if (CustomTracks::CameraMode())
                        CustomTracks::ToggleCamera();
                    else
                        Windows::HandheldTrackPiecePickerOpen();
                }
                else if (action == Action::cameraMode)
                    CustomTracks::ToggleCamera();
                else if (!CustomTracks::Busy())
                {
                    if (action == Action::confirm)
                    {
                        if (CustomTracks::CameraMode())
                            CustomTracks::ToggleCamera();
                        else
                            CustomTracks::Action(true);
                    }
                    else if (action == Action::back)
                    {
                        if (CustomTracks::CameraMode())
                            CustomTracks::ToggleCamera();
                        else if (CustomTracks::Replacing() || CustomTracks::StationPlacement())
                            ReturnFromRideTool(CustomTracks::Target());
                        else
                            CustomTracks::Undo();
                    }
                    else if (action == Action::contextPrimary)
                    {
                        if (CustomTracks::CameraMode())
                            Invoke(ShortcutId::kViewGeneralRotateClockwise);
                        else
                            CustomTracks::Bank();
                    }
                    else if (action == Action::contextSecondary && !CustomTracks::CameraMode())
                    {
                        if (CustomTracks::StationPlacement())
                            CustomTracks::Rotate();
                        else
                            CustomTracks::Chain();
                    }
                    else if (action == Action::previousOption || action == Action::nextOption)
                    {
                        if (CustomTracks::CameraMode())
                            PocketZoom(action == Action::nextOption);
                        else if (CustomTracks::StationPlacement())
                            CustomTracks::Cycle(action == Action::nextOption ? 1 : -1);
                        else
                            CustomTracks::SetTurnSize(action == Action::nextOption ? 1 : -1);
                    }
                    else if (action == Action::slopeDown || action == Action::slopeUp)
                    {
                        if (!CustomTracks::CameraMode())
                            CustomTracks::Slope(action == Action::slopeUp ? 1 : -1);
                    }
                }
                input.suspend();
                break;
            }
            // While building, the D-pad drives the track like a car: left/right bend, up/down climb
            // or dive. One press is one decision, so held repeats are ignored. The camera follows the
            // end of the track on its own; camera mode (and station placement) still pan freely.
            if (!CustomTracks::CameraMode() && !CustomTracks::StationPlacement())
            {
                if (!frame.navigateRepeat && !CustomTracks::Busy() && frame.actions.empty())
                {
                    if (frame.navigateX != 0)
                        CustomTracks::Steer(frame.navigateX);
                    else if (frame.navigateY != 0)
                        CustomTracks::Slope(-frame.navigateY);
                }
                return;
            }
            if (frame.panX || frame.panY)
                InputScrollViewportSmooth({ frame.panX, frame.panY }, WindowGetMain());
            return;
        }
        if (gLegacyScene == LegacyScene::titleSequence)
        {
            const bool scenarios = wm->FindByClass(WindowClass::scenarioSelect) != nullptr;
            bool modal = HasTextOrModalInput() || wm->FindByClass(WindowClass::dropdown) != nullptr
                || wm->FindByClass(WindowClass::changelog) != nullptr;
            for (const auto& window : gWindowList)
            {
                if (window->classification != WindowClass::scenarioSelect
                    && !window->flags.hasAny(WindowFlag::dead, WindowFlag::stickToBack, WindowFlag::stickToFront))
                    modal = true;
            }
            if (!modal)
            {
                if (scenarios && frame.navigateY != 0)
                    Windows::ScenarioSelectHandheldMove(frame.navigateY);
                else if (!scenarios && (frame.navigateX != 0 || frame.navigateY != 0))
                    Windows::TitleMenuHandheldMove(frame.navigateX != 0 ? frame.navigateX : frame.navigateY);
            }
            for (const auto action : frame.actions)
            {
                if (action == Action::back)
                {
                    if (wm->FindByClass(WindowClass::dropdown) != nullptr)
                        wm->CloseByClass(WindowClass::dropdown);
                    else if (modal || scenarios)
                        wm->CloseTop();
                    else
                        ContextQuit();
                }
                else if (!modal && scenarios)
                {
                    if (action == Action::confirm)
                        Windows::ScenarioSelectHandheldConfirm();
                    else if (action == Action::previousOption || action == Action::nextOption)
                        Windows::ScenarioSelectHandheldTab(action == Action::previousOption ? -1 : 1);
                    else if (action == Action::contextPrimary)
                        Windows::ScenarioSelectHandheldToggleLocking();
                }
                else if (!modal && action == Action::confirm)
                    Windows::TitleMenuHandheldConfirm();
                input.suspend();
                break;
            }
            return;
        }
        if (gLegacyScene != LegacyScene::playing || HasTextOrModalInput())
        {
            for (const auto action : frame.actions)
                if (action == Action::back)
                    wm->CloseTop();
            if (!frame.actions.empty() || gLegacyScene != LegacyScene::playing)
                input.suspend();
            return;
        }
        for (const auto action : frame.actions)
        {
            if (FlatRides::Busy() || Designs::Busy())
            {
                input.suspend();
                return;
            }
            if (action == Action::back)
            {
                if (Paths::IsActive() && CanExplore() && Paths::CancelSelection())
                {
                    _terrainStatus.clear();
                    input.suspend();
                    return;
                }
                if (wm->FindByClass(WindowClass::error) != nullptr || gInputFlags.has(InputFlag::toolActive))
                    Invoke(ShortcutId::kInterfaceCancelConstruction);
                else if (wm->FindByClass(WindowClass::dropdown) != nullptr)
                    wm->CloseByClass(WindowClass::dropdown);
                else if ((_terrainActive || Paths::IsActive()) && CanExplore())
                {
                    // With something to undo, B waits to see whether it is a tap (leave the tool)
                    // or a hold (undo). Return without suspending, so the held button stays visible.
                    if (Undo::Available() || (Paths::IsActive() && Paths::BeginUndo()))
                    {
                        _backHold = MenuFeel::Clock();
                        return;
                    }
                    LeaveTool();
                }
                else if (const auto followed = GuestFollow::Current(); followed && CanExplore())
                    Windows::HandheldGuestOpen(*followed);
                else if (const auto riding = RideFollow::Current(); riding && CanExplore())
                {
                    // Back to the ride; the camera stays with the train until something moves it.
                    Windows::HandheldRideOpen(riding->ride);
                }
                else
                    wm->CloseTop();
            }
            else if (action == Action::openMenu && !gInputFlags.has(InputFlag::toolActive))
            {
                Windows::HandheldDashboardOpen();
            }
            else if (action == Action::openBuild && CanExplore())
            {
                if (Designs::Active())
                    Designs::Rotate();
                else if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
                    FlatRides::Rotate();
                else if (PeepPlacement::Active())
                    Windows::HandheldPeepPlacementOptionsOpen();
                else if (StaffPatrol::Active())
                    Windows::HandheldStaffPatrolOptionsOpen();
                else if (_rightsTool)
                    Windows::HandheldRightsOptionsOpen();
                else if (_surfaceTool)
                {
                    AreaTools::settings = {};
                    AreaTools::settings.operation = AreaTools::Operation::paint;
                    AreaTools::settings.surface = _surfaceIndex;
                    AreaTools::ResetColours();
                    _surfaceTool = false;
                    _areaTool = true;
                    Windows::HandheldTerrainMaterialPickerOpen(false);
                }
                else if (_areaTool)
                    Windows::HandheldTerrainMaterialPickerOpen(false);
                else if (_sceneryClearTool)
                    Windows::HandheldSceneryClearOptionsOpen();
                else if (_sceneryTool)
                {
                    if (ConstructionAdjust::SceneryEnabled(2))
                        SceneryTools::Configure(2);
                    else
                        _terrainStatus = "Rotation is automatic for this object";
                }
                else if (_shopTool)
                    RotateShop();
                else if (Paths::IsActive())
                    Windows::HandheldPathSurfacePickerOpen();
                else
                {
                    EndTerrain();
                    Windows::HandheldBuildOpen();
                }
            }
            else if (CanExplore())
            {
                switch (action)
                {
                    case Action::confirm:
                        if (RideAlongCycles())
                        {
                            RideFollow::Next();
                            input.suspend();
                            return;
                        }
                        if (Paths::IsActive())
                        {
                            const auto result = Paths::Action(true);
                            _terrainStatus = result.error == GameActions::Status::ok ? Paths::BatchActive() ? ""
                                    : Paths::IsRemoving()                                                   ? "Path removed."
                                    : Paths::IsQueue()                                                      ? "Queue placed."
                                                                                                            : "Path placed."
                                                                                     : result.getErrorMessage();
                        }
                        else if (_sceneryClearTool && _terrainTile)
                            Windows::HandheldSceneryClearReviewOpen(*_terrainTile);
                        else if (_rightsTool && _terrainTile)
                            Windows::HandheldRightsReviewOpen(*_terrainTile);
                        else if (_terrainActive)
                        {
                            if (_terrainTile)
                            {
                                if (Designs::Active())
                                {
                                    if (Designs::Review(_terrainTile))
                                        Windows::HandheldDesignReviewOpen();
                                    else
                                        _terrainStatus = Designs::Query(_terrainTile).getErrorMessage();
                                    break;
                                }
                                if (_areaTool && AreaTools::settings.operation == AreaTools::Operation::paint)
                                {
                                    Windows::HandheldTerrainPaintReviewOpen(*_terrainTile);
                                    input.suspend();
                                    return;
                                }
                                const bool picking = _sceneryTool && SceneryTools::Current().mode == SceneryTools::Mode::pick;
                                const bool undoable = PrepareUndo();
                                const auto result = TerrainAction(true);
                                if (undoable && result.error == GameActions::Status::ok)
                                    Undo::Commit(result.cost);
                                if (picking && result.error == GameActions::Status::ok)
                                {
                                    _terrainStatus = "Object selected / Move, then A Place";
                                    input.suspend(); // Selection cannot spill into placement on the same held press.
                                    return;
                                }
                                if (_sceneryTool && SceneryTools::ScatterActive())
                                {
                                    _scatterFeedback = result.error == GameActions::Status::ok;
                                    _terrainStatus = result.error == GameActions::Status::ok ? SceneryTools::ScatterStatus()
                                                                                             : result.getErrorMessage();
                                    input.suspend(); // One review/apply transition per released press.
                                    return;
                                }
                                _terrainStatus = result.error == GameActions::Status::ok
                                    ? (FlatRides::CurrentStage() != FlatRides::Stage::inactive ? ""
                                           : _shopTool                                         ? "Shop placed."
                                           : _treeTool                                  ? "Tree planted / move, then A again"
                                           : _sceneryTool                               ? "Scenery placed / move, then A again"
                                           : _colourRide                                ? "Track colour scheme applied."
                                           : _sceneryRemovalTool                        ? "Removed."
                                           : _surfaceTool                               ? "Surface painted."
                                           : _waterTool                                 ? "Water level changed."
                                           : Landscape::shape == Landscape::Shape::pond ? "Pond dug / A again to go deeper"
                                           : Landscape::shape == Landscape::Shape::flatten ? "Levelled one step"
                                                                                           : "Land shaped.")
                                    : result.getErrorMessage();
                                if (undoable && result.error == GameActions::Status::ok && Undo::Available())
                                    _terrainStatus += _terrainStatus.empty() ? "Hold B: undo" : "  |  Hold B: undo";
                            }
                        }
                        else
                        {
                            const auto info = GetMapCoordinatesFromPos(Centre(), { ViewportInteractionItem::ride });
                            if (const auto shop = ShopManagement::FromInfo(info))
                                Windows::HandheldRideOpen(shop->id);
                            else if (info.Element && !info.Element->isGhost())
                            {
                                const auto* ride = GetRide(info.Element->getRideIndex());
                                if (ride && !ride->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility))
                                    Windows::HandheldRideOpen(ride->id);
                            }
                            else if (_inspect.kind == InspectKind::path)
                            {
                                // Start the connected path tool on this path, as if chosen from
                                // Build > Paths > Extend and then pointed at.
                                const auto pathInfo = GetMapCoordinatesFromPos(Centre(), { ViewportInteractionItem::footpath });
                                const bool queue = pathInfo.Element && pathInfo.Element->getType() == TileElementType::path
                                    && pathInfo.Element->asPath()->isQueue();
                                EndTerrain();
                                Paths::BeginConnected(queue);
                                Paths::UpdateTarget(Centre());
                                const auto result = Paths::Action(true);
                                _terrainStatus = result.error == GameActions::Status::ok ? "Steer with the D-pad, A to lay path"
                                                                                         : result.getErrorMessage();
                                input.suspend();
                                return;
                            }
                            else if (_inspect.kind == InspectKind::sign)
                            {
                                _sceneryEdit = SceneryTools::Capture(GetMapCoordinatesFromPos(
                                    Centre(),
                                    { ViewportInteractionItem::banner, ViewportInteractionItem::wall,
                                      ViewportInteractionItem::largeScenery }));
                                Windows::HandheldSignEditOpen();
                                input.suspend();
                                return;
                            }
                            else if (!info.Element || !info.Element->isGhost())
                                ViewportInteractionLeftClick(Centre());
                        }
                        break;
                    case Action::contextPrimary:
                        if (Designs::Active() && !Designs::Busy())
                        {
                            Windows::HandheldDesignOptionsOpen();
                            input.suspend();
                            return;
                        }
                        if (Paths::IsActive() && !Paths::IsConnected())
                        {
                            Paths::ToggleRemove();
                            _terrainStatus.clear();
                            _scatterFeedback = false;
                            break;
                        }
                        Invoke(ShortcutId::kViewGeneralRotateClockwise);
                        break;
                    case Action::previousOption:
                        PocketZoom(false);
                        break;
                    case Action::nextOption:
                        PocketZoom(true);
                        break;
                    case Action::contextSecondary:
                        if (Paths::IsActive())
                        {
                            Paths::SetAdjusting(true);
                            _terrainStatus.clear();
                            _scatterFeedback = false;
                            break;
                        }
                        if (_terrainActive)
                        {
                            if (Designs::Active() && !Designs::Busy())
                                Windows::HandheldDesignPickerOpen();
                            else if (FlatRides::CurrentStage() == FlatRides::Stage::ride && !FlatRides::Busy())
                                Windows::HandheldRidePickerOpen();
                            else if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
                            {
                                // Entrance or exit: Y places both and lays their paths.
                                // Success hands back to the ride overview once it is built, so the
                                // summary travels with it; a refusal stays here in the status line.
                                std::string summary;
                                const auto placed = FlatRides::AutoPlace(summary);
                                if (placed.error == GameActions::Status::ok)
                                    Windows::HandheldRideReturnNotice(summary);
                                _terrainStatus = summary;
                                input.suspend();
                                return;
                            }
                            else if (_shopTool)
                                Windows::HandheldShopPickerOpen();
                            else if (PeepPlacement::Active())
                                Windows::HandheldPeepPlacementOptionsOpen();
                            else if (StaffPatrol::Active())
                                StaffPatrol::Configure(1);
                            else if (_rightsTool)
                                Windows::HandheldRightsOptionsOpen();
                            else if (_areaTool)
                                _constructionAdjust.active = true;
                            else if (_sceneryClearTool)
                                Windows::HandheldSceneryClearOptionsOpen();
                            else if (_sceneryTool)
                                _constructionAdjust.active = true;
                            else if (_treeTool)
                            {
                                Windows::HandheldTreePickerOpen();
                            }
                            else if (_colourRide)
                                _colourScheme = (_colourScheme + 1) % kNumRideColourSchemes;
                            else if (_surfaceTool)
                            {
                                AreaTools::settings = {};
                                AreaTools::settings.operation = AreaTools::Operation::paint;
                                AreaTools::settings.surface = _surfaceIndex;
                                AreaTools::ResetColours();
                                _surfaceTool = false;
                                _areaTool = true;
                                _constructionAdjust.active = true;
                                _constructionAdjust.focus.tab = 2;
                                _constructionAdjust.row = 0;
                            }
                            else if (SimpleTerrain())
                                Landscape::Cycle(1);
                            else if (!_sceneryRemovalTool)
                            {
                                _areaTool = true;
                                AreaTools::settings.operation = _waterTool
                                    ? (_raiseLand ? AreaTools::Operation::waterRaise : AreaTools::Operation::waterLower)
                                    : (_raiseLand ? AreaTools::Operation::raise : AreaTools::Operation::lower);
                                _waterTool = false;
                                _constructionAdjust.active = true;
                            }
                            _terrainStatus.clear();
                            _scatterFeedback = false;
                        }
                        else if (!Alerts::Current().empty())
                        {
                            // Decide on release (or after the hold time): return without
                            // suspending so the held button stays visible.
                            _alertHold = MenuFeel::Clock();
                            return;
                        }
                        else
                            Windows::HandheldNewsOpen();
                        break;
                    case Action::back:
                    case Action::openMenu:
                    case Action::openBuild:
                    case Action::quit:
                    case Action::slopeDown:
                    case Action::slopeUp:
                        // L2 with no tool in hand opens the quick wheel. Return without the usual
                        // suspend, which would make the still-held L2 read as released.
                        if (action == Action::slopeDown && !_terrainActive && !Paths::IsActive() && !Designs::Active()
                            && !StaffPatrol::Active())
                        {
                            _wheel.Open(MenuFeel::Clock());
                            Audio::Play(Audio::SoundId::click1, -600, Audio::kAudioPlayAtCentre);
                            return;
                        }
                        if (Designs::Active())
                            Designs::SetScenery(action == Action::slopeUp);
                        else if (StaffPatrol::Active())
                            StaffPatrol::ResizeBrush(action == Action::slopeUp ? 1 : -1);
                        else if (Paths::IsConnected())
                            Paths::SlopeConnected(action == Action::slopeUp ? 1 : -1);
                        else if (Paths::IsActive())
                            Paths::ChangeHeight(action == Action::slopeUp ? 1 : -1);
                        else if (_sceneryTool)
                            SceneryTools::Configure(action == Action::slopeUp ? 4 : 5);
                        else if (_areaTool)
                            AreaTools::Configure(1, action == Action::slopeUp ? 1 : -1);
                        else if (SimpleTerrain())
                            Landscape::Resize(action == Action::slopeUp ? 1 : -1);
                        break;
                    case Action::cameraMode:
                        break;
                }
            }
            // Never let a second queued action confirm/close a newly opened window.
            input.suspend();
            return;
        }
        auto pan = ScreenCoordsXY{ frame.panX, frame.panY };
        if (preferences.smartCursor && CanExplore() && !_terrainActive && !Paths::IsActive())
            pan = SmartCursor(frame, pan);
        if (CanExplore() && (pan.x != 0 || pan.y != 0))
        {
            auto* main = WindowGetMain();
            _terrainStatus.clear();
            _scatterFeedback = false;
            GuestFollow::Stop();
            RideFollow::Stop();
            WindowUnfollowSprite(*main);
            InputScrollViewportSmooth(pan, main);
        }
    }

    void DrawExplorer(Drawing::RenderTarget& rt)
    {
        MenuThemes::settings.Load(GetContext()->GetPlatformEnvironment().GetDirectoryPath(DirBase::user));
        Theme::Refresh();
        if (!IsEnabled() || !CanExplore())
            return;
        if (_photo.active)
            return DrawPhoto(rt);
        using namespace Drawing;
        const auto c = Centre();
        const auto height = ContextGetHeight();
        if (!_terrainActive && !Paths::IsActive())
        {
            const auto& recent = getGameState().park.newsItems.getRecent();
            if (!recent.empty())
            {
                Concept::Box(rt, { 8, 36 }, { ContextGetWidth() - 16, 28 });
                DrawModernText(rt, { 16, 46 }, "{GOLD}Y News");
                DrawModernEllipsised(rt, { 74, 46 }, ContextGetWidth() - 90, Windows::HandheldNewsText(recent.front().text));
            }
        }
        const auto context = _terrainActive || Paths::IsActive() ? Layout::Context::construction : Layout::Context::park;
        if (Layout::ShowsHud(context) && rt.y < Layout::hudTop + Layout::hudHeight && rt.y + rt.height > Layout::hudTop)
            DrawHud(rt, { ContextGetWidth(), height }, NativeHud());
        // The goal ticker shares the space under the HUD with toasts, which win while they show.
        if (context == Layout::Context::park && preferences.hudGoal && !_wheel.open && !ToastVisible())
            if (const auto goal = Goal::Current())
                DrawGoal(rt, { ContextGetWidth(), height }, *goal);
        // Completion can arrive after UpdateExplorer but before this frame is drawn.
        // Never fall through to terrain controls while waiting for the next update to end the tool.
        if (Designs::Completed()
            || (_terrainActive && FlatRides::CurrentStage() == FlatRides::Stage::inactive && FlatRides::Target()))
        {
            DrawGameplayFooter(rt, { ContextGetWidth(), height }, "", false, false);
            DrawMapFocus(rt, c);
            return;
        }
        if (Mazes::Active())
        {
            Mazes::DrawPanel(rt, { ContextGetWidth(), height });
            return;
        }
        if (CustomTracks::Active())
        {
            CustomTracks::DrawPanel(rt, { ContextGetWidth(), height });
            // Steering builds from the end of the track, not from the screen centre, so the cross
            // only appears where it means something: free camera and placing the station.
            if (CustomTracks::CameraMode() || CustomTracks::StationPlacement())
                DrawMapFocus(rt, c);
            return;
        }
        std::string status = GuestFollow::Current() ? "Follow / B Guest / D-pad Stop"
            : RideFollow::Current()                 ? RideFollow::Label()
                                                    : "L/R: Zoom   X: Rotate";
        std::string costAmount;
        if (_terrainActive)
        {
            status = StaffPatrol::Active()                                ? "PATROL / Select Brush"
                : PeepPlacement::Active()                                 ? "PLACE PERSON"
                : Designs::Active()                                       ? "PLACE DESIGN"
                : FlatRides::CurrentStage() != FlatRides::Stage::inactive ? "PLACE RIDE"
                : _shopTool                                               ? "PLACE SHOP"
                : _treeTool                                               ? "PLACE TREE"
                : _sceneryTool                                            ? "SCENERY"
                : _areaTool           ? AreaTools::operations[static_cast<size_t>(AreaTools::settings.operation)]
                : _colourRide         ? "TRACK SCHEME " + std::to_string(_colourScheme + 1)
                : _sceneryRemovalTool ? "REMOVE SCENERY"
                : _surfaceTool        ? "PAINT SURFACE"
                : _waterTool          ? (_raiseLand ? "RAISE WATER" : "LOWER WATER")
                                      : std::string(Landscape::Name(Landscape::shape));
            if (_terrainTile)
            {
                const auto result = TerrainAction(false);
                if (result.error == GameActions::Status::ok)
                {
                    costAmount = FormatMoney(result.cost) + (_sceneryTool && SceneryTools::ScatterActive() ? " est." : "");
                }
                else
                    status = result.getErrorMessage(); // The panel already names the tool and stage.
            }
            else
                status = _sceneryTool && SceneryTools::Current().mode == SceneryTools::Mode::pick ? "Point at placed scenery"
                    : _colourRide                                                                 ? "Point at this ride's track"
                    : _sceneryRemovalTool                                                         ? "Point at small scenery"
                                                                                                  : "Move onto land";
            if (!_terrainStatus.empty())
                status = _sceneryTool && SceneryTools::ScatterActive() && _scatterFeedback ? SceneryTools::ScatterStatus()
                                                                                           : _terrainStatus;
        }
        if (Paths::IsActive())
        {
            const auto result = Paths::Action(false);
            status = Paths::IsRemoving() ? "REMOVE PATH" : "PLACE PATH";
            if (result.error == GameActions::Status::ok)
            {
                costAmount = FormatMoney(result.cost);
            }
            else
                status = result.getErrorMessage();
            if (!_terrainStatus.empty())
                status = _sceneryTool && SceneryTools::ScatterActive() && _scatterFeedback ? SceneryTools::ScatterStatus()
                                                                                           : _terrainStatus;
            if (Paths::BatchActive())
            {
                if (!Paths::BatchMessage().empty())
                    status = Paths::BatchMessage();
                else if (result.error != GameActions::Status::ok)
                    status = result.getErrorMessage();
            }
            Paths::DrawBatchWorld(rt);
            Paths::DrawOverlay(rt, Concept::kOrigin);
            Paths::DrawFooter(rt, { ContextGetWidth(), height });
        }
        else
        {
            if (_terrainActive)
            {
                if (Designs::Active())
                    Designs::DrawPanel(rt, { ContextGetWidth(), height });
                else if (FlatRides::CurrentStage() != FlatRides::Stage::inactive)
                    FlatRides::DrawPanel(rt, { ContextGetWidth(), height });
                else if (_shopTool)
                    DrawShopOverlay(rt, Concept::kOrigin, Shops::Name(_shopIndex), Shops::Eligible(_shopIndex), _shopDirection);
                else if (PeepPlacement::Active())
                    PeepPlacement::DrawOverlay(rt, Concept::kOrigin);
                else if (StaffPatrol::Active())
                    StaffPatrol::DrawOverlay(rt, Concept::kOrigin);
                else if (_rightsTool)
                    DrawConstructionOverlay(
                        rt, Concept::kOrigin, "Land / Rights", RightsTools::Kind(RightsTools::settings.construction),
                        "A: Purchase review", RightsTools::Summary(_terrainTile, RightsTools::settings),
                        std::to_string(RightsTools::settings.width) + " x " + std::to_string(RightsTools::settings.height)
                            + " tiles");
                else if ((_areaTool || _sceneryTool) && _constructionAdjust.active)
                    ConstructionAdjust::Draw(rt, Concept::kOrigin, _constructionAdjust, _sceneryTool);
                else if (_areaTool)
                {
                    AreaTools::DrawOverlay(rt, Concept::kOrigin);
                    DrawModernEllipsised(rt, Concept::kOrigin + ScreenCoordsXY{ 12, 154 }, 140, "{MUTED}L2/R2 Operation");
                }
                else if (_sceneryClearTool)
                    SceneryClear::DrawOverlay(rt, Concept::kOrigin);
                else if (_sceneryTool)
                    SceneryTools::DrawOverlay(rt, Concept::kOrigin, _sceneryEdit);
                else if (_treeTool)
                    DrawTreeOverlay(rt, Concept::kOrigin, Trees::Name(_treeIndex), Trees::Eligible(_treeIndex));
                else if (_colourRide)
                    DrawTrackColourOverlay(rt, Concept::kOrigin, _colourScheme, _colourTarget.has_value());
                else if (_sceneryRemovalTool)
                    DrawSceneryRemovalOverlay(
                        rt, Concept::kOrigin, SceneryRemoval::Name(_sceneryTarget), _sceneryTarget.has_value());
                else if (_surfaceTool)
                    DrawSurfaceOverlay(rt, Concept::kOrigin, Surface::Name(_surfaceIndex));
                else
                    DrawLandscapeOverlay(rt, Concept::kOrigin, static_cast<int>(Landscape::shape), Landscape::brush);
            }
            if (Designs::Active() || FlatRides::CurrentStage() != FlatRides::Stage::inactive)
            { /* The shared ride panel owns its action strip. */
            }
            else if (_shopTool)
                DrawShopFooter(rt, { ContextGetWidth(), height });
            else if (PeepPlacement::Active())
                DrawConstructionFooter(rt, { ContextGetWidth(), height }, "Place", "Options");
            else if (StaffPatrol::Active())
                DrawConstructionFooter(rt, { ContextGetWidth(), height }, "Apply", "Add/Remove");
            else if (_rightsTool)
                DrawConstructionFooter(rt, { ContextGetWidth(), height }, "Review", "Options");
            else if (_sceneryClearTool)
                DrawConstructionFooter(rt, { ContextGetWidth(), height }, "Review", "Options");
            else if (_areaTool || _sceneryTool)
                ConstructionAdjust::Footer(
                    rt, { ContextGetWidth(), height }, _constructionAdjust.active, _sceneryTool,
                    _areaTool ? AreaTools::settings.operation == AreaTools::Operation::paint ? "Review" : "Apply"
                        : SceneryTools::Current().mode == SceneryTools::Mode::pick ? "Pick"
                        : SceneryTools::ScatterActive() ? SceneryTools::ScatterReviewed() ? "Apply" : "Review"
                        : SceneryTools::Current().mode == SceneryTools::Mode::remove   ? "Remove"
                        : SceneryTools::Current().mode == SceneryTools::Mode::recolour ? "Paint"
                                                                                       : "Place",
                    _constructionAdjust.active && ConstructionAdjust::PickerAction(_constructionAdjust, _sceneryTool),
                    _constructionAdjust.palette,
                    _sceneryTool && _constructionAdjust.focus.tab == 0 && _constructionAdjust.row == 2
                        && ConstructionAdjust::SceneryEnabled(6));
            else if (_treeTool)
                DrawTreeFooter(rt, { ContextGetWidth(), height });
            else if (_colourRide)
                DrawTrackColourFooter(rt, { ContextGetWidth(), height });
            else if (_sceneryRemovalTool)
                DrawSceneryRemovalFooter(rt, { ContextGetWidth(), height });
            else if (_surfaceTool)
                ConstructionAdjust::Footer(rt, { ContextGetWidth(), height }, false, false, "Paint");
            else if (SimpleTerrain())
                DrawFooter(
                    rt, { ContextGetWidth(), height },
                    { { "D-pad", "Move" },
                      { "A", Landscape::Name(Landscape::shape) },
                      { "Y", "Shape" },
                      { "L2/R2", "Brush size" },
                      { "B", "Back" },
                      { "L1/R1", "Zoom", HintPriority::guide } });
            else if (!_wheel.open) // The wheel draws its own footer.
                DrawGameplayFooter(
                    rt, { ContextGetWidth(), height }, status, _terrainActive, _surfaceTool || _colourRide.has_value(),
                    RideAlongCycles() ? "Next" : _inspect.verb, Alerts::HintLabel());
        }
        // A colour palette owns the D-pad; placement feedback/cursor is unrelated
        // here and suggested a placement error while merely choosing a colour.
        if (_constructionAdjust.active && _constructionAdjust.palette && (_sceneryTool || _areaTool))
            return;
        const bool feedback = !_terrainStatus.empty() || (Paths::BatchActive() && !Paths::BatchMessage().empty());
        if (Paths::BatchActive() && (feedback || costAmount.empty()))
            DrawConstructionMessage(rt, { ContextGetWidth(), height }, status);
        else if ((_terrainActive || Paths::IsActive()) && (feedback || costAmount.empty()))
            DrawContextMessage(rt, Layout::ContextAnchor({ ContextGetWidth(), height }, kContextSize), status);
        else if (!costAmount.empty())
            DrawCostBadge(
                rt, Layout::ContextAnchor({ ContextGetWidth(), height }, kContextSize), costAmount, Paths::BatchActive());
        if ((GuestFollow::Current() || RideFollow::Current()) && !_terrainActive && !Paths::IsActive())
            DrawContextMessage(rt, Layout::ContextAnchor({ ContextGetWidth(), height }, kContextSize), status);
        else if (!_wheel.open && !_inspect.name.empty() && !_terrainActive && !Paths::IsActive())
            DrawInspectChip(rt, InspectChipOrigin(), InspectChipSize(), _inspect.name, InspectColour(_inspect.kind));
        if (ToastVisible())
        {
            // Clipped to its resting slot so it slides out from under the HUD rather than over it.
            const auto size = ToastSize();
            Drawing::RenderTarget slot;
            if (ClipRenderTarget(slot, rt, ToastOrigin(), size.width, size.height))
            {
                const ScreenCoordsXY origin{ 0, ToastOffset() };
                DrawCard(slot, origin, size);
                Fill(
                    slot, origin + ScreenCoordsXY{ 5, 4 }, { 4, size.height - 8 },
                    static_cast<Drawing::PaletteIndex>(_toast.notice ? PocketPalette::caution : PocketPalette::destructive));
                DrawStrongText(
                    slot, origin + ScreenCoordsXY{ 16, 6 }, (_toast.notice ? "{CAUTION}" : "{DANGER}") + _toast.title,
                    size.width - 24);
                if (!_toast.message.empty())
                    DrawModernEllipsised(slot, origin + ScreenCoordsXY{ 16, 23 }, size.width - 24, "{WHITE}" + _toast.message);
            }
        }
        // High contrast, fixed map focus; this does not move the desktop pointer. While exploring
        // it changes shape and colour over anything A can inspect; tools keep the precise cross.
        if (!_terrainActive && !Paths::IsActive() && _inspect.kind != InspectKind::none)
            DrawTargetBrackets(rt, c, InspectColour(_inspect.kind), BracketPulse());
        else
            DrawMapFocus(rt, c);
        if (_wheel.open)
        {
            const auto [label, sprite] = Windows::HandheldLastBuild();
            QuickWheel::Draw(rt, { ContextGetWidth(), ContextGetHeight() }, _wheel, label, sprite, gGamePaused != 0);
        }
    }

    void ShowToast(std::string_view title, std::string_view message)
    {
        Audio::Play(Audio::SoundId::error, -300, Audio::kAudioPlayAtCentre);
        if (!CanExplore() || _terrainActive || Paths::IsActive())
            return;
        _toast.title = Windows::HandheldNewsText(std::string(title));
        _toast.message = Windows::HandheldNewsText(std::string(message));
        _toast.shown = MenuFeel::Clock();
        _toast.notice = false;
    }

    static size_t _alertIndex{};
    // Y in the park: glide to the next thing that needs the player and say what usually fixes it.
    // The camera stops with the problem under the cursor, so A opens that ride (or the guest there)
    // exactly as if the player had walked over to it. With nothing to report, Y is News as before.
    bool JumpToNextAlert()
    {
        Alerts::Invalidate();
        const auto& alerts = Alerts::Current();
        auto* main = WindowGetMain();
        if (alerts.empty() || main == nullptr)
            return false;
        const auto& alert = alerts[_alertIndex++ % alerts.size()];
        JumpToAlert(alert);
        if (alerts.size() > 1)
            _toast.message = std::to_string((_alertIndex - 1) % alerts.size() + 1) + " of " + std::to_string(alerts.size())
                + "  |  " + _toast.message;
        return true;
    }
    void JumpToAlert(const Alerts::Alert& alert)
    {
        auto* main = WindowGetMain();
        if (main == nullptr)
            return;
        WindowScrollToLocation(*main, alert.location);
        Audio::Play(Audio::SoundId::click1, -400, Audio::kAudioPlayAtCentre);
        _toast.title = Windows::HandheldNewsText(alert.title);
        _toast.message = Windows::HandheldNewsText(alert.fix);
        _toast.shown = MenuFeel::Clock();
        _toast.notice = true;
    }
} // namespace OpenRCT2::Ui::Handheld

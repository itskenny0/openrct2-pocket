// SPDX-License-Identifier: GPL-3.0-only
#include "SceneryTools.h"

#include "TextEntry.h"

#include <openrct2-ui/interface/Dropdown.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/footpath/FootpathAdditionPlaceAction.h>
#include <openrct2/actions/footpath/FootpathAdditionRemoveAction.h>
#include <openrct2/actions/scenery/BannerPlaceAction.h>
#include <openrct2/actions/scenery/BannerRemoveAction.h>
#include <openrct2/actions/scenery/BannerSetColourAction.h>
#include <openrct2/actions/scenery/BannerSetNameAction.h>
#include <openrct2/actions/scenery/LargeSceneryPlaceAction.h>
#include <openrct2/actions/scenery/LargeSceneryRemoveAction.h>
#include <openrct2/actions/scenery/LargeScenerySetColourAction.h>
#include <openrct2/actions/scenery/SignSetNameAction.h>
#include <openrct2/actions/scenery/SmallSceneryPlaceAction.h>
#include <openrct2/actions/scenery/SmallSceneryRemoveAction.h>
#include <openrct2/actions/scenery/SmallScenerySetColourAction.h>
#include <openrct2/actions/scenery/WallPlaceAction.h>
#include <openrct2/actions/scenery/WallRemoveAction.h>
#include <openrct2/actions/scenery/WallSetColourAction.h>
#include <openrct2/drawing/Colour.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/object/BannerObject.h>
#include <openrct2/object/LargeSceneryObject.h>
#include <openrct2/object/ObjectEntryManager.h>
#include <openrct2/object/ObjectLimits.h>
#include <openrct2/object/SmallSceneryObject.h>
#include <openrct2/object/WallObject.h>
#include <openrct2/world/Banner.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/BannerElement.h>
#include <openrct2/world/tile_element/LargeSceneryElement.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SmallSceneryElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/WallElement.h>
namespace OpenRCT2::Ui::Handheld::SceneryTools
{
    static Settings current;
    static std::optional<CoordsXY> lastTile;
    struct Request
    {
        CoordsXY tile;
        Settings settings;
        bool operator==(const Request&) const = default;
    };
    static std::optional<Request> requested;
    static std::optional<Target> ghost;
    static uint32_t suspended{};
    static ObjectType Type(Category c)
    {
        if (c == Category::additions)
            return ObjectType::pathAdditions;
        return c == Category::small ? ObjectType::smallScenery
            : c == Category::large  ? ObjectType::largeScenery
            : c == Category::walls  ? ObjectType::walls
                                    : ObjectType::banners;
    }
    static uint8_t SceneryType(Category c)
    {
        if (c == Category::additions)
            return SCENERY_TYPE_PATH_ITEM;
        return c == Category::small ? SCENERY_TYPE_SMALL
            : c == Category::large  ? SCENERY_TYPE_LARGE
            : c == Category::walls  ? SCENERY_TYPE_WALL
                                    : SCENERY_TYPE_BANNER;
    }
    static GameActions::Result Invalid(const char* message)
    {
        GameActions::Result r;
        r.error = GameActions::Status::invalidParameters;
        r.errorMessage = message;
        return r;
    }
    bool Eligible(Category c, ObjectEntryIndex i)
    {
        return GetContext()->GetObjectManager().GetLoadedObject(Type(c), i) && IsSceneryAvailableToBuild({ SceneryType(c), i });
    }
    std::string CategoryName(Category c)
    {
        if (c == Category::additions)
            return "Path furniture";
        return c == Category::small ? "Small scenery / trees"
            : c == Category::large  ? "Large scenery"
            : c == Category::walls  ? "Walls / fences"
                                    : "Banners / signs";
    }
    std::vector<std::string> Categories()
    {
        return { "Small scenery / trees", "Large scenery",  "Walls / fences",
                 "Banners / signs",       "Path furniture", "Back to build" };
    }
    std::string Name(const Settings& s)
    {
        const auto* o = GetContext()->GetObjectManager().GetLoadedObject(Type(s.category), s.object);
        return o ? o->GetName() : "No objects available";
    }
    std::vector<PickerEntry> Entries(Category c)
    {
        std::vector<PickerEntry> result;
        const auto count = c == Category::additions ? kMaxPathAdditionObjects
            : c == Category::small                  ? kMaxSmallSceneryObjects
            : c == Category::large                  ? kMaxLargeSceneryObjects
            : c == Category::walls                  ? kMaxWallSceneryObjects
                                                    : kMaxBannerObjects;
        for (ObjectEntryIndex i = 0; i < count; ++i)
            if (Eligible(c, i))
                result.push_back({ { Type(c), i }, GetContext()->GetObjectManager().GetLoadedObject(Type(c), i)->GetName() });
        return result;
    }
    std::array<bool, 3> Channels(Category c, ObjectEntryIndex i)
    {
        if (c == Category::small)
        {
            const auto* e = ObjectEntryManager::GetObjectEntry<SmallSceneryEntry>(i);
            if (e)
                return { e->flags.hasAny(SmallSceneryFlag::hasPrimaryColour, SmallSceneryFlag::hasGlass),
                         e->flags.has(SmallSceneryFlag::hasSecondaryColour),
                         e->flags.has(SmallSceneryFlag::hasTertiaryColour) };
        }
        if (c == Category::large)
        {
            const auto* e = ObjectEntryManager::GetObjectEntry<LargeSceneryEntry>(i);
            if (e)
                return { e->flags.has(LargeSceneryFlag::hasPrimaryColour), e->flags.has(LargeSceneryFlag::hasSecondaryColour),
                         e->flags.has(LargeSceneryFlag::hasTertiaryColour) };
        }
        if (c == Category::walls)
        {
            const auto* e = ObjectEntryManager::GetObjectEntry<WallSceneryEntry>(i);
            if (e)
                return { e->flags.hasAny(WallSceneryFlag::hasPrimaryColour, WallSceneryFlag::hasGlass)
                             && !e->flags2.has(WallSceneryFlag2::disablePrimaryColour),
                         e->flags.has(WallSceneryFlag::hasSecondaryColour), e->flags.has(WallSceneryFlag::hasTertiaryColour) };
        }
        if (c == Category::banners)
        {
            const auto* e = ObjectEntryManager::GetObjectEntry<BannerSceneryEntry>(i);
            if (e)
                return { e->flags.has(BannerEntryFlag::hasPrimaryColour), false, false };
        }
        return {};
    }
    const Settings& Current()
    {
        return current;
    }
    void SelectCategory(Category c)
    {
        ClearPreview();
        current.category = c;
        current.object = kObjectEntryIndexNull;
        current.mode = Mode::place;
        const auto list = Entries(c);
        if (!list.empty())
            current.object = list.front().identity.index;
    }
    bool Select(ObjectEntryIndex i)
    {
        if (!Eligible(current.category, i))
            return false;
        ClearPreview();
        current.object = i;
        return true;
    }
    static std::string ColourName(Drawing::Colour c)
    {
        for (uint8_t i = 0; i < 72; ++i)
            if (Windows::ColourDropDownIndexToColour(i) == c)
                return FmtString(FormatStringID(Windows::ColourDropDownIndexToName(i))).WithoutFormatTokens();
        return "Special colour";
    }
    std::vector<std::string> Options()
    {
        const auto ch = Channels(current.category, current.object);
        return { "Resume",
                 "Choose object...",
                 current.category == Category::additions ? "Orientation: path edges"
                                                         : "Direction / edge: " + std::to_string(current.direction + 1),
                 current.category == Category::additions ? "Quadrant: not applicable"
                                                         : "Quadrant: " + std::to_string(current.quadrant + 1),
                 "Raise placement height",
                 "Lower placement height",
                 current.height ? "Automatic height" : "Height: automatic",
                 ch[0] ? "Primary: " + ColourName(current.colours[0]) : "Primary: unavailable",
                 ch[1] ? "Secondary: " + ColourName(current.colours[1]) : "Secondary: unavailable",
                 ch[2] ? "Tertiary: " + ColourName(current.colours[2]) : "Tertiary: unavailable",
                 current.mode == Mode::place          ? "Mode: Place"
                     : current.mode == Mode::recolour ? "Mode: Recolour"
                                                      : "Mode: Remove",
                 "Edit sign text..." };
    }
    void Configure(size_t row, int delta)
    {
        ClearPreview();
        if ((row == 4 || row == 5) && current.height == 0 && lastTile)
            if (const auto* surface = MapGetSurfaceElementAt(*lastTile))
                current.height = surface->getBaseZ();
        if (row == 2 && current.category != Category::additions)
            current.direction = (current.direction + delta + 4) % 4;
        if (row == 3 && current.category != Category::additions)
            current.quadrant = (current.quadrant + delta + 4) % 4;
        if (row == 4)
            current.height = std::min(2040, current.height + 8);
        if (row == 5)
            current.height = std::max(0, current.height - 8);
        if (row == 6)
            current.height = 0;
        if (row >= 7 && row <= 9 && Channels(current.category, current.object)[row - 7])
            current.colours[row - 7] = static_cast<Drawing::Colour>(
                (static_cast<int>(current.colours[row - 7]) + delta + 72) % 72);
        if (row == 10)
            current.mode = static_cast<Mode>((static_cast<int>(current.mode) + delta + 3) % 3);
    }
    static std::unique_ptr<GameActions::GameAction> Placement(CoordsXY tile, const Settings& s)
    {
        auto z = s.height;
        if ((s.category == Category::banners || s.category == Category::additions) && z == 0)
            for (const auto* path : TileElementsView<PathElement>(tile))
                if (!path->isGhost())
                {
                    z = path->getBaseZ();
                    if (s.category == Category::banners && path->isSloped() && path->getSlopeDirection() == s.direction)
                        z += kPathHeightStep;
                    break;
                }
        const CoordsXYZD loc{ tile, z, s.direction };
        const auto& c = s.colours;
        switch (s.category)
        {
            case Category::additions:
                return std::make_unique<GameActions::FootpathAdditionPlaceAction>(loc, s.object);
            case Category::small:
                return std::make_unique<GameActions::SmallSceneryPlaceAction>(loc, s.quadrant, s.object, c[0], c[1], c[2]);
            case Category::large:
                return std::make_unique<GameActions::LargeSceneryPlaceAction>(loc, s.object, c[0], c[1], c[2]);
            case Category::walls:
                return std::make_unique<GameActions::WallPlaceAction>(s.object, loc, s.direction, c[0], c[1], c[2]);
            case Category::banners:
                return std::make_unique<GameActions::BannerPlaceAction>(loc, s.object, c[0]);
        }
        return {};
    }
    static std::unique_ptr<GameActions::GameAction> Removal(const Target& t)
    {
        switch (t.category)
        {
            case Category::additions:
                return std::make_unique<GameActions::FootpathAdditionRemoveAction>(t.location);
            case Category::small:
                return std::make_unique<GameActions::SmallSceneryRemoveAction>(t.location, t.part, t.object);
            case Category::large:
                return std::make_unique<GameActions::LargeSceneryRemoveAction>(t.location, t.part);
            case Category::walls:
                return std::make_unique<GameActions::WallRemoveAction>(t.location);
            case Category::banners:
                return std::make_unique<GameActions::BannerRemoveAction>(CoordsXYZD{ t.location, t.part });
        }
        return {};
    }
    static TileElement* Find(const Target& t, bool isGhost)
    {
        if (!MapIsLocationValid(t.location))
            return nullptr;
        if (t.category == Category::additions)
        {
            auto* path = MapGetFootpathElement(t.location);
            return path && !path->isGhost() && path->hasAddition() && path->additionIsGhost() == isGhost
                    && path->getAdditionEntryIndex() == t.object
                ? reinterpret_cast<TileElement*>(path)
                : nullptr;
        }
        for (auto* e : TileElementsView<TileElement>(t.location))
        {
            if (e->getBaseZ() != t.location.z || e->isGhost() != isGhost)
                continue;
            if (t.category == Category::small && e->getType() == TileElementType::smallScenery
                && e->asSmallScenery()->getSceneryQuadrant() == t.part && e->asSmallScenery()->getEntryIndex() == t.object)
                return e;
            if (t.category == Category::large && e->getType() == TileElementType::largeScenery
                && e->getDirection() == t.location.direction && e->asLargeScenery()->getSequenceIndex() == t.part
                && e->asLargeScenery()->getEntryIndex() == t.object)
                return e;
            if (t.category == Category::walls && e->getType() == TileElementType::wall
                && e->getDirection() == t.location.direction && e->asWall()->getEntryIndex() == t.object)
                return e;
            if (t.category == Category::banners && e->getType() == TileElementType::banner
                && e->asBanner()->getPosition() == t.part && e->asBanner()->getIndex() == t.banner)
                return e;
        }
        return nullptr;
    }
    std::optional<Target> Capture(const InteractionInfo& info)
    {
        const auto* e = info.Element;
        if (!e || e->isGhost())
            return {};
        Target t{};
        t.location = { info.Loc.x & ~31, info.Loc.y & ~31, e->getBaseZ(), e->getDirection() };
        if (e->getType() == TileElementType::smallScenery)
        {
            t.category = Category::small;
            const auto* s = e->asSmallScenery();
            t.object = s->getEntryIndex();
            t.part = s->getSceneryQuadrant();
            t.colours = { s->getPrimaryColour(), s->getSecondaryColour(), s->getTertiaryColour() };
        }
        else if (e->getType() == TileElementType::largeScenery)
        {
            t.category = Category::large;
            const auto* s = e->asLargeScenery();
            t.object = s->getEntryIndex();
            t.part = s->getSequenceIndex();
            t.banner = s->getBannerIndex();
            t.colours = { s->getPrimaryColour(), s->getSecondaryColour(), s->getTertiaryColour() };
        }
        else if (e->getType() == TileElementType::wall)
        {
            t.category = Category::walls;
            const auto* s = e->asWall();
            t.object = s->getEntryIndex();
            t.banner = s->getBannerIndex();
            t.colours = { s->getPrimaryColour(), s->getSecondaryColour(), s->getTertiaryColour() };
        }
        else if (e->getType() == TileElementType::path)
        {
            const auto* path = e->asPath();
            if (!path->hasAddition() || path->additionIsGhost())
                return {};
            t.category = Category::additions;
            t.object = path->getAdditionEntryIndex();
        }
        else if (e->getType() == TileElementType::banner)
        {
            t.category = Category::banners;
            const auto* s = e->asBanner();
            const auto* b = s->getBanner();
            if (!b)
                return {};
            t.object = b->type;
            t.part = s->getPosition();
            t.banner = s->getIndex();
            t.colours[0] = b->colour;
        }
        else
            return {};
        if (const auto* banner = GetBanner(t.banner))
            t.text = banner->text;
        // Reject ambiguous native identities: normal small removal can match ghosts.
        if (Find(t, false) != e || Find(t, true))
            return {};
        return t;
    }
    GameActions::Result Modify(const Target& t, Mode mode, const std::array<Drawing::Colour, 3>& proposed, bool execute)
    {
        auto* e = Find(t, false);
        if (!e || Find(t, true))
            return Invalid("Scenery target changed");
        InteractionInfo hit{};
        hit.Loc = t.location;
        hit.Element = e;
        const auto fresh = Capture(hit);
        if (!fresh || fresh->colours != t.colours)
            return Invalid("Scenery colours changed");
        if (mode == Mode::remove)
        {
            auto a = Removal(t);
            return execute ? GameActions::Execute(a.get(), getGameState()) : GameActions::Query(a.get(), getGameState());
        }
        auto c = t.colours;
        const auto ch = Channels(t.category, t.object);
        for (size_t i = 0; i < 3; ++i)
            if (ch[i])
                c[i] = proposed[i];
        if (c == t.colours)
            return Invalid("No supported colour change");
        std::unique_ptr<GameActions::GameAction> a;
        switch (t.category)
        {
            case Category::additions:
                return Invalid("Path furniture has no colour channels");
            case Category::small:
                a = std::make_unique<GameActions::SmallScenerySetColourAction>(t.location, t.part, t.object, c[0], c[1], c[2]);
                break;
            case Category::large:
                a = std::make_unique<GameActions::LargeScenerySetColourAction>(t.location, t.part, c[0], c[1], c[2]);
                break;
            case Category::walls:
                a = std::make_unique<GameActions::WallSetColourAction>(t.location, c[0], c[1], c[2]);
                break;
            case Category::banners:
                a = std::make_unique<GameActions::BannerSetColourAction>(CoordsXYZD{ t.location, t.part }, c[0]);
                break;
        }
        return execute ? GameActions::Execute(a.get(), getGameState()) : GameActions::Query(a.get(), getGameState());
    }
    GameActions::Result SetText(const Target& t, const std::string& text, bool execute)
    {
        const auto* banner = GetBanner(t.banner);
        if (!Find(t, false) || Find(t, true) || !banner || banner->text != t.text || text == t.text
            || TextEntry{ text }.Length() > TextEntry::limit)
            return Invalid("Sign changed or text unavailable");
        std::unique_ptr<GameActions::GameAction> action;
        if (t.category == Category::banners)
            action = std::make_unique<GameActions::BannerSetNameAction>(t.banner, text);
        else
            action = std::make_unique<GameActions::SignSetNameAction>(t.banner, text);
        return execute ? GameActions::Execute(action.get(), getGameState()) : GameActions::Query(action.get(), getGameState());
    }
    static void RemoveGhost()
    {
        if (!ghost)
            return;
        const auto t = *ghost;
        ghost.reset();
        if (!Find(t, true))
            return;
        auto a = Removal(t);
        a->SetFlags(
            { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
              GameActions::CommandFlag::allowDuringPaused });
        GameActions::Execute(a.get(), getGameState());
    }
    void ClearPreview()
    {
        requested.reset();
        RemoveGhost();
    }
    static GameActions::Result Query(std::optional<CoordsXY> tile, const Settings& s)
    {
        const auto max = GetMapSizeMaxXY();
        if (!tile || tile->x < 32 || tile->y < 32 || tile->x > max.x || tile->y > max.y || tile->x % 32 || tile->y % 32
            || !MapGetSurfaceElementAt(*tile))
            return Invalid("Move onto valid land");
        if (!Eligible(s.category, s.object))
            return Invalid("Scenery unavailable / unresearched");
        if (s.direction > 3 || s.quadrant > 3 || s.height < 0 || s.height > 2040 || s.height % 8)
            return Invalid("Invalid scenery geometry");
        auto a = Placement(*tile, s);
        return GameActions::Query(a.get(), getGameState());
    }
    static void CreateGhost(const Request& request)
    {
        auto a = Placement(request.tile, request.settings);
        a->SetFlags(
            { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
              GameActions::CommandFlag::allowDuringPaused });
        const auto r = GameActions::Execute(a.get(), getGameState());
        if (r.error != GameActions::Status::ok)
            return;
        Target t{};
        t.category = request.settings.category;
        t.object = request.settings.object;
        t.location = { request.tile, 0, request.settings.direction };
        t.part = request.settings.quadrant;
        if (t.category == Category::additions)
            t.location = { request.tile, r.position.z, 0 };
        if (t.category == Category::small)
        {
            const auto d = r.getData<GameActions::SmallSceneryPlaceActionResult>();
            t.location.z = d.BaseHeight;
            t.part = d.SceneryQuadrant;
        }
        if (t.category == Category::large)
        {
            t.location.z = r.getData<GameActions::LargeSceneryPlaceActionResult>().firstTileHeight;
            t.part = 0;
        }
        if (t.category == Category::walls)
            t.location.z = r.getData<GameActions::WallPlaceActionResult>().BaseHeight;
        if (t.category == Category::banners)
        {
            t.banner = r.getData<GameActions::BannerPlaceActionResult>().bannerId;
            t.part = request.settings.direction;
            for (const auto* b : TileElementsView<BannerElement>(request.tile))
                if (b->getIndex() == t.banner)
                    t.location.z = b->getBaseZ();
        }
        ghost = t;
    }
    GameActions::Result Place(std::optional<CoordsXY> tile, const Settings& s, bool execute)
    {
        const auto old = requested;
        RemoveGhost();
        const auto r = Query(tile, s);
        if (!execute)
        {
            if (old && suspended == 0 && r.error == GameActions::Status::ok)
                CreateGhost(*old);
            return r;
        }
        requested.reset();
        if (r.error != GameActions::Status::ok)
            return r;
        auto a = Placement(*tile, s);
        return GameActions::Execute(a.get(), getGameState());
    }
    void UpdatePreview(std::optional<CoordsXY> tile)
    {
        lastTile = tile;
        if (current.mode != Mode::place)
        {
            ClearPreview();
            return;
        }
        // Remove own preview before ordinary query, important for banner occupancy.
        RemoveGhost();
        const auto r = Query(tile, current);
        if (r.error != GameActions::Status::ok)
        {
            requested.reset();
            return;
        }
        requested = Request{ *tile, current };
        if (!suspended)
            CreateGhost(*requested);
    }
    void RemovePreviewTemporarily()
    {
        ++suspended;
        RemoveGhost();
    }
    void RestorePreview()
    {
        if (!suspended || --suspended || !requested)
            return;
        const auto r = *requested;
        if (Query(r.tile, r.settings).error == GameActions::Status::ok)
            CreateGhost(r);
        else
            requested.reset();
    }
} // namespace OpenRCT2::Ui::Handheld::SceneryTools

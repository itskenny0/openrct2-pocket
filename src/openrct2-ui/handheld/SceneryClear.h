// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "AreaTools.h"
#include "ConstructionPanel.h"
#include "ManagementPanel.h"
#include "SceneryTools.h"

#include <cstring>
#include <openrct2/actions/terraform/ClearAction.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/LargeSceneryElement.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <set>
namespace OpenRCT2::Ui::Handheld::SceneryClear
{
    struct Settings
    {
        int width = 1, height = 1;
        GameActions::ClearableItems items{ GameActions::ClearableItem::smallScenery, GameActions::ClearableItem::largeScenery,
                                           GameActions::ClearableItem::walls };
    };
    inline Settings settings;
    inline constexpr std::array<const char*, 5> names{ "Small scenery", "Large scenery", "Paths + banners", "Walls",
                                                       "Path furniture" };
    inline std::vector<std::string> Options()
    {
        std::vector<std::string> rows{ "Resume", "Width: " + std::to_string(settings.width),
                                       "Height: " + std::to_string(settings.height) };
        for (size_t i = 0; i < names.size(); ++i)
            rows.push_back(
                std::string(names[i]) + (settings.items.has(static_cast<GameActions::ClearableItem>(i)) ? ": On" : ": Off"));
        return rows;
    }
    inline void Configure(size_t row, int delta = 1)
    {
        if (row == 1)
            settings.width = std::clamp(settings.width + delta, 1, 64);
        if (row == 2)
            settings.height = std::clamp(settings.height + delta, 1, 64);
        if (row >= 3 && row < 8)
            settings.items.flip(static_cast<GameActions::ClearableItem>(row - 3));
    }
    inline void DrawOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY origin)
    {
        DrawConstructionOverlay(
            rt, origin, "Clear area", "Native categories", "Review first",
            std::to_string(settings.width) + " x " + std::to_string(settings.height) + " tiles",
            "Whole objects / native skips");
    }
    struct Snapshot
    {
        std::vector<std::array<uint8_t, sizeof(TileElement)>> elements;
        std::vector<SceneryTools::Target> scenery;
        size_t targets{};
        bool operator==(const Snapshot&) const = default;
    };
    inline Snapshot Capture(const MapRange& range, const Settings& s)
    {
        std::set<std::pair<int, int>> tiles;
        for (int y = range.getY1(); y <= range.getY2(); y += 32)
            for (int x = range.getX1(); x <= range.getX2(); x += 32)
            {
                tiles.emplace(x, y);
                if (!s.items.has(GameActions::ClearableItem::largeScenery))
                    continue;
                for (const auto* large : TileElementsView<LargeSceneryElement>(CoordsXY{ x, y }))
                {
                    if (large->isGhost() || !large->getEntry())
                        continue;
                    const auto origin = MapLargeSceneryGetOrigin(
                        { x, y, large->getBaseZ(), large->getDirection() }, large->getSequenceIndex(), nullptr);
                    if (!origin)
                        continue;
                    for (const auto& part : large->getEntry()->tiles)
                    {
                        const auto point = CoordsXY(*origin) + CoordsXY(part.offset).rotate(large->getDirection());
                        tiles.emplace(point.x, point.y);
                    }
                }
            }
        Snapshot result;
        for (const auto& [x, y] : tiles)
        {
            if (!MapIsLocationValid(CoordsXY{ x, y }))
                continue;
            for (auto* e : TileElementsView<>(CoordsXY{ x, y }))
            {
                if (e->isGhost())
                    continue;
                std::array<uint8_t, sizeof(TileElement)> copy{};
                std::memcpy(copy.data(), e, sizeof(TileElement));
                result.elements.push_back(copy);
                InteractionInfo info{};
                info.Loc = { x, y };
                info.Element = e;
                if (const auto t = SceneryTools::Capture(info))
                    result.scenery.push_back(*t);
                const bool selected = e->getType() == TileElementType::smallScenery
                    ? s.items.has(GameActions::ClearableItem::smallScenery)
                    : e->getType() == TileElementType::largeScenery ? s.items.has(GameActions::ClearableItem::largeScenery)
                    : e->getType() == TileElementType::wall         ? s.items.has(GameActions::ClearableItem::walls)
                    : e->getType() == TileElementType::path
                    ? (s.items.has(GameActions::ClearableItem::footpaths)
                       || (s.items.has(GameActions::ClearableItem::pathAdditions) && e->asPath()->hasAddition()
                           && !e->asPath()->additionIsGhost()))
                    : false;
                if (selected && x >= range.getX1() && x <= range.getX2() && y >= range.getY1() && y <= range.getY2())
                    ++result.targets;
            }
        }
        return result;
    }
    inline GameActions::Result Query(std::optional<CoordsXY> tile, const Settings& s)
    {
        const auto range = AreaTools::Range(tile, s.width, s.height);
        if (!range || s.items.isEmpty() || (s.items.holder & ~31))
            return AreaTools::Invalid("Choose valid area and categories");
        if (!Capture(*range, s).targets)
            return AreaTools::Invalid("No selected elements in area");
        GameActions::ClearAction action(*range, s.items);
        return GameActions::Query(&action, getGameState());
    }
    struct Draft
    {
        CoordsXY tile;
        Settings settings;
        Snapshot baseline;
        money64 cost, cash;
        uint64_t parkFlags;
        bool consumed{};
    };
    inline std::optional<Draft> Begin(std::optional<CoordsXY> tile, const Settings& s)
    {
        const auto q = Query(tile, s);
        const auto range = AreaTools::Range(tile, s.width, s.height);
        if (!range || q.error != GameActions::Status::ok)
            return {};
        const auto& park = getGameState().park;
        return Draft{ *tile, s, Capture(*range, s), q.cost, park.cash, park.flags.holder, false };
    }
    inline GameActions::Result Action(Draft& d, bool execute)
    {
        const auto range = AreaTools::Range(d.tile, d.settings.width, d.settings.height);
        const auto& park = getGameState().park;
        if (d.consumed || !range || Capture(*range, d.settings) != d.baseline || park.cash != d.cash
            || park.flags.holder != d.parkFlags)
            return AreaTools::Invalid("Area or native baseline changed; reopen review");
        const auto q = Query(d.tile, d.settings);
        if (q.error != GameActions::Status::ok)
            return q;
        if (q.cost != d.cost)
            return AreaTools::Invalid("Clear cost changed; reopen review");
        if (!execute)
            return q;
        GameActions::ClearAction action(*range, d.settings.items);
        const auto result = GameActions::Execute(&action, getGameState());
        if (result.error == GameActions::Status::ok)
            d.consumed = true;
        return result;
    }
    inline ManagementPresentation Presentation(const Draft& d, size_t selected, const std::string& status = {})
    {
        ManagementPresentation model;
        model.breadcrumb = "Build / Clear area";
        model.title = "Clear selected categories?";
        model.confirmation = true;
        model.selected = selected;
        model.rows = { "Clear native area", "Cancel" };
        model.stats = { { "Area", std::to_string(d.settings.width) + " x " + std::to_string(d.settings.height) },
                        { "Elements", std::to_string(d.baseline.targets) },
                        { "Cost / refund", FormatMoney(d.cost) } };
        model.status = status.empty() ? "Whole objects; native skips" : status;
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::SceneryClear

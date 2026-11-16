// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "AreaTools.h"

#include <algorithm>
#include <openrct2/GameState.h>
#include <openrct2/actions/terraform/LandLowerAction.h>
#include <openrct2/actions/terraform/LandRaiseAction.h>
#include <openrct2/actions/terraform/LandSetHeightAction.h>
#include <openrct2/actions/terraform/LandSmoothAction.h>
#include <openrct2/actions/terraform/WaterSetHeightAction.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/tile_element/Slope.h>
#include <openrct2/world/tile_element/SurfaceElement.h>

// Quick landscaping, named for what the player wants rather than which corner moves. RCT's own tools
// raise or lower a tile's corners; a newcomer wants a hill, a hole, a pond or flat ground. Each shape
// maps onto the native terraform actions (so costs, ownership and undo stay exact) with a square
// brush the player sizes with L2/R2. Corner and edge control stays in Build > Landscape.
namespace OpenRCT2::Ui::Handheld::Landscape
{
    enum class Shape
    {
        hill,
        dig,
        pond,
        flatten
    };
    inline constexpr std::array kShapes{ Shape::hill, Shape::dig, Shape::pond, Shape::flatten };
    inline Shape shape{ Shape::hill };
    inline int brush{ 1 }; // Side length in tiles: 1, 3 or 5.

    inline const char* Name(Shape s)
    {
        switch (s)
        {
            case Shape::hill: return "Raise hill";
            case Shape::dig: return "Dig";
            case Shape::pond: return "Pond";
            case Shape::flatten: return "Flatten";
        }
        return "";
    }
    inline const char* Help(Shape s)
    {
        switch (s)
        {
            case Shape::hill: return "Lift the ground with gentle slopes";
            case Shape::dig: return "Lower the ground with gentle slopes";
            case Shape::pond: return "Dig down and fill with water";
            case Shape::flatten: return "Level the brush to the middle tile";
        }
        return "";
    }
    inline void Cycle(int direction)
    {
        const int index = static_cast<int>(shape);
        shape = kShapes[(index + (direction > 0 ? 1 : 3)) % 4];
    }
    inline void Resize(int direction)
    {
        brush = std::clamp(brush + (direction > 0 ? 2 : -2), 1, 5);
    }
    inline std::optional<MapRange> Range(std::optional<CoordsXY> tile)
    {
        return AreaTools::Range(tile, brush, brush);
    }

    namespace Detail
    {
        inline GameActions::Result Run(GameActions::GameAction& action, bool execute)
        {
            return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
        }
        template<typename F> inline void ForTiles(const MapRange& range, F&& f)
        {
            for (int32_t y = range.getY1(); y <= range.getY2(); y += kCoordsXYStep)
                for (int32_t x = range.getX1(); x <= range.getX2(); x += kCoordsXYStep)
                    if (auto* surface = MapGetSurfaceElementAt(CoordsXY{ x, y }))
                        f(CoordsXY{ x, y }, *surface);
        }
    } // namespace Detail

    inline GameActions::Result Action(std::optional<CoordsXY> tile, bool execute)
    {
        const auto range = Range(tile);
        if (!range)
            return AreaTools::Invalid(tile ? "Brush is past the edge of the map." : "Move onto land.");
        const CoordsXY centre{ tile->x + 16, tile->y + 16 };
        switch (shape)
        {
            case Shape::hill:
            {
                GameActions::LandSmoothAction action(centre, *range, MapSelectType::full, false);
                return Detail::Run(action, execute);
            }
            case Shape::dig:
            {
                GameActions::LandSmoothAction action(centre, *range, MapSelectType::full, true);
                return Detail::Run(action, execute);
            }
            case Shape::flatten:
            {
                // Every tile in the brush is set flat at the middle tile's height in one press. Tiles
                // the game refuses (a path or ride in the way) are skipped rather than failing the lot;
                // the press only fails if nothing could change.
                const auto* middle = MapGetSurfaceElementAt(*tile);
                if (middle == nullptr)
                    return AreaTools::Invalid("Move onto land.");
                const auto height = static_cast<uint8_t>(middle->getBaseZ() / kCoordsZStep);
                const int target = middle->getBaseZ();
                GameActions::Result total;
                total.error = GameActions::Status::invalidParameters;
                total.errorMessage = "Already flat.";
                bool changed{};
                std::vector<CoordsXY> tiles;
                Detail::ForTiles(*range, [&](CoordsXY loc, const SurfaceElement& s) {
                    if (s.getBaseZ() != target || s.getSlope() != OpenRCT2::kTileSlopeFlat)
                        tiles.push_back(loc);
                });
                for (const auto& loc : tiles)
                {
                    GameActions::LandSetHeightAction action(loc, height, OpenRCT2::kTileSlopeFlat);
                    const auto result = Detail::Run(action, execute);
                    if (result.error == GameActions::Status::ok)
                    {
                        if (!changed)
                            total = result;
                        else
                            total.cost += result.cost;
                        changed = true;
                    }
                    else if (!changed)
                        total = result;
                }
                return total;
            }
            case Shape::pond:
            {
                // The water line is the existing pond's surface if there is one (so pressing again
                // digs deeper without raising the water), otherwise the lowest ground in the brush (so
                // water never spills above the bank). Then the ground drops one step under it.
                int level = 0, lowest = INT32_MAX;
                Detail::ForTiles(*range, [&](CoordsXY, const SurfaceElement& s) {
                    level = std::max<int>(level, s.getWaterHeight());
                    lowest = std::min<int>(lowest, s.getBaseZ());
                });
                if (lowest == INT32_MAX)
                    return AreaTools::Invalid("Move onto land.");
                if (level == 0)
                    level = lowest;
                GameActions::LandLowerAction dig(centre, *range, MapSelectType::full);
                if (execute)
                {
                    // Fill only once the dig has really happened: actions can be queued (paused game,
                    // replays), so the water goes in from the dig's own completion callback.
                    const auto area = *range;
                    dig.SetCallback([area, level](const GameActions::GameAction*, const GameActions::Result* done) {
                        if (done->error != GameActions::Status::ok)
                            return;
                        Detail::ForTiles(area, [level](CoordsXY loc, const SurfaceElement& s) {
                            if (s.getBaseZ() >= level)
                                return;
                            GameActions::WaterSetHeightAction fill(loc, static_cast<uint8_t>(level / kCoordsZStep));
                            GameActions::Execute(&fill, getGameState());
                        });
                    });
                }
                auto result = Detail::Run(dig, execute);
                return result;
            }
        }
        return AreaTools::Invalid("Unknown shape.");
    }
} // namespace OpenRCT2::Ui::Handheld::Landscape

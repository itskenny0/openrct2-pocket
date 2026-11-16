// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/world/Location.hpp>
#include <algorithm>
#include <string>
#include <vector>

namespace OpenRCT2::Ui::Handheld::Paths
{
    enum class BuildMode { single, line, fill };
    constexpr size_t kSelectionLimit = 64;
    struct Selection
    {
        std::vector<CoordsXYZ> tiles;
        int width{}, height{};
        std::string error;
    };
    // Selection only: all path geometry, eligibility and costs come from native actions.
    inline Selection SelectTiles(BuildMode mode, CoordsXYZ start, CoordsXY end)
    {
        Selection result;
        result.width = std::abs(end.x - start.x) / kCoordsXYStep + 1;
        result.height = std::abs(end.y - start.y) / kCoordsXYStep + 1;
        if (mode == BuildMode::line && start.x != end.x && start.y != end.y)
            result.error = "Align with start: straight lines only";
        else if (result.width * result.height > static_cast<int>(kSelectionLimit))
            result.error = "Maximum 64 tiles; choose a smaller area";
        else
            for (int y = std::min(start.y, end.y); y <= std::max(start.y, end.y); y += kCoordsXYStep)
                for (int x = std::min(start.x, end.x); x <= std::max(start.x, end.x); x += kCoordsXYStep)
                    result.tiles.emplace_back(x, y, start.z);
        return result;
    }
}

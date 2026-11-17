// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../object/SmallSceneryEntry.h"
#include "../util/Util.h"
#include "Location.hpp"
#include "Scenery.h"

#include <utility>
namespace OpenRCT2
{
    inline int32_t SceneryScatterQuantity(uint16_t size, ScatterToolDensity density)
    {
        return size * (static_cast<int32_t>(density) + 1);
    }
    // Shared native desktop/controller proposals; placement actions own all gameplay rules.
    inline CoordsXY SceneryScatterProposal(
        CoordsXY centre, uint16_t size, const SmallSceneryEntry& entry, uint8_t& quadrant, Direction& direction)
    {
        if (!entry.flags.has(SmallSceneryFlag::occupiesFullTile))
            quadrant = UtilRand() & 3;
        auto x = static_cast<int32_t>(UtilRand() % size) - size / 2;
        auto y = static_cast<int32_t>(UtilRand() % size) - size / 2;
        if (size % 2 == 0)
        {
            ++x;
            ++y;
        }
        if (!entry.flags.has(SmallSceneryFlag::isRotatable))
            direction = (direction + 1) & 3;
        return { centre.x + x * kCoordsXYStep, centre.y + y * kCoordsXYStep };
    }
    inline std::pair<CoordsXY, CoordsXY> SceneryScatterBounds(CoordsXY centre, uint16_t size)
    {
        const auto half = (size - 1) * kCoordsXYStep / 2;
        const auto extra = size % 2 == 0 ? kCoordsXYStep : 0;
        return { { centre.x - half, centre.y - half }, { centre.x + half + extra, centre.y + half + extra } };
    }
} // namespace OpenRCT2

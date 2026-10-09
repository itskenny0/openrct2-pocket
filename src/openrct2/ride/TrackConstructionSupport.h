// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../GameState.h"
#include "../management/Research.h"
#include "RideData.h"
#include "RideEntry.h"

#include <algorithm>
namespace OpenRCT2::TrackConstructionSupport
{
    // Ordinary TED track geometry with regular station endpoints. Maze/flat
    // footprints use distinct native adapters, never this sequential editor.
    inline bool Type(ride_type_t type)
    {
        if (type == kRideTypeNull || type >= RIDE_TYPE_COUNT)
            return false;
        const auto& d = GetRideTypeDescriptor(type);
        return d.flags.has(RtdFlag::hasTrack) && !d.flags.hasAny(RtdFlag::isShopOrFacility, RtdFlag::hasSinglePieceStation)
            && d.StartTrackPiece == TrackElemType::endStation && d.SupportsTrackGroup(TrackGroup::stationEnd);
    }
    inline bool Piece(TrackElemType type)
    {
        return type < TrackElemType::count && type != TrackElemType::maze
            && !(type >= TrackElemType::flatTrack1x4A && type <= TrackElemType::flatTrack3x3);
    }
    inline bool Available(ObjectEntryIndex index, ride_type_t type)
    {
        const auto* entry = GetRideEntryByIndex(index);
        return Type(type) && entry
            && std::find(std::begin(entry->ride_type), std::end(entry->ride_type), type) != std::end(entry->ride_type)
            && (getGameState().cheats.ignoreResearchStatus || (RideTypeIsInvented(type) && RideEntryIsInvented(index)));
    }
    inline ride_type_t ForObject(ObjectEntryIndex index, ride_type_t requested = kRideTypeNull)
    {
        if (requested != kRideTypeNull)
            return Available(index, requested) ? requested : kRideTypeNull;
        if (const auto* entry = GetRideEntryByIndex(index))
            for (auto type : entry->ride_type)
                if (Available(index, type))
                    return type;
        return kRideTypeNull;
    }
} // namespace OpenRCT2::TrackConstructionSupport

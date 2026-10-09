// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../GameState.h"
#include "../management/Research.h"
#include "../world/tile_element/TileElement.h"
#include "../world/tile_element/TrackElement.h"
#include "RideData.h"
#include "RideEntry.h"
#include "Track.h"
#include "TrackData.h"
#include "TrackIteration.h"
#include "ted/TrackElementDescriptor.h"

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
    // Desktop TD6 sharing rules: ordinary speed pieces share their seat bits.
    inline bool SeatRotation(ride_type_t ride, TrackElemType piece)
    {
        return Type(ride) && Piece(piece) && GetRideTypeDescriptor(ride).flags.has(RtdFlag::hasSeatRotation)
            && (!trackTypeHasSpeedSetting(piece) || piece == TrackElemType::blockBrakes || piece > TrackElemType::highestAlias);
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
    struct BlockSnapshot
    {
        CoordsXYZD origin{};
        TrackElemType type{};
        uint8_t speed{}, seat{};
        bool chain{}, inverted{};
        bool operator==(const BlockSnapshot&) const = default;
    };
    inline std::optional<BlockSnapshot> Previous(const CoordsXYE& input)
    {
        TrackBeginEnd previous{};
        if (!trackBlockGetPrevious(input, &previous) || !previous.begin_element
            || !TrackIsConnectedByShape(previous.begin_element, input.element))
            return {};
        const auto* element = previous.begin_element->asTrack();
        if (!element || element->isGhost() || !Piece(element->getTrackType()))
            return {};
        const auto& ted = TrackMetadata::GetTrackElementDescriptor(element->getTrackType());
        if (element->getSequenceIndex() >= ted.sequenceData.numSequences)
            return {};
        const auto& clearance = ted.sequenceData.sequences[element->getSequenceIndex()].clearance;
        const auto xy = CoordsXY{ previous.end_x, previous.end_y }
            - CoordsXY{ clearance.x, clearance.y }.rotate(element->getDirection());
        return BlockSnapshot{ { xy, element->getBaseZ() - clearance.z, element->getDirection() },
                              element->getTrackType(),
                              element->getBrakeBoosterSpeed(),
                              element->getSeatRotation(),
                              element->hasChain(),
                              element->isInverted() };
    }
} // namespace OpenRCT2::TrackConstructionSupport

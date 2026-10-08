// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../Context.h"
#include "../object/ObjectLimits.h"
#include "../object/ObjectManager.h"
#include "../world/TileElementsView.h"
#include "../world/tile_element/TrackElement.h"
#include "ShopPlacement.h"
#include "TrackData.h"
#include "ted/TrackElementDescriptor.h"

#include <optional>
namespace OpenRCT2::FlatRidePlacement
{
    inline ride_type_t Type(ObjectEntryIndex index)
    {
        const auto* entry = GetRideEntryByIndex(index);
        if (!entry)
            return kRideTypeNull;
        for (auto type : entry->ride_type)
        {
            if (type == kRideTypeNull || type >= RIDE_TYPE_COUNT)
                continue;
            const auto& rtd = GetRideTypeDescriptor(type);
            if (!rtd.flags.has(RtdFlag::isFlatRide) || !rtd.flags.has(RtdFlag::hasSinglePieceStation)
                || rtd.flags.hasAny(RtdFlag::isShopOrFacility, RtdFlag::hasTrack))
                continue;
            switch (rtd.StartTrackPiece)
            {
                case TrackElemType::flatTrack1x4A:
                case TrackElemType::flatTrack2x2:
                case TrackElemType::flatTrack4x4:
                case TrackElemType::flatTrack2x4:
                case TrackElemType::flatTrack1x5:
                case TrackElemType::flatTrack1x1A:
                case TrackElemType::flatTrack1x4B:
                case TrackElemType::flatTrack1x1B:
                case TrackElemType::flatTrack1x4C:
                case TrackElemType::flatTrack3x3:
                    return type;
                default:
                    break;
            }
        }
        return kRideTypeNull;
    }
    inline ObjectEntryIndex StationObject()
    {
        auto& objects = GetContext()->GetObjectManager();
        for (ObjectEntryIndex i = 0; i < kMaxStationObjects; ++i)
            if (objects.GetLoadedObject(ObjectType::station, i))
                return i;
        return kObjectEntryIndexNull;
    }
    inline bool Eligible(ObjectEntryIndex index)
    {
        const auto type = Type(index);
        return type != kRideTypeNull
            && (!GetRideTypeDescriptor(type).flags.has(RtdFlag::hasEntranceAndExit) || StationObject() != kObjectEntryIndexNull)
            && (getGameState().cheats.ignoreResearchStatus || (RideTypeIsInvented(type) && RideEntryIsInvented(index)));
    }
    // Shared regular-station recovery/access support; construction eligibility stays flat-only.
    inline bool SupportsStation(const Ride& ride)
    {
        if (Type(ride.subtype) == ride.type)
            return true;
        if (ride.type != RIDE_TYPE_WOODEN_ROLLER_COASTER)
            return false;
        size_t count = 0;
        for (const auto& station : ride.getStations())
            count += !station.start.isNull();
        return count > 0 && !ride.getStation(StationIndex::FromUnderlying(0)).start.isNull();
    }
    // Native station sequence-side semantics; never allow a disconnected entrance.
    inline std::optional<CoordsXYZD> EntranceLocation(
        const Ride& ride, CoordsXY tile, StationIndex index = StationIndex::FromUnderlying(0))
    {
        if (!MapIsLocationValid(tile) || tile.x % 32 || tile.y % 32 || index.ToUnderlying() >= ride.getStations().size())
            return {};
        const auto& station = ride.getStation(index);
        if (ride.status != RideStatus::closed || station.start.isNull())
            return {};
        for (Direction d = 0; d < 4; ++d)
            for (auto* track : TileElementsView<TrackElement>(tile + CoordsDirectionDelta[d]))
            {
                if (track->isGhost() || track->getRideIndex() != ride.id || track->getBaseZ() != station.getBaseZ()
                    || track->getStationIndex() != index)
                    continue;
                const auto& ted = TrackMetadata::GetTrackElementDescriptor(track->getTrackType());
                const auto sequence = track->getSequenceIndex();
                if (sequence >= ted.sequenceData.numSequences)
                    continue;
                const Direction side = (d - DirectionReverse(track->getDirection())) & 3;
                if (ted.sequenceData.sequences[sequence].getEntranceConnectionSides() & (1 << side))
                    return CoordsXYZD{ tile, station.getBaseZ(), d };
            }
        return {};
    }
} // namespace OpenRCT2::FlatRidePlacement

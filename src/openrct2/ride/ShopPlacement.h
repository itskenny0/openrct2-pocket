// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../GameState.h"
#include "../management/Research.h"
#include "../object/RideObject.h"
#include "../world/Map.h"
#include "Ride.h"
#include "RideData.h"
#include "ted/TrackElemType.h"

namespace OpenRCT2::ShopPlacement
{
    inline ride_type_t Type(ObjectEntryIndex index)
    {
        const auto* entry = GetRideEntryByIndex(index);
        if (entry)
            for (auto type : entry->ride_type)
                if (type != kRideTypeNull && type < RIDE_TYPE_COUNT)
                {
                    const auto& rtd = GetRideTypeDescriptor(type);
                    if (rtd.flags.has(RtdFlag::isShopOrFacility)
                        && (rtd.StartTrackPiece == TrackElemType::flatTrack1x1A
                            || rtd.StartTrackPiece == TrackElemType::flatTrack1x1B))
                        return type;
                }
        return kRideTypeNull;
    }
    inline bool Eligible(ObjectEntryIndex index)
    {
        const auto type = Type(index);
        return type != kRideTypeNull
            && (getGameState().cheats.ignoreResearchStatus || (RideTypeIsInvented(type) && RideEntryIsInvented(index)));
    }
    // Track queries/rendering need a native ride record. Borrow an unused slot without
    // running RideCreate (names, prices, random presets, etc.). Restore its exact contents.
    // The UI removes this record with its ghost before simulation and serialization.
    class PreviewRide
    {
        RideId _id{ RideId::GetNull() };
        Ride _previous;

    public:
        explicit PreviewRide(ObjectEntryIndex index)
        {
            _id = GetNextFreeRideId();
            if (_id.IsNull())
                return;
            _previous = std::move(getGameState().rides[_id.ToUnderlying()]);
            auto* ride = RideAllocateAtIndex(_id);
            ride->type = Type(index);
            ride->subtype = index;
            ride->mode = RideMode::shopStall;
            ride->overallView.setNull();
            for (auto& station : ride->getStations())
            {
                station.start.setNull();
                station.entrance.setNull();
                station.exit.setNull();
            }
            ride->setColourPreset(0, 0);
        }
        ~PreviewRide()
        {
            if (_id.IsNull())
                return;
            RideDelete(_id);
            getGameState().rides[_id.ToUnderlying()] = std::move(_previous);
        }
        PreviewRide(const PreviewRide&) = delete;
        PreviewRide& operator=(const PreviewRide&) = delete;
        RideId id() const
        {
            return _id;
        }
    };
} // namespace OpenRCT2::ShopPlacement

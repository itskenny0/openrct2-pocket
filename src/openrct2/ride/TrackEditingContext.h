// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../Context.h"
#include "../object/ObjectManager.h"
#include "../object/RideObject.h"
#include "../world/TileElementsView.h"
#include "../world/tile_element/EntranceElement.h"
#include "../world/tile_element/PathElement.h"
#include "../world/tile_element/TrackElement.h"
#include "Ride.h"
#include "TrackData.h"

#include <vector>
namespace OpenRCT2::TrackConstructionSupport
{
    // Exact, typed, transient action-review data. No hash, tile pointers, padding,
    // moving trains, queue counts or additional park serialization.
    inline std::vector<int32_t> EditingContext(const Ride& ride)
    {
        std::vector<int32_t> result;
        const auto* object = GetContext()->GetObjectManager().GetLoadedObject<RideObject>(ride.subtype);
        if (!object)
            return result;
        auto descriptor = [&](const Object* loaded) {
            if (!loaded) { result.push_back(-1); return; }
            const auto d = loaded->GetDescriptor();
            const auto name = d.ToString();
            result.push_back(static_cast<int32_t>(name.size()));
            for (auto c : name) result.push_back(static_cast<unsigned char>(c));
            result.insert(result.end(), { static_cast<int32_t>(d.Generation), static_cast<int32_t>(d.Entry.flags),
                static_cast<int32_t>(d.Entry.checksum), std::get<0>(loaded->GetVersion()), std::get<1>(loaded->GetVersion()),
                std::get<2>(loaded->GetVersion()) });
        };
        descriptor(object);
        descriptor(GetContext()->GetObjectManager().GetLoadedObject(ObjectType::station, ride.entranceStyle));
        const auto& entry = object->GetEntry();
        result.insert(result.end(), { static_cast<int32_t>(entry.flags.holder), entry.maxHeight, entry.Clearance,
            entry.DefaultCar, entry.min_cars_in_train, entry.max_cars_in_train });
        for (auto type : entry.ride_type) result.push_back(type);
        const auto supported = RideEntryGetSupportedTrackPieces(entry);
        for (size_t i = 0; i < supported.size(); ++i) result.push_back(supported[i]);
        for (const auto& car : entry.Cars) result.push_back(static_cast<int32_t>(car.flags.holder));
        for (const auto& colour : ride.trackColours)
            result.insert(result.end(), { static_cast<int32_t>(colour.main), static_cast<int32_t>(colour.additional),
                static_cast<int32_t>(colour.supports) });
        result.insert(result.end(), { ride.type, ride.subtype, ride.buildDate, ride.overallView.x, ride.overallView.y,
                                      ride.numStations });
        for (const auto& station : ride.getStations())
            result.insert(result.end(), { station.start.x, station.start.y, station.height, station.length,
                station.entrance.x, station.entrance.y, station.entrance.z, station.entrance.direction,
                station.exit.x, station.exit.y, station.exit.z, station.exit.direction });
        const auto& state = getGameState();
        result.push_back(state.park.flags.has(ParkFlag::noMoney));
        for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
            for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
            {
                for (auto* e : TileElementsView<TrackElement>(CoordsXY{ x, y }))
                    if (!e->isGhost() && e->getRideIndex() == ride.id)
                        result.insert(result.end(), { 1, x, y, e->getBaseZ(), e->getClearanceZ(), e->getDirection(),
                            static_cast<int32_t>(e->getTrackType()), e->getSequenceIndex(),
                            e->getStationIndex().ToUnderlying(), e->hasChain(), e->isInverted(),
                            e->getBrakeBoosterSpeed(), e->getSeatRotation(), e->getColourScheme() });
                for (auto* e : TileElementsView<EntranceElement>(CoordsXY{ x, y }))
                    if (!e->isGhost() && e->getRideIndex() == ride.id && e->getEntranceType() != EntranceType::parkEntrance)
                    {
                        result.insert(result.end(), { 2, x, y, e->getBaseZ(), e->getDirection(),
                            static_cast<int32_t>(e->getEntranceType()), e->getStationIndex().ToUnderlying() });
                        for (auto delta : CoordsDirectionDelta)
                            for (auto* path : TileElementsView<PathElement>(CoordsXY{ x, y } + delta))
                                if (!path->isGhost())
                                    result.insert(result.end(), { 3, delta.x, delta.y, path->getBaseZ(),
                                        path->getEdges(), path->isSloped(), path->getSlopeDirection() });
                    }
            }
        return result;
    }
}

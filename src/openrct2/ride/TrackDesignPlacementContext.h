// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../Context.h"
#include "../object/ObjectList.h"
#include "../object/ObjectManager.h"
#include "../object/RideObject.h"
#include "TrackConstructionSupport.h"
#include "TrackDesign.h"
namespace OpenRCT2::TrackDesignPlacementContext
{
    inline bool Compatible(const TrackDesign& td, ObjectEntryIndex index)
    {
        const auto type = td.trackAndVehicle.rtdIndex;
        const auto* entry = GetRideEntryByIndex(index);
        if (!entry || type >= RIDE_TYPE_COUNT
            || std::find(std::begin(entry->ride_type), std::end(entry->ride_type), type) == std::end(entry->ride_type)
            || (!getGameState().cheats.ignoreResearchStatus && (!RideTypeIsInvented(type) || !RideEntryIsInvented(index))))
            return false;
        if (type == RIDE_TYPE_MAZE)
            return !td.mazeElements.empty() && td.trackElements.empty();
        if (!TrackConstructionSupport::Type(type) || td.trackElements.empty() || !td.mazeElements.empty())
            return false;
        Ride candidate{}; // Detached metadata-only native descriptor context, never a live ride.
        candidate.type = type;
        for (const auto& piece : td.trackElements)
            if (!TrackConstructionSupport::Supported(
                    candidate, *entry, piece.type, piece.flags.has(TrackDesignTrackElementFlag::isInverted)))
                return false;
        return true;
    }
    inline std::vector<int32_t> Capture(const TrackDesign& td)
    {
        std::vector<int32_t> result{ td.trackAndVehicle.rtdIndex, getGameState().lastEntranceStyle,
                                     getGameState().cheats.ignoreResearchStatus,
                                     getGameState().park.flags.has(ParkFlag::noMoney) };
        auto& manager = GetContext()->GetObjectManager();
        for (auto type : getAllObjectTypes())
        {
            if (type != ObjectType::ride && type != ObjectType::station && type != ObjectType::paths
                && type != ObjectType::footpathSurface && type != ObjectType::footpathRailings
                && type != ObjectType::smallScenery && type != ObjectType::largeScenery && type != ObjectType::walls)
                continue;
            for (ObjectEntryIndex index = 0; index < getObjectTypeLimit(type); ++index)
            {
                auto* object = manager.GetLoadedObject(type, index);
                if (!object)
                    continue;
                const auto descriptor = object->GetDescriptor();
                const auto name = descriptor.ToString();
                result.insert(result.end(), { static_cast<int32_t>(type), index, static_cast<int32_t>(name.size()) });
                for (auto c : name)
                    result.push_back(static_cast<unsigned char>(c));
                result.insert(
                    result.end(),
                    { static_cast<int32_t>(descriptor.Generation), static_cast<int32_t>(descriptor.Entry.flags),
                      static_cast<int32_t>(descriptor.Entry.checksum), std::get<0>(object->GetVersion()),
                      std::get<1>(object->GetVersion()), std::get<2>(object->GetVersion()), ResearchIsInvented(type, index) });
                if (type == ObjectType::ride)
                {
                    const auto& entry = static_cast<const RideObject*>(object)->GetEntry();
                    result.insert(
                        result.end(),
                        { static_cast<int32_t>(entry.flags.holder), entry.maxHeight, entry.Clearance, entry.DefaultCar,
                          entry.min_cars_in_train, entry.max_cars_in_train });
                    for (auto t : entry.ride_type)
                        result.push_back(t);
                    auto supported = RideEntryGetSupportedTrackPieces(entry);
                    for (size_t i = 0; i < supported.size(); ++i)
                        result.push_back(supported[i]);
                }
            }
        }
        result.push_back(td.trackAndVehicle.rtdIndex < RIDE_TYPE_COUNT && RideTypeIsInvented(td.trackAndVehicle.rtdIndex));
        return result;
    }
} // namespace OpenRCT2::TrackDesignPlacementContext

// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/Context.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/RideObject.h>
#include <openrct2/ride/FlatRidePlacement.h>
#include <openrct2/ride/TrackConstructionSupport.h>
#include <openrct2/ride/TrackDesign.h>

namespace OpenRCT2::Ui::Handheld::DesignAvailability
{
    inline std::string State(const ObjectEntryDescriptor& descriptor)
    {
        auto& manager = GetContext()->GetObjectManager();
        const auto* object = manager.GetLoadedObject(descriptor);
        if (!object) return "not loaded";
        return getGameState().cheats.ignoreResearchStatus
                || ResearchIsInvented(object->GetObjectType(), manager.GetLoadedObjectEntryIndex(object))
            ? "available" : "not researched";
    }
    inline bool Compatible(const TrackDesign& td, ObjectEntryIndex index)
    {
        const auto type = td.trackAndVehicle.rtdIndex;
        const auto* entry = GetRideEntryByIndex(index);
        if (!entry || type >= RIDE_TYPE_COUNT
            || std::find(std::begin(entry->ride_type), std::end(entry->ride_type), type) == std::end(entry->ride_type)
            || (!getGameState().cheats.ignoreResearchStatus && (!RideTypeIsInvented(type) || !RideEntryIsInvented(index))))
            return false;
        if (type == RIDE_TYPE_MAZE) return !td.mazeElements.empty() && td.trackElements.empty();
        if (!TrackConstructionSupport::Type(type) || td.trackElements.empty() || !td.mazeElements.empty()) return false;
        Ride candidate{}; // Detached metadata-only native descriptor context, never a live ride.
        candidate.type = type;
        for (const auto& piece : td.trackElements)
            if (!TrackConstructionSupport::Supported(candidate, *entry, piece.type,
                    piece.flags.has(TrackDesignTrackElementFlag::isInverted))) return false;
        return true;
    }
    inline std::vector<ObjectEntryIndex> Vehicles(const TrackDesign& td)
    {
        std::vector<ObjectEntryIndex> result;
        if (td.trackAndVehicle.rtdIndex >= RIDE_TYPE_COUNT) return result;
        for (auto i : GetContext()->GetObjectManager().GetAllRideEntries(td.trackAndVehicle.rtdIndex))
            if (Compatible(td, i)) result.push_back(i);
        return result;
    }
    inline std::vector<std::string> Lines(const TrackDesign& td)
    {
        auto& manager = GetContext()->GetObjectManager();
        const auto type = td.trackAndVehicle.rtdIndex;
        std::vector<std::string> result{ "Required: " + td.trackAndVehicle.vehicleObject.ToString(),
            "Vehicle: " + State(td.trackAndVehicle.vehicleObject) };
        result.push_back(type < RIDE_TYPE_COUNT && (getGameState().cheats.ignoreResearchStatus || RideTypeIsInvented(type))
            ? "Ride type: researched" : "Ride type: unavailable");
        result.push_back("Native track-compatible vehicles: " + std::to_string(Vehicles(td).size()));
        result.push_back(manager.GetLoadedObjectEntryIndex(td.appearance.stationObjectIdentifier) != kObjectEntryIndexNull
            ? "Station style: loaded" : "Station style: native default fallback");
        if (FlatRidePlacement::StationObject() == kObjectEntryIndexNull) result.push_back("No loaded station style for placement");
        if (td.sceneryElements.empty()) result.push_back("No optional design scenery");
        for (const auto& element : td.sceneryElements)
        {
            result.push_back("Optional: " + element.sceneryObject.ToString());
            const auto resolved = TrackDesignResolveScenery(element);
            if (!resolved) result.push_back("Unavailable: native policy omits ALL scenery");
            else if (element.sceneryObject.GetType() == ObjectType::paths)
            {
                const auto* object = manager.GetLoadedObject(resolved->Type, resolved->Index);
                result.push_back("Native path: " + (object ? object->GetDescriptor().ToString() : std::string("unavailable")));
                result.push_back("Path mapping/default surface and railing may differ");
            }
            else result.push_back("Scenery: " + State(element.sceneryObject));
        }
        return result;
    }
}

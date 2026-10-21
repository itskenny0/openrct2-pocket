// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/ride/TrackDesignPlacementContext.h>
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
    using TrackDesignPlacementContext::Compatible;
    inline std::vector<ObjectEntryIndex> Vehicles(const TrackDesign& td)
    {
        std::vector<ObjectEntryIndex> result;
        if (td.trackAndVehicle.rtdIndex >= RIDE_TYPE_COUNT) return result;
        for (auto i : GetContext()->GetObjectManager().GetAllRideEntries(td.trackAndVehicle.rtdIndex))
            if (Compatible(td, i)) result.push_back(i);
        return result;
    }
    inline std::vector<std::string> Lines(const TrackDesign& td, bool technical = false)
    {
        auto& manager = GetContext()->GetObjectManager();
        const auto type = td.trackAndVehicle.rtdIndex;
        const auto* vehicle = manager.GetLoadedObject(td.trackAndVehicle.vehicleObject);
        std::vector<std::string> result{ "Original vehicle: " + (vehicle ? vehicle->GetName() : "unavailable"),
            "Vehicle: " + State(td.trackAndVehicle.vehicleObject) };
        if (technical) result.push_back("Vehicle ID: " + td.trackAndVehicle.vehicleObject.ToString());
        result.push_back(type < RIDE_TYPE_COUNT && (getGameState().cheats.ignoreResearchStatus || RideTypeIsInvented(type))
            ? "Ride type: researched" : "Ride type: unavailable");
        result.push_back("Compatible vehicles: " + std::to_string(Vehicles(td).size()));
        result.push_back(manager.GetLoadedObjectEntryIndex(td.appearance.stationObjectIdentifier) != kObjectEntryIndexNull
            ? "Station style: available" : "Station style: uses default");
        if (FlatRidePlacement::StationObject() == kObjectEntryIndexNull) result.push_back("No loaded station style for placement");
        if (td.sceneryElements.empty()) result.push_back("No optional design scenery");
        size_t missing = 0;
        bool mappedPaths = false;
        std::vector<ObjectEntryDescriptor> shown;
        for (const auto& element : td.sceneryElements)
        {
            const auto resolved = TrackDesignResolveScenery(element);
            if (!resolved) ++missing;
            if (std::find(shown.begin(), shown.end(), element.sceneryObject) != shown.end()) continue;
            shown.push_back(element.sceneryObject);
            const auto* object = resolved ? manager.GetLoadedObject(resolved->Type, resolved->Index) : nullptr;
            if (technical) result.push_back("Scenery ID: " + element.sceneryObject.ToString());
            if (element.sceneryObject.GetType() == ObjectType::paths && resolved)
            {
                mappedPaths = true;
                if (technical) result.push_back("Mapped path: " + (object ? object->GetDescriptor().ToString() : std::string("unavailable")));
            }
            if (object) result.push_back("Scenery: " + object->GetName() + " / " + State(object->GetDescriptor()));
        }
        if (missing) result.insert(result.begin(), "Missing scenery: places without ALL design scenery");
        if (mappedPaths) result.push_back("Path surface and railings may differ from the original");
        return result;
    }
}

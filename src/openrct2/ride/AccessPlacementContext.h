// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "RideIdentity.h"
#include "FlatRidePlacement.h"
#include "../object/RideObject.h"
#include "../world/tile_element/TileElement.h"
#include <set>

namespace OpenRCT2::AccessPlacementContext
{
    // Transient native review identity, excluding ghosts and allocation-order tile flags.
    inline std::vector<int64_t> Capture(const Ride& ride, CoordsXY destination)
    {
        auto& state=getGameState();auto& objects=GetContext()->GetObjectManager();
        const auto* object=objects.GetLoadedObject<RideObject>(ride.subtype);
        const auto* stationObject=objects.GetLoadedObject(ObjectType::station,ride.entranceStyle);
        if(!object || !stationObject)return {};
        std::vector<int64_t> data{ride.id.ToUnderlying(),ride.type,ride.subtype,ride.buildDate,
            static_cast<int64_t>(RideAllocationGeneration(ride.id)),static_cast<int64_t>(ride.status),
            ride.overallView.x,ride.overallView.y,ride.mazeTiles,ride.entranceStyle,
            ride.flags.has(RideFlag::indestructibleTrack),state.park.flags.has(ParkFlag::noMoney),
            state.cheats.sandboxMode,state.cheats.disableClearanceChecks,state.cheats.disableSupportLimits,
            state.cheats.buildInPauseMode,state.cheats.ignoreResearchStatus,RideTypeIsInvented(ride.type),RideEntryIsInvented(ride.subtype)};
        for(const auto* o:{static_cast<const Object*>(object),stationObject}) {
            const auto identity=o->GetDescriptor();
            data.insert(data.end(),{static_cast<int64_t>(identity.Generation),identity.Entry.flags,identity.Entry.checksum});
            const auto descriptor=identity.ToString();data.push_back(descriptor.size());
            for(auto c:descriptor)data.push_back(static_cast<unsigned char>(c));
            data.insert(data.end(),{std::get<0>(o->GetVersion()),std::get<1>(o->GetVersion()),std::get<2>(o->GetVersion())});
        }
        data.push_back(object->GetEntry().flags.holder);
        std::set<std::pair<int32_t,int32_t>> tiles;
        auto area=[&](CoordsXY p){if(p.isNull())return;for(int y=-64;y<=64;y+=32)for(int x=-64;x<=64;x+=32)
            if(MapIsLocationValid(p+CoordsXY{x,y}))tiles.emplace(p.x+x,p.y+y);};
        area(destination);
        for(const auto& s:ride.getStations()) {
            data.insert(data.end(),{s.start.x,s.start.y,s.getBaseZ(),s.length,
                s.entrance.x,s.entrance.y,s.entrance.z,s.entrance.direction,s.exit.x,s.exit.y,s.exit.z,s.exit.direction});
            area(s.start);if(!s.entrance.isNull())area(s.entrance.toCoordsXY());if(!s.exit.isNull())area(s.exit.toCoordsXY());
        }
        for(auto [x,y]:tiles){data.insert(data.end(),{x,y});
            for(const auto* e:TileElementsView(CoordsXY{x,y}))if(!e->isGhost()){
                auto copy=*e;copy.setLastForTile(false);const auto* bytes=reinterpret_cast<const uint8_t*>(&copy);
                for(size_t i=0;i<sizeof(copy);++i)data.push_back(bytes[i]);
            }
            data.push_back(-1);
        }
        return data;
    }
}

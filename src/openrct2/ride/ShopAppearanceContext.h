// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../Context.h"
#include "../GameState.h"
#include "../actions/ride/RideSetAppearanceAction.h"
#include "../object/ObjectManager.h"
#include "../object/RideObject.h"
#include "../world/TileElementsView.h"
#include "../world/tile_element/TrackElement.h"
#include "RideIdentity.h"
#include "Ride.h"
#include "RideData.h"

namespace OpenRCT2::ShopAppearanceContext
{
    using A=GameActions::RideSetAppearanceType;
    inline bool Supported(const Ride& ride,A channel)
    {
        const auto* entry=ride.getRideEntry();
        const auto& type=ride.getRideTypeDescriptor();
        if (!entry || !type.flags.has(RtdFlag::isShopOrFacility) || entry->flags.has(RideEntryFlag::disableColourTab)) return false;
        switch(channel) {
            case A::trackColourMain:return type.flags.has(RtdFlag::hasTrackColourMain);
            case A::trackColourAdditional:return type.flags.has(RtdFlag::hasTrackColourAdditional);
            case A::trackColourSupports:return type.flags.has(RtdFlag::hasTrackColourSupports);
            case A::sellingItemColourIsRandom:return ride.hasRecolourableShopItems();
            default:return false;
        }
    }
    inline uint16_t Value(const Ride& ride,A channel)
    {
        switch(channel) {
            case A::trackColourMain:return static_cast<uint16_t>(ride.trackColours[0].main);
            case A::trackColourAdditional:return static_cast<uint16_t>(ride.trackColours[0].additional);
            case A::trackColourSupports:return static_cast<uint16_t>(ride.trackColours[0].supports);
            case A::sellingItemColourIsRandom:return ride.flags.has(RideFlag::randomShopColours);
            default:return UINT16_MAX;
        }
    }
    inline bool Allowed(const Ride& ride,A channel,uint16_t value,uint32_t index)
    {
        return index==0 && Supported(ride,channel) && value!=Value(ride,channel)
            && (channel==A::sellingItemColourIsRandom ? value<2 : value<(getGameState().cheats.allowSpecialColourSchemes?Drawing::kColourNumTotal:Drawing::kColourNumNormal));
    }
    inline std::vector<int64_t> Capture(const Ride& ride)
    {
        const auto* object=GetContext()->GetObjectManager().GetLoadedObject<RideObject>(ride.subtype);
        if (!object || !ride.getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility)) return {};
        const auto d=object->GetDescriptor();const auto name=d.ToString();
        std::vector<int64_t> result{ride.id.ToUnderlying(),ride.type,ride.subtype,ride.buildDate,ride.overallView.x,ride.overallView.y,
            static_cast<int64_t>(RideAllocationGeneration(ride.id)),static_cast<int64_t>(name.size()),
            static_cast<int64_t>(d.Generation),d.Entry.flags,d.Entry.checksum,
            std::get<0>(object->GetVersion()),std::get<1>(object->GetVersion()),std::get<2>(object->GetVersion()),
            static_cast<int64_t>(object->GetEntry().flags.holder),getGameState().cheats.allowSpecialColourSchemes};
        for(auto c:name) result.push_back(static_cast<unsigned char>(c));
        for(auto channel:{A::trackColourMain,A::trackColourAdditional,A::trackColourSupports,A::sellingItemColourIsRandom})
            result.push_back(Supported(ride,channel));
        for(auto item:object->GetEntry().shop_item)result.push_back(static_cast<int64_t>(item));
        for(const auto& colour:ride.trackColours)result.insert(result.end(),{static_cast<int64_t>(colour.main),static_cast<int64_t>(colour.additional),static_cast<int64_t>(colour.supports)});
        result.push_back(ride.flags.has(RideFlag::randomShopColours));
        if (!ride.overallView.isNull() && MapIsLocationValid(ride.overallView))
            for(auto* e:TileElementsView<TrackElement>(ride.overallView))
                if(!e->isGhost() && e->getRideIndex()==ride.id)
                    result.insert(result.end(),{e->getBaseZ(),e->getClearanceZ(),e->getDirection(),static_cast<int64_t>(e->getTrackType()),e->getSequenceIndex()});
        return result;
    }
}

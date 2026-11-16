// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../../ride/Ride.h"
#include "../GameAction.hpp"
namespace OpenRCT2::GameActions
{
    // One queued command composes native create, track placement and opening.
    // Only its top-level track cost is charged; no orphan ride on failed placement.
    class ShopPlaceAction final : public GameActionBase<GameCommand::placeShop>
    {
        ObjectEntryIndex _index{ kObjectEntryIndexNull };
        CoordsXYZD _origin{};

    public:
        ShopPlaceAction() = default;
        ShopPlaceAction(ObjectEntryIndex index, const CoordsXYZD& origin)
            : _index(index)
            , _origin(origin)
        {
        }
        void AcceptParameters(GameActionParameterVisitor&) final;
        void Serialise(DataSerialiser&) final;
        Result Query(GameState_t&, Park::ParkData&) const final;
        Result Execute(GameState_t&, Park::ParkData&) const final;
    };
} // namespace OpenRCT2::GameActions

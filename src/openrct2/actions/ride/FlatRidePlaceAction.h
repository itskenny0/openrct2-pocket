// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../../ride/Ride.h"
#include "../GameAction.hpp"
namespace OpenRCT2::GameActions
{
    // One queued command composes native create and single-piece flat ride placement.
    // Only its top-level track cost is charged; no orphan ride on failed placement.
    class FlatRidePlaceAction final : public GameActionBase<GameCommand::placeFlatRide>
    {
        ObjectEntryIndex _index{ kObjectEntryIndexNull };
        CoordsXYZD _origin{};

    public:
        FlatRidePlaceAction() = default;
        FlatRidePlaceAction(ObjectEntryIndex index, const CoordsXYZD& origin)
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

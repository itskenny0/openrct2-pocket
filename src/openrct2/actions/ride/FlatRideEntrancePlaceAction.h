// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../../ride/Ride.h"
#include "../GameAction.hpp"
namespace OpenRCT2::GameActions
{
    // Revalidate construction-tool station-side resolution at queued execution.
    class FlatRideEntrancePlaceAction final : public GameActionBase<GameCommand::placeFlatRideEntrance>
    {
        RideId _ride{ RideId::GetNull() };
        CoordsXYZD _location{};
        bool _exit{};
        StationIndex _station{ StationIndex::FromUnderlying(0) };
        ObjectEntryIndex _object{ kObjectEntryIndexNull };
        int32_t _buildDate{};
        CoordsXY _start{};
        TileCoordsXYZD _entrance{};

    public:
        FlatRideEntrancePlaceAction() = default;
        FlatRideEntrancePlaceAction(
            RideId ride, CoordsXYZD location, bool isExit, StationIndex station = StationIndex::FromUnderlying(0));
        void AcceptParameters(GameActionParameterVisitor&) final;
        void Serialise(DataSerialiser&) final;
        Result Query(GameState_t&, Park::ParkData&) const final;
        Result Execute(GameState_t&, Park::ParkData&) const final;
    };
} // namespace OpenRCT2::GameActions

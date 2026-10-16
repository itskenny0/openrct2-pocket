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
        std::vector<int64_t> _context;
        bool _editExisting{}, _removeExisting{};
        money64 _quote{kMoney64Undefined};

    public:
        FlatRideEntrancePlaceAction() = default;
        FlatRideEntrancePlaceAction(
            RideId ride, CoordsXYZD location, bool isExit, StationIndex station = StationIndex::FromUnderlying(0));
        void EditExisting(bool remove = false) { _editExisting = true; _removeExisting = remove; }
        void GuardContext(std::vector<int64_t> context,money64 quote) { _context=std::move(context);_quote=quote; }
        void AcceptParameters(GameActionParameterVisitor&) final;
        void Serialise(DataSerialiser&) final;
        Result Query(GameState_t&, Park::ParkData&) const final;
        Result Execute(GameState_t&, Park::ParkData&) const final;
    };
} // namespace OpenRCT2::GameActions

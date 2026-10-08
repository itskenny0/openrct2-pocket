// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "TrackPlaceAction.h"
namespace OpenRCT2::GameActions
{
    // Pocket's strict sequential context. Native TrackPlace owns geometry, cost and execution.
    class SequentialTrackPlaceAction final : public GameActionBase<GameCommand::placeSequentialTrack>
    {
        RideId _ride{ RideId::GetNull() };
        ObjectEntryIndex _object{ kObjectEntryIndexNull };
        int32_t _buildDate{};
        CoordsXYZD _origin{}, _source{};
        TrackElemType _piece{}, _sourceType{};
        bool _first{}, _chain{}, _noMoney{}, _remove{};
        money64 _cost{ kMoney64Undefined };

    public:
        SequentialTrackPlaceAction() = default;
        SequentialTrackPlaceAction(
            RideId ride, TrackElemType piece, CoordsXYZD origin, bool chain, bool first, CoordsXYZD source,
            TrackElemType sourceType, money64 cost = kMoney64Undefined);
        void UseRemoval()
        {
            _remove = true;
        }
        void AcceptParameters(GameActionParameterVisitor&) final;
        void Serialise(DataSerialiser&) final;
        Result Query(GameState_t&, Park::ParkData&) const final;
        Result Execute(GameState_t&, Park::ParkData&) const final;
    };
} // namespace OpenRCT2::GameActions

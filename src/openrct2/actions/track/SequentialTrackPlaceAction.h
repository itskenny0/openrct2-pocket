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
        ride_type_t _expectedType{ kRideTypeNull };
        CoordsXY _expectedView{};
        CoordsXYZD _origin{}, _source{};
        TrackElemType _piece{}, _sourceType{};
        bool _first{}, _chain{}, _noMoney{}, _remove{};
        bool _selected{}, _speedChange{}, _sourceInverted{};
        uint8_t _baselineSpeed{}, _newSpeed{};
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
        void UseSelectedRemoval(uint8_t speed, bool inverted)
        {
            _remove = _selected = true;
            _baselineSpeed = speed;
            _sourceInverted = inverted;
        }
        void UseSpeedChange(uint8_t baseline, uint8_t speed, bool inverted)
        {
            _selected = _speedChange = true;
            _baselineSpeed = baseline;
            _newSpeed = speed;
            _sourceInverted = inverted;
        }
        void AcceptParameters(GameActionParameterVisitor&) final;
        void Serialise(DataSerialiser&) final;
        Result Query(GameState_t&, Park::ParkData&) const final;
        Result Execute(GameState_t&, Park::ParkData&) const final;
    };
} // namespace OpenRCT2::GameActions

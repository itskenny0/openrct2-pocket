// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../../ride/TrackConstructionSupport.h"
#include "../../ride/TrackEditingContext.h"
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
        bool _first{}, _chain{}, _noMoney{}, _remove{}, _placementInverted{};
        bool _selected{}, _speedChange{}, _sourceInverted{};
        uint8_t _baselineSpeed{}, _baselineSeat{}, _newSpeed{}, _placementSpeed{ 6 }, _placementSeat{};
        bool _guardPrevious{}, _backward{}, _guardSource{}, _detached{};
        uint8_t _placementColour{};
        TrackConstructionSupport::BlockSnapshot _sourceSnapshot{};
        TrackConstructionSupport::BlockSnapshot _previous{};
        std::vector<int32_t> _editingContext;
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
        void UseSelectedRemoval(uint8_t speed, bool inverted, uint8_t seat = 0)
        {
            _remove = _selected = true;
            _baselineSpeed = speed;
            _sourceInverted = inverted;
            _baselineSeat = seat;
        }
        void UseSpeedChange(uint8_t baseline, uint8_t speed, bool inverted, uint8_t seat = 0)
        {
            _selected = _speedChange = true;
            _baselineSpeed = baseline;
            _newSpeed = speed;
            _sourceInverted = inverted;
            _baselineSeat = seat;
        }
        void SetPlacementInverted(bool inverted)
        {
            _placementInverted = inverted;
        }
        void SetPlacementSpeed(uint8_t speed)
        {
            _placementSpeed = speed;
        }
        void SetPlacementSeat(uint8_t seat)
        {
            _placementSeat = seat;
        }
        void GuardPrevious(const TrackConstructionSupport::BlockSnapshot& previous)
        {
            _guardPrevious = true;
            _previous = previous;
        }
        void GuardSource(const TrackConstructionSupport::BlockSnapshot& source)
        {
            _guardSource = true;
            _sourceSnapshot = source;
        }
        void UseBackward(const TrackConstructionSupport::BlockSnapshot& source)
        {
            _backward = _guardSource = true;
            _sourceSnapshot = source;
        }
        void UseDetached(const std::vector<int32_t>& context)
        {
            _detached = true;
            _first = false;
            _editingContext = context;
        }
        void SetPlacementColour(uint8_t colour)
        {
            _placementColour = colour;
        }
        void GuardEditingContext(const std::vector<int32_t>& context)
        {
            _editingContext = context;
        }
        void AcceptParameters(GameActionParameterVisitor&) final;
        void Serialise(DataSerialiser&) final;
        Result Query(GameState_t&, Park::ParkData&) const final;
        Result Execute(GameState_t&, Park::ParkData&) const final;
    };
} // namespace OpenRCT2::GameActions

// SPDX-License-Identifier: GPL-3.0-only
#include "SequentialTrackPlaceAction.h"

#include "../../GameState.h"
#include "../../management/Research.h"
#include "../../ride/RideData.h"
#include "../../ride/Track.h"
#include "../../ride/TrackConstructionSupport.h"
#include "../../ride/TrackData.h"
#include "../../ride/TrackIteration.h"
#include "../../ride/ted/TrackElementDescriptor.h"
#include "../../world/Map.h"
#include "../../world/TileElementsView.h"
#include "../../world/tile_element/TrackElement.h"
#include "../GameActionRunner.h"
#include "TrackRemoveAction.h"
#include "TrackSetBrakeSpeedAction.h"
namespace OpenRCT2::GameActions
{
    using namespace TrackMetadata;
    SequentialTrackPlaceAction::SequentialTrackPlaceAction(
        RideId id, TrackElemType piece, CoordsXYZD origin, bool chain, bool first, CoordsXYZD source, TrackElemType sourceType,
        money64 cost)
        : _ride(id)
        , _origin(origin)
        , _source(source)
        , _piece(piece)
        , _sourceType(sourceType)
        , _first(first)
        , _chain(chain)
        , _cost(cost)
    {
        if (auto* ride = GetRide(id))
        {
            _object = ride->subtype;
            _buildDate = ride->buildDate;
            _expectedType = ride->type;
            _expectedView = ride->overallView;
        }
        _noMoney = getGameState().park.flags.has(ParkFlag::noMoney);
    }
    void SequentialTrackPlaceAction::AcceptParameters(GameActionParameterVisitor& visitor)
    {
        visitor.Visit("ride", _ride);
        visitor.Visit("piece", _piece);
        visitor.Visit(_origin);
        visitor.Visit("sourceX", _source.x);
        visitor.Visit("sourceY", _source.y);
        visitor.Visit("sourceZ", _source.z);
        visitor.Visit("sourceDirection", _source.direction);
        visitor.Visit("sourceType", _sourceType);
        visitor.Visit("first", _first);
        visitor.Visit("remove", _remove);
        visitor.Visit("chain", _chain);
        visitor.Visit("object", _object);
        visitor.Visit("buildDate", _buildDate);
        visitor.Visit("expectedCost", _cost);
        visitor.Visit("noMoney", _noMoney);
        visitor.Visit("expectedType", _expectedType);
        visitor.Visit("expectedViewX", _expectedView.x);
        visitor.Visit("expectedViewY", _expectedView.y);
        visitor.Visit("selected", _selected);
        visitor.Visit("speedChange", _speedChange);
        visitor.Visit("baselineSpeed", _baselineSpeed);
        visitor.Visit("baselineSeat", _baselineSeat);
        visitor.Visit("newSpeed", _newSpeed);
        visitor.Visit("sourceInverted", _sourceInverted);
        visitor.Visit("placementSpeed", _placementSpeed);
        visitor.Visit("placementSeat", _placementSeat);
        visitor.Visit("backward", _backward);
        visitor.Visit("guardSource", _guardSource);
        visitor.Visit("sourceSpeed", _sourceSnapshot.speed);
        visitor.Visit("sourceSeat", _sourceSnapshot.seat);
        visitor.Visit("sourceChain", _sourceSnapshot.chain);
        visitor.Visit("sourceInvertedBaseline", _sourceSnapshot.inverted);
        visitor.Visit("guardPrevious", _guardPrevious);
        visitor.Visit("previousX", _previous.origin.x);
        visitor.Visit("previousY", _previous.origin.y);
        visitor.Visit("previousZ", _previous.origin.z);
        visitor.Visit("previousDirection", _previous.origin.direction);
        visitor.Visit("previousType", _previous.type);
        visitor.Visit("previousSpeed", _previous.speed);
        visitor.Visit("previousSeat", _previous.seat);
        visitor.Visit("previousChain", _previous.chain);
        visitor.Visit("previousInverted", _previous.inverted);
    }
    void SequentialTrackPlaceAction::Serialise(DataSerialiser& stream)
    {
        GameAction::Serialise(stream);
        stream << DS_TAG(_ride) << DS_TAG(_object) << DS_TAG(_buildDate) << DS_TAG(_origin) << DS_TAG(_source) << DS_TAG(_piece)
               << DS_TAG(_sourceType) << DS_TAG(_first) << DS_TAG(_chain) << DS_TAG(_cost) << DS_TAG(_noMoney)
               << DS_TAG(_remove) << DS_TAG(_selected) << DS_TAG(_speedChange) << DS_TAG(_baselineSpeed) << DS_TAG(_newSpeed)
               << DS_TAG(_baselineSeat) << DS_TAG(_sourceInverted) << DS_TAG(_placementSeat) << DS_TAG(_placementSpeed)
               << DS_TAG(_expectedType) << DS_TAG(_expectedView) << DS_TAG(_guardPrevious) << DS_TAG(_previous.origin)
               << DS_TAG(_previous.type) << DS_TAG(_previous.speed) << DS_TAG(_previous.seat) << DS_TAG(_previous.chain)
               << DS_TAG(_previous.inverted) << DS_TAG(_backward) << DS_TAG(_guardSource) << DS_TAG(_sourceSnapshot.speed)
               << DS_TAG(_sourceSnapshot.seat) << DS_TAG(_sourceSnapshot.chain) << DS_TAG(_sourceSnapshot.inverted);
    }
    Result SequentialTrackPlaceAction::Query(GameState_t& state, Park::ParkData&) const
    {
        auto invalid = []() {
            Result r;
            r.error = Status::disallowed;
            r.errorMessage = "Track endpoint or availability changed; resume construction";
            return r;
        };
        const auto* ride = GetRide(_ride);
        const auto* entry = GetRideEntryByIndex(_object);
        if (!ride || !TrackConstructionSupport::Type(ride->type) || ride->subtype != _object || ride->buildDate != _buildDate
            || ride->status != RideStatus::closed || !entry || _origin.direction > 3 || !TrackConstructionSupport::Piece(_piece)
            || _sourceType >= TrackElemType::count || _noMoney != state.park.flags.has(ParkFlag::noMoney)
            || (!state.cheats.ignoreResearchStatus && (!RideTypeIsInvented(ride->type) || !RideEntryIsInvented(_object))))
            return invalid();
        if (ride->type != _expectedType || ride->overallView != _expectedView)
            return invalid();
        if (_backward && (_first || _selected || _speedChange || !_guardSource))
            return invalid();
        if (_guardPrevious && (!_selected || !_remove || _first))
            return invalid();
        if (_selected && !_remove && !_speedChange)
            return invalid();
        if (!_remove && !_speedChange
            && (trackTypeHasSpeedSetting(_piece) ? (_placementSpeed < 2 || _placementSpeed > 30) : _placementSpeed != 6))
            return invalid();
        if (!_remove && !_speedChange
            && (TrackConstructionSupport::SeatRotation(ride->type, _piece) ? _placementSeat > 15 : _placementSeat != 0))
            return invalid();
        const auto& ted = GetTrackElementDescriptor(_piece);
        if (!ride->getRideTypeDescriptor().SupportsTrackGroup(ted.definition.group)
            || !RideEntryGetSupportedTrackPieces(*entry)[static_cast<size_t>(ted.definition.group)]
            || (_chain && !ted.flags.has(TrackElementFlag::allowLiftHill) && !ted.flags.has(TrackElementFlag::curveAllowsLift)))
            return invalid();
        if (_first)
        {
            if (_remove || _selected || _speedChange || _piece != TrackElemType::endStation)
                return invalid();
            if (!ride->overallView.isNull())
                for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
                    for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
                        for (auto* e : TileElementsView<TrackElement>(CoordsXY{ x, y }))
                            if (!e->isGhost() && e->getRideIndex() == _ride)
                                return invalid();
        }
        else
        {
            const auto& c = GetTrackElementDescriptor(_sourceType).sequenceData.sequences[0].clearance;
            const auto xy = CoordsXY(_source) + CoordsXY{ c.x, c.y }.rotate(_source.direction);
            if (_source.direction > 3 || !MapIsLocationValid(xy))
                return invalid();
            const TrackElement* found = nullptr;
            for (auto* e : TileElementsView<TrackElement>(xy))
                if (!e->isGhost() && e->getRideIndex() == _ride && e->getTrackType() == _sourceType
                    && e->getSequenceIndex() == 0 && e->getDirection() == _source.direction && e->getBaseZ() == _source.z + c.z)
                {
                    found = e;
                    break;
                }
            if (!found)
                return invalid();
            if (_backward && found->isInverted())
                return invalid();
            if (_guardSource
                && (found->getBrakeBoosterSpeed() != _sourceSnapshot.speed || found->getSeatRotation() != _sourceSnapshot.seat
                    || found->hasChain() != _sourceSnapshot.chain || found->isInverted() != _sourceSnapshot.inverted))
                return invalid();
            if (_selected
                && (_origin != _source || found->hasChain() != _chain || found->isInverted() != _sourceInverted
                    || found->getBrakeBoosterSpeed() != _baselineSpeed || found->getSeatRotation() != _baselineSeat))
                return invalid();
            if (_guardPrevious)
            {
                const auto previous = TrackConstructionSupport::Previous(
                    { xy, const_cast<TileElement*>(reinterpret_cast<const TileElement*>(found)) });
                if (!previous || *previous != _previous)
                    return invalid();
            }
            if (_speedChange)
            {
                if (!_selected || _remove || !trackTypeHasSpeedSetting(_sourceType) || _newSpeed > kMaximumTrackSpeed
                    || (_newSpeed < 2 && !state.cheats.unlockOperatingLimits)
                    || (!state.cheats.sandboxMode && !MapIsLocationOwned({ xy, _source.z + c.z })))
                    return invalid();
                TrackSetBrakeSpeedAction native({ xy, _source.z + c.z }, _sourceType, _newSpeed);
                native.SetFlags(GetFlags());
                auto result = QueryNested(&native, state);
                if (result.error == Status::ok && _cost != kMoney64Undefined && result.cost != _cost)
                    return invalid();
                return result;
            }
            CoordsXYE next;
            int32_t z{}, direction{};
            const CoordsXYE input{ xy, const_cast<TileElement*>(reinterpret_cast<const TileElement*>(found)) };
            TrackBeginEnd back{};
            if (_backward)
            {
                if (trackBlockGetPrevious(input, &back))
                    return invalid();
                const auto& sourceCoordinates = GetTrackElementDescriptor(_sourceType).coordinates;
                next = { back.end_x, back.end_y, nullptr };
                z = _source.z + sourceCoordinates.zBegin;
                direction = ((_source.direction + sourceCoordinates.rotationBegin) & 3) | (sourceCoordinates.rotationBegin & 4);
            }
            else if (!_selected && trackBlockGetNext(&input, &next, &z, &direction))
                return invalid();
            if (_remove)
            {
                TrackRemoveAction native(_sourceType, 0, { xy, _source.z + c.z, _source.direction });
                native.SetFlags(GetFlags());
                auto result = QueryNested(&native, state);
                if (result.error == Status::ok && _cost != kMoney64Undefined && result.cost != _cost)
                {
                    result.error = Status::disallowed;
                    result.errorMessage = "Removal refund changed; query again";
                }
                return result;
            }
            const auto endXY = CoordsXY(_origin) + CoordsXY{ ted.coordinates.x, ted.coordinates.y }.rotate(_origin.direction);
            const auto expected = _backward
                ? CoordsXYZD{ endXY, _origin.z + ted.coordinates.zEnd,
                              static_cast<Direction>(
                                  ((_origin.direction + ted.coordinates.rotationEnd) & 3) | (ted.coordinates.rotationEnd & 4)) }
                : CoordsXYZD{ _origin.x, _origin.y, _origin.z + ted.coordinates.zBegin,
                              static_cast<Direction>(
                                  ((_origin.direction + ted.coordinates.rotationBegin) & 3)
                                  | (ted.coordinates.rotationBegin & 4)) };
            if (next.x == kLocationNull || expected != CoordsXYZD{ next.x, next.y, z, static_cast<Direction>(direction) })
                return invalid();
            const auto& previous = GetTrackElementDescriptor(_sourceType).definition;
            if (_backward ? (ted.definition.pitchEnd != previous.pitchStart || ted.definition.rollEnd != previous.rollStart)
                          : (previous.pitchEnd != ted.definition.pitchStart || previous.rollEnd != ted.definition.rollStart))
                return invalid();
        }
        // Native endpoint lookup also checks a prospective closing connection's shape.
        // No route or connection is added here; native TrackPlace remains authoritative.
        CoordsXYE outgoing;
        const auto endXY = CoordsXY(_origin) + CoordsXY{ ted.coordinates.x, ted.coordinates.y }.rotate(_origin.direction);
        const auto endDirection = ((_origin.direction + ted.coordinates.rotationEnd) & 3) | (ted.coordinates.rotationEnd & 4);
        if (trackBlockGetNextFromZero(
                { endXY, _origin.z + ted.coordinates.zEnd }, *ride, endDirection, &outgoing, nullptr, nullptr, false))
        {
            const auto& next = GetTrackElementDescriptor(outgoing.element->asTrack()->getTrackType()).definition;
            if (ted.definition.pitchEnd != next.pitchStart || ted.definition.rollEnd != next.rollStart)
                return invalid();
        }
        if (_backward)
        {
            TrackBeginEnd incoming{};
            const auto beginDirection = ((_origin.direction + ted.coordinates.rotationBegin) & 3)
                | (ted.coordinates.rotationBegin & 4);
            if (trackBlockGetPreviousFromZero(
                    { _origin.x, _origin.y, _origin.z + ted.coordinates.zBegin }, *ride, beginDirection, &incoming)
                && incoming.begin_element)
            {
                const auto& definition = GetTrackElementDescriptor(incoming.begin_element->asTrack()->getTrackType())
                                             .definition;
                if (definition.pitchEnd != ted.definition.pitchStart || definition.rollEnd != ted.definition.rollStart)
                    return invalid();
            }
        }
        SelectedLiftAndInverted flags{};
        if (_chain)
            flags.set(LiftHillAndInverted::liftHill);
        TrackPlaceAction native(_ride, _piece, ride->type, _origin, _placementSpeed, 0, _placementSeat, flags, false);
        native.SetFlags(GetFlags());
        auto result = QueryNested(&native, state);
        if (result.error == Status::ok && _cost != kMoney64Undefined && result.cost != _cost)
        {
            result.error = Status::disallowed;
            result.errorMessage = "Track cost changed; query again";
        }
        return result;
    }
    Result SequentialTrackPlaceAction::Execute(GameState_t& state, Park::ParkData& park) const
    {
        auto result = Query(state, park);
        if (result.error != Status::ok)
            return result;
        if (_speedChange)
        {
            const auto& c = GetTrackElementDescriptor(_sourceType).sequenceData.sequences[0].clearance;
            const auto xy = CoordsXY(_source) + CoordsXY{ c.x, c.y }.rotate(_source.direction);
            TrackSetBrakeSpeedAction native({ xy, _source.z + c.z }, _sourceType, _newSpeed);
            native.SetFlags(GetFlags());
            return ExecuteNested(&native, state);
        }
        if (_remove)
        {
            const auto& c = GetTrackElementDescriptor(_sourceType).sequenceData.sequences[0].clearance;
            const auto xy = CoordsXY(_source) + CoordsXY{ c.x, c.y }.rotate(_source.direction);
            TrackRemoveAction native(_sourceType, 0, { xy, _source.z + c.z, _source.direction });
            native.SetFlags(GetFlags());
            return ExecuteNested(&native, state);
        }
        SelectedLiftAndInverted flags{};
        if (_chain)
            flags.set(LiftHillAndInverted::liftHill);
        TrackPlaceAction native(_ride, _piece, GetRide(_ride)->type, _origin, _placementSpeed, 0, _placementSeat, flags, false);
        native.SetFlags(GetFlags());
        return ExecuteNested(&native, state);
    }
} // namespace OpenRCT2::GameActions

// SPDX-License-Identifier: GPL-3.0-only
#include "SequentialTrackPlaceAction.h"

#include "../../GameState.h"
#include "../../management/Research.h"
#include "../../ride/RideData.h"
#include "../../ride/TrackConstructionSupport.h"
#include "../../ride/TrackData.h"
#include "../../ride/TrackIteration.h"
#include "../../ride/ted/TrackElementDescriptor.h"
#include "../../world/Map.h"
#include "../../world/TileElementsView.h"
#include "../../world/tile_element/TrackElement.h"
#include "../GameActionRunner.h"
#include "TrackRemoveAction.h"
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
    }
    void SequentialTrackPlaceAction::Serialise(DataSerialiser& stream)
    {
        GameAction::Serialise(stream);
        stream << DS_TAG(_ride) << DS_TAG(_object) << DS_TAG(_buildDate) << DS_TAG(_origin) << DS_TAG(_source) << DS_TAG(_piece)
               << DS_TAG(_sourceType) << DS_TAG(_first) << DS_TAG(_chain) << DS_TAG(_cost) << DS_TAG(_noMoney)
               << DS_TAG(_remove);
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
        const auto& ted = GetTrackElementDescriptor(_piece);
        if (!ride->getRideTypeDescriptor().SupportsTrackGroup(ted.definition.group)
            || !RideEntryGetSupportedTrackPieces(*entry)[static_cast<size_t>(ted.definition.group)]
            || (_chain && !ted.flags.has(TrackElementFlag::allowLiftHill) && !ted.flags.has(TrackElementFlag::curveAllowsLift)))
            return invalid();
        if (_first)
        {
            if (_remove || _piece != TrackElemType::endStation)
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
            CoordsXYE next;
            int32_t z{}, direction{};
            const CoordsXYE input{ xy, const_cast<TileElement*>(reinterpret_cast<const TileElement*>(found)) };
            if (trackBlockGetNext(&input, &next, &z, &direction))
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
            const auto expected = CoordsXYZD{ _origin.x, _origin.y, _origin.z + ted.coordinates.zBegin,
                                              static_cast<Direction>(
                                                  ((_origin.direction + ted.coordinates.rotationBegin) & 3)
                                                  | (ted.coordinates.rotationBegin & 4)) };
            if (next.x == kLocationNull || expected != CoordsXYZD{ next.x, next.y, z, static_cast<Direction>(direction) })
                return invalid();
            const auto& previous = GetTrackElementDescriptor(_sourceType).definition;
            if (previous.pitchEnd != ted.definition.pitchStart || previous.rollEnd != ted.definition.rollStart)
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
        SelectedLiftAndInverted flags{};
        if (_chain)
            flags.set(LiftHillAndInverted::liftHill);
        TrackPlaceAction native(_ride, _piece, ride->type, _origin, 6, 0, 0, flags, false);
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
        TrackPlaceAction native(_ride, _piece, GetRide(_ride)->type, _origin, 6, 0, 0, flags, false);
        native.SetFlags(GetFlags());
        return ExecuteNested(&native, state);
    }
} // namespace OpenRCT2::GameActions

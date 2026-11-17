// SPDX-License-Identifier: GPL-3.0-only
#include "FlatRideEntrancePlaceAction.h"

#include "../../ride/AccessPlacementContext.h"
#include "../../ride/FlatRidePlacement.h"
#include "../../world/tile_element/EntranceElement.h"
#include "../GameActionRunner.h"
#include "RideEntranceExitPlaceAction.h"
#include "RideEntranceExitRemoveAction.h"
namespace OpenRCT2::GameActions
{
    FlatRideEntrancePlaceAction::FlatRideEntrancePlaceAction(
        RideId id, CoordsXYZD location, bool isExit, StationIndex stationIndex)
        : _ride(id)
        , _location(location)
        , _exit(isExit)
        , _station(stationIndex)
    {
        if (const auto* ride = GetRide(id); ride && _station.ToUnderlying() < ride->getStations().size())
        {
            _object = ride->subtype;
            _buildDate = ride->buildDate;
            const auto& station = ride->getStation(_station);
            _start = station.start;
            _entrance = station.entrance;
            _context = AccessPlacementContext::Capture(*ride, location);
        }
    }
    void FlatRideEntrancePlaceAction::AcceptParameters(GameActionParameterVisitor& v)
    {
        v.Visit(_location);
        v.Visit("ride", _ride);
        v.Visit("exit", _exit);
        v.Visit("station", _station);
    }
    void FlatRideEntrancePlaceAction::Serialise(DataSerialiser& s)
    {
        GameAction::Serialise(s);
        s << DS_TAG(_ride) << DS_TAG(_location) << DS_TAG(_exit) << DS_TAG(_object) << DS_TAG(_buildDate) << DS_TAG(_start)
          << DS_TAG(_entrance) << DS_TAG(_station) << DS_TAG(_context) << DS_TAG(_quote) << DS_TAG(_editExisting)
          << DS_TAG(_removeExisting);
    }
    Result FlatRideEntrancePlaceAction::Query(GameState_t& state, Park::ParkData& park) const
    {
        const auto* ride = GetRide(_ride);
        auto invalid = [] {
            Result r;
            r.error = Status::invalidParameters;
            r.errorMessage = "Ride/station changed; resume construction";
            return r;
        };
        if (!ride || _station.ToUnderlying() >= ride->getStations().size() || ride->subtype != _object
            || ride->buildDate != _buildDate || !FlatRidePlacement::SupportsStation(*ride))
            return invalid();
        if (_context.empty() || _context != AccessPlacementContext::Capture(*ride, _location))
            return invalid();
        const auto& station = ride->getStation(_station);
        if (station.start != _start || station.entrance != _entrance
            || (_editExisting ? (_exit ? station.exit : station.entrance).isNull()
                              : !(_exit ? station.exit : station.entrance).isNull())
            || (!_editExisting && _exit && station.entrance.isNull()))
            return invalid();
        if (_editExisting)
        {
            const auto source = (_exit ? station.exit : station.entrance).toCoordsXYZD();
            const EntranceElement* first = nullptr;
            for (const auto* element : TileElementsView<EntranceElement>(source))
            {
                if (element->getRideIndex() == _ride && element->getStationIndex() == _station
                    && element->getEntranceType() == (_exit ? EntranceType::rideExit : EntranceType::rideEntrance))
                {
                    first = element;
                    break;
                }
            }
            // Match the exact first element that the native removal subaction resolves.
            if (!first || first->isGhost() || first->getBaseZ() != source.z || first->getDirection() != source.direction)
                return invalid();
        }
        if (_removeExisting)
        {
            if (!_editExisting || GetFlags().has(CommandFlag::ghost))
                return invalid();
            auto action = RideEntranceExitRemoveAction(
                (_exit ? station.exit : station.entrance).toCoordsXY(), _ride, _station, _exit);
            action.SetFlags(GetFlags());
            auto result = QueryNested(&action, state);
            if (_quote != kMoney64Undefined && result.error == Status::ok && result.cost != _quote)
                return invalid();
            return result;
        }
        const auto location = FlatRidePlacement::EntranceLocation(*ride, _location, _station);
        if (!location || *location != _location)
            return invalid();
        auto action = RideEntranceExitPlaceAction(_location, _location.direction, _ride, _station, _exit);
        action.SetFlags(GetFlags());
        if (GetFlags().has(CommandFlag::ghost))
            action.SetPureGhost();
        auto result = QueryNested(&action, state);
        if (_quote != kMoney64Undefined && result.error == Status::ok && result.cost != _quote)
            return invalid();
        return result;
    }
    Result FlatRideEntrancePlaceAction::Execute(GameState_t& state, Park::ParkData& park) const
    {
        auto valid = Query(state, park);
        if (valid.error != Status::ok)
            return valid;
        if (_removeExisting)
        {
            const auto& station = GetRide(_ride)->getStation(_station);
            auto remove = RideEntranceExitRemoveAction(
                (_exit ? station.exit : station.entrance).toCoordsXY(), _ride, _station, _exit);
            remove.SetFlags(GetFlags());
            return ExecuteNested(&remove, state);
        }
        auto action = RideEntranceExitPlaceAction(_location, _location.direction, _ride, _station, _exit);
        action.SetFlags(GetFlags());
        if (GetFlags().has(CommandFlag::ghost))
            action.SetPureGhost();
        return ExecuteNested(&action, state);
    }
} // namespace OpenRCT2::GameActions

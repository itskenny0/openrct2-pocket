// SPDX-License-Identifier: GPL-3.0-only
#include "FlatRideEntrancePlaceAction.h"

#include "../../ride/FlatRidePlacement.h"
#include "../GameActionRunner.h"
#include "RideEntranceExitPlaceAction.h"
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
          << DS_TAG(_entrance) << DS_TAG(_station);
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
        const auto& station = ride->getStation(_station);
        if (station.start != _start || station.entrance != _entrance || !(_exit ? station.exit : station.entrance).isNull()
            || (_exit && station.entrance.isNull()))
            return invalid();
        const auto location = FlatRidePlacement::EntranceLocation(*ride, _location, _station);
        if (!location || *location != _location)
            return invalid();
        auto action = RideEntranceExitPlaceAction(_location, _location.direction, _ride, _station, _exit);
        action.SetFlags(GetFlags());
        return QueryNested(&action, state);
    }
    Result FlatRideEntrancePlaceAction::Execute(GameState_t& state, Park::ParkData& park) const
    {
        auto valid = Query(state, park);
        if (valid.error != Status::ok)
            return valid;
        auto action = RideEntranceExitPlaceAction(_location, _location.direction, _ride, _station, _exit);
        action.SetFlags(GetFlags());
        return ExecuteNested(&action, state);
    }
} // namespace OpenRCT2::GameActions

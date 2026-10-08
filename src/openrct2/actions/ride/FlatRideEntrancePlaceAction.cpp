// SPDX-License-Identifier: GPL-3.0-only
#include "FlatRideEntrancePlaceAction.h"

#include "../../ride/FlatRidePlacement.h"
#include "../GameActionRunner.h"
#include "RideEntranceExitPlaceAction.h"
namespace OpenRCT2::GameActions
{
    FlatRideEntrancePlaceAction::FlatRideEntrancePlaceAction(RideId id, CoordsXYZD location, bool isExit)
        : _ride(id)
        , _location(location)
        , _exit(isExit)
    {
        if (const auto* ride = GetRide(id))
        {
            _object = ride->subtype;
            _buildDate = ride->buildDate;
            const auto& station = ride->getStation(StationIndex::FromUnderlying(0));
            _start = station.start;
            _entrance = station.entrance;
        }
    }
    void FlatRideEntrancePlaceAction::AcceptParameters(GameActionParameterVisitor& v)
    {
        v.Visit(_location);
        v.Visit("ride", _ride);
        v.Visit("exit", _exit);
    }
    void FlatRideEntrancePlaceAction::Serialise(DataSerialiser& s)
    {
        GameAction::Serialise(s);
        s << DS_TAG(_ride) << DS_TAG(_location) << DS_TAG(_exit) << DS_TAG(_object) << DS_TAG(_buildDate) << DS_TAG(_start)
          << DS_TAG(_entrance);
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
        if (!ride || ride->subtype != _object || ride->buildDate != _buildDate
            || FlatRidePlacement::Type(_object) != ride->type)
            return invalid();
        const auto& station = ride->getStation(StationIndex::FromUnderlying(0));
        if (station.start != _start || station.entrance != _entrance || !(_exit ? station.exit : station.entrance).isNull()
            || (_exit && station.entrance.isNull()))
            return invalid();
        const auto location = FlatRidePlacement::EntranceLocation(*ride, _location);
        if (!location || *location != _location)
            return invalid();
        auto action = RideEntranceExitPlaceAction(
            _location, _location.direction, _ride, StationIndex::FromUnderlying(0), _exit);
        action.SetFlags(GetFlags());
        return QueryNested(&action, state);
    }
    Result FlatRideEntrancePlaceAction::Execute(GameState_t& state, Park::ParkData& park) const
    {
        auto valid = Query(state, park);
        if (valid.error != Status::ok)
            return valid;
        auto action = RideEntranceExitPlaceAction(
            _location, _location.direction, _ride, StationIndex::FromUnderlying(0), _exit);
        action.SetFlags(GetFlags());
        return ExecuteNested(&action, state);
    }
} // namespace OpenRCT2::GameActions

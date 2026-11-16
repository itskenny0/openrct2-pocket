// SPDX-License-Identifier: GPL-3.0-only
#include "FlatRidePlaceAction.h"

#include "../../ride/FlatRidePlacement.h"
#include "../GameActionRunner.h"
#include "../track/TrackPlaceAction.h"
#include "RideCreateAction.h"
#include "RideSetStatusAction.h"

namespace OpenRCT2::GameActions
{
    void FlatRidePlaceAction::AcceptParameters(GameActionParameterVisitor& v)
    {
        v.Visit(_origin);
        v.Visit("object", _index);
    }
    void FlatRidePlaceAction::Serialise(DataSerialiser& s)
    {
        GameAction::Serialise(s);
        s << DS_TAG(_index) << DS_TAG(_origin);
    }
    Result FlatRidePlaceAction::Query(GameState_t& state, Park::ParkData& park) const
    {
        if (!FlatRidePlacement::Eligible(_index) || !DirectionValid(_origin.direction))
        {
            Result result;
            result.error = Status::disallowed;
            result.errorTitle = "Cannot place ride";
            result.errorMessage = "Ride unavailable";
            return result;
        }
        const auto type = FlatRidePlacement::Type(_index);
        auto create = RideCreateAction(type, _index, 0, 0, FlatRidePlacement::StationObject(), RideInspection::every30Minutes);
        auto result = QueryNested(&create, state);
        if (result.error != Status::ok)
            return result;
        ShopPlacement::PreviewRide preview(_index, type);
        auto track = TrackPlaceAction(
            preview.id(), GetRideTypeDescriptor(type).StartTrackPiece, type, _origin, 0, 0, 0, {}, false);
        track.SetFlags(GetFlags());
        return QueryNested(&track, state);
    }
    Result FlatRidePlaceAction::Execute(GameState_t& state, Park::ParkData& park) const
    {
        const auto type = FlatRidePlacement::Type(_index);
        auto create = RideCreateAction(type, _index, 0, 0, FlatRidePlacement::StationObject(), RideInspection::every30Minutes);
        create.SetFlags(GetFlags());
        auto created = ExecuteNested(&create, state);
        if (created.error != Status::ok)
            return created;
        const auto id = created.getData<RideId>();
        auto track = TrackPlaceAction(id, GetRideTypeDescriptor(type).StartTrackPiece, type, _origin, 0, 0, 0, {}, false);
        track.SetFlags(GetFlags());
        auto result = ExecuteNested(&track, state);
        if (result.error != Status::ok)
        {
            RideDelete(id);
            return result;
        }
        result.setData(RideId{ id });
        return result;
    }
} // namespace OpenRCT2::GameActions

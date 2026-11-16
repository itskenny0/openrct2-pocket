// SPDX-License-Identifier: GPL-3.0-only
#include "ShopPlaceAction.h"

#include "../../ride/ShopPlacement.h"
#include "../GameActionRunner.h"
#include "../track/TrackPlaceAction.h"
#include "RideCreateAction.h"
#include "RideSetStatusAction.h"

namespace OpenRCT2::GameActions
{
    void ShopPlaceAction::AcceptParameters(GameActionParameterVisitor& v)
    {
        v.Visit(_origin);
        v.Visit("object", _index);
    }
    void ShopPlaceAction::Serialise(DataSerialiser& s)
    {
        GameAction::Serialise(s);
        s << DS_TAG(_index) << DS_TAG(_origin);
    }
    Result ShopPlaceAction::Query(GameState_t& state, Park::ParkData& park) const
    {
        if (!ShopPlacement::Eligible(_index) || !DirectionValid(_origin.direction))
        {
            Result result;
            result.error = Status::disallowed;
            result.errorTitle = "Cannot place shop";
            result.errorMessage = "Shop unavailable";
            return result;
        }
        const auto type = ShopPlacement::Type(_index);
        auto create = RideCreateAction(type, _index, 0, 0, kObjectEntryIndexNull, RideInspection::every30Minutes);
        auto result = QueryNested(&create, state);
        if (result.error != Status::ok)
            return result;
        ShopPlacement::PreviewRide preview(_index);
        auto track = TrackPlaceAction(
            preview.id(), GetRideTypeDescriptor(type).StartTrackPiece, type, _origin, 0, 0, 0, {}, false);
        track.SetFlags(GetFlags());
        return QueryNested(&track, state);
    }
    Result ShopPlaceAction::Execute(GameState_t& state, Park::ParkData& park) const
    {
        const auto type = ShopPlacement::Type(_index);
        auto create = RideCreateAction(type, _index, 0, 0, kObjectEntryIndexNull, RideInspection::every30Minutes);
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
        auto open = RideSetStatusAction(id, RideStatus::open);
        open.SetFlags(GetFlags());
        ExecuteNested(&open, state); // Native shops can operate immediately; no entrance/exit construction.
        result.setData(RideId{ id });
        return result;
    }
} // namespace OpenRCT2::GameActions

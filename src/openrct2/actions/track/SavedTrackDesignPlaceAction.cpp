// SPDX-License-Identifier: GPL-3.0-only
#include "SavedTrackDesignPlaceAction.h"

#include "../../ride/FlatRidePlacement.h"
#include "../../ride/TrackDesignPlacementContext.h"
#include "../../ride/TrackConstructionSupport.h"
#include "../GameActionRunner.h"
namespace OpenRCT2::GameActions
{
    SavedTrackDesignPlaceAction::SavedTrackDesignPlaceAction(
        CoordsXYZD origin, const TrackDesign& design, RideInspection inspection, money64 expectedCost, bool placeScenery)
        : _origin(origin)
        , _design(design)
        , _inspection(inspection)
        , _expectedCost(expectedCost)
        , _noMoney(getGameState().park.flags.has(ParkFlag::noMoney))
        , _placeScenery(placeScenery)
    {
    }
    bool SavedTrackDesignPlaceAction::Eligible(const TrackDesign& design)
    {
        const bool maze = design.trackAndVehicle.rtdIndex == RIDE_TYPE_MAZE;
        if (maze)
        {
            if (design.mazeElements.empty() || !design.trackElements.empty())
                return false;
        }
        else
        {
            if (!TrackConstructionSupport::Type(design.trackAndVehicle.rtdIndex) || design.trackElements.empty()
                || !design.mazeElements.empty())
                return false;
            const auto stations = std::count_if(
                design.trackElements.begin(), design.trackElements.end(),
                [](const auto& piece) { return piece.type == TrackElemType::endStation; });
            if (!stations || stations > Limits::kMaxStationsPerRide)
                return false;
        }
        const auto index = GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(design.trackAndVehicle.vehicleObject);
        const auto* entry = GetRideEntryByIndex(index);
        if (!TrackDesignPlacementContext::Compatible(design,index) || !entry
            || std::find(std::begin(entry->ride_type), std::end(entry->ride_type), design.trackAndVehicle.rtdIndex)
                == std::end(entry->ride_type)
            || FlatRidePlacement::StationObject() == kObjectEntryIndexNull)
            return false;
        return getGameState().cheats.ignoreResearchStatus
            || (RideTypeIsInvented(design.trackAndVehicle.rtdIndex) && RideEntryIsInvented(index));
    }
    void SavedTrackDesignPlaceAction::BindReview(const std::vector<int32_t>& context, const FileIdentity::Identity& source)
    {
        _objectContext=context; _sourcePath=source.path.u8string();
        _sourceDigest=source.digest; _sourceSize=source.size;
        _sourceModified=source.modified.time_since_epoch().count();
    }
    void SavedTrackDesignPlaceAction::AcceptParameters(GameActionParameterVisitor& visitor)
    {
        visitor.Visit(_origin);
        visitor.Visit("cost", _expectedCost);
        visitor.Visit("scenery", _placeScenery);
    }
    void SavedTrackDesignPlaceAction::Serialise(DataSerialiser& serialiser)
    {
        GameAction::Serialise(serialiser);
        if (serialiser.isLoading())
            _queryRide = RideId::GetNull();
        serialiser << DS_TAG(_origin) << DS_TAG(_inspection) << DS_TAG(_expectedCost) << DS_TAG(_noMoney)
                   << DS_TAG(_placeScenery) << DS_TAG(_objectContext) << DS_TAG(_sourcePath)
                   << DS_TAG(_sourceDigest) << DS_TAG(_sourceSize) << DS_TAG(_sourceModified);
        _design.Serialise(serialiser);
    }
    Result SavedTrackDesignPlaceAction::Query(GameState_t& state, Park::ParkData& park) const
    {
        bool sourceCurrent=true;
        if (!_sourcePath.empty()) {
            const auto current=FileIdentity::Capture(std::filesystem::u8path(_sourcePath));
            sourceCurrent=current && current->size==_sourceSize && current->digest==_sourceDigest
                && current->modified.time_since_epoch().count()==_sourceModified;
        }
        if (!sourceCurrent || (!_objectContext.empty() && _objectContext!=TrackDesignPlacementContext::Capture(_design)))
        {
            Result result; result.error=Status::disallowed; result.errorMessage="Design objects or file changed; review again"; return result;
        }
        if (!Eligible(_design) || (_expectedCost != kMoney64Undefined && park.flags.has(ParkFlag::noMoney) != _noMoney))
        {
            Result result;
            result.error = Status::disallowed;
            result.errorMessage = "Design unavailable / park rules changed; review again";
            return result;
        }
        Result result;
        if (!_queryRide.IsNull())
        {
            const auto index = GetContext()->GetObjectManager().GetLoadedObjectEntryIndex(
                _design.trackAndVehicle.vehicleObject);
            auto* ride = GetRide(_queryRide);
            if (!ride || ride->type != _design.trackAndVehicle.rtdIndex || ride->subtype != index)
                return Result(Status::invalidParameters, kStringIdNone, kStringIdNone);
            // Query the native track layout against the UI-owned provisional ride.
            // The outer action runner still applies pause and funds rules. Real
            // placement always uses TrackDesignAction after removing that record.
            result = TrackDesignPlace(_design, {}, _placeScenery, *ride, _origin);
            if (_trackDesignPlaceStateSceneryUnavailable)
                result = TrackDesignPlace(_design, {}, false, *ride, _origin);
        }
        else
        {
            auto action = TrackDesignAction(_origin, _design, _placeScenery, _inspection);
            action.SetFlags(GetFlags());
            result = QueryNested(&action, state);
        }
        if (result.error == Status::ok && _expectedCost != kMoney64Undefined && result.cost != _expectedCost)
        {
            result.error = Status::disallowed;
            result.errorMessage = "Placement cost changed; review again";
        }
        return result;
    }
    Result SavedTrackDesignPlaceAction::Execute(GameState_t& state, Park::ParkData& park) const
    {
        if (!_queryRide.IsNull())
            return Result(Status::invalidParameters, kStringIdNone, kStringIdNone);
        auto result = Query(state, park);
        if (result.error != Status::ok)
            return result;
        auto action = TrackDesignAction(_origin, _design, _placeScenery, _inspection);
        action.SetFlags(GetFlags());
        return ExecuteNested(&action, state);
    }
} // namespace OpenRCT2::GameActions

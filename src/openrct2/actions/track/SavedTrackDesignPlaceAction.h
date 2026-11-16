// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "TrackDesignAction.h"
#include "../../core/FileIdentity.h"
namespace OpenRCT2::GameActions
{
    // Strict object/research selection for Pocket; desktop design fallback stays unchanged.
    // All construction and cost calculation are delegated to TrackDesignAction.
    class SavedTrackDesignPlaceAction final : public GameActionBase<GameCommand::placeSavedTrackDesign>
    {
        CoordsXYZD _origin{};
        TrackDesign _design;
        RideInspection _inspection{ RideInspection::every30Minutes };
        money64 _expectedCost{ kMoney64Undefined };
        bool _noMoney{};
        bool _placeScenery{};
        int32_t _expectedSceneryOmitted{-1};
        std::vector<int32_t> _objectContext;
        std::string _sourcePath;
        uint64_t _sourceDigest{}, _sourceSize{};
        int64_t _sourceModified{};
        RideId _queryRide{ RideId::GetNull() }; // Transient native provisional record, never serialized.

    public:
        SavedTrackDesignPlaceAction() = default;
        SavedTrackDesignPlaceAction(
            CoordsXYZD origin, const TrackDesign& design, RideInspection inspection, money64 expectedCost = kMoney64Undefined,
            bool placeScenery = false);
        void BindReview(const std::vector<int32_t>& context, const FileIdentity::Identity& source, int32_t sceneryOmitted = -1);
        static bool Eligible(const TrackDesign& design);
        void UsePreviewRideForQuery(RideId ride)
        {
            _queryRide = ride;
        }
        void AcceptParameters(GameActionParameterVisitor&) final;
        void Serialise(DataSerialiser&) final;
        Result Query(GameState_t&, Park::ParkData&) const final;
        Result Execute(GameState_t&, Park::ParkData&) const final;
    };
} // namespace OpenRCT2::GameActions

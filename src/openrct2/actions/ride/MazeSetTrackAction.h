/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../GameAction.hpp"

namespace OpenRCT2::GameActions
{
    enum class MazeBuildMode : uint8_t
    {
        build,
        move,
        fill,
    };

    class MazeSetTrackAction final : public GameActionBase<GameCommand::setMazeTrack>
    {
    private:
        CoordsXYZD _loc;
        bool _initialPlacement{};
        RideId _rideIndex{ RideId::GetNull() };
        MazeBuildMode _mode{};
        bool _pureGhost{}, _clearPreview{};
        std::vector<int64_t> _context;
        money64 _quote{ kMoney64Undefined };

    public:
        MazeSetTrackAction() = default;
        MazeSetTrackAction(const CoordsXYZD& location, bool initialPlacement, RideId rideIndex, MazeBuildMode mode);

        void SetPureGhost(bool clear = false)
        {
            _pureGhost = true;
            _clearPreview = clear;
        }
        void GuardContext(std::vector<int64_t> context, money64 quote)
        {
            _context = std::move(context);
            _quote = quote;
        }
        void AcceptParameters(GameActionParameterVisitor&) final;
        void Serialise(DataSerialiser& stream) override;
        Result Query(GameState_t& gameState, Park::ParkData& park) const override;
        Result Execute(GameState_t& gameState, Park::ParkData& park) const override;

    private:
        uint8_t MazeGetSegmentBit(const CoordsXY&) const;
    };
} // namespace OpenRCT2::GameActions

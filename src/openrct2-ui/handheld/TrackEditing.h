// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "CustomTracks.h"
#include "RideManagement.h"

#include <openrct2/GameState.h>
#include <openrct2/actions/track/SequentialTrackPlaceAction.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/ride/Track.h>
#include <openrct2/ride/TrackConstructionSupport.h>
#include <openrct2/ride/TrackData.h>
#include <openrct2/ride/ted/TrackElementDescriptor.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>

namespace OpenRCT2::Ui::Handheld::TrackEditing
{
    struct Block
    {
        RideManagement::Target ride;
        CoordsXYZD origin;
        TrackElemType type;
        uint8_t speed, seat;
        bool chain, inverted;
        bool operator==(const Block&) const = default;
    };
    inline bool Eligible(const RideManagement::Target& target)
    {
        const auto* ride = RideManagement::Resolve(target);
        return ride && ride->status == RideStatus::closed && TrackConstructionSupport::Type(ride->type);
    }
    inline std::vector<Block> Blocks(const RideManagement::Target& target)
    {
        std::vector<Block> result;
        if (!Eligible(target))
            return result;
        const auto& state = getGameState();
        for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
            for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
                for (auto* e : TileElementsView<TrackElement>(CoordsXY{ x, y }))
                    if (!e->isGhost() && e->getRideIndex() == target.id && e->getSequenceIndex() == 0
                        && TrackConstructionSupport::Piece(e->getTrackType()))
                    {
                        const auto&
                            c = TrackMetadata::GetTrackElementDescriptor(e->getTrackType()).sequenceData.sequences[0].clearance;
                        const auto xy = CoordsXY{ x, y } - CoordsXY{ c.x, c.y }.rotate(e->getDirection());
                        result.push_back(
                            { target,
                              { xy, e->getBaseZ() - c.z, e->getDirection() },
                              e->getTrackType(),
                              e->getBrakeBoosterSpeed(),
                              e->getSeatRotation(),
                              e->hasChain(),
                              e->isInverted() });
                    }
        return result;
    }
    inline std::string Label(const Block& block)
    {
        auto name = CustomTracks::PieceName(block.type);
        return name + " / " + std::to_string(block.origin.x / 32) + "," + std::to_string(block.origin.y / 32) + " Z"
            + std::to_string(block.origin.z / 8);
    }
    inline bool Valid(const Block& block)
    {
        if (!Eligible(block.ride))
            return false;
        const auto& c = TrackMetadata::GetTrackElementDescriptor(block.type).sequenceData.sequences[0].clearance;
        const auto xy = CoordsXY(block.origin) + CoordsXY{ c.x, c.y }.rotate(block.origin.direction);
        if (!MapIsLocationValid(xy))
            return false;
        for (auto* e : TileElementsView<TrackElement>(xy))
            if (!e->isGhost() && e->getRideIndex() == block.ride.id && e->getTrackType() == block.type
                && e->getSequenceIndex() == 0 && e->getDirection() == block.origin.direction
                && e->getBaseZ() == block.origin.z + c.z && e->hasChain() == block.chain && e->isInverted() == block.inverted
                && e->getBrakeBoosterSpeed() == block.speed && e->getSeatRotation() == block.seat)
                return true;
        return false;
    }
    inline std::optional<Block> Previous(const Block& block)
    {
        if (!Valid(block))
            return {};
        const auto& c = TrackMetadata::GetTrackElementDescriptor(block.type).sequenceData.sequences[0].clearance;
        const auto xy = CoordsXY(block.origin) + CoordsXY{ c.x, c.y }.rotate(block.origin.direction);
        for (auto* e : TileElementsView<TrackElement>(xy))
            if (!e->isGhost() && e->getRideIndex() == block.ride.id && e->getTrackType() == block.type
                && e->getSequenceIndex() == 0 && e->getDirection() == block.origin.direction
                && e->getBaseZ() == block.origin.z + c.z)
            {
                const auto previous = TrackConstructionSupport::Previous({ xy, reinterpret_cast<TileElement*>(e) });
                if (previous)
                    return Block{ block.ride,     previous->origin, previous->type,    previous->speed,
                                  previous->seat, previous->chain,  previous->inverted };
            }
        return {};
    }
    inline bool CanReplace(const Block& block)
    {
        const auto previous = Previous(block);
        return previous && Valid(*previous) && block.origin != previous->origin && !block.inverted && !previous->inverted
            && block.type != TrackElemType::beginStation && block.type != TrackElemType::middleStation
            && block.type != TrackElemType::endStation;
    }
    inline GameActions::Result Replace(
        const Block& block, const Block& previous, money64 cost = kMoney64Undefined, bool execute = false,
        GameActions::GameAction::Callback_t callback = {})
    {
        if (!CanReplace(block) || !Valid(previous) || Previous(block) != previous)
        {
            GameActions::Result error;
            error.error = GameActions::Status::disallowed;
            error.errorMessage = "Block or predecessor changed; select again";
            return error;
        }
        GameActions::SequentialTrackPlaceAction action(
            block.ride.id, block.type, block.origin, block.chain, false, block.origin, block.type, cost);
        action.UseSelectedRemoval(block.speed, block.inverted, block.seat);
        action.GuardPrevious(
            { previous.origin, previous.type, previous.speed, previous.seat, previous.chain, previous.inverted });
        action.SetCallback(std::move(callback));
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    inline GameActions::Result Change(
        const Block& block, std::optional<uint8_t> speed, money64 cost = kMoney64Undefined, bool execute = false)
    {
        if (!Valid(block))
        {
            GameActions::Result result;
            result.error = GameActions::Status::disallowed;
            result.errorMessage = "Ride or selected block changed; select again";
            return result;
        }
        GameActions::SequentialTrackPlaceAction action(
            block.ride.id, block.type, block.origin, block.chain, false, block.origin, block.type, cost);
        if (speed)
            action.UseSpeedChange(block.speed, *speed, block.inverted, block.seat);
        else
            action.UseSelectedRemoval(block.speed, block.inverted, block.seat);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
} // namespace OpenRCT2::Ui::Handheld::TrackEditing

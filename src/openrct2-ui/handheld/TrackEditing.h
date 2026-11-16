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
        uint8_t colour{};
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
                              e->isInverted(), e->getColourScheme() });
                    }
        return result;
    }
    inline std::string Label(const Block& block)
    {
        auto name = CustomTracks::PieceName(block.type);
        return name;
    }
    inline bool Valid(const Block& block)
    {
        if (!Eligible(block.ride) || !TrackConstructionSupport::Piece(block.type) || block.origin.direction > 3)
            return false;
        const auto& sequences = TrackMetadata::GetTrackElementDescriptor(block.type).sequenceData;
        if (!sequences.numSequences) return false;
        for (uint8_t sequence = 0; sequence < sequences.numSequences; ++sequence)
        {
            const auto& c = sequences.sequences[sequence].clearance;
            const auto xy = CoordsXY(block.origin) + CoordsXY{ c.x, c.y }.rotate(block.origin.direction);
            if (!MapIsLocationValid(xy)) return false;
            bool found = false;
            for (auto* e : TileElementsView<TrackElement>(xy))
                if (!e->isGhost() && e->getRideIndex() == block.ride.id && e->getTrackType() == block.type
                    && e->getSequenceIndex() == sequence && e->getDirection() == block.origin.direction
                    && e->getBaseZ() == block.origin.z + c.z && e->hasChain() == block.chain && e->isInverted() == block.inverted
                    && e->getBrakeBoosterSpeed() == block.speed && e->getSeatRotation() == block.seat
                    && e->getColourScheme() == block.colour) { found = true; break; }
            if (!found) return false;
        }
        return true;
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
                {
                    const auto blocks = Blocks(block.ride);
                    const auto found = std::find_if(blocks.begin(), blocks.end(), [&](const auto& candidate) {
                        return candidate.origin == previous->origin && candidate.type == previous->type;
                    });
                    if (found != blocks.end()) return *found;
                }
            }
        return {};
    }
    inline std::optional<Block> Next(const Block& block)
    {
        if (!Valid(block)) return {};
        const auto& c=TrackMetadata::GetTrackElementDescriptor(block.type).sequenceData.sequences[0].clearance;
        const auto xy=CoordsXY(block.origin)+CoordsXY{c.x,c.y}.rotate(block.origin.direction);
        for(auto* e:TileElementsView<TrackElement>(xy))
            if(!e->isGhost() && e->getRideIndex()==block.ride.id && e->getTrackType()==block.type
                && e->getSequenceIndex()==0 && e->getDirection()==block.origin.direction && e->getBaseZ()==block.origin.z+c.z) {
                CoordsXYE input{xy,reinterpret_cast<TileElement*>(e)}, next{};
                int32_t z{}, direction{};
                if(!trackBlockGetNext(&input,&next,&z,&direction) || !TrackIsConnectedByShape(input.element,next.element))return {};
                auto* track=next.element->asTrack();
                const auto& offset=TrackMetadata::GetTrackElementDescriptor(track->getTrackType()).sequenceData.sequences[track->getSequenceIndex()].clearance;
                const auto origin=CoordsXYZD{CoordsXY{next.x,next.y}-CoordsXY{offset.x,offset.y}.rotate(track->getDirection()),track->getBaseZ()-offset.z,track->getDirection()};
                for(const auto& candidate:Blocks(block.ride))
                    if(candidate.origin==origin && candidate.type==track->getTrackType())return candidate;
            }
        return {};
    }
    inline std::vector<int32_t> Context(const Block& block)
    {
        const auto* ride = RideManagement::Resolve(block.ride);
        return Valid(block) && ride ? TrackConstructionSupport::EditingContext(*ride) : std::vector<int32_t>{};
    }
    inline std::pair<size_t, size_t> AccessRemoval(const Block& block)
    {
        std::pair<size_t, size_t> result{};
        if (!Valid(block) || (block.type != TrackElemType::beginStation && block.type != TrackElemType::middleStation
            && block.type != TrackElemType::endStation)) return result;
        for (auto delta : CoordsDirectionDelta)
            for (auto* e : TileElementsView<EntranceElement>(CoordsXY(block.origin) + delta))
                if (!e->isGhost() && e->getRideIndex() == block.ride.id && e->getBaseZ() == block.origin.z
                    && CoordsXY(block.origin) == CoordsXY(block.origin) + delta + CoordsDirectionDelta[e->getDirection()])
                {
                    if (e->getEntranceType() == EntranceType::rideEntrance) ++result.first;
                    if (e->getEntranceType() == EntranceType::rideExit) ++result.second;
                }
        return result;
    }
    inline std::string AccessRemovalLabel(const Block& block)
    {
        const auto [entrances, exits] = AccessRemoval(block);
        return "Deletes access here: " + std::to_string(entrances) + " entrance / " + std::to_string(exits) + " exit";
    }
    inline std::vector<std::string> Inspection(const Block& block, bool technical = false)
    {
        if (!Valid(block)) return { "Selected block no longer exists", "Back to block" };
        const auto& ted = TrackMetadata::GetTrackElementDescriptor(block.type);
        const auto& c = ted.sequenceData.sequences[0].clearance;
        const auto xy = CoordsXY(block.origin) + CoordsXY{ c.x, c.y }.rotate(block.origin.direction);
        auto previous = Previous(block);
        std::vector<std::string> rows{
            previous ? "Behind: " + Label(*previous) : "Behind: open end" };
        if (technical)
        {
            rows.push_back("Tile: " + std::to_string(block.origin.x / 32) + ", " + std::to_string(block.origin.y / 32));
            rows.push_back("Level: " + std::to_string(block.origin.z / kCoordsZStep));
            rows.push_back("Direction index: " + std::to_string(block.origin.direction));
            rows.push_back("Native sequences: " + std::to_string(ted.sequenceData.numSequences));
        }
        for (auto* e : TileElementsView<TrackElement>(xy))
            if (!e->isGhost() && e->getRideIndex() == block.ride.id && e->getTrackType() == block.type
                && e->getSequenceIndex() == 0 && e->getDirection() == block.origin.direction
                && e->getBaseZ() == block.origin.z + c.z)
            {
                CoordsXYE input{ xy, reinterpret_cast<TileElement*>(e) }, next{};
                int32_t z{}, direction{};
                const bool connected = trackBlockGetNext(&input, &next, &z, &direction)
                    && TrackIsConnectedByShape(input.element, next.element);
                rows.push_back(connected ? "Ahead: " + CustomTracks::PieceName(next.element->asTrack()->getTrackType())
                                         : "Ahead: open end");
                if (block.type == TrackElemType::beginStation || block.type == TrackElemType::middleStation
                    || block.type == TrackElemType::endStation)
                {
                    const auto index = e->getStationIndex();
                    const auto* ride = RideManagement::Resolve(block.ride);
                    if (!index.IsNull() && index.ToUnderlying() < ride->getStations().size())
                    {
                        const auto& station = ride->getStation(index);
                        rows.push_back(block.type == TrackElemType::beginStation ? "Station piece: Begin"
                            : block.type == TrackElemType::middleStation ? "Station piece: Middle" : "Station piece: End");
                        rows.push_back("Station " + std::to_string(index.ToUnderlying() + 1));
                        auto access = [technical](const auto& loc) {
                            if (loc.isNull()) return std::string("Needs building");
                            return !technical ? std::string("Built") : std::to_string(loc.x) + "," + std::to_string(loc.y)
                                + " Z" + std::to_string(loc.z) + " D" + std::to_string(loc.direction);
                        };
                        rows.push_back("Entrance: " + access(station.entrance));
                        rows.push_back("Exit: " + access(station.exit));
                        rows.push_back(AccessRemovalLabel(block));
                        rows.push_back("Removal may split the station");
                        rows.push_back("Check rebuilt entrance and exit");
                    }
                }
                break;
            }
        rows.push_back(technical ? "Hide technical details" : "Technical details...");
        rows.push_back("Back to piece");
        return rows;
    }
    inline bool CanReplace(const Block& block)
    {
        if (!Valid(block)) return false;
        const auto& c = TrackMetadata::GetTrackElementDescriptor(block.type).coordinates;
        return !(c.rotationBegin & 4) && !(c.rotationEnd & 4);
    }
    inline GameActions::Result Replace(
        const Block& block, const Block& previous, money64 cost = kMoney64Undefined, bool execute = false,
        GameActions::GameAction::Callback_t callback = {}, const std::vector<int32_t>& context = {})
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
        action.GuardEditingContext(context);
        action.SetCallback(std::move(callback));
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    inline GameActions::Result RemoveForReplacement(
        const Block& block, money64 cost = kMoney64Undefined, bool execute = false,
        GameActions::GameAction::Callback_t callback = {}, const std::vector<int32_t>& context = {})
    {
        if (!CanReplace(block))
        {
            GameActions::Result error;
            error.error = GameActions::Status::disallowed;
            error.errorMessage = "Selected replacement requires ordinary orthogonal endpoints";
            return error;
        }
        GameActions::SequentialTrackPlaceAction action(
            block.ride.id, block.type, block.origin, block.chain, false, block.origin, block.type, cost);
        action.UseSelectedRemoval(block.speed, block.inverted, block.seat);
        action.GuardEditingContext(context);
        action.SetCallback(std::move(callback));
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    inline GameActions::Result Change(
        const Block& block, std::optional<uint8_t> speed, money64 cost = kMoney64Undefined, bool execute = false, const std::vector<int32_t>& context = {})
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
        action.GuardEditingContext(context);
        if (speed)
            action.UseSpeedChange(block.speed, *speed, block.inverted, block.seat);
        else
            action.UseSelectedRemoval(block.speed, block.inverted, block.seat);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
} // namespace OpenRCT2::Ui::Handheld::TrackEditing

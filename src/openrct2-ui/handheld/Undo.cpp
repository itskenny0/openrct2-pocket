// SPDX-License-Identifier: GPL-3.0-only
#include "Undo.h"

#include <algorithm>
#include <optional>
#include <openrct2/GameState.h>
#include <openrct2/management/Finance.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/scenery/BannerRemoveAction.h>
#include <openrct2/actions/scenery/LargeSceneryRemoveAction.h>
#include <openrct2/actions/scenery/SmallSceneryRemoveAction.h>
#include <openrct2/actions/scenery/WallRemoveAction.h>
#include <openrct2/actions/terraform/LandSetHeightAction.h>
#include <openrct2/actions/terraform/WaterSetHeightAction.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/BannerElement.h>
#include <openrct2/world/tile_element/LargeSceneryElement.h>
#include <openrct2/world/tile_element/SmallSceneryElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <openrct2/world/tile_element/WallElement.h>
#include <vector>

namespace OpenRCT2::Ui::Handheld::Undo
{
    namespace
    {
        // Elements are matched by identity, never by raw bytes: the simulation changes element
        // bytes on its own, and a byte diff would mistake those for things the tool placed.
        struct Identity
        {
            TileElementType type;
            uint8_t baseHeight;
            uint32_t object;
            uint8_t detail; // quadrant, direction, sequence or banner position
            bool operator==(const Identity&) const = default;
        };
        struct Surface
        {
            uint8_t height, slope;
            int32_t water;
        };
        struct Tile
        {
            CoordsXY location;
            std::optional<Surface> surface;
            std::vector<Identity> elements;
        };
        struct Step
        {
            Kind kind{};
            std::string label;
            std::vector<Tile> tiles;
            money64 cost{};
        };
        std::optional<Step> pending, last;

        bool Tracked(Kind kind, TileElementType type)
        {
            switch (kind)
            {
                case Kind::trees: return type == TileElementType::smallScenery;
                case Kind::scenery:
                    return type == TileElementType::smallScenery || type == TileElementType::largeScenery
                        || type == TileElementType::wall || type == TileElementType::banner;
                case Kind::shop: return type == TileElementType::track;
                default: return false;
            }
        }
        std::optional<Identity> Identify(Kind kind, const TileElement& element)
        {
            const auto type = element.getType();
            if (!Tracked(kind, type) || element.isGhost())
                return std::nullopt;
            Identity id{ type, element.baseHeight, 0, 0 };
            if (const auto* small = element.asSmallScenery())
            {
                id.object = small->getEntryIndex();
                id.detail = small->getSceneryQuadrant();
            }
            else if (const auto* large = element.asLargeScenery())
            {
                id.object = large->getEntryIndex();
                id.detail = large->getSequenceIndex();
            }
            else if (const auto* wall = element.asWall())
            {
                id.object = wall->getEntryIndex();
                id.detail = wall->getDirection();
            }
            else if (const auto* banner = element.asBanner())
                id.detail = banner->getPosition();
            else if (const auto* track = element.asTrack())
                id.object = track->getRideIndex().ToUnderlying();
            return id;
        }
    } // namespace

    void Prepare(Kind kind, CoordsXY centre, int radius, std::string label)
    {
        Step step{ kind, std::move(label), {} };
        const TileCoordsXY middle(centre);
        for (int dy = -radius; dy <= radius; ++dy)
            for (int dx = -radius; dx <= radius; ++dx)
            {
                const CoordsXY location = TileCoordsXY{ middle.x + dx, middle.y + dy }.toCoordsXY();
                if (!MapIsLocationValid(location))
                    continue;
                Tile tile{ location, std::nullopt, {} };
                if (kind == Kind::land)
                {
                    if (const auto* surface = MapGetSurfaceElementAt(location))
                        tile.surface = Surface{ surface->baseHeight, surface->getSlope(), surface->getWaterHeight() };
                }
                else
                    for (const auto* element : TileElementsView(location))
                        if (const auto id = Identify(kind, *element))
                            tile.elements.push_back(*id);
                step.tiles.push_back(std::move(tile));
            }
        pending = std::move(step);
    }

    void Commit(money64 cost)
    {
        if (pending)
        {
            pending->cost = cost;
            last = std::move(pending);
        }
        pending.reset();
    }

    void Clear()
    {
        pending.reset();
        last.reset();
    }

    bool Available()
    {
        return last.has_value();
    }

    const std::string& Label()
    {
        static const std::string none;
        return last ? last->label : none;
    }

    GameActions::Result Perform()
    {
        GameActions::Result result;
        if (!last)
        {
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = "Nothing to undo";
            return result;
        }
        auto& state = getGameState();
        const auto step = std::move(*last);
        last.reset();
        size_t undone = 0;
        std::string message;
        money64 spent = 0;
        auto run = [&](GameActions::GameAction& action) {
            const auto outcome = GameActions::Execute(&action, state);
            if (outcome.error == GameActions::Status::ok)
            {
                ++undone;
                spent += outcome.cost;
            }
            else if (message.empty())
                message = outcome.getErrorMessage();
        };
        if (step.kind == Kind::land)
        {
            // Restore height and slope first, then water, which sits on the restored height.
            for (const auto& tile : step.tiles)
            {
                const auto* surface = MapGetSurfaceElementAt(tile.location);
                if (!surface || !tile.surface)
                    continue;
                if (surface->baseHeight != tile.surface->height || surface->getSlope() != tile.surface->slope)
                {
                    GameActions::LandSetHeightAction action(tile.location, tile.surface->height, tile.surface->slope);
                    run(action);
                }
            }
            for (const auto& tile : step.tiles)
            {
                const auto* surface = MapGetSurfaceElementAt(tile.location);
                if (surface && tile.surface && surface->getWaterHeight() != tile.surface->water)
                {
                    GameActions::WaterSetHeightAction action(tile.location, static_cast<uint8_t>(tile.surface->water / kCoordsZStep));
                    run(action);
                }
            }
        }
        else
        {
            std::vector<RideId> demolished;
            std::vector<uint32_t> removedLarge;
            for (const auto& tile : step.tiles)
            {
                // Collect first: removing changes the tile being iterated.
                std::vector<std::pair<Identity, CoordsXYZD>> added;
                for (const auto* element : TileElementsView(tile.location))
                    if (const auto id = Identify(step.kind, *element);
                        id && std::find(tile.elements.begin(), tile.elements.end(), *id) == tile.elements.end())
                        added.push_back({ *id, { tile.location, element->getBaseZ(), element->getDirection() } });
                for (const auto& [id, at] : added)
                {
                    switch (id.type)
                    {
                        case TileElementType::smallScenery:
                        {
                            GameActions::SmallSceneryRemoveAction action(at, id.detail, static_cast<ObjectEntryIndex>(id.object));
                            run(action);
                            break;
                        }
                        case TileElementType::largeScenery:
                        {
                            // One removal takes every tile of the object.
                            const uint32_t key = (id.object << 8) ^ static_cast<uint32_t>(at.z);
                            if (std::find(removedLarge.begin(), removedLarge.end(), key) != removedLarge.end())
                                break;
                            removedLarge.push_back(key);
                            GameActions::LargeSceneryRemoveAction action(at, id.detail);
                            run(action);
                            break;
                        }
                        case TileElementType::wall:
                        {
                            GameActions::WallRemoveAction action({ at, at.z, id.detail });
                            run(action);
                            break;
                        }
                        case TileElementType::banner:
                        {
                            GameActions::BannerRemoveAction action({ at, at.z, id.detail });
                            run(action);
                            break;
                        }
                        case TileElementType::track:
                        {
                            const auto ride = RideId::FromUnderlying(static_cast<uint16_t>(id.object));
                            const auto* placed = GetRide(ride);
                            if (!placed || !placed->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility)
                                || std::find(demolished.begin(), demolished.end(), ride) != demolished.end())
                                break;
                            demolished.push_back(ride);
                            // A queued demolition reports no cost until it runs, then credits the
                            // refund, so count the refund up front (already negative: it is the sum
                            // of the track removals' costs).
                            const auto refund = RideGetRefundPrice(*placed);
                            GameActions::RideDemolishAction action(ride, GameActions::RideModifyType::demolish);
                            const auto before = spent;
                            run(action);
                            if (spent == before)
                                spent += refund;
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
        // Undo returns the money too: the original charge and whatever putting it back cost (a
        // demolition's refund is negative, so it nets out). Booked as a negative payment so the
        // finance records still balance.
        if (undone > 0 && !state.park.flags.has(ParkFlag::noMoney))
            FinancePayment(-(step.cost + spent),
                step.kind == Kind::shop ? ExpenditureType::rideConstruction : ExpenditureType::landscaping);
        if (undone == 0 && message.empty())
            message = "Nothing left to undo here";
        result.error = undone > 0 ? GameActions::Status::ok : GameActions::Status::disallowed;
        result.errorMessage = undone > 0 ? "Undone: " + step.label : message;
        return result;
    }
} // namespace OpenRCT2::Ui::Handheld::Undo

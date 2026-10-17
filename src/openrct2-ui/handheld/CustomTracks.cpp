// SPDX-License-Identifier: GPL-3.0-only
#include "CustomTracks.h"

#include "Explorer.h"
#include "TrackPreview.h"

#include <openrct2-ui/interface/Window.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/track/SequentialTrackPlaceAction.h>
#include <openrct2/actions/track/TrackPlaceAction.h>
#include <openrct2/actions/track/TrackRemoveAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/RideObject.h>
#include <openrct2/ride/FlatRidePlacement.h>
#include <openrct2/ride/Track.h>
#include <openrct2/ride/TrackConstructionSupport.h>
#include <openrct2/ride/TrackData.h>
#include <openrct2/ride/TrackIteration.h>
#include <openrct2/ride/ted/TrackElementDescriptor.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <set>
namespace OpenRCT2::Ui::Handheld::CustomTracks
{
    namespace GA = GameActions;
    using namespace TrackMetadata;
    static bool active{}, busy{}, camera{}, chain{}, createdHere{}, backward{}, placementInverted{};
    static std::optional<TrackConstructionSupport::BlockSnapshot> sourceBaseline;
    static std::optional<TrackConstructionSupport::BlockSnapshot> replacement;
    static std::vector<int32_t> replacementContext;
    static uint8_t placementColour{};
    static int32_t stationOffset{};
    static uint8_t placementSpeed{ 6 }, placementSeat{ 4 };
    static uint32_t depth{};
    static uint64_t revision{};
    static std::optional<RideManagement::Target> target, completed;
    static std::optional<CoordsXYZD> endpoint;
    static TrackPitch pitch{};
    static TrackRoll roll{};
    static TrackElemType selected = TrackElemType::endStation;
    struct Block
    {
        CoordsXYZD origin;
        TrackElemType type;
        bool operator==(const Block&) const = default;
    };
    struct Preview
    {
        Block block;
        bool chain;
        uint8_t speed, seat;
        bool inverted;
        bool operator==(const Preview&) const = default;
    };
    static std::optional<Block> previous, anchor;
    static std::optional<Preview> ghost;
    static TrackPreview nativePreview;
    static std::string feedback;
    static GA::Result Invalid(const char* text)
    {
        GA::Result r;
        r.error = GA::Status::invalidParameters;
        r.errorMessage = text;
        return r;
    }
    bool Eligible(ObjectEntryIndex index, ride_type_t type)
    {
        return TrackConstructionSupport::ForObject(index, type) != kRideTypeNull
            && FlatRidePlacement::StationObject() != kObjectEntryIndexNull;
    }
    std::vector<PickerEntry> RideEntries()
    {
        std::vector<PickerEntry> result;
        for (ObjectEntryIndex i = 0; i < kMaxRideObjects; ++i)
            if (const auto* object = GetContext()->GetObjectManager().GetLoadedObject<RideObject>(i))
                for (auto type : object->GetEntry().ride_type)
                    if (Eligible(i, type) && type != kRideTypeNull)
                    {
                        const auto key = type == RIDE_TYPE_WOODEN_ROLLER_COASTER
                            ? "custom:" + std::to_string(i)
                            : "custom-type:" + std::to_string(type) + ":" + std::to_string(i);
                        const auto label = FmtString(FormatStringID(GetRideTypeDescriptor(type).Naming.Name))
                                               .WithoutFormatTokens();
                        result.push_back({ { ObjectType::ride, i, key }, label + " / " + object->GetName(), ZoomLevel{ 1 } });
                    }
        return result;
    }
    // Native real-track traversal is the authority for resume and circuit completion.
    // Retain coordinates/types only, never tile pointers across actions/map compaction.
    static bool Recover()
    {
        previous.reset();
        sourceBaseline.reset();
        pitch = TrackPitch::none;
        roll = TrackRoll::none;
        auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        if (!ride || !TrackConstructionSupport::Type(ride->type) || ride->status != RideStatus::closed)
            return false;
        std::optional<CoordsXYE> first;
        if (anchor)
        {
            const auto& clearance = GetTrackElementDescriptor(anchor->type).sequenceData.sequences[0].clearance;
            const auto tile = CoordsXY(anchor->origin) + CoordsXY{ clearance.x, clearance.y }.rotate(anchor->origin.direction);
            for (auto* e : TileElementsView<TrackElement>(tile))
                if (!e->isGhost() && e->getRideIndex() == ride->id && e->getSequenceIndex() == 0
                    && e->getTrackType() == anchor->type && e->getDirection() == anchor->origin.direction
                    && e->getBaseZ() == anchor->origin.z + clearance.z)
                {
                    first = CoordsXYE{ tile, reinterpret_cast<TileElement*>(e) };
                    break;
                }
            if (!first)
                return false;
        }
        for (const auto& station : ride->getStations())
            if (!first && !station.start.isNull())
            {
                auto* e = RideGetStationStartTrackElement(
                    *ride, StationIndex::FromUnderlying(&station - ride->getStations().data()));
                if (e && !e->isGhost())
                {
                    first = CoordsXYE{ station.start, e };
                    break;
                }
            }
        if (!first)
            for (int y = 32; y < getGameState().mapSize.y * 32 - 32 && !first; y += 32)
                for (int x = 32; x < getGameState().mapSize.x * 32 - 32 && !first; x += 32)
                    for (auto* e : TileElementsView<TrackElement>(CoordsXY{ x, y }))
                        if (!e->isGhost() && e->getRideIndex() == ride->id && e->getSequenceIndex() == 0)
                        {
                            first = CoordsXYE{ x, y, reinterpret_cast<TileElement*>(e) };
                            break;
                        }
        if (!first)
            return true;
        std::set<std::tuple<int, int, int, int>> seen;
        auto current = *first;
        while (true)
        {
            auto* e = current.element->asTrack();
            if (!seen.emplace(current.x, current.y, e->getBaseZ(), e->getDirection()).second)
            {
                endpoint.reset();
                completed = target;
                return true;
            }
            const auto& ted = GetTrackElementDescriptor(e->getTrackType());
            if (backward)
            {
                const auto snapshot = TrackConstructionSupport::Previous(current);
                if (!snapshot)
                {
                    TrackBeginEnd gap{};
                    if (trackBlockGetPrevious(current, &gap))
                        return false;
                    const auto& c = ted.sequenceData.sequences[e->getSequenceIndex()].clearance;
                    const auto origin = CoordsXY(current) - CoordsXY{ c.x, c.y }.rotate(e->getDirection());
                    sourceBaseline = TrackConstructionSupport::BlockSnapshot{ { origin, e->getBaseZ() - c.z,
                                                                                e->getDirection() },
                                                                              e->getTrackType(),
                                                                              e->getBrakeBoosterSpeed(),
                                                                              e->getSeatRotation(),
                                                                              e->hasChain(),
                                                                              e->isInverted() };
                    previous = Block{ sourceBaseline->origin, sourceBaseline->type };
                    endpoint = CoordsXYZD{ gap.end_x, gap.end_y, sourceBaseline->origin.z + ted.coordinates.zBegin,
                                           static_cast<Direction>(
                                               (((e->getDirection() + ted.coordinates.rotationBegin) & 3)
                                                | (ted.coordinates.rotationBegin & 4))
                                               ^ 2) };
                    pitch = ted.definition.pitchStart;
                    roll = TrackGetActualBank2(ride->type, e->isInverted(), ted.definition.rollStart);
                    placementInverted = e->isInverted();
                    return true;
                }
                const auto& c = GetTrackElementDescriptor(snapshot->type).sequenceData.sequences[0].clearance;
                const auto tile = CoordsXY(snapshot->origin) + CoordsXY{ c.x, c.y }.rotate(snapshot->origin.direction);
                current = { tile, nullptr };
                for (auto* track : TileElementsView<TrackElement>(tile))
                    if (!track->isGhost() && track->getRideIndex() == ride->id && track->getSequenceIndex() == 0
                        && track->getTrackType() == snapshot->type && track->getDirection() == snapshot->origin.direction
                        && track->getBaseZ() == snapshot->origin.z + c.z)
                    {
                        current.element = reinterpret_cast<TileElement*>(track);
                        break;
                    }
                if (!current.element)
                    return false;
                continue;
            }
            CoordsXYE next;
            int32_t z{}, direction{};
            const bool connected = trackBlockGetNext(&current, &next, &z, &direction);
            if (!connected)
            {
                if (next.x == kLocationNull)
                    return false;
                const auto& firstSequence = ted.sequenceData.sequences[0].clearance;
                const auto origin = CoordsXY(current) - CoordsXY{ firstSequence.x, firstSequence.y }.rotate(e->getDirection());
                previous = Block{ { origin, e->getBaseZ() - firstSequence.z, e->getDirection() }, e->getTrackType() };
                endpoint = CoordsXYZD{ next.x, next.y, z, static_cast<Direction>(direction) };
                sourceBaseline = TrackConstructionSupport::BlockSnapshot{
                    previous->origin,     previous->type, e->getBrakeBoosterSpeed(),
                    e->getSeatRotation(), e->hasChain(),  e->isInverted()
                };
                pitch = ted.definition.pitchEnd;
                roll = TrackGetActualBank2(ride->type, e->isInverted(), ted.definition.rollEnd);
                placementInverted = e->isInverted();
                return true;
            }
            if (!TrackIsConnectedByShape(current.element, next.element))
                return false;
            current = next;
        }
    }
    static void FocusEndpoint()
    {
        auto* main = WindowGetMain();
        if (endpoint && main && main->viewport)
            WindowScrollToLocation(*main, { endpoint->x + 16, endpoint->y + 16, endpoint->z });
    }
    bool Active()
    {
        return active;
    }
    bool Busy()
    {
        return busy;
    }
    bool CameraMode()
    {
        return camera;
    }
    bool Backward()
    {
        return active && backward;
    }
    bool StationPlacement()
    {
        return active && !previous && !completed && !replacement;
    }
    TrackElemType Selection()
    {
        return selected;
    }
    std::optional<CoordsXYZD> Endpoint()
    {
        return endpoint;
    }
    std::optional<RideManagement::Target> Target()
    {
        return target;
    }
    std::optional<RideManagement::Target> Completed()
    {
        return completed;
    }
    static bool Supported(TrackElemType type, bool variant = placementInverted)
    {
        if (!TrackConstructionSupport::Piece(type))
            return false;
        const auto& ted = GetTrackElementDescriptor(type);
        const auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        if (!ride || !Eligible(ride->subtype, ride->type)
            || !TrackConstructionSupport::Supported(*ride, *GetRideEntryByIndex(ride->subtype), type, variant)
            || ted.sequenceData.numSequences == 0)
            return false;
        // Loaded vehicle support and native TED endpoint geometry filter all
        // conventional special pieces, including inversions/helixes/photo.
        if (replacement)
            return !(ted.coordinates.rotationBegin & 4) && !(ted.coordinates.rotationEnd & 4);
        if (StationPlacement())
            return type == TrackElemType::endStation;
        return endpoint
            && (backward ? ted.definition.pitchEnd == pitch
                        && TrackGetActualBank2(ride->type, variant, ted.definition.rollEnd) == roll
                        && (ted.coordinates.rotationEnd & 4) == (endpoint->direction & 4)
                         : ted.definition.pitchStart == pitch
                        && TrackGetActualBank2(ride->type, variant, ted.definition.rollStart) == roll
                        && (ted.coordinates.rotationBegin & 4) == (endpoint->direction & 4));
    }
    bool PlacementInverted()
    {
        return active && placementInverted;
    }
    bool ToggleInverted()
    {
        const auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        if (!active || busy || !ride || !ride->getRideTypeDescriptor().flags.has(RtdFlag::hasInvertedVariant))
            return false;
        const auto old = placementInverted;
        placementInverted = !placementInverted;
        if (!Supported(selected))
        {
            auto alternative = TrackElemType::none;
            for (int i = 0; i < static_cast<int>(TrackElemType::count); ++i)
                if (Supported(static_cast<TrackElemType>(i)))
                {
                    alternative = static_cast<TrackElemType>(i);
                    break;
                }
            if (alternative == TrackElemType::none)
            {
                placementInverted = old;
                return false;
            }
            selected = alternative;
        }
        ClearPreview();
        chain = false;
        return true;
    }
    bool ToggleCovered()
    {
        if (!active || busy)
            return false;
        const auto base = TrackConstructionSupport::CoveredBase(selected);
        const auto next = base.value_or(GetTrackElementDescriptor(selected).alternativeType);
        if (next == TrackElemType::none || !Supported(next))
            return false;
        return Select(next);
    }
    std::string PieceName(TrackElemType type)
    {
        const auto& ted = GetTrackElementDescriptor(type);
        if (const auto base = TrackConstructionSupport::CoveredBase(type))
            return PieceName(*base) + " / covered";
        if (type == TrackElemType::endStation || type == TrackElemType::beginStation || type == TrackElemType::middleStation)
            return "Station";
        const auto native = FormatStringIDLegacy(ted.description, nullptr);
        if (!native.empty())
            return native;
        const auto& d = ted.definition;
        auto slopeName = [](TrackPitch value) -> std::string {
            switch (value)
            {
                case TrackPitch::up25:
                    return "Up 25";
                case TrackPitch::up60:
                    return "Up 60";
                case TrackPitch::down25:
                    return "Down 25";
                case TrackPitch::down60:
                    return "Down 60";
                default:
                    return "Level";
            }
        };
        const int turn = (ted.coordinates.rotationEnd - ted.coordinates.rotationBegin) & 3;
        if (turn == 1 || turn == 3)
            return std::string(ted.flags.has(TrackElementFlag::turnLeft) ? "Left turn" : "Right turn")
                + (d.rollEnd != TrackRoll::none ? " / bank" : "")
                + (d.pitchEnd != TrackPitch::none ? " / " + slopeName(d.pitchEnd) : "");
        if (d.pitchStart != d.pitchEnd)
            return slopeName(d.pitchStart) + " > " + slopeName(d.pitchEnd);
        if (d.rollStart != d.rollEnd)
            return d.rollEnd == TrackRoll::none ? "Bank > level" : d.rollEnd == TrackRoll::left ? "Bank left" : "Bank right";
        if (d.rollEnd != TrackRoll::none)
            return d.rollEnd == TrackRoll::left ? "Left bank" : "Right bank";
        if (d.pitchEnd != TrackPitch::none)
            return slopeName(d.pitchEnd);
        return (ted.coordinates.rotationBegin & 4) ? "Diagonal straight" : "Straight";
    }
    // Shared native-metadata preview: live picker and component validation use the same renderer.
    void DrawPiecePreview(Drawing::RenderTarget& rt, TrackElemType type)
    {
        const auto& ted = GetTrackElementDescriptor(type);
        int minX = 0, maxX = 0, minY = 0, maxY = 0;
        for (int n = 0; n < ted.sequenceData.numSequences; ++n)
        {
            auto c = ted.sequenceData.sequences[n].clearance;
            minX = std::min<int>(minX, c.x);
            maxX = std::max<int>(maxX, c.x);
            minY = std::min<int>(minY, c.y);
            maxY = std::max<int>(maxY, c.y);
        }
        const int scale = std::max(32, std::max(maxX - minX, maxY - minY) + 32);
        for (int n = 0; n < ted.sequenceData.numSequences; ++n)
        {
            auto c = ted.sequenceData.sequences[n].clearance;
            Concept::DrawIcon(
                rt, { rt.x + 4 + (c.x - minX) * 40 / scale, rt.y + 12 + (c.y - minY) * 40 / scale }, Concept::Icon::path);
        }
        DrawModernText(rt, { rt.x + 4, rt.y + 48 }, "{MUTED}" + std::to_string(ted.sequenceData.numSequences) + " tiles");
    }
    std::vector<PickerEntry> Pieces()
    {
        std::vector<PickerEntry> result;
        for (int i = 0; i < static_cast<int>(TrackElemType::count); ++i)
        {
            auto type = static_cast<TrackElemType>(i);
            if (!Supported(type) || type == TrackElemType::beginStation || type == TrackElemType::middleStation)
                continue;
            PickerEntry entry{ { ObjectType::ride, kObjectEntryIndexNull, std::to_string(i) },
                               PieceName(type),
                               ZoomLevel{ 0 } };
            entry.drawPreview = [type](Drawing::RenderTarget& rt) { DrawPiecePreview(rt, type); };
            result.push_back(std::move(entry));
        }
        const auto coveredBase = TrackConstructionSupport::CoveredBase(selected);
        const auto coveredNext = coveredBase.value_or(GetTrackElementDescriptor(selected).alternativeType);
        if (coveredNext != TrackElemType::none && Supported(coveredNext))
            result.push_back(
                { { ObjectType::ride, kObjectEntryIndexNull, "covered" },
                  coveredBase ? "Open track" : "Covered track",
                  ZoomLevel{ 0 } });
        bool alternativeAvailable = false;
        if (const auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
            ride && ride->getRideTypeDescriptor().flags.has(RtdFlag::hasInvertedVariant))
            for (int i = 0; i < static_cast<int>(TrackElemType::count); ++i)
                alternativeAvailable |= Supported(static_cast<TrackElemType>(i), !placementInverted);
        if (alternativeAvailable)
            result.push_back(
                { { ObjectType::ride, kObjectEntryIndexNull, "inverted" },
                  placementInverted ? "Use normal variant" : "Use inverted variant",
                  ZoomLevel{ 0 } });
        if (SeatSupported())
            for (const auto& option : { std::pair{ "seat-minus", "Seat -" }, std::pair{ "seat-plus", "Seat +" },
                                        std::pair{ "seat-reset", "Seat default" } })
                result.push_back(
                    { { ObjectType::ride, kObjectEntryIndexNull, option.first },
                      std::string(option.second) + " (" + std::to_string(placementSeat) + ")",
                      ZoomLevel{ 0 } });
        result.push_back(
            { { ObjectType::ride, kObjectEntryIndexNull, "camera" },
              camera ? "Piece controls" : "Camera controls",
              ZoomLevel{ 0 },
              [](Drawing::RenderTarget& rt) { Concept::DrawIcon(rt, { rt.x + 20, rt.y + 22 }, Concept::Icon::play); } });
        result.push_back(
            { { ObjectType::ride, kObjectEntryIndexNull, "leave" },
              "Return to ride",
              ZoomLevel{ 0 },
              [](Drawing::RenderTarget& rt) { Concept::DrawIcon(rt, { rt.x + 20, rt.y + 22 }, Concept::Icon::play); } });
        return result;
    }
    static bool ChainAllowed()
    {
        const auto flags = GetTrackElementDescriptor(selected).flags;
        return flags.has(TrackElementFlag::allowLiftHill) || flags.has(TrackElementFlag::curveAllowsLift);
    }
    void ClearPreview()
    {
        ghost.reset();
        nativePreview.clear();
    }
    void End()
    {
        ClearPreview();
        bool hasRealTrack = false;
        if (target)
            for (int y = 32; y < getGameState().mapSize.y * 32 - 32 && !hasRealTrack; y += 32)
                for (int x = 32; x < getGameState().mapSize.x * 32 - 32 && !hasRealTrack; x += 32)
                    for (auto* e : TileElementsView<TrackElement>(CoordsXY{ x, y }))
                        hasRealTrack |= !e->isGhost() && e->getRideIndex() == target->id;
        const auto empty = createdHere && target && !hasRealTrack;
        if (empty && RideManagement::Resolve(*target))
        {
            GA::RideDemolishAction remove(target->id, GA::RideModifyType::demolish);
            remove.SetFlags({ GA::CommandFlag::allowDuringPaused });
            GA::Execute(&remove, getGameState());
        }
        replacement.reset();
        replacementContext.clear();
        placementColour = 0;
        createdHere = false;
        stationOffset = 0;
        placementSpeed = 6;
        placementSeat = 4;
        active = false;
        busy = false;
        target.reset();
        completed.reset();
        endpoint.reset();
        previous.reset();
        anchor.reset();
        depth = 0;
        camera = false;
        backward = false;
        placementInverted = false;
        sourceBaseline.reset();
        ++revision;
    }
    bool Begin(ObjectEntryIndex index, ride_type_t requested)
    {
        const auto type = TrackConstructionSupport::ForObject(index, requested);
        if (!Eligible(index, type) || type == kRideTypeNull)
            return false;
        End();
        const auto entrance = FlatRidePlacement::StationObject();
        GA::RideCreateAction create(type, index, 0, 0, entrance, RideInspection::every30Minutes);
        const auto q = GA::Query(&create, getGameState());
        if (q.error != GA::Status::ok)
            return false;
        active = true;
        busy = true;
        createdHere = true;
        selected = TrackElemType::endStation;
        placementInverted = GetRideTypeDescriptor(type).flags.has(RtdFlag::startConstructionInverted);
        chain = false;
        feedback.clear();
        const auto token = revision;
        create.SetCallback([token](const auto*, const GA::Result* result) {
            if (result->error != GA::Status::ok)
            {
                if (token == revision)
                {
                    busy = false;
                    active = false;
                }
                return;
            }
            const auto id = result->getData<RideId>();
            if (token != revision)
            {
                GA::RideDemolishAction remove(id, GA::RideModifyType::demolish);
                remove.SetFlags({ GA::CommandFlag::allowDuringPaused });
                GA::Execute(&remove, getGameState());
                return;
            }
            target = RideManagement::Identify(id);
            busy = false;
        });
        const auto r = GA::Execute(&create, getGameState());
        if (r.error != GA::Status::ok)
        {
            End();
            return false;
        }
        return true;
    }
    bool CanResume(const RideManagement::Target& t)
    {
        const auto* ride = RideManagement::Resolve(t);
        if (!ride || !TrackConstructionSupport::Type(ride->type) || ride->status != RideStatus::closed
            || !Eligible(ride->subtype, ride->type))
            return false;
        // A native gap, rather than a Pocket save marker, identifies an unfinished course.
        for (const auto& station : ride->getStations())
            if (!station.start.isNull())
            {
                auto* e = RideGetStationStartTrackElement(
                    *ride, StationIndex::FromUnderlying(&station - ride->getStations().data()));
                CoordsXYE gap;
                return e && findTrackGap(*ride, { station.start, e }, &gap);
            }
        return true;
    }
    bool Resume(const RideManagement::Target& t)
    {
        if (!CanResume(t))
            return false;
        return ResumeFrom(t, std::nullopt);
    }
    bool ResumeFrom(const RideManagement::Target& t, std::optional<std::pair<CoordsXYZD, TrackElemType>> source, bool reverse)
    {
        const auto* ride = RideManagement::Resolve(t);
        if (!ride || ride->status != RideStatus::closed || !Eligible(ride->subtype, ride->type))
            return false;
        if (source
            && (source->second >= TrackElemType::count
                || GetTrackElementDescriptor(source->second).sequenceData.numSequences == 0))
            return false;
        End();
        target = t;
        backward = reverse;
        if (source)
            anchor = Block{ source->first, source->second };
        active = true;
        selected = TrackElemType::flat;
        chain = false;
        feedback.clear();
        if (!Recover() || completed)
        {
            End();
            return false;
        }
        FocusEndpoint();
        if (!Supported(selected))
            for (const auto& entry : Pieces())
                if (entry.identity.key.front() >= '0' && entry.identity.key.front() <= '9')
                {
                    selected = static_cast<TrackElemType>(std::stoi(entry.identity.key));
                    break;
                }
        return true;
    }
    bool Replacing() { return active && replacement.has_value(); }
    bool ResumeReplacement(const RideManagement::Target& old,
        const TrackConstructionSupport::BlockSnapshot& removed, uint8_t colour)
    {
        const auto now = RideManagement::Identify(old.id);
        if (!now || now->id != old.id || now->type != old.type || now->object != old.object
            || now->buildDate != old.buildDate || now->generation != old.generation
            || !Eligible(now->object, now->type)) return false;
        const auto* ride = RideManagement::Resolve(*now);
        if (!ride || ride->status != RideStatus::closed) return false;
        End();
        target = *now;
        replacement = removed;
        replacementContext = TrackConstructionSupport::EditingContext(*ride);
        endpoint = removed.origin;
        selected = removed.type == TrackElemType::beginStation || removed.type == TrackElemType::middleStation
            ? TrackElemType::endStation : removed.type;
        placementInverted = removed.inverted;
        placementSpeed = removed.speed;
        placementSeat = removed.seat;
        placementColour = colour;
        chain = removed.chain;
        active = true;
        feedback.clear();
        if (!Supported(selected)) { End(); return false; }
        FocusEndpoint();
        return true;
    }
    bool Select(TrackElemType type)
    {
        if (type >= TrackElemType::count)
            return false;
        if (!active || busy || !Supported(type))
            return false;
        ClearPreview();
        selected = type;
        if (!ChainAllowed())
            chain = false;
        feedback.clear();
        return true;
    }
    void Cycle(int direction)
    {
        auto entries = Pieces();
        std::erase_if(entries, [](const auto& entry) {
            return entry.identity.key.empty() || entry.identity.key.front() < '0' || entry.identity.key.front() > '9';
        });
        if (entries.empty())
            return;
        size_t index = 0;
        for (size_t i = 0; i < entries.size(); ++i)
            if (std::stoi(entries[i].identity.key) == static_cast<int>(selected))
                index = i;
        index = (index + entries.size() + (direction > 0 ? 1 : -1)) % entries.size();
        Select(static_cast<TrackElemType>(std::stoi(entries[index].identity.key)));
    }
    void Slope(int direction)
    {
        if (StationPlacement())
        {
            ClearPreview();
            stationOffset = std::clamp(stationOffset + (direction > 0 ? 8 : -8), 0, 256);
            return;
        }
        const std::array slopes{ TrackPitch::down60, TrackPitch::down25, TrackPitch::none, TrackPitch::up25, TrackPitch::up60 };
        const auto current = backward ? GetTrackElementDescriptor(selected).definition.pitchStart
                                      : GetTrackElementDescriptor(selected).definition.pitchEnd;
        auto it = std::find(slopes.begin(), slopes.end(), current);
        int index = it == slopes.end() ? 2 : it - slopes.begin();
        const auto desired = slopes[std::clamp(index + (direction > 0 ? 1 : -1), 0, 4)];
        for (const auto& e : Pieces())
            if (e.identity.key.front() >= '0' && e.identity.key.front() <= '9')
            {
                auto type = static_cast<TrackElemType>(std::stoi(e.identity.key));
                const auto& ted = GetTrackElementDescriptor(type);
                if ((backward ? ted.definition.pitchStart : ted.definition.pitchEnd) == desired
                    && (backward ? ted.definition.rollStart : ted.definition.rollEnd) == roll
                    && !ted.flags.has(TrackElementFlag::turnLeft) && !ted.flags.has(TrackElementFlag::turnRight))
                {
                    Select(type);
                    return;
                }
            }
    }
    uint8_t PlacementSpeed()
    {
        return placementSpeed;
    }
    bool SetPlacementSpeed(uint8_t speed)
    {
        if (!active || busy || !trackTypeHasSpeedSetting(selected) || speed < 2 || speed > 30)
            return false;
        ClearPreview();
        placementSpeed = speed;
        return true;
    }
    bool SeatSupported()
    {
        const auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        return active && ride && TrackConstructionSupport::SeatRotation(ride->type, selected);
    }
    uint8_t PlacementSeat()
    {
        return placementSeat;
    }
    bool SetPlacementSeat(uint8_t seat)
    {
        if (!SeatSupported() || busy || seat > 15)
            return false;
        ClearPreview();
        placementSeat = seat;
        return true;
    }
    void Bank()
    {
        if (trackTypeHasSpeedSetting(selected))
        {
            SetPlacementSpeed(std::min<int>(30, placementSpeed + 2));
            return;
        }
        const std::array banks{ TrackRoll::none, TrackRoll::left, TrackRoll::right };
        const auto current = backward ? GetTrackElementDescriptor(selected).definition.rollStart
                                      : GetTrackElementDescriptor(selected).definition.rollEnd;
        auto it = std::find(banks.begin(), banks.end(), current);
        size_t index = it == banks.end() ? 0 : it - banks.begin();
        for (size_t offset = 1; offset <= banks.size(); ++offset)
            for (const auto& e : Pieces())
                if (e.identity.key.front() >= '0' && e.identity.key.front() <= '9')
                {
                    auto type = static_cast<TrackElemType>(std::stoi(e.identity.key));
                    const auto& ted = GetTrackElementDescriptor(type);
                    if ((backward ? ted.definition.rollStart : ted.definition.rollEnd) == banks[(index + offset) % banks.size()]
                        && (backward ? ted.definition.pitchStart : ted.definition.pitchEnd)
                            == (backward ? GetTrackElementDescriptor(selected).definition.pitchStart
                                         : GetTrackElementDescriptor(selected).definition.pitchEnd)
                        && !ted.flags.has(TrackElementFlag::turnLeft) && !ted.flags.has(TrackElementFlag::turnRight))
                    {
                        Select(type);
                        return;
                    }
                }
    }
    void Chain()
    {
        if (trackTypeHasSpeedSetting(selected))
        {
            SetPlacementSpeed(std::max<int>(2, placementSpeed - 2));
            return;
        }
        if (!busy && ChainAllowed())
        {
            ClearPreview();
            chain = !chain;
        }
    }
    void ToggleCamera()
    {
        camera = !camera;
    }
    void Rotate()
    {
        if (StationPlacement() && endpoint)
        {
            ClearPreview();
            endpoint->direction = (endpoint->direction + 1) & 3;
        }
    }
    static std::optional<Block> Request()
    {
        if (!active || !target || !endpoint || !Supported(selected))
            return {};
        if (replacement) return Block{ replacement->origin, selected };
        if (backward)
            return Block{ TrackConstructionSupport::BackwardOrigin(*endpoint, selected), selected };
        auto origin = *endpoint;
        const auto& coords = GetTrackElementDescriptor(selected).coordinates;
        origin.direction = (origin.direction - coords.rotationBegin) & 3;
        origin.z -= coords.zBegin;
        return Block{ origin, selected };
    }
    GameActions::Result Action(bool execute)
    {
        const auto request = Request();
        auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        if (!request || !ride || ride->status != RideStatus::closed || busy)
            return Invalid("Construction unavailable");
        const auto source = previous.value_or(Block{});
        auto query = GA::SequentialTrackPlaceAction(
            target->id, request->type, request->origin, chain, !previous, source.origin, source.type);
        query.SetPlacementSpeed(trackTypeHasSpeedSetting(request->type) ? placementSpeed : 6);
        query.SetPlacementSeat(SeatSupported() ? placementSeat : 0);
        query.SetPlacementInverted(placementInverted);
        query.SetPlacementColour(placementColour);
        if (replacement) query.UseDetached(replacementContext);
        if (sourceBaseline && !backward)
            query.GuardSource(*sourceBaseline);
        if (backward)
        {
            if (!sourceBaseline)
                return Invalid("Backward source unavailable");
            query.UseBackward(*sourceBaseline);
        }
        auto q = GA::Query(&query, getGameState());
        if (!execute || q.error != GA::Status::ok)
            return q;
        auto action = GA::SequentialTrackPlaceAction(
            target->id, request->type, request->origin, chain, !previous, source.origin, source.type, q.cost);
        action.SetPlacementSpeed(trackTypeHasSpeedSetting(request->type) ? placementSpeed : 6);
        action.SetPlacementSeat(SeatSupported() ? placementSeat : 0);
        action.SetPlacementInverted(placementInverted);
        action.SetPlacementColour(placementColour);
        if (replacement) action.UseDetached(replacementContext);
        if (sourceBaseline && !backward)
            action.GuardSource(*sourceBaseline);
        if (backward)
            action.UseBackward(*sourceBaseline);
        ClearPreview();
        busy = true;
        const auto token = revision;
        action.SetCallback([token, placed = *request](const auto*, const GA::Result* result) {
            if (token != revision)
                return;
            busy = false;
            feedback = result->error == GA::Status::ok ? "" : result->getErrorMessage();
            if (result->error == GA::Status::ok)
            {
                target = RideManagement::Identify(target->id);
                if (replacement)
                {
                    anchor = placed;
                    const auto& c = GetTrackElementDescriptor(placed.type).sequenceData.sequences[0].clearance;
                    const auto xy = CoordsXY(placed.origin) + CoordsXY{ c.x, c.y }.rotate(placed.origin.direction);
                    for (auto* track : TileElementsView<TrackElement>(xy))
                        if (!track->isGhost() && track->getRideIndex() == target->id && track->getSequenceIndex() == 0
                            && track->getBaseZ() == placed.origin.z + c.z && track->getDirection() == placed.origin.direction)
                        { anchor->type = track->getTrackType(); break; }
                    replacement.reset();
                    replacementContext.clear();
                }
                if (!Recover())
                {
                    End();
                    return;
                }
                if (!completed)
                    FocusEndpoint();
                if (!completed && !Supported(selected))
                {
                    selected = TrackElemType::flat;
                    for (const auto& e : Pieces())
                        if (e.identity.key.front() >= '0' && e.identity.key.front() <= '9')
                        {
                            auto t = static_cast<TrackElemType>(std::stoi(e.identity.key));
                            if (t == TrackElemType::flat)
                            {
                                selected = t;
                                break;
                            }
                            if (!Supported(selected))
                                selected = t;
                        }
                }
            }
        });
        auto r = GA::Execute(&action, getGameState());
        if (r.error != GA::Status::ok)
            busy = false;
        return r;
    }
    GameActions::Result Undo(bool execute)
    {
        ClearPreview();
        if (replacement) return Invalid("Rebuild cancelled only through B / Return to park");
        if (!previous || !target || busy)
            return Invalid("No preceding track piece");
        const auto block = *previous;
        auto query = GA::SequentialTrackPlaceAction(
            target->id, block.type, block.origin, false, false, block.origin, block.type);
        query.UseRemoval();
        query.SetPlacementInverted(sourceBaseline ? sourceBaseline->inverted : placementInverted);
        if (sourceBaseline && !backward)
            query.GuardSource(*sourceBaseline);
        if (backward && sourceBaseline)
            query.UseBackward(*sourceBaseline);
        if (backward)
        {
            if (!sourceBaseline)
                return Invalid("Backward source unavailable");
            query.UseBackward(*sourceBaseline);
        }
        auto q = GA::Query(&query, getGameState());
        if (!execute || q.error != GA::Status::ok)
            return q;
        auto remove = GA::SequentialTrackPlaceAction(
            target->id, block.type, block.origin, false, false, block.origin, block.type, q.cost);
        remove.UseRemoval();
        remove.SetPlacementInverted(sourceBaseline ? sourceBaseline->inverted : placementInverted);
        if (sourceBaseline && !backward)
            remove.GuardSource(*sourceBaseline);
        if (backward && sourceBaseline)
            remove.UseBackward(*sourceBaseline);
        const auto token = revision;
        busy = true;
        remove.SetCallback([token](const auto*, const GA::Result* result) {
            if (token != revision)
                return;
            busy = false;
            feedback = result->error == GA::Status::ok ? "" : result->getErrorMessage();
            if (result->error == GA::Status::ok)
            {
                const auto removed = previous;
                target = RideManagement::Identify(target->id);
                if (!Recover())
                {
                    End();
                    return;
                }
                if (!previous && removed)
                    endpoint = removed->origin;
                selected = TrackElemType::flat;
                if (!previous)
                    selected = TrackElemType::endStation;
                if (!Supported(selected))
                    for (const auto& entry : Pieces())
                        if (entry.identity.key.front() >= '0' && entry.identity.key.front() <= '9')
                        {
                            selected = static_cast<TrackElemType>(std::stoi(entry.identity.key));
                            break;
                        }
                chain = false;
                FocusEndpoint();
            }
        });
        auto r = GA::Execute(&remove, getGameState());
        if (r.error != GA::Status::ok)
            busy = false;
        return r;
    }
    void Update(std::optional<CoordsXY> tile)
    {
        if (!active || busy)
            return;
        if (!target || !RideManagement::Resolve(*target)
            || !Eligible(RideManagement::Resolve(*target)->subtype, RideManagement::Resolve(*target)->type))
        {
            End();
            return;
        }
        if (StationPlacement())
        {
            const auto direction = endpoint ? endpoint->direction : 0;
            if (tile && MapGetSurfaceElementAt(*tile))
                endpoint = CoordsXYZD{ *tile, MapGetHighestZ(*tile) + stationOffset, static_cast<Direction>(direction) };
            else
                endpoint.reset();
        }
        if (depth || busy || completed)
            return;
        const auto request = Request();
        const auto q = Action(false);
        if (!request || q.error != GA::Status::ok)
        {
            ClearPreview();
            return;
        }
        const Preview next{ *request, chain, trackTypeHasSpeedSetting(request->type) ? placementSpeed : uint8_t{ 6 },
                            SeatSupported() ? placementSeat : uint8_t{ 0 }, placementInverted };
        if (ghost && *ghost == next)
            return;
        ClearPreview();
        SelectedLiftAndInverted flags{};
        if (chain)
            flags.set(LiftHillAndInverted::liftHill);
        if (placementInverted)
            flags.set(LiftHillAndInverted::inverted);
        auto* ride = RideManagement::Resolve(*target);
        if (nativePreview.place(ride->subtype, ride->type, request->origin, request->type, flags, next.speed, next.seat))
            ghost = next;
    }
    void RemovePreviewTemporarily()
    {
        ++depth;
        ClearPreview();
    }
    void RestorePreview()
    {
        if (depth)
            --depth;
    }
    void DrawPanel(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        using namespace Concept;
        const auto p = kOrigin;
        if (completed)
        {
            Box(rt, p, { 164, 76 });
            Heading(rt, p + ScreenCoordsXY{ 10, 12 }, "Circuit complete");
            DrawModernText(rt, p + ScreenCoordsXY{ 10, 46 }, "{MUTED}Entrance / exit next");
            DrawActionStrip(rt, screen);
            return;
        }
        Box(rt, p, { 216, 254 });
        Heading(
            rt, p + ScreenCoordsXY{ 10, 12 },
            replacement ? "Rebuild removed"
                : StationPlacement() ? "Place station"
                : backward     ? "Build backward"
                               : "Build track");
        Rule(rt, p + ScreenCoordsXY{ 8, 38 }, 184);
        Box(rt, p + ScreenCoordsXY{ 8, 48 }, { 200, 40 }, true);
        DrawWrappedLabel(rt, p + ScreenCoordsXY{ 14, 54 }, 188, "{BLACK}" + PieceName(selected), 2);
        const auto& d = GetTrackElementDescriptor(selected).definition;
        const auto displayPitch = backward ? d.pitchStart : d.pitchEnd;
        const auto displayRoll = backward ? d.rollStart : d.rollEnd;
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 98 },
            "{WHITE}Slope "
                + std::string(
                    displayPitch == TrackPitch::none         ? "Level"
                        : displayPitch == TrackPitch::up25   ? "Up 25"
                        : displayPitch == TrackPitch::up60   ? "Up 60"
                        : displayPitch == TrackPitch::down25 ? "Down 25"
                        : displayPitch == TrackPitch::down60 ? "Down 60"
                                                             : "Special"));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 120 },
            trackTypeHasSpeedSetting(selected) ? "{MUTED}Speed "
                    + FormatStringID(STR_VELOCITY, static_cast<uint16_t>((placementSpeed * 9) >> 2))
                                               : "{WHITE}Bank "
                    + std::string(displayRoll == TrackRoll::none        ? "Level"
                                      : displayRoll == TrackRoll::left  ? "Left"
                                      : displayRoll == TrackRoll::right ? "Right"
                                                                        : "Inverted"));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 142 },
            std::string("{MUTED}Chain ") + (ChainAllowed() ? (chain ? "On" : "Off") : "Unavailable"));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 164 }, "{WHITE}Height " + (endpoint ? std::to_string(endpoint->z / 8) : "Unavailable"));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 206 },
            camera ? "{GOLD}CAMERA / B Build"
                : SeatSupported()
                ? "{MUTED}Seat " + std::to_string(placementSeat) + (placementInverted ? " / Inv" : " / SEL options")
                : placementInverted ? "{WHITE}Inverted / START Ride"
                                    : "{WHITE}START Ride / SEL Pieces");
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 230 }, camera ? "{WHITE}D-pad Pan / B Build" : "{WHITE}L2/R2 Slope / Select Pieces");
        static const char* directions[]{"West","South","East","North"};
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 186 },
            endpoint ? "{WHITE}" + std::string(directions[endpoint->direction & 3]) : "{WHITE}Choose origin");
        Drawing::RenderTarget preview;
        if (ClipRenderTarget(preview, rt, p + ScreenCoordsXY{ 146, 135 }, 60, 62))
            DrawPiecePreview(preview, selected);
        DrawModeBadge(rt, p + ScreenCoordsXY{ 144, 8 }, camera ? "CAMERA" : "BUILD");
        std::vector<ControlHint> hints{ { "D-pad", "Move" },
                                        { "A", camera ? "Build mode" : "Build" },
                                        { "B",
                                          camera                                  ? "Build"
                                              : replacement || StationPlacement() ? "Cancel"
                                                                                  : "Undo" },
                                        { "L1/R1", camera ? "Zoom" : "Piece" },
                                        { "X",
                                          camera                                   ? "Rotate"
                                              : trackTypeHasSpeedSetting(selected) ? "Speed +"
                                                                                   : "Bank" },
                                        { "SELECT", camera ? "Build" : "Pieces" } };
        if (!camera)
            hints.push_back({ "Y", StationPlacement() ? "Turn" : trackTypeHasSpeedSetting(selected) ? "Speed -" : "Chain" });
        DrawFooter(rt, screen, hints);
        const auto q = Action(false);
        if (q.error == GA::Status::ok)
            DrawCostBadge(rt, Layout::ContextAnchor(screen, kContextSize, true), FormatMoney(q.cost));
        else
            DrawContextMessage(
                rt, Layout::ContextAnchor(screen, kContextSize, true), feedback.empty() ? q.getErrorMessage() : feedback);
    }
} // namespace OpenRCT2::Ui::Handheld::CustomTracks


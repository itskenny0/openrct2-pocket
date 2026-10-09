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
    static bool active{}, busy{}, camera{}, chain{}, createdHere{};
    static int32_t stationOffset{};
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
        bool operator==(const Preview&) const = default;
    };
    static std::optional<Block> previous;
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
        pitch = TrackPitch::none;
        roll = TrackRoll::none;
        auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        if (!ride || !TrackConstructionSupport::Type(ride->type) || ride->status != RideStatus::closed)
            return false;
        std::optional<CoordsXYE> first;
        for (const auto& station : ride->getStations())
            if (!station.start.isNull())
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
                pitch = ted.definition.pitchEnd;
                roll = ted.definition.rollEnd;
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
    bool StationPlacement()
    {
        return active && !previous && !completed;
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
    static bool Supported(TrackElemType type)
    {
        if (!TrackConstructionSupport::Piece(type))
            return false;
        const auto& ted = GetTrackElementDescriptor(type);
        const auto group = ted.definition.group;
        const auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        if (!ride || !Eligible(ride->subtype, ride->type) || !ride->getRideTypeDescriptor().SupportsTrackGroup(group)
            || !RideEntryGetSupportedTrackPieces(*GetRideEntryByIndex(ride->subtype))[static_cast<size_t>(group)]
            || ted.sequenceData.numSequences == 0)
            return false;
        // Loaded vehicle support and native TED endpoint geometry filter all
        // conventional special pieces, including inversions/helixes/photo.
        if (StationPlacement())
            return type == TrackElemType::endStation;
        return endpoint && ted.definition.pitchStart == pitch && ted.definition.rollStart == roll
            && (ted.coordinates.rotationBegin & 4) == (endpoint->direction & 4);
    }
    static std::string Name(TrackElemType type)
    {
        const auto& ted = GetTrackElementDescriptor(type);
        if (type == TrackElemType::endStation)
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
            PickerEntry entry{ { ObjectType::ride, kObjectEntryIndexNull, std::to_string(i) }, Name(type), ZoomLevel{ 0 } };
            entry.drawPreview = [type](Drawing::RenderTarget& rt) { DrawPiecePreview(rt, type); };
            result.push_back(std::move(entry));
        }
        result.push_back(
            { { ObjectType::ride, kObjectEntryIndexNull, "leave" },
              "Return to park",
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
        createdHere = false;
        stationOffset = 0;
        active = false;
        busy = false;
        target.reset();
        completed.reset();
        endpoint.reset();
        previous.reset();
        depth = 0;
        camera = false;
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
        End();
        target = t;
        active = true;
        selected = TrackElemType::flat;
        chain = false;
        feedback.clear();
        if (!Recover())
        {
            End();
            return false;
        }
        FocusEndpoint();
        if (!Supported(selected))
            for (const auto& entry : Pieces())
                if (entry.identity.key != "leave")
                {
                    selected = static_cast<TrackElemType>(std::stoi(entry.identity.key));
                    break;
                }
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
        entries.pop_back();
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
        const auto current = GetTrackElementDescriptor(selected).definition.pitchEnd;
        auto it = std::find(slopes.begin(), slopes.end(), current);
        int index = it == slopes.end() ? 2 : it - slopes.begin();
        const auto desired = slopes[std::clamp(index + (direction > 0 ? 1 : -1), 0, 4)];
        for (const auto& e : Pieces())
            if (e.identity.key != "leave")
            {
                auto type = static_cast<TrackElemType>(std::stoi(e.identity.key));
                const auto& ted = GetTrackElementDescriptor(type);
                if (ted.definition.pitchEnd == desired && ted.definition.rollEnd == roll
                    && !ted.flags.has(TrackElementFlag::turnLeft) && !ted.flags.has(TrackElementFlag::turnRight))
                {
                    Select(type);
                    return;
                }
            }
    }
    void Bank()
    {
        const std::array banks{ TrackRoll::none, TrackRoll::left, TrackRoll::right };
        const auto current = GetTrackElementDescriptor(selected).definition.rollEnd;
        auto it = std::find(banks.begin(), banks.end(), current);
        size_t index = it == banks.end() ? 0 : it - banks.begin();
        for (size_t offset = 1; offset <= banks.size(); ++offset)
            for (const auto& e : Pieces())
                if (e.identity.key != "leave")
                {
                    auto type = static_cast<TrackElemType>(std::stoi(e.identity.key));
                    const auto& ted = GetTrackElementDescriptor(type);
                    if (ted.definition.rollEnd == banks[(index + offset) % banks.size()]
                        && ted.definition.pitchEnd == GetTrackElementDescriptor(selected).definition.pitchEnd
                        && !ted.flags.has(TrackElementFlag::turnLeft) && !ted.flags.has(TrackElementFlag::turnRight))
                    {
                        Select(type);
                        return;
                    }
                }
    }
    void Chain()
    {
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
        auto q = GA::Query(&query, getGameState());
        if (!execute || q.error != GA::Status::ok)
            return q;
        auto action = GA::SequentialTrackPlaceAction(
            target->id, request->type, request->origin, chain, !previous, source.origin, source.type, q.cost);
        ClearPreview();
        busy = true;
        const auto token = revision;
        action.SetCallback([token](const auto*, const GA::Result* result) {
            if (token != revision)
                return;
            busy = false;
            feedback = result->error == GA::Status::ok ? "" : result->getErrorMessage();
            if (result->error == GA::Status::ok)
            {
                target = RideManagement::Identify(target->id);
                Recover();
                if (!completed)
                    FocusEndpoint();
                if (!completed && !Supported(selected))
                {
                    selected = TrackElemType::flat;
                    for (const auto& e : Pieces())
                        if (e.identity.key != "leave")
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
        if (!previous || !target || busy)
            return Invalid("No preceding track piece");
        const auto block = *previous;
        auto query = GA::SequentialTrackPlaceAction(
            target->id, block.type, block.origin, false, false, block.origin, block.type);
        query.UseRemoval();
        auto q = GA::Query(&query, getGameState());
        if (!execute || q.error != GA::Status::ok)
            return q;
        auto remove = GA::SequentialTrackPlaceAction(
            target->id, block.type, block.origin, false, false, block.origin, block.type, q.cost);
        remove.UseRemoval();
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
                Recover();
                if (!previous && removed)
                    endpoint = removed->origin;
                selected = TrackElemType::flat;
                if (!previous)
                    selected = TrackElemType::endStation;
                if (!Supported(selected))
                    for (const auto& entry : Pieces())
                        if (entry.identity.key != "leave")
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
        const Preview next{ *request, chain };
        if (ghost && *ghost == next)
            return;
        ClearPreview();
        SelectedLiftAndInverted flags{};
        if (chain)
            flags.set(LiftHillAndInverted::liftHill);
        auto* ride = RideManagement::Resolve(*target);
        if (nativePreview.place(ride->subtype, ride->type, request->origin, request->type, flags))
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
        Box(rt, p, { 164, 238 });
        Heading(rt, p + ScreenCoordsXY{ 10, 12 }, StationPlacement() ? "Place station" : "Build track");
        Rule(rt, p + ScreenCoordsXY{ 8, 38 }, 148);
        Row(rt, p + ScreenCoordsXY{ 8, 48 }, Name(selected), Icon::path, true);
        const auto& d = GetTrackElementDescriptor(selected).definition;
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 86 },
            "{MUTED}Slope "
                + std::string(
                    d.pitchEnd == TrackPitch::none         ? "Level"
                        : d.pitchEnd == TrackPitch::up25   ? "Up 25"
                        : d.pitchEnd == TrackPitch::up60   ? "Up 60"
                        : d.pitchEnd == TrackPitch::down25 ? "Down 25"
                        : d.pitchEnd == TrackPitch::down60 ? "Down 60"
                                                           : "Special"));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 108 },
            "{MUTED}Bank "
                + std::string(
                    d.rollEnd == TrackRoll::none        ? "Level"
                        : d.rollEnd == TrackRoll::left  ? "Left"
                        : d.rollEnd == TrackRoll::right ? "Right"
                                                        : "Inverted"));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 130 },
            std::string("{MUTED}Chain ") + (ChainAllowed() ? (chain ? "On" : "Off") : "Unavailable"));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 152 }, "{MUTED}Height " + (endpoint ? std::to_string(endpoint->z / 8) : "Unavailable"));
        DrawModernText(rt, p + ScreenCoordsXY{ 10, 180 }, camera ? "{GOLD}Camera controls" : "{MUTED}L3 Camera controls");
        DrawModernText(rt, p + ScreenCoordsXY{ 10, 208 }, "{MUTED}L2/R2 Slope | SEL Pieces");
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 8, y }, 17, "A", "Build");
        DrawCommand(rt, { 100, y }, 17, "B", StationPlacement() ? "Cancel" : "Undo");
        DrawCommand(rt, { 192, y }, 30, "L/R", camera ? "Zoom" : "Piece");
        DrawCommand(rt, { 306, y }, 17, "X", camera ? "Rotate" : "Bank");
        DrawCommand(rt, { 411, y }, 17, "Y", StationPlacement() ? "Turn" : "Chain");
        const auto q = Action(false);
        if (q.error == GA::Status::ok)
            DrawCostBadge(rt, Layout::ContextAnchor(screen, kContextSize, true), FormatMoney(q.cost));
        else
            DrawContextMessage(
                rt, Layout::ContextAnchor(screen, kContextSize, true), feedback.empty() ? q.getErrorMessage() : feedback);
    }
} // namespace OpenRCT2::Ui::Handheld::CustomTracks

// SPDX-License-Identifier: GPL-3.0-only
#include "CustomTracks.h"

#include "Explorer.h"
#include "NativeTrackThumbnail.h"
#include "TrackPreview.h"

#include <cctype>
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
    static std::optional<CoordsXY> previewTile;
    static bool previewWanted{};
    static PieceGroup pieceGroup{ PieceGroup::all };
    static int turnSize{ 1 }; // 0 tight, 1 normal, 2 wide
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
    // Keeps the end of the track in the middle of the screen. The slim build strip sits at the top,
    // so the end lands a little below centre. Smooth follow uses the engine's scroll-to-location
    // easing; the first focus of a session snaps so the player never watches a long fly-in.
    static void FocusEndpoint(bool smooth = false)
    {
        auto* main = WindowGetMain();
        if (endpoint && main && main->viewport)
        {
            auto& view = *main->viewport;
            main->savedViewPos = Translate3DTo2DWithZ(view.rotation, { endpoint->x + 16, endpoint->y + 16, endpoint->z })
                - ScreenCoordsXY{ view.zoom.ApplyTo(ContextGetWidth() / 2 - view.pos.x),
                                  view.zoom.ApplyTo(ContextGetHeight() * 11 / 20 - view.pos.y) };
            if (smooth)
                main->flags.set(WindowFlag::scrollingToLocation);
            else
            {
                view.viewPos = main->savedViewPos;
                main->flags.unset(WindowFlag::scrollingToLocation);
            }
            main->invalidate();
        }
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
            return native
                + (ted.sequenceData.numSequences > 1 ? " / " + std::to_string(ted.sequenceData.numSequences) + " tiles" : "");
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
            return std::string(ted.flags.has(TrackElementFlag::turnLeft) ? "Left turn" : "Right turn") + " / "
                + std::to_string(ted.sequenceData.numSequences) + " tiles" + (d.rollEnd != TrackRoll::none ? " / bank" : "")
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
        if (target && RideManagement::Resolve(*target))
        {
            SelectedLiftAndInverted flags{};
            if (chain && type == selected)
                flags.set(LiftHillAndInverted::liftHill);
            if (placementInverted)
                flags.set(LiftHillAndInverted::inverted);
            Windows::DrawNativeTrackThumbnail(rt, target->id, type, endpoint ? endpoint->direction : 0, flags);
            return;
        }
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
    PieceGroup GroupOf(TrackElemType type)
    {
        switch (GetTrackElementDescriptor(type).definition.group)
        {
            case TrackGroup::flat:
            case TrackGroup::straight:
                return PieceGroup::straight;
            case TrackGroup::stationEnd:
                return PieceGroup::station;
            case TrackGroup::curveVerySmall:
            case TrackGroup::curveSmall:
            case TrackGroup::curve:
            case TrackGroup::curveLarge:
            case TrackGroup::sBend:
                return PieceGroup::turns;
            case TrackGroup::slope:
            case TrackGroup::slopeSteepDown:
            case TrackGroup::slopeSteepUp:
            case TrackGroup::flatToSteepSlope:
            case TrackGroup::slopeToFlat:
            case TrackGroup::slopeVertical:
            case TrackGroup::slopeSteepLong:
            case TrackGroup::diagSlope:
            case TrackGroup::diagSlopeSteepUp:
            case TrackGroup::diagSlopeSteepDown:
            case TrackGroup::diagSlopeSteepLong:
            case TrackGroup::liftHill:
            case TrackGroup::liftHillSteep:
            case TrackGroup::poweredLift:
            case TrackGroup::liftHillCable:
                return PieceGroup::slopes;
            case TrackGroup::flatRollBanking:
            case TrackGroup::slopeRollBanking:
            case TrackGroup::slopeCurveBanked:
            case TrackGroup::slopeCurveLargeBanked:
            case TrackGroup::helixUpBankedHalf:
            case TrackGroup::helixDownBankedHalf:
            case TrackGroup::helixUpBankedQuarter:
            case TrackGroup::helixDownBankedQuarter:
                return PieceGroup::banks;
            default:
                return PieceGroup::special;
        }
    }
    PieceGroup CurrentGroup()
    {
        return pieceGroup;
    }
    std::string GroupName(PieceGroup group)
    {
        static const char* names[]{ "All", "Straight", "Stations", "Turns", "Slopes", "Banking", "Special" };
        return names[static_cast<size_t>(group)];
    }
    void NextGroup()
    {
        for (int n = 0; n < static_cast<int>(PieceGroup::count); ++n)
        {
            pieceGroup = static_cast<PieceGroup>((static_cast<int>(pieceGroup) + 1) % static_cast<int>(PieceGroup::count));
            if (pieceGroup == PieceGroup::all)
                return;
            for (const auto& e : Pieces())
                if (!e.identity.key.empty() && std::isdigit(static_cast<unsigned char>(e.identity.key[0]))
                    && GroupOf(static_cast<TrackElemType>(std::stoi(e.identity.key))) == pieceGroup)
                    return;
        }
    }
    // Steering works on shape rather than piece names: every ordinary piece is a turn (left or right)
    // or not, has an end slope, and has a footprint. The D-pad picks the shape and the game finds the
    // piece. Only plain track takes part; loops, helixes, diagonals and S-bends stay in the catalog.
    static bool Steerable(TrackElemType type)
    {
        switch (GetTrackElementDescriptor(type).definition.group)
        {
            case TrackGroup::flat:
            case TrackGroup::straight:
            case TrackGroup::liftHill:
            case TrackGroup::liftHillSteep:
            case TrackGroup::liftHillCurve:
            case TrackGroup::flatRollBanking:
            case TrackGroup::slope:
            case TrackGroup::slopeSteepDown:
            case TrackGroup::slopeSteepUp:
            case TrackGroup::flatToSteepSlope:
            case TrackGroup::slopeToFlat:
            case TrackGroup::slopeCurve:
            case TrackGroup::slopeCurveSteep:
            case TrackGroup::curveVerySmall:
            case TrackGroup::curveSmall:
            case TrackGroup::curve:
            case TrackGroup::curveLarge:
            case TrackGroup::slopeCurveBanked:
            case TrackGroup::slopeRollBanking:
                return true;
            default:
                return false;
        }
    }
    // -1 left, 0 straight, 1 right, 2 anything else. Building backward mirrors the turn, because the
    // player is driving the other way along the piece.
    static int TurnOf(TrackElemType type)
    {
        const auto& c = GetTrackElementDescriptor(type).coordinates;
        if ((c.rotationBegin & 4) != (c.rotationEnd & 4))
            return 2;
        const int delta = (c.rotationEnd - c.rotationBegin) & 3;
        const int turn = delta == 0 ? (c.y == 0 ? 0 : 2) : delta == 3 ? -1 : delta == 1 ? 1 : 2;
        return backward && turn != 2 ? -turn : turn;
    }
    static TrackPitch EndPitch(TrackElemType type)
    {
        const auto& d = GetTrackElementDescriptor(type).definition;
        return backward ? d.pitchStart : d.pitchEnd;
    }
    static bool KeepsBank(TrackElemType type)
    {
        const auto& d = GetTrackElementDescriptor(type).definition;
        return d.rollStart == d.rollEnd;
    }
    static std::optional<TrackElemType> FindShape(int turn, TrackPitch wantPitch, int size)
    {
        const bool covered = TrackConstructionSupport::CoveredBase(selected).has_value();
        std::vector<std::pair<int, TrackElemType>> found;
        for (int pass = 0; pass < 2 && found.empty(); ++pass)
            for (int i = 0; i < static_cast<int>(TrackElemType::count); ++i)
            {
                const auto type = static_cast<TrackElemType>(i);
                if (!Steerable(type) || !Supported(type) || TurnOf(type) != turn || EndPitch(type) != wantPitch
                    || !KeepsBank(type))
                    continue;
                // Prefer the covered/open look the player already has, but don't let it block a turn.
                if (pass == 0 && TrackConstructionSupport::CoveredBase(type).has_value() != covered)
                    continue;
                found.emplace_back(GetTrackElementDescriptor(type).sequenceData.numSequences, type);
            }
        if (found.empty())
            return std::nullopt;
        std::stable_sort(found.begin(), found.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        if (turn == 0)
            return found.front().second;
        std::vector<int> sizes;
        for (const auto& [footprint, type] : found)
            if (sizes.empty() || sizes.back() != footprint)
                sizes.push_back(footprint);
        // Normal is the middle size, rounding down: with only small and large turns, normal is the
        // small one (RCT's usual 3-tile quarter turn) and wide is the large one.
        const size_t pick = size <= 0 ? 0 : size >= 2 ? sizes.size() - 1 : (sizes.size() - 1) / 2;
        for (const auto& [footprint, type] : found)
            if (footprint == sizes[pick])
                return type;
        return found.front().second;
    }
    int CurrentTurn()
    {
        const int turn = TurnOf(selected);
        return turn == 2 ? 0 : turn;
    }
    int TurnSize()
    {
        return turnSize;
    }
    std::string TurnSizeName()
    {
        static const char* names[]{ "Tight", "Normal", "Wide" };
        return names[std::clamp(turnSize, 0, 2)];
    }
    bool Steer(int direction)
    {
        if (!active || busy || StationPlacement() || direction == 0)
            return false;
        const int current = CurrentTurn();
        // Same direction again tightens the bend; the opposite direction straightens it out.
        int turn = current == 0 ? direction : current == direction ? direction : 0;
        if (current == direction && turnSize > 0)
            --turnSize;
        auto pitchNow = Steerable(selected) ? EndPitch(selected) : pitch;
        auto found = FindShape(turn, pitchNow, turnSize);
        if (!found && turn != 0)
            found = FindShape(turn, pitch, turnSize);
        if (!found)
        {
            ShowToast(
                "Can't turn here",
                pitch == TrackPitch::none ? "This ride has no turn that fits." : "Level out first, then turn.");
            return false;
        }
        return Select(*found);
    }
    void SetTurnSize(int direction)
    {
        if (!active || busy)
            return;
        turnSize = std::clamp(turnSize + (direction > 0 ? 1 : -1), 0, 2);
        if (const int turn = CurrentTurn(); turn != 0)
            if (const auto found = FindShape(turn, EndPitch(selected), turnSize))
                Select(*found);
    }
    std::vector<PickerEntry> Pieces(bool filtered)
    {
        std::vector<PickerEntry> result;
        for (int i = 0; i < static_cast<int>(TrackElemType::count); ++i)
        {
            auto type = static_cast<TrackElemType>(i);
            if (!Supported(type) || type == TrackElemType::beginStation || type == TrackElemType::middleStation)
                continue;
            if (filtered && pieceGroup != PieceGroup::all && GroupOf(type) != pieceGroup)
                continue;
            PickerEntry entry{ { ObjectType::ride, kObjectEntryIndexNull, std::to_string(i) },
                               PieceName(type),
                               ZoomLevel{ 0 } };
            entry.drawPreview = [type](Drawing::RenderTarget& rt) { DrawPiecePreview(rt, type); };
            result.push_back(std::move(entry));
        }
        std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return GroupOf(static_cast<TrackElemType>(std::stoi(a.identity.key)))
                < GroupOf(static_cast<TrackElemType>(std::stoi(b.identity.key)));
        });
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
    static void RemoveGhost()
    {
        ghost.reset();
        nativePreview.clear();
    }
    void ClearPreview()
    {
        previewWanted = false;
        RemoveGhost();
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
        previewTile.reset();
        pieceGroup = PieceGroup::all;
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
    bool Replacing()
    {
        return active && replacement.has_value();
    }
    bool ResumeReplacement(
        const RideManagement::Target& old, const TrackConstructionSupport::BlockSnapshot& removed, uint8_t colour)
    {
        const auto now = RideManagement::Identify(old.id);
        if (!now || now->id != old.id || now->type != old.type || now->object != old.object || now->buildDate != old.buildDate
            || now->generation != old.generation || !Eligible(now->object, now->type))
            return false;
        const auto* ride = RideManagement::Resolve(*now);
        if (!ride || ride->status != RideStatus::closed)
            return false;
        End();
        target = *now;
        replacement = removed;
        replacementContext = TrackConstructionSupport::EditingContext(*ride);
        endpoint = removed.origin;
        selected = removed.type == TrackElemType::beginStation || removed.type == TrackElemType::middleStation
            ? TrackElemType::endStation
            : removed.type;
        placementInverted = removed.inverted;
        placementSpeed = removed.speed;
        placementSeat = removed.seat;
        placementColour = colour;
        chain = removed.chain;
        active = true;
        feedback.clear();
        if (!Supported(selected))
        {
            End();
            return false;
        }
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
        auto entries = Pieces(true);
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
            stationOffset = std::clamp(stationOffset + (direction > 0 ? kLandHeightStep : -kLandHeightStep), 0, 256);
            return;
        }
        const std::array slopes{ TrackPitch::down60, TrackPitch::down25, TrackPitch::none, TrackPitch::up25, TrackPitch::up60 };
        const auto current = EndPitch(selected);
        auto it = std::find(slopes.begin(), slopes.end(), current);
        int index = it == slopes.end() ? 2 : it - slopes.begin();
        // Step one notch at a time; if this ride can't climb that way, try the next notch so a press
        // never silently does nothing.
        for (int step = 1; step <= 2; ++step)
        {
            const int next = index + (direction > 0 ? step : -step);
            if (next < 0 || next > 4)
                break;
            // Keep turning if the ride has a climbing turn; otherwise climb straight.
            auto found = FindShape(CurrentTurn(), slopes[next], turnSize);
            if (!found && CurrentTurn() != 0)
                found = FindShape(0, slopes[next], turnSize);
            if (found)
            {
                Select(*found);
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
        if (replacement)
            return Block{ replacement->origin, selected };
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
        if (replacement)
            query.UseDetached(replacementContext);
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
        if (replacement)
            action.UseDetached(replacementContext);
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
                        {
                            anchor->type = track->getTrackType();
                            break;
                        }
                    replacement.reset();
                    replacementContext.clear();
                }
                if (!Recover())
                {
                    End();
                    return;
                }
                if (!completed)
                    FocusEndpoint(true);
                // Carry the player's intent onto the next piece: keep turning the same way and keep the
                // slope the last piece finished on (a "flat to up" piece becomes a steady climb).
                if (!completed && !Supported(selected) && Steerable(placed.type))
                    if (const auto next = FindShape(TurnOf(placed.type) == 2 ? 0 : TurnOf(placed.type), pitch, turnSize))
                        selected = *next;
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
        if (replacement)
            return Invalid("Rebuild cancelled only through B / Return to park");
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
                FocusEndpoint(true);
            }
        });
        auto r = GA::Execute(&remove, getGameState());
        if (r.error != GA::Status::ok)
            busy = false;
        return r;
    }
    void Update(std::optional<CoordsXY> tile)
    {
        previewTile = tile;
        previewWanted = true;
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
            RemoveGhost();
            return;
        }
        const Preview next{ *request, chain, trackTypeHasSpeedSetting(request->type) ? placementSpeed : uint8_t{ 6 },
                            SeatSupported() ? placementSeat : uint8_t{ 0 }, placementInverted };
        if (ghost && *ghost == next && nativePreview.exists())
            return;
        RemoveGhost();
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
        RemoveGhost();
    }
    void RestorePreview()
    {
        if (depth && --depth == 0 && active && !busy && previewWanted)
            Update(previewTile);
    }
    bool HasPreview()
    {
        return ghost.has_value() && nativePreview.exists();
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
        // A slim strip instead of a half-screen panel: the track you are building is the interface, and
        // the camera keeps its end in view, so the panel only needs to say what A will place.
        const int width = std::min(screen.width - 2 * Layout::margin, 360);
        Box(rt, p, { width, 66 });
        Drawing::RenderTarget preview;
        if (ClipRenderTarget(preview, rt, p + ScreenCoordsXY{ 6, 6 }, 62, 54))
            DrawPiecePreview(preview, selected);
        const std::string mode = camera ? "CAMERA"
            : replacement               ? "REBUILD"
            : StationPlacement()        ? "STATION"
            : backward                  ? "REVERSE"
                                        : "BUILD";
        const int badge = TextWidth(mode, true) + 12;
        DrawModeBadge(rt, p + ScreenCoordsXY{ width - badge - 6, 6 }, mode);
        const int textWidth = width - 80 - badge - 8;
        DrawModernText(rt, p + ScreenCoordsXY{ 76, 9 }, "{WHITE}" + PieceName(selected), {}, textWidth, true);
        const auto& d = GetTrackElementDescriptor(selected).definition;
        const auto displayPitch = backward ? d.pitchStart : d.pitchEnd;
        const auto bank = backward ? d.rollStart : d.rollEnd;
        const std::string slope = displayPitch == TrackPitch::none ? "Level"
            : displayPitch == TrackPitch::up25                     ? "Climb"
            : displayPitch == TrackPitch::up60                     ? "Steep climb"
            : displayPitch == TrackPitch::down25                   ? "Dive"
            : displayPitch == TrackPitch::down60                   ? "Steep dive"
                                                                   : "Special";
        const int turn = TurnOf(selected);
        const std::string direction = StationPlacement() ? "Station"
            : turn == -1                                 ? "Left, " + TurnSizeName()
            : turn == 1                                  ? "Right, " + TurnSizeName()
            : turn == 0                                  ? "Straight"
                                                         : "Special piece";
        DrawModernEllipsised(rt, p + ScreenCoordsXY{ 76, 28 }, width - 84, "{MUTED}" + direction + "  |  " + slope);
        std::string detail = trackTypeHasSpeedSetting(selected)
            ? FormatStringID(STR_VELOCITY, static_cast<uint16_t>((placementSpeed * 9) >> 2))
            : bank == TrackRoll::none  ? "No bank"
            : bank == TrackRoll::left  ? "Banked left"
            : bank == TrackRoll::right ? "Banked right"
                                       : "Inverted";
        if (ChainAllowed())
            detail += chain ? "  |  Chain lift" : "  |  No chain";
        if (endpoint)
            detail += "  |  Height " + std::to_string(endpoint->z / 8);
        if (SeatSupported())
            detail += "  |  Seat " + std::to_string(placementSeat);
        DrawModernEllipsised(rt, p + ScreenCoordsXY{ 76, 46 }, width - 84, "{MUTED}" + detail);
        const bool steering = !camera && !StationPlacement();
        std::vector<ControlHint> hints{ { "D-pad",
                                          camera                   ? "Pan"
                                              : StationPlacement() ? "Move"
                                                                   : "Steer",
                                          camera || steering ? HintPriority::normal : HintPriority::guide },
                                        { "A", camera ? "Build mode" : "Build" },
                                        { "B",
                                          camera                                  ? "Build"
                                              : replacement || StationPlacement() ? "Cancel"
                                                                                  : "Undo" },
                                        { "L1/R1",
                                          camera         ? "Zoom"
                                              : steering ? "Turn size"
                                                         : "Piece" },
                                        { "X",
                                          camera                                   ? "Rotate"
                                              : trackTypeHasSpeedSetting(selected) ? "Speed +"
                                                                                   : "Bank" },
                                        { "SELECT", camera ? "Build" : "Pieces", HintPriority::guide } };
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

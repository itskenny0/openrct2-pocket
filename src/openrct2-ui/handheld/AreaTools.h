// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Surface.h"
#include "ConstructionPanel.h"
#include "Currency.h"
#include "ManagementPanel.h"
#include <openrct2-ui/interface/Dropdown.h>
#include <openrct2-ui/interface/LandTool.h>
#include <openrct2/network/Network.h>
#include <openrct2/drawing/Drawing.h>

#include <algorithm>
#include <openrct2/actions/terraform/LandLowerAction.h>
#include <openrct2/actions/terraform/LandRaiseAction.h>
#include <openrct2/actions/terraform/LandSmoothAction.h>
#include <openrct2/actions/terraform/WaterLowerAction.h>
#include <openrct2/actions/terraform/WaterRaiseAction.h>
#include <openrct2/object/TerrainEdgeObject.h>
#include <openrct2/world/MapSelection.h>
namespace OpenRCT2::Ui::Handheld::AreaTools
{
    enum class Operation
    {
        raise,
        lower,
        smoothRaise,
        smoothLower,
        waterRaise,
        waterLower,
        paint
    };
    struct Settings
    {
        Operation operation{};
        int width = 1, height = 1;
        size_t selection{};
        ObjectEntryIndex surface = kObjectEntryIndexNull, edge = kObjectEntryIndexNull;
        Drawing::Colour surfaceColour = Drawing::Colour::black, edgeColour = Drawing::Colour::black;
        bool operator==(const Settings&) const = default;
    };
    inline Settings settings;
    inline const std::array<MapSelectType, 9> selections{
        MapSelectType::full,  MapSelectType::corner0, MapSelectType::corner1, MapSelectType::corner2, MapSelectType::corner3,
        MapSelectType::edge0, MapSelectType::edge1,   MapSelectType::edge2,   MapSelectType::edge3
    };
    inline const std::array<const char*, 7> operations{ "Raise land",  "Lower land",  "Smooth / raise",      "Smooth / lower",
                                                        "Raise water", "Lower water", "Paint surface / edge" };
    inline std::optional<MapRange> Range(std::optional<CoordsXY> tile, int width, int height)
    {
        if (!tile || width < 1 || width > 64 || height < 1 || height > 64 || tile->x % 32 || tile->y % 32)
            return {};
        const auto max = GetMapSizeMaxXY();
        const auto first = *tile - CoordsXY{ ((width - 1) / 2) * 32, ((height - 1) / 2) * 32 };
        const auto last = first + CoordsXY{ (width - 1) * 32, (height - 1) * 32 };
        if (first.x < 32 || first.y < 32 || last.x > max.x || last.y > max.y)
            return {};
        return MapRange{ first.x, first.y, last.x, last.y };
    }
    inline GameActions::Result Invalid(const char* m)
    {
        GameActions::Result r;
        r.error = GameActions::Status::invalidParameters;
        r.errorMessage = m;
        return r;
    }
    inline MapSelectType Selection(const Settings& s)
    {
        if (s.operation == Operation::waterRaise || s.operation == Operation::waterLower)
            return MapSelectType::fullWater;
        if (s.operation == Operation::paint)
            return MapSelectType::full;
        return s.selection < selections.size() ? selections[s.selection] : MapSelectType::full;
    }
    inline std::string SelectionName(const Settings& s)
    {
        if (s.operation == Operation::waterRaise || s.operation == Operation::waterLower || s.operation == Operation::paint)
            return "Whole area";
        return s.selection == 0 ? "Whole tile"
            : s.selection < 5   ? "Corner " + std::to_string(s.selection)
                                : "Edge " + std::to_string(s.selection - 4);
    }
    inline std::string SizeName(const Settings& s)
    {
        return std::to_string(s.width) + " x " + std::to_string(s.height) + " tiles";
    }
    inline ObjectEntryIndex CycleStyle(ObjectType type, ObjectEntryIndex current, int delta)
    {
        std::vector<ObjectEntryIndex> items{ kObjectEntryIndexNull };
        for (ObjectEntryIndex i = 0; i < 255; ++i)
            if (GetContext()->GetObjectManager().GetLoadedObject(type, i))
                items.push_back(i);
        auto it = std::find(items.begin(), items.end(), current);
        int index = it == items.end() ? 0 : it - items.begin();
        return items[(index + delta + items.size()) % items.size()];
    }
    inline std::string StyleName(ObjectType type, ObjectEntryIndex i)
    {
        const auto* o = GetContext()->GetObjectManager().GetLoadedObject(type, i);
        return o ? o->GetName() : "Keep unchanged";
    }
    inline bool SurfaceColourSupported(ObjectEntryIndex index)
    {
        const auto* o=GetContext()->GetObjectManager().GetLoadedObject<TerrainSurfaceObject>(index);
        return o && o->Flags.has(TerrainSurfaceFlag::hasPrimaryColour);
    }
    inline bool EdgeColourSupported(ObjectEntryIndex index)
    {
        const auto* o=GetContext()->GetObjectManager().GetLoadedObject<TerrainEdgeObject>(index);
        return o && o->flags.has(TerrainEdgeFlag::hasPrimaryColour);
    }
    inline std::string ColourName(Drawing::Colour colour)
    {
        for (uint8_t i=0;i<72;++i)
            if (Windows::ColourDropDownIndexToColour(i)==colour)
                return FmtString(FormatStringID(Windows::ColourDropDownIndexToName(i))).WithoutFormatTokens();
        return "Unavailable";
    }
    inline Drawing::Colour CycleColour(Drawing::Colour colour,int delta)
    {
        int index=0;
        for (uint8_t i=0;i<72;++i) if (Windows::ColourDropDownIndexToColour(i)==colour) {index=i;break;}
        return Windows::ColourDropDownIndexToColour((index+delta+72)%72);
    }
    inline void ResetColours()
    {
        bool enabled;
        LandTool::resetSurfaceColourSelection(settings.surface,settings.surfaceColour,enabled);
        LandTool::resetEdgeColourSelection(settings.edge,settings.edgeColour,enabled);
    }
    inline void DrawOverlay(Drawing::RenderTarget& rt,ScreenCoordsXY position)
    {
        const auto& s=settings;
        if (s.operation!=Operation::paint)
        {
            DrawConstructionOverlay(rt,position,"Terrain / Water",operations[static_cast<size_t>(s.operation)],
                "Area brush",SizeName(s),SelectionName(s));
            return;
        }
        DrawConstructionOverlay(rt,position,"Terrain paint",
            s.surface!=kObjectEntryIndexNull ? StyleName(ObjectType::terrainSurface,s.surface) : StyleName(ObjectType::terrainEdge,s.edge),
            SurfaceColourSupported(s.surface) ? "Surface: "+ColourName(s.surfaceColour) : "Surface colour: fixed",
            EdgeColourSupported(s.edge) ? "Edge: "+ColourName(s.edgeColour) : "Edge colour: fixed",SizeName(s));
        Concept::Box(rt,position+ScreenCoordsXY{0,180},{164,62});
        Concept::Heading(rt,position+ScreenCoordsXY{12,187},"Style previews");
        auto& objects=GetContext()->GetObjectManager();
        if (const auto* o=objects.GetLoadedObject<TerrainSurfaceObject>(s.surface))
            GfxDrawSprite(rt,ImageId(o->IconImageId,SurfaceColourSupported(s.surface)?o->getPrimaryColour(s.surfaceColour):o->getPreviewColour()),
                position+ScreenCoordsXY{44,208});
        if (const auto* o=objects.GetLoadedObject<TerrainEdgeObject>(s.edge))
            GfxDrawSprite(rt,ImageId(o->IconImageId,EdgeColourSupported(s.edge)?s.edgeColour:o->getPreviewColour()),
                position+ScreenCoordsXY{116,208});
    }
    inline std::vector<std::string> Options()
    {
        return { "Resume",
                 operations[static_cast<size_t>(settings.operation)],
                 "Brush width: " + std::to_string(settings.width) + " tiles",
                 "Brush depth: " + std::to_string(settings.height) + " tiles",
                 "Selection: " + SelectionName(settings),
                 "Surface: " + StyleName(ObjectType::terrainSurface, settings.surface),
                 "Cliff face: " + StyleName(ObjectType::terrainEdge, settings.edge),
                 SurfaceColourSupported(settings.surface) ? "Surface colour: "+ColourName(settings.surfaceColour) : "Surface colour: fixed",
                 EdgeColourSupported(settings.edge) ? "Cliff colour: "+ColourName(settings.edgeColour) : "Cliff colour: fixed" };
    }
    inline void Configure(size_t row, int delta = 1)
    {
        if (row == 1)
            settings.operation = static_cast<Operation>((static_cast<int>(settings.operation) + delta + 7) % 7);
        if (row == 2)
            settings.width = std::clamp(settings.width + delta, 1, 64);
        if (row == 3)
            settings.height = std::clamp(settings.height + delta, 1, 64);
        if (row == 4)
            settings.selection = (settings.selection + delta + selections.size()) % selections.size();
        if (row == 5)
        {
            settings.surface = CycleStyle(ObjectType::terrainSurface, settings.surface, delta);
            bool enabled; LandTool::resetSurfaceColourSelection(settings.surface,settings.surfaceColour,enabled);
        }
        if (row == 6)
        {
            settings.edge = CycleStyle(ObjectType::terrainEdge, settings.edge, delta);
            bool enabled; LandTool::resetEdgeColourSelection(settings.edge,settings.edgeColour,enabled);
        }
        if (row==7 && SurfaceColourSupported(settings.surface)) settings.surfaceColour=CycleColour(settings.surfaceColour,delta);
        if (row==8 && EdgeColourSupported(settings.edge)) settings.edgeColour=CycleColour(settings.edgeColour,delta);
    }
    inline GameActions::Result Action(std::optional<CoordsXY> tile, const Settings& s, bool execute)
    {
        const auto range = Range(tile, s.width, s.height);
        if (!range || s.selection >= selections.size())
            return Invalid("Selection outside buildable map");
        const CoordsXY centre{ (range->getX1() + range->getX2()) / 2 + 16, (range->getY1() + range->getY2()) / 2 + 16 };
        std::unique_ptr<GameActions::GameAction> a;
        switch (s.operation)
        {
            case Operation::raise:
                a = std::make_unique<GameActions::LandRaiseAction>(centre, *range, Selection(s));
                break;
            case Operation::lower:
                a = std::make_unique<GameActions::LandLowerAction>(centre, *range, Selection(s));
                break;
            case Operation::smoothRaise:
                a = std::make_unique<GameActions::LandSmoothAction>(centre, *range, Selection(s), false);
                break;
            case Operation::smoothLower:
                a = std::make_unique<GameActions::LandSmoothAction>(centre, *range, Selection(s), true);
                break;
            case Operation::waterRaise:
                a = std::make_unique<GameActions::WaterRaiseAction>(*range);
                break;
            case Operation::waterLower:
                a = std::make_unique<GameActions::WaterLowerAction>(*range);
                break;
            case Operation::paint:
            {
                if (s.surface == kObjectEntryIndexNull && s.edge == kObjectEntryIndexNull)
                    return Invalid("Choose a surface or edge style");
                if ((s.surface != kObjectEntryIndexNull
                     && !GetContext()->GetObjectManager().GetLoadedObject(ObjectType::terrainSurface, s.surface))
                    || (s.edge != kObjectEntryIndexNull
                        && !GetContext()->GetObjectManager().GetLoadedObject(ObjectType::terrainEdge, s.edge)))
                    return Invalid("Terrain style unavailable");
                const auto* surface = MapGetSurfaceElementAt(*tile);
                if (!surface)
                    return Invalid("Terrain unavailable");
                if ((SurfaceColourSupported(s.surface) && static_cast<int>(s.surfaceColour)>=72)
                    || (EdgeColourSupported(s.edge) && static_cast<int>(s.edgeColour)>=72))
                    return Invalid("Invalid native terrain colour");
                a = std::make_unique<GameActions::SurfaceSetStyleAction>(
                    *range,s.surface,s.edge,SurfaceColourSupported(s.surface)?s.surfaceColour:surface->getPrimarySurfaceColour(),
                    EdgeColourSupported(s.edge)?s.edgeColour:surface->getPrimaryEdgeColour());
                break;
            }
            default:
                return Invalid("Terrain operation unavailable");
        }
        return execute ? GameActions::Execute(a.get(), getGameState()) : GameActions::Query(a.get(), getGameState());
    }
    struct TileState
    {
        int32_t baseZ{},water{};
        uint8_t slope{},ownership{};
        ObjectEntryIndex surface{},edge{};
        Drawing::Colour surfaceColour{},edgeColour{};
        bool operator==(const TileState&) const = default;
    };
    inline std::vector<TileState> CapturePaint(const MapRange& range)
    {
        std::vector<TileState> result;
        for (int y=range.getY1();y<=range.getY2();y+=32) for(int x=range.getX1();x<=range.getX2();x+=32)
        {
            const auto* s=MapGetSurfaceElementAt(CoordsXY{x,y});
            if (!s) return {};
            result.push_back({s->getBaseZ(),s->getWaterHeight(),s->getSlope(),s->getOwnership().holder,
                s->getSurfaceObjectIndex(),s->getEdgeObjectIndex(),s->getPrimarySurfaceColour(),s->getPrimaryEdgeColour()});
        }
        return result;
    }
    inline std::string Descriptor(ObjectType type,ObjectEntryIndex index)
    {
        const auto* o=GetContext()->GetObjectManager().GetLoadedObject(type,index);
        return o?std::string(o->GetIdentifier()):"";
    }
    struct PaintDraft
    {
        CoordsXY tile;
        Settings settings;
        std::vector<TileState> baseline;
        std::string surfaceDescriptor,edgeDescriptor;
        money64 cost{};
        decltype(getGameState().park.flags.holder) flags{};
        bool submitted{};
    };
    inline auto paintPending=std::make_shared<bool>(false);
    inline void ResetPaint() {paintPending=std::make_shared<bool>(false);}
    inline std::optional<PaintDraft> BeginPaint(CoordsXY tile,const Settings& s)
    {
        if (*paintPending || s.operation!=Operation::paint || Network::GetMode()!=Network::Mode::none) return {};
        const auto range=Range(tile,s.width,s.height); const auto q=Action(tile,s,false);
        if (!range || q.error!=GameActions::Status::ok) return {};
        return PaintDraft{tile,s,CapturePaint(*range),Descriptor(ObjectType::terrainSurface,s.surface),
            Descriptor(ObjectType::terrainEdge,s.edge),q.cost,getGameState().park.flags.holder,false};
    }
    inline GameActions::Result ApplyPaint(PaintDraft& d,bool execute)
    {
        const auto range=Range(d.tile,d.settings.width,d.settings.height);
        if (d.submitted || *paintPending || !range || CapturePaint(*range)!=d.baseline
            || Descriptor(ObjectType::terrainSurface,d.settings.surface)!=d.surfaceDescriptor
            || Descriptor(ObjectType::terrainEdge,d.settings.edge)!=d.edgeDescriptor || getGameState().park.flags.holder!=d.flags)
            return Invalid("Terrain changed / review again");
        const auto q=Action(d.tile,d.settings,false);
        if (q.error!=GameActions::Status::ok) return q;
        if (q.cost!=d.cost) return Invalid("Price changed / review again");
        if (!execute) return q;
        d.submitted=true;
        *paintPending=true;
        const auto pending=paintPending;
        const auto* surface=MapGetSurfaceElementAt(d.tile);
        GameActions::SurfaceSetStyleAction action(*range,d.settings.surface,d.settings.edge,
            SurfaceColourSupported(d.settings.surface)?d.settings.surfaceColour:surface->getPrimarySurfaceColour(),
            EdgeColourSupported(d.settings.edge)?d.settings.edgeColour:surface->getPrimaryEdgeColour());
        action.SetCallback([pending](const GameActions::GameAction*,const GameActions::Result*) {*pending=false;});
        return GameActions::Execute(&action,getGameState());
    }
    inline ManagementPresentation PaintPresentation(const PaintDraft& d,size_t selected,const std::string& status)
    {
        ManagementPresentation model;
        model.breadcrumb="Terrain / Paint review";
        model.title=SizeName(d.settings);
        model.stats={{"Surface",StyleName(ObjectType::terrainSurface,d.settings.surface)},
            {"Surface colour",SurfaceColourSupported(d.settings.surface)?ColourName(d.settings.surfaceColour):"Unsupported / preserved"},
            {"Edge",StyleName(ObjectType::terrainEdge,d.settings.edge)},
            {"Edge colour",EdgeColourSupported(d.settings.edge)?ColourName(d.settings.edgeColour):"Unsupported / preserved"},
            {"Cost",FormatMoney(d.cost)}};
        model.rows={"Apply paint","Cancel"}; model.selected=selected; model.confirmation=true;
        model.status=status.empty()?"Area / colour / price":status;
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::AreaTools

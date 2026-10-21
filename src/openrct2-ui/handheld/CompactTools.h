// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "TrackEditing.h"
#include "ManagementPanel.h"

namespace OpenRCT2::Ui::Handheld
{
    // Data and actions have separate layouts. Ordinary readouts never consume focus.
    inline void DrawTrackConnections(Drawing::RenderTarget& rt, const TrackEditing::Block& block,
        size_t selected, const std::string& status)
    {
        const auto lines=TrackEditing::Inspection(block);
        auto value=[&](const std::string& prefix,const std::string& fallback){
            for(const auto& line:lines) if(line.starts_with(prefix)) return line.substr(prefix.size());
            return fallback;
        };
        DrawSurface(rt,{8,36},{240,214});
        DrawHeadingText(rt,{20,48},"Track connections",216);
        DrawWrappedLabel(rt,{20,76},216,TrackEditing::Label(block),2);
        for(size_t i=0;i<2;++i)
        {
            const ScreenCoordsXY p{16,108+static_cast<int>(i)*68};
            const auto text=value(i?"Ahead: ":"Behind: ","Unavailable");
            DrawCard(rt,p,{224,62});
            DrawModernText(rt,p+ScreenCoordsXY{10,10},std::string("{MUTED}")+(i?"Ahead":"Behind"));
            DrawWrappedLabel(rt,p+ScreenCoordsXY{10,30},204,text,2);
        }
        const auto station=std::find_if(lines.begin(),lines.end(),[](const auto& line){return line.starts_with("Station ") && !line.starts_with("Station piece:");});
        if(station!=lines.end())
        {
            DrawSurface(rt,{8,258},{496,56});
            DrawStrongText(rt,{20,278},*station,84);
            DrawModernText(rt,{116,270},"{MUTED}Entrance");
            DrawStrongText(rt,{116,291},value("Entrance: ","Unavailable"),184);
            DrawModernText(rt,{320,270},"{MUTED}Exit");
            DrawStrongText(rt,{320,291},value("Exit: ","Unavailable"),172);
        }
        else if(!status.empty())
        {
            DrawSurface(rt,{8,258},{496,56});
            DrawWrappedLabel(rt,{20,270},472,status,2);
        }
        if(!status.empty() && station!=lines.end())
        {
            DrawSurface(rt,{264,36},{240,72});
            DrawWrappedLabel(rt,{276,48},216,status,3);
        }
        DrawActionRow(rt,{8,320},{242,28},"Technical details...","",selected==0);
        DrawActionRow(rt,{258,320},{246,28},"Back to piece","",selected==1);
        DrawFooter(rt,{ContextGetWidth(),ContextGetHeight()},{{"D-pad","Choose"},{"A","Select"},{"B","Piece"}});
    }
    inline void DrawDesignCheck(Drawing::RenderTarget& rt, const std::string& vehicle,
        const std::string& vehicleState, const std::string& scenery, const std::string& failure, size_t selected, const Object* vehicleObject, bool sceneryCanRemain)
    {
        DrawSurface(rt,{8,8},{496,340});
        DrawHeadingText(rt,{20,23},"Design check",472);
        DrawCard(rt,{16,54},{480,58});
        if(vehicleObject)
        {
            Drawing::RenderTarget preview;
            if(ClipRenderTarget(preview,rt,{22,62},34,42))
            {
                preview.zoom_level=ZoomLevel{1};
                vehicleObject->DrawPreview(preview,68,84);
            }
        }
        DrawStrongText(rt,{62,65},vehicle,420);
        DrawModernEllipsised(rt,{62,89},420,"{MUTED}"+vehicleState);
        DrawCard(rt,{16,122},{480,62});
        DrawModernText(rt,{28,133},"{MUTED}Scenery");
        DrawWrappedLabel(rt,{28,153},456,scenery,2);
        DrawCard(rt,{16,194},{480,78});
        DrawModernText(rt,{28,205},"{MUTED}Before placing");
        DrawWrappedLabel(rt,{28,225},456,
            sceneryCanRemain ? "Train limits may change settings. If placement fails, completed scenery can remain."
                : "Train limits may change the number of trains or cars.",2);
        if(!failure.empty()) DrawWrappedLabel(rt,{20,282},472,"{DANGER}"+failure,2);
        DrawActionRow(rt,{16,316},{230,28},"Technical details...","",selected==0);
        DrawActionRow(rt,{258,316},{238,28},"Back to options","",selected==1);
        DrawFooter(rt,{ContextGetWidth(),ContextGetHeight()},{{"D-pad","Choose"},{"A","Select"},{"B","Back"}});
    }

}

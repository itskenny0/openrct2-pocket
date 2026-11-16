// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ConceptStyle.h"
#include <filesystem>
#include <optional>
#include <openrct2/Context.h>
#include <openrct2/park/ParkPreview.h>
#include <openrct2/ParkImporter.h>
#include <openrct2/core/FileStream.h>
#include <openrct2/localisation/Localisation.Date.h>

namespace OpenRCT2::Ui::Handheld::SavedParkPresentation
{
    inline std::optional<ParkPreview> Read(const std::filesystem::path& path)
    {
        if (!ParkImporter::ExtensionIsOpenRCT2ParkFile(path.extension().u8string())) return {};
        try {
            auto importer=ParkImporter::CreateParkFile(GetContext()->GetObjectRepository());
            auto stream=FileStream(path.u8string(),FileMode::open);
            importer->LoadFromStream(&stream,false,true,path.u8string());
            return importer->GetParkPreview();
        } catch(const std::exception&) { return {}; }
    }
    inline std::string Date(const ParkPreview& park)
    {
        const auto month=FormatStringIDLegacy(DateGameMonthNames[std::clamp(park.month,0,7)],nullptr);
        return std::to_string(park.day+1)+" "+month+", year "+std::to_string(park.year+1);
    }
    inline void Draw(Drawing::RenderTarget& rt, const ParkPreview& park)
    {
        DrawWrappedLabel(rt,{258,50},236,park.parkName,2);
        const PreviewImage* image=nullptr;
        for(const auto& candidate:park.images) {
            if(!candidate.width || !candidate.height)continue;
            const auto begin=std::begin(candidate.pixels),end=begin+candidate.width*candidate.height;
            const bool useful=std::any_of(begin,end,[&](auto colour){return colour!=*begin;});
            if(useful && (!image || candidate.type==PreviewImageType::screenshot))image=&candidate;
        }
        if(image) {
            const int scale=std::min(236000/image->width,156000/image->height);
            const int width=image->width*scale/1000,height=image->height*scale/1000;
            const ScreenCoordsXY origin{258+(236-width)/2,90+(156-height)/2};
            Drawing::RenderTarget target;
            if(ClipRenderTarget(target,rt,origin,width,height))
                for(int y=target.y;y<target.y+target.height;++y)
                    for(int x=target.x;x<target.x+target.width;++x)
                        target.bits[(y-target.y)*target.LineStride()+x-target.x]=image->pixels[y*image->height/height*image->width+x*image->width/width];
        } else DrawModernText(rt,{258,154},"Preview unavailable",{},236);
        DrawModernText(rt,{258,262},Date(park),{},236);
        DrawModernText(rt,{258,287},"Guests "+std::to_string(park.numGuests)+" / Rides "+std::to_string(park.numRides),{},236);
        DrawModernText(rt,{258,312},"Park rating "+std::to_string(park.parkRating),{},236);
    }
}

// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "TabbedActions.h"
#include "StaffManagement.h"
#include "StaffPatrol.h"
#include "GuestManagement.h"
#include "ParkInformation.h"
namespace OpenRCT2::Ui::Handheld::PeopleTabs
{
    using State = TabFocus<4>;
    inline std::vector<size_t> StaffActions(size_t tab)
    {
        return tab == 0 ? std::vector<size_t>{0,9}
            : tab == 2 ? std::vector<size_t>{6}
            : std::vector<size_t>{2,7,8,4,3};
    }
    inline std::vector<size_t> GuestActions(size_t tab)
    {
        return tab == 0 ? std::vector<size_t>{0,11,9} : std::vector<size_t>{3,4,8,10,6,7};
    }
    inline void Actions(ManagementPresentation& model, const ManagementPresentation& native, const std::vector<size_t>& indices)
    {
        model.rows.clear();model.rowEnabled.clear();model.rowDetails.clear();
        for(auto index:indices) {
            if(index>=native.rows.size()) continue;
            model.rows.push_back(native.rows[index]);
            model.rowEnabled.push_back(index<native.rowEnabled.size()?native.rowEnabled[index]:true);
        }
        model.actionGrid=true;
    }
    inline void Activity(ManagementPresentation& model, const std::string& status)
    {
        const auto wrapped=GuestManagement::TextLines(status);
        model.bodyLines.assign(wrapped.begin(),wrapped.begin()+std::min<size_t>(2,wrapped.size()));
    }
    inline ManagementPresentation Staff(const StaffManagement::Target& target,const State& tabs,size_t selected,uint8_t draft)
    {
        using V=StaffManagement::View;
        auto native=StaffManagement::Presentation(V::inspection,selected,{},target);
        auto model=native;
        if(!StaffManagement::Resolve(target))return model;
        model.tabs={"Overview","Duties","Patrol","Details"};model.selectedTab=tabs.tab;model.selected=selected;
        model.headerStatus=StaffManagement::RoleName(target.type);model.summaryMetrics=true;model.metricHeight=40;
        if(tabs.tab==1) {
            const auto orders=StaffManagement::Presentation(V::orders,selected,{},target,StaffType::handyman,draft);
            model.stats.clear();model.rows=orders.rows;model.actionGrid=false;model.rowHeight=26;
            const bool changed = draft != StaffManagement::Resolve(target)->staffOrders;
            model.headerStatus = changed ? "Unsaved duties" : "Current duties";
            model.headerInk = changed ? "{CAUTION}" : "{WHITE}";
            model.status = changed ? "Unsaved duties" : "";
            model.bodyLines.clear();
            if(StaffManagement::Orders(target.type).empty()) {model.rows={"Overview"};model.bodyLines={"This role has no configurable duties."};}
        } else {
            Actions(model,native,StaffActions(tabs.tab));
            if(tabs.tab==0) {
                Activity(model,native.stats.back().second);
                model.stats.resize(2);
                const auto counters=StaffManagement::Presentation(V::statistics,0,{},target);
                for(size_t i=1;i<counters.stats.size();++i)model.stats.push_back(counters.stats[i]);
            } else if(tabs.tab==2) {
                const auto count=StaffPatrol::Snapshot(target).size();
                model.stats={{"Patrol tiles",std::to_string(count)},{"Coverage",count?"Assigned area":"Unrestricted"}};
                model.bodyLines={"Paint this employee's patrol area in the park.","A applies brush / B returns to this tab."};
            } else {
                model.stats={native.stats[1],StaffManagement::Presentation(V::statistics,0,{},target).stats.front()};
            }
            const auto* staff=StaffManagement::Resolve(target);
            for(size_t i=0;i<model.rows.size();++i) {
                if(model.rows[i]=="Costume...")model.rowEnabled[i]=target.type==StaffType::entertainer;
                if(model.rows[i]=="Uniform colour...")model.rowEnabled[i]=target.type!=StaffType::entertainer;
                if(model.rows[i]=="Pick up...")model.rowEnabled[i]=staff->canBePickedUp();
            }
        }
        return model;
    }
    // Native thought formatting adds smart quotes. Normalize only punctuation
    // supported by PocketSans so ordinary English thoughts use the readable UI
    // face; other scripts retain the engine fallback and the full native text.
    inline std::string ThoughtLabel(std::string text)
    {
        for(const auto& [from,to]:std::array<std::pair<std::string_view,std::string_view>,4>{
            std::pair{"\xE2\x80\x98","'"}, {"\xE2\x80\x99","'"},
            {"\xE2\x80\x9C","\""}, {"\xE2\x80\x9D","\""}})
        {
            size_t pos=0;
            while((pos=text.find(from,pos))!=std::string::npos){text.replace(pos,from.size(),to);pos+=to.size();}
        }
        return text;
    }
    inline ManagementPresentation Guest(const GuestManagement::Target& target,const State& tabs,size_t selected)
    {
        using V=GuestManagement::View;
        const auto native=GuestManagement::Presentation(V::inspection,selected,{},target);
        auto model=native;const auto* guest=GuestManagement::Resolve(target);if(!guest)return model;
        model.tabs={"Overview","Needs","Thoughts","Details"};model.selectedTab=tabs.tab;model.selected=selected;
        model.stats.clear();model.headerStatus=guest->peepFlags.has(PeepFlag::tracking)?"Tracked":"Guest";
        model.summaryMetrics=true;model.metricHeight=40;
        if(tabs.tab==1) {
            model.stats=GuestManagement::Presentation(V::needs,0,{},target).stats;
            model.metricHeight = 56;
            model.rows = { "Overview" };
            for (size_t i = 0; i < model.stats.size(); ++i)
            {
                model.meters.push_back(std::stoi(model.stats[i].second));
                model.meterDirections.push_back(i < 2 ? 1 : -1);
            }
            // Guest.cpp: native tired/hungry/thirsty/nausea/toilet thought predicates.
            const bool attention[]{ false,
                                    guest->energy <= 70 && guest->happiness < 128,
                                    guest->hunger <= 10 && !guest->hasFoodOrDrink(),
                                    guest->thirst <= 25 && !guest->hasFoodOrDrink(),
                                    guest->nausea >= 140,
                                    guest->toilet >= 160 };
            for (size_t i = 0; i < model.stats.size(); ++i)
                model.meterAttention.push_back(attention[i]);
            model.bodyLines.clear();
        } else if(tabs.tab==2) {
            model.rows = GuestManagement::Thoughts(*guest);
            for(auto& thought:model.rows)thought=ThoughtLabel(ParkInformation::PlainText(thought,true));
            model.actionGrid = false;
            model.rowHeight = 44;
            model.listLabel = "CURRENT THOUGHTS / A READ FULL TEXT";
            if(model.rows.empty()){model.bodyLines={"No current thoughts"};model.rows={"Overview"};}
        } else {
            Actions(model,native,GuestActions(tabs.tab));
            if(tabs.tab==0) {
                model.stats={{"Happiness",std::to_string(guest->happiness*100/255)+"%"},{"Rides ridden",std::to_string(guest->guestNumRides)},{"Tracking",guest->peepFlags.has(PeepFlag::tracking)?"On":"Off"}};
                Activity(model,GuestManagement::Status(*guest));
                for(size_t i=0;i<model.rows.size();++i)if(model.rows[i]=="Pick up...")model.rowEnabled[i]=guest->canBePickedUp();
            } else {
                auto spending=GuestManagement::Presentation(V::spending,0,{},target);
                model.stats={spending.stats[0],spending.stats[1]};
            }
        }
        return model;
    }
}


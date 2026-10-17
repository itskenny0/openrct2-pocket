// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "TabbedActions.h"
#include "StaffManagement.h"
#include "StaffPatrol.h"
#include "GuestManagement.h"
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
            model.stats.clear();model.rows=orders.rows;model.actionGrid=false;
            model.bodyLines={"A toggles duties. Apply orders saves changes."};
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
            model.metricHeight=60;model.rows={"Overview"};
            for(const auto& stat:model.stats)model.meters.push_back(std::stoi(stat.second));
            model.bodyLines={"Higher hunger, thirst and toilet bars indicate more need."};
        } else if(tabs.tab==2) {
            model.rows=GuestManagement::Thoughts(*guest);model.actionGrid=false;model.listLabel="CURRENT THOUGHTS / A READ FULL TEXT";
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

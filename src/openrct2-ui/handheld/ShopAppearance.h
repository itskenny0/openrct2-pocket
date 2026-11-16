// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ShopManagement.h"
#include "RideManagement.h"
#include "ManagementPanel.h"
#include "../interface/Dropdown.h"
#include <openrct2/ride/ShopAppearanceContext.h>
#include <openrct2/localisation/Formatting.h>

namespace OpenRCT2::Ui::Handheld::ShopAppearance
{
    using Setting=RideManagement::Setting;
    using A=GameActions::RideSetAppearanceType;
    inline std::vector<Setting> Settings(const ShopManagement::Target& target)
    {
        std::vector<Setting> result;
        const auto* ride=ShopManagement::Find(target);if(!ride)return result;
        for(auto [channel,name]:{std::pair{A::trackColourMain,"Building main"},
            {A::trackColourAdditional,"Building additional"},{A::trackColourSupports,"Supports"},
            {A::sellingItemColourIsRandom,"Random item colours"}}) {
            if(!ShopAppearanceContext::Supported(*ride,channel))continue;
            Setting s{GameActions::RideSetSetting::mode,name,{},{},ShopAppearanceContext::Value(*ride,channel)};
            s.appearance=channel;s.section=RideManagement::View::appearance;
            if(channel==A::sellingItemColourIsRandom){s.values={0,1};s.labels={"Off","On"};}
            else for(uint8_t i=0;i<(getGameState().cheats.allowSpecialColourSchemes?Drawing::kColourNumTotal:Drawing::kColourNumNormal);++i){
                s.values.push_back(static_cast<uint16_t>(Windows::ColourDropDownIndexToColour(i)));
                s.labels.push_back(FormatStringID(Windows::ColourDropDownIndexToName(i)));
            }
            result.push_back(std::move(s));
        }
        return result;
    }
    inline std::string Label(const Setting& s,uint16_t value)
    {
        const auto it=std::find(s.values.begin(),s.values.end(),value);
        return it==s.values.end()?"Native value unavailable":s.labels[it-s.values.begin()];
    }
    struct Draft {ShopManagement::Target target;Setting setting;uint16_t value;std::vector<int64_t> context;};
    inline std::optional<Draft> Begin(const ShopManagement::Target& target,size_t channel)
    {
        auto settings=Settings(target);const auto* ride=ShopManagement::Find(target);
        if(!ride || channel>=settings.size())return {};
        return Draft{target,settings[channel],settings[channel].current,ShopAppearanceContext::Capture(*ride)};
    }
    inline GameActions::Result Action(const Draft& draft,bool execute,GameActions::GameAction::Callback_t callback={})
    {
        const auto* ride=ShopManagement::Find(draft.target);
        if(!ride || !draft.setting.appearance || draft.context.empty() || draft.context!=ShopAppearanceContext::Capture(*ride)) {
            GameActions::Result result;result.error=GameActions::Status::disallowed;result.errorMessage="Shop changed; reopen appearance";return result;
        }
        if(draft.value==ShopAppearanceContext::Value(*ride,*draft.setting.appearance)) {
            GameActions::Result result;result.error=GameActions::Status::disallowed;
            result.errorMessage="Choose a different native value";return result;
        }
        GameActions::RideSetAppearanceAction action(draft.target.id,*draft.setting.appearance,draft.value,0);
        action.GuardShopContext(draft.context);action.SetCallback(std::move(callback));
        return execute?GameActions::Execute(&action,getGameState()):GameActions::Query(&action,getGameState());
    }
}

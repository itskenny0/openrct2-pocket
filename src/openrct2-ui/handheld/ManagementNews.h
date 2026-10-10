// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ParkInformation.h"
#include "ResearchCatalog.h"
namespace OpenRCT2::Ui::Handheld::ManagementNews
{
    enum class Link
    {
        none,
        finance,
        marketing,
        research
    };
    inline std::optional<ResearchCatalog::Target> ResearchTarget(const ParkInformation::NewsTarget& news)
    {
        if (!ParkInformation::Current(news) || news.item.hasButton() || !news.researchObject)
            return {};
        for (const auto& target : ResearchCatalog::Roster(0))
            if (target.item.rawValue == news.item.assoc && target.descriptor == news.researchObject
                && ResearchCatalog::Current(target) && ResearchCatalog::Invented(target.item))
                return target;
        return {};
    }
    inline Link Available(const ParkInformation::NewsTarget& news)
    {
        if (!ParkInformation::Current(news) || news.item.hasButton())
            return Link::none;
        if (news.item.type == News::ItemType::money)
            return Link::finance;
        if (news.item.type == News::ItemType::campaign)
            return Link::marketing;
        if (news.item.type == News::ItemType::research && ResearchTarget(news))
            return Link::research;
        return Link::none;
    }
    inline std::string Label(const ParkInformation::NewsTarget& news)
    {
        switch (Available(news))
        {
            case Link::finance:
                return "View Finances";
            case Link::marketing:
                return "View Marketing";
            case Link::research:
                return "Inspect invention";
            default:
                return ParkInformation::SubjectCurrent(news) ? "Inspect subject" : "Subject unavailable";
        }
    }
} // namespace OpenRCT2::Ui::Handheld::ManagementNews

// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "GuestManagement.h"
#include "ManagementPanel.h"
#include "RideManagement.h"
#include "StaffManagement.h"

#include <openrct2-ui/interface/Graph.h>
#include <openrct2/Date.h>
#include <openrct2/GameState.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/localisation/Localisation.Date.h>
#include <openrct2/localisation/StringIds.h>
#include <openrct2/management/Award.h>
#include <openrct2/management/NewsItem.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/Object.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/world/Map.h>

namespace OpenRCT2::Ui::Handheld::ParkInformation
{
    inline std::string PlainText(const std::string& text, bool oneLine = false)
    {
        std::string result;
        for (const auto& token : FmtString(std::string_view{ text }))
        {
            if (token.IsLiteral())
                result += token.text;
            else if (token.kind == FormatToken::newline || token.kind == FormatToken::newlineSmall)
                result += oneLine ? ' ' : '\n';
        }
        if (oneLine)
            for (auto& ch : result)
                if (ch == '\n' || ch == '\r' || ch == '\t')
                    ch = ' ';
        return result;
    }
    struct NewsTarget
    {
        News::Item item;
        bool archived{};
        std::optional<RideManagement::Target> ride;
        std::optional<GuestManagement::Target> guest;
        std::optional<StaffManagement::Target> staff;
        std::optional<ObjectEntryDescriptor> researchObject;
    };
    inline bool Same(const News::Item& a, const News::Item& b)
    {
        // Ticks advance while text/date/type/subject form the copied review baseline.
        return a.type == b.type && a.flags == b.flags && a.assoc == b.assoc && a.monthYear == b.monthYear && a.day == b.day
            && a.text == b.text;
    }
    inline std::vector<NewsTarget> NewsList()
    {
        std::vector<NewsTarget> result;
        const auto& queues = getGameState().park.newsItems;
        auto append = [&](const auto& queue, bool archived) {
            for (const auto& item : queue)
            {
                NewsTarget target{ item, archived, {}, {}, {}, {} };
                if (item.type == News::ItemType::ride && item.assoc < RideId::GetNull().ToUnderlying())
                    target.ride = RideManagement::Identify(RideId::FromUnderlying(item.assoc), true);
                else if (
                    (item.type == News::ItemType::peep || item.type == News::ItemType::peepOnRide)
                    && item.assoc < EntityId::GetNull().ToUnderlying())
                {
                    target.guest = GuestManagement::Identify(EntityId::FromUnderlying(item.assoc));
                    target.staff = StaffManagement::Identify(EntityId::FromUnderlying(item.assoc));
                }
                if (item.type == News::ItemType::research)
                {
                    const ResearchItem research(item.assoc, ResearchCategory::transport, 0);
                    if ((research.type == Research::EntryType::ride && research.baseRideType < RIDE_TYPE_COUNT)
                        || research.type == Research::EntryType::scenery)
                    {
                        const auto* object = GetContext()->GetObjectManager().GetLoadedObject(
                            research.type == Research::EntryType::ride ? ObjectType::ride : ObjectType::sceneryGroup,
                            research.entryIndex);
                        if (object)
                            target.researchObject = object->GetDescriptor();
                    }
                }
                result.push_back(std::move(target));
            }
        };
        append(queues.getRecent(), false);
        append(queues.getArchived(), true);
        return result;
    }
    inline bool Current(const NewsTarget& target)
    {
        const auto& queues = getGameState().park.newsItems;
        if (target.archived)
            return std::any_of(
                queues.getArchived().begin(), queues.getArchived().end(), [&](const auto& i) { return Same(i, target.item); });
        return std::any_of(
            queues.getRecent().begin(), queues.getRecent().end(), [&](const auto& i) { return Same(i, target.item); });
    }
    inline bool IsCurrentMessage(const NewsTarget& target)
    {
        const auto& recent=getGameState().park.newsItems.getRecent();
        return !target.archived && Current(target) && !recent.empty() && Same(recent.front(),target.item);
    }
    inline bool DismissCurrentMessage(const NewsTarget& target)
    {
        if(!IsCurrentMessage(target))return false;
        News::CloseCurrentItem(getGameState().park.newsItems);return true;
    }
    inline bool SubjectCurrent(const NewsTarget& target)
    {
        return Current(target) && !target.item.hasButton()
            && ((target.ride && RideManagement::Resolve(*target.ride))
                || (target.guest && GuestManagement::Resolve(*target.guest))
                || (target.staff && StaffManagement::Resolve(*target.staff)));
    }
    inline std::optional<CoordsXYZ> Location(const NewsTarget& target)
    {
        if (!Current(target) || !target.item.typeHasLocation() || target.item.hasButton())
            return {};
        if (target.item.type == News::ItemType::blank)
        {
            const CoordsXY point{ static_cast<int16_t>(target.item.assoc & 0xffff),
                                  static_cast<int16_t>(target.item.assoc >> 16) };
            if (!MapIsLocationValid(point))
                return {};
            return News::GetSubjectLocation(target.item.type, target.item.assoc);
        }
        if (!SubjectCurrent(target))
            return {};
        return News::GetSubjectLocation(target.item.type, target.item.assoc);
    }
    inline std::string Date(const News::Item& item)
    {
        if (item.day < 1 || item.day > 31)
            return "Date unavailable";
        auto ft = Formatter();
        ft.Add<StringId>(DateDayNames[item.day - 1]);
        ft.Add<StringId>(DateGameMonthNames[DateGetMonth(item.monthYear)]);
        return FormatStringIDLegacy(STR_NEWS_DATE_FORMAT, ft.Data()) + ", year "
            + std::to_string(DateGetYear(item.monthYear) + 1);
    }
    inline ManagementPresentation NewsPresentation(const std::vector<NewsTarget>& list, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Overview / Park news";
        model.title = "Park news";
        model.selected = selected;
        for (const auto& target : list)
            model.rows.push_back(std::string(target.archived ? "Archive: " : "Recent: ") + PlainText(target.item.text, true));
        if (list.empty())
        {
            model.rows = { "Refresh" };
            model.status = "No park news yet";
        }
        else
            model.status = "A Details  B Overview";
        if (!list.empty())
            model.rows.push_back("Refresh news");
        return model;
    }
    inline ManagementPresentation NewsDetail(const NewsTarget& target, size_t page, size_t selected, const std::string& status)
    {
        ManagementPresentation model;
        model.breadcrumb = "Overview / Park news";
        model.title = Current(target) ? Date(target.item) : "News item expired";
        const auto lines = GuestManagement::TextLines(PlainText(target.item.text));
        const auto pages = std::max<size_t>(1, (lines.size() + 5) / 6);
        const auto first = std::min(page, pages - 1) * 6;
        for (size_t i = first; i < std::min(first + 6, lines.size()); ++i)
            model.bodyLines.push_back(lines[i]);
        model.rows = { "Next text page", Location(target) ? "Locate subject" : "Location unavailable",
                       SubjectCurrent(target) ? "Inspect subject" : "Subject unavailable", "Back to news" };
        if(IsCurrentMessage(target))model.rows.push_back("Dismiss current message");
        model.selected = selected;
        model.status = status.empty() ? "Text page " + std::to_string(first / 6 + 1) + " / " + std::to_string(pages) : status;
        return model;
    }
    inline ManagementPresentation Awards(size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Overview / Awards";
        model.title = "Current park awards";
        model.selected = selected;
        for (const auto& award : getGameState().park.currentAwards)
            if (award.time && award.type < AwardType::count)
                model.rows.push_back(
                    std::string(AwardIsPositive(award.type) ? "Positive: " : "Negative: ")
                    + PlainText(FormatStringIDLegacy(AwardGetText(award.type), nullptr), true));
        if (model.rows.empty())
            model.status = "No active awards";
        else
            model.status = "Current awards";
        model.rows.push_back("Return to Overview");
        return model;
    }
    inline ManagementPresentation History(bool guests, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Overview / History";
        model.title = guests ? "Guests in park history" : "Park rating history";
        model.stats = { { guests ? "Guests in park" : "Current rating",
                          std::to_string(guests ? getGameState().park.numGuestsInPark : getGameState().park.rating) } };
        model.rows = { guests ? "Show park rating" : "Show guests in park", "Return to Overview" };
        model.selected = selected;
        model.status = "Weekly history; gaps mean no record";
        return model;
    }
    inline void DrawHistory(Drawing::RenderTarget& rt, bool guests)
    {
        // Exact native graph ordering, timestamps and undefined sentinel handling.
        if (guests)
        {
            Graph::GraphProperties<uint32_t> props{};
            props.series = getGameState().park.guestsInParkHistory;
            props.min = 0;
            props.max = 3;
            props.hoverIdx = -1;
            for (auto value : getGameState().park.guestsInParkHistory)
                if (value != kGuestsInParkHistoryUndefined)
                    props.max = std::max(props.max, value);
            props.darkBackground = true;
            props.lineCol = ColourWithFlags{ Drawing::Colour::grey };
            props.RecalculateLayout({ { 64, 124 }, { 476, 236 } }, 4, kGuestsInParkHistorySize);
            Graph::DrawGuestGraph(rt, props);
        }
        else
        {
            Graph::GraphProperties<uint16_t> props{};
            props.series = getGameState().park.ratingHistory;
            props.min = 0;
            props.max = 1000;
            props.hoverIdx = -1;
            props.darkBackground = true;
            props.lineCol = ColourWithFlags{ Drawing::Colour::grey };
            props.RecalculateLayout({ { 64, 124 }, { 476, 236 } }, 4, kParkRatingHistorySize);
            Graph::DrawRatingGraph(rt, props);
        }
    }
} // namespace OpenRCT2::Ui::Handheld::ParkInformation

// SPDX-License-Identifier: GPL-3.0-only
#include "GuestManagement.h"

#include "TextEntry.h"

#include <openrct2/GameState.h>
#include <openrct2/actions/peep/GuestSetFlagsAction.h>
#include <openrct2/actions/peep/GuestSetNameAction.h>
#include <openrct2/core/EnumUtils.hpp>
#include <openrct2/drawing/Drawing.String.h>
#include <openrct2/entity/EntityList.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/interface/Window.h>
#include <openrct2/interface/WindowBase.h>
#include <openrct2/localisation/Formatter.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/localisation/StringIds.h>
#include <openrct2/peep/PeepActionFormat.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/RideManager.hpp>
#include <unordered_map>

namespace OpenRCT2::Ui::Handheld::GuestManagement
{
    static GameActions::Result Invalid(const char* text)
    {
        GameActions::Result result;
        result.error = GameActions::Status::disallowed;
        result.errorMessage = text;
        return result;
    }
    GameActions::Result SetName(const Target& target, const std::string& baseline, const std::string& value, bool execute)
    {
        const auto* guest = Resolve(target);
        if (!guest || guest->getName() != baseline || value == baseline || (value.empty() && !guest->name)
            || TextEntry{ value }.Length() > TextEntry::limit)
            return Invalid("Name changed or unavailable");
        GameActions::GuestSetNameAction action(target.id, value);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    GameActions::Result SetTracking(const Target& target, PeepFlags baseline, bool value, bool execute)
    {
        const auto* guest = Resolve(target);
        if (!guest || guest->peepFlags.holder != baseline.holder || baseline.has(PeepFlag::tracking) == value)
            return Invalid("Guest flags changed; reopen review");
        auto flags = baseline;
        flags.flip(PeepFlag::tracking);
        GameActions::GuestSetFlagsAction action(target.id, flags);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    ManagementPresentation TrackingPresentation(const Target& target, bool value, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Guests / Review tracking";
        const auto* guest = Resolve(target);
        model.title = guest ? guest->getName() : "Guest unavailable";
        model.stats = { { "Tracking", value ? "Enable" : "Disable" } };
        model.rows = { value ? "Enable tracking" : "Stop tracking", "Cancel" };
        model.selected = selected;
        model.confirmation = true;
        return model;
    }
    static std::unordered_map<uint16_t, uint64_t> removedGuests;
    void Invalidate(EntityId id)
    {
        if (auto it = removedGuests.find(id.ToUnderlying()); it != removedGuests.end())
            ++it->second;
    }
    std::optional<Target> Identify(EntityId id)
    {
        const auto* guest = getGameState().entities.tryGetEntity<Guest>(id);
        if (!guest || guest->outsideOfPark)
            return {};
        return Target{ id, guest->peepId, guest->getParkEntryTime(), removedGuests[id.ToUnderlying()] };
    }
    const Guest* Resolve(const Target& target)
    {
        const auto current = Identify(target.id);
        return current && *current == target ? getGameState().entities.tryGetEntity<Guest>(target.id) : nullptr;
    }
    static std::array<uint8_t, 12> Arguments(const Guest& guest, GroupMode mode)
    {
        std::array<uint8_t, 12> args{};
        Formatter ft(args.data());
        if (mode == GroupMode::actions)
            formatPeepActionTo(guest, ft, true);
        else if (guest.thoughts[0].type != PeepThoughtType::none && guest.thoughts[0].freshness <= 5)
            PeepThoughtSetFormatArgs(&guest.thoughts[0], ft);
        return args;
    }
    std::vector<Group> Groups(GroupMode mode)
    {
        std::vector<Group> groups;
        for (const auto* guest : EntityList<Guest>())
        {
            if (guest->outsideOfPark)
                continue;
            const auto args = Arguments(*guest, mode);
            StringId id{};
            std::memcpy(&id, args.data(), sizeof(id));
            if (id == kStringIdEmpty)
                continue; // Same empty-group exclusion as desktop GuestList.
            auto found = std::find_if(groups.begin(), groups.end(), [&](const auto& g) { return g.arguments == args; });
            if (found == groups.end())
                groups.push_back({ mode, args, FormatStringIDLegacy(STR_STRINGID, args.data()), 1 });
            else
                ++found->count;
        }
        std::stable_sort(groups.begin(), groups.end(), [](const auto& a, const auto& b) { return a.count > b.count; });
        return groups;
    }
    ManagementPresentation GroupPresentation(GroupMode mode, const std::vector<Group>& groups, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Guests / Native groups";
        model.title = mode == GroupMode::actions ? "Action groups" : "Thought groups";
        model.selected = selected;
        for (const auto& g : groups)
            model.rows.push_back(std::to_string(g.count) + " / " + g.label);
        model.rows.push_back("Back to Guests");
        if (groups.empty())
            model.status = "No current groups";
        return model;
    }
    std::vector<Target> Roster(const Filter& filter)
    {
        std::vector<Target> result;
        for (const auto* guest : EntityList<Guest>())
            if (auto target = Identify(guest->id); target && (!filter.tracked || guest->peepFlags.has(PeepFlag::tracking))
                && (!filter.group || Arguments(*guest, filter.group->mode) == filter.group->arguments))
                result.push_back(*target);
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return a.peepId == b.peepId ? a.id < b.id : a.peepId < b.peepId;
        });
        return result;
    }
    std::string Status(const Guest& guest)
    {
        Formatter ft;
        formatPeepActionTo(guest, ft);
        return FormatStringIDLegacy(STR_STRINGID, ft.Data());
    }
    std::vector<std::string> Thoughts(const Guest& guest)
    {
        std::vector<std::string> result;
        for (const auto& thought : guest.thoughts)
        {
            if (thought.type == PeepThoughtType::none)
                break;
            if (!thought.freshness)
                continue;
            Formatter ft;
            PeepThoughtSetFormatArgs(&thought, ft);
            result.push_back(FormatStringIDLegacy(STR_STRINGID, ft.Data()));
        }
        return result;
    }
    std::vector<std::string> Inventory(const Guest& guest)
    {
        std::vector<std::string> result;
        for (ShopItem item = ShopItem::balloon; item < ShopItem::count; item++)
        {
            if (!guest.hasItem(item))
                continue;
            // Same native display-string arguments as the desktop Guest inventory.
            Formatter ft;
            ft.Add<StringId>(GetShopItemDescriptor(item).Naming.Display);
            ft.Add<StringId>(STR_STRING);
            ft.Add<const char*>(getGameState().park.name.c_str());
            const Ride* ride = nullptr;
            switch (item)
            {
                case ShopItem::photo:
                    ride = GetRide(guest.photo1RideRef);
                    break;
                case ShopItem::photo2:
                    ride = GetRide(guest.photo2RideRef);
                    break;
                case ShopItem::photo3:
                    ride = GetRide(guest.photo3RideRef);
                    break;
                case ShopItem::photo4:
                    ride = GetRide(guest.photo4RideRef);
                    break;
                case ShopItem::voucher:
                    ft.Rewind();
                    ft.Increment(2);
                    switch (guest.voucherType)
                    {
                        case VOUCHER_TYPE_PARK_ENTRY_FREE:
                            ft.Add<StringId>(STR_PEEP_INVENTORY_VOUCHER_PARK_ENTRY_FREE);
                            ft.Add<StringId>(STR_STRING);
                            ft.Add<const char*>(getGameState().park.name.c_str());
                            break;
                        case VOUCHER_TYPE_PARK_ENTRY_HALF_PRICE:
                            ft.Add<StringId>(STR_PEEP_INVENTORY_VOUCHER_PARK_ENTRY_HALF_PRICE);
                            ft.Add<StringId>(STR_STRING);
                            ft.Add<const char*>(getGameState().park.name.c_str());
                            break;
                        case VOUCHER_TYPE_RIDE_FREE:
                            ft.Add<StringId>(STR_PEEP_INVENTORY_VOUCHER_RIDE_FREE);
                            ride = GetRide(guest.voucherRideId);
                            if (ride)
                                ride->formatNameTo(ft);
                            else
                                ft.Add<StringId>(kStringIdNone);
                            break;
                        case VOUCHER_TYPE_FOOD_OR_DRINK_FREE:
                            ft.Add<StringId>(STR_PEEP_INVENTORY_VOUCHER_FOOD_OR_DRINK_FREE);
                            ft.Add<StringId>(GetShopItemDescriptor(guest.voucherShopItem).Naming.Singular);
                            break;
                        default:
                            break;
                    }
                    ride = nullptr;
                    break;
                default:
                    break;
            }
            if (ride)
            {
                ft.Rewind();
                ft.Increment(2);
                ride->formatNameTo(ft);
            }
            result.push_back(FormatStringIDLegacy(STR_STRINGID, ft.Data()));
        }
        return result;
    }
    std::vector<std::string> TextLines(const std::string& text)
    {
        // Native UTF-8/format-aware wrapping. Six lines per page keeps full text accessible.
        std::string wrapped;
        int32_t count{};
        Drawing::wrapString(text, 320, FontStyle::medium, &wrapped, &count);
        std::vector<std::string> lines;
        size_t start = 0;
        while (start < wrapped.size())
        {
            const auto end = wrapped.find('\0', start);
            lines.push_back(wrapped.substr(start, end == std::string::npos ? end : end - start));
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        return lines;
    }
    std::optional<CoordsXYZ> Location(const Target& target)
    {
        const auto* guest = Resolve(target);
        if (!guest)
            return {};
        WindowBase subject{};
        // Reuse the native guest viewport resolver, including vehicles and ride fallback.
        ViewportUpdateSmartFollowGuest(&subject, *guest);
        if (!subject.focus)
            return {};
        const auto location = subject.focus->GetPos();
        return location.isNull() ? std::nullopt : std::optional<CoordsXYZ>{ location };
    }
    bool Locate(const Target& target)
    {
        const auto location = Location(target);
        auto* main = WindowGetMain();
        if (!location || !main)
            return false;
        WindowScrollToLocation(*main, *location);
        return true;
    }
    std::vector<RideManagement::Target> RideHistory(const Target& target)
    {
        std::vector<RideManagement::Target> result;
        const auto* guest = Resolve(target);
        if (!guest) return result;
        // Desktop Guest::onUpdateRides uses this native set membership and RideManager order.
        for (const auto& ride : RideManager(getGameState()))
            if (ride.isRide() && guest->hasRidden(ride))
                if (auto identity = RideManagement::Identify(ride.id)) result.push_back(*identity);
        return result;
    }
    bool HistoryRideCurrent(const Target& target, const RideManagement::Target& rideTarget)
    {
        const auto* guest = Resolve(target);
        const auto* ride = RideManagement::Resolve(rideTarget);
        return guest && ride && ride->isRide() && guest->hasRidden(*ride);
    }
    ManagementPresentation HistoryPresentation(const Target& target, const std::vector<RideManagement::Target>& list, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Guests / Native ride history";
        const auto* guest = Resolve(target);
        model.title = guest ? guest->getName() : "Guest unavailable";
        model.selected = selected;
        for (const auto& rideTarget : list)
        {
            const auto* ride = RideManagement::Resolve(rideTarget);
            model.rows.push_back(HistoryRideCurrent(target, rideTarget) ? ride->getName() : "Ride unavailable");
        }
        model.rows.push_back("Back to guest");
        model.status = !guest ? "Guest no longer in park" : list.empty() ? "No retained native rides" : "Native ride order; A Inspect ride";
        return model;
    }
    ManagementPresentation Presentation(
        View view, size_t selected, const std::vector<Target>& roster, const std::optional<Target>& target,
        const std::string& status, const std::string& text, size_t textPage)
    {
        ManagementPresentation model;
        model.breadcrumb = "Park dashboard / Guests";
        model.title = "Guests";
        model.selected = selected;
        model.status = status;
        if (view == View::overview)
        {
            uint64_t happiness = 0;
            size_t count = 0;
            for (const auto* guest : EntityList<Guest>())
                if (!guest->outsideOfPark)
                {
                    happiness += guest->happiness;
                    ++count;
                }
            model.stats = { { "In park", std::to_string(getGameState().park.numGuestsInPark) },
                            { "Heading here", std::to_string(getGameState().park.numGuestsHeadingForPark) },
                            { "Avg happiness",
                              count == 0 ? "Unavailable" : std::to_string(happiness * 100 / (count * 255)) + "%" } };
            model.rows = { "Guest roster...", "Back to dashboard", "Action groups...", "Thought groups...",
                           "Tracked guests..." };
            return model;
        }
        if (view == View::roster)
        {
            for (const auto& entry : roster)
                model.rows.push_back(Resolve(entry) ? Resolve(entry)->getName() : "Guest unavailable");
            model.rows.push_back("Back to Guests");
            if (roster.empty())
                model.status = "No guests inside the park";
            return model;
        }
        const auto* guest = target ? Resolve(*target) : nullptr;
        if (!guest)
        {
            model.status = "Guest no longer in park";
            model.rows = { "Back to roster" };
            return model;
        }
        model.title = guest->getName();
        if (view == View::inspection)
        {
            model.stats = { { "Status", Status(*guest) }, { "Rides ridden", std::to_string(guest->guestNumRides) } };
            model.rows = {
                "Locate",           "Needs...",
                "Thoughts...",      "Spending...",
                "Carried items...", "Back to roster",
                "Name...",          guest->peepFlags.has(PeepFlag::tracking) ? "Tracking: On..." : "Tracking: Off...",
                "Preferences...",   "Pick up...", "Ride history..."
            };
        }
        else if (view == View::needs)
        {
            auto percent = [](int value, int maximum, int minimum = 0) {
                return std::to_string(std::clamp(value * 100 / maximum, minimum, 100)) + "%";
            };
            model.breadcrumb = "Guests / Needs";
            // Exact desktop Guest progress-bar normalization, including hunger/thirst inversion.
            model.stats = { { "Happiness", percent(guest->happiness, 255, 3) },
                            { "Energy", percent(guest->energy - kPeepMinEnergy, kPeepMaxEnergy - kPeepMinEnergy, 3) },
                            { "Hunger", std::to_string(100 - std::clamp((guest->hunger - 32) * 100 / 158, 0, 100)) + "%" },
                            { "Thirst", std::to_string(100 - std::clamp((guest->thirst - 32) * 100 / 158, 0, 100)) + "%" },
                            { "Nausea", percent(guest->nausea - 32, 223) },
                            { "Toilet", percent(guest->toilet - 64, 178) } };
            model.rows = { "Back to guest" };
        }
        else if (view == View::preferences)
        {
            model.breadcrumb = "Guests / Preferences";
            Formatter ft;
            const auto min = guest->intensity.getMinimum(), max = guest->intensity.getMaximum();
            auto intensityId = STR_GUEST_STAT_PREFERRED_INTESITY_BELOW;
            if (min)
            {
                ft.Add<uint16_t>(min);
                ft.Add<uint16_t>(max);
                intensityId = max == 15 ? STR_GUEST_STAT_PREFERRED_INTESITY_ABOVE : STR_GUEST_STAT_PREFERRED_INTESITY_BETWEEN;
            }
            else
                ft.Add<uint16_t>(max);
            model.bodyLines.push_back(FmtString(FormatStringIDLegacy(intensityId, ft.Data())).WithoutFormatTokens());
            constexpr StringId tolerances[] = { STR_PEEP_STAT_NAUSEA_TOLERANCE_NONE, STR_PEEP_STAT_NAUSEA_TOLERANCE_LOW,
                                                STR_PEEP_STAT_NAUSEA_TOLERANCE_AVERAGE, STR_PEEP_STAT_NAUSEA_TOLERANCE_HIGH };
            model.stats = { { "Nausea tolerance", FormatStringID(tolerances[EnumValue(guest->nauseaTolerance) & 3]) } };
            ft = Formatter();
            if (const auto* ride = GetRide(guest->favouriteRide))
            {
                ride->formatNameTo(ft);
                model.bodyLines.push_back("Favourite: " + FormatStringIDLegacy(STR_STRINGID, ft.Data()));
            }
            else
                model.bodyLines.push_back("Favourite ride: None");
            if (guest->getParkEntryTime() != -1)
            {
                ft = Formatter();
                ft.Add<uint16_t>(((getGameState().currentTicks - guest->getParkEntryTime()) >> 11) & 0xffff);
                model.bodyLines.push_back(
                    FmtString(FormatStringIDLegacy(STR_GUEST_STAT_TIME_IN_PARK, ft.Data())).WithoutFormatTokens());
            }
            model.rows = { "Back to guest" };
        }
        else if (view == View::spending)
        {
            model.breadcrumb = "Guests / Spending";
            auto money = [](money64 amount) {
                return getGameState().park.flags.has(ParkFlag::noMoney) ? std::string("Not applicable") : FormatMoney(amount);
            };
            model.stats = { { "Cash carried", money(guest->cashInPocket) }, { "Total spent", money(guest->cashSpent) },
                            { "Park entry", money(guest->paidToEnter) },    { "Rides", money(guest->paidOnRides) },
                            { "Food", money(guest->paidOnFood) },           { "Drinks", money(guest->paidOnDrink) },
                            { "Souvenirs", money(guest->paidOnSouvenirs) } };
            model.rows = { "Back to guest" };
        }
        else if (view == View::thoughts || view == View::inventory)
        {
            model.breadcrumb = view == View::thoughts ? "Guests / Thoughts" : "Guests / Carried items";
            model.rows = view == View::thoughts ? Thoughts(*guest) : Inventory(*guest);
            if (model.rows.empty())
                model.status = view == View::thoughts ? "No current thoughts" : "Nothing carried";
            model.rows.push_back("Back to guest");
        }
        else
        {
            model.breadcrumb = "Guests / Read detail";
            const auto lines = TextLines(text);
            const auto pages = std::max<size_t>(1, (lines.size() + 5) / 6);
            const auto page = std::min(textPage, pages - 1);
            const auto first = std::min(page * 6, lines.size());
            model.bodyLines.assign(lines.begin() + first, lines.begin() + std::min(first + 6, lines.size()));
            if (pages > 1)
            {
                model.rows = { "Previous page", "Next page" };
                model.status = "Text " + std::to_string(page + 1) + " / " + std::to_string(pages);
            }
            model.rows.push_back("Back to list");
        }
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::GuestManagement

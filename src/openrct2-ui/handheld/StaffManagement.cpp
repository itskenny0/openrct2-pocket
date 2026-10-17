// SPDX-License-Identifier: GPL-3.0-only
#include "StaffManagement.h"

#include <openrct2-ui/interface/Dropdown.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/peep/StaffFireAction.h>
#include <openrct2/actions/peep/StaffHireNewAction.h>
#include <openrct2/actions/peep/StaffSetColourAction.h>
#include <openrct2/actions/peep/StaffSetCostumeAction.h>
#include <openrct2/actions/peep/StaffSetNameAction.h>
#include <openrct2/actions/peep/StaffSetOrdersAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/entity/EntityList.h>
#include <openrct2/interface/Window.h>
#include <openrct2/localisation/Formatter.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/PeepAnimationsObject.h>
#include <openrct2/peep/PeepActionFormat.h>
#include <openrct2/peep/PeepAnimations.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Map.h>
#include <unordered_map>

namespace OpenRCT2::Ui::Handheld::StaffManagement
{
    static std::unordered_map<uint16_t, uint64_t> gRemovedStaff;
    void Invalidate(EntityId id)
    {
        if (auto found = gRemovedStaff.find(id.ToUnderlying()); found != gRemovedStaff.end())
            ++found->second;
    }
    static GameActions::Result Invalid(const char* message)
    {
        GameActions::Result result;
        result.error = GameActions::Status::disallowed;
        result.errorMessage = message;
        return result;
    }
    bool Supported(StaffType type)
    {
        return type < StaffType::count;
    }
    std::string RoleName(StaffType type)
    {
        const char* names[] = { "Handyman", "Mechanic", "Security guard", "Entertainer" };
        return Supported(type) ? names[EnumValue(type)] : "Unavailable role";
    }
    std::vector<std::pair<ObjectEntryIndex, std::string>> Costumes()
    {
        std::vector<std::pair<ObjectEntryIndex, std::string>> entries;
        for (const auto& costume : getAvailableCostumeStrings(AnimationPeepType::entertainer))
            entries.push_back({ costume.index, costume.rawName });
        return entries;
    }
    GameActions::Result SetCostume(const Target& target, ObjectEntryIndex baseline, ObjectEntryIndex value, bool execute)
    {
        const auto* staff = Resolve(target);
        if (!staff || target.type != StaffType::entertainer || staff->animationObjectIndex != baseline || value == baseline)
            return Invalid("Costume changed or unavailable");
        const auto entries = Costumes();
        if (std::none_of(entries.begin(), entries.end(), [value](const auto& item) { return item.first == value; }))
            return Invalid("Costume unavailable");
        GameActions::StaffSetCostumeAction action(target.id, value);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    GameActions::Result SetUniform(const Target& target, Drawing::Colour baseline, Drawing::Colour value, bool execute)
    {
        if (!Resolve(target) || target.type == StaffType::entertainer || EnumValue(value) >= 72
            || StaffGetColour(target.type) != baseline || value == baseline)
            return Invalid("Uniform changed or unavailable");
        GameActions::StaffSetColourAction action(target.type, value);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    ManagementPresentation UniformPresentation(const Target& target, Drawing::Colour value, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Staff / Role uniform";
        model.title = RoleName(target.type) + " uniforms";
        std::string name = "Special colour";
        for (uint8_t i = 0; i < 72; ++i)
            if (Windows::ColourDropDownIndexToColour(i) == value)
                name = FmtString(FormatStringID(Windows::ColourDropDownIndexToName(i))).WithoutFormatTokens();
        model.rows = { "Colour: " + name, "Apply to entire role", "Cancel" };
        model.selected = selected;
        model.status = "Left / Right changes colour";
        return model;
    }
    GameActions::Result SetName(const Target& target, const std::string& baseline, const std::string& value, bool execute)
    {
        const auto* staff = Resolve(target);
        if (!staff || staff->getName() != baseline || value == baseline || (value.empty() && !staff->name)
            || TextEntry{ value }.Length() > TextEntry::limit)
            return Invalid("Name changed or unavailable");
        GameActions::StaffSetNameAction action(target.id, value);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    ManagementPresentation NamePresentation(const TextEntry& entry, size_t selected, bool review, const std::string& status)
    {
        ManagementPresentation model;
        model.breadcrumb = review ? "Staff / Review name" : "Staff / Name keyboard";
        model.title = entry.text.empty() ? "Default name" : entry.text;
        model.selected = selected;
        model.rows = review ? std::vector<std::string>{ "Apply name", "Cancel" } : entry.Keys();
        model.columns = review ? 1 : TextEntry::columns;
        model.confirmation = review;
        model.primaryAction = review ? "Confirm" : "Type";
        model.status = status.empty() ? std::to_string(entry.Length()) + " / 32 characters" : status;
        return model;
    }
    std::optional<Target> Identify(EntityId id)
    {
        auto* staff = getGameState().entities.tryGetEntity<Staff>(id);
        if (!staff || !Supported(staff->assignedStaffType))
            return {};
        return Target{ staff->id, staff->assignedStaffType, staff->peepId, staff->hireDate, gRemovedStaff[id.ToUnderlying()] };
    }
    Staff* Resolve(const Target& target)
    {
        const auto current = Identify(target.id);
        return current && *current == target ? getGameState().entities.tryGetEntity<Staff>(target.id) : nullptr;
    }
    std::vector<Target> Roster(std::optional<StaffType> filter)
    {
        std::vector<Target> result;
        for (auto* staff : EntityList<Staff>())
            if (auto target = Identify(staff->id); target && (!filter || target->type == *filter))
                result.push_back(*target);
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return a.type != b.type ? a.type < b.type : a.peepId < b.peepId;
        });
        return result;
    }
    std::vector<std::pair<uint8_t, std::string>> Orders(StaffType type)
    {
        if (type == StaffType::handyman)
            return { { STAFF_ORDERS_SWEEPING, "Sweep paths" },
                     { STAFF_ORDERS_WATER_FLOWERS, "Water gardens" },
                     { STAFF_ORDERS_EMPTY_BINS, "Empty bins" },
                     { STAFF_ORDERS_MOWING, "Mow grass" } };
        if (type == StaffType::mechanic)
            return { { STAFF_ORDERS_INSPECT_RIDES, "Inspect rides" }, { STAFF_ORDERS_FIX_RIDES, "Fix rides" } };
        return {};
    }
    GameActions::Result Hire(StaffType type, bool execute, GameActions::GameAction::Callback_t callback, bool manual)
    {
        if (!Supported(type))
            return Invalid("Unsupported staff role");
        const auto animation = findPeepAnimationsIndexForType(AnimationPeepType(EnumValue(type) + 1));
        if (animation == kObjectEntryIndexNull
            || !GetContext()->GetObjectManager().GetLoadedObject<PeepAnimationsObject>(animation))
            return Invalid("Staff animation unavailable");
        // Native auto-placement can fall back to picked-up staff if no entry or walking guest exists.
        // This workflow has no manual pickup tool, so avoid creating an unplaced employee.
        bool canPosition = !getGameState().park.entrances.empty();
        for (auto* guest : EntityList<Guest>())
            if (guest->state == PeepState::walking && MapGetPathElementAt(TileCoordsXYZ{ guest->nextLoc }))
                canPosition = true;
        if (!manual && !canPosition)
            return Invalid("No automatic placement location");
        uint8_t orders = type == StaffType::mechanic ? STAFF_ORDERS_INSPECT_RIDES | STAFF_ORDERS_FIX_RIDES
            : type == StaffType::handyman ? STAFF_ORDERS_SWEEPING | STAFF_ORDERS_WATER_FLOWERS | STAFF_ORDERS_EMPTY_BINS
                                          : 0;
        if (type == StaffType::handyman && Config::Get().general.handymenMowByDefault)
            orders |= STAFF_ORDERS_MOWING;
        GameActions::StaffHireNewAction action(!manual, type, animation, orders);
        action.SetCallback(std::move(callback));
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    GameActions::Result SetOrders(
        const Target& target, uint8_t baseline, uint8_t value, bool execute, GameActions::GameAction::Callback_t callback)
    {
        const auto* staff = Resolve(target);
        if (!staff)
            return Invalid("Employee unavailable");
        if (staff->staffOrders != baseline)
            return Invalid("Orders changed; reopen editor");
        uint8_t mask = 0;
        for (const auto& order : Orders(target.type))
            mask |= order.first;
        if ((value & ~mask) != (baseline & ~mask))
            return Invalid("Unsupported orders");
        GameActions::StaffSetOrdersAction action(target.id, value);
        action.SetCallback(std::move(callback));
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    GameActions::Result Dismiss(const Target& target, bool execute)
    {
        if (!Resolve(target))
            return Invalid("Employee unavailable");
        GameActions::StaffFireAction action(target.id);
        action.SetCallback([id = target.id](const GameActions::GameAction*, const GameActions::Result* result) {
            if (result->error == GameActions::Status::ok)
                Invalidate(id);
        });
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    bool Locate(const Target& target)
    {
        const auto* staff = Resolve(target);
        if (!staff || staff->getLocation().isNull())
            return false;
        auto* main = WindowGetMain();
        if (!main)
            return false;
        WindowScrollToLocation(*main, staff->getLocation());
        return true;
    }
    ManagementPresentation Presentation(
        View view, size_t selected, const std::vector<Target>& roster, const std::optional<Target>& target, StaffType hireType,
        std::optional<uint8_t> orders, const std::string& status)
    {
        ManagementPresentation model;
        model.breadcrumb = "Park dashboard / Staff";
        model.title = "Staff";
        model.selected = selected;
        model.status = status;
        auto wage = [](StaffType type) {
            return getGameState().park.flags.has(ParkFlag::noMoney) ? std::string("Not applicable")
                                                                    : FormatMoney(GetStaffWage(type));
        };
        if (view == View::roster)
        {
            model.rows = { "Hire employee...", "Filter role...", "Counts / wages..." };
            model.rowDetails.resize(3);
            model.toolbarCount=3;model.denseRoster=true;model.listLabel="EMPLOYEES / ROLE / CURRENT ACTIVITY";
            const auto capacity=MeasureManagement({ContextGetWidth()-16,Layout::ContentHeight({ContextGetWidth(),ContextGetHeight()})},model).capacity;
            for (const auto& entry : roster)
            {
                const auto* staff = Resolve(entry);
                model.rows.push_back(staff ? staff->getName() : "Employee unavailable");
                const auto index=model.rows.size()-1;
                const auto first=3+(std::max<size_t>(selected,3)-3)/capacity*capacity;
                std::string detail;
                if(staff && index>=first && index<first+capacity) {
                    Formatter ft;formatPeepActionTo(*staff,ft);
                    detail=RoleName(entry.type)+" / "+FormatStringIDLegacy(STR_STRINGID,ft.Data());
                }
                model.rowDetails.push_back(detail);
            }

            model.rows.push_back("Back to dashboard");
            if (roster.empty())
                model.status = "No staff employed";
            return model;
        }
        if (view == View::summary)
        {
            model.title = "Staff counts / monthly wages";
            money64 total = 0;
            for (auto role : { StaffType::handyman, StaffType::mechanic, StaffType::security, StaffType::entertainer })
            {
                const auto count = Roster(role).size();
                total += count * GetStaffWage(role);
                model.stats.push_back({ RoleName(role), std::to_string(count) + " / " + wage(role) + " each" });
            }
            model.stats.push_back(
                { "Monthly total", getGameState().park.flags.has(ParkFlag::noMoney) ? "Not applicable" : FormatMoney(total) });
            model.rows = { "Back to roster" };
            return model;
        }
        if (view == View::roles)
        {
            model.title = "Choose staff role";
            for (auto role : { StaffType::handyman, StaffType::mechanic, StaffType::security, StaffType::entertainer })
                model.rows.push_back(RoleName(role));
            model.rows.push_back("Back to roster");
            return model;
        }
        if (view == View::hire)
        {
            model.title = "Hire " + RoleName(hireType);
            model.stats = { { "Monthly wage", wage(hireType) },
                            { "Upfront charge",
                              getGameState().park.flags.has(ParkFlag::noMoney) ? "Not applicable" : FormatMoney(0) },
                            { "Placement", "Automatic" } };
            model.rows = { "Hire", "Cancel" };
            model.confirmation = true;
            auto result = Hire(hireType, false);
            if (result.error != GameActions::Status::ok && model.status.empty())
                model.status = result.getErrorMessage().empty() ? result.getErrorTitle() : result.getErrorMessage();
            return model;
        }
        const auto* staff = target ? Resolve(*target) : nullptr;
        if (!staff)
        {
            model.status = "Employee unavailable";
            model.rows = { "Back to roster" };
            return model;
        }
        model.title = staff->getName();
        if (view == View::inspection)
        {
            Formatter ft;
            formatPeepActionTo(*staff, ft);
            model.stats = { { "Role", RoleName(target->type) },
                            { "Monthly wage", wage(target->type) },
                            { "Status", FormatStringIDLegacy(STR_STRINGID, ft.Data()) } };
            model.rows = { "Locate",
                           Orders(target->type).empty() ? "No configurable orders" : "Orders...",
                           "Statistics...",
                           "Dismiss...",
                           "Costume...",
                           "Back to roster",
                           "Patrol area...",
                           "Name...",
                           "Uniform colour...",
                           "Pick up..." };
        }
        else if (view == View::statistics)
        {
            model.breadcrumb = "Staff / Statistics";
            model.stats = { { "Employed months",
                              std::to_string(
                                  std::max(0, static_cast<int32_t>(GetDate().GetMonthsElapsed()) - staff->getHireDate())) } };
            if (target->type == StaffType::handyman)
                for (const auto& value :
                     std::vector<std::pair<std::string, uint32_t>>{ { "Lawns mown", staff->staffLawnsMown },
                                                                    { "Gardens watered", staff->staffGardensWatered },
                                                                    { "Litter swept", staff->staffLitterSwept },
                                                                    { "Bins emptied", staff->staffBinsEmptied } })
                    model.stats.push_back({ value.first, std::to_string(value.second) });
            else if (target->type == StaffType::mechanic)
                model.stats.insert(
                    model.stats.end(),
                    { { "Rides inspected", std::to_string(staff->staffRidesInspected) },
                      { "Rides fixed", std::to_string(staff->staffRidesFixed) } });
            if (target->type == StaffType::security)
                model.stats.push_back({ "Vandals stopped", std::to_string(staff->staffVandalsStopped) });
            if (target->type == StaffType::entertainer)
                model.stats.push_back({ "Guests entertained", std::to_string(staff->staffGuestsEntertained) });
            model.rows = { "Back to employee" };
        }
        else if (view == View::costumes)
        {
            model.breadcrumb = "Staff / Entertainer costume";
            if (target->type == StaffType::entertainer)
                for (const auto& entry : Costumes())
                    model.rows.push_back(entry.second + (entry.first == staff->animationObjectIndex ? " / current" : ""));
            model.rows.push_back("Back to employee");
            if (target->type != StaffType::entertainer)
                model.status = "Only entertainers have costumes";
        }
        else if (view == View::orders)
        {
            model.breadcrumb = "Staff / Orders draft";
            for (const auto& item : Orders(target->type))
                model.rows.push_back(item.second + ((orders.value_or(staff->staffOrders) & item.first) ? "  On" : "  Off"));
            model.rows.push_back("Apply orders");
            model.rows.push_back("Cancel");
        }
        else
        {
            model.breadcrumb = "Staff / Confirm dismissal";
            model.stats = { { "Monthly wage", wage(target->type) },
                            { "Refund",
                              getGameState().park.flags.has(ParkFlag::noMoney) ? "Not applicable" : FormatMoney(0) } };
            model.rows = { "Dismiss employee", "Cancel" };
            model.confirmation = true;
            model.destructive = true;
            model.bodyLines = {"Removes this employee and their patrol assignment."};
            const auto result = Dismiss(*target, false);
            if (result.error != GameActions::Status::ok && model.status.empty())
                model.status = result.getErrorMessage().empty() ? result.getErrorTitle() : result.getErrorMessage();
        }
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::StaffManagement


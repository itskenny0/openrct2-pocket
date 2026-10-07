// SPDX-License-Identifier: GPL-3.0-only
#include "Currency.h"
#include "Explorer.h"
#include "Finances.h"
#include "ListFocus.h"
#include "Panel.h"
#include "Paths.h"
#include "SaveSlots.h"
#include "ShopManagement.h"
#include "Shops.h"
#include "StaffManagement.h"
#include "TreePicker.h"
#include "Trees.h"

#include <filesystem>
#include <iomanip>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/park/ParkSetResearchFundingAction.h>
#include <openrct2/actions/peep/StaffHireNewAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/localisation/StringIds.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/WindowManager.h>
#include <sstream>

namespace OpenRCT2::Ui::Windows
{
    using namespace Drawing;
    namespace fs = std::filesystem;
    static constexpr ScreenSize kSize{ 244, 278 };
    static constexpr auto kWidgets = makeWidgets(makeWindowShim(STR_SHORTCUT_PAUSE, kSize));
    enum class Page
    {
        park,
        build,
        paths,
        pathOptions,
        treePicker,
        shopPicker,
        dashboard,
        overview,
        finances,
        financeEditor,
        financeConfirm,
        staff,
        staffInspect,
        staffStats,
        staffOrders,
        staffHire,
        staffDismiss,
        guests,
        research,
        ride,
        shop,
        shopPrices,
        shopDemolish,
        save,
        load,
        overwrite,
        discard,
        exit
    };

    struct ManagementSection
    {
        Page page;
        const char* label;
    };
    static constexpr std::array kManagementSections{ ManagementSection{ Page::overview, "Overview" },
                                                     ManagementSection{ Page::finances, "Finances" },
                                                     ManagementSection{ Page::staff, "Staff" },
                                                     ManagementSection{ Page::guests, "Guests" },
                                                     ManagementSection{ Page::research, "Research" } };
    Handheld::ManagementPresentation HandheldDashboardPresentation(size_t selected)
    {
        Handheld::ManagementPresentation model;
        model.breadcrumb = "Park management";
        model.title = getGameState().park.name;
        model.selected = selected;
        for (const auto& section : kManagementSections)
            model.rows.emplace_back(section.label);
        model.rows.emplace_back("Back to park");
        return model;
    }

    static fs::path SlotPath(size_t slot)
    {
        return fs::u8path(GetContext()->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::saves))
            / ("Pocket-" + std::to_string(slot + 1) + ".park");
    }

    class HandheldMenuWindow final : public Window
    {
        Handheld::ListFocus _focus;
        Page _page = Page::park;
        Handheld::ObjectPicker _picker;
        bool _pausedHere{};
        bool _shopPickerFromBuild{};
        bool _exitAfterSave{};
        size_t _slot{};
        size_t _dashboardFocus{};
        std::string _status;
        RideId _rideId = RideId::GetNull();
        std::optional<Handheld::ShopManagement::Target> _shop;
        std::optional<money64> _priceDraft;
        std::optional<Handheld::Finances::Draft> _financeDraft;
        std::vector<Handheld::StaffManagement::Target> _staffRoster;
        std::optional<Handheld::StaffManagement::Target> _staffTarget;
        StaffType _hireType = StaffType::handyman;
        uint8_t _ordersBaseline{}, _ordersDraft{};
        bool _staffBusy{};
        size_t _staffRosterFocus{};
        std::shared_ptr<int> _staffLifetime = std::make_shared<int>(0);
        uint64_t _staffRequest{};
        bool staffManagement() const
        {
            return _page == Page::staff || _page == Page::staffInspect || _page == Page::staffStats
                || _page == Page::staffOrders || _page == Page::staffHire || _page == Page::staffDismiss;
        }
        Handheld::ManagementPresentation staffPresentation() const
        {
            using Handheld::StaffManagement::View;
            const auto view = _page == Page::staff ? View::roster
                : _page == Page::staffInspect      ? View::inspection
                : _page == Page::staffStats        ? View::statistics
                : _page == Page::staffOrders       ? View::orders
                : _page == Page::staffHire         ? View::hire
                                                   : View::dismissal;
            return Handheld::StaffManagement::Presentation(
                view, _focus.index(), _staffRoster, _staffTarget, _hireType, _ordersDraft, _status);
        }
        bool financeManagement() const
        {
            return _page == Page::finances || _page == Page::financeEditor || _page == Page::financeConfirm;
        }
        bool managementShell() const
        {
            return _page == Page::dashboard || financeManagement() || staffManagement();
        }
        bool shopManagement() const
        {
            return _page == Page::shop || _page == Page::shopPrices || _page == Page::shopDemolish;
        }
        void pauseManagement()
        {
            if (!shopManagement())
                return;
            if (GameIsNotPaused())
            {
                PauseToggle();
                _pausedHere = true;
            }
            page(Page::park);
        }
        bool information() const
        {
            return _page == Page::overview || _page == Page::finances || _page == Page::guests || _page == Page::research
                || _page == Page::ride;
        }
        static std::string money(money64 value)
        {
            return Handheld::FormatMoney(value);
        }

        void layout()
        {
            const auto size = Handheld::OverlaySize(std::min<size_t>(8, rows().size()));
            // Confirmation text needs room; normal tools leave the centre cursor exposed.
            const ScreenSize target{ _page == Page::overwrite || _page == Page::discard || _page == Page::exit ? 330
                                                                                                               : size.width,
                                     size.height };
            const ScreenSize bounds = (_page == Page::pathOptions || picker() || shopManagement() || managementShell())
                ? ScreenSize{ ContextGetWidth(), ContextGetHeight() }
                : information() ? ScreenSize{ 330, _page == Page::ride ? 340 : 310 }
                                : target;
            WindowSetResize(*this, bounds, bounds);
            WindowSetPosition(
                *this,
                (_page == Page::pathOptions || picker() || shopManagement() || managementShell())
                    ? ScreenCoordsXY{ 0, 0 }
                    : ScreenCoordsXY{ 8, std::max(8, (ContextGetHeight() - bounds.height) / 2) });
        }

        void page(Page next)
        {
            _staffBusy = false;
            ++_staffRequest;
            if (next == Page::staff)
                _staffRoster = Handheld::StaffManagement::Roster();
            _page = next;
            if (next != Page::financeEditor && next != Page::financeConfirm)
                _financeDraft.reset();
            _focus = {};
            _status.clear();
            _priceDraft.reset();
            layout();
            if (next == Page::staff)
                _focus.move(static_cast<int32_t>(std::min(_staffRosterFocus, rows().size() - 1)), rows().size());
            if (next == Page::dashboard)
                _focus.move(static_cast<int32_t>(_dashboardFocus), rows().size());
            if (next == Page::overwrite || next == Page::discard || next == Page::exit || next == Page::shopDemolish
                || next == Page::financeConfirm || next == Page::staffHire || next == Page::staffDismiss)
                _focus.move(1, 2); // destructive confirmations default to Cancel
            invalidate();
        }
        std::vector<std::string> rows() const
        {
            switch (_page)
            {
                case Page::park:
                    return { "Resume park",   "Build",     "Park dashboard",     "Save park",
                             "Save and exit", "Load park", "Exit without saving" };
                case Page::dashboard:
                    return HandheldDashboardPresentation(0).rows;
                case Page::staff:
                case Page::staffInspect:
                case Page::staffStats:
                case Page::staffOrders:
                case Page::staffHire:
                case Page::staffDismiss:
                    return staffPresentation().rows;
                case Page::finances:
                    return Handheld::Finances::Presentation(Handheld::Finances::View::overview, 0).rows;
                case Page::financeEditor:
                    return { "Review change...", "Cancel" };
                case Page::financeConfirm:
                    return { "Apply", "Cancel" };
                case Page::overview:
                case Page::guests:
                    return { "Back to dashboard" };
                case Page::research:
                    return { "Cycle research funding", "Back to dashboard" };
                case Page::shop:
                    return { "Status", "Prices", "Demolish" };
                case Page::shopPrices:
                {
                    std::vector<std::string> result;
                    if (_shop)
                        for (const auto& price : Handheld::ShopManagement::Prices(*_shop))
                            result.push_back(price.name);
                    return result;
                }
                case Page::shopDemolish:
                    return { "Demolish", "Cancel" };
                case Page::ride:
                    return { "Open ride", "Close ride", "Back to park" };
                case Page::build:
                    return { "Raise land", "Lower land",         "Paths",       "Water", "Surface", "Remove scenery",
                             "Trees",      "Shops / Facilities", "Back to park" };
                case Page::treePicker:
                case Page::shopPicker:
                    return {};
                case Page::paths:
                    return { "Normal path", "Queue path", "Remove path", "Back to build" };
                case Page::pathOptions:
                    return { "Resume",       "Follow ground", "Raise height",  "Lower height",
                             "Flat / slope", "Rotate slope",  "Change surface" };
                case Page::save:
                case Page::load:
                {
                    std::vector<std::string> result;
                    for (size_t slot = 0; slot < 3; ++slot)
                    {
                        std::error_code error;
                        const bool exists = fs::exists(SlotPath(slot), error);
                        result.push_back("Pocket " + std::to_string(slot + 1) + (exists ? "  Saved park" : "  Empty"));
                    }
                    return result;
                }
                case Page::overwrite:
                    return { "Replace this saved park", "Cancel" };
                case Page::discard:
                    return { "Load and discard current changes", "Cancel" };
                case Page::exit:
                    return { "Exit WITHOUT saving", "Cancel" };
            }
            return {};
        }
        void save()
        {
            const auto path = SlotPath(_slot);
            const bool saved = Handheld::WriteSaveAtomically(path, [](const fs::path& temporary) {
                SaveFlags saveFlags{};
                saveFlags.set(SaveFlag::exportObjects, Config::Get().general.savePluginData);
                return ScenarioSave(getGameState(), temporary.u8string(), saveFlags) != 0;
            });
            if (!saved)
            {
                _status = "Save failed. Your previous save is kept.";
                invalidate();
                return;
            }
            gScenarioSavePath = path.u8string();
            gCurrentLoadedPath = path.u8string();
            gFirstTimeSaving = false;
            gIsAutosaveLoaded = false;
            if (_exitAfterSave)
            {
                close();
                ContextQuit();
                return;
            }
            page(Page::park);
            _status = "Saved to Pocket " + std::to_string(_slot + 1);
        }
        void load()
        {
            const auto path = SlotPath(_slot).u8string();
            Handheld::EndTerrain();
            close();
            GetContext()->LoadParkFromFile(path);
        }

    public:
        void onOpen() override
        {
            Handheld::Paths::ClearPreview();
            Handheld::Trees::ClearPreview();
            Handheld::Shops::ClearPreview();
            setWidgets(kWidgets);
            layout();
            colours[0] = ColourWithFlags{ Colour::darkBlue };
            colours[1] = ColourWithFlags{ Colour::darkBlue };
            _pausedHere = gLegacyScene == LegacyScene::playing && GameIsNotPaused();
            if (_pausedHere)
                PauseToggle();
        }
        void onClose() override
        {
            if (_pausedHere && gLegacyScene == LegacyScene::playing && GameIsPaused())
                PauseToggle();
        }
        void openBuild()
        {
            page(Page::build);
        }
        void openPathOptions()
        {
            page(Page::pathOptions);
        }
        void refreshPicker(std::optional<Handheld::PickerIdentity> preferred = {})
        {
            _picker.setEntries(
                _page == Page::shopPicker ? Handheld::Shops::Entries() : Handheld::TreePickerEntries(), preferred);
        }
        void openTreePicker()
        {
            page(Page::treePicker);
            refreshPicker(Handheld::PickerIdentity{ ObjectType::smallScenery, Handheld::TreeSelection() });
        }
        void openShopPicker(bool fromBuild = false)
        {
            Handheld::Shops::ClearPreview();
            _shopPickerFromBuild = fromBuild;
            page(Page::shopPicker);
            refreshPicker(Handheld::PickerIdentity{ ObjectType::ride, Handheld::ShopSelection() });
        }
        bool picker() const
        {
            return _page == Page::treePicker || _page == Page::shopPicker;
        }
        bool treePicker() const
        {
            return _page == Page::treePicker;
        }
        ObjectEntryIndex pickerFocus() const
        {
            return _picker.selected() ? _picker.selected()->identity.index : kObjectEntryIndexNull;
        }
        void pickerMove(int x, int y)
        {
            if (!picker() || (x == 0 && y == 0))
                return;
            refreshPicker();
            _picker.move(x, y);
            _status.clear();
            invalidate();
        }
        void pickerPause()
        {
            if (picker())
                page(Page::park); // Discard unconfirmed focus, keep the placement selection.
        }
        void onUpdate() override
        {
            if (_page == Page::staff)
            {
                const auto updated = Handheld::StaffManagement::Roster();
                if (updated != _staffRoster)
                {
                    std::optional<Handheld::StaffManagement::Target> selected;
                    if (_focus.index() >= 2 && _focus.index() - 2 < _staffRoster.size())
                        selected = _staffRoster[_focus.index() - 2];
                    size_t newFocus = std::min(_focus.index(), updated.size() + 2);
                    if (selected)
                    {
                        const auto found = std::find(updated.begin(), updated.end(), *selected);
                        if (found != updated.end())
                            newFocus = 2 + std::distance(updated.begin(), found);
                    }
                    _staffRoster = updated;
                    _focus = {};
                    _focus.move(static_cast<int32_t>(newFocus), rows().size());
                    invalidate();
                }
            }
            if (shopManagement() && _shop && !Handheld::ShopManagement::Find(*_shop))
            {
                _shop.reset();
                _priceDraft.reset();
                _status = "Shop no longer available";
                invalidate();
            }
            if ((treePicker() && !Handheld::IsTreeToolActive())
                || (_page == Page::shopPicker && !_shopPickerFromBuild && !Handheld::IsShopToolActive()))
                close();
        }
        void openRide(RideId id)
        {
            _rideId = id;
            _shop = Handheld::ShopManagement::Capture(id);
            const auto* ride = GetRide(id);
            const bool isShop = ride && ride->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility);
            if (isShop && _pausedHere && GameIsPaused())
            {
                // Keep a shop live; native demolition must obey actual pause rules.
                PauseToggle();
                _pausedHere = false;
            }
            page(isShop ? Page::shop : Page::ride);
        }
        void openLoad()
        {
            page(Page::load);
        }
        void staffRemoved(EntityId id)
        {
            if (_staffTarget && _staffTarget->id == id)
                _staffTarget.reset();
            std::erase_if(_staffRoster, [id](const auto& target) { return target.id == id; });
            if (staffManagement())
            {
                _status = "Employee removed";
                invalidate();
            }
        }
        bool staffIsOpen() const
        {
            return staffManagement();
        }
        void staffPause()
        {
            if (staffManagement())
                page(Page::park);
        }
        void staffHired(const GameActions::Result& result)
        {
            if (!_staffBusy || !staffManagement())
                return;
            _staffBusy = false;
            if (result.error != GameActions::Status::ok)
                _status = Handheld::Finances::Feedback(result);
            else
            {
                _staffTarget = Handheld::StaffManagement::Identify(
                    result.getData<GameActions::StaffHireNewActionResult>().StaffEntityId);
                page(_staffTarget ? Page::staffInspect : Page::staff);
            }
            invalidate();
        }
        bool financeIsOpen() const
        {
            return financeManagement();
        }
        void financeMove(int x, int y)
        {
            if (!financeManagement())
                return;
            if (y != 0)
                move(y);
            if (_page == Page::financeEditor && _financeDraft && x != 0)
            {
                Handheld::Finances::Adjust(*_financeDraft, x);
                _status.clear();
                invalidate();
            }
        }
        void financePause()
        {
            if (financeManagement())
                page(Page::park);
        }
        bool shopIsOpen() const
        {
            return shopManagement();
        }
        void shopMove(int x, int y)
        {
            if (!shopManagement())
                return;
            if (y != 0)
            {
                move(y);
                _priceDraft.reset();
            }
            if (_page == Page::shopPrices && _shop && x != 0)
            {
                const auto prices = Handheld::ShopManagement::Prices(*_shop);
                if (_focus.index() < prices.size())
                    _priceDraft = std::clamp(
                        _priceDraft.value_or(prices[_focus.index()].value) + x, kRideMinPrice, kRideMaxPrice);
                invalidate();
            }
        }
        void shopPause()
        {
            pauseManagement();
        }
        void move(int32_t direction)
        {
            _focus.move(direction, rows().size());
            _status.clear();
            invalidate();
        }
        void back()
        {
            switch (_page)
            {
                case Page::shopPicker:
                    if (_shopPickerFromBuild)
                        page(Page::build);
                    else
                        close();
                    break;
                case Page::shopPrices:
                case Page::shopDemolish:
                    page(Page::shop);
                    break;
                case Page::shop:
                case Page::ride:
                case Page::pathOptions:
                case Page::treePicker:
                    close();
                    break;
                case Page::staffHire:
                case Page::staffInspect:
                    page(Page::staff);
                    break;
                case Page::staffStats:
                case Page::staffOrders:
                case Page::staffDismiss:
                    page(Page::staffInspect);
                    break;
                case Page::staff:
                    page(Page::dashboard);
                    break;
                case Page::financeConfirm:
                    page(Page::financeEditor);
                    break;
                case Page::financeEditor:
                    page(Page::finances);
                    break;
                case Page::overview:
                case Page::finances:
                case Page::guests:
                case Page::research:
                    page(Page::dashboard);
                    break;
                case Page::paths:
                    page(Page::build);
                    break;
                case Page::overwrite:
                    page(Page::save);
                    break;
                case Page::discard:
                    page(Page::load);
                    break;
                case Page::exit:
                    page(Page::park);
                    break;
                case Page::park:
                    close();
                    break;
                default:
                    if (gLegacyScene == LegacyScene::playing)
                        page(Page::park);
                    else
                        close();
                    break;
            }
        }
        void confirm()
        {
            const auto choice = _focus.index();
            switch (_page)
            {
                case Page::shopPicker:
                    if (_picker.selected() && Handheld::SetShopSelection(_picker.selected()->identity.index))
                        close();
                    else
                    {
                        refreshPicker();
                        _status = "Shop unavailable";
                        invalidate();
                    }
                    break;
                case Page::treePicker:
                    if (_picker.selected() && Handheld::SetTreeSelection(_picker.selected()->identity.index))
                        close();
                    else
                    {
                        refreshPicker();
                        _status = "Tree unavailable";
                        invalidate();
                    }
                    break;
                case Page::park:
                    switch (choice)
                    {
                        case 0:
                            close();
                            break;
                        case 1:
                            page(Page::build);
                            break;
                        case 2:
                            page(Page::dashboard);
                            break;
                        case 3:
                            _exitAfterSave = false;
                            page(Page::save);
                            break;
                        case 4:
                            _exitAfterSave = true;
                            page(Page::save);
                            break;
                        case 5:
                            page(Page::load);
                            break;
                        case 6:
                            page(Page::exit);
                            break;
                    }
                    break;
                case Page::build:
                    if (choice == 7)
                        openShopPicker(true);
                    else if (choice == 2)
                        page(Page::paths);
                    else
                    {
                        close();
                        if (choice < 2)
                            Handheld::BeginTerrain(choice == 0);
                        else if (choice == 3)
                            Handheld::BeginWater();
                        else if (choice == 4)
                            Handheld::BeginSurface();
                        else if (choice == 5)
                            Handheld::BeginSceneryRemoval();
                        else if (choice == 6)
                            Handheld::BeginTrees();
                    }
                    break;
                case Page::paths:
                    if (choice == 3)
                        page(Page::build);
                    else
                    {
                        close();
                        Handheld::EndTerrain();
                        Handheld::Paths::Begin(choice == 1, choice == 2);
                    }
                    break;
                case Page::pathOptions:
                {
                    namespace Paths = Handheld::Paths;
                    bool changed = true;
                    switch (choice)
                    {
                        case 0:
                            close();
                            return;
                        case 1:
                            Paths::FollowGround();
                            break;
                        case 2:
                        case 3:
                            changed = Paths::ChangeHeight(choice == 2 ? 1 : -1);
                            break;
                        case 4:
                            changed = Paths::CycleSlope();
                            break;
                        case 5:
                            changed = Paths::RotateSlope();
                            break;
                        case 6:
                            Paths::CycleSurface();
                            break;
                    }
                    _status = changed ? Paths::PlacementName() : "No target or height limit reached.";
                    invalidate();
                    break;
                }
                case Page::dashboard:
                    _dashboardFocus = choice;
                    page(choice < kManagementSections.size() ? kManagementSections[choice].page : Page::park);
                    break;
                case Page::staff:
                    _staffRosterFocus = choice;
                    if (choice < 2)
                    {
                        _hireType = choice == 0 ? StaffType::handyman : StaffType::mechanic;
                        page(Page::staffHire);
                    }
                    else if (choice - 2 < _staffRoster.size())
                    {
                        _staffTarget = _staffRoster[choice - 2];
                        page(Page::staffInspect);
                    }
                    else
                        page(Page::dashboard);
                    break;
                case Page::staffHire:
                    if (choice == 1)
                        page(Page::staff);
                    else if (!_staffBusy)
                    {
                        _staffBusy = true;
                        auto result = Handheld::StaffManagement::Hire(
                            _hireType, true,
                            [this, lifetime = std::weak_ptr<int>(_staffLifetime),
                             request = _staffRequest](const GameActions::GameAction*, const GameActions::Result* response) {
                                if (lifetime.lock() && request == _staffRequest)
                                    staffHired(*response);
                            });
                        if (result.error != GameActions::Status::ok)
                        {
                            _staffBusy = false;
                            _status = Handheld::Finances::Feedback(result);
                        }
                        else
                            _status = "Hiring...";
                    }
                    break;
                case Page::staffInspect:
                    if (!_staffTarget || !Handheld::StaffManagement::Resolve(*_staffTarget))
                    {
                        page(Page::staff);
                        break;
                    }
                    if (choice == 0)
                    {
                        const auto target = *_staffTarget;
                        if (!Handheld::StaffManagement::Resolve(target)->getLocation().isNull() && WindowGetMain())
                        {
                            close();
                            Handheld::StaffManagement::Locate(target);
                            return;
                        }
                        _status = "Location unavailable";
                    }
                    else if (choice == 1)
                    {
                        _ordersBaseline = _ordersDraft = Handheld::StaffManagement::Resolve(*_staffTarget)->staffOrders;
                        page(Page::staffOrders);
                    }
                    else
                        page(choice == 2 ? Page::staffStats : choice == 3 ? Page::staffDismiss : Page::staff);
                    break;
                case Page::staffStats:
                    page(Page::staffInspect);
                    break;
                case Page::staffOrders:
                    if (!_staffTarget || !Handheld::StaffManagement::Resolve(*_staffTarget))
                    {
                        page(Page::staff);
                        break;
                    }
                    {
                        const auto orders = Handheld::StaffManagement::Orders(_staffTarget->type);
                        if (choice < orders.size())
                            _ordersDraft ^= orders[choice].first;
                        else if (choice == orders.size())
                        {
                            const auto result = Handheld::StaffManagement::SetOrders(
                                *_staffTarget, _ordersBaseline, _ordersDraft, true);
                            if (result.error == GameActions::Status::ok)
                                page(Page::staffInspect);
                            else
                                _status = Handheld::Finances::Feedback(result);
                        }
                        else
                            page(Page::staffInspect);
                    }
                    break;
                case Page::staffDismiss:
                    if (choice == 1)
                        page(Page::staffInspect);
                    else if (_staffTarget)
                    {
                        const auto result = Handheld::StaffManagement::Dismiss(*_staffTarget, true);
                        if (result.error == GameActions::Status::ok)
                            page(Page::staff);
                        else
                            _status = Handheld::Finances::Feedback(result);
                    }
                    break;
                case Page::finances:
                    if (choice == 2)
                        page(Page::dashboard);
                    else
                    {
                        const auto field = choice == 0 ? Handheld::Finances::Field::loan : Handheld::Finances::Field::admission;
                        if (Handheld::Finances::Available(field))
                        {
                            _financeDraft = Handheld::Finances::Begin(field);
                            page(Page::financeEditor);
                        }
                    }
                    break;
                case Page::financeEditor:
                    if (choice == 1)
                        page(Page::finances);
                    else if (_financeDraft)
                    {
                        const auto result = Handheld::Finances::Action(*_financeDraft, false);
                        if (result.error == GameActions::Status::ok)
                            page(Page::financeConfirm);
                        else
                        {
                            _status = Handheld::Finances::Feedback(result);
                            invalidate();
                        }
                    }
                    break;
                case Page::financeConfirm:
                    if (choice == 1)
                        page(Page::finances);
                    else if (_financeDraft)
                    {
                        const auto result = Handheld::Finances::Action(*_financeDraft, true);
                        if (result.error == GameActions::Status::ok)
                        {
                            page(Page::finances);
                            _status = "Financial change requested";
                        }
                        else
                            _status = Handheld::Finances::Feedback(result);
                        invalidate();
                    }
                    break;
                case Page::overview:
                case Page::guests:
                    page(Page::dashboard);
                    break;
                case Page::research:
                    if (choice == 1)
                        page(Page::dashboard);
                    else
                    {
                        const auto& state = getGameState();
                        auto action = GameActions::ParkSetResearchFundingAction(
                            state.researchPriorities, (state.researchFundingLevel + 1) % 4);
                        const auto result = GameActions::Execute(&action, getGameState());
                        _status = result.error == GameActions::Status::ok ? "Funding change requested."
                                                                          : result.getErrorMessage();
                        invalidate();
                    }
                    break;
                case Page::shop:
                    if (!_shop || !Handheld::ShopManagement::Find(*_shop))
                        _status = "Shop no longer available";
                    else if (choice == 1)
                    {
                        if (!getGameState().park.flags.has(ParkFlag::noMoney)
                            && !Handheld::ShopManagement::Prices(*_shop).empty())
                            page(Page::shopPrices);
                    }
                    else if (choice == 2)
                        page(Page::shopDemolish);
                    else
                    {
                        const auto result = Handheld::ShopManagement::SetStatus(
                            *_shop, Handheld::ShopManagement::Find(*_shop)->status != RideStatus::open);
                        _status = result.error == GameActions::Status::ok ? "Status change requested"
                                                                          : result.getErrorMessage();
                    }
                    invalidate();
                    break;
                case Page::shopPrices:
                    if (_shop)
                    {
                        const auto prices = Handheld::ShopManagement::Prices(*_shop);
                        if (choice < prices.size() && _priceDraft)
                        {
                            const auto result = Handheld::ShopManagement::SetPrice(
                                *_shop, prices[choice].primary, *_priceDraft);
                            _status = result.error == GameActions::Status::ok ? "Price change requested"
                                                                              : result.getErrorMessage();
                            _priceDraft.reset();
                        }
                    }
                    invalidate();
                    break;
                case Page::shopDemolish:
                    if (choice == 1)
                        page(Page::shop);
                    else if (_shop)
                    {
                        const auto result = Handheld::ShopManagement::Demolish(*_shop, true);
                        if (result.error == GameActions::Status::ok)
                            close();
                        else
                        {
                            _status = result.getErrorMessage();
                            invalidate();
                        }
                    }
                    break;
                case Page::ride:
                    if (choice == 2)
                        close();
                    else
                    {
                        auto action = GameActions::RideSetStatusAction(
                            _rideId, choice == 0 ? RideStatus::open : RideStatus::closed);
                        const auto result = GameActions::Execute(&action, getGameState());
                        _status = result.error == GameActions::Status::ok ? "Ride status change requested."
                                                                          : result.getErrorMessage();
                        invalidate();
                    }
                    break;
                case Page::save:
                {
                    _slot = choice;
                    std::error_code error;
                    if (fs::exists(SlotPath(_slot), error))
                        page(Page::overwrite);
                    else
                        save();
                    break;
                }
                case Page::load:
                {
                    _slot = choice;
                    std::error_code error;
                    if (!fs::exists(SlotPath(_slot), error))
                    {
                        _status = "This slot is empty.";
                        invalidate();
                    }
                    else if (gLegacyScene == LegacyScene::playing)
                        page(Page::discard);
                    else
                        load();
                    break;
                }
                case Page::overwrite:
                    if (choice == 0)
                        save();
                    else
                        page(Page::save);
                    break;
                case Page::discard:
                    if (choice == 0)
                        load();
                    else
                        page(Page::load);
                    break;
                case Page::exit:
                    if (choice == 0)
                    {
                        close();
                        ContextQuit();
                    }
                    else
                        page(Page::park);
                    break;
            }
        }
        void onMouseUp(WidgetIndex widget) override
        {
            if (widget == 2)
                back();
        }
        void onDraw(RenderTarget& rt) override
        {
            if (managementShell())
            {
                Handheld::ManagementPresentation model;
                if (_page == Page::dashboard)
                {
                    model = HandheldDashboardPresentation(_focus.index());
                    model.status = _status;
                }
                else if (staffManagement())
                    model = staffPresentation();
                else
                {
                    using Handheld::Finances::View;
                    model = Handheld::Finances::Presentation(
                        _page == Page::finances            ? View::overview
                            : _page == Page::financeEditor ? View::editor
                                                           : View::confirmation,
                        _focus.index(), _financeDraft, _status);
                }
                Handheld::DrawManagementPanel(rt, { ContextGetWidth(), ContextGetHeight() }, model);
                return;
            }
            if (shopManagement())
            {
                using Handheld::ShopManagement::View;
                Handheld::ShopManagement::Draw(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _shop,
                    _page == Page::shop             ? View::inspection
                        : _page == Page::shopPrices ? View::prices
                                                    : View::demolition,
                    _focus.index(), _priceDraft, _status);
                return;
            }
            if (_page == Page::shopPicker)
            {
                Handheld::DrawObjectPicker(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Shops / Facilities", _status,
                    "No shops available");
                return;
            }
            if (_page == Page::treePicker)
            {
                Handheld::DrawObjectPicker(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Choose tree", _status, "No trees available");
                return;
            }
            if (_page == Page::pathOptions)
            {
                Handheld::DrawPathOptions(
                    rt, { ContextGetWidth(), ContextGetHeight() }, rows(), _focus.index(),
                    _status.empty() ? Handheld::Paths::PlacementName() : _status);
                return;
            }
            const char* title = "PARK / PAUSE";
            switch (_page)
            {
                case Page::dashboard:
                    title = "PARK DASHBOARD";
                    break;
                case Page::overview:
                    title = "PARK OVERVIEW";
                    break;
                case Page::finances:
                    title = "FINANCES";
                    break;
                case Page::guests:
                    title = "GUESTS";
                    break;
                case Page::research:
                    title = "RESEARCH";
                    break;
                case Page::ride:
                    title = "RIDE INSPECTION";
                    break;
                case Page::paths:
                    title = "PATHS";
                    break;
                case Page::pathOptions:
                    title = "PATH OPTIONS";
                    break;
                case Page::build:
                    title = "BUILD";
                    break;
                case Page::save:
                    title = _exitAfterSave ? "SAVE AND EXIT" : "SAVE PARK";
                    break;
                case Page::load:
                    title = "LOAD PARK";
                    break;
                case Page::overwrite:
                    title = "REPLACE SAVE?";
                    break;
                case Page::discard:
                    title = "DISCARD UNSAVED CHANGES?";
                    break;
                case Page::exit:
                    title = "EXIT WITHOUT SAVING?";
                    break;
                default:
                    break;
            }
            Handheld::MenuPresentation model;
            model.title = title;
            model.rows = rows();
            model.selected = _focus.index();
            model.status = _status;
            if (_page == Page::pathOptions)
            {
                model.icons = { Handheld::PanelIcon::play,  Handheld::PanelIcon::terrain, Handheld::PanelIcon::raise,
                                Handheld::PanelIcon::lower, Handheld::PanelIcon::path,    Handheld::PanelIcon::path,
                                Handheld::PanelIcon::path };
                if (model.status.empty())
                    model.status = Handheld::Paths::PlacementName();
            }
            if (_page == Page::park)
                model.icons = { Handheld::PanelIcon::play, Handheld::PanelIcon::terrain, Handheld::PanelIcon::folder,
                                Handheld::PanelIcon::save, Handheld::PanelIcon::exit,    Handheld::PanelIcon::folder,
                                Handheld::PanelIcon::exit };
            else if (_page == Page::build)
            {
                model.icons = { Handheld::PanelIcon::raise,   Handheld::PanelIcon::lower,   Handheld::PanelIcon::path,
                                Handheld::PanelIcon::terrain, Handheld::PanelIcon::terrain, Handheld::PanelIcon::remove,
                                Handheld::PanelIcon::tree,    Handheld::PanelIcon::folder,  Handheld::PanelIcon::back };
                if (model.status.empty())
                    model.status = "Choose a tool, then edit in the park.";
            }
            // Never replace a save failure with the confirmation's normal hint.
            if (model.status.empty())
            {
                if (_page == Page::discard || _page == Page::exit)
                    model.status = "Unsaved changes will be lost.";
                else if (_page == Page::overwrite)
                    model.status = "This replaces the selected saved park.";
            }
            if (information())
            {
                const auto& state = getGameState();
                const auto& park = state.park;
                model.detailTitle = park.name;
                switch (_page)
                {
                    case Page::overview:
                        model.stats = { { "Guests", std::to_string(park.numGuestsInPark) },
                                        { "Rating", std::to_string(park.rating) },
                                        { "Park value", money(park.value) },
                                        { "Company value", money(park.companyValue) },
                                        { "Cash", money(park.cash) } };
                        break;
                    case Page::finances:
                        model.stats = { { "Cash", money(park.cash) },
                                        { "Bank loan", money(park.bankLoan) },
                                        { "Current profit", money(park.currentProfit) },
                                        { "Expenditure", money(park.currentExpenditure) },
                                        { "Park value", money(park.value) } };
                        break;
                    case Page::guests:
                        model.stats = { { "In park", std::to_string(park.numGuestsInPark) },
                                        { "Arriving", std::to_string(park.numGuestsHeadingForPark) },
                                        { "Total admissions", std::to_string(park.totalAdmissions) },
                                        { "Park rating", std::to_string(park.rating) } };
                        break;
                    case Page::research:
                    {
                        static constexpr const char* funding[] = { "None", "Minimum", "Normal", "Maximum" };
                        model.stats = { { "Funding", funding[std::min<int>(state.researchFundingLevel, 3)] },
                                        { "Stage progress",
                                          std::to_string(static_cast<uint32_t>(state.researchProgress) * 100 / 65536) + "%" },
                                        { "Stage",
                                          state.researchProgressStage == 0       ? "Initial research"
                                              : state.researchProgressStage == 1 ? "Designing"
                                              : state.researchProgressStage == 2 ? "Completing design"
                                                                                 : "Complete" } };
                        break;
                    }
                    case Page::ride:
                        if (const auto* ride = GetRide(_rideId))
                        {
                            model.detailTitle = ride->getName();
                            model.stats = { { "Status",
                                              ride->status == RideStatus::open         ? "Open"
                                                  : ride->status == RideStatus::closed ? "Closed"
                                                                                       : "Testing" },
                                            { "Riders", std::to_string(ride->numRiders) },
                                            { "Reliability", std::to_string(ride->reliabilityPercentage) + "%" },
                                            { "Profit", money(ride->profit) } };
                        }
                        else
                            model.status = "This ride is no longer available.";
                        break;
                    default:
                        break;
                }
                Handheld::DrawInformationOverlay(rt, *this, model);
            }
            else
                Handheld::DrawOverlayShell(rt, *this, model);
        }
    };

    static HandheldMenuWindow* OpenPanel()
    {
        auto* wm = GetWindowManager();
        auto* window = wm->BringToFrontByClass(WindowClass::handheldMenu);
        if (window == nullptr)
            window = wm->Create<HandheldMenuWindow>(
                WindowClass::handheldMenu, {}, kSize, { WindowFlag::transparent, WindowFlag::noBackground });
        return static_cast<HandheldMenuWindow*>(window);
    }
    void HandheldMenuOpen()
    {
        OpenPanel();
    }
    void HandheldBuildOpen()
    {
        if (auto* window = OpenPanel())
            window->openBuild();
    }
    void HandheldPathOptionsOpen()
    {
        if (auto* window = OpenPanel())
            window->openPathOptions();
    }
    void HandheldLoadOpen()
    {
        if (auto* window = OpenPanel())
            window->openLoad();
    }
    void HandheldMenuMove(int32_t direction)
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->move(direction);
    }
    void HandheldMenuConfirm()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->confirm();
    }
    void HandheldMenuBack()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->back();
    }
    void HandheldRideOpen(RideId ride)
    {
        if (auto* window = OpenPanel())
            window->openRide(ride);
    }
} // namespace OpenRCT2::Ui::Windows

namespace OpenRCT2::Ui::Windows
{
    bool HandheldShopManagementIsOpen()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->shopIsOpen();
    }
    void HandheldShopManagementMove(int x, int y)
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->shopMove(x, y);
    }
    void HandheldShopManagementPause()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->shopPause();
    }
    void HandheldShopPickerOpen()
    {
        if (auto* window = OpenPanel())
            window->openShopPicker(!Handheld::IsShopToolActive());
    }
    bool HandheldObjectPickerIsOpen()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->picker();
    }
    void HandheldObjectPickerMove(int x, int y)
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->pickerMove(x, y);
    }
    void HandheldObjectPickerPause()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->pickerPause();
    }
    ObjectEntryIndex HandheldObjectPickerFocus()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->picker()
            ? static_cast<const HandheldMenuWindow*>(window)->pickerFocus()
            : kObjectEntryIndexNull;
    }
    void HandheldTreePickerOpen()
    {
        if (!Handheld::IsTreeToolActive())
            return;
        if (auto* window = OpenPanel())
            window->openTreePicker();
    }
    bool HandheldTreePickerIsOpen()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->treePicker();
    }
    void HandheldTreePickerMove(int x, int y)
    {
        HandheldObjectPickerMove(x, y);
    }
    void HandheldTreePickerPause()
    {
        HandheldObjectPickerPause();
    }
    ObjectEntryIndex HandheldTreePickerFocus()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->treePicker()
            ? static_cast<const HandheldMenuWindow*>(window)->pickerFocus()
            : kObjectEntryIndexNull;
    }
} // namespace OpenRCT2::Ui::Windows

namespace OpenRCT2::Ui::Windows
{
    bool HandheldFinancesIsOpen()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->financeIsOpen();
    }
    void HandheldFinancesMove(int x, int y)
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->financeMove(x, y);
    }
    void HandheldFinancesPause()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->financePause();
    }
} // namespace OpenRCT2::Ui::Windows

namespace OpenRCT2::Ui::Windows
{
    void HandheldStaffRemoved(EntityId id)
    {
        Handheld::StaffManagement::Invalidate(id);
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->staffRemoved(id);
    }
    bool HandheldStaffIsOpen()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->staffIsOpen();
    }
    void HandheldStaffPause()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->staffPause();
    }
} // namespace OpenRCT2::Ui::Windows

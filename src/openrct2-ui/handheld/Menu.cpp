// SPDX-License-Identifier: GPL-3.0-only
#include "AreaTools.h"
#include "Currency.h"
#include "CustomTracks.h"
#include "Dashboard.h"
#include "Designs.h"
#include "Explorer.h"
#include "Finances.h"
#include "FlatRides.h"
#include "GuestManagement.h"
#include "ListFocus.h"
#include "MarketingManagement.h"
#include "Panel.h"
#include "ParkOverview.h"
#include "Paths.h"
#include "ResearchManagement.h"
#include "RideAccess.h"
#include "RideManagement.h"
#include "RightsTools.h"
#include "SaveSlots.h"
#include "SceneryTools.h"
#include "ShopManagement.h"
#include "Shops.h"
#include "StaffManagement.h"
#include "TextEntry.h"
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
#include <openrct2/ride/TrackDesignRepository.h>
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
        sceneryCategory,
        sceneryPicker,
        sceneryOptions,
        areaOptions,
        rightsOptions,
        rightsConfirm,
        sceneryName,
        sceneryConfirm,
        shopPicker,
        ridePicker,
        designPicker,
        customChoice,
        trackPiecePicker,
        designConfirm,
        dashboard,
        overview,
        overviewConfirm,
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
        guestRoster,
        guestInspect,
        guestNeeds,
        guestThoughts,
        guestSpending,
        guestInventory,
        guestText,
        ridesRoster,
        ridesInspect,
        ridesAccess,
        ridesStats,
        ridesCustomers,
        ridesStations,
        ridesEarnings,
        ridesGraphs,
        ridesOperations,
        ridesMaintenance,
        ridesVehicles,
        ridesDispatch,
        ridesMusic,
        ridesAppearance,
        ridesName,
        ridesPrice,
        ridesConfirm,
        research,
        researchEditor,
        researchConfirm,
        marketing,
        marketingActive,
        marketingTypes,
        marketingEditor,
        marketingTargets,
        marketingConfirm,
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

    Handheld::ManagementPresentation HandheldDashboardPresentation(size_t selected)
    {
        Handheld::ManagementPresentation model;
        model.breadcrumb = "Park management";
        model.title = getGameState().park.name;
        model.selected = selected;
        for (const auto& section : Handheld::Dashboard::kCategories)
            model.rows.emplace_back(section.name);
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
        bool _ridePickerFromBuild{};
        bool _designPickerFromBuild{};
        ObjectEntryIndex _customObject = kObjectEntryIndexNull;
        bool _exitAfterSave{};
        size_t _slot{};
        Handheld::Dashboard::State _dashboard;
        struct SectionResume
        {
            std::optional<Page> page;
            size_t row{};
        };
        std::array<SectionResume, Handheld::Dashboard::kCategories.size()> _sectionResume;
        std::string _status;
        RideId _rideId = RideId::GetNull();
        std::optional<Handheld::ShopManagement::Target> _shop;
        std::optional<money64> _priceDraft;
        std::optional<Handheld::Finances::Draft> _financeDraft;
        std::optional<Handheld::ParkOverview::Draft> _overviewDraft;
        bool _overviewBusy{};
        uint64_t _overviewRequest{};
        std::shared_ptr<int> _overviewLifetime = std::make_shared<int>(0);
        bool overviewManagement() const
        {
            return _page == Page::overview || _page == Page::overviewConfirm;
        }
        Handheld::ManagementPresentation overviewPresentation() const
        {
            return Handheld::ParkOverview::Presentation(_focus.index(), _overviewDraft, _status);
        }
        std::optional<Handheld::ResearchManagement::Draft> _researchDraft;
        std::optional<Handheld::MarketingManagement::Draft> _marketingDraft;
        std::vector<int> _marketingTypes, _marketingActiveTypes;
        std::vector<Handheld::MarketingManagement::Target> _marketingTargets;
        int _marketingActiveType = -1;
        bool _marketingBusy{};
        std::shared_ptr<int> _marketingLifetime = std::make_shared<int>(0);
        std::vector<Handheld::GuestManagement::Target> _guestRoster;
        std::optional<Handheld::GuestManagement::Target> _guestTarget;
        size_t _guestRosterFocus{};
        Page _guestTextParent = Page::guestThoughts;
        std::string _guestText;
        size_t _guestTextPage{};
        std::vector<Handheld::RideManagement::Target> _ridesRoster;
        std::optional<Handheld::RideManagement::Target> _ridesTarget;
        std::optional<Handheld::RideManagement::Draft> _ridesDraft;
        size_t _ridesRosterFocus{};
        bool _ridesBusy{};
        Handheld::TextEntry _nameEntry;
        std::optional<Handheld::SceneryTools::Target> _signTarget;
        std::optional<Handheld::RightsTools::Draft> _rightsDraft;
        bool _rightsReviewResumed{};
        size_t _rideGraphMetric{}, _rideGraphOffset{};
        bool _ridesReviewResumed{};
        uint64_t _ridesRequest{};
        std::shared_ptr<int> _ridesLifetime = std::make_shared<int>(0);
        Page ridesDraftParent() const
        {
            if (_ridesDraft && !_ridesDraft->randomColours.empty())
                return Page::ridesAppearance;
            if (_ridesDraft && _ridesDraft->renaming)
                return Page::ridesName;
            if (_ridesDraft && _ridesDraft->setting
                && _ridesDraft->setting->section == Handheld::RideManagement::View::appearance)
                return Page::ridesAppearance;
            if (_ridesDraft && _ridesDraft->refurbishment)
                return Page::ridesMaintenance;
            if (_ridesDraft && _ridesDraft->setting
                && _ridesDraft->setting->section == Handheld::RideManagement::View::vehicles)
                return Page::ridesVehicles;
            if (_ridesDraft && _ridesDraft->setting
                && _ridesDraft->setting->section == Handheld::RideManagement::View::dispatch)
                return Page::ridesDispatch;
            if (_ridesDraft && _ridesDraft->setting && _ridesDraft->setting->section == Handheld::RideManagement::View::music)
                return Page::ridesMusic;
            if (_ridesDraft && _ridesDraft->setting)
                return _ridesDraft->setting->key == GameActions::RideSetSetting::inspectionInterval ? Page::ridesMaintenance
                                                                                                    : Page::ridesOperations;
            return _ridesTarget && Handheld::RideManagement::Resolve(*_ridesTarget) ? Page::ridesInspect : Page::ridesRoster;
        }
        bool ridesManagement() const
        {
            return _page == Page::ridesRoster || _page == Page::ridesInspect || _page == Page::ridesGraphs
                || _page == Page::ridesStations || _page == Page::ridesCustomers || _page == Page::ridesEarnings
                || _page == Page::ridesStats || _page == Page::ridesVehicles || _page == Page::ridesName
                || _page == Page::ridesAppearance || _page == Page::ridesMusic || _page == Page::ridesDispatch
                || _page == Page::ridesOperations || _page == Page::ridesMaintenance || _page == Page::ridesAccess
                || _page == Page::ridesPrice || _page == Page::ridesConfirm;
        }
        Handheld::ManagementPresentation ridesPresentationFor(Page current, size_t selected) const
        {
            using Handheld::RideManagement::View;
            if (current == Page::ridesName)
            {
                Handheld::ManagementPresentation model;
                model.breadcrumb = "Name / Keyboard";
                model.title = _nameEntry.text.empty() ? "Default name" : _nameEntry.text;
                model.rows = _nameEntry.Keys();
                model.columns = Handheld::TextEntry::columns;
                model.selected = selected;
                model.status = std::to_string(_nameEntry.Length()) + " / 32 characters";
                if (!_status.empty())
                    model.status = _status;
                model.primaryAction = "Type";
                return model;
            }
            if (current == Page::ridesAccess && _ridesTarget)
                return Handheld::RideAccess::Presentation(*_ridesTarget, selected, _status);
            return Handheld::RideManagement::Presentation(
                current == Page::ridesRoster            ? View::roster
                    : current == Page::ridesInspect     ? View::inspection
                    : current == Page::ridesGraphs      ? View::graphs
                    : current == Page::ridesStations    ? View::stations
                    : current == Page::ridesCustomers   ? View::customers
                    : current == Page::ridesEarnings    ? View::earnings
                    : current == Page::ridesStats       ? View::statistics
                    : current == Page::ridesOperations  ? View::operations
                    : current == Page::ridesMaintenance ? View::maintenance
                    : current == Page::ridesVehicles    ? View::vehicles
                    : current == Page::ridesAppearance  ? View::appearance
                    : current == Page::ridesMusic       ? View::music
                    : current == Page::ridesDispatch    ? View::dispatch
                    : current == Page::ridesPrice       ? View::price
                                                        : View::confirmation,
                selected, _ridesRoster, _ridesTarget, _ridesDraft, _status, _rideGraphMetric, _rideGraphOffset);
        }
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
        Handheld::ManagementPresentation staffPresentationFor(Page currentPage, size_t selected) const
        {
            using Handheld::StaffManagement::View;
            const auto view = currentPage == Page::staff ? View::roster
                : currentPage == Page::staffInspect      ? View::inspection
                : currentPage == Page::staffStats        ? View::statistics
                : currentPage == Page::staffOrders       ? View::orders
                : currentPage == Page::staffHire         ? View::hire
                                                         : View::dismissal;
            return Handheld::StaffManagement::Presentation(
                view, selected, _staffRoster, _staffTarget, _hireType, _ordersDraft, _status);
        }
        Handheld::ManagementPresentation staffPresentation() const
        {
            return staffPresentationFor(_page, _focus.index());
        }
        bool financeManagement() const
        {
            return _page == Page::finances || _page == Page::financeEditor || _page == Page::financeConfirm;
        }
        bool researchManagement() const
        {
            return _page == Page::research || _page == Page::researchEditor || _page == Page::researchConfirm;
        }
        Handheld::ManagementPresentation researchPresentation() const
        {
            using Handheld::ResearchManagement::View;
            return Handheld::ResearchManagement::Presentation(
                _page == Page::research             ? View::overview
                    : _page == Page::researchEditor ? View::editor
                                                    : View::confirmation,
                _focus.index(), _researchDraft, _status);
        }
        bool marketingManagement() const
        {
            return _page == Page::marketing || _page == Page::marketingActive || _page == Page::marketingTypes
                || _page == Page::marketingEditor || _page == Page::marketingTargets || _page == Page::marketingConfirm;
        }
        Handheld::ManagementPresentation marketingPresentationFor(Page current, size_t selected) const
        {
            using Handheld::MarketingManagement::View;
            const auto view = current == Page::marketing ? View::overview
                : current == Page::marketingActive       ? View::active
                : current == Page::marketingTypes        ? View::types
                : current == Page::marketingTargets      ? View::targets
                : current == Page::marketingEditor       ? View::editor
                                                         : View::confirmation;
            return Handheld::MarketingManagement::Presentation(
                view, selected, _marketingDraft, _marketingActiveType,
                view == View::overview ? _marketingActiveTypes : _marketingTypes, _marketingTargets, _status);
        }
        bool guestManagement() const
        {
            return _page == Page::guests || _page == Page::guestRoster || _page == Page::guestInspect
                || _page == Page::guestNeeds || _page == Page::guestThoughts || _page == Page::guestSpending
                || _page == Page::guestInventory || _page == Page::guestText;
        }
        Handheld::ManagementPresentation guestPresentationFor(Page current, size_t selected) const
        {
            using Handheld::GuestManagement::View;
            const auto view = current == Page::guests ? View::overview
                : current == Page::guestRoster        ? View::roster
                : current == Page::guestInspect       ? View::inspection
                : current == Page::guestNeeds         ? View::needs
                : current == Page::guestThoughts      ? View::thoughts
                : current == Page::guestSpending      ? View::spending
                : current == Page::guestInventory     ? View::inventory
                                                      : View::text;
            return Handheld::GuestManagement::Presentation(
                view, selected, _guestRoster, _guestTarget, _status, _guestText, _guestTextPage);
        }
        bool managementShell() const
        {
            return _page == Page::dashboard || financeManagement() || staffManagement() || researchManagement()
                || marketingManagement() || guestManagement() || ridesManagement() || overviewManagement();
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

    public:
        bool constructionMenu() const
        {
            return _page == Page::rightsOptions || _page == Page::rightsConfirm || _page == Page::areaOptions
                || _page == Page::sceneryOptions || _page == Page::sceneryName || _page == Page::sceneryConfirm;
        }
        void constructionMove(int x, int y)
        {
            if (_page == Page::sceneryName)
            {
                const auto next = _nameEntry.Move(_focus.index(), x, y);
                _focus.move(static_cast<int>(next) - static_cast<int>(_focus.index()), rows().size());
            }
            else if (y)
                move(y);
            else if (x && _page == Page::rightsOptions)
                Handheld::RightsTools::Configure(_focus.index(), x);
            else if (x && _page == Page::areaOptions)
                Handheld::AreaTools::Configure(_focus.index(), x);
            else if (x && _page == Page::sceneryOptions)
                Handheld::SceneryTools::Configure(_focus.index(), x);
            invalidate();
        }

    private:
        void layout()
        {
            const auto size = Handheld::OverlaySize(std::min<size_t>(8, rows().size()));
            // Confirmation text needs room; normal tools leave the centre cursor exposed.
            const ScreenSize target{ _page == Page::overwrite || _page == Page::discard || _page == Page::exit ? 330
                                                                                                               : size.width,
                                     size.height };
            const ScreenSize bounds = (_page == Page::rightsConfirm || _page == Page::sceneryName
                                       || _page == Page::sceneryConfirm || _page == Page::pathOptions || picker()
                                       || shopManagement() || managementShell())
                ? ScreenSize{ ContextGetWidth(), ContextGetHeight() }
                : information() ? ScreenSize{ 330, _page == Page::ride ? 340 : 310 }
                                : target;
            WindowSetResize(*this, bounds, bounds);
            WindowSetPosition(
                *this,
                (_page == Page::rightsConfirm || _page == Page::sceneryName || _page == Page::sceneryConfirm
                 || _page == Page::pathOptions || picker() || shopManagement() || managementShell())
                    ? ScreenCoordsXY{ 0, 0 }
                    : ScreenCoordsXY{ 8, std::max(8, (ContextGetHeight() - bounds.height) / 2) });
        }

        void page(Page next)
        {
            if (_rightsReviewResumed && next != Page::rightsConfirm)
            {
                if (_pausedHere && !(gGamePaused & GAME_PAUSED_NORMAL))
                    PauseToggle();
                _rightsReviewResumed = false;
            }

            if (_ridesReviewResumed && next != Page::ridesConfirm)
            {
                // Return only our Dashboard pause; never clear native/external pause reasons.
                if (_pausedHere && !(gGamePaused & GAME_PAUSED_NORMAL))
                    PauseToggle();
                _ridesReviewResumed = false;
            }
            if (next != Page::designConfirm)
                Handheld::Designs::CancelReview();
            if (next != Page::overviewConfirm)
                _overviewDraft.reset();
            _overviewBusy = false;
            ++_overviewRequest;
            if (_page == Page::ridesRoster)
                _ridesRosterFocus = _focus.index();
            if (next == Page::ridesRoster)
                _ridesRoster = Handheld::RideManagement::Roster();
            if (next != Page::ridesPrice && next != Page::ridesConfirm && next != Page::ridesName)
                _ridesDraft.reset();
            _ridesBusy = false;
            ++_ridesRequest;
            if (_page == Page::guestRoster)
                _guestRosterFocus = _focus.index();
            if (next == Page::dashboard && _page != Page::dashboard && managementShell() && dashboardCanSwitch())
                _sectionResume[_dashboard.index()] = { _page, _focus.index() };
            _staffBusy = false;
            ++_staffRequest;
            if (next == Page::guestRoster)
                _guestRoster = Handheld::GuestManagement::Roster();
            if (next == Page::staff)
                _staffRoster = Handheld::StaffManagement::Roster();
            if (next == Page::marketing)
                _marketingActiveTypes = Handheld::MarketingManagement::ActiveTypes();
            if (next == Page::marketingTypes)
                _marketingTypes = Handheld::MarketingManagement::Types();
            if (next == Page::marketingTargets && _marketingDraft)
                _marketingTargets = Handheld::MarketingManagement::Targets(_marketingDraft->type);
            if (next != Page::marketingEditor && next != Page::marketingTargets && next != Page::marketingConfirm)
                _marketingDraft.reset();
            _page = next;
            if (next == Page::ridesName)
            {
                _dashboard.focus = Handheld::Dashboard::Focus::content;
                _dashboard.section = Handheld::Dashboard::Section::rides;
            }
            if (next == Page::dashboard)
                _dashboard.focus = Handheld::Dashboard::Focus::rail;
            else if (next == Page::staff)
                _dashboard.section = Handheld::Dashboard::Section::staff;
            else if (next == Page::finances)
                _dashboard.section = Handheld::Dashboard::Section::finances;
            if (next == Page::overview)
                _dashboard.section = Handheld::Dashboard::Section::overview;
            if (next == Page::ridesRoster)
                _dashboard.section = Handheld::Dashboard::Section::rides;
            if (next == Page::guests)
                _dashboard.section = Handheld::Dashboard::Section::guests;
            if (next == Page::marketing)
                _dashboard.section = Handheld::Dashboard::Section::marketing;
            if (next == Page::research)
                _dashboard.section = Handheld::Dashboard::Section::research;
            if (next != Page::researchEditor && next != Page::researchConfirm)
                _researchDraft.reset();
            if (next != Page::financeEditor && next != Page::financeConfirm)
                _financeDraft.reset();
            _focus = {};
            _status.clear();
            _priceDraft.reset();
            layout();
            if (next == Page::staff)
                _focus.move(static_cast<int32_t>(std::min(_staffRosterFocus, rows().size() - 1)), rows().size());

            if (next == Page::ridesRoster)
                _focus.move(static_cast<int32_t>(std::min(_ridesRosterFocus, rows().size() - 1)), rows().size());
            if (next == Page::guestRoster)
                _focus.move(static_cast<int32_t>(std::min(_guestRosterFocus, rows().size() - 1)), rows().size());
            if (next == Page::overwrite || next == Page::discard || next == Page::exit || next == Page::shopDemolish
                || next == Page::overviewConfirm || next == Page::ridesConfirm || next == Page::financeConfirm
                || next == Page::marketingConfirm || next == Page::staffHire || next == Page::staffDismiss)
                _focus.move(1, 2); // destructive confirmations default to Cancel
            if (next == Page::researchConfirm)
                _focus.move(static_cast<int32_t>(rows().size() - 1), rows().size());
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
                case Page::overviewConfirm:
                    return overviewPresentation().rows;
                case Page::ridesRoster:
                case Page::ridesInspect:
                case Page::ridesAccess:
                case Page::ridesVehicles:
                case Page::ridesName:
                case Page::ridesAppearance:
                case Page::ridesMusic:
                case Page::ridesDispatch:
                case Page::ridesOperations:
                case Page::ridesMaintenance:
                case Page::ridesGraphs:
                case Page::ridesStations:
                case Page::ridesCustomers:
                case Page::ridesEarnings:
                case Page::ridesStats:
                case Page::ridesPrice:
                case Page::ridesConfirm:
                    return ridesPresentationFor(_page, _focus.index()).rows;
                case Page::guests:
                case Page::guestRoster:
                case Page::guestInspect:
                case Page::guestNeeds:
                case Page::guestThoughts:
                case Page::guestSpending:
                case Page::guestInventory:
                case Page::guestText:
                    return guestsPresentation().rows;
                case Page::marketing:
                case Page::marketingActive:
                case Page::marketingTypes:
                case Page::marketingEditor:
                case Page::marketingTargets:
                case Page::marketingConfirm:
                    return marketingPresentationFor(_page, _focus.index()).rows;
                case Page::research:
                case Page::researchEditor:
                case Page::researchConfirm:
                    return researchPresentation().rows;
                case Page::shop:
                    return { "Status", "Prices", "Demolish", "Rename" };
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
                {
                    std::vector<std::string> result{ "Open ride", "Close ride", "Back to park" };
                    if (_ridesTarget && Handheld::RideAccess::Supported(*_ridesTarget))
                        result.push_back("Access / paths...");
                    if (_ridesTarget && Handheld::CustomTracks::CanResume(*_ridesTarget))
                        result.push_back("Continue track...");
                    return result;
                }
                case Page::customChoice:
                    return { "Custom build", "Saved designs...", "Back" };
                case Page::build:
                    return { "Raise land",
                             "Lower land",
                             "Paths",
                             "Water",
                             "Surface",
                             "Remove scenery",
                             "Trees",
                             "Shops / Facilities",
                             "Rides",
                             "Wooden designs",
                             "Scenery...",
                             "Terrain / Water...",
                             "Land / Rights...",
                             "Back to park" };
                case Page::sceneryCategory:
                    return { "Small scenery / trees", "Large scenery", "Walls / fences", "Banners / signs", "Back to build" };
                case Page::rightsOptions:
                    return Handheld::RightsTools::Options();
                case Page::rightsConfirm:
                    return { "Purchase available tiles", "Cancel" };
                case Page::areaOptions:
                    return Handheld::AreaTools::Options();
                case Page::sceneryName:
                    return _nameEntry.Keys();
                case Page::sceneryConfirm:
                    return { "Apply text", "Cancel" };
                case Page::sceneryOptions:
                    return Handheld::SceneryTools::Options();
                case Page::sceneryPicker:
                    return {};
                case Page::designConfirm:
                    return { "Cancel", "Place design" };
                case Page::trackPiecePicker:
                case Page::designPicker:
                case Page::treePicker:
                case Page::shopPicker:
                case Page::ridePicker:
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
        size_t dashboardSectionIndex() const
        {
            return _dashboard.index();
        }
        bool overviewIsOpen() const
        {
            return overviewManagement();
        }
        Handheld::ManagementPresentation overviewModel() const
        {
            return overviewPresentation();
        }
        bool ridesIsOpen() const
        {
            return ridesManagement();
        }
        Handheld::ManagementPresentation ridesPresentation() const
        {
            return ridesPresentationFor(_page, _focus.index());
        }
        bool guestsIsOpen() const
        {
            return guestManagement();
        }
        Handheld::ManagementPresentation guestsPresentation() const
        {
            return guestPresentationFor(_page, _focus.index());
        }
        void onOpen() override
        {
            Handheld::Paths::ClearPreview();
            Handheld::Trees::ClearPreview();
            Handheld::SceneryTools::ClearPreview();
            Handheld::Shops::ClearPreview();
            Handheld::FlatRides::ClearPreview();
            Handheld::Designs::ClearPreview();
            Handheld::CustomTracks::ClearPreview();
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
            if (_ridesReviewResumed || _rightsReviewResumed)
                _pausedHere = false; // Our pause was already released; retain any external pause.
            if (_page == Page::designConfirm && _focus.index() == 0)
                Handheld::Designs::CancelReview();
            if (_pausedHere && gLegacyScene == LegacyScene::playing && GameIsPaused())
                PauseToggle();
        }
        void openRideAccess(const Handheld::RideManagement::Target& target)
        {
            _ridesTarget = target;
            _dashboard.section = Handheld::Dashboard::Section::rides;
            _dashboard.focus = Handheld::Dashboard::Focus::content;
            page(Page::ridesAccess);
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
            auto rideEntries = Handheld::FlatRides::Entries();
            auto custom = Handheld::CustomTracks::RideEntries();
            rideEntries.insert(rideEntries.end(), custom.begin(), custom.end());
            _picker.setEntries(
                _page == Page::trackPiecePicker    ? Handheld::CustomTracks::Pieces()
                    : _page == Page::designPicker  ? Handheld::Designs::Entries()
                    : _page == Page::ridePicker    ? rideEntries
                    : _page == Page::shopPicker    ? Handheld::Shops::Entries()
                    : _page == Page::sceneryPicker ? Handheld::SceneryTools::Entries(Handheld::SceneryTools::Current().category)
                                                   : Handheld::TreePickerEntries(),
                preferred);
        }
        void openRightsOptions()
        {
            page(Page::rightsOptions);
        }
        void openRightsReview(CoordsXY tile)
        {
            if (_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL))
            {
                PauseToggle();
                _rightsReviewResumed = true;
            }

            const auto q = Handheld::RightsTools::Query(tile, Handheld::RightsTools::settings);
            _rightsDraft = Handheld::RightsTools::Begin(tile, Handheld::RightsTools::settings);
            if (_rightsDraft)
            {
                page(Page::rightsConfirm);
                _focus.move(1, 2);
            }
            else
            {
                page(Page::rightsOptions);
                _status = q.getErrorMessage();
            }
        }
        void openAreaOptions()
        {
            page(Page::areaOptions);
        }
        void openSceneryOptions()
        {
            page(Page::sceneryOptions);
        }
        void openTreePicker()
        {
            page(Page::treePicker);
            refreshPicker(Handheld::PickerIdentity{ ObjectType::smallScenery, Handheld::TreeSelection() });
        }
        void openShopPicker(bool fromBuild = false)
        {
            Handheld::Shops::ClearPreview();
            Handheld::FlatRides::ClearPreview();
            Handheld::Designs::ClearPreview();
            Handheld::CustomTracks::ClearPreview();
            _shopPickerFromBuild = fromBuild;
            page(Page::shopPicker);
            refreshPicker(Handheld::PickerIdentity{ ObjectType::ride, Handheld::ShopSelection() });
        }
        void openRidePicker(bool fromBuild = false)
        {
            Handheld::FlatRides::ClearPreview();
            Handheld::Designs::ClearPreview();
            Handheld::CustomTracks::ClearPreview();
            _ridePickerFromBuild = fromBuild;
            page(Page::ridePicker);
            refreshPicker(Handheld::PickerIdentity{ ObjectType::ride, Handheld::FlatRides::Selection() });
        }
        void openDesignPicker(bool fromBuild = false)
        {
            TrackRepositoryScan();
            Handheld::Designs::ClearPreview();
            Handheld::CustomTracks::ClearPreview();
            Handheld::Designs::CancelReview();
            _designPickerFromBuild = fromBuild;
            page(Page::designPicker);
            refreshPicker(Handheld::PickerIdentity{ ObjectType::ride, kObjectEntryIndexNull, Handheld::Designs::Selection() });
        }
        void pauseDesignReview()
        {
            Handheld::Designs::CancelReview();
            page(Page::park);
        }
        void openDesignReview()
        {
            page(Page::designConfirm);
            _status = "Cost " + Handheld::FormatMoney(Handheld::Designs::ReviewCost());
        }
        void openTrackPiecePicker()
        {
            Handheld::CustomTracks::ClearPreview();
            page(Page::trackPiecePicker);
            refreshPicker(
                Handheld::PickerIdentity{ ObjectType::ride, kObjectEntryIndexNull,
                                          std::to_string(static_cast<int>(Handheld::CustomTracks::Selection())) });
        }
        bool picker() const
        {
            return _page == Page::sceneryPicker || _page == Page::trackPiecePicker || _page == Page::designPicker
                || _page == Page::treePicker || _page == Page::shopPicker || _page == Page::ridePicker;
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
            if (_page == Page::ridesRoster)
            {
                const auto updated = Handheld::RideManagement::Roster();
                if (updated != _ridesRoster)
                {
                    const auto selected = _focus.index() < _ridesRoster.size()
                        ? std::optional<Handheld::RideManagement::Target>{ _ridesRoster[_focus.index()] }
                        : std::nullopt;
                    const auto found = selected ? std::find(updated.begin(), updated.end(), *selected) : updated.end();
                    const auto row = found == updated.end() ? std::min(_focus.index(), updated.size())
                                                            : static_cast<size_t>(found - updated.begin());
                    _ridesRoster = updated;
                    _ridesRosterFocus = row;
                    _focus = {};
                    _focus.move(static_cast<int32_t>(row), rows().size());
                }
            }
            if (ridesManagement() || overviewManagement())
                invalidate();
            if (_page == Page::guestRoster)
            {
                const auto updated = Handheld::GuestManagement::Roster();
                if (updated != _guestRoster)
                {
                    const auto selected = _focus.index() < _guestRoster.size()
                        ? std::optional<Handheld::GuestManagement::Target>{ _guestRoster[_focus.index()] }
                        : std::nullopt;
                    size_t row = std::min(_focus.index(), updated.size());
                    if (selected)
                        if (const auto found = std::find(updated.begin(), updated.end(), *selected); found != updated.end())
                            row = std::distance(updated.begin(), found);
                    _guestRoster = updated;
                    _guestRosterFocus = row;
                    _focus = {};
                    _focus.move(static_cast<int32_t>(row), rows().size());
                    invalidate();
                }
            }
            if (guestManagement())
                invalidate(); // read native state without clearing native guest invalidation flags
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
                || (_page == Page::shopPicker && !_shopPickerFromBuild && !Handheld::IsShopToolActive())
                || (_page == Page::ridePicker && !_ridePickerFromBuild
                    && Handheld::FlatRides::CurrentStage() != Handheld::FlatRides::Stage::ride))
                close();
        }
        void openRide(RideId id)
        {
            _rideId = id;
            _ridesTarget = Handheld::RideManagement::Identify(id);
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
        bool dashboardIsOpen() const
        {
            return managementShell();
        }
        bool dashboardRailFocused() const
        {
            return managementShell() && _dashboard.focus == Handheld::Dashboard::Focus::rail;
        }
        bool dashboardCanSwitch() const
        {
            return _page == Page::dashboard || _page == Page::overview || _page == Page::finances || _page == Page::staff
                || _page == Page::staffInspect || _page == Page::staffStats || _page == Page::research
                || _page == Page::marketing || _page == Page::marketingActive || guestManagement() || _page == Page::ridesRoster
                || _page == Page::ridesInspect || _page == Page::ridesAccess || _page == Page::ridesGraphs
                || _page == Page::ridesStations || _page == Page::ridesCustomers || _page == Page::ridesEarnings
                || _page == Page::ridesStats || _page == Page::ridesVehicles || _page == Page::ridesAppearance
                || _page == Page::ridesMusic || _page == Page::ridesDispatch || _page == Page::ridesOperations
                || _page == Page::ridesMaintenance;
        }
        void enterDashboardContent()
        {
            using Handheld::Dashboard::Section;
            const auto root = _dashboard.section == Section::staff ? std::optional<Page>{ Page::staff }
                : _dashboard.section == Section::finances          ? std::optional<Page>{ Page::finances }
                : _dashboard.section == Section::research          ? std::optional<Page>{ Page::research }
                : _dashboard.section == Section::marketing         ? std::optional<Page>{ Page::marketing }
                : _dashboard.section == Section::guests            ? std::optional<Page>{ Page::guests }
                : _dashboard.section == Section::rides             ? std::optional<Page>{ Page::ridesRoster }
                : _dashboard.section == Section::overview          ? std::optional<Page>{ Page::overview }
                                                                   : std::nullopt;
            if (!root || !_dashboard.enter())
                return;
            const auto resume = _sectionResume[_dashboard.index()];
            page(resume.page.value_or(*root));
            if (resume.page)
            {
                _focus = {};
                _focus.move(static_cast<int32_t>(std::min(resume.row, rows().size() - 1)), rows().size());
            }
            invalidate();
        }
        void openDashboard()
        {
            page(Page::dashboard);
        }
        void dashboardPause()
        {
            if (managementShell())
                page(Page::park);
        }
        void dashboardSwitchPane()
        {
            if (!managementShell() || !dashboardCanSwitch())
                return;
            if (dashboardRailFocused())
                enterDashboardContent();
            else
            {
                if (_page == Page::guestRoster)
                    _guestRosterFocus = _focus.index();
                if (_page == Page::staff)
                    _staffRosterFocus = _focus.index();
                page(Page::dashboard);
            }
        }
        void dashboardMove(int x, int y)
        {
            if (dashboardRailFocused())
            {
                if (y)
                {
                    _dashboard.move(y);
                    _status.clear();
                    invalidate();
                }
            }
            else if (marketingManagement())
            {
                if (y)
                    move(y);
                if (x && _page == Page::marketingEditor && _marketingDraft
                    && _focus.index() == (Handheld::MarketingManagement::NeedsTarget(_marketingDraft->type) ? 1u : 0u))
                {
                    Handheld::MarketingManagement::AdjustWeeks(*_marketingDraft, x);
                    _status.clear();
                    invalidate();
                }
            }
            else if (researchManagement())
            {
                if (y)
                    move(y);
                if (x && _page == Page::researchEditor && _focus.index() == 0 && _researchDraft)
                {
                    Handheld::ResearchManagement::AdjustFunding(*_researchDraft, x);
                    _status.clear();
                    invalidate();
                }
            }
            else if (_page == Page::ridesGraphs)
            {
                if (y)
                    move(y);
                if (x)
                {
                    const auto* ride = _ridesTarget ? Handheld::RideManagement::Resolve(*_ridesTarget) : nullptr;
                    const auto max = ride && ride->measurement && ride->measurement->num_items
                        ? ride->measurement->num_items - 1
                        : 0;
                    _rideGraphOffset = std::clamp<int>(static_cast<int>(_rideGraphOffset) + x * 120, 0, max);
                    invalidate();
                }
            }
            else if (_page == Page::ridesName)
            {
                const auto current = _focus.index();
                const auto next = _nameEntry.Move(current, x, y);
                _focus.move(static_cast<int>(next) - static_cast<int>(current), rows().size());
                invalidate();
            }
            else if (_page == Page::ridesPrice)
            {
                if (y)
                    move(y);
                if (x && _ridesDraft)
                {
                    Handheld::RideManagement::AdjustPrice(*_ridesDraft, x);
                    _status.clear();
                    invalidate();
                }
            }
            else if (financeManagement())
                financeMove(x, y);
            else if (y)
                move(y);
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
            if (dashboardRailFocused())
            {
                _dashboard.move(direction);
                _status.clear();
                invalidate();
                return;
            }
            _focus.move(direction, rows().size());
            _status.clear();
            invalidate();
        }
        void back()
        {
            switch (_page)
            {
                case Page::sceneryCategory:
                    page(Page::build);
                    break;
                case Page::sceneryPicker:
                    page(Page::sceneryOptions);
                    break;
                case Page::rightsConfirm:
                    _rightsDraft.reset();
                    close();
                    break;
                case Page::rightsOptions:
                    close();
                    break;
                case Page::areaOptions:
                    close();
                    break;
                case Page::sceneryName:
                    page(Page::sceneryOptions);
                    break;
                case Page::sceneryConfirm:
                    page(Page::sceneryName);
                    break;
                case Page::sceneryOptions:
                    close();
                    break;
                case Page::dashboard:
                    close();
                    break;
                case Page::designConfirm:
                    Handheld::Designs::CancelReview();
                    close();
                    break;
                case Page::customChoice:
                    openRidePicker(true);
                    break;
                case Page::trackPiecePicker:
                    close();
                    break;
                case Page::designPicker:
                    if (_designPickerFromBuild)
                        page(Page::build);
                    else
                        close();
                    break;
                case Page::ridePicker:
                    if (_ridePickerFromBuild)
                        page(Page::build);
                    else
                        close();
                    break;
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
                case Page::ridesRoster:
                    page(Page::dashboard);
                    break;
                case Page::ridesInspect:
                    page(Page::ridesRoster);
                    break;
                case Page::ridesName:
                    page(_ridesTarget && _ridesTarget->shop ? Page::shop : Page::ridesInspect);
                    break;
                case Page::ridesAccess:
                case Page::ridesVehicles:
                case Page::ridesAppearance:
                case Page::ridesMusic:
                case Page::ridesDispatch:
                case Page::ridesOperations:
                case Page::ridesMaintenance:
                case Page::ridesPrice:
                    page(ridesDraftParent());
                    break;
                case Page::ridesStations:
                    page(Page::ridesCustomers);
                    break;
                case Page::ridesGraphs:
                case Page::ridesCustomers:
                case Page::ridesEarnings:
                    page(Page::ridesStats);
                    break;
                case Page::ridesStats:
                    page(
                        _ridesTarget && Handheld::RideManagement::Resolve(*_ridesTarget) ? Page::ridesInspect
                                                                                         : Page::ridesRoster);
                    break;
                case Page::ridesConfirm:
                    if (_ridesDraft
                        && (_ridesDraft->renaming || _ridesDraft->refurbishment || !_ridesDraft->randomColours.empty()))
                        page(ridesDraftParent());
                    else if (_ridesDraft && !_ridesDraft->status && !_ridesDraft->demolition)
                        page(Page::ridesPrice);
                    else
                        page(
                            _ridesTarget && Handheld::RideManagement::Resolve(*_ridesTarget) ? Page::ridesInspect
                                                                                             : Page::ridesRoster);
                    break;
                case Page::guestRoster:
                    page(Page::guests);
                    break;
                case Page::guestInspect:
                    page(Page::guestRoster);
                    break;
                case Page::guestText:
                    page(
                        _guestTarget && Handheld::GuestManagement::Resolve(*_guestTarget) ? _guestTextParent
                                                                                          : Page::guestRoster);
                    break;
                case Page::guestNeeds:
                case Page::guestThoughts:
                case Page::guestSpending:
                case Page::guestInventory:
                    page(
                        _guestTarget && Handheld::GuestManagement::Resolve(*_guestTarget) ? Page::guestInspect
                                                                                          : Page::guestRoster);
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
                case Page::marketing:
                    page(Page::dashboard);
                    break;
                case Page::marketingActive:
                case Page::marketingTypes:
                    page(Page::marketing);
                    break;
                case Page::marketingEditor:
                    page(Page::marketingTypes);
                    break;
                case Page::marketingTargets:
                case Page::marketingConfirm:
                    page(Page::marketingEditor);
                    break;
                case Page::researchConfirm:
                    page(Page::researchEditor);
                    break;
                case Page::researchEditor:
                    page(Page::research);
                    break;
                case Page::financeConfirm:
                    page(Page::financeEditor);
                    break;
                case Page::financeEditor:
                    page(Page::finances);
                    break;
                case Page::overviewConfirm:
                    page(Page::overview);
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
            if ((overviewManagement() && _overviewBusy) || (ridesManagement() && _ridesBusy)
                || (marketingManagement() && _marketingBusy))
                return;
            if (ridesManagement() && _page != Page::ridesRoster
                && (!_ridesTarget || !Handheld::RideManagement::Resolve(*_ridesTarget)))
            {
                page(Page::ridesRoster);
                return;
            }
            const auto choice = _focus.index();
            switch (_page)
            {
                case Page::designConfirm:
                    if (choice == 0)
                    {
                        Handheld::Designs::CancelReview();
                        close();
                    }
                    else
                    {
                        // Release only the pause owned by this menu before native revalidation.
                        close();
                        const auto result = Handheld::Designs::Confirm();
                        if (result.error != GameActions::Status::ok)
                            GetWindowManager()->ShowError("Cannot place design", result.getErrorMessage());
                    }
                    break;
                case Page::designPicker:
                    if (_picker.selected() && Handheld::BeginSavedDesign(_picker.selected()->identity.key))
                        close();
                    else
                    {
                        refreshPicker();
                        _status = "Design unavailable";
                        invalidate();
                    }
                    break;
                case Page::customChoice:
                    if (choice == 0)
                    {
                        if (Handheld::BeginCustomTrack(_customObject))
                            close();
                        else
                        {
                            _status = "Wooden coaster unavailable";
                            invalidate();
                        }
                    }
                    else if (choice == 1)
                        openDesignPicker(true);
                    else
                        openRidePicker(true);
                    break;
                case Page::trackPiecePicker:
                    if (_picker.selected())
                    {
                        const auto key = _picker.selected()->identity.key;
                        if (key == "leave")
                        {
                            Handheld::EndTerrain();
                            close();
                        }
                        else if (Handheld::CustomTracks::Select(static_cast<TrackElemType>(std::stoi(key))))
                            close();
                        else
                        {
                            refreshPicker();
                            _status = "Piece unavailable";
                            invalidate();
                        }
                    }
                    break;
                case Page::ridePicker:
                    if (_picker.selected() && _picker.selected()->identity.key.starts_with("custom:"))
                    {
                        _customObject = _picker.selected()->identity.index;
                        page(Page::customChoice);
                        break;
                    }
                    if (_picker.selected() && Handheld::BeginFlatRide(_picker.selected()->identity.index))
                        close();
                    else
                    {
                        refreshPicker();
                        _status = "Ride unavailable";
                        invalidate();
                    }
                    break;
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
                case Page::sceneryCategory:
                    if (choice == 4)
                        page(Page::build);
                    else
                    {
                        Handheld::SceneryTools::SelectCategory(static_cast<Handheld::SceneryTools::Category>(choice));
                        page(Page::sceneryPicker);
                        refreshPicker();
                    }
                    break;
                case Page::sceneryPicker:
                    if (_picker.selected() && Handheld::SceneryTools::Select(_picker.selected()->identity.index))
                    {
                        close();
                        if (!Handheld::IsSceneryToolActive())
                            Handheld::BeginScenery();
                    }
                    else
                    {
                        refreshPicker();
                        _status = "Scenery unavailable";
                    }
                    break;
                case Page::sceneryName:
                    if (choice == 47)
                        page(Page::sceneryOptions);
                    else if (choice == 46 && _signTarget)
                    {
                        const auto query = Handheld::SceneryTools::SetText(*_signTarget, _nameEntry.text, false);
                        if (query.error == GameActions::Status::ok)
                        {
                            page(Page::sceneryConfirm);
                            _focus.move(1, 2);
                        }
                        else
                        {
                            _status = query.getErrorMessage();
                            invalidate();
                        }
                    }
                    else
                    {
                        _nameEntry.Press(choice);
                        invalidate();
                    }
                    break;
                case Page::sceneryConfirm:
                    if (choice == 1)
                        page(Page::sceneryName);
                    else if (_signTarget)
                    {
                        const auto result = Handheld::SceneryTools::SetText(*_signTarget, _nameEntry.text, true);
                        if (result.error == GameActions::Status::ok)
                            page(Page::sceneryOptions);
                        else
                        {
                            _status = result.getErrorMessage();
                            invalidate();
                        }
                    }
                    break;
                case Page::sceneryOptions:
                    if (choice == 11)
                    {
                        _signTarget = Handheld::SceneryEditTarget();
                        if (_signTarget && _signTarget->banner != BannerIndex::GetNull())
                        {
                            _nameEntry = { _signTarget->text };
                            page(Page::sceneryName);
                        }
                        else
                        {
                            _status = "Point at a banner or sign";
                            invalidate();
                        }
                    }
                    else if (choice == 0)
                        close();
                    else if (choice == 1)
                    {
                        page(Page::sceneryPicker);
                        refreshPicker();
                    }
                    else
                    {
                        Handheld::SceneryTools::Configure(choice);
                        invalidate();
                    }
                    break;
                case Page::rightsOptions:
                    if (choice == 0)
                        close();
                    else
                    {
                        Handheld::RightsTools::Configure(choice);
                        invalidate();
                    }
                    break;
                case Page::rightsConfirm:
                    if (choice == 1)
                    {
                        _rightsDraft.reset();
                        close();
                    }
                    else if (_rightsDraft)
                    {
                        const auto r = Handheld::RightsTools::Action(*_rightsDraft, true);
                        if (r.error == GameActions::Status::ok)
                        {
                            _rightsDraft.reset();
                            close();
                        }
                        else
                        {
                            _status = r.getErrorMessage();
                            invalidate();
                        }
                    }
                    break;
                case Page::areaOptions:
                    if (choice == 0)
                        close();
                    else
                    {
                        Handheld::AreaTools::Configure(choice);
                        invalidate();
                    }
                    break;
                case Page::build:
                    if (choice == 12)
                    {
                        close();
                        Handheld::BeginRightsTools();
                    }
                    else if (choice == 11)
                    {
                        close();
                        Handheld::BeginAreaTools();
                    }
                    else if (choice == 10)
                        page(Page::sceneryCategory);
                    else if (choice == 9)
                        openDesignPicker(true);
                    else if (choice == 8)
                        openRidePicker(true);
                    else if (choice == 7)
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
                    if (_dashboard.category().available)
                        enterDashboardContent();
                    else
                    {
                        _status = "Section not implemented";
                        invalidate();
                    }
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
                    if (choice == 0)
                    {
                        _overviewDraft = Handheld::ParkOverview::Begin();
                        if (_overviewDraft)
                            page(Page::overviewConfirm);
                        else
                        {
                            _status = "Park status locked by objective";
                            invalidate();
                        }
                    }
                    else if (Handheld::Finances::Available(Handheld::Finances::Field::admission))
                    {
                        _dashboard.section = Handheld::Dashboard::Section::finances;
                        _dashboard.focus = Handheld::Dashboard::Focus::content;
                        page(Page::finances);
                        _focus.move(1, rows().size()); // existing Finances admission row; no editor/action spill
                    }
                    else
                    {
                        _status = "Admission pricing unavailable";
                        invalidate();
                    }
                    break;
                case Page::overviewConfirm:
                    if (choice == 1)
                    {
                        page(Page::overview);
                        break;
                    }
                    if (_overviewDraft)
                    {
                        const auto query = Handheld::ParkOverview::Action(*_overviewDraft, false);
                        if (query.error != GameActions::Status::ok)
                        {
                            _status = Handheld::Finances::Feedback(query);
                            invalidate();
                            break;
                        }
                        _overviewBusy = true;
                        const auto request = ++_overviewRequest;
                        const auto result = Handheld::ParkOverview::Action(
                            *_overviewDraft, true,
                            [this, request, lifetime = std::weak_ptr<int>(_overviewLifetime)](
                                const GameActions::GameAction*, const GameActions::Result* outcome) {
                                if (lifetime.expired() || request != _overviewRequest || _page != Page::overviewConfirm)
                                    return;
                                _overviewBusy = false;
                                if (outcome->error == GameActions::Status::ok)
                                {
                                    page(Page::overview);
                                    _status = "Park status updated";
                                }
                                else
                                    _status = Handheld::Finances::Feedback(*outcome);
                                invalidate();
                            });
                        if (result.error != GameActions::Status::ok)
                        {
                            _overviewBusy = false;
                            _status = Handheld::Finances::Feedback(result);
                        }
                        invalidate();
                    }
                    break;
                case Page::ridesRoster:
                    _ridesRosterFocus = choice;
                    if (choice >= _ridesRoster.size())
                        page(Page::dashboard);
                    else if (Handheld::RideManagement::Resolve(_ridesRoster[choice]))
                    {
                        _ridesTarget = _ridesRoster[choice];
                        page(Page::ridesInspect);
                    }
                    else
                    {
                        _status = "Ride unavailable";
                        invalidate();
                    }
                    break;
                case Page::ridesInspect:
                {
                    if (!_ridesTarget || !Handheld::RideManagement::Resolve(*_ridesTarget))
                    {
                        page(Page::ridesRoster);
                        break;
                    }
                    const auto rows = ridesPresentationFor(Page::ridesInspect, choice).rows;
                    if (choice == rows.size() - 8)
                    {
                        _ridesDraft = Handheld::RideManagement::BeginName(*_ridesTarget);
                        if (_ridesDraft)
                        {
                            _nameEntry = { _ridesDraft->name };
                            page(Page::ridesName);
                        }
                        break;
                    }
                    if (choice == rows.size() - 7)
                    {
                        page(Page::ridesAppearance);
                        break;
                    }
                    if (choice == rows.size() - 6)
                    {
                        page(Page::ridesMusic);
                        break;
                    }
                    if (choice == rows.size() - 5)
                    {
                        page(Page::ridesVehicles);
                        break;
                    }
                    if (choice == rows.size() - 4)
                    {
                        page(Page::ridesDispatch);
                        break;
                    }
                    if (choice == rows.size() - 3)
                    {
                        page(Page::ridesOperations);
                        break;
                    }
                    if (choice == rows.size() - 2)
                    {
                        page(Page::ridesMaintenance);
                        break;
                    }
                    if (choice == rows.size() - 1)
                    {
                        _ridesDraft = Handheld::RideManagement::BeginDemolition(*_ridesTarget);
                        if (_ridesDraft)
                        {
                            if (_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL))
                            {
                                PauseToggle();
                                _ridesReviewResumed = true;
                            }
                            page(Page::ridesConfirm);
                        }
                        break;
                    }
                    if (choice == 0)
                    {
                        const auto target = *_ridesTarget;
                        if (WindowGetMain() && Handheld::RideManagement::Location(target))
                        {
                            close();
                            Handheld::RideManagement::Locate(target);
                        }
                        else
                        {
                            _status = "Location unavailable";
                            invalidate();
                        }
                    }
                    else if (choice == 1)
                        page(Page::ridesStats);
                    else if (choice == 6)
                        page(Page::ridesRoster);
                    else if (
                        choice == (Handheld::FlatRides::CanResume(*_ridesTarget) ? 8u : 7u)
                        && Handheld::RideAccess::Supported(*_ridesTarget))
                        page(Page::ridesAccess);
                    else if (
                        Handheld::CustomTracks::CanResume(*_ridesTarget)
                        && choice
                            == 7u + (Handheld::FlatRides::CanResume(*_ridesTarget) ? 1u : 0u)
                                + (Handheld::RideAccess::Supported(*_ridesTarget) ? 1u : 0u))
                    {
                        const auto target = *_ridesTarget;
                        close();
                        Handheld::ResumeCustomTrack(target.id);
                    }
                    else if (choice == 7 && Handheld::FlatRides::CanResume(*_ridesTarget))
                    {
                        const auto target = *_ridesTarget;
                        close();
                        Handheld::ResumeFlatRide(target.id);
                        Handheld::RideManagement::Locate(target);
                    }
                    else
                    {
                        const auto status = choice == 2 ? std::optional<RideStatus>{ RideStatus::open }
                            : choice == 3               ? std::optional<RideStatus>{ RideStatus::closed }
                            : choice == 4               ? std::optional<RideStatus>{ RideStatus::testing }
                                                        : std::nullopt;
                        _ridesDraft = Handheld::RideManagement::Begin(*_ridesTarget, status);
                        if (_ridesDraft)
                            page(status ? Page::ridesConfirm : Page::ridesPrice);
                        else
                        {
                            _status = "Action unavailable for this ride";
                            invalidate();
                        }
                    }
                    break;
                }
                case Page::ridesAccess:
                    if (choice == 0)
                        close();
                    else if (!_ridesTarget || !Handheld::RideAccess::Inspect(*_ridesTarget))
                    {
                        _status = "Ride/access changed; return to inspection";
                        invalidate();
                    }
                    else if (choice == 1 || choice == 2)
                    {
                        const auto target = *_ridesTarget;
                        close();
                        Handheld::RideAccess::BeginPaths(target, choice == 2);
                    }
                    else if (choice == 3)
                    {
                        _ridesDraft = Handheld::RideManagement::Begin(*_ridesTarget, RideStatus::open);
                        if (_ridesDraft)
                            page(Page::ridesConfirm);
                    }
                    else if (choice == 4)
                        page(Page::ridesInspect);
                    break;
                case Page::ridesVehicles:
                case Page::ridesAppearance:
                case Page::ridesMusic:
                case Page::ridesDispatch:
                case Page::ridesOperations:
                case Page::ridesMaintenance:
                {
                    const auto* ride = _ridesTarget ? Handheld::RideManagement::Resolve(*_ridesTarget) : nullptr;
                    const auto settings = ride
                        ? (_page == Page::ridesAppearance     ? Handheld::RideManagement::AppearanceSettings(*ride)
                               : _page == Page::ridesMusic    ? Handheld::RideManagement::MusicSettings(*ride)
                               : _page == Page::ridesVehicles ? Handheld::RideManagement::VehicleSettings(*ride)
                               : _page == Page::ridesDispatch
                               ? Handheld::RideManagement::DispatchSettings(*ride)
                               : Handheld::RideManagement::Settings(*ride, _page == Page::ridesMaintenance))
                        : std::vector<Handheld::RideManagement::Setting>{};
                    const auto appearanceRows = _page == Page::ridesAppearance ? ridesPresentationFor(_page, choice).rows
                                                                               : std::vector<std::string>{};
                    if (_page == Page::ridesAppearance && choice < appearanceRows.size()
                        && appearanceRows[choice] == "Randomize vehicle colours...")
                    {
                        _ridesDraft = Handheld::RideManagement::BeginRandomColours(*_ridesTarget);
                        if (_ridesDraft)
                            page(Page::ridesConfirm);
                    }
                    else if (
                        ride && _page == Page::ridesAppearance && choice < appearanceRows.size()
                        && appearanceRows[choice] == "Paint track scheme...")
                    {
                        const auto target = *_ridesTarget;
                        close();
                        Handheld::BeginTrackColour(target);
                    }
                    else if (
                        ride && _page == Page::ridesMaintenance && !ride->getRideTypeDescriptor().availableBreakdowns.isEmpty()
                        && choice == settings.size())
                    {
                        if (Handheld::RideManagement::LocateMechanic(*_ridesTarget))
                            close();
                        else
                        {
                            _status = "Unable to locate mechanic";
                            invalidate();
                        }
                    }
                    else if (
                        ride && _page == Page::ridesMaintenance && !ride->getRideTypeDescriptor().availableBreakdowns.isEmpty()
                        && choice == settings.size() + 1)
                    {
                        _ridesDraft = Handheld::RideManagement::BeginRefurbishment(*_ridesTarget);
                        if (_ridesDraft)
                        {
                            if (_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL))
                            {
                                PauseToggle();
                                _ridesReviewResumed = true;
                            }
                            page(Page::ridesConfirm);
                        }
                    }
                    else if (choice >= settings.size())
                        page(ride ? Page::ridesInspect : Page::ridesRoster);
                    else
                    {
                        _ridesDraft = Handheld::RideManagement::BeginSetting(*_ridesTarget, settings[choice]);
                        if (_ridesDraft)
                            page(Page::ridesPrice);
                    }
                    break;
                }
                case Page::ridesGraphs:
                    if (choice == 4)
                        page(Page::ridesStats);
                    else
                    {
                        _rideGraphMetric = choice;
                        invalidate();
                    }
                    break;
                case Page::ridesStations:
                    page(Page::ridesCustomers);
                    break;
                case Page::ridesCustomers:
                    page(choice == 0 ? Page::ridesStations : Page::ridesStats);
                    break;
                case Page::ridesEarnings:
                    page(Page::ridesStats);
                    break;
                case Page::ridesStats:
                    if (choice == 2 && _ridesTarget)
                    {
                        Handheld::RideManagement::BeginGraphs(*_ridesTarget);
                        _rideGraphOffset = _rideGraphMetric = 0;
                    }
                    page(
                        choice == 0                                                            ? Page::ridesCustomers
                            : choice == 1                                                      ? Page::ridesEarnings
                            : choice == 2                                                      ? Page::ridesGraphs
                            : _ridesTarget && Handheld::RideManagement::Resolve(*_ridesTarget) ? Page::ridesInspect
                                                                                               : Page::ridesRoster);
                    break;
                case Page::ridesName:
                    if (choice == 47)
                        page(_ridesTarget && _ridesTarget->shop ? Page::shop : Page::ridesInspect);
                    else if (choice == 46 && _ridesDraft)
                    {
                        _ridesDraft->name = _nameEntry.text;
                        page(Page::ridesConfirm);
                    }
                    else
                    {
                        _nameEntry.Press(choice);
                        invalidate();
                    }
                    break;
                case Page::ridesPrice:
                    if (!_ridesDraft)
                    {
                        page(Page::ridesRoster);
                        break;
                    }
                    if (choice < 2)
                    {
                        Handheld::RideManagement::AdjustPrice(*_ridesDraft, choice == 0 ? -1 : 1);
                        _status.clear();
                        invalidate();
                    }
                    else if (choice == 2)
                        page(Page::ridesConfirm);
                    else
                        page(ridesDraftParent());
                    break;
                case Page::ridesConfirm:
                    if (choice == 1)
                    {
                        page(ridesDraftParent());
                        break;
                    }
                    if (_ridesDraft)
                    {
                        const auto query = Handheld::RideManagement::Action(*_ridesDraft, false);
                        if (query.error != GameActions::Status::ok)
                        {
                            _status = Handheld::Finances::Feedback(query);
                            invalidate();
                            break;
                        }
                        const auto request = ++_ridesRequest;
                        _ridesBusy = true;
                        const auto result = Handheld::RideManagement::Action(
                            *_ridesDraft, true,
                            [this, request, lifetime = std::weak_ptr<int>(_ridesLifetime)](
                                const GameActions::GameAction*, const GameActions::Result* outcome) {
                                if (lifetime.expired() || request != _ridesRequest || _page != Page::ridesConfirm)
                                    return;
                                _ridesBusy = false;
                                if (outcome->error == GameActions::Status::ok)
                                {
                                    const bool demolished = _ridesDraft && _ridesDraft->demolition;
                                    // A native vehicle-object change intentionally changes the identity's object field.
                                    // Refresh only after this owned action succeeds; external changes remain stale.
                                    if (_ridesDraft && _ridesDraft->setting
                                        && _ridesDraft->setting->vehicle == GameActions::RideSetVehicleType::rideEntry)
                                        _ridesTarget = Handheld::RideManagement::Identify(_ridesDraft->target.id);
                                    if (_ridesDraft && _ridesDraft->refurbishment)
                                        _ridesTarget = Handheld::RideManagement::Identify(_ridesDraft->target.id);
                                    const auto destination = _ridesDraft && _ridesDraft->renaming
                                        ? (_ridesTarget && _ridesTarget->shop ? Page::shop : Page::ridesInspect)
                                        : ridesDraftParent();
                                    page(demolished ? Page::ridesRoster : destination);
                                    _status = demolished ? "Ride demolished" : "Ride updated";
                                }
                                else
                                    _status = Handheld::Finances::Feedback(*outcome);
                                invalidate();
                            });
                        if (result.error != GameActions::Status::ok)
                        {
                            _ridesBusy = false;
                            _status = Handheld::Finances::Feedback(result);
                        }
                        invalidate();
                    }
                    break;
                case Page::guests:
                    page(choice == 0 ? Page::guestRoster : Page::dashboard);
                    break;
                case Page::guestRoster:
                    _guestRosterFocus = choice;
                    if (choice >= _guestRoster.size())
                        page(Page::guests);
                    else if (Handheld::GuestManagement::Resolve(_guestRoster[choice]))
                    {
                        _guestTarget = _guestRoster[choice];
                        page(Page::guestInspect);
                    }
                    else
                    {
                        _status = "Guest no longer in park";
                        invalidate();
                    }
                    break;
                case Page::guestInspect:
                    if (!_guestTarget || !Handheld::GuestManagement::Resolve(*_guestTarget))
                    {
                        page(Page::guestRoster);
                        break;
                    }
                    if (choice == 0)
                    {
                        const auto target = *_guestTarget;
                        if (WindowGetMain() && Handheld::GuestManagement::Location(target))
                        {
                            close();
                            Handheld::GuestManagement::Locate(target);
                            return;
                        }
                        _status = "Location unavailable";
                        invalidate();
                    }
                    else
                        page(
                            choice == 1       ? Page::guestNeeds
                                : choice == 2 ? Page::guestThoughts
                                : choice == 3 ? Page::guestSpending
                                : choice == 4 ? Page::guestInventory
                                              : Page::guestRoster);
                    break;
                case Page::guestNeeds:
                case Page::guestSpending:
                    page(
                        _guestTarget && Handheld::GuestManagement::Resolve(*_guestTarget) ? Page::guestInspect
                                                                                          : Page::guestRoster);
                    break;
                case Page::guestThoughts:
                case Page::guestInventory:
                    if (_guestTarget)
                    {
                        const auto* guest = Handheld::GuestManagement::Resolve(*_guestTarget);
                        if (!guest)
                        {
                            page(Page::guestRoster);
                            break;
                        }
                        const auto values = _page == Page::guestThoughts ? Handheld::GuestManagement::Thoughts(*guest)
                                                                         : Handheld::GuestManagement::Inventory(*guest);
                        if (choice >= values.size())
                            page(Page::guestInspect);
                        else
                        {
                            _guestText = values[choice];
                            _guestTextParent = _page;
                            _guestTextPage = 0;
                            page(Page::guestText);
                        }
                    }
                    else
                        page(Page::guestRoster);
                    break;
                case Page::guestText:
                    if (!_guestTarget || !Handheld::GuestManagement::Resolve(*_guestTarget))
                    {
                        page(Page::guestRoster);
                        break;
                    }
                    if (rows().size() == 1 || choice == 2)
                        page(_guestTextParent);
                    else
                    {
                        const auto pages = std::max<size_t>(
                            1, (Handheld::GuestManagement::TextLines(_guestText).size() + 5) / 6);
                        if (choice == 0 && _guestTextPage)
                            --_guestTextPage;
                        if (choice == 1 && _guestTextPage + 1 < pages)
                            ++_guestTextPage;
                        invalidate();
                    }
                    break;
                case Page::marketing:
                    if (choice == 0)
                    {
                        if (!Handheld::MarketingManagement::Types().empty())
                            page(Page::marketingTypes);
                    }
                    else if (choice == 1)
                        page(Page::dashboard);
                    else if (choice - 2 < _marketingActiveTypes.size())
                    {
                        _marketingActiveType = _marketingActiveTypes[choice - 2];
                        page(Page::marketingActive);
                    }
                    break;
                case Page::marketingActive:
                    page(Page::marketing);
                    break;
                case Page::marketingTypes:
                    if (choice >= _marketingTypes.size())
                        page(Page::marketing);
                    else if (Handheld::MarketingManagement::Eligible(_marketingTypes[choice]))
                    {
                        _marketingDraft = Handheld::MarketingManagement::Draft{ _marketingTypes[choice], 2, {} };
                        page(Page::marketingEditor);
                    }
                    else
                    {
                        _status = "Campaign no longer available";
                        invalidate();
                    }
                    break;
                case Page::marketingEditor:
                    if (_marketingDraft)
                    {
                        const size_t duration = Handheld::MarketingManagement::NeedsTarget(_marketingDraft->type) ? 1 : 0;
                        if (duration && choice == 0)
                            page(Page::marketingTargets);
                        else if (choice == duration)
                        {
                            Handheld::MarketingManagement::AdjustWeeks(
                                *_marketingDraft, _marketingDraft->weeks == 12 ? -10 : 1);
                            _status.clear();
                            invalidate();
                        }
                        else if (choice == duration + 2)
                            page(Page::marketing);
                        else if (choice == duration + 1)
                        {
                            const auto result = Handheld::MarketingManagement::Action(*_marketingDraft, false);
                            if (result.error == GameActions::Status::ok)
                                page(Page::marketingConfirm);
                            else
                            {
                                _status = Handheld::Finances::Feedback(result);
                                invalidate();
                            }
                        }
                    }
                    break;
                case Page::marketingTargets:
                    if (choice >= _marketingTargets.size())
                        page(Page::marketingEditor);
                    else if (
                        _marketingDraft
                        && Handheld::MarketingManagement::SelectTarget(*_marketingDraft, _marketingTargets[choice]))
                        page(Page::marketingEditor);
                    else
                    {
                        _status = "Target changed; choose again";
                        invalidate();
                    }
                    break;
                case Page::marketingConfirm:
                    if (choice == 1)
                        page(Page::marketing);
                    else if (_marketingDraft)
                    {
                        _marketingBusy = true;
                        const auto result = Handheld::MarketingManagement::Action(
                            *_marketingDraft, true,
                            [this, lifetime = std::weak_ptr<int>(_marketingLifetime)](
                                const GameActions::GameAction*, const GameActions::Result* outcome) {
                                if (lifetime.expired())
                                    return;
                                _marketingBusy = false;
                                if (_page == Page::marketing)
                                {
                                    page(Page::marketing);
                                    _status = outcome->error == GameActions::Status::ok
                                        ? "Campaign started"
                                        : Handheld::Finances::Feedback(*outcome);
                                    invalidate();
                                }
                            });
                        if (result.error == GameActions::Status::ok)
                        {
                            page(Page::marketing);
                            _status = _marketingBusy ? "Campaign start requested" : "Campaign started";
                        }
                        else
                        {
                            _marketingBusy = false;
                            _status = Handheld::Finances::Feedback(result);
                        }
                        invalidate();
                    }
                    break;
                case Page::research:
                    if (choice == 1)
                        page(Page::dashboard);
                    else if (
                        Handheld::ResearchManagement::FundingAvailable() || Handheld::ResearchManagement::AvailableCategories())
                    {
                        _researchDraft = Handheld::ResearchManagement::Begin();
                        page(Page::researchEditor);
                    }
                    break;
                case Page::researchEditor:
                    if (choice == 9)
                        page(Page::research);
                    else if (_researchDraft)
                    {
                        if (choice == 0)
                        {
                            Handheld::ResearchManagement::AdjustFunding(*_researchDraft, _researchDraft->funding == 3 ? -3 : 1);
                            _status.clear();
                        }
                        else if (choice < 8)
                        {
                            Handheld::ResearchManagement::ToggleCategory(*_researchDraft, choice - 1);
                            _status.clear();
                        }
                        else
                        {
                            const auto result = Handheld::ResearchManagement::Action(*_researchDraft, false);
                            if (result.error == GameActions::Status::ok)
                                page(Page::researchConfirm);
                            else
                                _status = Handheld::Finances::Feedback(result);
                        }
                        invalidate();
                    }
                    break;
                case Page::researchConfirm:
                    if (choice == 8)
                        page(Page::research);
                    else if (choice == 7 && _researchDraft)
                    {
                        const auto result = Handheld::ResearchManagement::Action(*_researchDraft, true);
                        if (result.error == GameActions::Status::ok)
                        {
                            page(Page::research);
                            _status = "Research settings requested";
                        }
                        else
                            _status = Handheld::Finances::Feedback(result);
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
                    else if (choice == 3)
                    {
                        _ridesTarget = Handheld::RideManagement::Identify(_shop->id, true);
                        if (_ridesTarget)
                            _ridesDraft = Handheld::RideManagement::BeginName(*_ridesTarget);
                        if (_ridesDraft)
                        {
                            _nameEntry = { _ridesDraft->name };
                            page(Page::ridesName);
                        }
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
                    if (_ridesTarget && Handheld::CustomTracks::CanResume(*_ridesTarget)
                        && choice == (Handheld::RideAccess::Supported(*_ridesTarget) ? 4u : 3u))
                    {
                        const auto target = *_ridesTarget;
                        close();
                        Handheld::ResumeCustomTrack(target.id);
                    }
                    else if (choice == 3 && _ridesTarget && Handheld::RideAccess::Supported(*_ridesTarget))
                        openRideAccess(*_ridesTarget);
                    else if (choice == 2)
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
                    if (_dashboard.section == Handheld::Dashboard::Section::overview)
                        model = Handheld::ParkOverview::Presentation(0);
                    else if (_dashboard.section == Handheld::Dashboard::Section::staff)
                    {
                        const auto resume = _sectionResume[_dashboard.index()];
                        if (resume.page && *resume.page != Page::staff)
                            model = staffPresentationFor(*resume.page, resume.row);
                        else
                            model = Handheld::StaffManagement::Presentation(
                                Handheld::StaffManagement::View::roster, resume.row, Handheld::StaffManagement::Roster());
                    }
                    else if (_dashboard.section == Handheld::Dashboard::Section::rides)
                    {
                        const auto resume = _sectionResume[_dashboard.index()];
                        model = resume.page && *resume.page != Page::ridesRoster
                            ? ridesPresentationFor(*resume.page, resume.row)
                            : Handheld::RideManagement::Presentation(
                                  Handheld::RideManagement::View::roster, resume.row, Handheld::RideManagement::Roster());
                    }
                    else if (_dashboard.section == Handheld::Dashboard::Section::guests)
                    {
                        const auto resume = _sectionResume[_dashboard.index()];
                        model = resume.page
                            ? guestPresentationFor(*resume.page, resume.row)
                            : Handheld::GuestManagement::Presentation(Handheld::GuestManagement::View::overview, 0);
                    }
                    else if (_dashboard.section == Handheld::Dashboard::Section::finances)
                        model = Handheld::Finances::Presentation(Handheld::Finances::View::overview, 0);
                    else if (_dashboard.section == Handheld::Dashboard::Section::marketing)
                    {
                        const auto resume = _sectionResume[_dashboard.index()];
                        model = resume.page && *resume.page == Page::marketingActive
                            ? marketingPresentationFor(Page::marketingActive, resume.row)
                            : Handheld::MarketingManagement::Presentation(
                                  Handheld::MarketingManagement::View::overview, 0, {}, -1,
                                  Handheld::MarketingManagement::ActiveTypes());
                    }
                    else if (_dashboard.section == Handheld::Dashboard::Section::research)
                        model = Handheld::ResearchManagement::Presentation(Handheld::ResearchManagement::View::overview, 0);
                    else
                        model = Handheld::Dashboard::Unavailable(_dashboard.section);
                    if (!_status.empty())
                        model.status = _status;
                }
                else if (overviewManagement())
                    model = overviewPresentation();
                else if (ridesManagement())
                    model = ridesPresentationFor(_page, _focus.index());
                else if (guestManagement())
                    model = guestsPresentation();
                else if (staffManagement())
                    model = staffPresentation();
                else if (researchManagement())
                    model = researchPresentation();
                else if (marketingManagement())
                    model = marketingPresentationFor(_page, _focus.index());
                else
                {
                    using Handheld::Finances::View;
                    model = Handheld::Finances::Presentation(
                        _page == Page::finances            ? View::overview
                            : _page == Page::financeEditor ? View::editor
                                                           : View::confirmation,
                        _focus.index(), _financeDraft, _status);
                }
                Handheld::Dashboard::Draw(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _dashboard, model, dashboardCanSwitch());
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
            if (_page == Page::trackPiecePicker)
            {
                Handheld::DrawObjectPicker(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Track pieces", _status, "No compatible pieces");
                return;
            }
            if (_page == Page::designPicker)
            {
                Handheld::DrawObjectPicker(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Wooden designs", _status,
                    "No supported designs installed");
                return;
            }
            if (_page == Page::ridePicker)
            {
                Handheld::DrawObjectPicker(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Rides", _status, "No rides available");
                return;
            }
            if (_page == Page::shopPicker)
            {
                Handheld::DrawObjectPicker(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Shops / Facilities", _status,
                    "No shops available");
                return;
            }
            if (_page == Page::rightsConfirm && _rightsDraft)
            {
                Handheld::DrawManagementPanel(
                    rt, { ContextGetWidth(), ContextGetHeight() },
                    Handheld::RightsTools::Presentation(*_rightsDraft, _focus.index(), _status));
                return;
            }
            if (_page == Page::sceneryName || _page == Page::sceneryConfirm)
            {
                Handheld::ManagementPresentation model;
                model.breadcrumb = "Scenery / Sign text";
                model.title = _nameEntry.text.empty() ? "Empty text" : _nameEntry.text;
                model.rows = rows();
                model.selected = _focus.index();
                model.columns = _page == Page::sceneryName ? Handheld::TextEntry::columns : 1;
                model.confirmation = _page == Page::sceneryConfirm;
                model.status = _status.empty() ? "32 characters / native sign action" : _status;
                Handheld::DrawManagementContent(
                    rt, Handheld::Concept::kMenuOrigin,
                    { 324, Handheld::Layout::ContentHeight({ ContextGetWidth(), ContextGetHeight() }) }, model);
                Handheld::DrawActionStrip(rt, { ContextGetWidth(), ContextGetHeight() });
                return;
            }
            if (_page == Page::sceneryPicker)
            {
                Handheld::DrawObjectPicker(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker,
                    Handheld::SceneryTools::CategoryName(Handheld::SceneryTools::Current().category), _status);
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
                case Page::customChoice:
                    title = "WOODEN COASTER";
                    break;
                case Page::designConfirm:
                    title = "PLACE DESIGN?";
                    break;
                case Page::rightsOptions:
                    title = "LAND / RIGHTS OPTIONS";
                    break;
                case Page::areaOptions:
                    title = "AREA BRUSH OPTIONS";
                    break;
                case Page::sceneryCategory:
                    title = "SCENERY CATEGORY";
                    break;
                case Page::sceneryOptions:
                    title = "SCENERY OPTIONS";
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
    void HandheldRideAccessOpen(const Handheld::RideManagement::Target& target)
    {
        if (auto* window = OpenPanel())
            window->openRideAccess(target);
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
    void HandheldDesignPickerOpen()
    {
        if (auto* window = OpenPanel())
            window->openDesignPicker(!Handheld::Designs::Active());
    }
    void HandheldDesignReviewPause()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->pauseDesignReview();
    }
    void HandheldDesignReviewOpen()
    {
        if (auto* window = OpenPanel())
            window->openDesignReview();
    }
    void HandheldRidePickerOpen()
    {
        if (auto* window = OpenPanel())
            window->openRidePicker(Handheld::FlatRides::CurrentStage() != Handheld::FlatRides::Stage::ride);
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
    bool HandheldConstructionMenuIsOpen()
    {
        const auto* w = static_cast<HandheldMenuWindow*>(GetWindowManager()->FindByClass(WindowClass::handheldMenu));
        return w && w->constructionMenu();
    }
    void HandheldConstructionMove(int x, int y)
    {
        if (auto* w = static_cast<HandheldMenuWindow*>(GetWindowManager()->FindByClass(WindowClass::handheldMenu)))
            w->constructionMove(x, y);
    }
    void HandheldRightsOptionsOpen()
    {
        if (auto* w = OpenPanel())
            w->openRightsOptions();
    }
    void HandheldRightsReviewOpen(CoordsXY tile)
    {
        if (auto* w = OpenPanel())
            w->openRightsReview(tile);
    }
    void HandheldAreaOptionsOpen()
    {
        if (auto* w = OpenPanel())
            w->openAreaOptions();
    }
    void HandheldSceneryOptionsOpen()
    {
        if (auto* window = OpenPanel())
            window->openSceneryOptions();
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

namespace OpenRCT2::Ui::Windows
{
    void HandheldDashboardPause()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->dashboardPause();
    }
    void HandheldDashboardOpen()
    {
        if (auto* window = OpenPanel())
            window->openDashboard();
    }
    bool HandheldDashboardIsOpen()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->dashboardIsOpen();
    }
    bool HandheldDashboardRailFocused()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->dashboardRailFocused();
    }
    void HandheldDashboardMove(int x, int y)
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->dashboardMove(x, y);
    }
    void HandheldDashboardSwitchPane()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->dashboardSwitchPane();
    }
} // namespace OpenRCT2::Ui::Windows

namespace OpenRCT2::Ui::Windows
{
    size_t HandheldDashboardSectionIndex()
    {
        auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window ? static_cast<const HandheldMenuWindow*>(window)->dashboardSectionIndex() : 0;
    }
    bool HandheldOverviewIsOpen()
    {
        auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->overviewIsOpen();
    }
    Handheld::ManagementPresentation HandheldOverviewPresentation()
    {
        auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window ? static_cast<const HandheldMenuWindow*>(window)->overviewModel() : Handheld::ManagementPresentation{};
    }
    bool HandheldRidesIsOpen()
    {
        auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->ridesIsOpen();
    }
    Handheld::ManagementPresentation HandheldRidesPresentation()
    {
        auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window ? static_cast<const HandheldMenuWindow*>(window)->ridesPresentation()
                      : Handheld::ManagementPresentation{};
    }
    bool HandheldGuestsIsOpen()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->guestsIsOpen();
    }
    Handheld::ManagementPresentation HandheldGuestsPresentation()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window ? static_cast<const HandheldMenuWindow*>(window)->guestsPresentation()
                      : Handheld::ManagementPresentation{};
    }
} // namespace OpenRCT2::Ui::Windows

namespace OpenRCT2::Ui::Windows
{
    void HandheldTrackPiecePickerOpen()
    {
        if (auto* window = OpenPanel())
            window->openTrackPiecePicker();
    }
} // namespace OpenRCT2::Ui::Windows

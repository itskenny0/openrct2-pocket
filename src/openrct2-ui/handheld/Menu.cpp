// SPDX-License-Identifier: GPL-3.0-only
#include "AreaTools.h"
#include "Currency.h"
#include "CustomTracks.h"
#include "Dashboard.h"
#include "Designs.h"
#include "DesignAuthoring.h"
#include "Explorer.h"
#include "Finances.h"
#include "FlatRides.h"
#include "GuestManagement.h"
#include "ListFocus.h"
#include "MarketingManagement.h"
#include "Mazes.h"
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
#include "StaffPatrol.h"
#include "TextEntry.h"
#include "TrackEditing.h"
#include "TreePicker.h"
#include "Trees.h"

#include <filesystem>
#include <iomanip>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/input/ShortcutIds.h>
#include <openrct2-ui/input/ShortcutManager.h>
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
        designOptions,
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
        staffRoles,
        staffCostumes,
        staffName,
        staffNameConfirm,
        staffSummary,
        staffUniform,
        staffPatrolOptions,
        staffPatrolClear,
        guests,
        guestRoster,
        guestInspect,
        guestNeeds,
        guestThoughts,
        guestSpending,
        guestInventory,
        guestText,
        guestGroups,
        guestName,
        guestNameConfirm,
        guestTracking,
        guestPreferences,
        peepOptions,
        peepCancel,
        ridesRoster,
        ridesInspect,
        trackBlocks,
        trackBlock,
        trackRemoveConfirm,
        trackReplaceConfirm,
        trackSpeedConfirm,
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
        designName,
        designScenery,
        designExport,
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
        ride_type_t _customType = kRideTypeNull;
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
        size_t _guestRosterFocus{}, _guestGroupFocus{};
        Handheld::GuestManagement::GroupMode _guestGroupMode{};
        std::vector<Handheld::GuestManagement::Group> _guestGroups;
        Handheld::GuestManagement::Filter _guestFilter;
        Page _guestRosterParent = Page::guests;
        Page _guestTextParent = Page::guestThoughts;
        std::string _guestNameBaseline;
        PeepFlags _guestFlagsBaseline{};
        bool _guestTrackingValue{};
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
        std::vector<Handheld::TrackEditing::Block> _trackBlocks;
        std::optional<Handheld::TrackEditing::Block> _trackBlock, _trackPrevious;
        money64 _trackCost{};
        uint8_t _trackSpeed{};
        bool trackManagement() const
        {
            return _page == Page::trackBlocks || _page == Page::trackBlock || _page == Page::trackRemoveConfirm
                || _page == Page::trackSpeedConfirm || _page == Page::trackReplaceConfirm;
        }
        bool ridesManagement() const
        {
            return trackManagement() || _page == Page::ridesRoster || _page == Page::ridesInspect || _page == Page::ridesGraphs
                || _page == Page::ridesStations || _page == Page::ridesCustomers || _page == Page::ridesEarnings
                || _page == Page::ridesStats || _page == Page::ridesVehicles || _page == Page::ridesName
                || _page == Page::designName || _page == Page::designScenery || _page == Page::designExport
                || _page == Page::ridesAppearance || _page == Page::ridesMusic || _page == Page::ridesDispatch
                || _page == Page::ridesOperations || _page == Page::ridesMaintenance || _page == Page::ridesAccess
                || _page == Page::ridesPrice || _page == Page::ridesConfirm;
        }
        Handheld::ManagementPresentation ridesPresentationFor(Page current, size_t selected) const
        {
            using Handheld::RideManagement::View;
            if (trackManagement())
            {
                Handheld::ManagementPresentation model;
                model.breadcrumb = "Dashboard / Rides / Track";
                model.title = current == Page::trackBlocks ? "SELECT TRACK BLOCK" : "TRACK BLOCK";
                model.selected = selected;
                model.primaryAction = "Select";
                model.status = _status;
                if (current == Page::trackBlocks)
                {
                    for (const auto& block : _trackBlocks)
                        model.rows.push_back(Handheld::TrackEditing::Label(block));
                    model.rows.push_back("Back to ride");
                }
                else if (_trackBlock && Handheld::TrackEditing::Valid(*_trackBlock))
                {
                    model.stats.emplace_back("Block", Handheld::TrackEditing::Label(*_trackBlock));
                    model.stats.emplace_back("Variant", _trackBlock->inverted ? "Inverted" :
                        TrackConstructionSupport::CoveredBase(_trackBlock->type) ? "Covered" : "Normal");
                    if (TrackConstructionSupport::SeatRotation(_trackBlock->ride.type,_trackBlock->type))
                        model.stats.emplace_back("Seat", std::to_string(_trackBlock->seat));
                    if (trackTypeHasSpeedSetting(_trackBlock->type))
                        model.stats.emplace_back(
                            "Speed", FormatStringID(STR_VELOCITY, static_cast<uint16_t>((_trackSpeed * 9) >> 2)));
                    model.rows = { "Locate block", "Remove block...", "Back to blocks" };
                    if (trackTypeHasSpeedSetting(_trackBlock->type))
                    {
                        model.rows.push_back("Speed -");
                        model.rows.push_back("Speed +");
                        model.rows.push_back("Review speed...");
                    }
                    model.rows.push_back("Continue section...");
                    model.rows.push_back("Build backward...");
                    if (Handheld::TrackEditing::CanReplace(*_trackBlock)) model.rows.push_back("Replace block...");
                    if (current == Page::trackRemoveConfirm || current == Page::trackSpeedConfirm || current == Page::trackReplaceConfirm)
                    {
                        model.confirmation = true;
                        model.title = current == Page::trackReplaceConfirm ? "REMOVE / REBUILD BLOCK?"
                            : current == Page::trackRemoveConfirm ? "REMOVE BLOCK?" : "CHANGE TRACK SPEED?";
                        if (current == Page::trackReplaceConfirm) model.bodyLines = { "Removal leaves a gap.", "Build the replacement explicitly." };
                        model.rows = { "Cancel", "Confirm" };
                        model.status = current != Page::trackSpeedConfirm && _trackCost < 0
                            ? "Native refund " + Handheld::FormatMoney(-_trackCost)
                            : "Native cost " + Handheld::FormatMoney(_trackCost);
                    }
                }
                else
                {
                    model.rows = { "Back to blocks" };
                    model.status = "Block changed; select again";
                }
                return model;
            }
            if (current == Page::designScenery)
            {
                Handheld::ManagementPresentation model;
                model.breadcrumb = "Rides / Save / Scenery";
                model.title = "DESIGN SCENERY";
                model.stats.emplace_back("Selected", std::to_string(Handheld::DesignAuthoring::SceneryCount()));
                model.rows = { "Name / export...", "Select native nearby", "No scenery" };
                for (size_t i = 0; i < Handheld::DesignAuthoring::sceneryBaseline.size(); ++i)
                    model.rows.push_back(Handheld::DesignAuthoring::SceneryLabel(i));
                model.selected = selected;
                model.primaryAction = selected >= 3 ? "Toggle" : "Choose";
                model.status = _status.empty() ? "Native nearby selection; A toggles" : _status;
                return model;
            }
            if (current == Page::designExport)
            {
                Handheld::ManagementPresentation model;
                model.breadcrumb = "Rides / Save design";
                model.title = Handheld::DesignAuthoring::Overwrite() ? "OVERWRITE DESIGN?" : "EXPORT DESIGN?";
                model.bodyLines.push_back("File: " + Handheld::DesignAuthoring::Filename());
                model.bodyLines.push_back("Scenery: " + std::to_string(Handheld::DesignAuthoring::SceneryCount()));
                model.rows = { "Cancel", "Export design" };
                model.confirmation = true;
                model.selected = selected;
                model.status = _status.empty() ? "Native tested / rated design" : _status;
                model.primaryAction = "Choose";
                return model;
            }
            if (current == Page::ridesName || current == Page::designName)
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
        ObjectEntryIndex _costumeBaseline = kObjectEntryIndexNull;
        std::optional<Handheld::StaffPatrol::Draft> _patrolDraft;
        Drawing::Colour _staffUniformBaseline{}, _staffUniformDraft{};
        std::string _staffNameBaseline;
        std::optional<StaffType> _staffFilter;
        bool _hireManual{};
        bool _staffBusy{};
        size_t _staffRosterFocus{};
        std::shared_ptr<int> _staffLifetime = std::make_shared<int>(0);
        uint64_t _staffRequest{};
        bool staffManagement() const
        {
            return _page == Page::staff || _page == Page::staffInspect || _page == Page::staffStats
                || _page == Page::staffOrders || _page == Page::staffHire || _page == Page::staffDismiss
                || _page == Page::staffRoles || _page == Page::staffCostumes || _page == Page::staffName
                || _page == Page::staffNameConfirm || _page == Page::staffSummary || _page == Page::staffUniform;
        }
        Handheld::ManagementPresentation staffPresentationFor(Page currentPage, size_t selected) const
        {
            using Handheld::StaffManagement::View;
            if (currentPage == Page::staffUniform && _staffTarget)
                return Handheld::StaffManagement::UniformPresentation(*_staffTarget, _staffUniformDraft, selected);
            if (currentPage == Page::staffName || currentPage == Page::staffNameConfirm)
                return Handheld::StaffManagement::NamePresentation(
                    _nameEntry, selected, currentPage == Page::staffNameConfirm, _status);
            const auto view = currentPage == Page::staffSummary ? View::summary
                : currentPage == Page::staffRoles               ? View::roles
                : currentPage == Page::staffCostumes            ? View::costumes
                : currentPage == Page::staff                    ? View::roster
                : currentPage == Page::staffInspect             ? View::inspection
                : currentPage == Page::staffStats               ? View::statistics
                : currentPage == Page::staffOrders              ? View::orders
                : currentPage == Page::staffHire                ? View::hire
                                                                : View::dismissal;
            auto model = Handheld::StaffManagement::Presentation(
                view, selected, _staffRoster, _staffTarget, _hireType, _ordersDraft, _status);
            if (currentPage == Page::staffRoles)
                model.rows.push_back(_hireManual ? "Placement: Manual" : "Placement: Automatic");
            if (currentPage == Page::staffHire)
            {
                model.stats.back().second = _hireManual ? "Manual" : "Automatic";
                const auto query = Handheld::StaffManagement::Hire(_hireType, false, {}, _hireManual);
                model.status = query.error == GameActions::Status::ok ? _status : Handheld::Finances::Feedback(query);
            }
            if (currentPage == Page::staff)
            {
                model.rows[_staffRoster.size() + 3] = "Filter: "
                    + (_staffFilter ? Handheld::StaffManagement::RoleName(*_staffFilter) : "All staff");
                if (_staffFilter && _staffRoster.empty() && _status.empty())
                    model.status = "No employees in this role";
            }
            return model;
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
                || _page == Page::guestInventory || _page == Page::guestText || _page == Page::guestGroups
                || _page == Page::guestName || _page == Page::guestNameConfirm || _page == Page::guestTracking
                || _page == Page::guestPreferences;
        }
        Handheld::ManagementPresentation guestPresentationFor(Page current, size_t selected) const
        {
            using Handheld::GuestManagement::View;
            if (current == Page::guestName || current == Page::guestNameConfirm)
            {
                auto model = Handheld::StaffManagement::NamePresentation(
                    _nameEntry, selected, current == Page::guestNameConfirm, _status);
                model.breadcrumb = current == Page::guestName ? "Guests / Name keyboard" : "Guests / Review name";
                return model;
            }
            if (current == Page::guestTracking && _guestTarget)
                return Handheld::GuestManagement::TrackingPresentation(*_guestTarget, _guestTrackingValue, selected);
            if (current == Page::guestGroups)
                return Handheld::GuestManagement::GroupPresentation(_guestGroupMode, _guestGroups, selected);
            const auto view = current == Page::guests ? View::overview
                : current == Page::guestRoster        ? View::roster
                : current == Page::guestPreferences   ? View::preferences
                : current == Page::guestInspect       ? View::inspection
                : current == Page::guestNeeds         ? View::needs
                : current == Page::guestThoughts      ? View::thoughts
                : current == Page::guestSpending      ? View::spending
                : current == Page::guestInventory     ? View::inventory
                                                      : View::text;
            auto model = Handheld::GuestManagement::Presentation(
                view, selected, _guestRoster, _guestTarget, _status, _guestText, _guestTextPage);
            if (current == Page::guestRoster && (_guestFilter.group || _guestFilter.tracked))
            {
                model.title = _guestFilter.tracked ? "Tracked guests" : "Filtered guests";
                model.rows.back() = _guestRosterParent == Page::guestGroups ? "Back to groups" : "Back to Guests";
                model.status = std::to_string(_guestRoster.size()) + " guests / "
                    + (_guestFilter.group ? _guestFilter.group->label : "Tracking enabled");
            }
            return model;
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
            return _page == Page::peepOptions || _page == Page::peepCancel || _page == Page::staffPatrolOptions
                || _page == Page::staffPatrolClear || _page == Page::rightsOptions || _page == Page::rightsConfirm
                || _page == Page::areaOptions || _page == Page::sceneryOptions || _page == Page::sceneryName
                || _page == Page::sceneryConfirm;
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
            else if (x && _page == Page::staffPatrolOptions)
            {
                Handheld::StaffPatrol::Configure(_focus.index(), x);
                invalidate();
            }
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
            const ScreenSize bounds = (_page == Page::staffPatrolClear || _page == Page::rightsConfirm
                                       || _page == Page::sceneryName || _page == Page::sceneryConfirm
                                       || _page == Page::pathOptions || picker() || shopManagement() || managementShell())
                ? ScreenSize{ ContextGetWidth(), ContextGetHeight() }
                : information() ? ScreenSize{ 330, _page == Page::ride ? 340 : 310 }
                                : target;
            WindowSetResize(*this, bounds, bounds);
            WindowSetPosition(
                *this,
                (_page == Page::staffPatrolClear || _page == Page::rightsConfirm || _page == Page::sceneryName
                 || _page == Page::sceneryConfirm || _page == Page::pathOptions || picker() || shopManagement()
                 || managementShell())
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
            if (next == Page::guestGroups)
                _guestGroups = Handheld::GuestManagement::Groups(_guestGroupMode);
            if (next == Page::guestRoster)
                _guestRoster = Handheld::GuestManagement::Roster(_guestFilter);
            if (next == Page::staff)
                _staffRoster = Handheld::StaffManagement::Roster(_staffFilter);
            if (next == Page::marketing)
                _marketingActiveTypes = Handheld::MarketingManagement::ActiveTypes();
            if (next == Page::marketingTypes)
                _marketingTypes = Handheld::MarketingManagement::Types();
            if (next == Page::marketingTargets && _marketingDraft)
                _marketingTargets = Handheld::MarketingManagement::Targets(_marketingDraft->type);
            if (next != Page::marketingEditor && next != Page::marketingTargets && next != Page::marketingConfirm)
                _marketingDraft.reset();
            if ((_page == Page::designName || _page == Page::designScenery || _page == Page::designExport)
                && next != Page::designName && next != Page::designScenery && next != Page::designExport)
                Handheld::DesignAuthoring::End();
            _page = next;
            if (next == Page::ridesName || next == Page::designName || next == Page::designScenery || next == Page::designExport)
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
            if (next == Page::guestGroups)
                _focus.move(static_cast<int32_t>(std::min(_guestGroupFocus, rows().size() - 1)), rows().size());
            if (next == Page::guestRoster)
                _focus.move(static_cast<int32_t>(std::min(_guestRosterFocus, rows().size() - 1)), rows().size());
            if (next == Page::overwrite || next == Page::discard || next == Page::exit || next == Page::shopDemolish
                || next == Page::overviewConfirm || next == Page::ridesConfirm || next == Page::financeConfirm
                || next == Page::marketingConfirm || next == Page::staffHire || next == Page::staffDismiss
                || next == Page::staffNameConfirm || next == Page::guestNameConfirm || next == Page::guestTracking
                || next == Page::peepCancel)
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
                case Page::staffRoles:
                case Page::staffCostumes:
                case Page::staffName:
                case Page::staffNameConfirm:
                case Page::staffSummary:
                case Page::staffUniform:
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
                case Page::trackBlocks:
                case Page::trackBlock:
                case Page::trackRemoveConfirm:
                case Page::trackReplaceConfirm:
                case Page::trackSpeedConfirm:
                case Page::ridesRoster:
                case Page::ridesInspect:
                case Page::ridesAccess:
                case Page::ridesVehicles:
                case Page::ridesName:
                case Page::designName:
                case Page::designScenery:
                case Page::designExport:
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
                case Page::guestGroups:
                case Page::guestName:
                case Page::guestNameConfirm:
                case Page::guestTracking:
                case Page::guestPreferences:
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
                    if (_ridesTarget && Handheld::Mazes::CanResume(*_ridesTarget))
                        result.push_back("Edit maze...");
                    if (_ridesTarget && Handheld::TrackEditing::Eligible(*_ridesTarget))
                        result.push_back("Edit track blocks...");
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
                             "Saved designs",
                             "Scenery...",
                             "Terrain / Water...",
                             "Land / Rights...",
                             "Back to park" };
                case Page::sceneryCategory:
                    return Handheld::SceneryTools::Categories();
                case Page::peepOptions:
                    return { "Resume placement", "Cancel placement...", "Back to person" };
                case Page::peepCancel:
                    return Handheld::PeepPlacement::CancelPresentation(_focus.index()).rows;
                case Page::staffPatrolOptions:
                    return Handheld::StaffPatrol::Options();
                case Page::staffPatrolClear:
                    return { "Clear patrol", "Cancel" };
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
                case Page::designOptions:
                    return { "Height -",    "Height +",      "Automatic height", "Scenery on",
                             "Scenery off", "Rotate camera", "Back to placement" };
                case Page::designConfirm:
                    return { "Cancel", "Place design" };
                case Page::trackPiecePicker:
                case Page::designPicker:
                case Page::treePicker:
                case Page::shopPicker:
                case Page::ridePicker:
                    return {};
                case Page::paths:
                    return { "Normal path", "Queue path", "Remove path", "Back to build", "Connected path", "Connected queue" };
                case Page::pathOptions:
                    if (Handheld::Paths::IsConnected())
                        return { "Resume", "Turn direction", "Rise", "Descend", "Flat", "Restart endpoint", "Change surface" };
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
            Handheld::StaffPatrol::HideOverlay();
            Drawing::pickupPeepClear();
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
            Handheld::DesignAuthoring::End();
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
            auto mazes = Handheld::Mazes::Entries();
            rideEntries.insert(rideEntries.end(), mazes.begin(), mazes.end());
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
        void openDesignOptions()
        {
            page(Page::designOptions);
            _status = Handheld::Designs::HeightNotice() + " / " + Handheld::Designs::SceneryNotice();
        }
        void openDesignReview()
        {
            page(Page::designConfirm);
            _status = "Cost " + Handheld::FormatMoney(Handheld::Designs::ReviewCost()) + " / "
                + (Handheld::Designs::HeightOffset() ? Handheld::Designs::HeightNotice() : Handheld::Designs::SceneryNotice());
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
            if (_page == Page::guestGroups)
            {
                const auto updated = Handheld::GuestManagement::Groups(_guestGroupMode);
                if (updated != _guestGroups)
                {
                    size_t row = std::min(_focus.index(), updated.size());
                    if (_focus.index() < _guestGroups.size())
                    {
                        const auto key = _guestGroups[_focus.index()].arguments;
                        const auto found = std::find_if(
                            updated.begin(), updated.end(), [&](const auto& g) { return g.arguments == key; });
                        if (found != updated.end())
                            row = std::distance(updated.begin(), found);
                    }
                    _guestGroups = updated;
                    _focus = {};
                    _focus.move(static_cast<int>(row), rows().size());
                    invalidate();
                }
            }
            if (_page == Page::guestRoster)
            {
                const auto updated = Handheld::GuestManagement::Roster(_guestFilter);
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
                const auto updated = Handheld::StaffManagement::Roster(_staffFilter);
                if (updated != _staffRoster)
                {
                    std::optional<Handheld::StaffManagement::Target> selected;
                    if (_focus.index() >= 2 && _focus.index() - 2 < _staffRoster.size())
                        selected = _staffRoster[_focus.index() - 2];
                    size_t newFocus = std::min(_focus.index(), updated.size() + 5);
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
                || _page == Page::marketing || _page == Page::marketingActive
                || (guestManagement() && _page != Page::guestName && _page != Page::guestNameConfirm
                    && _page != Page::guestTracking)
                || _page == Page::ridesRoster || _page == Page::ridesInspect || _page == Page::ridesAccess
                || _page == Page::ridesGraphs || _page == Page::ridesStations || _page == Page::ridesCustomers
                || _page == Page::ridesEarnings || _page == Page::ridesStats || _page == Page::ridesVehicles
                || _page == Page::ridesAppearance || _page == Page::ridesMusic || _page == Page::ridesDispatch
                || _page == Page::ridesOperations || _page == Page::ridesMaintenance;
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
            else if (_page == Page::staffUniform)
            {
                if (y)
                    move(y);
                if (x && _focus.index() == 0)
                    _staffUniformDraft = static_cast<Drawing::Colour>((EnumValue(_staffUniformDraft) + x + 72) % 72);
                invalidate();
            }
            else if (_page == Page::ridesName || _page == Page::designName || _page == Page::staffName || _page == Page::guestName)
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
        void openStaffPatrolOptions()
        {
            page(Page::staffPatrolOptions);
        }
        void openGuest(const Handheld::GuestManagement::Target& person)
        {
            _guestTarget = person;
            _dashboard.section = Handheld::Dashboard::Section::guests;
            _dashboard.focus = Handheld::Dashboard::Focus::content;
            page(Page::guestInspect);
        }
        void openPeepOptions()
        {
            page(Page::peepOptions);
        }
        void openStaff(std::optional<Handheld::StaffManagement::Target> target)
        {
            _staffTarget = target;
            if (!target)
                _staffRosterFocus = 0;
            _dashboard.focus = Handheld::Dashboard::Focus::content;
            page(target ? Page::staffInspect : Page::staff);
        }
        Handheld::ManagementPresentation staffModel() const
        {
            return staffPresentation();
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
                if (_hireManual && _staffTarget)
                {
                    const auto employee = *_staffTarget;
                    close();
                    Handheld::BeginPeepPlacement({ {}, employee });
                    return;
                }
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
                case Page::staffPatrolClear:
                    page(Page::staffPatrolOptions);
                    break;
                case Page::peepCancel:
                    page(Page::peepOptions);
                    break;
                case Page::peepOptions:
                    close();
                    return;
                case Page::staffPatrolOptions:
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
                case Page::designOptions:
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
                case Page::trackBlocks:
                    page(Page::ridesInspect);
                    break;
                case Page::trackBlock:
                    if (_ridesTarget)
                        _trackBlocks = Handheld::TrackEditing::Blocks(*_ridesTarget);
                    page(Page::trackBlocks);
                    break;
                case Page::trackRemoveConfirm:
                case Page::trackReplaceConfirm:
                case Page::trackSpeedConfirm:
                    page(Page::trackBlock);
                    break;
                case Page::ridesInspect:
                    page(Page::ridesRoster);
                    break;
                case Page::designExport:
                    Handheld::DesignAuthoring::CancelReview();
                    page(Page::designName);
                    break;
                case Page::designName:
                    page(Page::designScenery);
                    break;
                case Page::designScenery:
                    page(Page::ridesInspect);
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
                case Page::guestGroups:
                    page(Page::guests);
                    break;
                case Page::guestRoster:
                    page(_guestRosterParent);
                    break;
                case Page::guestInspect:
                    page(Page::guestRoster);
                    break;
                case Page::guestText:
                    page(
                        _guestTarget && Handheld::GuestManagement::Resolve(*_guestTarget) ? _guestTextParent
                                                                                          : Page::guestRoster);
                    break;
                case Page::guestNameConfirm:
                    page(Page::guestName);
                    break;
                case Page::guestName:
                case Page::guestTracking:
                case Page::guestPreferences:
                case Page::guestNeeds:
                case Page::guestThoughts:
                case Page::guestSpending:
                case Page::guestInventory:
                    page(
                        _guestTarget && Handheld::GuestManagement::Resolve(*_guestTarget) ? Page::guestInspect
                                                                                          : Page::guestRoster);
                    break;
                case Page::staffRoles:
                case Page::staffHire:
                case Page::staffInspect:
                    page(Page::staff);
                    break;
                case Page::staffNameConfirm:
                    page(Page::staffName);
                    break;
                case Page::staffSummary:
                    page(Page::staff);
                    break;
                case Page::staffUniform:
                case Page::staffName:
                case Page::staffStats:
                case Page::staffOrders:
                case Page::staffCostumes:
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
                case Page::designOptions:
                    if (choice < 2)
                        Handheld::Designs::Height(choice == 1 ? 1 : -1);
                    else if (choice == 2)
                        Handheld::Designs::AutomaticHeight();
                    else if (choice < 5)
                        Handheld::Designs::SetScenery(choice == 3);
                    else if (choice == 5)
                    {
                        if (auto* shortcut = GetShortcutManager().getShortcut(ShortcutId::kViewGeneralRotateClockwise))
                            shortcut->action();
                    }
                    else
                    {
                        close();
                        break;
                    }
                    _status = Handheld::Designs::HeightNotice() + " / " + Handheld::Designs::SceneryNotice();
                    invalidate();
                    break;
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
                        if (Handheld::BeginCustomTrack(_customObject, _customType))
                            close();
                        else
                        {
                            _status = "Ride construction unavailable";
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
                        else if (key == "camera")
                        {
                            Handheld::CustomTracks::ToggleCamera();
                            close();
                        }
                        else if (key == "covered" || key == "inverted")
                        {
                            const auto ok=key=="covered" ? Handheld::CustomTracks::ToggleCovered() : Handheld::CustomTracks::ToggleInverted();
                            if (ok) close(); else { _status="Variant cannot connect here"; invalidate(); }
                        }
                        else if (key.starts_with("seat-"))
                        {
                            const auto currentSeat = Handheld::CustomTracks::PlacementSeat();
                            const auto value = key == "seat-reset" ? 4 : key == "seat-minus" ? std::max<int>(0, currentSeat - 1)
                                : std::min<int>(15, currentSeat + 1);
                            if (Handheld::CustomTracks::SetPlacementSeat(value)) close();
                            else { _status = "Seat rotation unavailable"; invalidate(); }
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
                    if (_picker.selected() && _picker.selected()->identity.key.starts_with("maze:"))
                    {
                        if (Handheld::BeginMaze(_picker.selected()->identity.index))
                            close();
                        else
                        {
                            refreshPicker();
                            _status = "Maze unavailable";
                            invalidate();
                        }
                        break;
                    }
                    if (_picker.selected()
                        && (_picker.selected()->identity.key.starts_with("custom:")
                            || _picker.selected()->identity.key.starts_with("custom-type:")))
                    {
                        _customObject = _picker.selected()->identity.index;
                        const auto key = _picker.selected()->identity.key;
                        _customType = key.starts_with("custom-type:")
                            ? static_cast<ride_type_t>(std::stoi(key.substr(12)))
                            : static_cast<ride_type_t>(RIDE_TYPE_WOODEN_ROLLER_COASTER);
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
                    if (choice == 5)
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
                case Page::peepOptions:
                    if (choice == 1)
                        page(Page::peepCancel);
                    else if (choice == 2)
                    {
                        const auto person = Handheld::PeepPlacement::target;
                        close();
                        Handheld::EndTerrain();
                        if (person && person->guest)
                            HandheldGuestOpen(*person->guest);
                        else if (person && person->staff)
                            HandheldStaffOpen(*person->staff);
                        return;
                    }
                    else
                        close();
                    break;
                case Page::peepCancel:
                    if (choice == 1)
                        page(Page::peepOptions);
                    else
                    {
                        close();
                        Handheld::EndTerrain();
                    }
                    break;
                case Page::staffPatrolOptions:
                    if (choice == 0)
                        close();
                    else if (choice == 5)
                    {
                        auto employee = Handheld::StaffPatrol::target;
                        close();
                        Handheld::EndTerrain();
                        if (employee)
                            HandheldStaffOpen(employee);
                    }
                    else if (choice == 4)
                    {
                        _patrolDraft = Handheld::StaffPatrol::ReviewClear();
                        if (_patrolDraft)
                        {
                            page(Page::staffPatrolClear);
                            _focus.move(1, rows().size());
                        }
                        else
                            _status = "No patrol tiles to clear";
                    }
                    else
                    {
                        Handheld::StaffPatrol::Configure(choice);
                        invalidate();
                    }
                    break;
                case Page::staffPatrolClear:
                    if (choice == 1)
                        page(Page::staffPatrolOptions);
                    else if (_patrolDraft)
                    {
                        auto result = Handheld::StaffPatrol::Clear(*_patrolDraft, true);
                        if (result.error == GameActions::Status::ok)
                            page(Page::staffPatrolOptions);
                        else
                        {
                            _status = result.getErrorMessage();
                            invalidate();
                        }
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
                        if (choice >= 4)
                            Handheld::Paths::BeginConnected(choice == 5);
                        else
                            Handheld::Paths::Begin(choice == 1, choice == 2);
                    }
                    break;
                case Page::pathOptions:
                {
                    namespace Paths = Handheld::Paths;
                    bool changed = true;
                    if (Paths::IsConnected())
                    {
                        if (choice == 0)
                            close();
                        else if (choice == 1)
                            Paths::TurnConnected();
                        else if (choice == 2 || choice == 3)
                            Paths::SlopeConnected(choice == 2 ? 1 : -1);
                        else if (choice == 4)
                        {
                            Paths::SlopeConnected(1);
                            Paths::SlopeConnected(1);
                            Paths::SlopeConnected(-1);
                        }
                        else if (choice == 5)
                            Paths::BeginConnected(Paths::IsQueue());
                        else
                            Paths::CycleSurface();
                        invalidate();
                        break;
                    }
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
                    else if (choice == _staffRoster.size() + 2)
                        page(Page::staffRoles);
                    else if (choice == _staffRoster.size() + 3)
                    {
                        _staffFilter = !_staffFilter ? std::optional<StaffType>(StaffType::handyman)
                            : *_staffFilter == StaffType::entertainer
                            ? std::nullopt
                            : std::optional<StaffType>(static_cast<StaffType>(EnumValue(*_staffFilter) + 1));
                        _staffRosterFocus = 0;
                        page(Page::staff);
                    }
                    else if (choice == _staffRoster.size() + 4)
                        page(Page::staffSummary);
                    else
                        page(Page::dashboard);
                    break;
                case Page::staffRoles:
                    if (choice < 4)
                    {
                        _hireType = static_cast<StaffType>(choice);
                        page(Page::staffHire);
                    }
                    else if (choice == 5)
                        _hireManual = !_hireManual;
                    else
                        page(Page::staff);
                    break;
                case Page::staffCostumes:
                    if (_staffTarget)
                    {
                        const auto entries = Handheld::StaffManagement::Costumes();
                        if (choice >= entries.size())
                            page(Page::staffInspect);
                        else
                        {
                            const auto result = Handheld::StaffManagement::SetCostume(
                                *_staffTarget, _costumeBaseline, entries[choice].first, true);
                            if (result.error == GameActions::Status::ok)
                                page(Page::staffInspect);
                            else
                                _status = Handheld::Finances::Feedback(result);
                        }
                    }
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
                            },
                            _hireManual);
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
                    else if (choice == 9)
                    {
                        const auto person = *_staffTarget;
                        if (!Handheld::StaffManagement::Resolve(person)->canBePickedUp())
                        {
                            _status = "Person cannot be picked up now";
                            break;
                        }
                        close();
                        Handheld::BeginPeepPlacement({ {}, person });
                        return;
                    }
                    else if (choice == 8)
                    {
                        if (_staffTarget->type == StaffType::entertainer)
                            _status = "Entertainers use costume colours";
                        else
                        {
                            _staffUniformBaseline = _staffUniformDraft = StaffGetColour(_staffTarget->type);
                            page(Page::staffUniform);
                        }
                    }
                    else if (choice == 7)
                    {
                        _staffNameBaseline = Handheld::StaffManagement::Resolve(*_staffTarget)->getName();
                        _nameEntry = { _staffNameBaseline };
                        page(Page::staffName);
                    }
                    else if (choice == 6)
                    {
                        const auto employee = *_staffTarget;
                        close();
                        Handheld::BeginStaffPatrol(employee);
                        return;
                    }
                    else if (choice == 4)
                    {
                        _costumeBaseline = Handheld::StaffManagement::Resolve(*_staffTarget)->animationObjectIndex;
                        page(Page::staffCostumes);
                    }
                    else if (choice == 1)
                    {
                        if (Handheld::StaffManagement::Orders(_staffTarget->type).empty())
                        {
                            _status = "This role has no native orders";
                            break;
                        }
                        _ordersBaseline = _ordersDraft = Handheld::StaffManagement::Resolve(*_staffTarget)->staffOrders;
                        page(Page::staffOrders);
                    }
                    else
                        page(choice == 2 ? Page::staffStats : choice == 3 ? Page::staffDismiss : Page::staff);
                    break;
                case Page::staffSummary:
                    page(Page::staff);
                    break;
                case Page::staffUniform:
                    if (choice == 0)
                        _staffUniformDraft = static_cast<Drawing::Colour>((EnumValue(_staffUniformDraft) + 1) % 72);
                    else if (choice == 1 && _staffTarget)
                    {
                        const auto result = Handheld::StaffManagement::SetUniform(
                            *_staffTarget, _staffUniformBaseline, _staffUniformDraft, true);
                        if (result.error == GameActions::Status::ok)
                            page(Page::staffInspect);
                        else
                            _status = Handheld::Finances::Feedback(result);
                    }
                    else
                        page(Page::staffInspect);
                    break;
                case Page::staffName:
                    if (choice == 47)
                        page(Page::staffInspect);
                    else if (choice == 46 && _staffTarget)
                    {
                        const auto result = Handheld::StaffManagement::SetName(
                            *_staffTarget, _staffNameBaseline, _nameEntry.text, false);
                        if (result.error == GameActions::Status::ok)
                            page(Page::staffNameConfirm);
                        else
                            _status = Handheld::Finances::Feedback(result);
                    }
                    else
                        _nameEntry.Press(choice);
                    break;
                case Page::staffNameConfirm:
                    if (choice == 1)
                        page(Page::staffName);
                    else if (_staffTarget)
                    {
                        const auto result = Handheld::StaffManagement::SetName(
                            *_staffTarget, _staffNameBaseline, _nameEntry.text, true);
                        if (result.error == GameActions::Status::ok)
                            page(Page::staffInspect);
                        else
                            _status = Handheld::Finances::Feedback(result);
                    }
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
                case Page::trackBlocks:
                    if (choice < _trackBlocks.size())
                    {
                        _trackBlock = _trackBlocks[choice];
                        _trackSpeed = _trackBlock->speed;
                        page(Page::trackBlock);
                    }
                    else
                        page(Page::ridesInspect);
                    break;
                case Page::trackBlock:
                    if (!_trackBlock || !Handheld::TrackEditing::Valid(*_trackBlock))
                    {
                        if (_ridesTarget)
                            _trackBlocks = Handheld::TrackEditing::Blocks(*_ridesTarget);
                        page(Page::trackBlocks);
                        _status = "Block changed; select again";
                    }
                    else if (ridesPresentationFor(Page::trackBlock, choice).rows[choice] == "Continue section...")
                    {
                        const auto block = *_trackBlock;
                        close();
                        if (!Handheld::ResumeCustomTrackFrom(block.ride, block.origin, block.type))
                            GetWindowManager()->ShowError(
                                "Cannot continue section", "No compatible forward endpoint; select again");
                    }
                    else if (ridesPresentationFor(Page::trackBlock, choice).rows[choice] == "Build backward...")
                    {
                        const auto block = *_trackBlock;
                        close();
                        if (!Handheld::ResumeCustomTrackFrom(block.ride, block.origin, block.type, true))
                            GetWindowManager()->ShowError("Cannot build backward", "No compatible backward endpoint; select again");
                    }
                    else if (ridesPresentationFor(Page::trackBlock,choice).rows[choice] == "Replace block...")
                    {
                        _trackPrevious = Handheld::TrackEditing::Previous(*_trackBlock);
                        const auto result = _trackPrevious ? Handheld::TrackEditing::Replace(*_trackBlock,*_trackPrevious)
                            : Handheld::DesignAuthoring::Error("Native predecessor unavailable");
                        if (result.error == GameActions::Status::ok) { _trackCost=result.cost; page(Page::trackReplaceConfirm); }
                        else { _status=result.getErrorMessage(); invalidate(); }
                    }
                    else if (choice == 0)
                    {
                        const auto origin = _trackBlock->origin;
                        close();
                        if (auto* main = WindowGetMain())
                            WindowScrollToLocation(*main, origin);
                    }
                    else if (choice == 1 || choice == 5)
                    {
                        const auto speed = choice == 5 ? std::optional<uint8_t>{ _trackSpeed } : std::nullopt;
                        const auto result = Handheld::TrackEditing::Change(*_trackBlock, speed);
                        if (result.error == GameActions::Status::ok)
                        {
                            _trackCost = result.cost;
                            page(choice == 1 ? Page::trackRemoveConfirm : Page::trackSpeedConfirm);
                        }
                        else
                        {
                            _status = result.getErrorMessage();
                            invalidate();
                        }
                    }
                    else if (choice == 2)
                    {
                        _trackBlocks = Handheld::TrackEditing::Blocks(*_ridesTarget);
                        page(Page::trackBlocks);
                    }
                    else if (trackTypeHasSpeedSetting(_trackBlock->type))
                    {
                        const int minimum = getGameState().cheats.unlockOperatingLimits ? 0 : 2;
                        _trackSpeed = std::clamp<int>(_trackSpeed + (choice == 3 ? -2 : 2), minimum, kMaximumTrackSpeed);
                        invalidate();
                    }
                    break;
                case Page::trackReplaceConfirm:
                    if (choice == 0) page(Page::trackBlock);
                    else if (_trackBlock && _trackPrevious)
                    {
                        const auto block=*_trackBlock, previous=*_trackPrevious;
                        const auto cost=_trackCost;
                        close();
                        const auto result=Handheld::TrackEditing::Replace(block,previous,cost,true,
                            [block,previous](const auto*,const GameActions::Result* outcome) {
                                if (outcome->error != GameActions::Status::ok)
                                { GetWindowManager()->ShowError("Cannot replace track",outcome->getErrorMessage()); return; }
                                if (!Handheld::TrackEditing::Valid(previous)
                                    || !Handheld::ResumeCustomTrackFrom(previous.ride,previous.origin,previous.type))
                                { GetWindowManager()->ShowError("Track removed", "Reselect the predecessor to rebuild"); return; }
                                Handheld::CustomTracks::Select(block.type);
                                if (trackTypeHasSpeedSetting(block.type)) Handheld::CustomTracks::SetPlacementSpeed(block.speed);
                                if (Handheld::CustomTracks::SeatSupported()) Handheld::CustomTracks::SetPlacementSeat(block.seat);
                                if (block.chain) Handheld::CustomTracks::Chain();
                            });
                        if (result.error != GameActions::Status::ok)
                            GetWindowManager()->ShowError("Cannot replace track",result.getErrorMessage());
                    }
                    break;
                case Page::trackRemoveConfirm:
                case Page::trackSpeedConfirm:
                    if (choice == 0)
                        page(Page::trackBlock);
                    else if (_trackBlock)
                    {
                        const auto block = *_trackBlock;
                        const auto speed = _page == Page::trackSpeedConfirm ? std::optional<uint8_t>{ _trackSpeed }
                                                                            : std::nullopt;
                        const auto cost = _trackCost;
                        close();
                        const auto result = Handheld::TrackEditing::Change(block, speed, cost, true);
                        if (result.error != GameActions::Status::ok)
                            GetWindowManager()->ShowError("Cannot edit track", result.getErrorMessage());
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
                    if (choice < rows.size() && rows[choice] == "Edit track blocks...")
                    {
                        if (_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL))
                        {
                            PauseToggle();
                            _pausedHere = false;
                        }
                        _trackBlocks = Handheld::TrackEditing::Blocks(*_ridesTarget);
                        _trackBlock.reset();
                        _dashboard.section = Handheld::Dashboard::Section::rides;
                        _dashboard.focus = Handheld::Dashboard::Focus::content;
                        page(Page::trackBlocks);
                        break;
                    }

                    if (choice < rows.size() && rows[choice] == "Save design...")
                    {
                        const auto result = Handheld::DesignAuthoring::Begin(*_ridesTarget);
                        if (result.error == GameActions::Status::ok)
                        {
                            _nameEntry = { Handheld::DesignAuthoring::DefaultName() };
                            page(Page::designScenery);
                        }
                        else _status = result.getErrorMessage();
                        invalidate();
                        break;
                    }
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
                    if (choice < rows.size() && rows[choice] == "Edit maze...")
                    {
                        const auto t = *_ridesTarget;
                        close();
                        Handheld::ResumeMaze(t.id);
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
                case Page::designScenery:
                    if (choice == 0) page(Page::designName);
                    else
                    {
                        const auto result = choice < 3 ? Handheld::DesignAuthoring::SetNearby(choice == 1)
                            : Handheld::DesignAuthoring::ToggleScenery(choice - 3);
                        _status = result.error == GameActions::Status::ok ? "Native scenery selection updated" : result.getErrorMessage();
                        invalidate();
                    }
                    break;
                case Page::designName:
                    if (choice == 47) page(Page::designScenery);
                    else if (choice == 46)
                    {
                        const auto result = Handheld::DesignAuthoring::Review(_nameEntry.text);
                        if (result.error == GameActions::Status::ok) page(Page::designExport);
                        else _status = result.getErrorMessage();
                        invalidate();
                    }
                    else { _nameEntry.Press(choice); invalidate(); }
                    break;
                case Page::designExport:
                    if (choice == 0)
                    {
                        Handheld::DesignAuthoring::CancelReview();
                        page(Page::designName);
                    }
                    else
                    {
                        const auto filename = Handheld::DesignAuthoring::Filename();
                        const auto result = Handheld::DesignAuthoring::Confirm();
                        page(Page::ridesInspect);
                        _status = result.error == GameActions::Status::ok ? "Saved " + filename : result.getErrorMessage();
                        invalidate();
                    }
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
                    if (choice == 2 || choice == 3)
                    {
                        _guestGroupMode = choice == 2 ? Handheld::GuestManagement::GroupMode::actions
                                                      : Handheld::GuestManagement::GroupMode::thoughts;
                        _guestGroupFocus = 0;
                        page(Page::guestGroups);
                    }
                    else if (choice == 0 || choice == 4)
                    {
                        _guestFilter = { {}, choice == 4 };
                        _guestRosterParent = Page::guests;
                        _guestRosterFocus = 0;
                        page(Page::guestRoster);
                    }
                    else
                        page(Page::dashboard);
                    break;
                case Page::guestGroups:
                    _guestGroupFocus = choice;
                    if (choice >= _guestGroups.size())
                        page(Page::guests);
                    else
                    {
                        _guestFilter = { _guestGroups[choice], false };
                        _guestRosterParent = Page::guestGroups;
                        _guestRosterFocus = 0;
                        page(Page::guestRoster);
                    }
                    break;
                case Page::guestRoster:
                    _guestRosterFocus = choice;
                    if (choice >= _guestRoster.size())
                        page(_guestRosterParent);
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
                    else if (choice == 6)
                    {
                        _guestNameBaseline = Handheld::GuestManagement::Resolve(*_guestTarget)->getName();
                        _nameEntry = { _guestNameBaseline };
                        page(Page::guestName);
                    }
                    else if (choice == 7)
                    {
                        _guestFlagsBaseline = Handheld::GuestManagement::Resolve(*_guestTarget)->peepFlags;
                        _guestTrackingValue = !_guestFlagsBaseline.has(PeepFlag::tracking);
                        page(Page::guestTracking);
                    }
                    else if (choice == 9)
                    {
                        const auto person = *_guestTarget;
                        if (!Handheld::GuestManagement::Resolve(person)->canBePickedUp())
                        {
                            _status = "Person cannot be picked up now";
                            break;
                        }
                        close();
                        Handheld::BeginPeepPlacement({ person, {} });
                        return;
                    }
                    else if (choice == 8)
                        page(Page::guestPreferences);
                    else
                        page(
                            choice == 1       ? Page::guestNeeds
                                : choice == 2 ? Page::guestThoughts
                                : choice == 3 ? Page::guestSpending
                                : choice == 4 ? Page::guestInventory
                                              : Page::guestRoster);
                    break;
                case Page::guestName:
                    if (choice == 47)
                        page(Page::guestInspect);
                    else if (choice == 46 && _guestTarget)
                    {
                        const auto result = Handheld::GuestManagement::SetName(
                            *_guestTarget, _guestNameBaseline, _nameEntry.text, false);
                        if (result.error == GameActions::Status::ok)
                            page(Page::guestNameConfirm);
                        else
                            _status = Handheld::Finances::Feedback(result);
                    }
                    else
                        _nameEntry.Press(choice);
                    break;
                case Page::guestNameConfirm:
                case Page::guestTracking:
                    if (choice == 1)
                        page(_page == Page::guestTracking ? Page::guestInspect : Page::guestName);
                    else if (_guestTarget)
                    {
                        const auto result = _page == Page::guestTracking
                            ? Handheld::GuestManagement::SetTracking(
                                  *_guestTarget, _guestFlagsBaseline, _guestTrackingValue, true)
                            : Handheld::GuestManagement::SetName(*_guestTarget, _guestNameBaseline, _nameEntry.text, true);
                        if (result.error == GameActions::Status::ok)
                            page(Page::guestInspect);
                        else
                            _status = Handheld::Finances::Feedback(result);
                    }
                    break;
                case Page::guestPreferences:
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
                    if (_ridesTarget && choice < rows().size() && rows()[choice] == "Edit maze...")
                    {
                        const auto t = *_ridesTarget;
                        close();
                        Handheld::ResumeMaze(t.id);
                    }
                    else if (_ridesTarget && choice < rows().size() && rows()[choice] == "Edit track blocks...")
                    {
                        if (_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL))
                        {
                            PauseToggle();
                            _pausedHere = false;
                        }
                        _trackBlocks = Handheld::TrackEditing::Blocks(*_ridesTarget);
                        _trackBlock.reset();
                        _dashboard.section = Handheld::Dashboard::Section::rides;
                        _dashboard.focus = Handheld::Dashboard::Focus::content;
                        page(Page::trackBlocks);
                    }
                    else if (
                        _ridesTarget && Handheld::CustomTracks::CanResume(*_ridesTarget)
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
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Saved designs", _status,
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
            if (_page == Page::peepCancel)
            {
                Handheld::DrawManagementPanel(
                    rt, { ContextGetWidth(), ContextGetHeight() }, Handheld::PeepPlacement::CancelPresentation(_focus.index()));
                return;
            }
            if (_page == Page::staffPatrolClear && _patrolDraft)
            {
                Handheld::DrawManagementPanel(
                    rt, { ContextGetWidth(), ContextGetHeight() },
                    Handheld::StaffPatrol::Presentation(*_patrolDraft, _focus.index()));
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
                const int footerY = ContextGetHeight() - 21;
                Handheld::DrawCommand(rt, { 12, footerY }, 38, "D-pad", "Choose");
                Handheld::DrawCommand(rt, { 184, footerY }, 17, "A", "Select");
                Handheld::DrawCommand(rt, { 356, footerY }, 17, "B", "Back");
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
                    title = "RIDE CONSTRUCTION";
                    break;
                case Page::designOptions:
                    title = "DESIGN OPTIONS";
                    break;
                case Page::designConfirm:
                    title = "PLACE DESIGN?";
                    break;
                case Page::peepOptions:
                case Page::peepCancel:
                    title = "PERSON PLACEMENT";
                    break;
                case Page::staffPatrolOptions:
                    title = "STAFF PATROL";
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
    void HandheldDesignOptionsOpen()
    {
        if (auto* window = OpenPanel())
            window->openDesignOptions();
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
    void HandheldStaffPatrolOptionsOpen()
    {
        if (auto* window = OpenPanel())
            window->openStaffPatrolOptions();
    }
    void HandheldGuestOpen(const Handheld::GuestManagement::Target& person)
    {
        auto* window = OpenPanel();
        if (window)
            window->openGuest(person);
    }
    void HandheldPeepPlacementOptionsOpen()
    {
        auto* window = OpenPanel();
        if (window)
            window->openPeepOptions();
    }
    void HandheldStaffOpen(std::optional<Handheld::StaffManagement::Target> target)
    {
        if (auto* window = OpenPanel())
            window->openStaff(target);
    }
    Handheld::ManagementPresentation HandheldStaffPresentation()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window ? static_cast<const HandheldMenuWindow*>(window)->staffModel() : Handheld::ManagementPresentation{};
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

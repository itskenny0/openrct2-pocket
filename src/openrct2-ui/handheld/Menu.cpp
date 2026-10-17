#include "ShopAppearance.h"
#include "DesignAvailability.h"
// SPDX-License-Identifier: GPL-3.0-only
#include "AreaTools.h"
#include "Currency.h"
#include "CustomTracks.h"
#include "Dashboard.h"
#include "Designs.h"
#include "DesignAuthoring.h"
#include "Explorer.h"
#include "Finances.h"
#include "FinanceInformation.h"
#include "FlatRides.h"
#include "GuestManagement.h"
#include "GuestFollow.h"
#include "ListFocus.h"
#include "MarketingManagement.h"
#include "Mazes.h"
#include "Panel.h"
#include "ParkOverview.h"
#include "ParkMap.h"
#include "ParkInformation.h"
#include "MapSubjects.h"
#include "Paths.h"
#include "PathSurfacePicker.h"
#include "TerrainMaterials.h"
#include "ResearchManagement.h"
#include "ResearchCatalog.h"
#include "ManagementNews.h"
#include "RideAccess.h"
#include "RideManagement.h"
#include "RideTabs.h"
#include "RideNavigation.h"
#include "PeopleNavigation.h"
#include "RightsTools.h"
#include "SaveSlots.h"
#include "FileBrowsing.h"
#include "NamedSave.h"
#include <openrct2-ui/interface/FileBrowser.h>
#include <openrct2/interface/WindowTypes.h>
#include "SceneryTools.h"
#include "SceneryClear.h"
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
#include <openrct2/ParkImporter.h>
#include <openrct2/core/FileStream.h>
#include <openrct2/park/ParkPreview.h>
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
        pathSurfacePicker,
        terrainMaterialPicker,
        pathUndoConfirm,
        treePicker,
        sceneryCategory,
        sceneryPicker,
        sceneryOptions,
        sceneryClearOptions,
        sceneryClearConfirm,
        sceneryGroups,
        sceneryStyle,
        sceneryStyleEditor,
        sceneryStyleConfirm,
        areaOptions,
        terrainPaintConfirm,
        rightsOptions,
        rightsConfirm,
        sceneryName,
        scenerySearch,
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
        parkMap,
        parkNews,
        parkNewsDetail,
        parkAwards,
        parkHistory,
        mapSubjects,
        mapSubject,
        finances,
        financeLedger,
        financeHistory,
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
        guestFilterName,
        guestNameConfirm,
        guestTracking,
        guestPreferences,
        guestRideHistory,
        peepOptions,
        peepCancel,
        ridesRoster,
        ridesInspect,
        trackBlocks,
        trackBlock,
        trackContext,
        trackRemoveConfirm,
        trackReplaceConfirm,
        trackSpeedConfirm,
        ridesAccess,
        accessManage,
        accessPoint,
        accessReview,
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
        researchCatalog,
        researchItem,
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
        shopAppearance,
        shopAppearanceEditor,
        shopAppearanceReview,
        shopDemolish,
        save,
        load,
        parkInspect,
        designFiles,
        designInspect,
        designVehicles,
        designWarnings,
        parkFiles,
        saveFiles,
        saveName,
        saveReview,
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

    class HandheldMenuWindow;
    static HandheldMenuWindow* OpenPanel();

    class HandheldMenuWindow final : public Window
    {
        Handheld::ListFocus _focus;
        Page _page = Page::park;
        Handheld::ObjectPicker _picker;
        bool _pausedHere{};
        size_t _buildFocus{}, _pathModeFocus{};
        std::array<size_t, 2> _pathOptionsFocus{};
        bool _shopPickerFromBuild{};
        bool _ridePickerFromBuild{};
        bool _designPickerFromBuild{};
        ObjectEntryIndex _customObject = kObjectEntryIndexNull;
        ride_type_t _customType = kRideTypeNull;
        bool _exitAfterSave{};
        size_t _slot{};
        Handheld::FileBrowsing::State _files;
        std::optional<Handheld::FileBrowsing::Identity> _loadFile;
        bool _fileLoad{};
        size_t _fileFocus{}, _fileDetail{}, _designDetail{};
        std::optional<ObjectEntryDescriptor> _designChoice;
        std::vector<ObjectEntryDescriptor> _designVehicles;
        std::vector<int32_t> _designVehicleContext;
        bool _designVehicleFromFile{};
        bool fileDesignEligible() const {
            if (!_designMetadata) return false;
            auto td=*_designMetadata;
            if (_designChoice) td.trackAndVehicle.vehicleObject=*_designChoice;
            return Handheld::Designs::Eligible(td);
        }
        void openDesignVehicles(bool file) {
            _designVehicleFromFile=file;_designVehicles.clear();
            const auto* td=file?_designMetadata.get():Handheld::Designs::Draft();
            if (td) {
                _designVehicleContext=TrackDesignPlacementContext::Capture(*td);
                for (auto i:Handheld::DesignAvailability::Vehicles(*td))
                    if (auto* o=GetContext()->GetObjectManager().GetLoadedObject<RideObject>(i))
                        _designVehicles.push_back(o->GetDescriptor());
            }
            _designDetail=0; page(Page::designVehicles);
        }
        std::optional<ParkPreview> _parkFilePreview;
        std::string _parkPreviewStatus;
        std::optional<Handheld::FileBrowsing::Identity> _designFile;
        std::unique_ptr<TrackDesign> _designMetadata;
        std::optional<Handheld::PickerEntry> _designFilePreview;
        std::string _designFileStatus;
        bool _namedSave{};
        std::optional<Handheld::NamedSave::Draft> _saveDraft;
        std::string fileDetailPath() const
        {
            const size_t offset=_page==Page::saveFiles ? 3 : 2;
            return _focus.index()>=offset && _focus.index()-offset<_files.entries.size()
                ? _files.entries[_focus.index()-offset].path.u8string() : _files.directory.u8string();
        }
        void openSaveFiles()
        {
            _namedSave=true; _saveDraft.reset(); _fileFocus=0; _fileDetail=0;
            _files.Open(fs::u8path(Ui::FileBrowser::GetInitialDirectoryByType(LoadSaveType::park)),"*.park");
            page(Page::saveFiles);
        }
        void editSaveName(std::string name)
        {
            _saveDraft.reset(); _nameEntry={name};
            if (!Handheld::NamedSave::SafeName(name)) _nameEntry.text="My park";
            page(Page::saveName);
        }
        void openParkFiles()
        {
            _fileLoad = true;
            _fileFocus = 0; _fileDetail = 0;
            _files.Open(fs::u8path(Ui::FileBrowser::GetInitialDirectoryByType(LoadSaveType::park)),
                Ui::FileBrowser::GetFilterPatternByType(LoadSaveType::park, false));
            page(Page::parkFiles);
        }

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
        Handheld::ParkMap::State _parkMap;
        std::vector<Handheld::ParkInformation::NewsTarget> _parkNews;
        std::optional<Handheld::ParkInformation::NewsTarget> _parkNewsTarget;
        size_t _parkNewsPage{}, _parkNewsFocus{};
        bool _parkGuestHistory{};
        bool _managementNewsReturn{};
        std::vector<Handheld::MapSubjects::Target> _mapSubjects;
        std::optional<Handheld::MapSubjects::Target> _mapTarget;
        size_t _mapSubjectsFocus{};
        bool _mapInspection{};
        Page _mapInspectionReturn = Page::mapSubject;
        void inspectMapSubject(const Handheld::MapSubjects::Target& target, Page parent)
        {
            if (!Handheld::MapSubjects::Current(target)) { _status = "Subject departed or changed"; invalidate(); return; }
            std::visit([&](const auto& person) {
                using T = std::decay_t<decltype(person)>;
                if constexpr (std::is_same_v<T, Handheld::GuestManagement::Target>) openGuest(person);
                else if constexpr (std::is_same_v<T, Handheld::StaffManagement::Target>)
                { _dashboard.section = Handheld::Dashboard::Section::staff; openStaff(person); }
                else
                { _ridesTarget = person; _dashboard.section = Handheld::Dashboard::Section::rides;
                  _dashboard.focus = Handheld::Dashboard::Focus::content;
                  if (person.shop) { _shop = Handheld::ShopManagement::Capture(person.id); page(Page::shop); }
                  else page(Page::ridesInspect); }
            }, target);
            _mapInspection = true;
            _mapInspectionReturn = parent;
        }
        Handheld::FinanceInformation::Ledger _ledger;
        size_t _financeGraph{}, _financeSample{};
        uint8_t _researchFilter{};
        std::vector<Handheld::ResearchCatalog::Target> _researchCatalog;
        std::optional<Handheld::ResearchCatalog::Target> _researchItem;
        size_t _researchCatalogFocus{};
        bool parkInformation() const
        {
            return _page == Page::researchCatalog || _page == Page::researchItem || _page == Page::financeHistory || _page == Page::financeLedger || _page == Page::mapSubjects || _page == Page::mapSubject || _page == Page::parkNews || _page == Page::parkNewsDetail || _page == Page::parkAwards
                || _page == Page::parkHistory;
        }
        Handheld::ManagementPresentation parkInformationPresentation() const
        {
            using namespace Handheld::ParkInformation;
            if (_page == Page::researchCatalog) return Handheld::ResearchCatalog::Presentation(_researchCatalog,_researchFilter,_focus.index());
            if (_page == Page::researchItem && _researchItem) return Handheld::ResearchCatalog::Detail(*_researchItem,_focus.index());
            if (_page == Page::financeHistory) return Handheld::FinanceInformation::History(_financeGraph,_financeSample,_focus.index());
            if (_page == Page::financeLedger) return Handheld::FinanceInformation::LedgerPresentation(_ledger, _focus.index());
            if (_page == Page::mapSubjects) return Handheld::MapSubjects::Presentation(_mapSubjects, _parkMap.mode, _parkMap.filter, _focus.index(), _parkMap.guestFilter);
            if (_page == Page::mapSubject && _mapTarget) return Handheld::MapSubjects::Detail(*_mapTarget, _focus.index(), _status);
            if (_page == Page::parkNews) return NewsPresentation(_parkNews, _focus.index());
            if (_page == Page::parkNewsDetail && _parkNewsTarget) {
                auto model=NewsDetail(*_parkNewsTarget,_parkNewsPage,_focus.index(),_status);
                model.rows[2]=Handheld::ManagementNews::Label(*_parkNewsTarget); return model;
            }
            if (_page == Page::parkAwards) return Awards(_focus.index());
            return History(_parkGuestHistory, _focus.index());
        }
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
        size_t _guestRosterFocus=3, _guestGroupFocus{};
        std::vector<Handheld::RideManagement::Target> _guestRides;
        size_t _guestRideFocus{};
        bool _guestRideInspection{};
        Handheld::PeopleTabs::State _staffTabs, _guestTabs;
        bool _staffFromPark{}, _guestFromPark{};
        Handheld::GuestManagement::GroupMode _guestGroupMode{};
        std::vector<Handheld::GuestManagement::Group> _guestGroups;
        Handheld::GuestManagement::Filter _guestFilter;
        Page _guestRosterParent = Page::guests;
        bool _guestMapReturn{};
        std::optional<Handheld::RideManagement::Target> _guestRosterRideParent;
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
        size_t _ridesInspectFocus{};
        bool _rideFromPark{};
        Handheld::RideTabs::State _rideTabs;
        bool _ridesTabChild{};
        bool _ridesBusy{};
        std::vector<Handheld::RideAccess::ExistingPoint> _accessPoints;
        std::optional<Handheld::RideAccess::ExistingPoint> _accessPoint;
        std::optional<Handheld::RideAccess::EditDraft> _accessDraft;
        size_t _accessFocus{};
        Handheld::TextEntry _nameEntry;
        Page _scenerySearchParent = Page::sceneryOptions;
        size_t _scenerySearchRow{};
        std::optional<Handheld::SceneryTools::Target> _signTarget;
        std::optional<Handheld::SceneryTools::StyleDraft> _signStyle;
        std::optional<Handheld::SceneryClear::Draft> _clearDraft;
        bool _clearReviewResumed{};
        std::optional<Handheld::AreaTools::PaintDraft> _paintDraft;
        bool _paintReviewResumed{};
        bool _pathUndoResumed{};
        std::optional<Handheld::Paths::UndoDraft> _pathUndo;
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
        std::vector<int32_t> _trackContext;
        money64 _trackCost{};
        uint8_t _trackSpeed{};
        bool trackManagement() const
        {
            return _page == Page::trackBlocks || _page == Page::trackBlock || _page == Page::trackContext || _page == Page::trackRemoveConfirm
                || _page == Page::trackSpeedConfirm || _page == Page::trackReplaceConfirm;
        }
        bool ridesManagement() const
        {
            return _page == Page::accessManage || _page == Page::accessPoint || _page == Page::accessReview
                || trackManagement() || _page == Page::ridesRoster || _page == Page::ridesInspect || _page == Page::ridesGraphs
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
                    model.rows.push_back("Inspect connections / station...");
                    if (current == Page::trackContext)
                    {
                        model.title = "NATIVE TRACK CONTEXT";
                        model.stats.clear();
                        model.rows = Handheld::TrackEditing::Inspection(*_trackBlock);
                    }
                    if (current == Page::trackRemoveConfirm || current == Page::trackSpeedConfirm || current == Page::trackReplaceConfirm)
                    {
                        model.confirmation = true;
                        model.title = current == Page::trackReplaceConfirm ? "REMOVE / REBUILD BLOCK?"
                            : current == Page::trackRemoveConfirm ? "REMOVE BLOCK?" : "CHANGE TRACK SPEED?";
                        if (current == Page::trackReplaceConfirm)
                        {
                            model.stats.clear();
                            model.bodyLines = { "Remove: " + Handheld::TrackEditing::Label(*_trackBlock),
                                "Then choose / build at this location.", "Removal leaves a gap; no rollback." };
                            if (_trackBlock->type == TrackElemType::beginStation || _trackBlock->type == TrackElemType::middleStation
                                || _trackBlock->type == TrackElemType::endStation)
                            {
                                model.bodyLines.push_back("Native stations may split / renumber.");
                                model.bodyLines.push_back(Handheld::TrackEditing::AccessRemovalLabel(*_trackBlock));
                                model.bodyLines.push_back("Rebuild deleted entrances/exits explicitly.");
                            }
                        }
                        if (current == Page::trackRemoveConfirm && (_trackBlock->type == TrackElemType::beginStation
                            || _trackBlock->type == TrackElemType::middleStation || _trackBlock->type == TrackElemType::endStation))
                        {
                            model.stats.clear();
                            model.bodyLines = { "Remove: " + Handheld::TrackEditing::Label(*_trackBlock),
                                "Native stations may split / renumber.", Handheld::TrackEditing::AccessRemovalLabel(*_trackBlock), "Rebuild deleted entrances/exits explicitly." };
                        }
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
            if (current == Page::accessReview && _accessDraft)
                return Handheld::RideAccess::ReviewPresentation(*_accessDraft, selected, _status);
            if (current == Page::accessManage || current == Page::accessPoint)
            {
                Handheld::ManagementPresentation model;
                model.title = "Entrances / exits"; model.breadcrumb = "Rides / Access"; model.selected = selected;
                if (current == Page::accessManage)
                {
                    for (const auto& point : _accessPoints) model.rows.push_back(Handheld::RideAccess::PointLabel(point));
                    model.rows.push_back("Back to ride");
                    model.status = _accessPoints.empty() ? "No supported existing access points" : "Select a station entrance or exit";
                }
                else if (_accessPoint)
                {
                    const auto& point = *_accessPoint;
                    model.title = Handheld::RideAccess::PointLabel(point);
                    model.bodyLines = {"Tile " + std::to_string(point.source.x / 32) + "," + std::to_string(point.source.y / 32),
                        "Height Z " + std::to_string(point.source.z) + " / direction " + std::to_string(point.source.direction),
                        "Native edit updates queues / paths.", "Maze hedges follow native access rules."};
                    model.rows = Handheld::RideAccess::Current(point)
                        ? std::vector<std::string>{"Move access...", "Remove access...", "Back to access list"}
                        : std::vector<std::string>{"Back to access list"};
                    if (canConnectAccessPoint())
                        model.rows.insert(model.rows.end() - 1, point.exit ? "Connect exit path..." : "Connect entrance queue...");
                    const auto* ride = Handheld::RideManagement::Resolve(point.target);
                    model.status = !Handheld::RideAccess::Current(point) ? "Access changed; select it again"
                        : ride && ride->status == RideStatus::closed ? "Move keeps source until Apply" : "Close the ride before editing access";
                }
                if (!_status.empty()) model.status = _status;
                return model;
            }
            if (current == Page::ridesAccess && _ridesTarget)
                return Handheld::RideAccess::Presentation(*_ridesTarget, selected, _status);
            auto model = Handheld::RideManagement::Presentation(
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
            if (current == Page::ridesInspect)
            {
                const auto stats = Handheld::RideManagement::Presentation(View::statistics,0,{},_ridesTarget);
                model = Handheld::RideTabs::Compose(model,stats,_rideTabs,selected);
                if (_ridesTarget && Handheld::RideAccess::Supported(*_ridesTarget)) model.contextualAction = "Access";
                if(const auto* ride=_ridesTarget?Handheld::RideManagement::Resolve(*_ridesTarget):nullptr)
                    model.headerInk=ride->status==RideStatus::open?"{POSITIVE}":ride->status==RideStatus::testing?"{CAUTION}":"{MUTED}";
            }
            if (current == Page::ridesConfirm && _ridesDraft && _ridesDraft->demolition) model.destructive=true;
            if (current == Page::ridesRoster) model.contentSized = true;
            if (current == Page::ridesCustomers && _ridesTarget && Handheld::RideManagement::Resolve(*_ridesTarget))
                model.rows.insert(model.rows.end(), { "Guests riding...", "Guests queuing...", "Guests thinking about ride..." });
            return model;
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
            if(currentPage==Page::staffInspect && _staffTarget) {model=Handheld::PeopleTabs::Staff(*_staffTarget,_staffTabs,selected,_ordersDraft);model.status=_status;}
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
                model.rows[1] = "Filter: "
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
                || _page == Page::guestPreferences || _page == Page::guestRideHistory || _page == Page::guestFilterName;
        }
        Handheld::ManagementPresentation guestPresentationFor(Page current, size_t selected) const
        {
            using Handheld::GuestManagement::View;
            if (current == Page::guestRideHistory && _guestTarget)
                return Handheld::GuestManagement::HistoryPresentation(*_guestTarget, _guestRides, selected);
            if (current == Page::guestFilterName)
            {
                auto model = Handheld::StaffManagement::NamePresentation(_nameEntry, selected, false, _status);
                model.breadcrumb = "Guests / Filter by name";
                model.title = _nameEntry.text.empty() ? "All guest names" : _nameEntry.text;
                model.status = "Case insensitive / OK Apply / Clr Remove";
                return model;
            }
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
            if(current==Page::guestInspect && _guestTarget){model=Handheld::PeopleTabs::Guest(*_guestTarget,_guestTabs,selected);model.status=_status;}
            if (current == Page::guestRoster)
            {
                model.title = Handheld::GuestManagement::FilterName(_guestFilter);
                model.rows.back() = _guestRosterParent == Page::guestGroups ? "Back to groups"
                    : _guestRosterParent == Page::ridesCustomers ? "Back to ride customers" : "Back to Guests";
                model.rows[2]=_guestFilter.tracked ? "All tracking" : "Tracked only";
                model.status = std::to_string(_guestRoster.size()) + " guests / A Inspect or map";
            }
            return model;
        }
        bool managementShell() const
        {
            return _page == Page::dashboard || financeManagement() || staffManagement() || researchManagement()
                || marketingManagement() || guestManagement() || ridesManagement() || overviewManagement();
        }
        std::optional<Handheld::ShopAppearance::Draft> _shopAppearanceDraft;
        size_t _shopAppearanceChannel{};
        bool _shopAppearanceBusy{};
        uint64_t _shopAppearanceRequest{};
        std::shared_ptr<int> _shopAppearanceLifetime=std::make_shared<int>(0);
    public:
        Handheld::ManagementPresentation shopAppearancePresentation() const {
            using namespace Handheld;
            ManagementPresentation model;model.breadcrumb="Shops / Native appearance";
            const auto* ride=_shop?ShopManagement::Find(*_shop):nullptr;
            model.title=ride?ride->getName():"Shop unavailable";model.selected=_focus.index();
            model.rows=rows();model.status=_status;
            if(_page==Page::shopAppearance) {
                if(ride) {const auto* object=GetContext()->GetObjectManager().GetLoadedObject<RideObject>(ride->subtype);
                    model.bodyLines={object?object->GetName():"Object unavailable","Native supported building / item controls"};}
                if(!_shop || ShopAppearance::Settings(*_shop).empty())model.status="No native appearance controls for this object";
            } else if(_shopAppearanceDraft) {
                const auto& d=*_shopAppearanceDraft;
                model.stats={{"Channel",d.setting.name},{"Current",ShopAppearance::Label(d.setting,d.setting.current)}};
                if(_page==Page::shopAppearanceReview) {
                    model.confirmation=true;model.title="Apply shop appearance?";
                    model.stats.push_back({"Proposed",ShopAppearance::Label(d.setting,d.value)});
                    const auto quote=ShopAppearance::Action(d,false);
                    model.stats.push_back({"Native cost",quote.error==GameActions::Status::ok?Handheld::FormatMoney(quote.cost):"Unavailable"});
                    model.bodyLines={"Native appearance action / exact reviewed object"};
                } else model.bodyLines={"A: review chosen value / B: cancel"};
            }
            if(_shopAppearanceBusy)model.status="Applying native change...";
            return model;
        }
    private:
        bool shopManagement() const
        {
            return _page == Page::shop || _page == Page::shopPrices || _page == Page::shopDemolish || _page==Page::shopAppearance || _page==Page::shopAppearanceEditor || _page==Page::shopAppearanceReview;
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
        void parkMapSubjects()
        {
            if (_page != Page::parkMap) return;
            _mapSubjects = Handheld::MapSubjects::Roster(_parkMap.mode, _parkMap.filter, _parkMap.guestFilter);
            page(Page::mapSubjects);
            if (_mapTarget)
            {
                const auto found = std::find(_mapSubjects.begin(), _mapSubjects.end(), *_mapTarget);
                if (found != _mapSubjects.end()) _focus.move(static_cast<int>(found - _mapSubjects.begin()), rows().size());
            }
        }
        Handheld::ManagementPresentation mapSubjectsModel() const { return parkInformationPresentation(); }
        CoordsXY mapFocus() const { return _parkMap.focus; }
        bool parkMapOpen() const { return _page == Page::parkMap; }
        void parkMapControl(int control)
        {
            if (!parkMapOpen()) return;
            if (control == 0) _parkMap.Mode();
            else _parkMap.zoom = std::clamp(_parkMap.zoom + control, 1, 8);
            _parkMap.Update();
            invalidate();
        }
        bool constructionMenu() const
        {
            return _page == Page::researchCatalog || _page == Page::financeHistory || _page == Page::financeLedger || _page == Page::parkMap || _page == Page::peepOptions || _page == Page::peepCancel || _page == Page::staffPatrolOptions
                || _page == Page::staffPatrolClear || _page == Page::rightsOptions || _page == Page::rightsConfirm
                || (_page == Page::parkFiles || _page == Page::saveFiles || _page == Page::designFiles) || _page == Page::areaOptions || _page == Page::terrainPaintConfirm || _page == Page::sceneryOptions || _page == Page::parkInspect || (_page == Page::designInspect || _page == Page::designVehicles || _page == Page::designWarnings || _page == Page::designConfirm) || _page == Page::saveName || _page == Page::saveReview || _page == Page::sceneryName || _page == Page::scenerySearch
                || _page == Page::sceneryConfirm || _page == Page::sceneryGroups || _page == Page::sceneryStyle
                || _page == Page::sceneryStyleEditor || _page == Page::sceneryStyleConfirm || _page == Page::sceneryClearOptions || _page == Page::sceneryClearConfirm;
        }
        void constructionMove(int x, int y)
        {
            if ((_page == Page::designInspect || _page == Page::designVehicles || _page == Page::designWarnings) && x) {
                const auto labels=rows(); const auto text=_focus.index()<labels.size()?labels[_focus.index()]:std::string{};
                _designDetail=static_cast<size_t>(std::clamp<int64_t>(static_cast<int64_t>(_designDetail)+x,0,Handheld::FileBrowsing::DetailPages(text)-1));
                invalidate(); return;
            }
            if ((_page == Page::designInspect || _page == Page::designVehicles || _page == Page::designWarnings) && y) _designDetail=0;
            if ((_page == Page::parkFiles || _page == Page::saveFiles || _page == Page::designFiles) && x)
            { _fileDetail=static_cast<size_t>(std::clamp<int64_t>(static_cast<int64_t>(_fileDetail)+x,0,Handheld::FileBrowsing::DetailPages(fileDetailPath())-1)); invalidate(); return; }
            if (_page == Page::researchCatalog)
            {
                if (x) { _researchFilter=static_cast<uint8_t>((_researchFilter+x+8)%8); _researchCatalog=Handheld::ResearchCatalog::Roster(_researchFilter); _focus={}; }
                if (y) move(y);
                invalidate(); return;
            }
            if (_page == Page::financeHistory)
            {
                if (x) _financeSample=std::clamp<int>(static_cast<int>(_financeSample)-x,0,63);
                if (y) move(y);
                invalidate(); return;
            }
            if (_page == Page::financeLedger)
            {
                if (x) _ledger.Move(x);
                if (y) move(y);
                invalidate(); return;
            }
            if (_page == Page::parkMap)
            {
                _parkMap.Move(x, y);
                invalidate();
                return;
            }
            if (_page == Page::saveName || _page == Page::sceneryName || _page == Page::scenerySearch)
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
            else if (x && _page == Page::sceneryClearOptions)
                Handheld::SceneryClear::Configure(_focus.index(), x);
            else if (x && _page == Page::sceneryStyleEditor && _focus.index() == 0 && _signStyle)
                Handheld::SceneryTools::AdjustStyle(*_signStyle, x);
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
                                       || _page == Page::parkInspect || (_page == Page::designInspect || _page == Page::designVehicles || _page == Page::designWarnings || _page == Page::designConfirm) || _page == Page::saveName || _page == Page::saveReview || _page == Page::sceneryName || _page == Page::scenerySearch || _page == Page::sceneryConfirm
                                       || (_page == Page::parkFiles || _page == Page::saveFiles || _page == Page::designFiles) || _page == Page::parkMap || parkInformation() || _page == Page::pathOptions || picker() || shopManagement() || managementShell())
                ? ScreenSize{ ContextGetWidth(), ContextGetHeight() }
                : information() ? ScreenSize{ 330, _page == Page::ride ? 340 : 310 }
                                : target;
            WindowSetResize(*this, bounds, bounds);
            WindowSetPosition(
                *this,
                (_page == Page::staffPatrolClear || _page == Page::rightsConfirm || _page == Page::parkInspect || (_page == Page::designInspect || _page == Page::designVehicles || _page == Page::designWarnings || _page == Page::designConfirm) || _page == Page::saveName || _page == Page::saveReview || _page == Page::sceneryName || _page == Page::scenerySearch
                 || _page == Page::sceneryConfirm || (_page == Page::parkFiles || _page == Page::saveFiles || _page == Page::designFiles) || _page == Page::parkMap || parkInformation() || _page == Page::pathOptions || picker() || shopManagement()
                 || managementShell())
                    ? ScreenCoordsXY{ 0, 0 }
                    : ScreenCoordsXY{ 8, std::max(8, (ContextGetHeight() - bounds.height) / 2) });
        }

        void page(Page next)
        {
            if(next==Page::sceneryPicker && _page!=Page::scenerySearch) _sceneryPickerDirect=false;
            if (_page == Page::build) _buildFocus = _focus.index();
            if (_page == Page::paths) _pathModeFocus = _focus.index();
            if (_page == Page::pathOptions) _pathOptionsFocus[Handheld::Paths::IsConnected()] = _focus.index();
            if(next==Page::park || next==Page::dashboard) _managementNewsReturn=false;
            if (_paintReviewResumed && next != Page::terrainPaintConfirm) {
                if (_pausedHere && !(gGamePaused & GAME_PAUSED_NORMAL)) PauseToggle();
                _paintReviewResumed=false;
            }
            if (_clearReviewResumed && next != Page::sceneryClearConfirm) {
                if (_pausedHere && !(gGamePaused & GAME_PAUSED_NORMAL))
                    PauseToggle();
                _clearReviewResumed = false;
            }
            if (_pathUndoResumed && next != Page::pathUndoConfirm)
            {
                if (_pausedHere && !(gGamePaused & GAME_PAUSED_NORMAL)) PauseToggle();
                _pathUndoResumed = false;
            }
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
            if (_page == Page::staffInspect) _staffTabs.save(_focus.index());
            if (_page == Page::guestInspect) _guestTabs.save(_focus.index());
            if (_page == Page::ridesInspect)
            {
                _ridesInspectFocus = _focus.index();
                _rideTabs.save(_focus.index());
            }
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
            if (next == Page::guestRideHistory)
                _guestRides = _guestTarget ? Handheld::GuestManagement::RideHistory(*_guestTarget) : std::vector<Handheld::RideManagement::Target>{};
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
            if(next!=Page::shopAppearanceEditor && next!=Page::shopAppearanceReview)_shopAppearanceDraft.reset();
            _shopAppearanceBusy=false;++_shopAppearanceRequest;
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
            if (next == Page::staffInspect) _focus.move(static_cast<int>(std::min(_staffTabs.rows[_staffTabs.tab],rows().size()-1)),rows().size());
            if (next == Page::guestInspect) _focus.move(static_cast<int>(std::min(_guestTabs.rows[_guestTabs.tab],rows().size()-1)),rows().size());
            if (next == Page::ridesInspect)
                _focus.move(static_cast<int>(std::min(_rideTabs.rows[_rideTabs.tab], rows().size() - 1)), rows().size());
            if (next == Page::build) _focus.move(static_cast<int>(_buildFocus), rows().size());
            if (next == Page::paths) _focus.move(static_cast<int>(_pathModeFocus), rows().size());
            if (next == Page::pathOptions) _focus.move(static_cast<int>(_pathOptionsFocus[Handheld::Paths::IsConnected()]), rows().size());
            _status.clear();
            _priceDraft.reset();
            layout();
            if (next == Page::parkFiles || next == Page::saveFiles || next == Page::designFiles) _focus.move(static_cast<int>(_fileFocus), rows().size());
            if (next == Page::staff)
                _focus.move(static_cast<int32_t>(std::min(_staffRosterFocus, rows().size() - 1)), rows().size());

            if (next == Page::trackBlocks && _trackBlock)
            {
                const auto found = std::find(_trackBlocks.begin(), _trackBlocks.end(), *_trackBlock);
                if (found != _trackBlocks.end()) _focus.move(static_cast<int>(found - _trackBlocks.begin()), rows().size());
            }
            if (next == Page::ridesRoster)
                _focus.move(static_cast<int32_t>(std::min(_ridesRosterFocus, rows().size() - 1)), rows().size());
            if (next == Page::guestGroups)
                _focus.move(static_cast<int32_t>(std::min(_guestGroupFocus, rows().size() - 1)), rows().size());
            if (next == Page::guestRideHistory)
                _focus.move(static_cast<int32_t>(std::min(_guestRideFocus, rows().size() - 1)), rows().size());
            if (next == Page::guestRoster)
                _focus.move(static_cast<int32_t>(std::min(_guestRosterFocus, rows().size() - 1)), rows().size());
            if (next == Page::accessReview || next == Page::saveReview || next == Page::overwrite || next == Page::discard || next == Page::exit || next == Page::shopDemolish
                || next == Page::overviewConfirm || next == Page::ridesConfirm || next == Page::financeConfirm
                || next == Page::marketingConfirm || next == Page::staffHire || next == Page::staffDismiss
                || next == Page::staffNameConfirm || next == Page::guestNameConfirm || next == Page::guestTracking
                || next == Page::peepCancel)
                _focus.move(1, 2); // destructive confirmations default to Cancel
            if (next == Page::researchConfirm)
                _focus.move(static_cast<int32_t>(rows().size() - 1), rows().size());
            invalidate();
        }
        std::vector<Handheld::SceneryTools::Style> signStyles() const
        {
            std::vector<Handheld::SceneryTools::Style> result;
            if (_signTarget)
                for (auto style : {Handheld::SceneryTools::Style::noEntry, Handheld::SceneryTools::Style::textColour,
                                   Handheld::SceneryTools::Style::mainColour})
                    if (Handheld::SceneryTools::BeginStyle(*_signTarget, style)) result.push_back(style);
            return result;
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
                case Page::researchCatalog:
                case Page::researchItem:
                case Page::financeHistory:
                case Page::financeLedger:
                case Page::mapSubjects:
                case Page::mapSubject:
                case Page::parkNews:
                case Page::parkNewsDetail:
                case Page::parkAwards:
                case Page::parkHistory:
                    return parkInformationPresentation().rows;
                case Page::parkMap:
                    return { "Locate tile" };
                case Page::overview:
                case Page::overviewConfirm:
                    return overviewPresentation().rows;
                case Page::trackBlocks:
                case Page::trackContext:
                case Page::trackBlock:
                case Page::trackRemoveConfirm:
                case Page::trackReplaceConfirm:
                case Page::trackSpeedConfirm:
                case Page::ridesRoster:
                case Page::ridesInspect:
                case Page::accessManage:
                case Page::accessPoint:
                case Page::accessReview:
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
                case Page::guestFilterName:
                case Page::guestNameConfirm:
                case Page::guestTracking:
                case Page::guestPreferences:
                case Page::guestRideHistory:
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
                    return { "Status", "Prices", "Demolish", "Rename", "Appearance" };
                case Page::shopAppearance: {
                    std::vector<std::string> result;
                    if(_shop)for(const auto& setting:Handheld::ShopAppearance::Settings(*_shop))
                        result.push_back(setting.name+" / "+Handheld::ShopAppearance::Label(setting,setting.current));
                    result.push_back("Back to shop");return result;
                }
                case Page::shopAppearanceEditor:
                    return _shopAppearanceDraft?_shopAppearanceDraft->setting.labels:std::vector<std::string>{"Shop unavailable"};
                case Page::shopAppearanceReview:return {"Cancel","Apply native change"};
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
                    if (_ridesTarget && Handheld::FlatRides::CanResume(*_ridesTarget))
                        result.insert(result.begin(), "Place entrance / exit...");
                    if (_ridesTarget && Handheld::RideAccess::Supported(*_ridesTarget))
                    {
                        result.insert(result.begin() + (Handheld::FlatRides::CanResume(*_ridesTarget) ? 1 : 0), "Entrances / exits...");
                        result.push_back("Access / paths...");
                    }
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
                case Page::scenerySearch:
                    return _nameEntry.Keys();
                case Page::sceneryConfirm:
                    return { "Apply text", "Cancel" };
                case Page::sceneryClearOptions: return Handheld::SceneryClear::Options();
                case Page::terrainPaintConfirm: return {"Apply paint","Cancel"};
                case Page::sceneryClearConfirm: return {"Clear native area", "Cancel"};
                case Page::sceneryGroups:
                {
                    std::vector<std::string> list;
                    for (const auto& group : Handheld::SceneryTools::Groups(Handheld::SceneryTools::Current().category))
                        list.push_back(group.name);
                    return list;
                }
                case Page::sceneryStyle:
                    {
                    std::vector<std::string> result;
                    for (auto style : signStyles())
                        result.push_back(style == Handheld::SceneryTools::Style::noEntry ? "No entry..."
                            : style == Handheld::SceneryTools::Style::textColour ? "Text colour..." : "Primary colour...");
                    result.push_back("Back");
                    return result;
                    }
                case Page::sceneryStyleEditor:
                    return {_signStyle ? Handheld::SceneryTools::StyleValue(*_signStyle) : "Sign unavailable", "Review change", "Cancel"};
                case Page::sceneryStyleConfirm:
                    return {"Apply native style", "Cancel"};
                case Page::sceneryOptions:
                    return Handheld::SceneryTools::Options();
                case Page::sceneryPicker:
                    return {};
                case Page::designOptions:
                    return { "Height -",    "Height +",      "Automatic height", "Scenery on",
                             "Scenery off", "Rotate camera", "Back to placement", "Choose vehicle", "Availability / warnings" };
                case Page::designConfirm:
                    return { "Cancel", "Place design" };
                case Page::terrainMaterialPicker:
                case Page::pathSurfacePicker:
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
                        return { "Resume", "Turn direction", "Rise", "Descend", "Flat", "Restart endpoint", "Choose surface...", "Undo section..." };
                    return { "Resume",       "Follow ground", "Raise height",  "Lower height",
                             "Flat / slope", "Rotate slope",  "Choose surface..." };
                case Page::pathUndoConfirm: return {"Remove last new section", "Cancel"};
                case Page::parkInspect: return { "Load this park", "Back to files" };
                case Page::designFiles: return _files.Rows();
                case Page::designVehicles: {
                    std::vector<std::string> result;
                    for (const auto& d:_designVehicles) {
                        const auto* object=GetContext()->GetObjectManager().GetLoadedObject(d);
                        result.push_back((object?object->GetName():std::string("Vehicle unavailable"))+" / "+d.ToString());
                    }
                    result.push_back("Cancel / back");return result;
                }
                case Page::designWarnings: {
                    auto result=Handheld::Designs::WarningLines();
                    if (const auto* td=Handheld::Designs::Draft()) {auto detail=Handheld::DesignAvailability::Lines(*td);result.insert(result.end(),detail.begin(),detail.end());}
                    result.push_back("Back to options");return result;
                }
                case Page::designInspect: {
                    std::vector<std::string> result{ fileDesignEligible() ? "Place this design" : "Placement unavailable", "Back to files", "Choose native vehicle" };
                    if (_designMetadata) { auto lines=Handheld::DesignAvailability::Lines(*_designMetadata);result.insert(result.end(),lines.begin(),lines.end()); }
                    return result;
                }
                case Page::parkFiles: return _files.Rows();
                case Page::saveFiles: {
                    auto result=_files.Rows(); result.insert(result.begin(),"Save in this folder..."); return result;
                }
                case Page::saveName: return _nameEntry.Keys();
                case Page::saveReview: return { _saveDraft && _saveDraft->baseline ? "Replace named park" : "Save named park", "Cancel" };
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
                    result.push_back(_page == Page::load ? "Browse parks / autosaves..." : "Named Save As...");
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
            const auto draft=_saveDraft;
            const auto path = _namedSave && draft ? draft->destination : SlotPath(_slot);
            if(_namedSave && (!draft || !draft->Current())) {
                _saveDraft.reset(); _status="Destination changed. Cancel and review again."; invalidate(); return;
            }
            const bool saved = Handheld::WriteSaveAtomically(path, [](const fs::path& temporary) {
                SaveFlags saveFlags{};
                saveFlags.set(SaveFlag::exportObjects, Config::Get().general.savePluginData);
                return ScenarioSave(getGameState(), temporary.u8string(), saveFlags) != 0;
            }, [&] { return !draft || draft->Current(); });
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
            _status = "Saved to " + path.filename().u8string();
            _saveDraft.reset();
        }
        void load()
        {
            if (!_loadFile || !Handheld::FileBrowsing::Current(*_loadFile))
            {
                _status = "File removed, changed or unreadable. Select it again.";
                invalidate(); return;
            }
            const auto path = _loadFile->path.u8string();
            const bool browsing = _fileLoad;
            const auto folder = _files.directory;
            const auto pattern = _files.pattern;
            Handheld::GuestFollow::Stop();
            Handheld::EndTerrain();
            close();
            if (!GetContext()->LoadParkFromFile(path))
            {
                // Import may close all windows or change scenes. Never access this after close/load.
                auto* recovery = OpenPanel();
                if (recovery) recovery->recoverLoad(folder, pattern, browsing, fs::u8path(path));
            }
        }

    public:
        void recoverLoad(const fs::path& folder, const std::string& pattern, bool browsing, const fs::path& selected)
        {
            _fileLoad = browsing;
            if (browsing) {
                _files.Open(folder, pattern); _fileFocus=0;
                for(size_t i=0;i<_files.entries.size();++i)if(_files.entries[i].path==selected)_fileFocus=i+2;
                page(Page::parkFiles);
            }
            else page(Page::load);
            _status = "Native load failed. Choose another file or Back.";
            invalidate();
        }
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
            Handheld::Mazes::ClearPreview();
            Handheld::Designs::ClearPreview();
            Handheld::CustomTracks::ClearPreview();
            setWidgets(kWidgets);
            layout();
            colours[0] = ColourWithFlags{ Colour::grey };
            colours[1] = ColourWithFlags{ Colour::grey };
            _pausedHere = gLegacyScene == LegacyScene::playing && GameIsNotPaused();
            if (_pausedHere)
                PauseToggle();
        }
        void onClose() override
        {
            Handheld::DesignAuthoring::End();
            if (_ridesReviewResumed || _rightsReviewResumed || _clearReviewResumed || _paintReviewResumed || _pathUndoResumed)
                _pausedHere = false; // Our pause was already released; retain any external pause.
            if (_page == Page::designConfirm && _focus.index() == 0)
                Handheld::Designs::CancelReview();
            if (_pausedHere && gLegacyScene == LegacyScene::playing && (gGamePaused & GAME_PAUSED_NORMAL))
                PauseToggle();
        }
        void resumePark()
        {
            if (gLegacyScene == LegacyScene::playing && (gGamePaused & GAME_PAUSED_NORMAL))
                PauseToggle();
            _pausedHere = false;
            close();
        }
        bool canConnectAccessPoint() const
        {
            if (!_accessPoint || !Handheld::RideAccess::Current(*_accessPoint)) return false;
            const auto state = Handheld::RideAccess::Inspect(_accessPoint->target);
            // The existing guided handoff chooses Inspect's next unresolved station.
            // Only offer it here when that is the point actually being inspected.
            return state && (_accessPoint->exit ? state->exit.station : state->entrance.station) == _accessPoint->station;
        }
        void openAccessManagement()
        {
            if (!_ridesTarget) return;
            _accessPoints = Handheld::RideAccess::Existing(*_ridesTarget);
            _dashboard.section = Handheld::Dashboard::Section::rides;
            _dashboard.focus = Handheld::Dashboard::Focus::content;
            page(Page::accessManage);
            _focus.move(static_cast<int32_t>(std::min(_accessFocus, _accessPoints.size())), rows().size());
        }
        void openAccessReview(const Handheld::RideAccess::EditDraft& draft)
        {
            _ridesTarget = draft.point.target; _accessPoint = draft.point; _accessDraft.emplace(draft);
            _dashboard.section = Handheld::Dashboard::Section::rides;
            _dashboard.focus = Handheld::Dashboard::Focus::content;
            page(Page::accessReview);
        }
        void cancelAccessReview()
        {
            if (!_accessDraft) { openAccessManagement(); return; }
            const auto draft = *_accessDraft; _accessDraft.reset();
            if (Handheld::FlatRides::CanResume(draft.point.target))
            {
                Handheld::EndTerrain(); openRideAccess(draft.point.target);
            }
            else if (draft.remove) page(Page::accessPoint);
            else close(); // Existing relocation cursor remains; no native change on Cancel.
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
        bool _terrainMaterialEdge{}, _sceneryPickerDirect{}, _pathSurfaceDirect{};
        void openTerrainMaterialPicker(bool edge)
        {
            _terrainMaterialEdge=edge;
            page(Page::terrainMaterialPicker);
            const auto entries=Handheld::TerrainMaterials::Entries(edge);
            const auto index=edge ? Handheld::AreaTools::settings.edge : Handheld::AreaTools::settings.surface;
            const auto it=std::find_if(entries.begin(),entries.end(),[&](const auto& e){return e.identity.index==index;});
            _picker.setEntries(entries,it==entries.end() ? std::nullopt : std::optional{it->identity});
        }
        void openSceneryCatalog()
        {
            page(Page::sceneryPicker);_sceneryPickerDirect=true;refreshPicker();
        }
        void openPathSurfacePicker(bool direct = false)
        {
            _pathSurfaceDirect = direct;
            Handheld::Paths::ClearPreview();
            _pathOptionsFocus[Handheld::Paths::IsConnected()] = 6;
            page(Page::pathSurfacePicker);
            _picker.setEntries(Handheld::PathSurfaces::Entries(), Handheld::PathSurfaces::Current());
        }
        void refreshPicker(std::optional<Handheld::PickerIdentity> preferred = {})
        {
            auto rideEntries = Handheld::FlatRides::Entries();
            auto mazes = Handheld::Mazes::Entries();
            rideEntries.insert(rideEntries.end(), mazes.begin(), mazes.end());
            auto custom = Handheld::CustomTracks::RideEntries();
            rideEntries.insert(rideEntries.end(), custom.begin(), custom.end());
            _picker.setEntries(
                _page == Page::terrainMaterialPicker ? Handheld::TerrainMaterials::Entries(_terrainMaterialEdge)
                    : _page == Page::pathSurfacePicker   ? Handheld::PathSurfaces::Entries()
                    : _page == Page::trackPiecePicker ? Handheld::CustomTracks::Pieces()
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
        void openTerrainPaintReview(CoordsXY tile)
        {
            if (_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL)) {PauseToggle();_paintReviewResumed=true;}
            _paintDraft=Handheld::AreaTools::BeginPaint(tile,Handheld::AreaTools::settings);
            if (_paintDraft) {page(Page::terrainPaintConfirm);_focus.move(1,2);}
            else {page(Page::areaOptions);_status="Paint unavailable / check native quote or pending actions";}
        }
        void openSceneryClearOptions() {page(Page::sceneryClearOptions);}
        void openSceneryClearReview(CoordsXY tile) {
            if(_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL)){PauseToggle();_clearReviewResumed=true;}
            const auto query=Handheld::SceneryClear::Query(tile,Handheld::SceneryClear::settings);
            _clearDraft=Handheld::SceneryClear::Begin(tile,Handheld::SceneryClear::settings);
            if(_clearDraft){page(Page::sceneryClearConfirm);_focus.move(1,2);}else{page(Page::sceneryClearOptions);_status=query.getErrorMessage();}
        }
        void openSceneryStyle(const Handheld::SceneryTools::Target& target)
        {
            _signTarget = target;
            page(Page::sceneryStyle);
        }
        void openSceneryOptions()
        {
            page(Page::sceneryOptions);
        }
        bool sceneryPicker() const { return _page == Page::sceneryPicker; }
        void openScenerySearch()
        {
            _scenerySearchParent = _page == Page::sceneryPicker ? Page::sceneryPicker : Page::sceneryOptions;
            _scenerySearchRow = _focus.index();
            _nameEntry = { Handheld::SceneryTools::Search() };
            page(Page::scenerySearch);
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
            Handheld::Mazes::ClearPreview();
            Handheld::Designs::ClearPreview();
            Handheld::CustomTracks::ClearPreview();
            _shopPickerFromBuild = fromBuild;
            page(Page::shopPicker);
            refreshPicker(Handheld::PickerIdentity{ ObjectType::ride, Handheld::ShopSelection() });
        }
        void openRidePicker(bool fromBuild = false)
        {
            Handheld::FlatRides::ClearPreview();
            Handheld::Mazes::ClearPreview();
            Handheld::Designs::ClearPreview();
            Handheld::CustomTracks::ClearPreview();
            _ridePickerFromBuild = fromBuild;
            page(Page::ridePicker);
            refreshPicker(Handheld::PickerIdentity{ ObjectType::ride, Handheld::FlatRides::Selection() });
        }
        bool designPickerOpen() const { return _page==Page::designPicker; }
        void openDesignFiles(bool fromBuild=false)
        {
            TrackRepositoryScan(); Handheld::Designs::ClearPreview();Handheld::Designs::CancelReview();
            _designPickerFromBuild=fromBuild;
            auto folder=fs::u8path(Ui::FileBrowser::GetInitialDirectoryByType(LoadSaveType::track));
            if(!Handheld::Designs::Selection().empty()) folder=fs::u8path(Handheld::Designs::Selection()).parent_path();
            _files.Open(folder,Ui::FileBrowser::GetFilterPatternByType(LoadSaveType::track,false));
            _fileFocus=0;_fileDetail=0;
            for(size_t i=0;i<_files.entries.size();++i)if(_files.entries[i].path.u8string()==Handheld::Designs::Selection())_fileFocus=i+2;
            page(Page::designFiles);
        }
        void openDesignPicker(bool fromBuild = false)
        {
            if(Handheld::Designs::BrowserReturn()) { openDesignFiles(fromBuild);return; }
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
            return _page == Page::terrainMaterialPicker || _page == Page::pathSurfacePicker || _page == Page::sceneryPicker || _page == Page::trackPiecePicker || _page == Page::designPicker
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
            if (_page == Page::sceneryPicker) { refreshPicker(); invalidate(); }
            if (_page == Page::parkMap) { _parkMap.Update(); invalidate(); }
            if (parkInformation()) { _ledger.Clamp(); invalidate(); }
            if (_page == Page::researchCatalog)
            {
                const auto updated=Handheld::ResearchCatalog::Roster(_researchFilter);
                if(updated!=_researchCatalog) {
                    const auto selected=_focus.index()<_researchCatalog.size()?std::optional<Handheld::ResearchCatalog::Target>{_researchCatalog[_focus.index()]}:std::nullopt;
                    const auto found=selected?std::find(updated.begin(),updated.end(),*selected):updated.end();
                    const auto row=found==updated.end()?std::min(_focus.index(),updated.size()):static_cast<size_t>(found-updated.begin());
                    _researchCatalog=updated; _focus={}; _focus.move(static_cast<int>(row),rows().size());
                }
            }
            if (_page == Page::mapSubjects)
            {
                const auto updated = Handheld::MapSubjects::Roster(_parkMap.mode, _parkMap.filter, _parkMap.guestFilter);
                if (updated != _mapSubjects)
                {
                    const auto selected = _focus.index() < _mapSubjects.size()
                        ? std::optional<Handheld::MapSubjects::Target>{_mapSubjects[_focus.index()]} : std::nullopt;
                    const auto found = selected ? std::find(updated.begin(), updated.end(), *selected) : updated.end();
                    const auto row = found == updated.end() ? std::min(_focus.index(), updated.size()) : static_cast<size_t>(found - updated.begin());
                    _mapSubjects = updated; _focus = {}; _focus.move(static_cast<int>(row), rows().size());
                }
            }
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
            if (_page == Page::guestRideHistory && _guestTarget)
            {
                const auto updated = Handheld::GuestManagement::RideHistory(*_guestTarget);
                if (updated != _guestRides)
                {
                    const auto selected = _focus.index() < _guestRides.size() ? std::optional<Handheld::RideManagement::Target>{_guestRides[_focus.index()]} : std::nullopt;
                    const auto found = selected ? std::find(updated.begin(), updated.end(), *selected) : updated.end();
                    const auto row = found == updated.end() ? std::min(_focus.index(), updated.size()) : static_cast<size_t>(found-updated.begin());
                    _guestRides = updated; _guestRideFocus = row; _focus = {}; _focus.move(static_cast<int>(row),rows().size());
                }
                invalidate();
            }
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
                    const auto selected = _focus.index() >= 3 && _focus.index()-3 < _guestRoster.size()
                        ? std::optional<Handheld::GuestManagement::Target>{ _guestRoster[_focus.index()-3] }
                        : std::nullopt;
                    size_t row = std::min(_focus.index(), updated.size()+3);
                    if (selected)
                        if (const auto found = std::find(updated.begin(), updated.end(), *selected); found != updated.end())
                            row = 3+std::distance(updated.begin(), found);
                    _guestRoster = updated;
                    _guestRosterFocus = row;
                    _focus = {};
                    _focus.move(static_cast<int32_t>(row), rows().size());
                    invalidate();
                }
            }
            if (guestManagement() || staffManagement())
                invalidate(); // read native state without clearing native guest invalidation flags
            if (_page == Page::staff)
            {
                const auto updated = Handheld::StaffManagement::Roster(_staffFilter);
                if (updated != _staffRoster)
                {
                    std::optional<Handheld::StaffManagement::Target> selected;
                    if (_focus.index() >= 3 && _focus.index() - 3 < _staffRoster.size())
                        selected = _staffRoster[_focus.index() - 3];
                    size_t newFocus = std::min(_focus.index(), updated.size() + 3);
                    if (selected)
                    {
                        const auto found = std::find(updated.begin(), updated.end(), *selected);
                        if (found != updated.end())
                            newFocus = 3 + std::distance(updated.begin(), found);
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
        Handheld::RideNavigation::Context rideContext() const
        {
            return {*_ridesTarget,_rideTabs,_rideFromPark,_ridesRosterFocus};
        }
        template<typename Begin> void launchRideTool(Begin begin)
        {
            const auto context=rideContext();
            close();
            if(begin()) Handheld::RideNavigation::construction=context;
            else HandheldRideReturn(context.target,context);
        }
        bool quickRideAccess()
        {
            if(_page!=Page::ridesInspect || !_ridesTarget || !Handheld::RideAccess::Supported(*_ridesTarget)) return false;
            _rideTabs.save(_focus.index());
            if(Handheld::FlatRides::CanResume(*_ridesTarget)) openRideAccess(*_ridesTarget);else openAccessManagement();
            return true;
        }
        void returnRide(const Handheld::RideManagement::Target& target, const std::optional<Handheld::RideNavigation::Context>& context)
        {
            if(!Handheld::RideManagement::Resolve(target)) { close();return; }
            openRide(target.id);
            if(context && context->target==target)
            {
                _rideTabs=context->tabs;_rideFromPark=context->fromPark;_ridesRosterFocus=context->rosterFocus;
                _focus={};_focus.move(static_cast<int>(_rideTabs.rows[_rideTabs.tab]),rows().size());invalidate();
            }
        }
        void openRide(RideId id)
        {
            _rideId = id;
            _ridesInspectFocus = 0;
            _rideTabs = {};
            _ridesTabChild = false;
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
            _rideFromPark=true;
            if(isShop) page(Page::shop);
            else
            {
                _dashboard.section=Handheld::Dashboard::Section::rides;
                _dashboard.focus=Handheld::Dashboard::Focus::content;
                page(Page::ridesInspect);
                // Incomplete native access remains a one-press contextual action.
                const auto visible=rows();
                const auto place=std::find(visible.begin(),visible.end(),"Place entrance / exit...");
                if(place!=visible.end()) _focus.move(static_cast<int>(place-visible.begin()),visible.size());
            }
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
                || (guestManagement() && _page != Page::guestName && _page != Page::guestFilterName && _page != Page::guestNameConfirm
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
            _managementNewsReturn=false;
            _guestRideInspection=false;
            _guestMapReturn=false;
            _parkMap.guestFilter.reset();
            if (managementShell())
                page(Page::park);
        }
        void dashboardSwitchPane()
        {
            if (_managementNewsReturn || _guestRideInspection || _guestMapReturn) return;
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
                else if (x > 0)
                    enterDashboardContent();
            }
            else if((_page==Page::staff || _page==Page::guestRoster) && _focus.index()<3)
            {
                if(x) _focus.move(std::clamp<int>(static_cast<int>(_focus.index())+x,0,2)-static_cast<int>(_focus.index()),rows().size());
                if(y>0) move(static_cast<int>(3-_focus.index()));
                invalidate();
            }
            else if (_page == Page::ridesInspect || _page == Page::overview
                || (_page==Page::staffInspect && _staffTabs.tab!=1) || (_page==Page::guestInspect && _guestTabs.tab!=1 && _guestTabs.tab!=2))
            {
                const auto count = rows().size();
                const auto current = _focus.index();
                // A two-column grid: horizontal movement stays in its row;
                // vertical movement preserves its column, including the final odd row.
                size_t next = current;
                if (y)
                {
                    const auto candidate = static_cast<int>(current) + y * 2;
                    if (candidate >= 0 && candidate < static_cast<int>(count))
                        next = static_cast<size_t>(candidate);
                    else if (y > 0 && current / 2 < (count - 1) / 2)
                        next = count - 1; // Enter a final short row without wrapping at the edge.
                }
                else if (x < 0 && current % 2) --next;
                else if (x > 0 && current % 2 == 0 && current + 1 < count) ++next;
                _focus.move(static_cast<int>(next) - static_cast<int>(current), count);
                _status.clear();
                invalidate();
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
            else if (_page == Page::ridesName || _page == Page::designName || _page == Page::staffName || _page == Page::guestName || _page == Page::guestFilterName)
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
        void dashboardPage(int direction)
        {
            if (dashboardRailFocused())
            {
                _dashboard.move(direction);
                _status.clear();
                invalidate();
                return;
            }
            if(_page==Page::staffInspect || _page==Page::guestInspect)
            {
                auto& tabs=_page==Page::staffInspect?_staffTabs:_guestTabs;
                tabs.save(_focus.index());const auto remembered=tabs.move(direction);
                if(_page==Page::staffInspect && _staffTarget) if(const auto* staff=Handheld::StaffManagement::Resolve(*_staffTarget)) _ordersBaseline=_ordersDraft=staff->staffOrders;
                _focus={};_focus.move(static_cast<int>(std::min(remembered,rows().size()-1)),rows().size());_status.clear();invalidate();return;
            }
            if (_page == Page::ridesInspect)
            {
                _rideTabs.save(_focus.index());
                const auto remembered = _rideTabs.move(direction);
                _focus = {};
                _focus.move(static_cast<int>(std::min(remembered,rows().size()-1)),rows().size());
                _status.clear();
                invalidate();
                return;
            }
            if (!dashboardCanSwitch() || _managementNewsReturn || _guestRideInspection || _guestMapReturn)
                return;
            Handheld::ManagementPresentation model;
            if (ridesManagement()) model = ridesPresentationFor(_page, _focus.index());
            else if(_page==Page::staff)model=staffPresentation();
            else if(_page==Page::guestRoster)model=guestsPresentation();
            else return;
            const auto workspaceWidth = ContextGetWidth() - 2 * Handheld::Layout::margin;
            const auto capacity = Handheld::MeasureManagement(
                {workspaceWidth, Handheld::Layout::ContentHeight({ContextGetWidth(), ContextGetHeight()})}, model).capacity;
            const auto current = _focus.index();
            const auto listStart=model.denseRoster?3:0;
            const auto next = std::clamp<int>(std::max(static_cast<int>(current),listStart) + direction * static_cast<int>(capacity),
                listStart, static_cast<int>(rows().size()) - 1);
            _focus.move(next - static_cast<int>(current), rows().size());
            _status.clear();
            invalidate();
        }
        void saveStaffReturn()
        {
            _staffTabs.save(_focus.index());
            Handheld::PeopleNavigation::staff=Handheld::PeopleNavigation::StaffContext{*_staffTarget,_staffTabs,_staffRosterFocus,_staffFilter,_staffFromPark};
        }
        void saveGuestReturn()
        {
            _guestTabs.save(_focus.index());
            Handheld::PeopleNavigation::guest=Handheld::PeopleNavigation::GuestContext{*_guestTarget,_guestTabs,_guestRosterFocus,_guestFilter,_guestFromPark,static_cast<int>(_guestRosterParent),_guestRosterRideParent};
        }
        void openStaffPatrolOptions()
        {
            page(Page::staffPatrolOptions);
        }
        void openGuest(const Handheld::GuestManagement::Target& person)
        {
            _guestRideInspection = false;
            _guestTabs={};_guestFromPark=true;
            if(const auto context=Handheld::PeopleNavigation::guest;context && context->target==person) {
                _guestTabs=context->tabs;_guestRosterFocus=context->rosterFocus;_guestFilter=context->filter;_guestFromPark=context->fromPark;_guestRosterParent=static_cast<Page>(context->rosterParent);_guestRosterRideParent=context->rideParent;
            }
            Handheld::PeopleNavigation::guest.reset();
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
            _staffTabs={};_staffFromPark=target.has_value();
            if(const auto context=Handheld::PeopleNavigation::staff;target && context && context->target==*target) {
                _staffTabs=context->tabs;_staffRosterFocus=context->rosterFocus;_staffFilter=context->filter;_staffFromPark=context->fromPark;
            }
            Handheld::PeopleNavigation::staff.reset();
            _staffTarget = target;
            if(target)if(const auto* staff=Handheld::StaffManagement::Resolve(*target))_ordersBaseline=_ordersDraft=staff->staffOrders;
            if (!target)
                _staffRosterFocus = 0;
            _dashboard.section = Handheld::Dashboard::Section::staff;
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
                _staffTabs={};_staffFromPark=false;
                _staffTarget = Handheld::StaffManagement::Identify(
                    result.getData<GameActions::StaffHireNewActionResult>().StaffEntityId);
                if (_hireManual && _staffTarget)
                {
                    const auto employee = *_staffTarget;
                    saveStaffReturn();close();
                    if(!Handheld::BeginPeepPlacement({ {}, employee }))HandheldStaffOpen(employee);
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
            if ((_page == Page::parkFiles || _page == Page::saveFiles || _page == Page::designFiles)) { _fileFocus = _focus.index(); _fileDetail = 0; }
            _status.clear();
            invalidate();
        }
        void back()
        {
            if (_guestRideInspection && (_page == Page::ridesInspect || _page == Page::ridesRoster))
            {
                _guestRideInspection = false;
                _dashboard.section = Handheld::Dashboard::Section::guests;
                _dashboard.focus = Handheld::Dashboard::Focus::content;
                page(Page::guestRideHistory);
                return;
            }
            if (_managementNewsReturn && (_page==Page::finances || _page==Page::marketing || _page==Page::researchItem || _page==Page::researchCatalog))
            {
                _managementNewsReturn=false; _dashboard.section=Handheld::Dashboard::Section::overview;
                _dashboard.focus=Handheld::Dashboard::Focus::content; page(Page::parkNewsDetail); return;
            }
            if (_mapInspection && (_page == Page::guestInspect || _page == Page::staffInspect || _page == Page::ridesInspect
                || _page == Page::guestRoster || _page == Page::staff || _page == Page::ridesRoster || _page == Page::shop))
            {
                _mapInspection = false;
                _dashboard.section = Handheld::Dashboard::Section::overview;
                _dashboard.focus = Handheld::Dashboard::Focus::content;
                page(_mapInspectionReturn);
                return;
            }
            switch (_page)
            {
                case Page::researchCatalog:
                    page(Page::research); break;
                case Page::researchItem:
                    page(Page::researchCatalog); _focus.move(static_cast<int>(_researchCatalogFocus),rows().size()); break;
                case Page::financeHistory:
                case Page::financeLedger:
                    page(Page::finances); break;
                case Page::mapSubject:
                    page(Page::mapSubjects);
                    _focus.move(static_cast<int>(_mapSubjectsFocus), rows().size());
                    break;
                case Page::mapSubjects:
                    page(Page::parkMap);
                    break;
                case Page::parkMap:
                    if (_guestMapReturn)
                    {
                        _guestMapReturn = false;
                        _dashboard.section = Handheld::Dashboard::Section::guests;
                        page(Page::guestRoster);
                        _focus.move(static_cast<int>(_guestRosterFocus), rows().size());
                    }
                    else { _dashboard.section = Handheld::Dashboard::Section::overview; page(Page::overview); }
                    break;
                case Page::parkNewsDetail:
                    page(Page::parkNews);
                    _focus.move(static_cast<int>(_parkNewsFocus), rows().size());
                    break;
                case Page::parkNews:
                case Page::parkAwards:
                case Page::parkHistory:
                    _dashboard.section = Handheld::Dashboard::Section::overview;
                    page(Page::overview);
                    break;
                case Page::sceneryCategory:
                    page(Page::build);
                    break;
                case Page::sceneryPicker:
                    if(_sceneryPickerDirect) close();else page(Page::sceneryOptions);
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
                case Page::scenerySearch:
                    page(_scenerySearchParent);
                    if (_scenerySearchParent == Page::sceneryPicker) refreshPicker();
                    else _focus.move(static_cast<int>(_scenerySearchRow),rows().size());
                    break;
                case Page::sceneryName:
                    page(Page::sceneryOptions);
                    break;
                case Page::sceneryConfirm:
                    page(Page::sceneryName);
                    break;
                case Page::terrainPaintConfirm: _paintDraft.reset(); close(); break;
                case Page::sceneryClearConfirm: _clearDraft.reset(); close(); break;
                case Page::sceneryClearOptions: close(); break;
                case Page::sceneryGroups:
                case Page::sceneryStyle:
                    page(Page::sceneryOptions);
                    break;
                case Page::sceneryStyleEditor:
                    page(Page::sceneryStyle);
                    break;
                case Page::sceneryStyleConfirm:
                    page(Page::sceneryStyleEditor);
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
                case Page::shopAppearanceEditor: page(Page::shopAppearance);_focus.move(static_cast<int>(_shopAppearanceChannel),rows().size());break;
                case Page::shopAppearanceReview: {
                    page(Page::shopAppearanceEditor);
                    if(_shopAppearanceDraft){const auto& d=*_shopAppearanceDraft;const auto it=std::find(d.setting.values.begin(),d.setting.values.end(),d.value);if(it!=d.setting.values.end())_focus.move(static_cast<int>(it-d.setting.values.begin()),rows().size());}
                    break;
                }
                case Page::shopAppearance:
                case Page::shopPrices:
                case Page::shopDemolish:
                    page(Page::shop);
                    break;
                case Page::terrainMaterialPicker:
                    close();break;
                case Page::pathSurfacePicker:
                    if (_pathSurfaceDirect) close();
                    else page(Page::pathOptions);
                    break;
                case Page::pathUndoConfirm:
                    _pathUndo.reset();
                    page(Page::pathOptions);
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
                case Page::trackContext:
                case Page::trackRemoveConfirm:
                case Page::trackReplaceConfirm:
                case Page::trackSpeedConfirm:
                    page(Page::trackBlock);
                    break;
                case Page::ridesInspect:
                    if(_rideFromPark) close();else page(Page::ridesRoster);
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
                case Page::accessManage:
                    page(Page::ridesInspect); break;
                case Page::accessPoint:
                    openAccessManagement(); break;
                case Page::accessReview:
                    if (!_ridesBusy) cancelAccessReview();
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
                    page(_ridesTabChild ? Page::ridesInspect : Page::ridesStats);
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
                    if (_guestRosterParent == Page::ridesCustomers)
                    { _dashboard.section = Handheld::Dashboard::Section::rides; _ridesTarget = _guestRosterRideParent; }
                    page(_guestRosterParent);
                    break;
                case Page::guestInspect:
                    if(_guestFromPark)close();else page(Page::guestRoster);
                    break;
                case Page::guestText:
                    page(
                        _guestTarget && Handheld::GuestManagement::Resolve(*_guestTarget) ? _guestTextParent
                                                                                          : Page::guestRoster);
                    break;
                case Page::guestFilterName:
                    page(Page::guestRoster);
                    break;
                case Page::guestNameConfirm:
                    page(Page::guestName);
                    break;
                case Page::guestName:
                case Page::guestTracking:
                case Page::guestRideHistory:
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
                    page(Page::staff);break;
                case Page::staffInspect:
                    if(_staffFromPark)close();else page(Page::staff);
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
                case Page::parkInspect: page(Page::parkFiles); break;
                case Page::designVehicles: page(_designVehicleFromFile?Page::designInspect:Page::designOptions); break;
                case Page::designWarnings: page(Page::designOptions);break;
                case Page::designInspect: _designMetadata.reset();_designFilePreview.reset();page(Page::designFiles);break;
                case Page::designFiles: page(Page::designPicker);refreshPicker();break;
                case Page::saveFiles: _saveDraft.reset(); page(Page::save); break;
                case Page::saveName: _saveDraft.reset(); page(Page::saveFiles); break;
                case Page::saveReview: _saveDraft.reset(); page(Page::saveName); break;
                case Page::parkFiles: page(Page::load); break;
                case Page::discard:
                    page(_fileLoad ? Page::parkFiles : Page::load);
                    break;
                case Page::exit:
                    page(Page::park);
                    break;
                case Page::build:
                    close();
                    break;
                case Page::park:
                    resumePark();
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
            if (_shopAppearanceBusy) return;
            if ((overviewManagement() && _overviewBusy) || (ridesManagement() && _ridesBusy)
                || (marketingManagement() && _marketingBusy))
                return;
            if (ridesManagement() && _page != Page::ridesRoster
                && (!_ridesTarget || !Handheld::RideManagement::Resolve(*_ridesTarget)))
            {
                page(Page::ridesRoster);
                return;
            }
            auto choice = _focus.index();
            if(_page==Page::guestRoster) choice=choice<3 ? _guestRoster.size()+choice : choice==_guestRoster.size()+3 ? _guestRoster.size()+3 : choice-3;
            auto actionPage=_page;
            if(_page==Page::staffInspect) {
                const auto model=staffPresentation();
                if(choice<model.rowEnabled.size() && !model.rowEnabled[choice]){_status="Unavailable for this role or current state";invalidate();return;}
                _staffTabs.save(choice);
                if(_staffTabs.tab==1) {
                    if(!_staffTarget || Handheld::StaffManagement::Orders(_staffTarget->type).empty()){_staffTabs.tab=0;page(Page::staffInspect);return;}
                    actionPage=Page::staffOrders;
                } else {const auto indices=Handheld::PeopleTabs::StaffActions(_staffTabs.tab);if(choice>=indices.size())return;choice=indices[choice];}
            }
            if(_page==Page::guestInspect) {
                _guestTabs.save(choice);
                if(_guestTabs.tab==1){_guestTabs.tab=0;page(Page::guestInspect);return;}
                if(_guestTabs.tab==2) {
                    const auto* guest=_guestTarget?Handheld::GuestManagement::Resolve(*_guestTarget):nullptr;
                    const auto thoughts=guest?Handheld::GuestManagement::Thoughts(*guest):std::vector<std::string>{};
                    if(choice>=thoughts.size()){_guestTabs.tab=0;page(Page::guestInspect);return;}
                    _guestText=thoughts[choice];_guestTextParent=Page::guestInspect;_guestTextPage=0;page(Page::guestText);return;
                }
                const auto indices=Handheld::PeopleTabs::GuestActions(_guestTabs.tab);if(choice>=indices.size())return;choice=indices[choice];
            }
            switch (actionPage)
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
                    else if (choice==7) { openDesignVehicles(false);break; }
                    else if (choice==8) { _designDetail=0;page(Page::designWarnings);break; }
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
                            const auto target = Handheld::CustomTracks::Target();
                            const auto context = Handheld::RideNavigation::Take();
                            Handheld::EndTerrain();
                            close();
                            if (target) HandheldRideReturn(*target, context);
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
                case Page::terrainMaterialPicker:
                    if (_picker.selected() && Handheld::TerrainMaterials::Select(_terrainMaterialEdge,_picker.selected()->identity)) close();
                    else {refreshPicker();_status="Material changed; select again";invalidate();}
                    break;
                case Page::pathSurfacePicker:
                    if (_picker.selected() && Handheld::PathSurfaces::Select(_picker.selected()->identity))
                        close();
                    else { refreshPicker(); _status = "Surface no longer available"; invalidate(); }
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
                            resumePark();
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
                    if (choice == 5) {close();Handheld::BeginSceneryClear();}
                    else if (choice == 6)
                        page(Page::build);
                    else
                    {
                        Handheld::SceneryTools::SelectCategory(static_cast<Handheld::SceneryTools::Category>(choice));
                        page(Page::sceneryPicker);
                        refreshPicker();
                    }
                    break;
                case Page::sceneryPicker:
                    if (_picker.selected() && Handheld::SceneryTools::Select(_picker.selected()->identity.index, _picker.selected()->identity.key))
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
                case Page::scenerySearch:
                    if (choice == 47) back();
                    else if (choice == 46)
                    {
                        if (Handheld::SceneryTools::SetSearch(_nameEntry.text))
                        { page(Page::sceneryPicker); refreshPicker(); }
                        else { _status = "Search is limited to 32 characters"; invalidate(); }
                    }
                    else { _nameEntry.Press(choice); invalidate(); }
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
                case Page::sceneryClearOptions:
                    if(choice==0)close();else{Handheld::SceneryClear::Configure(choice);invalidate();}break;
                case Page::terrainPaintConfirm:
                    if (choice==1) {_paintDraft.reset();close();}
                    else if (_paintDraft)
                    {
                        const auto result=Handheld::AreaTools::ApplyPaint(*_paintDraft,true);
                        if (result.error==GameActions::Status::ok) {_paintDraft.reset();close();}
                        else {_status=result.getErrorMessage();invalidate();}
                    }
                    break;
                case Page::sceneryClearConfirm:
                    if(choice==1){_clearDraft.reset();close();}
                    else if(_clearDraft){const auto result=Handheld::SceneryClear::Action(*_clearDraft,true);
                        if(result.error==GameActions::Status::ok){_clearDraft.reset();close();}else{_status=result.getErrorMessage();invalidate();}}
                    break;
                case Page::sceneryGroups:
                {
                    const auto groups = Handheld::SceneryTools::Groups(Handheld::SceneryTools::Current().category);
                    if (choice < groups.size() && Handheld::SceneryTools::SetGroup(groups[choice].index))
                    { page(Page::sceneryPicker); refreshPicker(); }
                    else { _status = "Scenery group changed"; invalidate(); }
                    break;
                }
                case Page::sceneryStyle:
                {
                    const auto styles = signStyles();
                    if (choice >= styles.size()) page(Page::sceneryOptions);
                    else if (_signTarget)
                    {
                        _signStyle = Handheld::SceneryTools::BeginStyle(*_signTarget, styles[choice]);
                        if (_signStyle) page(Page::sceneryStyleEditor);
                        else { _status = "Style unavailable for this sign"; invalidate(); }
                    }
                    break;
                }
                case Page::sceneryStyleEditor:
                    if (choice == 2) page(Page::sceneryStyle);
                    else if (_signStyle && choice == 0) { Handheld::SceneryTools::AdjustStyle(*_signStyle, 1); invalidate(); }
                    else if (_signStyle)
                    {
                        const auto result = Handheld::SceneryTools::ApplyStyle(*_signStyle, false);
                        if (result.error == GameActions::Status::ok) { page(Page::sceneryStyleConfirm); _focus.move(1, 2); }
                        else { _status = result.getErrorMessage(); invalidate(); }
                    }
                    break;
                case Page::sceneryStyleConfirm:
                    if (choice == 1) page(Page::sceneryStyleEditor);
                    else if (_signStyle)
                    {
                        const auto result = Handheld::SceneryTools::ApplyStyle(*_signStyle, true);
                        if (result.error == GameActions::Status::ok) { _signTarget.reset(); _signStyle.reset(); page(Page::sceneryOptions); }
                        else { _status = result.getErrorMessage(); invalidate(); }
                    }
                    break;
                case Page::sceneryOptions:
                    if (choice == 15) openScenerySearch();
                    else if (choice == 14) { Handheld::SceneryTools::BeginPick(); close(); }
                    else if (choice == 12) page(Page::sceneryGroups);
                    else if (choice == 13)
                    {
                        _signTarget = Handheld::SceneryEditTarget();
                        if (_signTarget && _signTarget->banner != BannerIndex::GetNull()) page(Page::sceneryStyle);
                        else { _status = "Point at a banner or sign"; invalidate(); }
                    }
                    else if (choice == 11)
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
                case Page::pathUndoConfirm:
                    if (choice == 1) { _pathUndo.reset(); page(Page::pathOptions); }
                    else if (_pathUndo)
                    {
                        const auto result = Handheld::Paths::Undo(*_pathUndo, true);
                        if (result.error == GameActions::Status::ok) { _pathUndo.reset(); close(); }
                        else { _status = result.getErrorMessage(); invalidate(); }
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
                        else if (choice == 6)
                            openPathSurfacePicker();
                        else if (choice == 7)
                        {
                            if (_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL)) { PauseToggle(); _pathUndoResumed = true; }
                            _pathUndo = Paths::BeginUndo();
                            if (_pathUndo) { page(Page::pathUndoConfirm); _focus.move(1, 2); }
                            else
                            {
                                if (_pathUndoResumed && _pausedHere && !(gGamePaused & GAME_PAUSED_NORMAL)) PauseToggle();
                                _pathUndoResumed = false;
                                _status = "No unchanged new section to undo";
                            }
                        }
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
                            openPathSurfacePicker();
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
                    if(choice==0)page(Page::staffRoles);
                    else if(choice>=3 && choice-3<_staffRoster.size())
                    {
                        _staffTarget=_staffRoster[choice-3];_staffTabs={};_staffFromPark=false;
                        _ordersBaseline=_ordersDraft=Handheld::StaffManagement::Resolve(*_staffTarget)->staffOrders;
                        page(Page::staffInspect);
                    }
                    else if (choice == 1)
                    {
                        _staffFilter = !_staffFilter ? std::optional<StaffType>(StaffType::handyman)
                            : *_staffFilter == StaffType::entertainer
                            ? std::nullopt
                            : std::optional<StaffType>(static_cast<StaffType>(EnumValue(*_staffFilter) + 1));
                        _staffRosterFocus = 0;
                        page(Page::staff);
                    }
                    else if (choice == 2)
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
                        saveStaffReturn();close();
                        if(!Handheld::BeginPeepPlacement({ {}, person }))HandheldStaffOpen(person);
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
                        saveStaffReturn();close();
                        if(!Handheld::BeginStaffPatrol(employee))HandheldStaffOpen(employee);
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
                            if (result.error == GameActions::Status::ok) {
                                _ordersBaseline=_ordersDraft;
                                page(Page::staffInspect);_status="Orders submitted";
                            }
                            else
                                _status = Handheld::Finances::Feedback(result);
                        }
                        else {
                            _ordersBaseline=_ordersDraft=Handheld::StaffManagement::Resolve(*_staffTarget)->staffOrders;
                            page(Page::staffInspect);
                        }
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
                case Page::financeHistory:
                    if (choice == 0) _financeGraph=(_financeGraph+1)%3;
                    else if (choice == 1) _financeSample=std::min<size_t>(63,_financeSample+1);
                    else if (choice == 2 && _financeSample) --_financeSample;
                    else if (choice == 3) page(Page::finances);
                    invalidate(); break;
                case Page::financeLedger:
                    if (choice == 14) _ledger.Move(-1);
                    else if (choice == 15) _ledger.Move(1);
                    else if (choice == 16) page(Page::finances);
                    invalidate(); break;
                case Page::finances:
                    if (choice == 2 && _managementNewsReturn) { back(); break; }
                    if (choice == 4) { _financeGraph=0; _financeSample=0; page(Page::financeHistory); }
                    else if (choice == 3) { _ledger.Begin(); page(Page::financeLedger); }
                    else if (choice == 2)
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
                case Page::mapSubjects:
                    if (choice < _mapSubjects.size())
                    { _mapTarget = _mapSubjects[choice]; _mapSubjectsFocus = choice; page(Page::mapSubject); }
                    else if (choice == _mapSubjects.size() && _parkMap.mode >= 2)
                    {
                        _parkMap.filter = (_parkMap.filter + 1) % (_parkMap.mode == 2 ? 2 : 5);
                        _mapSubjects = Handheld::MapSubjects::Roster(_parkMap.mode, _parkMap.filter, _parkMap.guestFilter);
                        _focus = {}; invalidate();
                    }
                    else page(Page::parkMap);
                    break;
                case Page::mapSubject:
                    if (choice == 1 && _mapTarget) inspectMapSubject(*_mapTarget, Page::mapSubject);
                    else if ((choice == 0 || choice == 2) && _mapTarget)
                    {
                        const auto location = Handheld::MapSubjects::Location(*_mapTarget);
                        if (location && MapIsLocationValid(*location))
                        {
                            if (choice == 0) { _parkMap.focus = *location; _parkMap.Clamp(); page(Page::parkMap); }
                            else if (auto* main = WindowGetMain()) { close(); WindowScrollToLocation(*main, *location); }
                        }
                        else { _status = "Subject departed or location unavailable"; invalidate(); }
                    }
                    else back();
                    break;
                case Page::parkNews:
                    if (choice < _parkNews.size())
                    {
                        _parkNewsTarget = _parkNews[choice]; _parkNewsFocus = choice; _parkNewsPage = 0;
                        page(Page::parkNewsDetail);
                    }
                    else { _parkNews = Handheld::ParkInformation::NewsList(); invalidate(); }
                    break;
                case Page::parkNewsDetail:
                    if (choice == 0 && _parkNewsTarget)
                    {
                        const auto count = Handheld::GuestManagement::TextLines(Handheld::ParkInformation::PlainText(_parkNewsTarget->item.text)).size();
                        _parkNewsPage = (_parkNewsPage + 1) % std::max<size_t>(1, (count + 5) / 6); invalidate();
                    }
                    else if (choice == 1 && _parkNewsTarget)
                    {
                        const auto location = Handheld::ParkInformation::Location(*_parkNewsTarget);
                        if (location && WindowGetMain())
                        { auto* main = WindowGetMain(); close(); WindowScrollToLocation(*main, *location); }
                        else { _status = "News expired or subject unavailable"; invalidate(); }
                    }
                    else if (choice == 2 && _parkNewsTarget)
                    {
                        const auto link=Handheld::ManagementNews::Available(*_parkNewsTarget);
                        if(link!=Handheld::ManagementNews::Link::none)
                        {
                            if(link==Handheld::ManagementNews::Link::finance) { _dashboard.section=Handheld::Dashboard::Section::finances; page(Page::finances); }
                            else if(link==Handheld::ManagementNews::Link::marketing) { _dashboard.section=Handheld::Dashboard::Section::marketing; page(Page::marketing); }
                            else { _researchItem=Handheld::ManagementNews::ResearchTarget(*_parkNewsTarget); _researchCatalog=Handheld::ResearchCatalog::Roster(0); _researchFilter=0; page(Page::researchItem); }
                            _managementNewsReturn=true;
                        }
                        else if (!Handheld::ParkInformation::SubjectCurrent(*_parkNewsTarget)) { _status = "News subject unavailable"; invalidate(); }
                        else if (_parkNewsTarget->ride) inspectMapSubject(*_parkNewsTarget->ride, Page::parkNewsDetail);
                        else if (_parkNewsTarget->guest) inspectMapSubject(*_parkNewsTarget->guest, Page::parkNewsDetail);
                        else if (_parkNewsTarget->staff) inspectMapSubject(*_parkNewsTarget->staff, Page::parkNewsDetail);
                    }
                    else back();
                    break;
                case Page::parkAwards:
                    if (choice + 1 == rows().size()) page(Page::overview);
                    break;
                case Page::parkHistory:
                    if (choice == 0) { _parkGuestHistory = !_parkGuestHistory; invalidate(); }
                    else page(Page::overview);
                    break;
                case Page::parkMap:
                    if (auto* main = WindowGetMain())
                    {
                        const auto location = _parkMap.focus;
                        close();
                        WindowScrollToLocation(*main, CoordsXYZ{ location, MapGetHighestZ(location) });
                    }
                    break;
                case Page::overview:
                    if (choice >= 3 && choice <= 5)
                    {
                        Handheld::EndTerrain();
                        _parkNews = Handheld::ParkInformation::NewsList();
                        page(choice == 3 ? Page::parkNews : choice == 4 ? Page::parkAwards : Page::parkHistory);
                    }
                    else if (choice == 2)
                    {
                        Handheld::EndTerrain();
                        _parkMap.Begin();
                        page(Page::parkMap);
                    }
                    else if (choice == 0)
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
                    _rideFromPark=false;
                    _ridesRosterFocus = choice;
                    _ridesInspectFocus = 0;
                    _rideTabs = {};
                    _ridesTabChild = false;
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
                case Page::trackContext:
                    if (!_trackBlock || !Handheld::TrackEditing::Valid(*_trackBlock)
                        || choice + 1 == Handheld::TrackEditing::Inspection(*_trackBlock).size()) page(Page::trackBlock);
                    break;
                case Page::trackBlock:
                    if (!_trackBlock || !Handheld::TrackEditing::Valid(*_trackBlock))
                    {
                        if (_ridesTarget)
                            _trackBlocks = Handheld::TrackEditing::Blocks(*_ridesTarget);
                        page(Page::trackBlocks);
                        _status = "Block changed; select again";
                    }
                    else if (ridesPresentationFor(Page::trackBlock, choice).rows[choice] == "Inspect connections / station...")
                        page(Page::trackContext);
                    else if (ridesPresentationFor(Page::trackBlock, choice).rows[choice] == "Continue section...")
                    {
                        const auto block = *_trackBlock;
                        launchRideTool([=] { return Handheld::ResumeCustomTrackFrom(block.ride, block.origin, block.type); });
                        if (!Handheld::CustomTracks::Active())
                            GetWindowManager()->ShowError(
                                "Cannot continue section", "No compatible forward endpoint; select again");
                    }
                    else if (ridesPresentationFor(Page::trackBlock, choice).rows[choice] == "Build backward...")
                    {
                        const auto block = *_trackBlock;
                        launchRideTool([=] { return Handheld::ResumeCustomTrackFrom(block.ride, block.origin, block.type, true); });
                        if (!Handheld::CustomTracks::Active())
                            GetWindowManager()->ShowError("Cannot build backward", "No compatible backward endpoint; select again");
                    }
                    else if (ridesPresentationFor(Page::trackBlock,choice).rows[choice] == "Replace block...")
                    {
                        _trackPrevious = Handheld::TrackEditing::Previous(*_trackBlock);
                        const auto result = Handheld::TrackEditing::RemoveForReplacement(*_trackBlock);
                        if (result.error == GameActions::Status::ok) { _trackCost=result.cost; _trackContext=Handheld::TrackEditing::Context(*_trackBlock); page(Page::trackReplaceConfirm); }
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
                            _trackContext = Handheld::TrackEditing::Context(*_trackBlock);
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
                    else if (_trackBlock)
                    {
                        const auto block = *_trackBlock;
                        const auto cost = _trackCost;
                        const auto context = _trackContext;
                        const auto navigation=rideContext();
                        close();
                        const auto result = Handheld::TrackEditing::RemoveForReplacement(block, cost, true,
                            [block,navigation](const auto*, const GameActions::Result* outcome) {
                                if (outcome->error != GameActions::Status::ok)
                                {
                                    HandheldRideOpen(block.ride.id);
                                    GetWindowManager()->ShowError("Cannot remove for rebuild", outcome->getErrorMessage());
                                    return;
                                }
                                if (!Handheld::ResumeCustomTrackReplacement(block.ride,
                                    { block.origin, block.type, block.speed, block.seat, block.chain, block.inverted }, block.colour))
                                {
                                    HandheldRideOpen(block.ride.id);
                                    GetWindowManager()->ShowError("Track removed", "Use native Continue track / Access to recover");
                                }
                                else Handheld::RideNavigation::construction=navigation;
                            }, context);
                        if (result.error != GameActions::Status::ok)
                        {
                            HandheldRideOpen(block.ride.id);
                            GetWindowManager()->ShowError("Cannot remove for rebuild", result.getErrorMessage());
                        }
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
                        const auto context = _trackContext;
                        close();
                        const auto result = Handheld::TrackEditing::Change(block, speed, cost, true, context);
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
                    _rideTabs.save(_focus.index());
                    if (_rideTabs.tab == Handheld::RideTabs::statistics)
                    {
                        _ridesTabChild = true;
                        if (choice == 2) Handheld::RideManagement::BeginGraphs(*_ridesTarget);
                        page(choice == 0 ? Page::ridesCustomers : choice == 1 ? Page::ridesEarnings : Page::ridesGraphs);
                        break;
                    }
                    auto nativeModel = Handheld::RideManagement::Presentation(
                        Handheld::RideManagement::View::inspection,0,_ridesRoster,_ridesTarget);
                    const auto indices = Handheld::RideTabs::Indices(nativeModel,_rideTabs.tab);
                    if (choice >= indices.size()) break;
                    choice = indices[choice];
                    auto rows = nativeModel.rows;
                    const auto accessRow = std::find(rows.begin(), rows.end(), "Entrances / exits...");
                    if (accessRow != rows.end())
                    {
                        const auto index = static_cast<size_t>(accessRow - rows.begin());
                        if (choice == index) { openAccessManagement(); break; }
                        rows.erase(accessRow); if (choice > index) --choice;
                    }
                    // Keep the placement handoff first without changing the established action indices.
                    if (Handheld::FlatRides::CanResume(*_ridesTarget))
                    {
                        if (choice == 0)
                        {
                            const auto target = *_ridesTarget;
                            launchRideTool([=] { return Handheld::ResumeFlatRide(target.id); });
                            Handheld::RideManagement::Locate(target);
                            break;
                        }
                        // Reconstruct the old ordering for the existing dispatcher below.
                        rows.erase(rows.begin());
                        rows.insert(rows.begin() + 7, "Place entrance / exit...");
                        if (choice <= 7) --choice;
                    }
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
                        launchRideTool([=] { return Handheld::ResumeMaze(t.id); });
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
                        launchRideTool([=] { return Handheld::ResumeCustomTrack(target.id); });
                    }
                    else if (choice == 7 && Handheld::FlatRides::CanResume(*_ridesTarget))
                    {
                        const auto target = *_ridesTarget;
                        launchRideTool([=] { return Handheld::ResumeFlatRide(target.id); });
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
                case Page::accessManage:
                    if (choice < _accessPoints.size())
                    {
                        _accessFocus = choice; _accessPoint = _accessPoints[choice]; page(Page::accessPoint);
                    }
                    else page(Page::ridesInspect);
                    break;
                case Page::accessPoint:
                    if (choice == 2 && canConnectAccessPoint())
                    {
                        const auto point = *_accessPoint;
                        // BeginPaths revalidates native access and locates its outside landing.
                        // Retain this page on a failed handoff instead of silently closing it.
                        const auto context=rideContext();
                        if (Handheld::RideAccess::BeginPaths(point.target, point.exit, true))
                        {Handheld::RideNavigation::construction=context;close();}
                        else { _status = "Cannot start path here; return to ride inspection"; invalidate(); }
                    }
                    else if (!_accessPoint || !Handheld::RideAccess::Current(*_accessPoint) || choice >= 2) openAccessManagement();
                    else if (choice == 0)
                    {
                        const auto point = *_accessPoint;
                        if (Handheld::RideManagement::Resolve(point.target)->status != RideStatus::closed)
                        { _status = "Close ride before moving access"; invalidate(); break; }
                        launchRideTool([=] { return Handheld::RideAccess::BeginRelocation(point); });
                        if (Handheld::FlatRides::CurrentStage()==Handheld::FlatRides::Stage::inactive)
                            GetWindowManager()->ShowError("Cannot move access", "Access changed; select it again");
                        else Handheld::RideManagement::Locate(point.target);
                    }
                    else
                    {
                        if (_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL)) { PauseToggle(); _ridesReviewResumed = true; }
                        std::string error;
                        auto draft = Handheld::RideAccess::Review(*_accessPoint, {}, error);
                        if (draft) openAccessReview(*draft);
                        else { _status = error; invalidate(); }
                    }
                    break;
                case Page::accessReview:
                    if (choice != 0) { cancelAccessReview(); break; }
                    if (_accessDraft)
                    {
                        if (_pausedHere && (gGamePaused & GAME_PAUSED_NORMAL)) { PauseToggle(); _ridesReviewResumed = true; }
                        auto action = _accessDraft->action;
                        const auto request = ++_ridesRequest;
                        const auto target = _accessDraft->point.target;
                        _ridesBusy = true;
                        action.SetCallback([request,target,lifetime=std::weak_ptr<int>(_ridesLifetime)](const auto*, const GameActions::Result* result) {
                            const auto alive = lifetime.lock();
                            if (!alive) return;
                            auto* base = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
                            if (!base) return;
                            auto* window = static_cast<HandheldMenuWindow*>(base);
                            if (window->_ridesLifetime != alive || window->_ridesRequest != request || window->_page != Page::accessReview) return;
                            window->_ridesBusy = false;
                            if (result->error == GameActions::Status::ok)
                            {
                                Handheld::EndTerrain(); window->_accessDraft.reset(); window->openRideAccess(target);
                                window->_status = "Access edited; check queue / path connections";
                            }
                            else window->_status = result->getErrorMessage().empty() ? result->getErrorTitle() : result->getErrorMessage();
                            window->invalidate();
                        });
                        const auto result = GameActions::Execute(&action, getGameState());
                        if (result.error != GameActions::Status::ok) { _ridesBusy = false; _status = result.getErrorMessage().empty() ? result.getErrorTitle() : result.getErrorMessage(); }
                        invalidate();
                    }
                    break;
                case Page::ridesAccess:
                    if (_ridesTarget && Handheld::FlatRides::CanResume(*_ridesTarget))
                    {
                        if (choice == 0)
                        {
                            const auto target = *_ridesTarget;
                            launchRideTool([=] { return Handheld::ResumeFlatRide(target.id); });
                            Handheld::RideManagement::Locate(target);
                        }
                        else if (choice == 1) page(Page::ridesInspect);
                        else close();
                        break;
                    }
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
                        const auto exit=choice==2;
                        launchRideTool([=] {return Handheld::RideAccess::BeginPaths(target,exit);});
                    }
                    else if (choice == 5 || choice == 6)
                    {
                        const auto target = *_ridesTarget;
                        const auto exit=choice==6;
                        launchRideTool([=] {return Handheld::RideAccess::BeginPaths(target,exit,true);});
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
                    if (choice >= 2 && choice <= 4 && _ridesTarget && Handheld::RideManagement::Resolve(*_ridesTarget))
                    {
                        _guestFilter = {};
                        _guestFilter.ride = Handheld::GuestManagement::RideFilter{
                            *_ridesTarget, static_cast<Handheld::GuestManagement::RideFilterKind>(choice - 2) };
                        _guestRosterParent = Page::ridesCustomers;
                        _guestRosterRideParent = _ridesTarget;
                        _guestRosterFocus = 3;
                        _dashboard.section = Handheld::Dashboard::Section::guests;
                        page(Page::guestRoster);
                    }
                    else page(choice == 0 ? Page::ridesStations : Page::ridesStats);
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
                        _guestRosterFocus = 3;
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
                        _guestRosterFocus = 3;
                        page(Page::guestRoster);
                    }
                    break;
                case Page::guestRoster:
                    _guestRosterFocus = _focus.index();
                    if (choice == _guestRoster.size())
                    {
                        _parkMap.Begin();
                        _parkMap.mode = 2;
                        _parkMap.rides = false;
                        _parkMap.filter = _guestFilter.tracked ? 1 : 0;
                        _parkMap.guestFilter = _guestFilter;
                        _parkMap.guestFilter->tracked = false;
                        _guestMapReturn = true;
                        page(Page::parkMap);
                    }
                    else if (choice == _guestRoster.size() + 1)
                    {
                        _nameEntry = { _guestFilter.name };
                        page(Page::guestFilterName);
                    }
                    else if (choice == _guestRoster.size() + 2)
                    {
                        _guestFilter.tracked = !_guestFilter.tracked;
                        page(Page::guestRoster);
                    }
                    else if (choice > _guestRoster.size())
                    {
                        if (_guestRosterParent == Page::ridesCustomers)
                        { _dashboard.section = Handheld::Dashboard::Section::rides; _ridesTarget = _guestRosterRideParent; }
                        page(_guestRosterParent);
                    }
                    else if (Handheld::GuestManagement::Resolve(_guestRoster[choice]))
                    {
                        _guestTarget = _guestRoster[choice];_guestTabs={};_guestFromPark=false;
                        page(Page::guestInspect);
                    }
                    else
                    {
                        _status = "Guest no longer in park";
                        invalidate();
                    }
                    break;
                case Page::guestRideHistory:
                    if (choice >= _guestRides.size()) back();
                    else if (_guestTarget && Handheld::GuestManagement::HistoryRideCurrent(*_guestTarget, _guestRides[choice]))
                    {
                        _guestRideFocus = choice; _ridesTarget = _guestRides[choice]; _guestRideInspection = true;
                        _dashboard.section = Handheld::Dashboard::Section::rides;
                        _dashboard.focus = Handheld::Dashboard::Focus::content; page(Page::ridesInspect);
                    }
                    else { _status = "Ride history changed; choose again"; invalidate(); }
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
                        saveGuestReturn();close();
                        if(!Handheld::BeginPeepPlacement({ person, {} }))HandheldGuestOpen(person);
                        return;
                    }
                    else if (choice == 11)
                    {
                        if (Handheld::GuestFollow::Current() == _guestTarget)
                        { Handheld::GuestFollow::Stop(); _status = "Follow stopped"; invalidate(); }
                        else if (Handheld::GuestFollow::Start(*_guestTarget))
                        { saveGuestReturn();close(); return; }
                        else { _status = "Guest cannot be followed now"; invalidate(); }
                    }
                    else if (choice == 10) { _guestRideFocus = 0; page(Page::guestRideHistory); }
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
                case Page::guestFilterName:
                    if (choice == 47) page(Page::guestRoster);
                    else if (choice == 46)
                    {
                        _guestFilter.name = _nameEntry.text;
                        _guestRosterFocus = 3;
                        page(Page::guestRoster);
                    }
                    else { _nameEntry.Press(choice); invalidate(); }
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
                    if(choice==1 && _managementNewsReturn) { back(); break; }
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
                case Page::researchCatalog:
                    if(choice<_researchCatalog.size()) { _researchItem=_researchCatalog[choice]; _researchCatalogFocus=choice; page(Page::researchItem); }
                    else if(choice==_researchCatalog.size()) { _researchFilter=(_researchFilter+1)%8; _researchCatalog=Handheld::ResearchCatalog::Roster(_researchFilter); _focus={}; invalidate(); }
                    else page(Page::research);
                    break;
                case Page::researchItem:
                    back(); break;
                case Page::research:
                    if(choice==2) { _researchFilter=0; _researchCatalog=Handheld::ResearchCatalog::Roster(0); page(Page::researchCatalog); }
                    else if (choice == 1)
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
                    else if (choice == 4) {page(Page::shopAppearance);_shopAppearanceChannel=0;}
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
                case Page::shopAppearance:
                    if(!_shop || choice>=Handheld::ShopAppearance::Settings(*_shop).size())page(Page::shop);
                    else {
                        _shopAppearanceChannel=choice;
                        _shopAppearanceDraft=Handheld::ShopAppearance::Begin(*_shop,choice);
                        if(_shopAppearanceDraft) {
                            page(Page::shopAppearanceEditor);
                            const auto& d=*_shopAppearanceDraft;const auto it=std::find(d.setting.values.begin(),d.setting.values.end(),d.value);
                            if(it!=d.setting.values.end())_focus.move(static_cast<int>(it-d.setting.values.begin()),rows().size());
                        }
                    }
                    break;
                case Page::shopAppearanceEditor:
                    if(_shopAppearanceDraft && choice<_shopAppearanceDraft->setting.values.size()) {
                        _shopAppearanceDraft->value=_shopAppearanceDraft->setting.values[choice];
                        const auto q=Handheld::ShopAppearance::Action(*_shopAppearanceDraft,false);
                        if(q.error==GameActions::Status::ok)page(Page::shopAppearanceReview);
                        else {_status=q.getErrorMessage();invalidate();}
                    }
                    break;
                case Page::shopAppearanceReview:
                    if(choice==0) {back();break;}
                    if(_shopAppearanceDraft) {
                        const auto q=Handheld::ShopAppearance::Action(*_shopAppearanceDraft,false);
                        if(q.error!=GameActions::Status::ok){_status=q.getErrorMessage();invalidate();break;}
                        _shopAppearanceBusy=true;const auto request=++_shopAppearanceRequest;
                        const auto result=Handheld::ShopAppearance::Action(*_shopAppearanceDraft,true,
                            [this,request,lifetime=std::weak_ptr<int>(_shopAppearanceLifetime)](const auto*,const GameActions::Result* outcome){
                                if(lifetime.expired() || flags.has(WindowFlag::dead) || request!=_shopAppearanceRequest || _page!=Page::shopAppearanceReview)return;
                                _shopAppearanceBusy=false;
                                if(outcome->error==GameActions::Status::ok) {
                                    page(Page::shopAppearance);_focus.move(static_cast<int>(_shopAppearanceChannel),rows().size());_status="Native appearance applied";
                                } else _status=outcome->getErrorMessage();invalidate();
                            });
                        if(result.error!=GameActions::Status::ok){_shopAppearanceBusy=false;_status=result.getErrorMessage();}
                        invalidate();
                    }
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
                    if (_ridesTarget && Handheld::RideAccess::Supported(*_ridesTarget))
                    {
                        const size_t index = Handheld::FlatRides::CanResume(*_ridesTarget) ? 1 : 0;
                        if (choice == index) { openAccessManagement(); break; }
                        if (choice > index) --choice;
                    }
                    if (_ridesTarget && Handheld::FlatRides::CanResume(*_ridesTarget))
                    {
                        if (choice == 0)
                        {
                            const auto target = *_ridesTarget;
                            launchRideTool([=] { return Handheld::ResumeFlatRide(target.id); });
                            Handheld::RideManagement::Locate(target);
                            break;
                        }
                        --choice;
                    }
                    if (_ridesTarget && _focus.index() < rows().size() && rows()[_focus.index()] == "Edit maze...")
                    {
                        const auto t = *_ridesTarget;
                        launchRideTool([=] { return Handheld::ResumeMaze(t.id); });
                    }
                    else if (_ridesTarget && _focus.index() < rows().size() && rows()[_focus.index()] == "Edit track blocks...")
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
                        launchRideTool([=] { return Handheld::ResumeCustomTrack(target.id); });
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
                case Page::saveName:
                    if(choice==47) { _saveDraft.reset(); page(Page::saveFiles); }
                    else if(choice==46) {
                        std::string error;
                        _saveDraft=Handheld::NamedSave::Draft::Review(_files.directory,_nameEntry.text,error);
                        if(_saveDraft) page(Page::saveReview);
                        else { _status=error; invalidate(); }
                    } else { _nameEntry.Press(choice); invalidate(); }
                    break;
                case Page::saveReview:
                    if(choice==0) save(); else { _saveDraft.reset(); page(Page::saveName); }
                    break;
                case Page::saveFiles:
                    if(choice==0) editSaveName(fs::u8path(gScenarioSavePath).stem().u8string());
                    else if(choice==1) { _files.Parent(); _fileFocus=0; page(Page::saveFiles); }
                    else if(choice==2) { _files.Refresh(); _fileFocus=0; _focus={}; invalidate(); }
                    else if(choice-3<_files.entries.size()) {
                        const auto entry=_files.entries[choice-3];
                        if(entry.directory) { _files.Open(entry.path,_files.pattern); _fileFocus=0; page(Page::saveFiles); }
                        else editSaveName(entry.path.stem().u8string());
                    }
                    break;
                case Page::save:
                {
                    if(choice==3) { openSaveFiles(); break; }
                    _namedSave=false; _saveDraft.reset();
                    _slot = choice;
                    std::error_code error;
                    if (fs::exists(SlotPath(_slot), error))
                        page(Page::overwrite);
                    else
                        save();
                    break;
                }
                case Page::parkInspect:
                    if(choice==1) page(Page::parkFiles);
                    else if(!_loadFile || !Handheld::FileBrowsing::Current(*_loadFile)) { _status="File changed. Back and select again.";invalidate(); }
                    else if(gLegacyScene==LegacyScene::playing) page(Page::discard);
                    else load();
                    break;
                case Page::designFiles:
                    if(choice==0) { const auto child=_files.directory;_files.Parent();_fileFocus=0;
                        for(size_t i=0;i<_files.entries.size();++i)if(_files.entries[i].path==child)_fileFocus=i+2;
                        page(Page::designFiles);
                    } else if(choice==1) { TrackRepositoryScan();_files.Refresh();_focus={};_fileFocus=0;invalidate(); }
                    else if(choice-2<_files.entries.size()) {
                        const auto entry=_files.entries[choice-2];
                        if(entry.directory) {_files.Open(entry.path,_files.pattern);_fileFocus=0;page(Page::designFiles);}
                        else {
                            _designFile=Handheld::FileBrowsing::Capture(entry.path);
                            _designChoice.reset();_designDetail=0;
                            _designMetadata.reset();_designFilePreview.reset();_designFileStatus.clear();
                            if(_designFile) _designMetadata=TrackDesignImport(entry.path.u8string().c_str());
                            if(!_designFile) _designFileStatus="File removed or unreadable";
                            else if(!_designMetadata) _designFileStatus="Native design parser rejected this file";
                            else if(!Handheld::Designs::Eligible(*_designMetadata)) _designFileStatus=Handheld::Designs::UnavailableReason(*_designMetadata);
                            else _designFilePreview=Handheld::Designs::FileEntry(entry.path.u8string());
                            page(Page::designInspect);
                        }
                    }
                    break;
                case Page::designWarnings:
                    if (choice+1==rows().size()) page(Page::designOptions);
                    break;
                case Page::designVehicles: {
                    if (choice>=_designVehicles.size()) {page(_designVehicleFromFile?Page::designInspect:Page::designOptions);break;}
                    const auto* td=_designVehicleFromFile?_designMetadata.get():Handheld::Designs::Draft();
                    if (!td || _designVehicleContext!=TrackDesignPlacementContext::Capture(*td)
                        || (_designVehicleFromFile && (!_designFile || !Handheld::FileBrowsing::Current(*_designFile)))) {
                        _status="Objects or file changed. Back and choose again.";invalidate();break;
                    }
                    const auto chosen=_designVehicles[choice];
                    if (_designVehicleFromFile) {
                        _designChoice=chosen;
                        _designFilePreview=Handheld::Designs::FileEntry(_designFile->path.u8string(),chosen);
                        page(Page::designInspect);
                    } else if (Handheld::Designs::ChooseVehicle(chosen)) page(Page::designOptions);
                    else {_status="Vehicle unavailable. Back and choose again.";invalidate();}
                    break;
                }
                case Page::designInspect:
                    if(choice==1) page(Page::designFiles);
                    else if(choice==2) { openDesignVehicles(true); }
                    else if(choice>2) { invalidate(); }
                    else if(!_designFile || !Handheld::FileBrowsing::Current(*_designFile)) { _status="Design file changed. Back and select again.";invalidate(); }
                    else if(!fileDesignEligible()) { _status=_designFileStatus;invalidate(); }
                    else if(Handheld::BeginSavedDesign(_designFile->path.u8string(),_designChoice)) {
                        Handheld::Designs::SetBrowserReturn(true); close();
                    } else { _status="Native placement unavailable. Back and inspect again.";invalidate(); }
                    break;
                case Page::parkFiles:
                {
                    if (choice == 0) {
                        const auto previous = _files.directory; _files.Parent(); _fileFocus=0;
                        for (size_t i=0;i<_files.entries.size();++i) if (_files.entries[i].path == previous) _fileFocus=i+2;
                        _fileDetail=0; page(Page::parkFiles);
                    }
                    else if (choice == 1) {
                        _files.Refresh(); _focus={}; _fileFocus=0; invalidate();
                    }
                    else if (choice - 2 < _files.entries.size()) {
                        const auto entry = _files.entries[choice - 2];
                        if (entry.directory) { _files.Open(entry.path, _files.pattern); _fileFocus=0; _fileDetail=0; page(Page::parkFiles); }
                        else {
                            _loadFile = Handheld::FileBrowsing::Capture(entry.path);
                            if (!_loadFile) { _status="File unavailable. Refresh the folder."; invalidate(); }
                            else {
                                _parkFilePreview.reset(); _parkPreviewStatus="Native preview unavailable for this format";
                                if(ParkImporter::ExtensionIsOpenRCT2ParkFile(_loadFile->path.extension().u8string())) {
                                    try {
                                        auto importer=ParkImporter::CreateParkFile(GetContext()->GetObjectRepository());
                                        auto stream=FileStream(_loadFile->path.u8string(),FileMode::open);
                                        importer->LoadFromStream(&stream,false,true,_loadFile->path.u8string());
                                        _parkFilePreview=importer->GetParkPreview();
                                        _parkPreviewStatus=_parkFilePreview->parkName.empty() ? "Native preview has no stored summary" : "";
                                    } catch(const std::exception&) { _parkPreviewStatus="Native preview unavailable; importer decides on Load"; }
                                }
                                page(Page::parkInspect);
                            }
                        }
                    }
                    break;
                }
                case Page::load:
                {
                    if (choice == 3) { openParkFiles(); break; }
                    _fileLoad = false;
                    _slot = choice;
                    _loadFile = Handheld::FileBrowsing::Capture(SlotPath(_slot));
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
                        page(_fileLoad ? Page::parkFiles : Page::load);
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
            if (_page==Page::designVehicles || _page==Page::designWarnings || _page==Page::designConfirm) {
                Handheld::ManagementPresentation model;
                model.breadcrumb="Designs / Native choices";
                model.title=_page==Page::designVehicles?"Choose native vehicle":_page==Page::designWarnings?"Availability and warnings":"Place saved design?";
                model.rows=rows();model.selected=_focus.index();
                if (_page==Page::designConfirm) {
                    model.confirmation=true;
                    model.bodyLines=Handheld::Designs::WarningLines();
                    model.stats={{"Native cost",Handheld::FormatMoney(Handheld::Designs::ReviewCost())}};
                    if (auto p=Handheld::Designs::ReviewLocation()) model.stats.push_back({"Origin / direction",std::to_string(p->x)+","+std::to_string(p->y)+","+std::to_string(p->z)+" / "+std::to_string(p->direction)});
                } else {
                    const auto labels=rows();const auto text=_focus.index()<labels.size()?labels[_focus.index()]:std::string{};
                    model.bodyLines={Handheld::FileBrowsing::Detail(text,_designDetail),"Left/Right: full selected detail"};
                    if (_page==Page::designVehicles && _designVehicles.empty()) model.status="No native compatible researched vehicles loaded";
                }
                if (!_status.empty()) model.status=_status;
                Handheld::DrawManagementContent(rt,{8,8},{496,348},model);
                Handheld::Concept::Box(rt,{8,359},{496,25});
                Handheld::DrawModernText(rt,{16,369},"D-pad Select / detail   A Choose   B Back",{},480);return;
            }
            if(_page==Page::parkInspect || _page==Page::designInspect)
            {
                Handheld::ManagementPresentation model;
                model.rows=rows(); model.selected=_focus.index();
                if(_page==Page::parkInspect) {
                    model.breadcrumb="Load / Native file information";
                    model.title=_loadFile ? _loadFile->path.filename().u8string() : "File unavailable";
                    if(_parkFilePreview && !_parkFilePreview->parkName.empty()) {
                        const auto& p=*_parkFilePreview;
                        model.bodyLines={p.parkName};
                        model.stats={{"Rating",std::to_string(p.parkRating)},{"Guests",std::to_string(p.numGuests)},{"Rides",std::to_string(p.numRides)}};
                    }
                    model.status=_status.empty() ? _parkPreviewStatus : _status;
                } else {
                    model.breadcrumb="Designs / Native file information";
                    model.title=_designFile ? _designFile->path.filename().u8string() : "File unavailable";
                    if(_designMetadata) {
                        const auto& d=*_designMetadata;
                        model.stats={{"Required vehicle",std::string(d.trackAndVehicle.vehicleObject.GetName())},
                            {"Track / maze",std::to_string(d.trackElements.size())+" / "+std::to_string(d.mazeElements.size())},
                            {"Scenery",std::to_string(d.sceneryElements.size())},
                            {"Format",trackDesignGetExtension(d.version)}};
                    }
                    if (_focus.index()>2) {
                        model.stats.clear();
                        const auto labels=rows(); const auto text=_focus.index()<labels.size()?labels[_focus.index()]:std::string{};
                        model.bodyLines={Handheld::FileBrowsing::Detail(text,_designDetail), "Left/Right: full selected detail"};
                    } else model.bodyLines={"Native preview / availability below", "", "", ""};
                    model.status=_status.empty() ? (fileDesignEligible() ? (_designChoice?"Selected vehicle: "+_designChoice->ToString():"Native original vehicle") : _designMetadata?Handheld::Designs::UnavailableReason(*_designMetadata):_designFileStatus) : _status;
                    if(_designFile && !Handheld::FileBrowsing::Current(*_designFile)) model.status="File changed. Back and select again.";
                }
                Handheld::DrawManagementContent(rt,{8,8},{496,348},model);
                if(_page==Page::designInspect && _focus.index()<=2 && _designFilePreview && _designFilePreview->drawPreview) {
                    Drawing::RenderTarget preview;
                    if(ClipRenderTarget(preview,rt,{420,166},60,62)) _designFilePreview->drawPreview(preview);
                }
                Handheld::Concept::Box(rt,{8,359},{496,25});
                Handheld::DrawModernText(rt,{16,369},"D-pad Select   A Choose   B Back",{},480); return;
            }
            if ((_page == Page::parkFiles || _page == Page::saveFiles || _page == Page::designFiles))
            {
                Handheld::ManagementPresentation model;
                model.breadcrumb = std::string(_page == Page::designFiles ? "Designs / " : _page == Page::saveFiles ? "Save / " : "Load / ") + _files.directory.u8string();
                model.title = _page == Page::designFiles ? "Track design files" : _page == Page::saveFiles ? "Save As destination" : "Parks and autosaves";
                model.rows = rows(); model.selected = _focus.index();
                model.bodyLines={Handheld::FileBrowsing::Detail(fileDetailPath(),_fileDetail)};
                model.status = !_status.empty() ? _status : !_files.error.empty() ? _files.error
                    : _files.entries.empty() ? "No supported files or folders" : "";
                Handheld::DrawManagementContent(rt, {8,8}, {496,348}, model);
                Handheld::Concept::Box(rt,{8,359},{496,25});
                Handheld::DrawModernText(rt,{16,369},"Up/Down Select  Left/Right Detail  A Open  B Back",{},480);
                return;
            }
            if (_page == Page::saveReview)
            {
                Handheld::ManagementPresentation model;
                model.breadcrumb="Save As / Review";
                model.title=_saveDraft ? _saveDraft->destination.filename().u8string() : "Review expired";
                model.confirmation=true; model.rows=rows(); model.selected=_focus.index();
                model.bodyLines={_saveDraft ? _saveDraft->directory.u8string() : _files.directory.u8string()};
                model.status=_status.empty() ? "Previous save kept until publication succeeds" : _status;
                Handheld::DrawManagementContent(rt,{8,8},{496,348},model);
                Handheld::Concept::Box(rt,{8,359},{496,25});
                Handheld::DrawModernText(rt,{16,369},"D-pad Select   A Choose   B Cancel",{},480); return;
            }
            if (_page == Page::saveName || _page == Page::scenerySearch)
            {
                auto model = Handheld::StaffManagement::NamePresentation(_nameEntry,_focus.index(),false,_status);
                model.breadcrumb = _page == Page::saveName ? "Save As / Filename" : "Scenery / Search";
                model.title = _nameEntry.text.empty() ? (_page == Page::saveName ? "Enter park filename" : "All eligible objects") : _nameEntry.text;
                model.status = _status.empty() ? "OK Apply / Clr Clear" : _status;
                model.rows[42] = "Sp";
                model.rows[44] = "Aa";
                model.rows[47] = "B";
                Handheld::DrawManagementPanel(rt,{ContextGetWidth(),ContextGetHeight()},model);
                return;
            }
            if (_page == Page::parkMap) { Handheld::ParkMap::Draw(rt, _parkMap); return; }
            if (parkInformation())
            {
                auto model = parkInformationPresentation();
                if (_page == Page::parkHistory) model.bodyLines.assign(10, "");
                if (_page == Page::financeHistory) model.bodyLines.assign(8, "");
                Handheld::DrawManagementContent(rt, {8, 8}, {496, 348}, model);
                if (_page == Page::parkHistory) Handheld::ParkInformation::DrawHistory(rt, _parkGuestHistory);
                if (_page == Page::financeHistory) Handheld::FinanceInformation::DrawHistory(rt,_financeGraph);
                if (_page == Page::researchItem && _researchItem) Handheld::ResearchCatalog::DrawPreview(rt,*_researchItem);
                Handheld::Concept::Box(rt, {8, 359}, {496, 25});
                Handheld::DrawModernText(rt, {16, 369}, "D-pad Select   A Choose   B Back   Start Back", {}, 480);
                return;
            }
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
            if (_page==Page::shopAppearance || _page==Page::shopAppearanceEditor || _page==Page::shopAppearanceReview)
            {
                Handheld::DrawManagementContent(rt,{8,8},{330,348},shopAppearancePresentation());
                Handheld::DrawActionStrip(rt,{ContextGetWidth(),ContextGetHeight()});
                Handheld::DrawModernText(rt,{12,369},"D-pad Choose   A Select / review   B Back",{},488);
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
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Track pieces", _status, "No compatible pieces", false, "Ride");
                return;
            }
            if (_page == Page::designPicker)
            {
                Handheld::DrawObjectPicker(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Saved designs", _status.empty() ? "X Browse files" : _status,
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
            if (_page==Page::terrainPaintConfirm && _paintDraft)
            {
                Handheld::DrawManagementPanel(rt,{ContextGetWidth(),ContextGetHeight()},
                    Handheld::AreaTools::PaintPresentation(*_paintDraft,_focus.index(),_status));
                return;
            }
            if(_page==Page::sceneryClearConfirm && _clearDraft) {
                Handheld::DrawManagementPanel(rt,{ContextGetWidth(),ContextGetHeight()},Handheld::SceneryClear::Presentation(*_clearDraft,_focus.index(),_status));return;
            }
            if (_page == Page::sceneryStyleEditor || _page == Page::sceneryStyleConfirm)
            {
                Handheld::ManagementPresentation model;
                model.breadcrumb = "Scenery / Sign style";
                model.title = _signStyle ? Handheld::SceneryTools::StyleValue(*_signStyle) : "Sign unavailable";
                model.rows = rows(); model.selected = _focus.index();
                model.confirmation = _page == Page::sceneryStyleConfirm;
                model.status = _status.empty() ? "Native style / no construction charge" : _status;
                Handheld::DrawManagementPanel(rt, {ContextGetWidth(), ContextGetHeight()}, model);
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
                    Handheld::SceneryTools::CategoryName(Handheld::SceneryTools::Current().category),
                    _status.empty() ? Handheld::SceneryTools::GroupName() + (Handheld::SceneryTools::Search().empty()
                        ? "" : " / " + Handheld::SceneryTools::Search()) : _status,
                    "No matching available objects", true);
                return;
            }
            if (_page == Page::treePicker)
            {
                Handheld::DrawObjectPicker(
                    rt, { ContextGetWidth(), ContextGetHeight() }, _picker, "Choose tree", _status, "No trees available");
                return;
            }
            if (_page == Page::pathUndoConfirm && _pathUndo)
            {
                Handheld::ManagementPresentation model;
                model.breadcrumb = "Build / Connected path";
                model.title = "Remove last new section?";
                model.rows = rows(); model.selected = _focus.index(); model.confirmation = true;
                model.stats = {{"Cost / refund", Handheld::FormatMoney(_pathUndo->cost)}};
                model.status = _status.empty() ? "Existing paths preserved" : _status;
                Handheld::DrawManagementPanel(rt, {ContextGetWidth(), ContextGetHeight()}, model);
                return;
            }
            if (_page == Page::terrainMaterialPicker)
            {
                Handheld::DrawObjectPicker(rt,{ContextGetWidth(),ContextGetHeight()},_picker,
                    _terrainMaterialEdge ? "Terrain edges" : "Terrain surfaces",_status,"No loaded materials");
                return;
            }
            if (_page == Page::pathSurfacePicker)
            {
                Handheld::DrawObjectPicker(rt, {ContextGetWidth(), ContextGetHeight()}, _picker,
                    Handheld::Paths::IsQueue() ? "Queue surfaces" : "Path surfaces", _status,
                    "No eligible surfaces");
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
                case Page::sceneryClearOptions: title="CLEAR AREA OPTIONS"; break;
                case Page::sceneryGroups:
                    title = "SCENERY GROUP";
                    break;
                case Page::sceneryStyle:
                    title = "SIGN STYLE";
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
            if (_page == Page::sceneryClearOptions)
                model.icons = { Handheld::PanelIcon::play, Handheld::PanelIcon::none, Handheld::PanelIcon::none,
                    Handheld::PanelIcon::tree, Handheld::PanelIcon::largeScenery, Handheld::PanelIcon::path,
                    Handheld::PanelIcon::walls, Handheld::PanelIcon::furniture };
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
                model.icons = { Handheld::PanelIcon::raise, Handheld::PanelIcon::lower, Handheld::PanelIcon::path,
                    Handheld::PanelIcon::none, Handheld::PanelIcon::terrain, Handheld::PanelIcon::remove,
                    Handheld::PanelIcon::tree, Handheld::PanelIcon::none, Handheld::PanelIcon::none,
                    Handheld::PanelIcon::folder, Handheld::PanelIcon::largeScenery, Handheld::PanelIcon::terrain,
                    Handheld::PanelIcon::none, Handheld::PanelIcon::back };
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
    void HandheldAccessReviewOpen(const Handheld::RideAccess::EditDraft& draft)
    {
        if (auto* window = OpenPanel()) window->openAccessReview(draft);
    }
    void HandheldRideAccessOpen(const Handheld::RideManagement::Target& target)
    {
        if (auto* window = OpenPanel())
            window->openRideAccess(target);
    }
    void HandheldPathSurfacePickerOpen()
    {
        if (auto* window = OpenPanel()) window->openPathSurfacePicker(true);
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
    Handheld::ManagementPresentation HandheldShopAppearancePresentation() {
        auto* window=GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window?static_cast<HandheldMenuWindow*>(window)->shopAppearancePresentation():Handheld::ManagementPresentation{};
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
    void HandheldRideReturn(const Handheld::RideManagement::Target& target, const std::optional<Handheld::RideNavigation::Context>& context)
    {
        if(auto* window=OpenPanel()) window->returnRide(target,context);
    }
    bool HandheldRideQuickAccess()
    {
        auto* window=GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<HandheldMenuWindow*>(window)->quickRideAccess();
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
    Handheld::ManagementPresentation HandheldParkMapSubjectsPresentation()
    {
        const auto* w = static_cast<HandheldMenuWindow*>(GetWindowManager()->FindByClass(WindowClass::handheldMenu));
        return w ? w->mapSubjectsModel() : Handheld::ManagementPresentation{};
    }
    std::optional<CoordsXY> HandheldParkMapLocation()
    {
        const auto* w = static_cast<HandheldMenuWindow*>(GetWindowManager()->FindByClass(WindowClass::handheldMenu));
        return w && w->parkMapOpen() ? std::optional<CoordsXY>{w->mapFocus()} : std::nullopt;
    }
    void HandheldParkMapSubjects()
    {
        if (auto* w = static_cast<HandheldMenuWindow*>(GetWindowManager()->FindByClass(WindowClass::handheldMenu))) w->parkMapSubjects();
    }
    bool HandheldParkMapIsOpen()
    {
        auto* w = static_cast<HandheldMenuWindow*>(GetWindowManager()->FindByClass(WindowClass::handheldMenu));
        return w && w->parkMapOpen();
    }
    void HandheldParkMapControl(int control)
    {
        if (auto* w = static_cast<HandheldMenuWindow*>(GetWindowManager()->FindByClass(WindowClass::handheldMenu))) w->parkMapControl(control);
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
    void HandheldTerrainPaintReviewOpen(CoordsXY tile)
    {
        if (auto* window=OpenPanel()) window->openTerrainPaintReview(tile);
    }
    void HandheldAreaOptionsOpen()
    {
        if (auto* w = OpenPanel())
            w->openAreaOptions();
    }
    void HandheldSceneryClearOptionsOpen() {if(auto* w=OpenPanel())w->openSceneryClearOptions();}
    void HandheldSceneryClearReviewOpen(CoordsXY tile) {if(auto* w=OpenPanel())w->openSceneryClearReview(tile);}
    void HandheldSceneryStyleOpen(const Handheld::SceneryTools::Target& target)
    {
        if (auto* window = OpenPanel()) window->openSceneryStyle(target);
    }
    void HandheldSceneryCatalogOpen()
    {
        if(auto* window=OpenPanel()) window->openSceneryCatalog();
    }
    void HandheldTerrainMaterialPickerOpen(bool edge)
    {
        if(auto* window=OpenPanel()) window->openTerrainMaterialPicker(edge);
    }
    void HandheldSceneryOptionsOpen()
    {
        if (auto* window = OpenPanel())
            window->openSceneryOptions();
    }
    bool HandheldSceneryPickerIsOpen()
    {
        const auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu);
        return window && static_cast<const HandheldMenuWindow*>(window)->sceneryPicker();
    }
    void HandheldScenerySearchOpen()
    {
        if (auto* window = OpenPanel()) window->openScenerySearch();
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
    void HandheldDashboardPage(int direction)
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::handheldMenu))
            static_cast<HandheldMenuWindow*>(window)->dashboardPage(direction);
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

namespace OpenRCT2::Ui::Windows {
    bool HandheldDesignPickerIsOpen() {
        const auto* w=static_cast<const HandheldMenuWindow*>(GetWindowManager()->FindByClass(WindowClass::handheldMenu));
        return w && w->designPickerOpen();
    }
    void HandheldDesignFilesOpen() { if(auto* w=OpenPanel()) w->openDesignFiles(!Handheld::Designs::Active()); }
}

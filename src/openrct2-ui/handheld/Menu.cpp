// SPDX-License-Identifier: GPL-3.0-only
#include "Explorer.h"
#include "ListFocus.h"
#include "Panel.h"
#include "Paths.h"
#include "SaveSlots.h"

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
        dashboard,
        overview,
        finances,
        guests,
        research,
        ride,
        save,
        load,
        overwrite,
        discard,
        exit
    };

    static fs::path SlotPath(size_t slot)
    {
        return fs::u8path(GetContext()->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::saves))
            / ("Pocket-" + std::to_string(slot + 1) + ".park");
    }

    class HandheldMenuWindow final : public Window
    {
        Handheld::ListFocus _focus;
        Page _page = Page::park;
        bool _pausedHere{};
        bool _exitAfterSave{};
        size_t _slot{};
        std::string _status;
        RideId _rideId = RideId::GetNull();
        bool information() const
        {
            return _page == Page::overview || _page == Page::finances || _page == Page::guests || _page == Page::research
                || _page == Page::ride;
        }
        static std::string money(money64 value)
        {
            if (value == kMoney64Undefined)
                return "Not available";
            std::ostringstream text;
            text << "$" << std::fixed << std::setprecision(2) << value / 100.0;
            return text.str();
        }

        void layout()
        {
            const auto size = Handheld::OverlaySize(rows().size());
            // Confirmation text needs room; normal tools leave the centre cursor exposed.
            const ScreenSize target{ _page == Page::overwrite || _page == Page::discard || _page == Page::exit ? 330
                                                                                                               : size.width,
                                     size.height };
            const ScreenSize bounds = _page == Page::pathOptions ? ScreenSize{ ContextGetWidth(), ContextGetHeight() }
                : information()                                  ? ScreenSize{ 330, _page == Page::ride ? 340 : 310 }
                                                                 : target;
            WindowSetResize(*this, bounds, bounds);
            WindowSetPosition(
                *this,
                _page == Page::pathOptions ? ScreenCoordsXY{ 0, 0 }
                                           : ScreenCoordsXY{ 8, std::max(8, (ContextGetHeight() - bounds.height) / 2) });
        }

        void page(Page next)
        {
            _page = next;
            _focus = {};
            _status.clear();
            layout();
            if (next == Page::overwrite || next == Page::discard || next == Page::exit)
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
                    return { "Overview", "Finances", "Guests", "Research", "Back to park" };
                case Page::overview:
                case Page::finances:
                case Page::guests:
                    return { "Back to dashboard" };
                case Page::research:
                    return { "Cycle research funding", "Back to dashboard" };
                case Page::ride:
                    return { "Open ride", "Close ride", "Back to park" };
                case Page::build:
                    return { "Raise land", "Lower land", "Paths", "Back to park" };
                case Page::paths:
                    return { "Normal path", "Queue path", "Remove path", "Back to build" };
                case Page::pathOptions:
                    return { "Resume",       "Follow ground", "Raise height", "Lower height",
                             "Flat / slope", "Rotate slope",  "Surface" };
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
        void openRide(RideId id)
        {
            _rideId = id;
            page(Page::ride);
        }
        void openLoad()
        {
            page(Page::load);
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
                case Page::ride:
                case Page::pathOptions:
                    close();
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
                    if (choice == 2)
                        page(Page::paths);
                    else
                    {
                        close();
                        if (choice < 2)
                            Handheld::BeginTerrain(choice == 0);
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
                    if (choice == 4)
                        page(Page::park);
                    else
                        page(
                            choice == 0       ? Page::overview
                                : choice == 1 ? Page::finances
                                : choice == 2 ? Page::guests
                                              : Page::research);
                    break;
                case Page::overview:
                case Page::finances:
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
                model.icons = { Handheld::PanelIcon::raise, Handheld::PanelIcon::lower, Handheld::PanelIcon::path,
                                Handheld::PanelIcon::back };
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

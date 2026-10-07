// SPDX-License-Identifier: GPL-3.0-only
#include "Explorer.h"
#include "ListFocus.h"
#include "Panel.h"
#include "SaveSlots.h"

#include <filesystem>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/config/Config.h>
#include <openrct2/localisation/StringIds.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/WindowManager.h>

namespace OpenRCT2::Ui::Windows
{
    using namespace Drawing;
    namespace fs = std::filesystem;
    static constexpr ScreenSize kSize{ 494, 342 };
    static constexpr auto kWidgets = makeWidgets(makeWindowShim(STR_SHORTCUT_PAUSE, kSize));
    enum class Page
    {
        park,
        build,
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

        void page(Page next)
        {
            _page = next;
            _focus = {};
            _status.clear();
            if (next == Page::overwrite || next == Page::discard || next == Page::exit)
                _focus.move(1, 2); // destructive confirmations default to Cancel
            invalidate();
        }
        std::vector<std::string> rows() const
        {
            switch (_page)
            {
                case Page::park:
                    return { "Resume park", "Build", "Save park", "Save and exit", "Load park", "Exit without saving" };
                case Page::build:
                    return { "Terrain: raise land", "Terrain: lower land", "Back to park" };
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
                            _exitAfterSave = false;
                            page(Page::save);
                            break;
                        case 3:
                            _exitAfterSave = true;
                            page(Page::save);
                            break;
                        case 4:
                            page(Page::load);
                            break;
                        case 5:
                            page(Page::exit);
                            break;
                    }
                    break;
                case Page::build:
                    close();
                    if (choice < 2)
                        Handheld::BeginTerrain(choice == 0);
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
            const char* title = "PARK / PAUSE";
            switch (_page)
            {
                case Page::build:
                    title = "BUILD / TERRAIN";
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
            model.section = _page == Page::build ? "LAND TOOLS" : "PARK ACTIONS";
            model.rows = rows();
            model.selected = _focus.index();
            model.status = _status;
            const bool playing = gLegacyScene == LegacyScene::playing;
            const auto& park = getGameState().park;
            model.detailTitle = playing ? park.name : "OpenRCT2 Pocket";
            model.detailSubtitle = playing ? "Park paused" : "Choose a saved park";
            if (playing)
                model.stats = { { "Guests", std::to_string(park.numGuestsInPark) },
                                { "Park rating", std::to_string(park.rating) },
                                { "Save slots", "3" } };
            model.help = { "A  Open selected action", "B  Return to your park", "D-pad  Move through menu" };
            if (_page == Page::park)
                model.icons = { Handheld::PanelIcon::play, Handheld::PanelIcon::terrain, Handheld::PanelIcon::save,
                                Handheld::PanelIcon::exit, Handheld::PanelIcon::folder,  Handheld::PanelIcon::exit };
            else if (_page == Page::build)
            {
                model.icons = { Handheld::PanelIcon::raise, Handheld::PanelIcon::lower, Handheld::PanelIcon::back };
                model.detailSubtitle = "Terrain / single tile";
                model.tabs = { "Terrain" };
                model.help = { "A  Edit tile under cursor", "Y  Switch raise / lower", "B  Finish editing" };
                model.status = "Select a tool, then move the cursor in your park.";
            }
            else if (_page == Page::save || _page == Page::load)
            {
                model.section = "POCKET SAVE SLOTS";
                model.help = { _page == Page::save ? "A  Save to selected slot" : "A  Load selected slot",
                               "B  Return to previous menu", "D-pad  Choose a slot" };
            }
            else
            {
                model.section = "CONFIRM ACTION";
                model.help = { "A  Confirm selected action", "B  Return without changes" };
            }
            Handheld::DrawMenuShell(rt, *this, model);
        }
    };

    static HandheldMenuWindow* OpenPanel()
    {
        auto* wm = GetWindowManager();
        auto* window = wm->BringToFrontByClass(WindowClass::handheldMenu);
        if (window == nullptr)
            window = wm->Create<HandheldMenuWindow>(WindowClass::handheldMenu, {}, kSize, { WindowFlag::centreScreen });
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
} // namespace OpenRCT2::Ui::Windows

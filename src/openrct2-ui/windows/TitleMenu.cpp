/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Currency.h>
#include <openrct2-ui/handheld/FeelSound.h>
#include <openrct2-ui/handheld/Panel.h>
#include <openrct2-ui/handheld/Resume.h>
#include <openrct2-ui/handheld/SavedParkPresentation.h>
#include <openrct2-ui/interface/Dropdown.h>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/scripting/CustomMenu.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/general/LoadOrQuitAction.h>
#include <openrct2/drawing/Line.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/Rectangle.h>
#include <openrct2/drawing/Text.h>
#include <openrct2/interface/ColourWithFlags.h>
#include <openrct2/scenes/SceneManager.h>
#include <openrct2/scenes/editor/EditorScene.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>

namespace OpenRCT2::Ui::Windows
{
    using namespace OpenRCT2::Drawing;

    enum WindowTitleMenuWidgetIdx : WidgetIndex
    {
        WIDX_START_NEW_GAME,
        WIDX_CONTINUE_SAVED_GAME,
        WIDX_MULTIPLAYER,
        WIDX_GAME_TOOLS,
        WIDX_NEW_VERSION,
    };

    enum
    {
        DDIDX_SCENARIO_EDITOR,
        DDIDX_CONVERT_SAVED_GAME,
        DDIDX_TRACK_DESIGNER,
        DDIDX_TRACK_MANAGER,
        DDIDX_OPEN_CONTENT_FOLDER,
        DDIDX_CUSTOM_BEGIN = 6,
    };

    static constexpr ScreenSize kMenuButtonDims = { 82, 82 };
    static constexpr ScreenSize kUpdateButtonDims = { kMenuButtonDims.width * 4, 28 };

    // clang-format off
    static constexpr auto _titleMenuWidgets = makeWidgets(
        makeWidget({0, kUpdateButtonDims.height}, kMenuButtonDims,   WidgetType::imgBtn, WindowColour::tertiary,  ImageId(SPR_MENU_NEW_GAME),       STR_START_NEW_GAME_TIP),
        makeWidget({0, kUpdateButtonDims.height}, kMenuButtonDims,   WidgetType::imgBtn, WindowColour::tertiary,  ImageId(SPR_MENU_LOAD_GAME),      STR_CONTINUE_SAVED_GAME_TIP),
        makeWidget({0, kUpdateButtonDims.height}, kMenuButtonDims,   WidgetType::imgBtn, WindowColour::tertiary,  ImageId(SPR_G2_MENU_MULTIPLAYER), STR_SHOW_MULTIPLAYER_TIP),
        makeWidget({0, kUpdateButtonDims.height}, kMenuButtonDims,   WidgetType::imgBtn, WindowColour::tertiary,  ImageId(SPR_MENU_TOOLBOX),        STR_GAME_TOOLS_TIP),
        makeWidget({0,                        0}, kUpdateButtonDims, WidgetType::button, WindowColour::secondary, STR_UPDATE_AVAILABLE)
    );
    // clang-format on

    static void WindowTitleMenuScenarioselectCallback(const utf8* path)
    {
        GameNotifyMapChange();
        GetContext()->LoadParkFromFile(path, false, true);
        GameLoadScripts();
        GameNotifyMapChanged();
    }

    static void InvokeCustomToolboxMenuItem(size_t index)
    {
#ifdef ENABLE_SCRIPTING
        const auto& customMenuItems = Scripting::CustomMenuItems;
        size_t i = 0;
        for (const auto& item : customMenuItems)
        {
            if (item.Kind == Scripting::CustomToolbarMenuItemKind::toolbox)
            {
                if (i == index)
                {
                    item.Invoke();
                    break;
                }
                i++;
            }
        }
#endif
    }

    class TitleMenuWindow final : public Window
    {
    private:
        ScreenRect _filterRect;
        WidgetIndex _handheldSelection = WIDX_START_NEW_GAME;
        Handheld::MenuFeel _feel;
        // Handheld rows: Continue (when a save exists) followed by the visible menu buttons.
        static constexpr int kContinueRow = -1;
        size_t _handheldRow{};
        std::optional<std::filesystem::path> _continuePath;
        std::string _continueName, _continueDetail;
        bool _continueScanned{};

        // The newest park in the saves folder, including slots, named saves and autosaves.
        void scanContinue()
        {
            if (_continueScanned)
                return;
            _continueScanned = true;
            namespace fs = std::filesystem;
            std::error_code error;
            const auto root = fs::u8path(GetContext()->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::saves));
            fs::file_time_type newest{};
            for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied, error);
                 !error && it != fs::recursive_directory_iterator(); it.increment(error))
            {
                if (!it->is_regular_file(error) || it->path().extension() != ".park")
                    continue;
                // OpenRCT2 also writes a park to saves/autosave every few minutes. Those are crash
                // insurance, not the player's save: after "Quit without saving", Continue picking
                // one up looked as if the game had saved anyway. Pocket's own resume save (also in
                // that folder, removed by a deliberate discard) is the only one Continue offers.
                if (it->path().parent_path().filename() == "autosave"
                    && it->path().filename() != Handheld::Resume::Path().filename())
                    continue;
                const auto time = it->last_write_time(error);
                if (!error && (!_continuePath || time > newest))
                {
                    newest = time;
                    _continuePath = it->path();
                }
            }
            if (!_continuePath)
                return;
            const auto preview = Handheld::SavedParkPresentation::Read(*_continuePath);
            if (!preview)
            {
                _continuePath.reset();
                return;
            }
            _continueName = preview->parkName.empty() ? _continuePath->stem().u8string() : preview->parkName;
            _continueDetail = Handheld::SavedParkPresentation::Date(*preview);
            if (preview->parkUsesMoney)
                _continueDetail += "  |  " + Handheld::FormatMoney(preview->cash);
        }
        std::vector<int> handheldRows()
        {
            scanContinue();
            std::vector<int> rows;
            if (_continuePath)
                rows.push_back(kContinueRow);
            // Game tools (scenario editor, track designer and manager, open folder) need a mouse and
            // a desktop, so the handheld menu leaves them out rather than offer a dead button.
            for (auto index : { WIDX_START_NEW_GAME, WIDX_CONTINUE_SAVED_GAME, WIDX_MULTIPLAYER })
                if (widgets[index].isVisible())
                    rows.push_back(index);
            return rows;
        }

    public:
        void handheldMove(int32_t direction)
        {
            const auto rows = handheldRows();
            _handheldRow = (_handheldRow + rows.size() + direction) % rows.size();
            if (rows[_handheldRow] != kContinueRow)
                _handheldSelection = rows[_handheldRow];
            invalidate();
        }

        void handheldConfirm()
        {
            const auto rows = handheldRows();
            const int row = rows[std::min(_handheldRow, rows.size() - 1)];
            if (row == kContinueRow)
            {
                // A failed load (moved or damaged file) falls back to the load browser.
                if (!_continuePath || !GetContext()->LoadParkFromFile(_continuePath->u8string()))
                    HandheldLoadOpen();
                return;
            }
            if (row == WIDX_CONTINUE_SAVED_GAME)
                HandheldLoadOpen();
            else if (row == WIDX_GAME_TOOLS)
                onMouseDown(row);
            else
                onMouseUp(row);
        }

        void onOpen() override
        {
            setWidgets(_titleMenuWidgets);

#ifdef DISABLE_NETWORK
            widgets[WIDX_MULTIPLAYER].setHidden();
#endif

            int32_t x = 0;
            for (Widget* widget = widgets.data(); widget != &widgets[WIDX_NEW_VERSION]; widget++)
            {
                if (widget->isVisible())
                {
                    widget->left = x;
                    widget->right = x + kMenuButtonDims.width - 1;

                    x += kMenuButtonDims.width;
                }
            }
            width = x;
            widgets[WIDX_NEW_VERSION].right = width;
            windowPos.x = (ContextGetWidth() - width) / 2;
            colours[1] = ColourWithFlags{ Drawing::Colour::lightOrange }.withFlag(ColourFlag::translucent, true);

            if (Handheld::IsEnabled())
            {
                onResize();
                int32_t row = 0;
                for (auto index : { WIDX_START_NEW_GAME, WIDX_CONTINUE_SAVED_GAME, WIDX_MULTIPLAYER, WIDX_GAME_TOOLS })
                {
                    if (widgets[index].isVisible())
                    {
                        auto& item = widgets[index];
                        item.left = (width - 320) / 2 + 12;
                        item.right = item.left + 295;
                        item.top = 197 + row * 34;
                        item.bottom = item.top + 31;
                        ++row;
                    }
                }
            }
            initScrollWidgets();
        }

        void onResize() override
        {
            if (!Handheld::IsEnabled())
                return;
            // The engine's desktop title anchor is y=height-182. A taller Pocket
            // menu must opt out or its rows/footer extend below the screen.
            width = ContextGetWidth();
            height = ContextGetHeight();
            windowPos = { 0, 0 };
        }

        void onMouseUp(WidgetIndex widgetIndex) override
        {
            auto* windowMgr = GetWindowManager();

            switch (widgetIndex)
            {
                case WIDX_START_NEW_GAME:
                    if (windowMgr->BringToFrontByClass(WindowClass::scenarioSelect) == nullptr)
                    {
                        windowMgr->CloseByClass(WindowClass::loadsave);
                        windowMgr->CloseByClass(WindowClass::serverList);
                        ScenarioselectOpen(WindowTitleMenuScenarioselectCallback);
                    }
                    break;
                case WIDX_CONTINUE_SAVED_GAME:
                    if (windowMgr->BringToFrontByClass(WindowClass::loadsave) == nullptr)
                    {
                        windowMgr->CloseByClass(WindowClass::scenarioSelect);
                        windowMgr->CloseByClass(WindowClass::serverList);
                        auto loadOrQuitAction = GameActions::LoadOrQuitAction(GameActions::LoadOrQuitModes::openSavePrompt);
                        GameActions::Execute(&loadOrQuitAction, getGameState());
                    }
                    break;
                case WIDX_MULTIPLAYER:
                    if (windowMgr->BringToFrontByClass(WindowClass::serverList) == nullptr)
                    {
                        windowMgr->CloseByClass(WindowClass::scenarioSelect);
                        windowMgr->CloseByClass(WindowClass::loadsave);
                        ContextOpenWindow(WindowClass::serverList);
                    }
                    break;
                case WIDX_NEW_VERSION:
                    ContextOpenWindowView(WindowView::newVersionInfo);
                    break;
            }
        }

        void onMouseDown(WidgetIndex widgetIndex) override
        {
            if (widgetIndex == WIDX_GAME_TOOLS)
            {
                int32_t i = 0;
                gDropdown.items[i++] = Dropdown::PlainMenuLabel(STR_SCENARIO_EDITOR);
                gDropdown.items[i++] = Dropdown::PlainMenuLabel(STR_CONVERT_SAVED_GAME_TO_SCENARIO);
                gDropdown.items[i++] = Dropdown::PlainMenuLabel(STR_ROLLER_COASTER_DESIGNER);
                gDropdown.items[i++] = Dropdown::PlainMenuLabel(STR_TRACK_DESIGNS_MANAGER);
                gDropdown.items[i++] = Dropdown::PlainMenuLabel(STR_OPEN_USER_CONTENT_FOLDER);

#ifdef ENABLE_SCRIPTING
                auto hasCustomItems = false;
                const auto& customMenuItems = Scripting::CustomMenuItems;
                if (!customMenuItems.empty())
                {
                    for (const auto& item : customMenuItems)
                    {
                        if (item.Kind == Scripting::CustomToolbarMenuItemKind::toolbox)
                        {
                            if (!hasCustomItems)
                            {
                                hasCustomItems = true;
                                gDropdown.items[i++] = Dropdown::Separator();
                            }

                            gDropdown.items[i] = Dropdown::PlainMenuLabel(item.Text.c_str());
                            i++;
                        }
                    }
                }
#endif

                Widget* widget = &widgets[widgetIndex];
                int32_t yOffset = 0;
                if (i > 5)
                {
                    yOffset = -(widget->height() - 1 + 5 + (i * 12));
                }

                WindowDropdownShowText(
                    windowPos + ScreenCoordsXY{ widget->left, widget->top + yOffset }, widget->height(),
                    colours[0].withFlag(ColourFlag::translucent, true), {}, i);
            }
        }

        void onDropdown(WidgetIndex widgetIndex, int32_t selectedIndex) override
        {
            if (selectedIndex == -1)
            {
                return;
            }
            if (widgetIndex == WIDX_GAME_TOOLS)
            {
                auto* sceneMgr = GetContext()->GetSceneManager();
                switch (selectedIndex)
                {
                    case DDIDX_SCENARIO_EDITOR:
                        sceneMgr->setActiveScene(sceneMgr->getScenarioEditorScene());
                        break;
                    case DDIDX_CONVERT_SAVED_GAME:
                    {
                        auto* editorScene = static_cast<EditorScene*>(sceneMgr->getScenarioEditorScene());
                        editorScene->ConvertSaveToScenario();
                        break;
                    }
                    case DDIDX_TRACK_DESIGNER:
                        sceneMgr->setActiveScene(sceneMgr->getTrackDesignerScene());
                        break;
                    case DDIDX_TRACK_MANAGER:
                        sceneMgr->setActiveScene(sceneMgr->getTrackManagerScene());
                        break;
                    case DDIDX_OPEN_CONTENT_FOLDER:
                    {
                        auto context = GetContext();
                        auto& env = context->GetPlatformEnvironment();
                        auto& uiContext = context->GetUiContext();
                        uiContext.OpenFolder(env.GetDirectoryPath(DirBase::user));
                        break;
                    }
                    default:
                        InvokeCustomToolboxMenuItem(selectedIndex - DDIDX_CUSTOM_BEGIN);
                        break;
                }
            }
        }

        CursorID onCursor(WidgetIndex, const ScreenCoordsXY&, CursorID cursorId) override
        {
            gTooltipCloseTimeout = gCurrentRealTimeTicks + 2000;
            return cursorId;
        }

        void onPrepareDraw() override
        {
            _filterRect = { windowPos + ScreenCoordsXY{ 0, kUpdateButtonDims.height },
                            windowPos + ScreenCoordsXY{ width - 1, kMenuButtonDims.height + kUpdateButtonDims.height - 1 } };

            const bool newVersionAvailable = GetContext()->HasNewVersionInfo();
            widgets[WIDX_NEW_VERSION].setVisible(newVersionAvailable);

            if (newVersionAvailable)
                _filterRect.point1.y = windowPos.y;
        }

        void onUpdate() override
        {
            if (!Handheld::IsEnabled())
                return;
            // Continue last park on launch: once per run, straight into the newest park. A failed
            // load leaves the title as it was.
            scanContinue();
            if (Handheld::Resume::LaunchContinue(_continuePath.value_or(std::filesystem::path{})))
                return;
            Handheld::PlayFeelSound(_feel.Observe("title", _handheldRow));
            if (_feel.NeedsDraw())
                invalidate();
        }

        void onDraw(RenderTarget& rt) override
        {
            if (Handheld::IsEnabled())
            {
                Handheld::PlayFeelSound(_feel.Observe("title", _handheldRow));
                Handheld::MenuFeelScope motion(_feel);
                _feel.base = { reinterpret_cast<const uint8_t*>(rt.bits), rt.x, rt.y, rt.LineStride() };
                // Preserve the native animated title park and OpenRCT2 branding.
                const ScreenCoordsXY logo{ (width - 232) / 2, 8 };
                GfxDrawSprite(rt, ImageId(SPR_G2_LOGO), logo);
                GfxDrawSprite(rt, ImageId(SPR_G2_TITLE), logo + ScreenCoordsXY{ 104, 18 });
                const auto rows = handheldRows();
                _handheldRow = std::min(_handheldRow, rows.size() - 1);
                const bool hasContinue = _continuePath.has_value();
                constexpr int kContinueHeight = 44;
                const int cardHeight = 68 + static_cast<int>(rows.size()) * 34 + (hasContinue ? kContinueHeight - 32 : 0);
                const ScreenCoordsXY p{ (width - 320) / 2, std::min(148, ContextGetHeight() - 34 - cardHeight) };
                Handheld::DrawSurface(rt, p, { 320, cardHeight });
                Handheld::DrawSectionLabel(rt, p + ScreenCoordsXY{ 12, 14 }, 296, "OPENRCT2 POCKET");
                static constexpr const char* labels[] = { "New park", "Load saved park", "Multiplayer", "Game tools" };
                static constexpr Handheld::PanelIcon icons[] = { Handheld::PanelIcon::terrain, Handheld::PanelIcon::folder,
                                                                 Handheld::PanelIcon::play, Handheld::PanelIcon::save };
                int32_t y = 49;
                for (size_t i = 0; i < rows.size(); ++i)
                {
                    const bool focused = i == _handheldRow;
                    if (rows[i] == kContinueRow)
                    {
                        Handheld::DrawActionRow(
                            rt, p + ScreenCoordsXY{ 12, y }, { 296, kContinueHeight }, "Continue: " + _continueName,
                            _continueDetail, focused, true, Handheld::PanelIcon::play);
                        y += kContinueHeight + 2;
                        continue;
                    }
                    Handheld::DrawActionRow(
                        rt, p + ScreenCoordsXY{ 12, y }, { 296, 32 }, labels[rows[i]], {}, focused, true, icons[rows[i]]);
                    y += 34;
                }
                Handheld::DrawFooter(
                    rt, { ContextGetWidth(), ContextGetHeight() },
                    { { "D-pad", "Navigate" }, { "A", "Select" }, { "B", "Exit" } });
                _feel.EndFrame();
                Handheld::DrawFocusPointer(rt, _feel);
                return;
            }
            Rectangle::filter(rt, _filterRect, FilterPaletteID::palette51);
            drawWidgets(rt);
        }
    };

    /**
     * Creates the window containing the menu buttons on the title screen.
     */
    WindowBase* TitleMenuOpen()
    {
        const uint16_t windowHeight = kMenuButtonDims.height + kUpdateButtonDims.height;

        auto* windowMgr = GetWindowManager();
        return windowMgr->Create<TitleMenuWindow>(
            WindowClass::titleMenu, ScreenCoordsXY(0, ContextGetHeight() - 182), { 0, windowHeight },
            { WindowFlag::stickToBack, WindowFlag::transparent, WindowFlag::noBackground, WindowFlag::noTitleBar });
    }

    void TitleMenuHandheldMove(int32_t direction)
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::titleMenu))
            static_cast<TitleMenuWindow*>(window)->handheldMove(direction);
    }

    void TitleMenuHandheldConfirm()
    {
        if (auto* window = GetWindowManager()->FindByClass(WindowClass::titleMenu))
            static_cast<TitleMenuWindow*>(window)->handheldConfirm();
    }
} // namespace OpenRCT2::Ui::Windows

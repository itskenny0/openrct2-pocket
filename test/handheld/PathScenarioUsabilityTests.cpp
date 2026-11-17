// SPDX-License-Identifier: GPL-3.0-only
#include <SDL.h>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/PathSurfacePicker.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/interface/WindowBase.h>
#include <openrct2/object/ObjectLimits.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/scenario/ScenarioCategory.h>
#include <openrct2/scenario/ScenarioRepository.h>
#include <openrct2/scenario/ScenarioSources.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <stdexcept>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace W = Ui::Windows;
static void Check(bool b, const char* s)
{
    if (!b)
        throw std::runtime_error(s);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 3)
            return 2;
        SDL_setenv("OPENRCT2_HANDHELD", "1", 1);
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;
        auto env = CreatePlatformEnvironment();
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Check(context->Initialise(), "Context failed");
        auto* wm = Ui::GetWindowManager();
        wm->CloseAllExceptFlags({});
        WindowCullDead();
        auto& objects = context->GetObjectManager();
        for (auto name : { "tarmac", "dirt", "queue_blue", "queue_yellow" })
            Check(objects.LoadObject(std::string("rct2.footpath_surface.") + name), "Surface fixture missing");
        Check(objects.LoadObject("rct2.footpath_railings.wood"), "Railings fixture missing");
        auto& state = getGameState();
        gameStateInitAll(state, { 16, 16 });
        state.park.cash = 1000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        gLegacyScene = LegacyScene::playing;
        H::Paths::Begin(false);
        const auto cash = state.park.cash;
        const auto original = gFootpathSelection.normalSurface;
        const auto entries = H::PathSurfaces::Entries();
        Check(entries.size() == 2, "Normal/queue filtering failed");
        W::HandheldPathSurfacePickerOpen();
        Check(W::HandheldObjectPickerIsOpen(), "Select did not open image grid");
        W::HandheldObjectPickerMove(1, 0);
        W::HandheldMenuBack();
        Check(gFootpathSelection.normalSurface == original, "B applied unconfirmed surface");
        W::HandheldMenuBack();
        Check(H::Paths::IsActive(), "Cancel ended native path tool");
        W::HandheldPathSurfacePickerOpen();
        W::HandheldObjectPickerMove(1, 0);
        const auto selected = W::HandheldObjectPickerFocus();
        W::HandheldMenuConfirm();
        Check(gFootpathSelection.normalSurface == selected && selected != original, "A did not apply focused surface");
        Check(state.park.cash == cash && H::Paths::IsActive(), "Choosing surface changed cash/tool");
        auto stale = H::PathSurfaces::Current().value();
        objects.UnloadObjects({ objects.GetLoadedObject(ObjectType::footpathSurface, stale.index)->GetDescriptor() });
        Check(!H::PathSurfaces::Select(stale), "Removed descriptor accepted");
        Check(objects.LoadObject("rct2.footpath_surface.dirt"), "Reload failed");
        const auto valid = H::PathSurfaces::Entries().front().identity;
        Check(H::PathSurfaces::Select(valid), "Valid choice rejected");
        H::Paths::Begin(true);
        Check(H::PathSurfaces::Entries().size() == 2, "Queue choices missing");
        Check(!H::PathSurfaces::Select(valid), "Normal choice reused in queue context");
        const auto queueOriginal = gFootpathSelection.queueSurface;
        W::HandheldPathSurfacePickerOpen();
        W::HandheldObjectPickerMove(1, 0);
        W::HandheldMenuConfirm();
        Check(gFootpathSelection.queueSurface != queueOriginal, "Queue choice failed");
        H::Paths::End();
        H::Paths::BeginConnected(false);
        const auto connectedSurface = gFootpathSelection.normalSurface;
        W::HandheldPathSurfacePickerOpen();
        W::HandheldObjectPickerMove(1, 0);
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        Check(
            H::Paths::IsConnected() && gFootpathSelection.normalSurface == connectedSurface,
            "Connected cancel lost mode/surface");
        W::HandheldPathSurfacePickerOpen();
        W::HandheldObjectPickerMove(1, 0);
        W::HandheldMenuConfirm();
        Check(H::Paths::IsConnected(), "Connected picker apply ended construction");
        H::Paths::End();
        H::Paths::Begin(false);
        Check(H::PathSurfaces::Select(valid), "Normal restore failed");
        const CoordsXY tile{ 64, 64 };
        auto* surface = MapGetSurfaceElementAt(tile);
        surface->setOwnership({ OwnershipFlag::landOwned });
        InteractionInfo info;
        info.Loc = tile;
        info.Element = reinterpret_cast<TileElement*>(surface);
        info.interactionType = ViewportInteractionItem::terrain;
        H::Paths::UpdateTargetFromInfo(info);
        const auto query = H::Paths::Action(false);
        Check(query.error == GameActions::Status::ok, "Selected surface native query failed");
        Check(state.park.cash == cash, "Query spent cash");
        Check(H::Paths::Action(true).error == GameActions::Status::ok, "Selected surface placement failed");
        GameActions::ProcessQueue(state);
        Check(state.park.cash == cash - query.cost, "Native path accounting mismatch");
        H::Paths::End();
        const auto save = std::string(argv[2]) + "/selected-path.park";
        Check(ScenarioSave(state, save, {}) != 0, "Native save failed");
        Check(context->LoadParkFromFile(save), "Native reload failed");
        auto* el = MapGetFirstElementAt(tile);
        bool found = false;
        do
        {
            if (el->getType() == TileElementType::path && !el->isGhost())
            {
                found = true;
                Check(el->asPath()->getSurfaceEntryIndex() == valid.index, "Saved native surface changed");
            }
        } while (!el++->isLastForTile());
        Check(found, "Selected path absent after reload");
        std::cout
            << "PASS path grid apply/cancel, normal/queue filtering, stale descriptors, native accounting and save/reload\n";
        wm->CloseAllExceptFlags({});
        WindowCullDead();
        const auto dir = std::filesystem::u8path(
            context->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::scenarios));
        std::filesystem::create_directories(dir);
        int i = 0;
        for (auto name :
             { "Forest Frontiers", "Dynamite Dunes", "Leafy Lake", "Diamond Heights", "Evergreen Gardens", "Thunder Rock" })
        {
            state.scenarioOptions.name = name;
            state.scenarioOptions.details = "Native scenario navigation fixture";
            state.scenarioOptions.category = i == 5 ? Scenario::Category::expert : Scenario::Category::beginner;
            Check(
                ScenarioSave(state, (dir / (std::to_string(i++) + ".park")).u8string(), { SaveFlag::scenario }) != 0,
                "Scenario export failed");
        }
        ScenarioRepositoryScan();
        Check(ScenarioRepositoryGetCount() == 6, "Native fixture scan count incorrect");
        Config::Get().general.scenarioUnlockingEnabled = true;
        Config::Get().general.scenarioHideMegaPark = true;
        Config::Get().interface.scenarioSelectLastTab = 0;
        std::string loaded;
        auto* scenarioWindow = W::ScenarioselectOpen([&](std::string_view p) { loaded = p; });
        // .park indexes intentionally use Other. Exercise the original-game lock policy
        // with test-only index metadata from the same native lookup used by S6Importer.
        // This does not alter scenario files, research, park state or production indexing.
        for (size_t n = 0; n < ScenarioRepositoryGetCount(); ++n)
        {
            auto* e = const_cast<ScenarioIndexEntry*>(ScenarioRepositoryGetByIndex(n));
            SourceDescriptor desc{};
            Check(ScenarioSources::TryGetByName(e->InternalName, &desc), "Native legacy source lookup failed");
            e->SourceGame = static_cast<ScenarioSource>(desc.source);
            e->SourceIndex = desc.index;
            e->ScenarioId = desc.id;
            e->Category = desc.category;
        }
        scenarioWindow->onMouseDown(4); // Native first-source tab, rebuilt from the fixture index.
        W::ScenarioSelectHandheldMove(1);
        W::ScenarioSelectHandheldMove(5);
        W::ScenarioSelectHandheldConfirm();
        Check(loaded.empty(), "Locked park started");
        W::ScenarioSelectHandheldToggleLocking();
        Check(!Config::Get().general.scenarioUnlockingEnabled, "Native unlocking setting inaccessible");
        W::ScenarioSelectHandheldConfirm();
        Check(std::filesystem::u8path(loaded).filename() == "5.park", "Expert selection lost after unlocking toggle");
        Check(ScenarioRepositoryGetCount() == 6, "Unlocking changed repository");
        std::cout << "PASS native exports plus native-classified legacy index fixture, expert/locked navigation, guarded "
                     "start, explicit setting and stable focus\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

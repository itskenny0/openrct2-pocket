// SPDX-License-Identifier: GPL-3.0-only
// Real Pocket components / native action-created scenario fixtures. No world sprites.
#pragma once
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/handheld/StaffManagement.h>
#include <openrct2-ui/handheld/StaffPatrol.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/object/ObjectManager.h>
#include <stdexcept>
namespace StaffCompletionPreview
{
    using namespace OpenRCT2;
    namespace H = Ui::Handheld;
    namespace S = H::StaffManagement;
    namespace P = H::StaffPatrol;
    inline void Setup()
    {
        auto& state = getGameState();
        gameStateInitAll(state, { 32, 32 });
        gLegacyScene = LegacyScene::playing;
        state.park.cash = 1000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        state.park.entrances.push_back({ 64, 64, 16, 0 }); // Initial scenario fixture.
        for (const char* id :
             { "rct2.peep_animations.handyman", "rct2.peep_animations.mechanic", "rct2.peep_animations.security",
               "rct2.peep_animations.entertainer_elephant", "rct2.peep_animations.entertainer_tiger" })
            if (!GetContext()->GetObjectManager().LoadObject(id))
                throw std::runtime_error("Staff capture public metadata unavailable");
        for (int i = 0; i < 12; ++i)
        {
            if (S::Hire(static_cast<StaffType>(i % 4), true).error != GameActions::Status::ok)
                throw std::runtime_error("Staff capture hire failed");
            GameActions::ProcessQueue(state);
        }
        const auto target = S::Roster().front();
        if (S::SetName(target, S::Resolve(target)->getName(), "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345", true).error
            != GameActions::Status::ok)
            throw std::runtime_error("Staff capture naming failed");
        GameActions::ProcessQueue(state);
        if (!P::Begin(target))
            throw std::runtime_error("Staff capture patrol failed");
        P::settings = { 4, 3, false };
        if (P::Action(CoordsXY{ 320, 320 }, true).error != GameActions::Status::ok)
            throw std::runtime_error("Staff capture area failed");
        GameActions::ProcessQueue(state);
    }
    inline void Draw(Drawing::RenderTarget& rt, const std::string& page)
    {
        const auto target = S::Roster().front();
        H::ManagementPresentation model;
        if (page == "staff-depth-patrol" || page == "staff-depth-patrol-remove")
        {
            P::settings.remove = page == "staff-depth-patrol-remove";
            H::DrawHud(rt, { 512, 384 }, H::NativeHud());
            P::DrawOverlay(rt, H::Concept::kOrigin);
            H::DrawConstructionFooter(rt, { 512, 384 }, "Apply", "Add/Remove");
            H::DrawMapFocus(rt, { 256, 192 });
            H::DrawModernEllipsised(
                rt, { 268, 64 }, 236, "{MUTED}Native coverage: " + std::to_string(P::Snapshot(target).size()) + " tiles");
        }
        else if (page == "staff-depth-patrol-options")
            ConstructionPreview::Menu(rt, "STAFF PATROL", P::Options(), 2);
        else if (page == "staff-depth-patrol-clear")
            H::DrawManagementPanel(rt, { 512, 384 }, P::Presentation(*P::ReviewClear(), 1));
        else
        {
            if (page == "staff-depth-roles")
                model = S::Presentation(S::View::roles, 3);
            else if (page == "staff-depth-roster" || page == "staff-depth-filter")
            {
                const auto filter = page == "staff-depth-filter" ? std::optional<StaffType>(StaffType::security) : std::nullopt;
                const auto roster = S::Roster(filter);
                model = S::Presentation(S::View::roster, filter ? roster.size() + 3 : 10, roster);
                model.rows[roster.size() + 3] = "Filter: " + (filter ? S::RoleName(*filter) : "All staff");
            }
            else if (page == "staff-depth-costumes")
                model = S::Presentation(S::View::costumes, 1, {}, S::Roster(StaffType::entertainer).front());
            else if (page == "staff-depth-summary")
                model = S::Presentation(S::View::summary, 0);
            else if (page == "staff-depth-orders")
                model = S::Presentation(S::View::orders, 2, {}, target);
            else if (page == "staff-depth-uniform")
                model = S::UniformPresentation(target, StaffGetColour(target.type), 0);
            else if (page == "staff-depth-name" || page == "staff-depth-name-review")
                model = S::NamePresentation(
                    H::TextEntry{ S::Resolve(target)->getName() }, page == "staff-depth-name" ? 46 : 1,
                    page == "staff-depth-name-review");
            else
                model = S::Presentation(S::View::inspection, page == "staff-depth-dense" ? 8 : 1, {}, target);
            H::Dashboard::State dashboard;
            dashboard.section = H::Dashboard::Section::staff;
            dashboard.focus = H::Dashboard::Focus::content;
            H::Dashboard::Draw(
                rt, { 512, 384 }, dashboard, model,
                page == "staff-depth-roster" || page == "staff-depth-filter" || page == "staff-depth-inspection"
                    || page == "staff-depth-dense");
        }
        // Outside production panels, never a composited or invented gameplay backdrop.
        if (page == "staff-depth-patrol" || page == "staff-depth-patrol-remove")
        {
            H::DrawModernEllipsised(rt, { 268, 38 }, 236, "{MUTED}COMPONENT FIXTURE");
            H::DrawModernEllipsised(rt, { 268, 50 }, 236, "{MUTED}Native actions / no world sprites");
        }
        else
            H::DrawModernEllipsised(rt, { 8, 0 }, 496, "{MUTED}COMPONENT FIXTURE / native staff actions / no world sprites");
        P::End();
    }
} // namespace StaffCompletionPreview

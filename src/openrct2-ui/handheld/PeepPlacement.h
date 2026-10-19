// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ConstructionPanel.h"
#include "GuestManagement.h"
#include "StaffManagement.h"

#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/actions/peep/PeepPickupAction.h>
#include <openrct2/drawing/PickupPeep.h>
#include <openrct2/network/Network.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/PeepAnimationsObject.h>
namespace OpenRCT2::Ui::Handheld::PeepPlacement
{
    struct Target
    {
        std::optional<GuestManagement::Target> guest;
        std::optional<StaffManagement::Target> staff;
    };
    inline std::optional<Target> target;
    inline bool busy{}, finished{};
    inline uint64_t revision{};
    inline uint8_t frame{};
    inline const Peep* Resolve(const Target& t)
    {
        if (t.guest && !t.staff)
            return GuestManagement::Resolve(*t.guest);
        if (t.staff && !t.guest)
            return StaffManagement::Resolve(*t.staff);
        return nullptr;
    }
    inline bool Active()
    {
        return target.has_value();
    }
    inline GameActions::Result Invalid(const char* text)
    {
        GameActions::Result result;
        result.error = GameActions::Status::disallowed;
        result.errorMessage = text;
        return result;
    }
    inline GameActions::Result CancelOwned(const Peep* peep)
    {
        const auto owner = Network::GetCurrentPlayerId();
        if (!peep || Network::GetPickupPeep(owner) != peep)
            return Invalid("Pickup no longer owned");
        GameActions::PeepPickupAction action(
            GameActions::PeepPickupType::cancel, peep->id, { Network::GetPickupPeepOldX(owner), 0, 0 }, owner);
        return GameActions::IsProcessingQueue() ? GameActions::ExecuteNested(&action, getGameState())
                                                : GameActions::Execute(&action, getGameState());
    }
    inline void End()
    {
        const auto old = target;
        target.reset();
        busy = finished = false;
        ++revision;
        Drawing::pickupPeepClear();
        if (old)
        {
            // An owner can become ineligible for roster inspection (for example
            // leaving the park) while still owned by this native pickup session.
            const auto* owned = Network::GetPickupPeep(Network::GetCurrentPlayerId());
            const auto id = old->guest ? old->guest->id : old->staff->id;
            if (owned && owned->id == id)
                CancelOwned(owned);
        }
        // Flush ordinary due commands before load/quit can destroy their target.
        // No direct position/ownership/network pointer mutation is performed.
        if (old && !gInUpdateCode && !GameActions::IsProcessingQueue() && Network::GetMode() == Network::Mode::none)
            GameActions::ProcessQueue(getGameState());
    }
    inline GameActions::Result Begin(const Target& t)
    {
        const auto* peep = Resolve(t);
        if (Active() || !peep || Network::GetPickupPeep(Network::GetCurrentPlayerId()))
            return Invalid("Person or pickup unavailable");
        GameActions::PeepPickupAction action(
            GameActions::PeepPickupType::pickup, peep->id, { kLocationNull, 0, 0 }, Network::GetCurrentPlayerId());
        const auto query = GameActions::Query(&action, getGameState());
        if (query.error != GameActions::Status::ok)
            return query;
        target = t;
        busy = true;
        finished = false;
        const auto request = ++revision;
        action.SetCallback([t, request](const auto*, const auto* result) {
            if (request != revision)
            {
                if (result->error == GameActions::Status::ok)
                    CancelOwned(Resolve(t));
                return;
            }
            busy = false;
            if (result->error != GameActions::Status::ok)
                target.reset();
        });
        const auto result = GameActions::Execute(&action, getGameState());
        if (result.error != GameActions::Status::ok)
        {
            target.reset();
            busy = false;
        }
        return result;
    }
    inline GameActions::Result Place(std::optional<CoordsXYZ> location, bool execute)
    {
        const auto* peep = target ? Resolve(*target) : nullptr;
        if (!peep || busy || finished || !location || Network::GetPickupPeep(Network::GetCurrentPlayerId()) != peep
            || peep->state != PeepState::picked)
            return Invalid("Select a valid placement location");
        GameActions::PeepPickupAction action(
            GameActions::PeepPickupType::place, peep->id, *location, Network::GetCurrentPlayerId());
        if (!execute)
            return GameActions::Query(&action, getGameState());
        busy = true;
        const auto request = revision;
        action.SetCallback([request](const auto*, const auto* result) {
            if (request == revision)
            {
                busy = false;
                finished = result->error == GameActions::Status::ok;
            }
        });
        const auto result = GameActions::Execute(&action, getGameState());
        if (result.error != GameActions::Status::ok)
            busy = false;
        return result;
    }
    inline void Preview(bool visible, ScreenCoordsXY centre)
    {
        Drawing::pickupPeepClear();
        const auto* peep = target ? Resolve(*target) : nullptr;
        if (!visible || !peep || busy || gOpenRCT2NoGraphics || peep->state != PeepState::picked)
            return;
        const auto* object = GetContext()->GetObjectManager().GetLoadedObject<PeepAnimationsObject>(peep->animationObjectIndex);
        if (!object)
            return;
        const auto image = object->GetPeepAnimation(peep->animationGroup, PeepAnimationType::hanging).baseImage;
        Drawing::pickupPeepSetPosition({ centre.x - 1, centre.y + 16 });
        Drawing::pickupPeepSetImage(image + (frame >> 2), peep->tShirtColour, peep->trousersColour);
        frame = (frame + 1) % 48;
    }
    inline void DrawOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY pos)
    {
        const auto* peep = target ? Resolve(*target) : nullptr;
        DrawConstructionOverlay(
            rt, pos, "Place person", peep ? peep->getName() : "Person unavailable",
            busy ? "Placement pending" : "A: Place / B: Cancel", "Land or path", "Select: Options");
    }
    inline ManagementPresentation CancelPresentation(size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Person / Cancel placement";
        const auto* peep = target ? Resolve(*target) : nullptr;
        model.title = peep ? peep->getName() : "Person unavailable";
        model.rows = { "Cancel pickup", "Keep placing" };
        model.selected = selected;
        model.confirmation = true;
        model.status = Network::GetPickupPeepOldX(Network::GetCurrentPlayerId()) == kLocationNull
            ? "No previous position; place this person first" : "Returns to the previous position";
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::PeepPlacement

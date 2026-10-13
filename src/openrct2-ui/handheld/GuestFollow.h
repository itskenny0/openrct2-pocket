// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "GuestManagement.h"

#include <openrct2/OpenRCT2.h>
#include <openrct2/interface/Window.h>
#include <openrct2/interface/WindowBase.h>

namespace OpenRCT2::Ui::Handheld::GuestFollow
{
    // Transient identity guard only; native viewport APIs own the camera and motion.
    inline std::optional<GuestManagement::Target> target;
    inline void Stop()
    {
        if (const auto* identity = target ? &*target : nullptr)
            if (auto* main = WindowGetMain(); main && main->viewportSmartFollowSprite == identity->id)
                WindowUnfollowSprite(*main);
        target.reset();
    }
    inline void Update()
    {
        if (!target)
            return;
        const auto* guest = GuestManagement::Resolve(*target);
        const auto* main = WindowGetMain();
        if (gLegacyScene != LegacyScene::playing || !guest || guest->state == PeepState::picked || !main || !main->viewport)
            Stop();
        else if (main->viewportSmartFollowSprite != target->id)
            target.reset(); // Native manual pan/locate or another camera owner ended our follow.
    }
    inline std::optional<GuestManagement::Target> Current()
    {
        Update();
        return target;
    }
    inline bool Start(const GuestManagement::Target& identity)
    {
        const auto* guest = GuestManagement::Resolve(identity);
        auto* main = WindowGetMain();
        if (gLegacyScene != LegacyScene::playing || !guest || guest->state == PeepState::picked || !main || !main->viewport
            || !GuestManagement::Location(identity))
            return false;
        Stop();
        WindowFollowSprite(*main, identity.id); // Same native API as desktop GuestFollow.
        target = identity;
        return true;
    }
    inline void Invalidate(EntityId id)
    {
        if (target && target->id == id)
            Stop(); // Called before native peep removal, preventing slot reuse from moving the camera.
    }
}

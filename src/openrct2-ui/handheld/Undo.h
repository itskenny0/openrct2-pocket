// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/actions/GameActionResult.h>
#include <openrct2/core/Money.hpp>
#include <openrct2/world/Location.hpp>
#include <string>

namespace OpenRCT2::Ui::Handheld::Undo
{
    // One step of undo for the park tools, taken by holding B. OpenRCT2 has no undo of its own,
    // so each tool snapshots what it is about to change and undo puts that back:
    // - land: the height, slope and water of every surface tile in reach, restored exactly;
    // - scenery, trees and shops: what is on those tiles, so anything new of that kind is removed.
    // The comparison happens when you undo, because actions are queued and the map has not changed
    // yet when the tool's action returns.
    enum class Kind
    {
        land,
        scenery,
        trees,
        shop
    };
    // Before the action: remember tiles within radius of centre.
    void Prepare(Kind kind, CoordsXY centre, int radius, std::string label);
    // After it succeeded: the prepared snapshot becomes the undo step. cost is what the action
    // charged, refunded by undo along with whatever the undo itself costs.
    void Commit(money64 cost = 0);
    // The action failed, or the tool ended.
    void Clear();
    bool Available();
    const std::string& Label();
    GameActions::Result Perform();
} // namespace OpenRCT2::Ui::Handheld::Undo

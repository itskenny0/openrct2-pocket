// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <openrct2/world/Location.hpp>

namespace OpenRCT2::Ui::Handheld::Alerts
{
    // What needs the player right now, most urgent first. Each alert says what is wrong in plain
    // words, what usually fixes it, and where it is, so Y in the park can glide the camera there and
    // A opens whatever is under the cursor.
    enum class Kind
    {
        crash,
        breakdown,
        queueFull,
        hungry,
        thirsty,
        toilet,
        dirty,
        vandalism,
        lost
    };
    struct Alert
    {
        Kind kind;
        std::string title;
        std::string fix;
        CoordsXYZ location;
        std::optional<uint16_t> ride;
    };
    // Recomputed at most every couple of seconds; walking every guest's thoughts each frame would
    // be wasted work for a list that changes slowly.
    const std::vector<Alert>& Current();
    void Invalidate();
    // "Alerts (3)" for footers, or "News" when nothing needs attention.
    std::string HintLabel();
} // namespace OpenRCT2::Ui::Handheld::Alerts

// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "RideManagement.h"
namespace OpenRCT2::Ui::Handheld::RideAccess
{
    struct Point
    {
        CoordsXYZD outside;
        bool attached{};
        bool reachesNormalPath{};
    };
    struct State
    {
        Point entrance, exit;
    };
    bool Supported(const RideManagement::Target& target);
    std::optional<State> Inspect(const RideManagement::Target& target);
    ManagementPresentation Presentation(
        const RideManagement::Target& target, size_t selected, const std::string& feedback = {});
    bool BeginPaths(const RideManagement::Target& target, bool exit);
} // namespace OpenRCT2::Ui::Handheld::RideAccess

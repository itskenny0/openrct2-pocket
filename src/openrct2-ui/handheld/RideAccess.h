// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "RideManagement.h"

#include <openrct2/actions/ride/FlatRideEntrancePlaceAction.h>
namespace OpenRCT2::Ui::Handheld::RideAccess
{
    struct Point
    {
        CoordsXYZD outside;
        bool attached{};
        bool reachesNormalPath{};
        StationIndex station{ StationIndex::FromUnderlying(0) };
        bool reachesParkEntrance{};
    };
    struct State
    {
        Point entrance, exit;
    };
    struct ExistingPoint
    {
        RideManagement::Target target;
        StationIndex station;
        bool exit{};
        CoordsXYZD source;
        std::vector<int64_t> context;
    };
    struct EditDraft
    {
        ExistingPoint point;
        CoordsXYZD destination;
        bool remove{};
        GameActions::FlatRideEntrancePlaceAction action;
        money64 cost{};
    };
    std::vector<ExistingPoint> Existing(const RideManagement::Target& target);
    bool Current(const ExistingPoint& point);
    std::optional<EditDraft> Review(const ExistingPoint& point, std::optional<CoordsXYZD> destination, std::string& error);
    ManagementPresentation ReviewPresentation(const EditDraft& draft, size_t selected, const std::string& feedback = {});
    std::string PointLabel(const ExistingPoint& point);
    bool BeginRelocation(const ExistingPoint& point);
    bool Supported(const RideManagement::Target& target);
    std::optional<State> Inspect(const RideManagement::Target& target);
    ManagementPresentation Presentation(
        const RideManagement::Target& target, size_t selected, const std::string& feedback = {});
    bool NormalPathReachesParkEntrance(CoordsXYZ position);
    bool PointValid(const RideManagement::Target& target, StationIndex station, bool exit, CoordsXYZD outside);
    bool BeginPaths(const RideManagement::Target& target, bool exit, bool connected = false);
} // namespace OpenRCT2::Ui::Handheld::RideAccess

namespace OpenRCT2::Ui::Windows
{
    void HandheldAccessReviewOpen(const Handheld::RideAccess::EditDraft& draft);
}

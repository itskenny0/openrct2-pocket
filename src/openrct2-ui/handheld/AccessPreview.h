// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "RideManagement.h"
#include <openrct2/actions/ride/FlatRideEntrancePlaceAction.h>
namespace OpenRCT2::Ui::Handheld::AccessPreview
{
    void Update(const RideManagement::Target& target,CoordsXYZD location,bool exit,StationIndex station,bool relocation = false);
    void Clear();
    void Suspend();
    void Restore();
    GameActions::Result Bind(GameActions::FlatRideEntrancePlaceAction& action,CoordsXYZD location);
}

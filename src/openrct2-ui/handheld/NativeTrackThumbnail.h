// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/ride/RideConstruction.h>
#include <openrct2/ride/ted/TrackElemType.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/world/Location.hpp>
namespace OpenRCT2::Ui::Windows {
void DrawNativeTrackThumbnail(Drawing::RenderTarget&, RideId, TrackElemType, Direction, SelectedLiftAndInverted);
}

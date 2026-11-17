// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"
#include "RideManagement.h"

#include <openrct2/ride/TrackConstructionSupport.h>
#include <openrct2/ride/ted/TrackElemType.h>
namespace OpenRCT2::Ui::Handheld::CustomTracks
{
    bool Eligible(ObjectEntryIndex index, ride_type_t type = kRideTypeNull);
    std::vector<PickerEntry> RideEntries();
    bool Begin(ObjectEntryIndex index, ride_type_t type = kRideTypeNull);
    bool CanResume(const RideManagement::Target& target);
    bool Resume(const RideManagement::Target& target);
    bool ResumeFrom(
        const RideManagement::Target& target, std::optional<std::pair<CoordsXYZD, TrackElemType>> source,
        bool backward = false);
    bool ResumeReplacement(
        const RideManagement::Target& target, const TrackConstructionSupport::BlockSnapshot& removed, uint8_t colour);
    bool Replacing();
    void End();
    bool Active();
    bool Busy();
    bool StationPlacement();
    bool CameraMode();
    bool Backward();
    bool PlacementInverted();
    bool ToggleInverted();
    bool ToggleCovered();
    void ToggleCamera();
    void Rotate();
    bool Select(TrackElemType type);
    void Cycle(int direction);
    void Slope(int direction);
    // D-pad steering: left/right bends the track, the opposite way straightens it. L1/R1 picks how
    // tight the bend is. Returns false (with a toast) when the ride has no piece of that shape here.
    bool Steer(int direction);
    void SetTurnSize(int direction);
    int CurrentTurn();
    int TurnSize();
    std::string TurnSizeName();
    uint8_t PlacementSpeed();
    bool SetPlacementSpeed(uint8_t speed);
    bool SeatSupported();
    uint8_t PlacementSeat();
    bool SetPlacementSeat(uint8_t seat);
    void Bank();
    void Chain();
    enum class PieceGroup
    {
        all,
        straight,
        station,
        turns,
        slopes,
        banks,
        special,
        count
    };
    PieceGroup GroupOf(TrackElemType);
    PieceGroup CurrentGroup();
    std::string GroupName(PieceGroup);
    void NextGroup();
    std::vector<PickerEntry> Pieces(bool filtered = false);
    bool HasPreview();
    std::string PieceName(TrackElemType type);
    void DrawPiecePreview(Drawing::RenderTarget& rt, TrackElemType type);
    TrackElemType Selection();
    std::optional<CoordsXYZD> Endpoint();
    std::optional<RideManagement::Target> Target();
    std::optional<RideManagement::Target> Completed();
    GameActions::Result Action(bool execute);
    GameActions::Result Undo(bool execute = true);
    void Update(std::optional<CoordsXY> tile);
    void ClearPreview();
    void RemovePreviewTemporarily();
    void RestorePreview();
    void DrawPanel(Drawing::RenderTarget& rt, ScreenSize screen);
} // namespace OpenRCT2::Ui::Handheld::CustomTracks

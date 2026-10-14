// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../Identifiers.h"
#include <cstdint>
#include <unordered_map>

namespace OpenRCT2
{
    // Process-local allocation identity for guarded actions. Never park-save state.
    inline std::unordered_map<uint16_t, uint64_t> RideAllocationGenerations;
    inline uint64_t RideAllocationGeneration(RideId id)
    {
        const auto it=RideAllocationGenerations.find(id.ToUnderlying());
        return it==RideAllocationGenerations.end()?0:it->second;
    }
    inline void RideAllocationChanged(RideId id) { ++RideAllocationGenerations[id.ToUnderlying()]; }
}

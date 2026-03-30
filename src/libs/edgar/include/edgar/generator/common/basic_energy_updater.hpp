#pragma once

#include "edgar/generator/common/energy_data.hpp"

namespace edgar::generator::common {

/// Maps `EnergyData` to a scalar for minimization (C# `BasicEnergyUpdater` subset).
struct BasicEnergyUpdater {
    static double total_penalty(const EnergyData& e, double energy_scale = 1.0) {
        return energy_scale * (e.overlap_penalty + e.corridor_penalty + e.minimum_distance_penalty);
    }
};

} // namespace edgar::generator::common

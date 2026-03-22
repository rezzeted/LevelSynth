#pragma once

#include "edgar/generator/common/energy_data.hpp"

namespace edgar::generator::common {

/// Maps `EnergyData` to a scalar for minimization (C# `BasicEnergyUpdater` subset).
struct BasicEnergyUpdater {
    static double total_penalty(const EnergyData& e) { return e.overlap_penalty; }
};

} // namespace edgar::generator::common

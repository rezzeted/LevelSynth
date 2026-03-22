#pragma once

namespace edgar::generator::common {

/// Aggregated constraint penalties for a layout (ported subset of Edgar `EnergyData`).
struct EnergyData {
    double overlap_penalty = 0;
};

} // namespace edgar::generator::common

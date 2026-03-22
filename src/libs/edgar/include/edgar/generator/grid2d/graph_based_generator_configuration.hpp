#pragma once

namespace edgar::generator::grid2d {

/// Subset of C# GraphBasedGeneratorConfiguration relevant to the C++ port.
struct GraphBasedGeneratorConfiguration {
    /// Extra spacing multiplier when strip-packing rooms (implementation detail).
    int strip_gap_cells = 0;
};

} // namespace edgar::generator::grid2d

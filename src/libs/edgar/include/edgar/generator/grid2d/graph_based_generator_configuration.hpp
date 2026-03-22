#pragma once

#include "edgar/generator/common/simulated_annealing_configuration.hpp"

namespace edgar::generator::grid2d {

enum class GraphBasedGeneratorBackend {
    /// Fast strip packer + Clipper2 overlap (original C++ port).
    strip_packing = 0,
    /// Chain decomposition (`BreadthFirstChainDecompositionOld`) + simulated annealing (C#-style pipeline subset).
    chain_simulated_annealing = 1,
};

/// Mirrors a subset of C# `GraphBasedGeneratorConfiguration` plus C++ backend selection.
struct GraphBasedGeneratorConfiguration {
    GraphBasedGeneratorBackend backend = GraphBasedGeneratorBackend::chain_simulated_annealing;

    /// Extra spacing multiplier when strip-packing rooms (implementation detail).
    int strip_gap_cells = 0;

    common::SimulatedAnnealingConfiguration simulated_annealing{};
};

} // namespace edgar::generator::grid2d

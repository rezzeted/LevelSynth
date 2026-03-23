#include <gtest/gtest.h>

#include <random>
#include <unordered_set>

#include "edgar/chain_decompositions/breadth_first_chain_decomposition.hpp"
#include "edgar/chain_decompositions/breadth_first_chain_decomposition_old.hpp"
#include "edgar/chain_decompositions/two_stage_chain_decomposition.hpp"
#include "edgar/edgar.hpp"
#include "edgar/generator/common/basic_energy_updater.hpp"
#include "edgar/generator/grid2d/configuration_spaces_generator.hpp"
#include "edgar/generator/grid2d/configuration_spaces_grid2d.hpp"
#include "edgar/generator/grid2d/constraints_evaluator_grid2d.hpp"
#include "edgar/generator/grid2d/detail/room_index_map.hpp"
#include "edgar/generator/grid2d/chain_based_generator_grid2d.hpp"
#include "edgar/generator/grid2d/graph_based_generator_configuration.hpp"
#include "edgar/generator/common/simulated_annealing_configuration.hpp"
#include "edgar/io/layout_json.hpp"
#include "edgar/io/png_rgba.hpp"
#include "edgar/graphs/graph_algorithms.hpp"
#include "edgar/graphs/undirected_graph.hpp"
#include "edgar/geometry/bipartite_matching.hpp"
#include "edgar/geometry/clipper2_util.hpp"
#include "edgar/geometry/grid_polygon_partitioning.hpp"
#include "edgar/geometry/orthogonal_line_grid2d.hpp"
#include "edgar/geometry/overlap.hpp"
#include "edgar/geometry/polygon_overlap_grid2d.hpp"
#include "edgar/generator/grid2d/simple_door_mode_grid2d.hpp"

TEST(EdgarChainDecomposition, BreadthFirstOld_CoversAllVertices_TwoGraphs) {
    using namespace edgar::chain_decompositions;
    using namespace edgar::graphs;

    {
        UndirectedAdjacencyListGraph<int> graph;
        for (int i = 0; i <= 4; ++i) {
            graph.add_vertex(i);
        }
        graph.add_edge(0, 1);
        graph.add_edge(1, 2);
        graph.add_edge(2, 0);
        graph.add_edge(1, 3);
        graph.add_edge(3, 4);
        graph.add_edge(4, 1);

        BreadthFirstChainDecompositionOld decomposer;
        const auto chains = decomposer.get_chains(graph);
        std::unordered_set<int> seen;
        for (const auto& ch : chains) {
            for (int n : ch.nodes) {
                seen.insert(n);
            }
        }
        EXPECT_EQ(seen.size(), graph.vertex_count());
    }

    {
        UndirectedAdjacencyListGraph<int> graph;
        for (int i = 0; i < 7; ++i) {
            graph.add_vertex(i);
        }
        graph.add_edge(0, 1);
        graph.add_edge(1, 2);
        graph.add_edge(2, 3);
        graph.add_edge(4, 1);
        graph.add_edge(1, 5);
        graph.add_edge(5, 6);

        BreadthFirstChainDecompositionOld decomposer;
        const auto chains = decomposer.get_chains(graph);
        std::unordered_set<int> seen;
        for (const auto& ch : chains) {
            for (int n : ch.nodes) {
                seen.insert(n);
            }
        }
        EXPECT_EQ(seen.size(), graph.vertex_count());
    }
}

TEST(EdgarChainDecomposition, BreadthFirstNew_CoversAllVertices_TriangleWithTail) {
    using namespace edgar::chain_decompositions;
    using namespace edgar::graphs;

    UndirectedAdjacencyListGraph<int> graph;
    for (int i = 0; i <= 4; ++i) {
        graph.add_vertex(i);
    }
    graph.add_edge(0, 1);
    graph.add_edge(1, 2);
    graph.add_edge(2, 0);
    graph.add_edge(1, 3);
    graph.add_edge(3, 4);
    graph.add_edge(4, 1);

    BreadthFirstChainDecomposition decomposer;
    const auto chains = decomposer.get_chains(graph);
    std::unordered_set<int> seen;
    for (const auto& ch : chains) {
        for (int n : ch.nodes) {
            seen.insert(n);
        }
    }
    EXPECT_EQ(seen.size(), graph.vertex_count());
}

TEST(EdgarChainDecomposition, TwoStage_EmbedsStageTwoRoom) {
    using namespace edgar::chain_decompositions;
    using namespace edgar::generator::grid2d;
    using namespace edgar::geometry;

    auto square = RoomTemplateGrid2D(PolygonGrid2D::get_square(4), std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D stage_one(false, {square}, 1);
    RoomDescriptionGrid2D stage_two(false, {square}, 2);

    LevelDescriptionGrid2D<int> level;
    level.add_room(0, stage_one);
    level.add_room(1, stage_one);
    level.add_room(2, stage_one);
    level.add_room(3, stage_two);
    level.add_connection(0, 1);
    level.add_connection(1, 2);
    level.add_connection(1, 3);

    edgar::generator::grid2d::detail::RoomIndexMap<int> rmap(level);
    BreadthFirstChainDecomposition inner;
    TwoStageChainDecomposition<int> ts(level, rmap, inner);
    const auto ig = rmap.int_graph(level);
    const auto chains = ts.get_chains(ig);
    std::unordered_set<int> seen;
    for (const auto& ch : chains) {
        for (int n : ch.nodes) {
            seen.insert(n);
        }
    }
    EXPECT_EQ(seen.size(), 4u);
}

namespace {

bool rect_eq(const edgar::geometry::RectangleGrid2D& a, const edgar::geometry::RectangleGrid2D& b) {
    return a.a == b.a && a.b == b.b;
}

bool partition_matches_any(const std::vector<edgar::geometry::RectangleGrid2D>& got,
                           const std::vector<std::vector<edgar::geometry::RectangleGrid2D>>& alternatives) {
    using edgar::geometry::RectangleGrid2D;
    for (const std::vector<RectangleGrid2D>& exp : alternatives) {
        if (exp.size() != got.size()) {
            continue;
        }
        bool ok = true;
        for (const RectangleGrid2D& r : exp) {
            bool found = false;
            for (const RectangleGrid2D& g : got) {
                if (rect_eq(g, r)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                ok = false;
                break;
            }
        }
        if (ok) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST(EdgarGeometry, BipartiteVertexCover_Basic) {
    using namespace edgar::geometry;
    {
        const auto c = bipartite_min_vertex_cover(2, 3, {{0, 0}, {0, 1}, {1, 1}, {1, 2}});
        EXPECT_EQ(c.first.size(), 2u);
        EXPECT_EQ(c.second.size(), 0u);
    }
    {
        const auto c = bipartite_min_vertex_cover(4, 2, {{0, 0}, {1, 0}, {1, 1}, {2, 1}, {3, 1}});
        EXPECT_EQ(c.first.size(), 0u);
        EXPECT_EQ(c.second.size(), 2u);
    }
}

TEST(EdgarGeometry, BipartiteIndependentSet_Basic) {
    using namespace edgar::geometry;
    {
        const auto s = bipartite_max_independent_set(2, 3, {{0, 0}, {0, 1}, {1, 1}, {1, 2}});
        EXPECT_EQ(s.size(), 3u);
    }
    {
        const auto s = bipartite_max_independent_set(4, 2, {{0, 0}, {1, 0}, {1, 1}, {2, 1}, {3, 1}});
        EXPECT_EQ(s.size(), 4u);
    }
}

TEST(EdgarGeometry, GridPolygonPartitioning_LShape) {
    using namespace edgar::geometry;
    const PolygonGrid2D poly = PolygonGrid2DBuilder()
                                   .add_point(0, 0)
                                   .add_point(0, 6)
                                   .add_point(3, 6)
                                   .add_point(3, 3)
                                   .add_point(7, 3)
                                   .add_point(7, 0)
                                   .build();
    const auto parts = partition_orthogonal_polygon_to_rectangles(poly);
    EXPECT_EQ(parts.size(), 2u);
    const std::vector<std::vector<RectangleGrid2D>> alts = {
        {RectangleGrid2D({0, 3}, {3, 6}), RectangleGrid2D({0, 0}, {7, 3})},
        {RectangleGrid2D({0, 0}, {3, 6}), RectangleGrid2D({3, 0}, {7, 3})},
    };
    EXPECT_TRUE(partition_matches_any(parts, alts));
}

TEST(EdgarGeometry, GridPolygonPartitioning_AnotherShape) {
    using namespace edgar::geometry;
    const PolygonGrid2D poly = PolygonGrid2DBuilder()
                                   .add_point(0, 0)
                                   .add_point(0, 3)
                                   .add_point(-1, 3)
                                   .add_point(-1, 5)
                                   .add_point(5, 5)
                                   .add_point(5, 3)
                                   .add_point(4, 3)
                                   .add_point(4, 0)
                                   .add_point(5, 0)
                                   .add_point(5, -2)
                                   .add_point(-1, -2)
                                   .add_point(-1, 0)
                                   .build();
    const auto parts = partition_orthogonal_polygon_to_rectangles(poly);
    EXPECT_EQ(parts.size(), 3u);
    const std::vector<std::vector<RectangleGrid2D>> alts = {
        {RectangleGrid2D({-1, -2}, {5, 0}), RectangleGrid2D({0, 0}, {4, 3}), RectangleGrid2D({-1, 3}, {5, 5})},
    };
    EXPECT_TRUE(partition_matches_any(parts, alts));
}

TEST(EdgarGeometry, GridPolygonPartitioning_ComplexShape) {
    using namespace edgar::geometry;
    const PolygonGrid2D poly =
        PolygonGrid2DBuilder()
            .add_point(2, 0)
            .add_point(2, 1)
            .add_point(1, 1)
            .add_point(1, 2)
            .add_point(0, 2)
            .add_point(0, 7)
            .add_point(1, 7)
            .add_point(1, 8)
            .add_point(2, 8)
            .add_point(2, 9)
            .add_point(7, 9)
            .add_point(7, 8)
            .add_point(8, 8)
            .add_point(8, 7)
            .add_point(9, 7)
            .add_point(9, 2)
            .add_point(8, 2)
            .add_point(8, 1)
            .add_point(7, 1)
            .add_point(7, 0)
            .build();
    const auto parts = partition_orthogonal_polygon_to_rectangles(poly);
    EXPECT_EQ(parts.size(), 5u);
    const std::vector<std::vector<RectangleGrid2D>> alts = {
        {RectangleGrid2D({2, 0}, {7, 1}), RectangleGrid2D({1, 1}, {8, 2}), RectangleGrid2D({0, 2}, {9, 7}),
         RectangleGrid2D({1, 7}, {8, 8}), RectangleGrid2D({2, 8}, {7, 9})},
        {RectangleGrid2D({0, 2}, {1, 7}), RectangleGrid2D({1, 1}, {2, 8}), RectangleGrid2D({2, 0}, {7, 9}),
         RectangleGrid2D({7, 1}, {8, 8}), RectangleGrid2D({8, 2}, {9, 7})},
    };
    EXPECT_TRUE(partition_matches_any(parts, alts));
}

TEST(EdgarGeometry, GridPolygonPartitioning_PlusShape) {
    using namespace edgar::geometry;
    const PolygonGrid2D poly =
        PolygonGrid2DBuilder()
            .add_point(0, 2)
            .add_point(0, 4)
            .add_point(2, 4)
            .add_point(2, 6)
            .add_point(4, 6)
            .add_point(4, 4)
            .add_point(6, 4)
            .add_point(6, 2)
            .add_point(4, 2)
            .add_point(4, 0)
            .add_point(2, 0)
            .add_point(2, 2)
            .build();
    const auto parts = partition_orthogonal_polygon_to_rectangles(poly);
    EXPECT_EQ(parts.size(), 3u);
    const std::vector<std::vector<RectangleGrid2D>> alts = {
        {RectangleGrid2D({2, 0}, {4, 2}), RectangleGrid2D({0, 2}, {6, 4}), RectangleGrid2D({2, 4}, {4, 6})},
        {RectangleGrid2D({0, 2}, {2, 4}), RectangleGrid2D({2, 0}, {4, 6}), RectangleGrid2D({4, 2}, {6, 4})},
    };
    EXPECT_TRUE(partition_matches_any(parts, alts));
}

TEST(EdgarGeometry, OverlapAlongLine_discreteSweep) {
    using namespace edgar::geometry;
    const auto a = PolygonGrid2D::get_rectangle(3, 2);
    const auto b = PolygonGrid2D::get_rectangle(3, 2);
    const OrthogonalLineGrid2D line({-5, 0}, {5, 0});
    const auto ev = overlap_along_line(a, b, line);
    EXPECT_FALSE(ev.empty());
}

TEST(EdgarGeometry, OverlapAlongLine_mergedMatchesBruteforce) {
    using namespace edgar::geometry;
    const auto a = PolygonGrid2D::get_rectangle(3, 2);
    const auto b = PolygonGrid2D::get_rectangle(3, 2);
    const OrthogonalLineGrid2D line_h({-5, 0}, {5, 0});
    const OrthogonalLineGrid2D line_v({0, -4}, {0, 4});
    const auto merged_h = overlap_along_line_polygon_partition(a, b, line_h);
    const auto brute_h = edgar::geometry::detail::overlap_along_line_polygon_partition_bruteforce(a, b, line_h);
    EXPECT_EQ(merged_h.size(), brute_h.size());
    for (std::size_t i = 0; i < merged_h.size(); ++i) {
        EXPECT_EQ(merged_h[i].first.x, brute_h[i].first.x);
        EXPECT_EQ(merged_h[i].first.y, brute_h[i].first.y);
        EXPECT_EQ(merged_h[i].second, brute_h[i].second);
    }
    const auto merged_v = overlap_along_line_polygon_partition(a, b, line_v);
    const auto brute_v = edgar::geometry::detail::overlap_along_line_polygon_partition_bruteforce(a, b, line_v);
    EXPECT_EQ(merged_v.size(), brute_v.size());
    for (std::size_t i = 0; i < merged_v.size(); ++i) {
        EXPECT_EQ(merged_v[i].first.x, brute_v[i].first.x);
        EXPECT_EQ(merged_v[i].first.y, brute_v[i].first.y);
        EXPECT_EQ(merged_v[i].second, brute_v[i].second);
    }
}

TEST(EdgarGraphs, IsTree_pathAndTriangle) {
    using namespace edgar::graphs;
    {
        UndirectedAdjacencyListGraph<int> g;
        g.add_vertex(0);
        EXPECT_TRUE(is_tree(g));
        g.add_vertex(1);
        g.add_edge(0, 1);
        EXPECT_TRUE(is_tree(g));
        g.add_vertex(2);
        g.add_edge(1, 2);
        EXPECT_TRUE(is_tree(g));
    }
    {
        UndirectedAdjacencyListGraph<int> g;
        for (int i = 0; i < 3; ++i) {
            g.add_vertex(i);
        }
        g.add_edge(0, 1);
        g.add_edge(1, 2);
        g.add_edge(0, 2);
        EXPECT_FALSE(is_tree(g));
    }
}

TEST(EdgarConfigSpaces, ConfigurationSpacesGenerator_nonEmptyForMatchingSquares) {
    using namespace edgar::geometry;
    using namespace edgar::generator::grid2d;
    const auto poly = PolygonGrid2D::get_square(8);
    SimpleDoorModeGrid2D mode(1, 1);
    const std::vector<DoorLineGrid2D> doors = mode.get_doors(poly);
    ConfigurationSpacesGenerator gen;
    const auto cs = gen.get_configuration_space(poly, doors, poly, doors);
    EXPECT_FALSE(cs.lines.empty());
}

TEST(EdgarGeometry, RectanglePolygonClockwise) {
    using namespace edgar::geometry;
    const auto poly = edgar::geometry::PolygonGrid2D::get_rectangle(6, 10);
    EXPECT_GE(poly.points().size(), 4u);
    EXPECT_EQ(poly.bounding_rectangle().width(), 6);
    EXPECT_EQ(poly.bounding_rectangle().height(), 10);
}

TEST(EdgarConfigSpaces, CompatibleNonOverlapping_twoRects) {
    using namespace edgar::geometry;
    using namespace edgar::generator::grid2d;
    const auto a = PolygonGrid2D::get_rectangle(2, 2);
    const auto b = PolygonGrid2D::get_rectangle(2, 2);
    EXPECT_TRUE(ConfigurationSpacesGrid2D::compatible_non_overlapping(a, {0, 0}, b, {4, 0}));
    EXPECT_FALSE(ConfigurationSpacesGrid2D::compatible_non_overlapping(a, {0, 0}, b, {1, 0}));
}

TEST(EdgarEnergy, ConstraintsEvaluator_noOverlapZeroPenalty) {
    using namespace edgar::geometry;
    using namespace edgar::generator::grid2d;
    using namespace edgar::generator::common;
    std::vector<PolygonGrid2D> polys = {PolygonGrid2D::get_rectangle(2, 2), PolygonGrid2D::get_rectangle(2, 2)};
    std::vector<Vector2Int> pos = {{0, 0}, {4, 0}};
    const EnergyData e = ConstraintsEvaluatorGrid2D::evaluate(polys, pos);
    EXPECT_DOUBLE_EQ(BasicEnergyUpdater::total_penalty(e), 0.0);
}

TEST(EdgarEnergy, Incident_to_room_sumMatchesTwiceTotal) {
    using namespace edgar::geometry;
    using namespace edgar::generator::grid2d;
    using namespace edgar::generator::common;
    std::vector<PolygonGrid2D> polys = {PolygonGrid2D::get_rectangle(2, 2), PolygonGrid2D::get_rectangle(2, 2),
                                      PolygonGrid2D::get_rectangle(2, 2)};
    std::vector<Vector2Int> pos = {{0, 0}, {1, 0}, {5, 0}};
    std::vector<bool> is_corridor = {false, true, false};
    const EnergyData full = ConstraintsEvaluatorGrid2D::evaluate(polys, pos, 0, &is_corridor);
    const double total = BasicEnergyUpdater::total_penalty(full);
    double sum_inc = 0.0;
    for (std::size_t r = 0; r < polys.size(); ++r) {
        sum_inc += BasicEnergyUpdater::total_penalty(
            ConstraintsEvaluatorGrid2D::incident_to_room(r, polys, pos, 0, &is_corridor));
    }
    EXPECT_NEAR(sum_inc, 2.0 * total, 1e-9);
}

TEST(EdgarGenerator, FourRoomCycle_stripBackend) {
    using namespace edgar;
    using namespace edgar::generator::grid2d;

    auto square = RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_square(8),
                                     std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    auto rectangle = RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_rectangle(6, 10),
                                        std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {square, rectangle});

    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    level.add_room(1, room_desc);
    level.add_room(2, room_desc);
    level.add_room(3, room_desc);
    level.add_connection(0, 1);
    level.add_connection(0, 3);
    level.add_connection(1, 2);
    level.add_connection(2, 3);

    GraphBasedGeneratorConfiguration cfg;
    cfg.backend = GraphBasedGeneratorBackend::strip_packing;
    GraphBasedGeneratorGrid2D<int> generator(level, cfg);
    std::mt19937 rng(12345);
    generator.inject_random_generator(std::move(rng));
    const auto layout = generator.generate_layout();

    ASSERT_EQ(layout.rooms.size(), 4u);
    for (std::size_t i = 0; i < layout.rooms.size(); ++i) {
        for (std::size_t j = i + 1; j < layout.rooms.size(); ++j) {
            EXPECT_FALSE(edgar::geometry::polygons_overlap_area(layout.rooms[i].outline, layout.rooms[i].position,
                                                               layout.rooms[j].outline, layout.rooms[j].position));
        }
    }
}

TEST(EdgarIo, LoadImageRgba_missingFile) {
    EXPECT_FALSE(edgar::io::load_image_rgba("nonexistent_path_that_should_not_exist.png").has_value());
}

TEST(EdgarGenerator, FourRoomCycle) {
    using namespace edgar;
    using namespace edgar::generator::grid2d;

    auto square = RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_square(8),
                                     std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    auto rectangle = RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_rectangle(6, 10),
                                        std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {square, rectangle});

    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    level.add_room(1, room_desc);
    level.add_room(2, room_desc);
    level.add_room(3, room_desc);
    level.add_connection(0, 1);
    level.add_connection(0, 3);
    level.add_connection(1, 2);
    level.add_connection(2, 3);

    GraphBasedGeneratorGrid2D<int> generator(level);
    std::mt19937 rng(12345);
    generator.inject_random_generator(std::move(rng));
    const auto layout = generator.generate_layout();

    ASSERT_EQ(layout.rooms.size(), 4u);
    for (std::size_t i = 0; i < layout.rooms.size(); ++i) {
        for (std::size_t j = i + 1; j < layout.rooms.size(); ++j) {
            EXPECT_FALSE(edgar::geometry::polygons_overlap_area(layout.rooms[i].outline, layout.rooms[i].position,
                                                               layout.rooms[j].outline, layout.rooms[j].position));
        }
    }
}

TEST(EdgarGeometry, PolygonsOverlap_edgeTouchingNoInteriorOverlap) {
    using namespace edgar::geometry;
    const auto a = PolygonGrid2D::get_rectangle(2, 2);
    const auto b = PolygonGrid2D::get_rectangle(2, 2);
    EXPECT_FALSE(polygons_overlap_area(a, {0, 0}, b, {2, 0}));
}

TEST(EdgarGeometry, PolygonsOverlap_separatedRectsNoOverlap) {
    using namespace edgar::geometry;
    const auto a = PolygonGrid2D::get_rectangle(2, 2);
    const auto b = PolygonGrid2D::get_rectangle(2, 2);
    EXPECT_FALSE(polygons_overlap_area(a, {0, 0}, b, {4, 0}));
}

TEST(EdgarGeometry, OrthogonalLineShrink_horizontal) {
    using namespace edgar::geometry;
    const OrthogonalLineGrid2D line({0, 0}, {5, 0});
    const OrthogonalLineGrid2D s = line.shrink(1, 2);
    EXPECT_EQ(s.from.x, 1);
    EXPECT_EQ(s.to.x, 3);
    EXPECT_EQ(s.from.y, 0);
    EXPECT_EQ(s.to.y, 0);
}

TEST(EdgarDoors, SimpleDoorModeSquareEight) {
    using namespace edgar::geometry;
    using namespace edgar::generator::grid2d;
    SimpleDoorModeGrid2D mode(1, 1);
    const auto poly = PolygonGrid2D::get_square(8);
    const auto doors = mode.get_doors(poly);
    EXPECT_EQ(doors.size(), 4u);
    for (const auto& d : doors) {
        EXPECT_EQ(d.length, 1);
        EXPECT_GE(d.line.length(), 1);
    }
}

TEST(EdgarGenerator, Chain_threeRoomsWithCorridor_lineGraph) {
    using namespace edgar;
    using namespace edgar::generator::grid2d;
    using namespace edgar::generator::common;

    auto square = RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_square(8),
                                     std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    auto corridor_rect =
        RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_rectangle(8, 2),
                             std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {square});
    RoomDescriptionGrid2D corridor_desc(true, {corridor_rect});

    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    level.add_room(1, corridor_desc);
    level.add_room(2, room_desc);
    level.add_connection(0, 1);
    level.add_connection(1, 2);

    SimulatedAnnealingConfiguration sa_config;
    sa_config.cycles = 8;
    sa_config.trials_per_cycle = 40;
    sa_config.max_stage_two_failures = 4;

    std::mt19937 rng(42);
    const auto result = ChainBasedGeneratorGrid2D<int>::generate(level, sa_config, rng);

    ASSERT_EQ(result.layout.rooms.size(), 3u);
    for (std::size_t i = 0; i < result.layout.rooms.size(); ++i) {
        for (std::size_t j = i + 1; j < result.layout.rooms.size(); ++j) {
            EXPECT_FALSE(edgar::geometry::polygons_overlap_area(result.layout.rooms[i].outline,
                                                                result.layout.rooms[i].position,
                                                                result.layout.rooms[j].outline,
                                                                result.layout.rooms[j].position));
        }
    }
}

TEST(EdgarGenerator, Chain_yieldStream_matchesSingleAndCountsEvents) {
    using namespace edgar;
    using namespace edgar::generator::grid2d;
    using namespace edgar::generator::common;

    auto square = RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_square(8),
                                     std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    auto corridor_rect =
        RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_rectangle(8, 2),
                             std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {square});
    RoomDescriptionGrid2D corridor_desc(true, {corridor_rect});

    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    level.add_room(1, corridor_desc);
    level.add_room(2, room_desc);
    level.add_connection(0, 1);
    level.add_connection(1, 2);

    SimulatedAnnealingConfiguration sa_config;
    sa_config.cycles = 8;
    sa_config.trials_per_cycle = 40;
    sa_config.max_stage_two_failures = 4;

    std::mt19937 rng_single(42);
    const auto baseline = ChainBasedGeneratorGrid2D<int>::generate(level, sa_config, rng_single);

    LayoutOrchestrationStats stats{};
    ChainGenerateContext<int> ctx;
    ctx.layout_stream = LayoutStreamMode::OnEachLayoutGenerated;
    ctx.max_layout_yields = 100;
    int layout_generated_events = 0;
    ctx.on_layout = [&](const LayoutYieldInfo& info, const LayoutGrid2D<int>& lay) {
        (void)lay;
        if (info.event_type == LayoutYieldEvent::LayoutGenerated) {
            ++layout_generated_events;
        }
    };
    ctx.stats_out = &stats;

    std::mt19937 rng_stream(42);
    const auto streamed = ChainBasedGeneratorGrid2D<int>::generate(
        level, sa_config, rng_stream, ChainDecompositionStrategy::breadth_first_old, {}, &ctx);

    EXPECT_EQ(baseline.iterations, streamed.iterations);
    EXPECT_EQ(baseline.layout.rooms.size(), streamed.layout.rooms.size());
    EXPECT_GE(layout_generated_events, 1);
    EXPECT_EQ(stats.layouts_generated, layout_generated_events);
}

TEST(EdgarGenerator, Golden_chainLayoutJson_nonEmpty) {
    using namespace edgar;
    using namespace edgar::generator::grid2d;
    using namespace edgar::generator::common;

    auto square = RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_square(8),
                                     std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    auto corridor_rect =
        RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_rectangle(8, 2),
                             std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {square});
    RoomDescriptionGrid2D corridor_desc(true, {corridor_rect});

    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    level.add_room(1, corridor_desc);
    level.add_room(2, room_desc);
    level.add_connection(0, 1);
    level.add_connection(1, 2);

    SimulatedAnnealingConfiguration sa_config;
    sa_config.cycles = 6;
    sa_config.trials_per_cycle = 30;
    sa_config.max_stage_two_failures = 4;

    std::mt19937 rng(12345);
    const auto result = ChainBasedGeneratorGrid2D<int>::generate(level, sa_config, rng);

    const auto j = edgar::io::layout_to_json(result.layout);
    ASSERT_TRUE(j.contains("rooms"));
    EXPECT_EQ(j["rooms"].size(), result.layout.rooms.size());
    EXPECT_FALSE(j["rooms"].empty());
}

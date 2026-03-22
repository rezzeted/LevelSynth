#include <gtest/gtest.h>

#include <random>
#include <unordered_set>

#include "edgar/chain_decompositions/breadth_first_chain_decomposition_old.hpp"
#include "edgar/edgar.hpp"
#include "edgar/generator/common/basic_energy_updater.hpp"
#include "edgar/generator/grid2d/configuration_spaces_grid2d.hpp"
#include "edgar/generator/grid2d/constraints_evaluator_grid2d.hpp"
#include "edgar/generator/grid2d/graph_based_generator_configuration.hpp"
#include "edgar/io/png_rgba.hpp"
#include "edgar/graphs/undirected_graph.hpp"
#include "edgar/geometry/orthogonal_line_grid2d.hpp"
#include "edgar/geometry/overlap.hpp"
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

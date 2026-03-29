#include <gtest/gtest.h>

#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"
#include "edgar/geometry/transformation_grid2d.hpp"
#include "edgar/geometry/rectangle_grid2d.hpp"
#include "edgar/geometry/orthogonal_line_grid2d.hpp"
#include "edgar/geometry/orthogonal_line_intersection.hpp"
#include "edgar/geometry/overlap.hpp"
#include "edgar/geometry/polygon_overlap_grid2d.hpp"
#include "edgar/geometry/clipper2_util.hpp"
#include "edgar/graphs/undirected_graph.hpp"
#include "edgar/graphs/graph_algorithms.hpp"
#include "edgar/generator/grid2d/level_description_grid2d.hpp"
#include "edgar/generator/grid2d/room_template_grid2d.hpp"
#include "edgar/generator/grid2d/simple_door_mode_grid2d.hpp"
#include "edgar/generator/grid2d/door_utils.hpp"
#include "edgar/generator/grid2d/configuration_spaces_generator.hpp"
#include "edgar/generator/grid2d/configuration_spaces_grid2d.hpp"

#include <set>
#include <stdexcept>
#include <vector>

using namespace edgar;
using namespace edgar::geometry;
using namespace edgar::graphs;
using namespace edgar::generator::grid2d;

TEST(EdgarGeometry, PolygonGetSquare_CorrectPointsAndWidth) {
    auto sq = PolygonGrid2D::get_square(8);
    EXPECT_EQ(sq.points().size(), 4u);
    auto br = sq.bounding_rectangle();
    EXPECT_EQ(br.width(), 8);
    EXPECT_EQ(br.height(), 8);
}

TEST(EdgarGeometry, PolygonGetRectangle_CorrectPointsAndWidth) {
    auto rect = PolygonGrid2D::get_rectangle(6, 10);
    EXPECT_EQ(rect.points().size(), 4u);
    auto br = rect.bounding_rectangle();
    EXPECT_EQ(br.width(), 6);
    EXPECT_EQ(br.height(), 10);
}

TEST(EdgarGeometry, PolygonBoundingRectangle_Rectangle) {
    auto rect = PolygonGrid2D::get_rectangle(2, 4);
    auto br = rect.bounding_rectangle();
    EXPECT_EQ(br.a, Vector2Int(0, 0));
    EXPECT_EQ(br.b, Vector2Int(2, 4));
}

TEST(EdgarGeometry, PolygonBoundingRectangle_LShape) {
    auto lshape = PolygonGrid2DBuilder()
        .add_point(0, 0)
        .add_point(0, 6)
        .add_point(3, 6)
        .add_point(3, 3)
        .add_point(7, 3)
        .add_point(7, 0)
        .build();
    auto br = lshape.bounding_rectangle();
    EXPECT_EQ(br.a, Vector2Int(0, 0));
    EXPECT_EQ(br.b, Vector2Int(7, 6));
}

TEST(EdgarGeometry, PolygonRotate_Square180) {
    auto sq = PolygonGrid2D::get_square(4);
    auto rotated = sq.rotate(180);
    std::vector<Vector2Int> expected = {{0, 0}, {0, -4}, {-4, -4}, {-4, 0}};
    ASSERT_EQ(rotated.points().size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(rotated.points()[i], expected[i]);
    }
}

TEST(EdgarGeometry, PolygonRotate_Rectangle270) {
    auto rect = PolygonGrid2D::get_rectangle(2, 5);
    auto rotated = rect.rotate(270);
    std::vector<Vector2Int> expected = {{0, 0}, {-5, 0}, {-5, 2}, {0, 2}};
    ASSERT_EQ(rotated.points().size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(rotated.points()[i], expected[i]);
    }
}

TEST(EdgarGeometry, PolygonTransform_AllEight) {
    auto poly = PolygonGrid2D::get_rectangle(2, 1);
    auto transforms = poly.get_all_transformations();
    ASSERT_EQ(transforms.size(), 8u);
    EXPECT_EQ(transforms[0].points()[0], Vector2Int(0, 0));
    EXPECT_EQ(transforms[0].points()[1], Vector2Int(0, 1));
    EXPECT_EQ(transforms[0].points()[2], Vector2Int(2, 1));
    EXPECT_EQ(transforms[0].points()[3], Vector2Int(2, 0));
    EXPECT_EQ(transforms[1].points()[0], Vector2Int(0, 0));
    EXPECT_EQ(transforms[1].points()[1], Vector2Int(1, 0));
    EXPECT_EQ(transforms[1].points()[2], Vector2Int(1, -2));
    EXPECT_EQ(transforms[1].points()[3], Vector2Int(0, -2));
}

TEST(EdgarGeometry, PolygonPlus_Translate) {
    auto sq = PolygonGrid2D::get_square(3);
    auto offset = Vector2Int(2, 3);
    auto translated = sq + offset;
    for (std::size_t i = 0; i < sq.points().size(); ++i) {
        EXPECT_EQ(translated.points()[i], sq.points()[i] + offset);
    }
}

TEST(EdgarGeometry, PolygonScale_ReturnsScaled) {
    auto sq = PolygonGrid2D::get_square(3);
    auto scaled = sq.scale(Vector2Int(2, 3));
    auto expected = PolygonGrid2D::get_rectangle(6, 9);
    EXPECT_EQ(scaled, expected);
}

TEST(EdgarGeometry, PolygonConstructor_TooFewPoints) {
    EXPECT_THROW(PolygonGrid2DBuilder().add_point(0, 0).add_point(0, 5).build(), std::invalid_argument);
    EXPECT_THROW(PolygonGrid2DBuilder().add_point(0, 0).add_point(0, 5).add_point(5, 5).build(), std::invalid_argument);
}

TEST(EdgarGeometry, PolygonConstructor_ValidPolygons) {
    EXPECT_NO_THROW(PolygonGrid2DBuilder().add_point(0, 0).add_point(0, 3).add_point(3, 3).add_point(3, 0).build());
    EXPECT_NO_THROW(PolygonGrid2DBuilder()
        .add_point(0, 0).add_point(0, 6).add_point(3, 6).add_point(3, 3).add_point(6, 3).add_point(6, 0).build());
}

TEST(EdgarGeometry, PolygonConstructor_CounterClockwise) {
    EXPECT_THROW(PolygonGrid2DBuilder().add_point(3, 0).add_point(3, 3).add_point(0, 3).add_point(0, 0).build(),
                 std::invalid_argument);
    EXPECT_THROW(PolygonGrid2DBuilder()
        .add_point(6, 0).add_point(6, 3).add_point(3, 3).add_point(3, 6).add_point(0, 6).add_point(0, 0).build(),
                 std::invalid_argument);
}

TEST(EdgarGeometry, PolygonConstructor_EdgesNotOrthogonal) {
    EXPECT_THROW(PolygonGrid2DBuilder().add_point(0, 0).add_point(0, 5).add_point(3, 4).add_point(4, 0).build(),
                 std::invalid_argument);
}

TEST(EdgarGeometry, PolygonEquals_SameOrder_ReturnTrue) {
    auto a = PolygonGrid2DBuilder().add_point(0, 0).add_point(0, 3).add_point(3, 3).add_point(3, 0).build();
    auto b = PolygonGrid2DBuilder().add_point(0, 0).add_point(0, 3).add_point(3, 3).add_point(3, 0).build();
    EXPECT_TRUE(a == b);
}

TEST(EdgarGeometry, PolygonEquals_DifferentOrder_ReturnFalse) {
    auto a = PolygonGrid2DBuilder().add_point(0, 0).add_point(0, 3).add_point(3, 3).add_point(3, 0).build();
    auto b = PolygonGrid2DBuilder().add_point(3, 0).add_point(0, 0).add_point(0, 3).add_point(3, 3).build();
    EXPECT_FALSE(a == b);
}

TEST(EdgarGeometry, Overlap_IdenticalSquares_ReturnsTrue) {
    auto sq = PolygonGrid2D::get_square(2);
    EXPECT_TRUE(polygons_overlap_area(sq, Vector2Int(0, 0), sq, Vector2Int(0, 0)));
}

TEST(EdgarGeometry, Overlap_OverlappingSquares_ReturnsTrue) {
    auto sq = PolygonGrid2D::get_square(2);
    EXPECT_TRUE(polygons_overlap_area(sq, Vector2Int(0, 0), sq, Vector2Int(1, 0)));
}

TEST(EdgarGeometry, Overlap_NestedRectangles_ReturnsTrue) {
    auto outer = PolygonGrid2D::get_rectangle(4, 5);
    auto inner = PolygonGrid2D::get_rectangle(2, 3);
    EXPECT_TRUE(polygons_overlap_area(outer, Vector2Int(0, 0), inner, Vector2Int(1, 1)));
}

TEST(EdgarGeometry, Overlap_NonOverlappingSquares_ReturnsFalse) {
    auto sq = PolygonGrid2D::get_square(2);
    EXPECT_FALSE(polygons_overlap_area(sq, Vector2Int(0, 0), sq, Vector2Int(6, 0)));
}

TEST(EdgarGeometry, Overlap_TouchingSquaresSide_ReturnsFalse) {
    auto sq = PolygonGrid2D::get_square(2);
    EXPECT_FALSE(polygons_overlap_area(sq, Vector2Int(0, 0), sq, Vector2Int(0, 2)));
}

TEST(EdgarGeometry, Overlap_TouchingRectanglesCorner_ReturnsFalse) {
    auto r1 = PolygonGrid2D::get_rectangle(6, 3);
    auto r2 = PolygonGrid2D::get_rectangle(4, 3);
    EXPECT_FALSE(polygons_overlap_area(r1, Vector2Int(0, 0), r2, Vector2Int(6, 3)));
}

TEST(EdgarGeometry, Overlap_LShapeVsSquare_Overlapping_ReturnsTrue) {
    auto p1 = PolygonGrid2DBuilder()
        .add_point(0, 0).add_point(0, 6).add_point(3, 6)
        .add_point(3, 3).add_point(6, 3).add_point(6, 0).build();
    auto p2 = PolygonGrid2D::get_square(3);
    EXPECT_TRUE(polygons_overlap_area(p1, Vector2Int(0, 0), p2, Vector2Int(3, 0)));
}

TEST(EdgarGeometry, Overlap_RotatedLShapeVsSquare_NonOverlapping_ReturnsFalse) {
    auto p1 = PolygonGrid2DBuilder()
        .add_point(0, 0).add_point(0, 6).add_point(3, 6)
        .add_point(3, 3).add_point(6, 3).add_point(6, 0).build();
    auto p1_rotated = p1.rotate(90);
    auto p2 = PolygonGrid2D::get_square(3);
    EXPECT_FALSE(polygons_overlap_area(p1_rotated, Vector2Int(0, 0), p2, Vector2Int(0, 0)));
}

TEST(EdgarGeometry, OverlapAlongLine_OverlapStart) {
    auto p1 = PolygonGrid2D::get_square(5);
    auto p2 = PolygonGrid2D::get_rectangle(2, 3);
    auto line = OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(0, 10));
    auto result = overlap_along_line(p1, p2, line);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].first, Vector2Int(0, 0));
    EXPECT_TRUE(result[0].second);
    EXPECT_EQ(result[1].first, Vector2Int(0, 3));
    EXPECT_FALSE(result[1].second);
}

TEST(EdgarGraphs, AddVertexDuplicate_Throws) {
    UndirectedAdjacencyListGraph<int> g;
    g.add_vertex(0);
    EXPECT_THROW(g.add_vertex(0), std::invalid_argument);
}

TEST(EdgarGraphs, AddEdgeDuplicate_Throws) {
    UndirectedAdjacencyListGraph<int> g;
    g.add_vertex(0);
    g.add_vertex(1);
    g.add_edge(0, 1);
    EXPECT_THROW(g.add_edge(0, 1), std::invalid_argument);
}

TEST(EdgarGraphs, AddEdgeNonExistingVertex_Throws) {
    UndirectedAdjacencyListGraph<int> g;
    g.add_vertex(0);
    EXPECT_THROW(g.add_edge(0, 1), std::invalid_argument);
}

TEST(EdgarGraphs, HasEdge_Correct) {
    UndirectedAdjacencyListGraph<int> g;
    g.add_vertex(0);
    g.add_vertex(1);
    g.add_vertex(2);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    EXPECT_TRUE(g.has_edge(0, 1));
    EXPECT_TRUE(g.has_edge(1, 0));
    EXPECT_TRUE(g.has_edge(1, 2));
    EXPECT_FALSE(g.has_edge(0, 2));
}

TEST(EdgarGraphs, VertexAndEdgeCount) {
    UndirectedAdjacencyListGraph<int> g;
    for (int i = 0; i < 5; ++i) g.add_vertex(i);
    for (int i = 0; i < 4; ++i) g.add_edge(i, i + 1);
    EXPECT_EQ(g.vertex_count(), 5u);
}

TEST(EdgarGraphs, IsConnected_C3) {
    UndirectedAdjacencyListGraph<int> g;
    for (int i = 0; i < 3; ++i) g.add_vertex(i);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 0);
    EXPECT_TRUE(is_connected(g));
}

TEST(EdgarGraphs, IsConnected_TwoC3sByEdge) {
    UndirectedAdjacencyListGraph<int> g;
    for (int i = 0; i < 6; ++i) g.add_vertex(i);
    g.add_edge(0, 1); g.add_edge(1, 2); g.add_edge(2, 0);
    g.add_edge(3, 4); g.add_edge(4, 5); g.add_edge(5, 3);
    g.add_edge(0, 3);
    EXPECT_TRUE(is_connected(g));
}

TEST(EdgarGraphs, IsConnected_NotConnected_TwoC3s) {
    UndirectedAdjacencyListGraph<int> g;
    for (int i = 0; i < 6; ++i) g.add_vertex(i);
    g.add_edge(0, 1); g.add_edge(1, 2); g.add_edge(2, 0);
    g.add_edge(3, 4); g.add_edge(4, 5); g.add_edge(5, 3);
    EXPECT_FALSE(is_connected(g));
}

TEST(EdgarGraphs, IsConnected_NoEdges) {
    UndirectedAdjacencyListGraph<int> g;
    for (int i = 0; i < 20; ++i) g.add_vertex(i);
    EXPECT_FALSE(is_connected(g));
}

TEST(EdgarGraphs, IsTree_Linear) {
    UndirectedAdjacencyListGraph<int> g;
    for (int i = 0; i < 4; ++i) g.add_vertex(i);
    g.add_edge(0, 1); g.add_edge(1, 2); g.add_edge(2, 3);
    EXPECT_TRUE(is_tree(g));
}

TEST(EdgarGraphs, IsTree_Cycle_NotTree) {
    UndirectedAdjacencyListGraph<int> g;
    for (int i = 0; i < 3; ++i) g.add_vertex(i);
    g.add_edge(0, 1); g.add_edge(1, 2); g.add_edge(2, 0);
    EXPECT_FALSE(is_tree(g));
}

TEST(EdgarGraphs, IsTree_Disconnected_NotTree) {
    UndirectedAdjacencyListGraph<int> g;
    for (int i = 0; i < 4; ++i) g.add_vertex(i);
    g.add_edge(0, 1); g.add_edge(2, 3);
    EXPECT_FALSE(is_tree(g));
}

TEST(EdgarGeometry, LineIntersection_NoIntersection_DifferentY) {
    OrthogonalLineGrid2D line1(Vector2Int(1, 1), Vector2Int(5, 1));
    OrthogonalLineGrid2D line2(Vector2Int(1, 2), Vector2Int(5, 2));
    OrthogonalLineGrid2D result;
    EXPECT_FALSE(OrthogonalLineIntersection::try_get_intersection(line1, line2, result));
}

TEST(EdgarGeometry, LineIntersection_PointIntersection) {
    OrthogonalLineGrid2D line1(Vector2Int(1, 1), Vector2Int(5, 1));
    OrthogonalLineGrid2D line2(Vector2Int(5, 1), Vector2Int(10, 1));
    OrthogonalLineGrid2D result;
    EXPECT_TRUE(OrthogonalLineIntersection::try_get_intersection(line1, line2, result));
    auto rn = result.normalized();
    EXPECT_EQ(rn.from, Vector2Int(5, 1));
    EXPECT_EQ(rn.to, Vector2Int(5, 1));
}

TEST(EdgarGeometry, LineIntersection_LineIntersection) {
    OrthogonalLineGrid2D line1(Vector2Int(3, 2), Vector2Int(10, 2));
    OrthogonalLineGrid2D line2(Vector2Int(7, 2), Vector2Int(13, 2));
    OrthogonalLineGrid2D result;
    EXPECT_TRUE(OrthogonalLineIntersection::try_get_intersection(line1, line2, result));
    auto rn = result.normalized();
    EXPECT_EQ(rn.from, Vector2Int(7, 2));
    EXPECT_EQ(rn.to, Vector2Int(10, 2));
}

TEST(EdgarGeometry, LineIntersection_PerpendicularPoint) {
    OrthogonalLineGrid2D line1(Vector2Int(1, 1), Vector2Int(5, 1));
    OrthogonalLineGrid2D line2(Vector2Int(3, -2), Vector2Int(3, 5));
    OrthogonalLineGrid2D result;
    EXPECT_TRUE(OrthogonalLineIntersection::try_get_intersection(line1, line2, result));
    EXPECT_EQ(result.from, Vector2Int(3, 1));
    EXPECT_EQ(result.to, Vector2Int(3, 1));
}

TEST(EdgarGeometry, PartitionByIntersection_OnePoint) {
    OrthogonalLineGrid2D line(Vector2Int(0, 0), Vector2Int(10, 0));
    std::vector<OrthogonalLineGrid2D> intersection = {OrthogonalLineGrid2D(Vector2Int(5, 0), Vector2Int(5, 0))};
    auto partitions = OrthogonalLineIntersection::partition_by_intersection(line, intersection);
    ASSERT_EQ(partitions.size(), 2u);
    EXPECT_EQ(partitions[0].from, Vector2Int(0, 0));
    EXPECT_EQ(partitions[0].to, Vector2Int(4, 0));
    EXPECT_EQ(partitions[1].from, Vector2Int(6, 0));
    EXPECT_EQ(partitions[1].to, Vector2Int(10, 0));
}

TEST(EdgarGeometry, PartitionByIntersection_LineSegment) {
    OrthogonalLineGrid2D line(Vector2Int(0, 0), Vector2Int(10, 0));
    std::vector<OrthogonalLineGrid2D> intersection = {OrthogonalLineGrid2D(Vector2Int(4, 0), Vector2Int(6, 0))};
    auto partitions = OrthogonalLineIntersection::partition_by_intersection(line, intersection);
    ASSERT_EQ(partitions.size(), 2u);
    EXPECT_EQ(partitions[0].from, Vector2Int(0, 0));
    EXPECT_EQ(partitions[0].to, Vector2Int(3, 0));
    EXPECT_EQ(partitions[1].from, Vector2Int(7, 0));
    EXPECT_EQ(partitions[1].to, Vector2Int(10, 0));
}

TEST(EdgarGeometry, RemoveIntersections_OneLine) {
    std::vector<OrthogonalLineGrid2D> lines = {OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(10, 0))};
    auto result = OrthogonalLineIntersection::remove_intersections(lines);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].from, lines[0].from);
    EXPECT_EQ(result[0].to, lines[0].to);
}

TEST(EdgarGeometry, RemoveIntersections_MultipleLines) {
    std::vector<OrthogonalLineGrid2D> lines = {
        OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(10, 0)),
        OrthogonalLineGrid2D(Vector2Int(0, 5), Vector2Int(10, 5)),
        OrthogonalLineGrid2D(Vector2Int(2, -5), Vector2Int(2, 10)),
        OrthogonalLineGrid2D(Vector2Int(7, -5), Vector2Int(7, 10)),
    };
    auto result = OrthogonalLineIntersection::remove_intersections(lines);
    std::set<Vector2Int> result_pts;
    for (const auto& l : result) {
        auto pts = l.grid_points_inclusive();
        result_pts.insert(pts.begin(), pts.end());
    }
    std::set<Vector2Int> expected_pts;
    for (const auto& l : lines) {
        auto pts = l.grid_points_inclusive();
        expected_pts.insert(pts.begin(), pts.end());
    }
    EXPECT_EQ(result_pts, expected_pts);
}

TEST(EdgarLevelDescription, AddRoomDuplicate_Throws) {
    auto sq = RoomTemplateGrid2D(PolygonGrid2D::get_square(8),
                                 std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {sq});
    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    EXPECT_THROW(level.add_room(0, room_desc), std::invalid_argument);
}

TEST(EdgarLevelDescription, AddConnectionDuplicate_Throws) {
    auto sq = RoomTemplateGrid2D(PolygonGrid2D::get_square(8),
                                 std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {sq});
    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    level.add_room(1, room_desc);
    level.add_connection(0, 1);
    EXPECT_THROW(level.add_connection(0, 1), std::invalid_argument);
}

TEST(EdgarLevelDescription, AddConnectionNonExistingRoom_Throws) {
    auto sq = RoomTemplateGrid2D(PolygonGrid2D::get_square(8),
                                 std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {sq});
    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    EXPECT_THROW(level.add_connection(0, 1), std::invalid_argument);
}

TEST(EdgarLevelDescription, CorridorWithTooManyNeighbors_Throws) {
    auto corridor_tmpl = RoomTemplateGrid2D(PolygonGrid2D::get_square(3),
                                            std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    auto sq_tmpl = RoomTemplateGrid2D(PolygonGrid2D::get_square(8),
                                      std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    LevelDescriptionGrid2D<int> level;
    level.add_room(0, RoomDescriptionGrid2D(true, {corridor_tmpl}));
    level.add_room(1, RoomDescriptionGrid2D(false, {sq_tmpl}));
    level.add_room(2, RoomDescriptionGrid2D(false, {sq_tmpl}));
    level.add_room(3, RoomDescriptionGrid2D(false, {sq_tmpl}));
    level.add_connection(0, 1);
    level.add_connection(0, 2);
    level.add_connection(0, 3);
    EXPECT_THROW(level.get_graph(), std::invalid_argument);
}

TEST(EdgarLevelDescription, TwoNeighboringCorridors_Throws) {
    auto corridor_tmpl = RoomTemplateGrid2D(PolygonGrid2D::get_square(3),
                                            std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    LevelDescriptionGrid2D<int> level;
    level.add_room(0, RoomDescriptionGrid2D(true, {corridor_tmpl}));
    level.add_room(1, RoomDescriptionGrid2D(true, {corridor_tmpl}));
    level.add_connection(0, 1);
    EXPECT_THROW(level.get_graph(), std::invalid_argument);
}

TEST(EdgarLevelDescription, OnlyBasicRooms_GraphBuilt) {
    auto sq = RoomTemplateGrid2D(PolygonGrid2D::get_square(8),
                                 std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {sq});
    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    level.add_room(1, room_desc);
    level.add_room(2, room_desc);
    level.add_room(3, room_desc);
    level.add_connection(0, 1);
    level.add_connection(1, 2);
    level.add_connection(2, 3);
    auto g = level.get_graph();
    EXPECT_EQ(g.vertex_count(), 4u);
}

// ===========================================================================
// Этап F: Door tests (SimpleDoorMode, MergeDoorLines)
// ===========================================================================

static std::vector<DoorLineGrid2D> sorted_by_from(const std::vector<DoorLineGrid2D>& doors) {
    auto copy = doors;
    std::sort(copy.begin(), copy.end(), [](const DoorLineGrid2D& a, const DoorLineGrid2D& b) {
        if (a.line.from != b.line.from) return a.line.from < b.line.from;
        return a.line.to < b.line.to;
    });
    return copy;
}

TEST(EdgarDoors, SimpleDoorMode_Length1_NoOverlap) {
    auto rect = PolygonGrid2D::get_rectangle(3, 5);
    SimpleDoorModeGrid2D mode(1, 0);
    auto doors = mode.get_doors(rect);

    ASSERT_EQ(doors.size(), 4u);
    for (const auto& d : doors) {
        EXPECT_EQ(d.length, 1);
    }

    auto s = sorted_by_from(doors);
    EXPECT_EQ(s[0].line.from, Vector2Int(0, 0));
    EXPECT_EQ(s[0].line.to, Vector2Int(0, 4));
    EXPECT_EQ(s[1].line.from, Vector2Int(0, 5));
    EXPECT_EQ(s[1].line.to, Vector2Int(2, 5));
    EXPECT_EQ(s[2].line.from, Vector2Int(3, 0));
    EXPECT_EQ(s[2].line.to, Vector2Int(1, 0));
    EXPECT_EQ(s[3].line.from, Vector2Int(3, 5));
    EXPECT_EQ(s[3].line.to, Vector2Int(3, 1));
}

TEST(EdgarDoors, SimpleDoorMode_Length1_OneOverlap) {
    auto rect = PolygonGrid2D::get_rectangle(3, 5);
    SimpleDoorModeGrid2D mode(1, 1);
    auto doors = mode.get_doors(rect);

    ASSERT_EQ(doors.size(), 4u);

    auto s = sorted_by_from(doors);
    EXPECT_EQ(s[0].line.from, Vector2Int(0, 1));
    EXPECT_EQ(s[0].line.to, Vector2Int(0, 3));
    EXPECT_EQ(s[1].line.from, Vector2Int(1, 5));
    EXPECT_EQ(s[1].line.to, Vector2Int(1, 5));
    EXPECT_EQ(s[2].line.from, Vector2Int(2, 0));
    EXPECT_EQ(s[2].line.to, Vector2Int(2, 0));
    EXPECT_EQ(s[3].line.from, Vector2Int(3, 4));
    EXPECT_EQ(s[3].line.to, Vector2Int(3, 2));
}

TEST(EdgarDoors, SimpleDoorMode_Length1_TwoOverlap) {
    auto rect = PolygonGrid2D::get_rectangle(3, 5);
    SimpleDoorModeGrid2D mode(1, 2);
    auto doors = mode.get_doors(rect);

    ASSERT_EQ(doors.size(), 2u);
    auto s = sorted_by_from(doors);
    EXPECT_EQ(s[0].line.from, Vector2Int(0, 2));
    EXPECT_EQ(s[0].line.to, Vector2Int(0, 2));
    EXPECT_EQ(s[1].line.from, Vector2Int(3, 3));
    EXPECT_EQ(s[1].line.to, Vector2Int(3, 3));
}

TEST(EdgarDoors, SimpleDoorMode_Length2_NoOverlap) {
    auto rect = PolygonGrid2D::get_rectangle(3, 5);
    SimpleDoorModeGrid2D mode(2, 0);
    auto doors = mode.get_doors(rect);

    ASSERT_EQ(doors.size(), 4u);
    for (const auto& d : doors) {
        EXPECT_EQ(d.length, 2);
    }

    auto s = sorted_by_from(doors);
    EXPECT_EQ(s[0].line.from, Vector2Int(0, 0));
    EXPECT_EQ(s[0].line.to, Vector2Int(0, 3));
    EXPECT_EQ(s[1].line.from, Vector2Int(0, 5));
    EXPECT_EQ(s[1].line.to, Vector2Int(1, 5));
    EXPECT_EQ(s[2].line.from, Vector2Int(3, 0));
    EXPECT_EQ(s[2].line.to, Vector2Int(2, 0));
    EXPECT_EQ(s[3].line.from, Vector2Int(3, 5));
    EXPECT_EQ(s[3].line.to, Vector2Int(3, 2));
}

TEST(EdgarDoors, SimpleDoorMode_InvalidArgs) {
    EXPECT_THROW(SimpleDoorModeGrid2D(0, 0), std::invalid_argument);
    EXPECT_THROW(SimpleDoorModeGrid2D(-1, 0), std::invalid_argument);
    EXPECT_THROW(SimpleDoorModeGrid2D(1, -1), std::invalid_argument);
    EXPECT_NO_THROW(SimpleDoorModeGrid2D(1, 0));
}

TEST(EdgarDoors, MergeDoorLines_CorrectlyMerges) {
    std::vector<DoorLineGrid2D> input = {
        {OrthogonalLineGrid2D(Vector2Int(1, 0), Vector2Int(2, 0)), 2},
        {OrthogonalLineGrid2D(Vector2Int(3, 0), Vector2Int(5, 0)), 2},
        {OrthogonalLineGrid2D(Vector2Int(-3, 0), Vector2Int(0, 0)), 1},
        {OrthogonalLineGrid2D(Vector2Int(-3, 0), Vector2Int(0, 0)), 2},
        {OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(0, 3)), 2},
        {OrthogonalLineGrid2D(Vector2Int(0, -2), Vector2Int(0, -1)), 2},
    };

    auto merged = merge_door_lines(std::move(input));

    std::vector<DoorLineGrid2D> expected = {
        {OrthogonalLineGrid2D(Vector2Int(-3, 0), Vector2Int(0, 0)), 1},
        {OrthogonalLineGrid2D(Vector2Int(-3, 0), Vector2Int(5, 0)), 2},
        {OrthogonalLineGrid2D(Vector2Int(0, -2), Vector2Int(0, 3)), 2},
    };

    auto ms = sorted_by_from(merged);
    auto es = sorted_by_from(expected);
    ASSERT_EQ(ms.size(), es.size());
    for (size_t i = 0; i < ms.size(); ++i) {
        EXPECT_EQ(ms[i].line.from, es[i].line.from) << "at index " << i;
        EXPECT_EQ(ms[i].line.to, es[i].line.to) << "at index " << i;
        EXPECT_EQ(ms[i].length, es[i].length) << "at index " << i;
    }
}

TEST(EdgarDoors, SimpleDoorMode_Square10_Symmetric) {
    auto sq = PolygonGrid2D::get_square(10);
    SimpleDoorModeGrid2D mode(1, 0);
    auto doors = mode.get_doors(sq);
    EXPECT_EQ(doors.size(), 4u);
    for (const auto& d : doors) {
        EXPECT_EQ(d.length, 1);
    }
}

// ===========================================================================
// Этап H: Utility tests (Vector2Int transform, OrthogonalLine)
// ===========================================================================

TEST(EdgarUtils, Vector2Int_Transform_All8) {
    Vector2Int v(2, 3);

    EXPECT_EQ(v.transform(TransformationGrid2D::Identity), Vector2Int(2, 3));
    EXPECT_EQ(v.transform(TransformationGrid2D::Rotate90), Vector2Int(3, -2));
    EXPECT_EQ(v.transform(TransformationGrid2D::Rotate180), Vector2Int(-2, -3));
    EXPECT_EQ(v.transform(TransformationGrid2D::Rotate270), Vector2Int(-3, 2));
    EXPECT_EQ(v.transform(TransformationGrid2D::MirrorX), Vector2Int(2, -3));
    EXPECT_EQ(v.transform(TransformationGrid2D::MirrorY), Vector2Int(-2, 3));
    EXPECT_EQ(v.transform(TransformationGrid2D::Diagonal13), Vector2Int(3, 2));
    EXPECT_EQ(v.transform(TransformationGrid2D::Diagonal24), Vector2Int(-3, -2));
}

TEST(EdgarUtils, OrthogonalLine_Direction) {
    EXPECT_EQ(OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(3, 0)).get_direction(), OrthogonalDirection::Right);
    EXPECT_EQ(OrthogonalLineGrid2D(Vector2Int(3, 0), Vector2Int(0, 0)).get_direction(), OrthogonalDirection::Left);
    EXPECT_EQ(OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(0, 3)).get_direction(), OrthogonalDirection::Top);
    EXPECT_EQ(OrthogonalLineGrid2D(Vector2Int(0, 3), Vector2Int(0, 0)).get_direction(), OrthogonalDirection::Bottom);
    EXPECT_EQ(OrthogonalLineGrid2D(Vector2Int(1, 1), Vector2Int(1, 1)).get_direction(), OrthogonalDirection::Undefined);
}

TEST(EdgarUtils, OrthogonalLine_GridPointsInclusive) {
    auto pts1 = OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(0, 3)).grid_points_inclusive();
    ASSERT_EQ(pts1.size(), 4u);
    EXPECT_EQ(pts1[0], Vector2Int(0, 0));
    EXPECT_EQ(pts1[3], Vector2Int(0, 3));

    auto pts2 = OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(3, 0)).grid_points_inclusive();
    ASSERT_EQ(pts2.size(), 4u);
    EXPECT_EQ(pts2[0], Vector2Int(0, 0));
    EXPECT_EQ(pts2[3], Vector2Int(3, 0));

    auto pts3 = OrthogonalLineGrid2D(Vector2Int(3, 0), Vector2Int(0, 0)).grid_points_inclusive();
    ASSERT_EQ(pts3.size(), 4u);
    EXPECT_EQ(pts3[0], Vector2Int(3, 0));
    EXPECT_EQ(pts3[3], Vector2Int(0, 0));
}

TEST(EdgarUtils, OrthogonalLine_ContainsPoint) {
    OrthogonalLineGrid2D line(Vector2Int(0, 0), Vector2Int(3, 0));
    EXPECT_TRUE(line.contains_point(Vector2Int(0, 0)));
    EXPECT_TRUE(line.contains_point(Vector2Int(1, 0)));
    EXPECT_TRUE(line.contains_point(Vector2Int(3, 0)));
    EXPECT_FALSE(line.contains_point(Vector2Int(4, 0)));
    EXPECT_FALSE(line.contains_point(Vector2Int(0, 1)));
    EXPECT_FALSE(line.contains_point(Vector2Int(-1, 0)));

    OrthogonalLineGrid2D vline(Vector2Int(0, 0), Vector2Int(0, 5));
    EXPECT_TRUE(vline.contains_point(Vector2Int(0, 2)));
    EXPECT_FALSE(vline.contains_point(Vector2Int(1, 2)));
}

TEST(EdgarUtils, OrthogonalLine_Shrink) {
    OrthogonalLineGrid2D line(Vector2Int(0, 0), Vector2Int(5, 0));
    auto s1 = line.shrink(1);
    EXPECT_EQ(s1.from, Vector2Int(1, 0));
    EXPECT_EQ(s1.to, Vector2Int(4, 0));

    auto s2 = line.shrink(1, 2);
    EXPECT_EQ(s2.from, Vector2Int(1, 0));
    EXPECT_EQ(s2.to, Vector2Int(3, 0));

    OrthogonalLineGrid2D vline(Vector2Int(0, 0), Vector2Int(0, 5));
    auto vs = vline.shrink(2);
    EXPECT_EQ(vs.from, Vector2Int(0, 2));
    EXPECT_EQ(vs.to, Vector2Int(0, 3));
}

TEST(EdgarUtils, OrthogonalLine_Normalized) {
    auto n1 = OrthogonalLineGrid2D(Vector2Int(3, 0), Vector2Int(0, 0)).normalized();
    EXPECT_EQ(n1.from, Vector2Int(0, 0));
    EXPECT_EQ(n1.to, Vector2Int(3, 0));

    auto n2 = OrthogonalLineGrid2D(Vector2Int(0, 5), Vector2Int(0, 0)).normalized();
    EXPECT_EQ(n2.from, Vector2Int(0, 0));
    EXPECT_EQ(n2.to, Vector2Int(0, 5));

    auto n3 = OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(3, 0)).normalized();
    EXPECT_EQ(n3.from, Vector2Int(0, 0));
    EXPECT_EQ(n3.to, Vector2Int(3, 0));
}

// ===========================================================================
// Этап E: Configuration space tests
// ===========================================================================

TEST(EdgarConfigSpace, BetweenTwoSquares_NonEmpty) {
    auto sq = PolygonGrid2D::get_square(5);
    SimpleDoorModeGrid2D door_mode(1, 0);
    auto doors = door_mode.get_doors(sq);

    ConfigurationSpacesGenerator gen;
    auto cs = gen.get_configuration_space(sq, doors, sq, doors);

    EXPECT_FALSE(cs.lines.empty());
    EXPECT_FALSE(cs.reverse_doors.empty());
}

TEST(EdgarConfigSpace, CompatibleNonOverlapping) {
    auto sq = PolygonGrid2D::get_square(5);

    EXPECT_TRUE(ConfigurationSpacesGrid2D::compatible_non_overlapping(
        sq, Vector2Int(0, 0), sq, Vector2Int(6, 0)));
    EXPECT_TRUE(ConfigurationSpacesGrid2D::compatible_non_overlapping(
        sq, Vector2Int(0, 0), sq, Vector2Int(0, 6)));
    EXPECT_FALSE(ConfigurationSpacesGrid2D::compatible_non_overlapping(
        sq, Vector2Int(0, 0), sq, Vector2Int(0, 0)));
    EXPECT_FALSE(ConfigurationSpacesGrid2D::compatible_non_overlapping(
        sq, Vector2Int(0, 0), sq, Vector2Int(3, 3)));
}

TEST(EdgarConfigSpace, OffsetOnCS_ValidAndInvalid) {
    auto sq = PolygonGrid2D::get_square(5);
    SimpleDoorModeGrid2D door_mode(1, 0);
    auto doors = door_mode.get_doors(sq);

    auto cs = ConfigurationSpacesGrid2D::configuration_space_between(sq, doors, sq, doors);

    EXPECT_TRUE(offset_on_configuration_space(Vector2Int(5, 0), cs));
    EXPECT_TRUE(offset_on_configuration_space(Vector2Int(-5, 0), cs));
    EXPECT_TRUE(offset_on_configuration_space(Vector2Int(0, 5), cs));
    EXPECT_TRUE(offset_on_configuration_space(Vector2Int(0, -5), cs));
    EXPECT_FALSE(offset_on_configuration_space(Vector2Int(0, 0), cs));
    EXPECT_FALSE(offset_on_configuration_space(Vector2Int(3, 3), cs));
}

TEST(EdgarConfigSpace, EnumerateOffsets_NonEmpty) {
    auto sq = PolygonGrid2D::get_square(5);
    SimpleDoorModeGrid2D door_mode(1, 0);
    auto doors = door_mode.get_doors(sq);

    auto cs = ConfigurationSpacesGrid2D::configuration_space_between(sq, doors, sq, doors);
    auto offsets = enumerate_configuration_space_offsets(cs);

    EXPECT_FALSE(offsets.empty());
    for (const auto& off : offsets) {
        EXPECT_TRUE(offset_on_configuration_space(off, cs));
    }
}

static std::set<Vector2Int> collect_cs_points(const ConfigurationSpaceGrid2D& cs) {
    std::set<Vector2Int> points;
    for (const auto& line : cs.lines) {
        auto pts = line.grid_points_inclusive();
        points.insert(pts.begin(), pts.end());
    }
    return points;
}

TEST(EdgarConfigSpace, OverCorridor_VerticalCorridor) {
    auto room = PolygonGrid2D::get_square(5);
    SimpleDoorModeGrid2D room_door_mode(1, 0);
    auto room_doors = room_door_mode.get_doors(room);

    auto corridor = PolygonGrid2D::get_rectangle(1, 2);
    std::vector<DoorLineGrid2D> corridor_doors = {
        {OrthogonalLineGrid2D(Vector2Int(1, 0), Vector2Int(0, 0)), 1},
        {OrthogonalLineGrid2D(Vector2Int(0, 2), Vector2Int(1, 2)), 1},
    };

    ConfigurationSpacesGenerator gen;
    auto cs = gen.get_configuration_space_over_corridor(
        room, room_doors, room, room_doors, corridor, corridor_doors);

    auto points = collect_cs_points(cs);
    EXPECT_FALSE(points.empty());
    EXPECT_FALSE(points.count(Vector2Int(0, 0)));
}

TEST(EdgarConfigSpace, OverCorridor_HorizontalCorridor) {
    auto room = PolygonGrid2D::get_square(5);
    SimpleDoorModeGrid2D room_door_mode(1, 0);
    auto room_doors = room_door_mode.get_doors(room);

    auto corridor = PolygonGrid2D::get_rectangle(2, 1);
    std::vector<DoorLineGrid2D> corridor_doors = {
        {OrthogonalLineGrid2D(Vector2Int(0, 1), Vector2Int(0, 0)), 1},
        {OrthogonalLineGrid2D(Vector2Int(2, 0), Vector2Int(2, 1)), 1},
    };

    ConfigurationSpacesGenerator gen;
    auto cs = gen.get_configuration_space_over_corridor(
        room, room_doors, room, room_doors, corridor, corridor_doors);

    EXPECT_EQ(cs.lines.size(), 0u);
}

TEST(EdgarConfigSpace, OverCorridors_CombinedHAndV) {
    auto room = PolygonGrid2D::get_square(5);
    SimpleDoorModeGrid2D room_door_mode(1, 0);
    auto room_doors = room_door_mode.get_doors(room);

    auto h_corridor = PolygonGrid2D::get_rectangle(2, 1);
    std::vector<DoorLineGrid2D> h_corridor_doors = {
        {OrthogonalLineGrid2D(Vector2Int(0, 1), Vector2Int(0, 0)), 1},
        {OrthogonalLineGrid2D(Vector2Int(2, 0), Vector2Int(2, 1)), 1},
    };

    auto v_corridor = PolygonGrid2D::get_rectangle(1, 2);
    std::vector<DoorLineGrid2D> v_corridor_doors = {
        {OrthogonalLineGrid2D(Vector2Int(1, 0), Vector2Int(0, 0)), 1},
        {OrthogonalLineGrid2D(Vector2Int(0, 2), Vector2Int(1, 2)), 1},
    };

    ConfigurationSpacesGenerator gen;
    auto cs = gen.get_configuration_space_over_corridors(
        room, room_doors, room, room_doors,
        {{h_corridor, h_corridor_doors}, {v_corridor, v_corridor_doors}});

    auto points = collect_cs_points(cs);
    EXPECT_FALSE(points.empty());
    EXPECT_FALSE(points.count(Vector2Int(0, 0)));
}

// ===========================================================================
// Этап A: additional geometry tests
// ===========================================================================

// A5: get_all_transformations - square (all unique)
// A6: rotate(90/270) - already have 2 tests
    EXPECT_FALSE(cs.lines.empty());
}
}
}

// A6: rectangle → 4 unique transforms
TEST(EdgarGeometry, RectangleRotate90) {
    auto rect = PolygonGrid2D::get_rectangle(3, 7);
    auto rotated = rect.rotate(90);
    auto expected = PolygonGrid2D::get_rectangle(7, 3);
    EXPECT_EQ(rotated, expected);
}
 }
}

// A10: constructor normalizes check (A10) on C# `NormalizePolygon` first edge must to go left)
 TEST(EdgarGeometry, PolygonNormalize_SquareNotNormalized) {
    auto shifted = PolygonGrid2D::get_rectangle(3, 7) + Vector2Int(5, 5);
    auto normalized = PolygonGrid2D::get_rectangle(3, 7);
    EXPECT_EQ(normalized, expected);
}

}

}

    EXPECT_TRUE(overlap_along_line(p1, r2, line). Start_before_first overlap))
    {
        // overlap at start
 with zero overlap
 only
 so overlap_along_line is empty

        auto r2 = PolygonGrid2D::get_rectangle(3, 5);
    auto line = OrthogonalLineGrid2D(Vector2Int(5, 2), Vector2Int(2, 3));
    auto result = overlap_along_line(r1, r2, line);
    // overlap at end, with offset=2 (line goes from to right)
        // fix positions: r2 just before right and left along line, don't overlap either.
        // join back right after offset,1
        auto [p1_end, = [p1_start]; result
 line [0] is be true(result.empty());
        EXPECT_EQ(result.front().first, Vector2Int(5, 2));
        EXPECT_TRUE(result.front().second);
    }

}

    {
        auto sq = PolygonGrid2D::get_square(5);
        auto line = OrthogonalLineGrid2D(Vector2Int(5, 2), Vector2Int(5, -2));
        auto result = overlap_along_line(sq, sq, line);
        // overlap at start, with offset=2 to right, then join back right
        // then go left along line to right
        // right of line is empty
 -> [p1_end];
 result = [p2_end]; result = [p2_start]; result = [p3_end]; result = [p4_end]; result = [p5_end]; result = [p6_end]; result = [p7_end]; result = [p8_end]; result = [p9_end]; result = [p10_end]; result = [p11_end]; result = [p12_end]; result = [p13_end]; result = [p14_end]; result = [p15_end]; result = [p16_end]; result = [p17_end]; result = [p18_end]; result = [p19_end]; result = [p20_end]; result = [p21_end]; result = [p22_end]; result = [p23_end]; result = [p24_end]; result = [p25_end]; result = [p26_end]; result = [p27_end]; result = [p28_end]; result = [p29_end]; result = [p30_end]; result = [p31_end]; result = [p32_end]; result = [p33_end]; result = [p34_end]; result = [p35_end]; result = [p36_end]; result = [p37_end]; result = [p38_end]; result = [p39_end]; result = [p40_end]; result = [p41_end]; result = [p42_end]; result = [p43_end]; result = [p44_end]; result = [p45_end]; result = [p46_end]; result = [p47_end]; result = [p48_end]; result = [p49_end]; result = [p50_end]; result = [p51_end]; result = [p52_end]; result = [p53_end]; result = [p54_end]; result = [p55_end]; result = [p56_end]; result = [p57_end]; result = [p58_end]; result = [p59_end]; result = [p60_end]; result = [p61_end]; result = [p62_end]; result = [p63_end]; result = [p64_end]; result = [p65_end]; result = [p66_end]; result = [p67_end]; result = [p68_end]; result = [p69_end]; result = [p70_end]; result = [p71_end]; result = [p72_end]; result = [p73_end]; result = [p74_end]; result = [p75_end]; result = [p76_end]; result = [p77_end]; result = [p78_end]; result = [p79_end]; result = [p80_end]; result = [p81_end]; result = [p82_end]; result = [p83_end]; result = [p84_end]; result = [p85_end]; result = [p86_end]; result = [p87_end]; result = [p88_end]; result = [p89_end]; result = [p90_end]; result = [p91_end]; result = [p92_end]; result = [p93_end]; result = [p94_end]; result = [p95_end]; result = [p96_end]; result = [p97_end]; result = [p98_end]; result = [p99_end]; result = [p100_end]; result = [p101_end]; result = [p102_end]; result = [p103_end]; result = [p104_end]; result = [p105_end]; result = [p106_end]; result = [p107_end]; result = [p108_end]; result = [p109_end]; result = [p110_end]; result = [p111_end]; result = [p112_end]; result = [p113_end]; result = [p114_end]; result = [p115_end]; result = [p116_end]; result = [p117_end]; result = [p118_end]; result = [p119_end]; result = [p120_end]; result = [p121_end]; result = [p122_end]; result = [p123_end]; result = [p124_end]; result = [p125_end]; result = [p126_end]; result = [p127_end]; result = [p128_end]; result = [p129_end]; result = [p130_end]; result =[131:     return result;
132: }

TEST(EdgarGeometry, OverlapAlongLine_OverlapEnd) {
    auto r1 = PolygonGrid2D::get_rectangle(3, 5);
    auto r2 = PolygonGrid2D::get_rectangle(2, 3);
    auto line = OrthogonalLineGrid2D(Vector2Int(5, 2), Vector2Int(5, 5));
    auto result = overlap_along_line(r1, r2, line);
    EXPECT_TRUE(result.empty());
}

}

TEST(EdgarGeometry, OverlapAlongLine_SingleInterval) {
    auto r1 = PolygonGrid2D::get_rectangle(3, 5);
    auto r2 = PolygonGrid2D::get_rectangle(2, 3);
    auto line = OrthogonalLineGrid2D(Vector2Int(5, -5), Vector2Int(5, 5));
    auto result = overlap_along_line(r1, r2, line);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].first, Vector2Int(5, 0));
    EXPECT_TRUE(result[0].second);
    EXPECT_EQ(result[1].first, Vector2Int(5, 5));
    EXPECT_FALSE(result[1].second);
}

}

TEST(EdgarGeometry, OverlapAlongLine_TwoIntervals) {
    auto sq = PolygonGrid2D::get_square(5);
    auto r2 = PolygonGrid2D::get_rectangle(2, 3);
    auto line = OrthogonalLineGrid2D(Vector2Int(0, -5), Vector2Int(0, 5));
    auto result = overlap_along_line(sq, r2, line);
    ASSERT_GE(result.size(), 2u);
    EXPECT_EQ(result[0].first, Vector2Int(0, 0));
    EXPECT_TRUE(result[0].second);
    EXPECT_EQ(result[1].first, Vector2Int(0, 5));
    EXPECT_FALSE(result[1].second);
}

}

TEST(EdgarGeometry, OverlapAlongLine_LShapeVsSquareHorizontal) {
    auto l = PolygonGrid2DBuilder()
        .add_point(0, 0).add_point(0, 6).add_point(3, 6)
        .add_point(3, 3).add_point(6, 3).add_point(6, 0).build();
    auto sq = PolygonGrid2D::get_square(4);
    auto line = OrthogonalLineGrid2D(Vector2Int(0, 0), Vector2Int(10, 0));
    auto result = overlap_along_line(l, sq, line);
    EXPECT_FALSE(result.empty());
}
}

TEST(EdgarGeometry, OverlapAlongLine_LShapeVsSquareVertical) {
    auto l = PolygonGrid2DBuilder()
        .add_point(0, 0).add_point(0, 6).add_point(3, 6)
        .add_point(3, 3).add_point(6, 3).add_point(6, 0).build();
    auto sq = PolygonGrid2D::get_square(4);
    auto line = OrthogonalLineGrid2D(Vector2Int(0, -5), Vector2Int(0, 5));
    auto result = overlap_along_line(l, sq, line);
    EXPECT_FALSE(result.empty());
}
}

// ===========================================================================
// Этап C: additional graph tests (planar faces)
// ===========================================================================

#include "edgar/graphs/planar_faces.hpp"

#include "edgar/generator/grid2d/constraints_evaluator_grid2d.hpp"

TEST(EdgarGraphs, IsTree_SingleVertex) {
    UndirectedAdjacencyListGraph<int> g;
    g.add_vertex(42);
    EXPECT_TRUE(is_tree(g));
}

 }
}

TEST(EdgarGraphs, IsTree_Star) {
    UndirectedAdjacencyListGraph<int> g;
    g.add_vertex(0); g.add_vertex(1); g.add_vertex(2); g.add_vertex(3);
    g.add_edge(0, 1); g.add_edge(1, 2); g.add_edge(2, 3);
    EXPECT_TRUE(is_tree(g));
}

 }
}

TEST(EdgarGraphs, PlanarFaces_Cycle) {
    UndirectedAdjacencyListGraph<int> g;
    g.add_vertex(0); g.add_vertex(1); g.add_vertex(2);
    g.add_edge(0, 1); g.add_edge(1, 2); g.add_edge(2, 0);
    auto faces = get_planar_faces(g);
    ASSERT_EQ(faces.size(), 2u);
    std::set<int> inner_face;
 sortedFaces[1].begin(), innerFace.end(), std::set<int> outer_face;
 sortedFaces[0].begin(), outerFace.end());
 std::set<int> outerFace;
 sortedFaces[1].begin(), outerFace.end());
 std::set<int> all_pts;
 sortedFaces[0].begin(), all_pts.end());
 EXPECT_EQ(all_pts.size(), 6u);
}
}
}
    auto corridor = PolygonGrid2DBuilder()
        .add_point(0, 1)
        .add_point(0, 2)
        .add_point(2, 2)
        .add_point(2, 0)
        .add_point(1, 0)
        .add_point(1, 1)
        .build();

    std::vector<DoorLineGrid2D> corridor_doors = {
        {OrthogonalLineGrid2D(Vector2Int(0, 2), Vector2Int(0, 1)), 1},
        {OrthogonalLineGrid2D(Vector2Int(2, 0), Vector2Int(1, 0)), 1},
    };

    ConfigurationSpacesGenerator gen;
    auto cs = gen.get_configuration_space_over_corridor(
        room, room_doors, room, room_doors, corridor, corridor_doors);

    EXPECT_FALSE(cs.lines.empty());
}

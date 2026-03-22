#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "edgar/edgar.hpp"
#include "edgar/geometry/overlap.hpp"

TEST(EdgarGeometry, RectanglePolygonClockwise) {
    using namespace edgar::geometry;
    const auto poly = edgar::geometry::PolygonGrid2D::get_rectangle(6, 10);
    EXPECT_GE(poly.points().size(), 4u);
    EXPECT_EQ(poly.bounding_rectangle().width(), 6);
    EXPECT_EQ(poly.bounding_rectangle().height(), 10);
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

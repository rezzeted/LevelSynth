#include "layout_preview.hpp"

#include <algorithm>
#include <limits>
#include <vector>

void draw_layout_preview_imgui(const edgar::generator::grid2d::LayoutGrid2D<int>& layout) {
    using namespace edgar::generator::grid2d;
    if (layout.rooms.empty()) {
        return;
    }
    int min_x = std::numeric_limits<int>::max();
    int min_y = std::numeric_limits<int>::max();
    int max_x = std::numeric_limits<int>::min();
    int max_y = std::numeric_limits<int>::min();
    for (const auto& room : layout.rooms) {
        for (const auto& p : room.outline.points()) {
            const int wx = p.x + room.position.x;
            const int wy = p.y + room.position.y;
            min_x = std::min(min_x, wx);
            min_y = std::min(min_y, wy);
            max_x = std::max(max_x, wx);
            max_y = std::max(max_y, wy);
        }
    }
    const float dx = static_cast<float>(std::max(1, max_x - min_x));
    const float dy = static_cast<float>(std::max(1, max_y - min_y));

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float canvas_w = std::max(100.0f, avail.x);
    const float canvas_h = std::max(200.0f, avail.y);
    const ImVec2 canvas_sz(canvas_w, canvas_h);
    const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    const ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(24, 26, 30, 255));
    dl->PushClipRect(canvas_p0, canvas_p1, true);

    constexpr float pad = 16.0f;
    const float scale =
        std::min((canvas_sz.x - 2.0f * pad) / dx, (canvas_sz.y - 2.0f * pad) / dy);

    auto to_screen = [&](int wx, int wy) {
        return ImVec2(canvas_p0.x + pad + (static_cast<float>(wx - min_x)) * scale,
                      canvas_p0.y + pad + (static_cast<float>(wy - min_y)) * scale);
    };

    for (const auto& room : layout.rooms) {
        std::vector<ImVec2> pts;
        pts.reserve(room.outline.points().size());
        for (const auto& p : room.outline.points()) {
            pts.push_back(to_screen(p.x + room.position.x, p.y + room.position.y));
        }
        if (pts.size() < 2) {
            continue;
        }
        const ImU32 stroke = room.is_corridor ? IM_COL32(120, 200, 230, 255) : IM_COL32(180, 220, 130, 255);
        dl->AddPolyline(pts.data(), static_cast<int>(pts.size()), stroke, ImDrawFlags_Closed, 2.5f);
    }

    dl->PopClipRect();
    dl->AddRect(canvas_p0, canvas_p1, IM_COL32(80, 80, 95, 255));

    ImGui::Dummy(canvas_sz);
}

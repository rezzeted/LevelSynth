#pragma once

#include "edgar/edgar.hpp"
#include "imgui.h"

extern bool g_preview_grid_lines;
extern bool g_preview_shading;

void draw_layout_preview_imgui(const edgar::generator::grid2d::LayoutGrid2D<int>& layout);

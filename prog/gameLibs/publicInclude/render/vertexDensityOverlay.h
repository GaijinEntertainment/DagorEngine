//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// purpose: display density of vertexes on screen, to catch badly configured LODs/too dense geometry in realtime
// method:
//   -for every vertex, write +1 in specialized square UAV at index corresponding to its position
//     (this is enaled per shader!)
//   -calculate density as sum of said values around local point divided by area where accumulation happens
//   -draw resulting value via color mapping on screen, red is high density (bad), blue is low (good)
// note: vertex density is not triangle size! so be aware that not all high density areas are surely bad one (check visually)

namespace VertexDensityOverlay
{
void init();
void shutdown();

// run once per frame before using any shader with writeVertexDensityPos
void before_render();

// bind/unbind pair, should pair-used inside rendering where shaders with writeVertexDensityPos are utilized
// TODO: remove this when vertex shader UAV-buffer is fully supported by compiler
void bind_UAV();
void unbind_UAV();

// draws heatmap to current render target, with blending enabled (to see context of situation)
void draw_heatmap();
} // namespace VertexDensityOverlay

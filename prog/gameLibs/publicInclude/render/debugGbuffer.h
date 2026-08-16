//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <EASTL/array.h>
#include <EASTL/string_view.h>
#include <EASTL/string.h>

enum class DebugGbufferMode
{
  None = -1,
#define MODE(mode, num)             mode = num - 1,
#define MODE_HAS_VECTORS(mode, num) mode = num - 1,
#define LAST_MODE(mode)
#define DEBUG_MESH_MODE(mode) mode,
#include "debugGbufferModes.h"
#undef DEBUG_MESH_MODE
#undef LAST_MODE
#undef MODE_HAS_VECTORS
#undef MODE
  Count,
};

extern const eastl::array<eastl::string_view, (size_t)DebugGbufferMode::Count> gbuffer_debug_options;
extern const eastl::array<eastl::string_view, (size_t)DebugGbufferMode::Count> gbuffer_debug_with_vectors_options;
extern DebugGbufferMode show_gbuffer;
bool shouldRenderGbufferDebug();

enum class DebugGbufferComposition
{
  Single = 0,
  Grid,     // 4x4 mosaic of 16 debug modes covering the whole screen
  Overview, // 12-mode border ring, gameplay frame visible in the center
};

extern DebugGbufferComposition show_gbuffer_composition;
extern const eastl::array<eastl::string_view, 2> gbuffer_debug_composition_options;

void setDebugGbufferMode(eastl::string_view mode, bool with_compositions = false);
void setDebugGbufferWithVectorsMode(eastl::string_view mode, int vectorsCount, float vectorsScale);
eastl::string getDebugGbufferUsage();
eastl::string getDebugGbufferWithVectorsUsage();
eastl::string getDebugGbufferModeName(DebugGbufferMode mode);

class DeferredRT;

extern const int USE_DEBUG_GBUFFER_MODE;

constexpr auto DEBUG_RENDER_GBUFFER_SHADER_NAME = "debug_final_gbuffer";
constexpr auto DEBUG_RENDER_GBUFFER_WITH_VECTORS_SHADER_NAME = "debug_final_gbuffer_vec";

void debug_render_gbuffer(const class PostFxRenderer &debugRenderer, DeferredRT &gbuffer, Texture *depth = nullptr,
  int mode = USE_DEBUG_GBUFFER_MODE);
void debug_render_gbuffer(const class PostFxRenderer &debugRenderer, Texture *depth, int mode = USE_DEBUG_GBUFFER_MODE);

void debug_render_gbuffer_tiles(const PostFxRenderer &debugRenderer, DebugGbufferComposition composition, bool with_labels = false);

void debug_render_gbuffer_with_vectors(const class DynamicShaderHelper &debugVecShader, DeferredRT &gbuffer,
  int mode = USE_DEBUG_GBUFFER_MODE, int vec_count = -1, float vec_scale = 0.f);
void debug_render_gbuffer_with_vectors(const class DynamicShaderHelper &debugVecShader, Texture *depth,
  int mode = USE_DEBUG_GBUFFER_MODE, int vec_count = -1, float vec_scale = 0.f);

DebugGbufferMode get_debug_gbuffer_mode();
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

void setDebugGbufferMode(eastl::string_view mode);
void setDebugGbufferWithVectorsMode(eastl::string_view mode, int vectorsCount, float vectorsScale);
eastl::string getDebugGbufferUsage();
eastl::string getDebugGbufferWithVectorsUsage();

class DeferredRT;

extern const int USE_DEBUG_GBUFFER_MODE;

constexpr auto DEBUG_RENDER_GBUFFER_SHADER_NAME = "debug_final_gbuffer";
constexpr auto DEBUG_RENDER_GBUFFER_WITH_VECTORS_SHADER_NAME = "debug_final_gbuffer_vec";

void debug_render_gbuffer(const class PostFxRenderer &debugRenderer, DeferredRT &gbuffer, int mode = USE_DEBUG_GBUFFER_MODE);
void debug_render_gbuffer(const class PostFxRenderer &debugRenderer, Texture *depth, int mode = USE_DEBUG_GBUFFER_MODE);

void debug_render_gbuffer_with_vectors(const class DynamicShaderHelper &debugVecShader, DeferredRT &gbuffer,
  int mode = USE_DEBUG_GBUFFER_MODE);
void debug_render_gbuffer_with_vectors(const class DynamicShaderHelper &debugVecShader, Texture *depth,
  int mode = USE_DEBUG_GBUFFER_MODE);

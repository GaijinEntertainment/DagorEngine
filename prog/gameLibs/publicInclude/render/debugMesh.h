//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/debugGbuffer.h>

namespace debug_mesh
{
enum class Type
{
  NONE,
  FIRST_DEBUG_MESH_MODE =
#define MODE(mode, num)             1 +
#define MODE_HAS_VECTORS(mode, num) 1 +
#define LAST_MODE(mode)             0,
#define DEBUG_MESH_MODE(mode)       mode = (int)DebugGbufferMode::mode,
#include <render/debugGbufferModes.h>
#undef DEBUG_MESH_MODE
#undef LAST_MODE
#undef MODE_HAS_VECTORS
#undef MODE
    ANY
};

Type debug_gbuffer_mode_to_type(DebugGbufferMode mode);

bool enable_debug(Type type);
bool is_enabled(Type type = Type::ANY);

bool set_debug_value(int lod);
void reset_debug_value();

void activate_mesh_coloring_master_override();
void deactivate_mesh_coloring_master_override();
} // namespace debug_mesh

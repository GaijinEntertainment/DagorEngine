// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/debugMesh.h>

#include <drv/3d/dag_driver.h>
#include <shaders/dag_overrideStates.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <render/debug_mesh.hlsli>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>

CONSOLE_INT_VAL("debug_mesh", overdraw_range, 256, 0, 256);

namespace debug_mesh
{
static Type debug_counter = Type::NONE;
static int range_min = 0, range_max = DEBUG_MESH_COLORS__MAX;
static shaders::UniqueOverrideStateId stencilMasterOverride;
static ShaderVariableInfo overdraw_rangeVarId("overdraw_range", /*opt*/ true);

Type debug_gbuffer_mode_to_type(DebugGbufferMode mode)
{
  if ((int)mode < (int)Type::FIRST_DEBUG_MESH_MODE)
    return Type::NONE;
  return (Type)mode;
}

bool enable_debug(Type type)
{
  ShaderGlobal::set_real(overdraw_rangeVarId, (float)overdraw_range.get());

  if (debug_counter == type)
    return false;

  shaders::OverrideState state;
  state.set(shaders::OverrideState::STENCIL);

  if (type == Type::overdraw)
    state.stencil.set(CMPF_ALWAYS, STNCLOP_KEEP, STNCLOP_INCR, STNCLOP_INCR, 0xFF, 0xFF);
  else
    state.stencil.set(CMPF_ALWAYS, STNCLOP_KEEP, STNCLOP_KEEP, STNCLOP_REPLACE, 0xFF, 0xFF);

  stencilMasterOverride.reset(shaders::overrides::create(state));

  debug_counter = type;
  return true;
}
bool is_enabled(Type type)
{
  if (type == Type::ANY)
    return debug_counter != Type::NONE;
  return debug_counter == type;
}


bool set_debug_value(int value)
{
  if (debug_counter != Type::NONE)
  {
    shaders::set_stencil_ref(max((value - range_min) * DEBUG_MESH_COLORS__MAX / (range_max - range_min), 0) + 1);
  }
  return debug_counter != Type::NONE;
}
void reset_debug_value()
{
  if (debug_counter != Type::NONE)
    shaders::set_stencil_ref(0);
}


void activate_mesh_coloring_master_override()
{
  if (debug_counter != Type::NONE)
  {
    shaders::overrides::set_master_state(shaders::overrides::get(stencilMasterOverride));
    debug_mesh::reset_debug_value();
  }
}
void deactivate_mesh_coloring_master_override()
{
  if (debug_counter != Type::NONE)
    shaders::overrides::reset_master_state();
}
} // namespace debug_mesh

static bool debug_mesh_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("debug_mesh", "range", 3, 3)
  {
    static const char *COLOR_NAMES[DEBUG_MESH_COLORS__MAX + 1] = {"white", "red", "yellow", "green", "cyan", "blue", "magenta"};

    debug_mesh::range_min = atoi(argv[1]);
    debug_mesh::range_max = atoi(argv[2]);
    if (debug_mesh::range_min == debug_mesh::range_max)
    {
      console::print_d("Range couldn't be empty! Reset values to default: 0 %d", DEBUG_MESH_COLORS__MAX);
      debug_mesh::range_min = 0;
      debug_mesh::range_max = DEBUG_MESH_COLORS__MAX;
    }
    else
    {
      console::print_d("Current range rules:\n %s < %d", COLOR_NAMES[0], debug_mesh::range_min + 1);
      for (uint32_t i = 1; i < DEBUG_MESH_COLORS__MAX; ++i)
      {
        const uint32_t rangeSize = debug_mesh::range_max - debug_mesh::range_min;
        const uint32_t colorBegin =
          (i * rangeSize + debug_mesh::range_min * DEBUG_MESH_COLORS__MAX + DEBUG_MESH_COLORS__MAX - 1) / DEBUG_MESH_COLORS__MAX;
        const uint32_t colorEnd =
          ((i + 1) * rangeSize + debug_mesh::range_min * DEBUG_MESH_COLORS__MAX + DEBUG_MESH_COLORS__MAX - 1) / DEBUG_MESH_COLORS__MAX;
        if (colorBegin < colorEnd)
          console::print_d("%d <= %s < %d", colorBegin, COLOR_NAMES[i], colorEnd);
      }
      console::print_d("%d <= %s", debug_mesh::range_max, COLOR_NAMES[DEBUG_MESH_COLORS__MAX]);
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(debug_mesh_console_handler);

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>

#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <ecs/render/resPtr.h>


CONSOLE_BOOL_VAL("render", show_lasers, true);
CONSOLE_FLOAT_VAL_MINMAX("render", debug_laser_size_mult, 1.0f, 0.001f, 100.f);

template <typename Callable>
inline void get_all_lasers_ecs_query(ecs::EntityManager &manager, Callable c);

static bool lasers_test_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("render", "enable_all_lasers", 1, 1)
  {
    get_all_lasers_ecs_query(*g_entity_mgr, [&](bool &laserActive) { laserActive = true; });
  }
  CONSOLE_CHECK_NAME("render", "disable_all_lasers", 1, 1)
  {
    get_all_lasers_ecs_query(*g_entity_mgr, [&](bool &laserActive) { laserActive = false; });
  }
  CONSOLE_CHECK_NAME("render", "switch_laser_debug_mode", 1, 1)
  {
    int varId = ::get_shader_variable_id("laser_decals_debug_mode", true);
    if (varId == -1)
      return found;
    int mode = ShaderGlobal::get_int(varId);
    ShaderGlobal::set_int(varId, mode ? 0 : 1);
  }
  return found;
}
REGISTER_CONSOLE_HANDLER(lasers_test_console_handler);
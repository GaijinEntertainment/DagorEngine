// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/input/debugInputEvents.h>
#include <shaders/dag_shaders.h>
#include <gui/dag_visualLog.h>

// Example:
// #define SHADER_VAR_TOGGLE_LIST \
//   VAR_INT(int_shader_var, 0, 1, HumanInput::DKEY_I) \
//   VAR_BOOL(bool_shader_var, HumanInput::DKEY_B)

#define SHADER_VAR_TOGGLE_LIST

ECS_TAG(render, dev)
void toggle_shadervars_es_event_handler(const EventDebugKeyboardPressed &evt)
{
  G_UNUSED(evt);
#define VAR_INT(name, minValue, maxValue, key)               \
  if (evt.get<0>() == key)                                   \
  {                                                          \
    int name##VarId = get_shader_variable_id(#name);         \
    if (VariableMap::isGlobVariablePresent(name##VarId))     \
    {                                                        \
      int value = ShaderGlobal::get_int_fast(name##VarId);   \
      value++;                                               \
      if (value > maxValue)                                  \
        value = minValue;                                    \
      ShaderGlobal::set_int(name##VarId, value);             \
      visuallog::logmsg(String(0, "%s = %d", #name, value)); \
    }                                                        \
  }
#define VAR_BOOL(name, key) VAR_INT(name, 0, 1, key)
  SHADER_VAR_TOGGLE_LIST
#undef VAR_BOOL
#undef VAR_INT
}
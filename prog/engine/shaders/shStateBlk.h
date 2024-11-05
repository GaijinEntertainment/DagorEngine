// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaders.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include "shadersBinaryData.h"


class ScriptedShaderMaterial;

namespace shaders_internal
{
void register_material(ScriptedShaderMaterial *mat);
void unregister_material(ScriptedShaderMaterial *mat);

void register_mat_elem(ScriptedShaderElement *elem);
void unregister_mat_elem(ScriptedShaderElement *elem);

bool reload_shaders_materials(ScriptedShadersBinDumpOwner &prev_sh_owner);
bool reload_shaders_materials_on_driver_reset();
void reset_shaders_state_blocks();

void init_stateblocks();
void close_stateblocks();
void close_shader_block_stateblocks(bool final);
void close_vprog();
void close_fshader();
void close_global_constbuffers();
void lock_block_critsec();
void unlock_block_critsec();
class BlockAutoLock
{
public:
  BlockAutoLock() { lock_block_critsec(); }
  ~BlockAutoLock() { unlock_block_critsec(); }
};

extern volatile int cached_state_block;

extern ska::flat_hash_set<ScriptedShaderMaterial *> shader_mats;
extern ska::flat_hash_set<ScriptedShaderElement *> shader_mat_elems;

extern bool shader_reload_allowed;
extern int nan_counter;
extern int shader_pad_for_reload;

inline bool check_var_nan(float v, int variable_id)
{
#if _TARGET_PC
  if (check_nan(v))
  {
#if DAGOR_DBGLEVEL > 0
    if (!((nan_counter++) & 63)) // rarely produce logs
#else
    G_ASSERTF(0, "setting <%s> variable to NaN", VariableMap::getVariableName(variable_id));
#endif
      logerr("setting <%s> variable to nan", VariableMap::getVariableName(variable_id));
    return false;
  }
#endif
  return true;
}
}; // namespace shaders_internal

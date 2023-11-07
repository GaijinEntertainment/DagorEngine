#include "shaders/sh_vars.h"
#include <shaders/dag_shaders.h>

// other vars
int shglobvars::globalTransRGvId = -1;
int shglobvars::worldViewPosGvId = -1;
int shglobvars::shaderLodGvId = -1;
int shglobvars::localWorldXGvId = -1, shglobvars::localWorldYGvId = -1, shglobvars::localWorldZGvId = -1;
int shglobvars::dynamic_pos_unpack_reg = 253;

void shglobvars::init_varids_loaded()
{
  globalTransRGvId = ShaderGlobal::get_glob_var_id(get_shader_variable_id("global_transp_r", true));
  ShaderGlobal::set_real_fast(globalTransRGvId, 1.0f);
  worldViewPosGvId = ShaderGlobal::get_glob_var_id(get_shader_variable_id("world_view_pos"));
  localWorldXGvId = ShaderGlobal::get_glob_var_id(get_shader_variable_id("local_world_x", true));
  localWorldYGvId = ShaderGlobal::get_glob_var_id(get_shader_variable_id("local_world_y", true));
  localWorldZGvId = ShaderGlobal::get_glob_var_id(get_shader_variable_id("local_world_z", true));
  shaderLodGvId = ShaderGlobal::get_glob_var_id(get_shader_variable_id("shader_lod", true));
  int dynamic_pos_unpack_regVarId = get_shader_variable_id("dynamic_pos_unpack_reg", true);
  if (dynamic_pos_unpack_regVarId >= 0)
  {
    dynamic_pos_unpack_reg = ShaderGlobal::get_int_fast(dynamic_pos_unpack_regVarId);
  }
}

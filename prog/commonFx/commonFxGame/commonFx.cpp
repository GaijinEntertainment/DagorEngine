// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fx/dag_commonFx.h>
#include <shaders/dag_shaders.h>


int softness_distance_var_id = -1;

void register_all_common_fx_factories()
{
  register_compound_ps_fx_factory();
  register_flow_ps_2_fx_factory();
  register_light_fx_factory();

  register_dafx_sparks_factory();
  register_dafx_modfx_factory();
  register_dafx_compound_factory();

  softness_distance_var_id = get_shader_variable_id("softness_distance", true);
}

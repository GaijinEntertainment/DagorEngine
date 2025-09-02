// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fx/dag_commonFx.h>
#include <shaders/dag_shaders.h>


void register_all_common_fx_factories()
{
  register_light_fx_factory();

  register_dafx_sparks_factory();
  register_dafx_modfx_factory();
  register_dafx_compound_factory();
}

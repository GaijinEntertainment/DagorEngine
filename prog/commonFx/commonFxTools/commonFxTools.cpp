// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fx/commonFxTools.h>
#include <util/dag_string.h>

String fx_devres_base_path;

void register_all_common_fx_tools()
{
  register_light_fx_tools();

  register_dafx_sparks_fx_tools();
  register_dafx_modfx_fx_tools();
  register_dafx_compound_fx_tools();
}

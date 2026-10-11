// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rootProps.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"

const bool DEFAULT_EXPORT = true;

void root_props_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "root");
  add_edit_bool_if_not_exists(params, panel, field_idx, "export", DEFAULT_EXPORT);
  add_edit_bool_if_not_exists(params, panel, field_idx, "defaultForeignAnim");
}

void root_props_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_bool(params, panel, "export", DEFAULT_EXPORT);
  remove_param_if_default_bool(params, panel, "defaultForeignAnim");
}

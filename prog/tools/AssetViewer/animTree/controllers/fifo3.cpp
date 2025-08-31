// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fifo3.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"

void fifo3_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "varname");
}

void fifo3_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "name");
  remove_param_if_default_str(params, panel, "varname");
}

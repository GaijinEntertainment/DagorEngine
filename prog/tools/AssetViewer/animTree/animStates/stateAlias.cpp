// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stateAlias.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"

void state_alias_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "sameAs");
  add_edit_box_if_not_exists(params, panel, field_idx, "tag");
}

void state_alias_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "tag");
}

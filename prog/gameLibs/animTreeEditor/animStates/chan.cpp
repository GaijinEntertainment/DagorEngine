// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "chan.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"

#include <propPanel/control/container.h>

static int const DEFAULT_COND_TARGET = -1;
static int const DEFAULT_AUTO_RESET_SINGLE_ANIM_MORPH_TIME = -1;

void chan_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "nodeMask", nameValue);
  add_edit_box_if_not_exists(params, panel, field_idx, "fifo3");
  add_edit_int_if_not_exists(params, panel, field_idx, "cond_target", DEFAULT_COND_TARGET);
  add_edit_box_if_not_exists(params, panel, field_idx, "cond_nodeMask");
  add_edit_int_if_not_exists(params, panel, field_idx, "autoResetSingleAnimMorphTime", DEFAULT_AUTO_RESET_SINGLE_ANIM_MORPH_TIME);
}

void chan_set_dependent_defaults(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  set_str_param_by_name_if_default(params, panel, "nodeMask", nameValue.c_str(), nameValue.c_str());
}

void chan_update_dependent_fields(AnimParamData &param, dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  if (param.name == "name")
  {
    const SimpleString nameValue = panel->getText(param.pid);
    AnimParamData *nodeMaskData = find_param_by_name(params, "nodeMask");
    if (nodeMaskData == params.end())
      return;

    if (nodeMaskData->dependent)
      panel->setText(nodeMaskData->pid, nameValue);
  }
  else if (param.name == "nodeMask")
  {
    const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
    param.dependent = panel->getText(param.pid) == nameValue;
  }
}

void chan_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  remove_param_if_default_str(params, panel, "nodeMask", nameValue);
  remove_param_if_default_int(params, panel, "cond_target", DEFAULT_COND_TARGET);
  remove_param_if_default_str(params, panel, "cond_nodeMask");
  remove_param_if_default_int(params, panel, "autoResetSingleAnimMorphTime", DEFAULT_AUTO_RESET_SINGLE_ANIM_MORPH_TIME);
}

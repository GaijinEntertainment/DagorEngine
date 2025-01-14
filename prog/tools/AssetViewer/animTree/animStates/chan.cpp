// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "chan.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../animParamData.h"

static int const DEFAULT_COND_TARGET = -1;
static int const DEFAULT_AUTO_RESET_SINGLE_ANIM_MORPH_TIME = -1;

void chan_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "nodeMask");
  add_edit_box_if_not_exists(params, panel, field_idx, "fifo3");
  add_edit_int_if_not_exists(params, panel, field_idx, "cond_target", DEFAULT_COND_TARGET);
  add_edit_box_if_not_exists(params, panel, field_idx, "cond_nodeMask");
  add_edit_int_if_not_exists(params, panel, field_idx, "autoResetSingleAnimMorphTime", DEFAULT_AUTO_RESET_SINGLE_ANIM_MORPH_TIME);
}

void chan_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_int(params, panel, "cond_target", DEFAULT_COND_TARGET);
  remove_param_if_default_str(params, panel, "cond_nodeMask");
  remove_param_if_default_int(params, panel, "autoResetSingleAnimMorphTime", DEFAULT_AUTO_RESET_SINGLE_ANIM_MORPH_TIME);
}

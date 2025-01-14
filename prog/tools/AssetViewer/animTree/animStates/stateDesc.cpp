// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stateDesc.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../animParamData.h"

static const float DEFAULT_DEF_MORPH_TIME = 0.15f;

void state_desc_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "defNodeName");
  add_edit_float_if_not_exists(params, panel, field_idx, "defMorphTime", DEFAULT_DEF_MORPH_TIME);
  add_edit_float_if_not_exists(params, panel, field_idx, "minTimeScale");
  add_edit_float_if_not_exists(params, panel, field_idx, "maxTimeScale", FLT_MAX);
}

void state_desc_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "defNodeName");
  remove_param_if_default_float(params, panel, "defMorphTime", DEFAULT_DEF_MORPH_TIME);
  remove_param_if_default_float(params, panel, "minTimeScale");
  remove_param_if_default_float(params, panel, "maxTimeScale", FLT_MAX);
}
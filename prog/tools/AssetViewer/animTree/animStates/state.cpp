// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "state.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"

static const float DEFAULT_DEF_MORPH_TIME = 0.15f;
static const float DEFAULT_FORCE_DUR = -1.f;
static const float DEFAULT_FORCE_SPD = -1.f;

void state_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx,
  const DataBlock &props)
{
  const float defMorphTime = props.getReal("defMorphTime", DEFAULT_DEF_MORPH_TIME);
  const float defMinTimeScale = props.getReal("minTimeScale", 0.f);
  const float defMaxTimeScale = props.getReal("maxTimeScale", FLT_MAX);
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "tag");
  add_edit_float_if_not_exists(params, panel, field_idx, "forceDur", DEFAULT_FORCE_DUR);
  add_edit_float_if_not_exists(params, panel, field_idx, "forceSpd", DEFAULT_FORCE_SPD);
  add_edit_float_if_not_exists(params, panel, field_idx, "morphTime", defMorphTime);
  add_edit_float_if_not_exists(params, panel, field_idx, "minTimeScale", defMinTimeScale);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxTimeScale", defMaxTimeScale);
}

void state_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const DataBlock &props)
{
  const float defMorphTime = props.getReal("defMorphTime", DEFAULT_DEF_MORPH_TIME);
  const float defMinTimeScale = props.getReal("minTimeScale", 0.f);
  const float defMaxTimeScale = props.getReal("maxTimeScale", FLT_MAX);
  remove_param_if_default_str(params, panel, "tag");
  remove_param_if_default_float(params, panel, "forceDur", DEFAULT_FORCE_DUR);
  remove_param_if_default_float(params, panel, "forceSpd", DEFAULT_FORCE_SPD);
  remove_param_if_default_float(params, panel, "morphTime", defMorphTime);
  remove_param_if_default_float(params, panel, "minTimeScale", defMinTimeScale);
  remove_param_if_default_float(params, panel, "maxTimeScale", defMaxTimeScale);
}

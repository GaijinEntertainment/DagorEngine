// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "morph.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"
#include "../animMorphType.h"

void morph_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "from");
  add_edit_box_if_not_exists(params, panel, field_idx, "to");
  add_edit_float_if_not_exists(params, panel, field_idx, "morphTime");
  add_combo_if_not_exists(params, panel, field_idx, "morphType", morph_type_names, DEFAULT_MORPH_TYPE);
  add_edit_bool_if_not_exists(params, panel, field_idx, "twoSided");
}

void morph_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "from");
  remove_param_if_default_str(params, panel, "to");
  remove_param_if_default_float(params, panel, "morphTime");
  remove_param_if_default_str(params, panel, "morphType", DEFAULT_MORPH_TYPE);
  remove_param_if_default_bool(params, panel, "twoSided");
}

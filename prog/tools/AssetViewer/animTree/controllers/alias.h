// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
class DataBlock;

void alias_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void alias_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
const char *alias_get_child_name_by_idx(const DataBlock &settings, int idx);
void alias_update_child_name(DataBlock &settings, const char *name, const String &old_name);

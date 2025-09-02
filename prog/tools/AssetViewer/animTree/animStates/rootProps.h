// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;

void root_props_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void root_props_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

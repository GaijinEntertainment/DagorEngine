// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;

void eye_ctrl_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void eye_ctrl_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

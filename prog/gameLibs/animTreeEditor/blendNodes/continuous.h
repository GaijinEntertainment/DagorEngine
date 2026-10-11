// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
class DataBlock;
class DagorAssetMgr;

void continuous_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx,
  bool default_foreign);
void continuous_set_dependent_defaults(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const DagorAssetMgr &mgr);
void continuous_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, bool default_foreign,
  const DagorAssetMgr &mgr);

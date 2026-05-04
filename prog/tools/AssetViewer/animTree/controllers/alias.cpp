// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "alias.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../animTree.h"
#include "animCtrlData.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

void alias_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "origin");
  add_edit_bool_if_not_exists(params, panel, field_idx, "allowSuffixless");
}

void alias_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_bool(params, panel, "allowSuffixless");
}

void AnimTreePlugin::aliasFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings)
{
  const char *childName = settings.getStr("origin", nullptr);
  if (childName)
  {
    add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, childName);
    check_ctrl_child_idx(data.childs[0], settings.getStr("name"), childName);
  }
}

const char *alias_get_child_name_by_idx(const DataBlock &settings, int) { return settings.getStr("origin", ""); }

void alias_update_child_name(DataBlock &settings, const char *name, const String &old_name)
{
  const char *childName = settings.getStr("origin", nullptr);
  String writeName;
  if (childName && get_updated_child_name(name, old_name, childName, writeName))
    settings.setStr("origin", writeName.c_str());
}

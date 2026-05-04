// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "postBlendCtrlOrder.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../controllers/animCtrlData.h"
#include "../animTree.h"
#include "animStatesData.h"
#include "animStatesType.h"

#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

Tab<String> post_blend_ctrl_order_prepare_combo_values(PropPanel::ContainerPropertyControl *panel,
  dag::ConstSpan<AnimCtrlData> controllers)
{
  Tab<String> names;
  names.emplace_back("##");
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  for (const AnimCtrlData &data : controllers)
  {
    if (is_post_blend_ctrl(data.type))
      names.emplace_back(ctrlsTree->getCaption(data.handle));
  }
  return names;
}

void post_blend_ctrl_order_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings,
  Tab<String> &combo_names)
{
  Tab<String> names;
  names.reserve(settings.paramCount());
  const int nameNid = settings.getNameId("name");
  for (int i = 0; i < settings.paramCount(); ++i)
    if (nameNid == settings.getParamNameId(i))
      names.emplace_back(String(settings.getStr(i)));

  const char *defaultValue = settings.paramCount() ? settings.getStr(0) : "";
  panel->createList(PID_STATES_NODES_LIST, "postBlendCtrlOrder", names, 0);
  panel->createButton(PID_STATES_NODES_LIST_ADD, "Add");
  panel->createButton(PID_STATES_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  int selectedValue = get_selected_name_idx_combo(combo_names, defaultValue);
  panel->createCombo(PID_STATES_POST_BLEND_CTRL_ORDER_NAME, "Ctrl name", combo_names, selectedValue);
  if (names.empty())
    panel->setEnabledById(PID_STATES_POST_BLEND_CTRL_ORDER_NAME, /*enabled*/ false);
}

static int get_param_idx_by_list_name(const DataBlock &settings, const SimpleString &list_name, int selected_idx)
{
  for (int i = selected_idx; i < settings.paramCount(); ++i)
    if (list_name == settings.getStr(i))
      return i;

  return -1;
}

void post_blend_ctrl_order_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings)
{
  const SimpleString listName = panel->getText(PID_STATES_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_STATES_NODES_LIST);
  // Skip commented fields
  int paramIdx = get_param_idx_by_list_name(settings, listName, selectedIdx);
  panel->setText(PID_STATES_POST_BLEND_CTRL_ORDER_NAME, paramIdx != -1 ? settings.getStr(paramIdx) : "");
}

void post_blend_ctrl_order_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock &settings)
{
  const SimpleString listName = panel->getText(PID_STATES_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_STATES_NODES_LIST);
  // Skip commented fields
  int paramIdx = get_param_idx_by_list_name(settings, listName, selectedIdx);
  if (paramIdx != -1)
    settings.removeParam(paramIdx);
}

void post_blend_ctrl_order_update_child_name(DataBlock &settings, const char *name, const String &old_name)
{
  const int nameNid = settings.getNameId("name");
  for (int i = 0; i < settings.paramCount(); ++i)
    if (settings.getParamNameId(i) == nameNid && settings.getStr(i) == old_name)
      settings.setStr(i, name);
}

void AnimTreePlugin::postBlendCtrlOrderSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock &settings,
  AnimStatesData &data)
{
  const SimpleString listName = panel->getText(PID_STATES_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_STATES_NODES_LIST);
  const SimpleString nodeName = get_text_from_combo(panel, PID_STATES_POST_BLEND_CTRL_ORDER_NAME);
  // Skip commented fields
  int paramIdx = get_param_idx_by_list_name(settings, listName, selectedIdx);
  if (paramIdx == -1 || (nodeName != settings.getStr(paramIdx)))
  {
    data.childs[selectedIdx] = find_state_child_idx_by_name(panel, data.handle, controllersData, blendNodesData, nodeName);
    check_init_anim_state_child_idx(data.childs[selectedIdx], "postBlendCtrlOrder", nodeName);
  }
  if (paramIdx != -1)
    settings.setStr(paramIdx, nodeName);
  else
    settings.addStr("name", nodeName);

  if (listName != nodeName)
    panel->setText(PID_STATES_NODES_LIST, nodeName);
}

class PostBlendCtrlOrderReorderHandler : public BaseAnimStatesReorderHandler
{
public:
  PostBlendCtrlOrderReorderHandler(AnimTreePlugin &plugin, dag::ConstSpan<AnimStatesData> states,
    PropPanel::ContainerPropertyControl *panel) :
    BaseAnimStatesReorderHandler(plugin, states, panel)
  {}

protected:
  AnimStatesType getTargetAnimStateType() const override { return AnimStatesType::INIT_ANIM_STATE; }

  void handleSpecificReorder(DataBlock &props, int from, int to) override
  {
    DataBlock *settings = props.addBlock("initAnimState")->addBlock("postBlendCtrlOrder");
    move_param_blk(*settings, from, to);
  }
};

IListReorderHandler *post_blend_ctrl_order_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimStatesData> states,
  PropPanel::ContainerPropertyControl *panel)
{
  return new PostBlendCtrlOrderReorderHandler(plugin, states, panel);
}

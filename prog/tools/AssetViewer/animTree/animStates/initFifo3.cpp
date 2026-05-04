// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "initFifo3.h"
#include "animStatesType.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../controllers/ctrlType.h"
#include "../controllers/animCtrlData.h"
#include "../animTree.h"

#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>
#include <ioSys/dag_dataBlockCommentsDef.h>

enum
{
  FIFO3_NAMES,
  VALUE_NAMES,
};

dag::Vector<Tab<String>> init_fifo3_prepare_combo_values(PropPanel::ContainerPropertyControl *panel,
  dag::ConstSpan<BlendNodeData> blend_nodes, dag::ConstSpan<AnimCtrlData> controllers)
{
  Tab<String> fifo3Names;
  fifo3Names.emplace_back("##");
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  for (const AnimCtrlData &data : controllers)
  {
    if (data.type == ctrl_type_fifo3)
      fifo3Names.emplace_back(ctrlsTree->getCaption(data.handle));
  }
  Tab<String> valueNames;
  valueNames.emplace_back("##");
  valueNames.emplace_back("default");
  fill_child_names(valueNames, panel, blend_nodes, controllers);
  return {eastl::move(fifo3Names), eastl::move(valueNames)};
}

void init_fifo3_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings,
  dag::Vector<Tab<String>> &combo_names)
{
  Tab<String> names;
  fill_param_names_without_comments(names, settings);

  const char *defaultName = settings.paramCount() ? settings.getParamName(0) : "";
  const char *defaultValue = settings.paramCount() ? settings.getStr(0) : "";
  panel->createList(PID_STATES_NODES_LIST, "fifo3", names, 0);
  panel->createButton(PID_STATES_NODES_LIST_ADD, "Add");
  panel->createButton(PID_STATES_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  int selectedFifo3 = get_selected_name_idx_combo(combo_names[FIFO3_NAMES], defaultName);
  panel->createCombo(PID_STATES_FIFO3_INIT_NAME, "Fifo3 name", combo_names[FIFO3_NAMES], selectedFifo3);
  int selectedValue = get_selected_name_idx_combo(combo_names[VALUE_NAMES], defaultValue);
  panel->createCombo(PID_STATES_FIFO3_INIT_VALUE, "Value", combo_names[VALUE_NAMES], selectedValue);
  if (names.empty())
    for (int i = PID_STATES_FIFO3_INIT_NAME; i <= PID_STATES_FIFO3_INIT_VALUE; ++i)
      panel->setEnabledById(i, /*enabled*/ false);
}

void init_fifo3_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings)
{
  const SimpleString listName = panel->getText(PID_STATES_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_STATES_NODES_LIST);
  // Skip commented fields
  int paramIdx = get_param_name_idx_by_list_name(settings, listName, selectedIdx);
  panel->setText(PID_STATES_FIFO3_INIT_NAME, paramIdx != -1 ? settings.getParamName(paramIdx) : "");
  panel->setText(PID_STATES_FIFO3_INIT_VALUE, paramIdx != -1 ? settings.getStr(paramIdx) : "");
}

void init_fifo3_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock &settings)
{
  const SimpleString listName = panel->getText(PID_STATES_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_STATES_NODES_LIST);
  // Skip commented fields
  int paramIdx = get_param_name_idx_by_list_name(settings, listName, selectedIdx);
  if (paramIdx != -1)
    settings.removeParam(paramIdx);
}

void init_fifo3_update_child_name(DataBlock &settings, const char *name, const String &old_name)
{
  for (int i = 0; i < settings.paramCount(); ++i)
  {
    if (!CHECK_COMMENT_PREFIX(settings.getParamName(i))) // ignore params with comments
    {
      if (settings.getParamName(i) == old_name)
        settings.changeParamName(i, name);
      else if (settings.getStr(i) == old_name)
        settings.setStr(i, name);
    }
  }
}

void AnimTreePlugin::initFifo3SaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock &settings, AnimStatesData &data)
{
  const SimpleString listName = panel->getText(PID_STATES_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_STATES_NODES_LIST);
  const SimpleString fifo3Name = get_text_from_combo(panel, PID_STATES_FIFO3_INIT_NAME);
  const SimpleString fifo3Value = get_text_from_combo(panel, PID_STATES_FIFO3_INIT_VALUE);
  // Skip commented fields
  int paramIdx = get_param_name_idx_by_list_name(settings, listName, selectedIdx);
  if (paramIdx == -1 || (fifo3Name != settings.getParamName(paramIdx)))
  {
    data.childs[selectedIdx * 2] = find_child_idx_by_name(panel, data.handle, controllersData, blendNodesData, fifo3Name);
    check_fifo3_ctrl_child_idx(data.childs[selectedIdx * 2], fifo3Name);
  }
  if (paramIdx == -1 || (fifo3Value != settings.getStr(paramIdx)))
  {
    data.childs[selectedIdx * 2 + 1] =
      find_init_fifo3_child_idx_by_name(panel, data.handle, controllersData, blendNodesData, fifo3Value);
    check_init_anim_state_child_idx(data.childs[selectedIdx * 2 + 1], "fifo3", fifo3Value);
  }
  if (paramIdx != -1)
  {
    settings.changeParamName(paramIdx, fifo3Name);
    settings.setStr(fifo3Name, fifo3Value);
  }
  else
    settings.addStr(fifo3Name, fifo3Value);

  if (listName != fifo3Name)
    panel->setText(PID_STATES_NODES_LIST, fifo3Name);
}

class InitFifo3ReorderHandler : public BaseAnimStatesReorderHandler
{
public:
  InitFifo3ReorderHandler(AnimTreePlugin &plugin, dag::ConstSpan<AnimStatesData> states, PropPanel::ContainerPropertyControl *panel) :
    BaseAnimStatesReorderHandler(plugin, states, panel)
  {}

protected:
  AnimStatesType getTargetAnimStateType() const override { return AnimStatesType::INIT_ANIM_STATE; }

  virtual void handleSpecificReorder(DataBlock &props, int from, int to) override
  {
    DataBlock *settings = props.addBlock("initAnimState")->addBlock("fifo3");
    move_param_blk(*settings, from, to);
  }
};

IListReorderHandler *init_fifo3_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimStatesData> states,
  PropPanel::ContainerPropertyControl *panel)
{
  return new InitFifo3ReorderHandler(plugin, states, panel);
}

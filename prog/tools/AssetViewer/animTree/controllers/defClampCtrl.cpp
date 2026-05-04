// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "defClampCtrl.h"
#include "../animTreeUtils.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

void def_clamp_ctrl_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
}

void def_clamp_ctrl_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> names;
  const int attachNid = settings->getNameId("paramContr");
  for (int i = 0; i < settings->blockCount(); ++i)
    if (settings->getBlock(i)->getBlockNameId() == attachNid)
      names.emplace_back(settings->getBlock(i)->getStr("param", ""));

  bool isEditable = !names.empty();
  const DataBlock *defaultBlock = settings->getBlockByNameEx("paramContr");
  panel->createList(PID_CTRLS_NODES_LIST, "Params", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add param");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_DEF_CLAMP_CTRL_PARAM, "param", defaultBlock->getStr("param", ""), isEditable);
  panel->createEditFloat(PID_CTRLS_DEF_CLAMP_CTRL_DEFAULT, "default", defaultBlock->getReal("default", 0.f), /*prec*/ 2, isEditable);
  panel->createPoint2(PID_CTRLS_DEF_CLAMP_CTRL_CLAMP, "clamp", defaultBlock->getPoint2("clamp", Point2::ZERO), /*prec*/ 2, isEditable);
}

void def_clamp_ctrl_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  DataBlock *selectedBlock = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *child = settings->getBlock(i);
    if (listName == child->getStr("param", nullptr))
      selectedBlock = child;
  }
  const SimpleString paramValue = panel->getText(PID_CTRLS_DEF_CLAMP_CTRL_PARAM);
  const float defaultValue = panel->getFloat(PID_CTRLS_DEF_CLAMP_CTRL_DEFAULT);
  const Point2 clampValue = panel->getPoint2(PID_CTRLS_DEF_CLAMP_CTRL_CLAMP);
  if (!selectedBlock)
    selectedBlock = settings->addNewBlock("paramContr");

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  selectedBlock->setStr("param", paramValue.c_str());
  if (float_nonzero(defaultValue))
    selectedBlock->setReal("default", defaultValue);
  if (clampValue != Point2::ZERO)
    selectedBlock->setPoint2("clamp", clampValue);

  if (listName != paramValue)
    panel->setText(PID_CTRLS_NODES_LIST, paramValue.c_str());
}

void def_clamp_ctrl_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (name == child->getStr("param", nullptr))
      selectedBlock = child;
  }
  panel->setText(PID_CTRLS_DEF_CLAMP_CTRL_PARAM, name);
  panel->setFloat(PID_CTRLS_DEF_CLAMP_CTRL_DEFAULT, selectedBlock->getReal("default", 0.f));
  panel->setPoint2(PID_CTRLS_DEF_CLAMP_CTRL_CLAMP, selectedBlock->getPoint2("clamp", Point2::ZERO));
  bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  for (int i = PID_CTRLS_DEF_CLAMP_CTRL_PARAM; i <= PID_CTRLS_DEF_CLAMP_CTRL_CLAMP; ++i)
    panel->setEnabledById(i, isEditable);
}

void def_clamp_ctrl_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  for (int i = 0; i < settings->blockCount(); ++i)
    if (removeName == settings->getBlock(i)->getStr("param", nullptr))
      settings->removeBlock(i);
}

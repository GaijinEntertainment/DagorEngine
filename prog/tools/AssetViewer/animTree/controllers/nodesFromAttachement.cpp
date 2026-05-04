// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodesFromAttachement.h"
#include "../animTreeUtils.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

const bool DEFAULT_RECURSIVE = false;
const bool DEFUALT_INCLUDING_ROOT = true;

void nodes_from_attachement_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "slot");
  add_edit_box_if_not_exists(params, panel, field_idx, "varSlot");
  add_edit_bool_if_not_exists(params, panel, field_idx, "copyWtm");
  add_edit_box_if_not_exists(params, panel, field_idx, "wtModulate");
  add_edit_bool_if_not_exists(params, panel, field_idx, "wtModulateInverse");
}

void nodes_from_attachement_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "slot");
  remove_param_if_default_str(params, panel, "varSlot");
  remove_param_if_default_bool(params, panel, "copyWtm");
  remove_param_if_default_str(params, panel, "wtModulate");
  remove_param_if_default_bool(params, panel, "wtModulateInverse");
}

void nodes_from_attachement_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> names;
  const int nodeNid = settings->getNameId("node");
  for (int i = 0; i < settings->blockCount(); ++i)
    if (settings->getBlock(i)->getBlockNameId() == nodeNid)
      names.emplace_back(settings->getBlock(i)->getStr("srcNode", ""));

  bool isEditable = !names.empty();
  const DataBlock *defaultBlock = settings->getBlockByNameEx("node");
  panel->createList(PID_CTRLS_NODES_LIST, "Nodes", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add node");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_NODES_FROM_ATTACHEMENT_SRC_NODE, "srcNode", defaultBlock->getStr("srcNode", ""), isEditable);
  panel->createEditBox(PID_CTRLS_NODES_FROM_ATTACHEMENT_DEST_NODE, "destNode", defaultBlock->getStr("destNode", ""), isEditable);
  panel->createCheckBox(PID_CTRLS_NODES_FROM_ATTACHEMENT_RECURSIVE, "recursive", defaultBlock->getBool("recursive", DEFAULT_RECURSIVE),
    isEditable);
  panel->createCheckBox(PID_CTRLS_NODES_FROM_ATTACHEMENT_INCLUDING_ROOT, "includingRoot",
    defaultBlock->getBool("includingRoot", DEFUALT_INCLUDING_ROOT), isEditable);
}

void nodes_from_attachement_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  DataBlock *selectedBlock = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *child = settings->getBlock(i);
    if (listName == child->getStr("srcNode", nullptr))
      selectedBlock = child;
  }
  const SimpleString srcNodeValue = panel->getText(PID_CTRLS_NODES_FROM_ATTACHEMENT_SRC_NODE);
  const SimpleString destNodeValue = panel->getText(PID_CTRLS_NODES_FROM_ATTACHEMENT_DEST_NODE);
  const bool recursiveValue = panel->getBool(PID_CTRLS_NODES_FROM_ATTACHEMENT_RECURSIVE);
  const bool includingRootValue = panel->getBool(PID_CTRLS_NODES_FROM_ATTACHEMENT_INCLUDING_ROOT);
  if (!selectedBlock)
    selectedBlock = settings->addNewBlock("node");

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  selectedBlock->setStr("srcNode", srcNodeValue.c_str());
  selectedBlock->setStr("destNode", destNodeValue.c_str());
  if (recursiveValue != DEFAULT_RECURSIVE)
    selectedBlock->setBool("recursive", recursiveValue);
  if (includingRootValue != DEFUALT_INCLUDING_ROOT)
    selectedBlock->setBool("includingRoot", includingRootValue);

  if (listName != srcNodeValue)
    panel->setText(PID_CTRLS_NODES_LIST, srcNodeValue.c_str());
}

void nodes_from_attachement_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (name == child->getStr("srcNode", nullptr))
      selectedBlock = child;
  }
  panel->setText(PID_CTRLS_NODES_FROM_ATTACHEMENT_SRC_NODE, name);
  panel->setText(PID_CTRLS_NODES_FROM_ATTACHEMENT_DEST_NODE, selectedBlock->getStr("destNode", ""));
  panel->setBool(PID_CTRLS_NODES_FROM_ATTACHEMENT_RECURSIVE, selectedBlock->getBool("recursive", DEFAULT_RECURSIVE));
  panel->setBool(PID_CTRLS_NODES_FROM_ATTACHEMENT_INCLUDING_ROOT, selectedBlock->getBool("includingRoot", DEFUALT_INCLUDING_ROOT));
  bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  for (int i = PID_CTRLS_FOOT_LOCKER_IK_HIP; i <= PID_CTRLS_FOOT_LOCKER_IK_TOE; ++i)
    panel->setEnabledById(i, isEditable);
}

void nodes_from_attachement_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  for (int i = 0; i < settings->blockCount(); ++i)
    if (removeName == settings->getBlock(i)->getStr("srcNode", nullptr))
      settings->removeBlock(i);
}

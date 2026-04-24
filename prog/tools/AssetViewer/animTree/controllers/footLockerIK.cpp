// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "footLockerIK.h"
#include "../animTreeUtils.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

static const float DEFAULT_UNLOCK_VISCOSITY = 0.1f / M_LN2;
static const float DEFAULT_MAX_REACH_SCALE = 1.0f;
static const float DEFAULT_UNLOCK_RADIUS = 0.2f;
static const float DEFAULT_UNLOCK_WHEN_UNREACHABLE_RADIUS = 0.05f;
static const float DEFAULT_TOE_NODE_HEIGHT = 0.025f;
static const float DEFAULT_ANKLE_NODE_HEIGHT = 0.13f;
static const float DEFAULT_MAX_FOOT_UP = 0.5f;
static const float DEFAULT_MAX_FOOT_DOWN = 0.15f;
static const float DEFAULT_MAX_TOE_MOVE_UP = 0.1f;
static const float DEFAULT_MAX_TOE_MOVE_DOWN = 0.15f;
static const float DEFAULT_FOOT_RAISING_SPEED = 2.5f;
static const float DEFAULT_FOOT_INCLINE_VISCOSITY = 0.15f;
static const float DEFAULT_MAX_ANKLE_ANGLE = 60.0f;
static const float DEFAULT_HIP_MOVE_VISCOSITY = 0.1f;
static const float DEFAULT_MAX_HIP_MOVE_DOWN = 0.15f;

void foot_locker_ik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_float_if_not_exists(params, panel, field_idx, "unlockViscosity", DEFAULT_UNLOCK_VISCOSITY);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxReachScale", DEFAULT_MAX_REACH_SCALE);
  add_edit_float_if_not_exists(params, panel, field_idx, "unlockRadius", DEFAULT_UNLOCK_RADIUS);
  add_edit_float_if_not_exists(params, panel, field_idx, "unlockWhenUnreachableRadius", DEFAULT_UNLOCK_WHEN_UNREACHABLE_RADIUS);
  add_edit_float_if_not_exists(params, panel, field_idx, "toeNodeHeight", DEFAULT_TOE_NODE_HEIGHT);
  add_edit_float_if_not_exists(params, panel, field_idx, "ankleNodeHeight", DEFAULT_ANKLE_NODE_HEIGHT);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxFootUp", DEFAULT_MAX_FOOT_UP);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxFootDown", DEFAULT_MAX_FOOT_DOWN);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxToeMoveUp", DEFAULT_MAX_TOE_MOVE_UP);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxToeMoveDown", DEFAULT_MAX_TOE_MOVE_DOWN);
  add_edit_float_if_not_exists(params, panel, field_idx, "footRaisingSpeed", DEFAULT_FOOT_RAISING_SPEED);
  add_edit_float_if_not_exists(params, panel, field_idx, "footInclineViscosity", DEFAULT_FOOT_INCLINE_VISCOSITY);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxAnkleAnlge", DEFAULT_MAX_ANKLE_ANGLE);
  add_edit_float_if_not_exists(params, panel, field_idx, "hipMoveViscosity", DEFAULT_HIP_MOVE_VISCOSITY);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxHipMoveDown", DEFAULT_MAX_HIP_MOVE_DOWN);
  add_edit_box_if_not_exists(params, panel, field_idx, "hipMoveParamName");
}

void foot_locker_ik_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_float(params, panel, "unlockViscosity", DEFAULT_UNLOCK_VISCOSITY);
  remove_param_if_default_float(params, panel, "maxReachScale", DEFAULT_MAX_REACH_SCALE);
  remove_param_if_default_float(params, panel, "unlockRadius", DEFAULT_UNLOCK_RADIUS);
  remove_param_if_default_float(params, panel, "unlockWhenUnreachableRadius", DEFAULT_UNLOCK_WHEN_UNREACHABLE_RADIUS);
  remove_param_if_default_float(params, panel, "toeNodeHeight", DEFAULT_TOE_NODE_HEIGHT);
  remove_param_if_default_float(params, panel, "ankleNodeHeight", DEFAULT_ANKLE_NODE_HEIGHT);
  remove_param_if_default_float(params, panel, "maxFootUp", DEFAULT_MAX_FOOT_UP);
  remove_param_if_default_float(params, panel, "maxFootDown", DEFAULT_MAX_FOOT_DOWN);
  remove_param_if_default_float(params, panel, "maxToeMoveUp", DEFAULT_MAX_TOE_MOVE_UP);
  remove_param_if_default_float(params, panel, "maxToeMoveDown", DEFAULT_MAX_TOE_MOVE_DOWN);
  remove_param_if_default_float(params, panel, "footRaisingSpeed", DEFAULT_FOOT_RAISING_SPEED);
  remove_param_if_default_float(params, panel, "footInclineViscosity", DEFAULT_FOOT_INCLINE_VISCOSITY);
  remove_param_if_default_float(params, panel, "maxAnkleAnlge", DEFAULT_MAX_ANKLE_ANGLE);
  remove_param_if_default_float(params, panel, "hipMoveViscosity", DEFAULT_HIP_MOVE_VISCOSITY);
  remove_param_if_default_float(params, panel, "maxHipMoveDown", DEFAULT_MAX_HIP_MOVE_DOWN);
  remove_param_if_default_str(params, panel, "hipMoveParamName");
}

void foot_locker_ik_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> legNames;
  const int leg_nid = settings->getNameId("leg");
  for (int i = 0; i < settings->blockCount(); ++i)
    if (settings->getBlock(i)->getBlockNameId() == leg_nid)
      legNames.emplace_back(settings->getBlock(i)->getStr("hip", ""));

  bool isEditable = !legNames.empty();
  const DataBlock *defaultBlock = settings->getBlockByNameEx("leg");
  panel->createList(PID_CTRLS_NODES_LIST, "Legs", legNames, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add leg");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_FOOT_LOCKER_IK_HIP, "hip", defaultBlock->getStr("hip", ""), isEditable);
  panel->createEditBox(PID_CTRLS_FOOT_LOCKER_IK_KNEE, "knee", defaultBlock->getStr("knee", ""), isEditable);
  panel->createEditBox(PID_CTRLS_FOOT_LOCKER_IK_ANKLE, "ankle", defaultBlock->getStr("ankle", ""), isEditable);
  panel->createEditBox(PID_CTRLS_FOOT_LOCKER_IK_TOE, "toe", defaultBlock->getStr("toe", ""), isEditable);
}

void foot_locker_ik_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  DataBlock *selectedBlock = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *child = settings->getBlock(i);
    if (listName == child->getStr("hip", nullptr))
      selectedBlock = child;
  }
  const SimpleString hipValue = panel->getText(PID_CTRLS_FOOT_LOCKER_IK_HIP);
  const SimpleString kneeValue = panel->getText(PID_CTRLS_FOOT_LOCKER_IK_KNEE);
  const SimpleString ankleValue = panel->getText(PID_CTRLS_FOOT_LOCKER_IK_ANKLE);
  const SimpleString toeValue = panel->getText(PID_CTRLS_FOOT_LOCKER_IK_TOE);
  if (!selectedBlock)
    selectedBlock = settings->addNewBlock("leg");

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  selectedBlock->setStr("hip", hipValue.c_str());
  selectedBlock->setStr("knee", kneeValue.c_str());
  selectedBlock->setStr("ankle", ankleValue.c_str());
  selectedBlock->setStr("toe", toeValue.c_str());

  if (listName != hipValue)
    panel->setText(PID_CTRLS_NODES_LIST, hipValue.c_str());
}

void foot_locker_ik_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (name == child->getStr("hip", nullptr))
      selectedBlock = child;
  }
  panel->setText(PID_CTRLS_FOOT_LOCKER_IK_HIP, name);
  panel->setText(PID_CTRLS_FOOT_LOCKER_IK_KNEE, selectedBlock->getStr("knee", ""));
  panel->setText(PID_CTRLS_FOOT_LOCKER_IK_ANKLE, selectedBlock->getStr("ankle", ""));
  panel->setText(PID_CTRLS_FOOT_LOCKER_IK_TOE, selectedBlock->getStr("toe", ""));
  bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  for (int i = PID_CTRLS_FOOT_LOCKER_IK_HIP; i <= PID_CTRLS_FOOT_LOCKER_IK_TOE; ++i)
    panel->setEnabledById(i, isEditable);
}

void foot_locker_ik_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  for (int i = 0; i < settings->blockCount(); ++i)
    if (removeName == settings->getBlock(i)->getStr("hip", nullptr))
      settings->removeBlock(i);
}

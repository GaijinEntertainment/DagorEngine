// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "twoBonesIK.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

static const float DEFAULT_MAX_REACH_SCALE = 1.0f;
static const Point3 DEFAULT_FLEX_DIRECTION = Point3(0.f, 0.f, 0.f);

void two_bones_ik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
}

void two_bones_ik_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> boneNames;
  const int bonesNid = settings->getNameId("bones");
  for (int i = 0; i < settings->blockCount(); ++i)
    if (settings->getBlock(i)->getBlockNameId() == bonesNid)
      boneNames.emplace_back(settings->getBlock(i)->getStr("start", ""));

  const bool isEditable = !boneNames.empty();
  const DataBlock *selectedBlock = settings->getBlockByNameEx("bones");

  panel->createList(PID_CTRLS_NODES_LIST, "Bones", boneNames, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add bones");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_TWO_BONES_IK_START, "start", selectedBlock->getStr("start", ""), isEditable);
  panel->createEditBox(PID_CTRLS_TWO_BONES_IK_MIDDLE, "middle", selectedBlock->getStr("middle", ""), isEditable);
  panel->createEditBox(PID_CTRLS_TWO_BONES_IK_END, "end", selectedBlock->getStr("end", ""), isEditable);
  panel->createEditFloat(PID_CTRLS_TWO_BONES_IK_MAX_REACH_SCALE, "maxReachScale",
    selectedBlock->getReal("maxReachScale", DEFAULT_MAX_REACH_SCALE), /*prec*/ 2, isEditable);
  panel->createCheckBox(PID_CTRLS_TWO_BONES_IK_REVERSE_FLEX_DIRECTION, "reverseFlexDirection",
    selectedBlock->getBool("reverseFlexDirection", false), isEditable);
  panel->createCheckBox(PID_CTRLS_TWO_BONES_IK_FORCE_REACH_TARGET, "forceReachTarget",
    selectedBlock->getBool("forceReachTarget", false), isEditable);
  panel->createPoint3(PID_CTRLS_TWO_BONES_IK_FLEX_DIRECTION, "flexDirection",
    selectedBlock->getPoint3("flexDirection", DEFAULT_FLEX_DIRECTION), /*prec*/ 2, isEditable);
  panel->createEditBox(PID_CTRLS_TWO_BONES_IK_FLEX_LOCAL_TO_NODE, "flexLocalToNode", selectedBlock->getStr("flexLocalToNode", ""),
    isEditable);
}

void two_bones_ik_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  DataBlock *selectedBlock = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *child = settings->getBlock(i);
    if (listName == child->getStr("start", nullptr))
      selectedBlock = child;
  }

  const SimpleString startValue = panel->getText(PID_CTRLS_TWO_BONES_IK_START);
  const SimpleString middleValue = panel->getText(PID_CTRLS_TWO_BONES_IK_MIDDLE);
  const SimpleString endValue = panel->getText(PID_CTRLS_TWO_BONES_IK_END);
  const float maxReachScaleValue = panel->getFloat(PID_CTRLS_TWO_BONES_IK_MAX_REACH_SCALE);
  const bool reverseFlexDirectionValue = panel->getBool(PID_CTRLS_TWO_BONES_IK_REVERSE_FLEX_DIRECTION);
  const bool forceReachTargetValue = panel->getBool(PID_CTRLS_TWO_BONES_IK_FORCE_REACH_TARGET);
  const Point3 flexDirectionValue = panel->getPoint3(PID_CTRLS_TWO_BONES_IK_FLEX_DIRECTION);
  const SimpleString flexLocalToNodeValue = panel->getText(PID_CTRLS_TWO_BONES_IK_FLEX_LOCAL_TO_NODE);

  if (!selectedBlock)
    selectedBlock = settings->addNewBlock("bones");

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  selectedBlock->setStr("start", startValue.c_str());
  selectedBlock->setStr("middle", middleValue.c_str());
  selectedBlock->setStr("end", endValue.c_str());
  if (!is_equal_float(maxReachScaleValue, DEFAULT_MAX_REACH_SCALE))
    selectedBlock->setReal("maxReachScale", maxReachScaleValue);
  if (reverseFlexDirectionValue)
    selectedBlock->setBool("reverseFlexDirection", reverseFlexDirectionValue);
  if (forceReachTargetValue)
    selectedBlock->setBool("forceReachTarget", forceReachTargetValue);
  if (flexDirectionValue.lengthSq() > 0.f)
    selectedBlock->setPoint3("flexDirection", flexDirectionValue);
  if (!flexLocalToNodeValue.empty())
    selectedBlock->setStr("flexLocalToNode", flexLocalToNodeValue.c_str());

  if (listName != startValue)
    panel->setText(PID_CTRLS_NODES_LIST, startValue.c_str());
}

void two_bones_ik_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (name == child->getStr("start", nullptr))
      selectedBlock = child;
  }

  panel->setText(PID_CTRLS_TWO_BONES_IK_START, selectedBlock->getStr("start", ""));
  panel->setText(PID_CTRLS_TWO_BONES_IK_MIDDLE, selectedBlock->getStr("middle", ""));
  panel->setText(PID_CTRLS_TWO_BONES_IK_END, selectedBlock->getStr("end", ""));
  panel->setFloat(PID_CTRLS_TWO_BONES_IK_MAX_REACH_SCALE, selectedBlock->getReal("maxReachScale", DEFAULT_MAX_REACH_SCALE));
  panel->setBool(PID_CTRLS_TWO_BONES_IK_REVERSE_FLEX_DIRECTION, selectedBlock->getBool("reverseFlexDirection", false));
  panel->setBool(PID_CTRLS_TWO_BONES_IK_FORCE_REACH_TARGET, selectedBlock->getBool("forceReachTarget", false));
  panel->setPoint3(PID_CTRLS_TWO_BONES_IK_FLEX_DIRECTION, selectedBlock->getPoint3("flexDirection", DEFAULT_FLEX_DIRECTION));
  panel->setText(PID_CTRLS_TWO_BONES_IK_FLEX_LOCAL_TO_NODE, selectedBlock->getStr("flexLocalToNode", ""));

  const bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  for (int i = PID_CTRLS_TWO_BONES_IK_START; i <= PID_CTRLS_TWO_BONES_IK_FLEX_LOCAL_TO_NODE; ++i)
    panel->setEnabledById(i, isEditable);
}

void two_bones_ik_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  for (int i = 0; i < settings->blockCount(); ++i)
    if (removeName == settings->getBlock(i)->getStr("start", nullptr))
    {
      settings->removeBlock(i);
      return;
    }
}

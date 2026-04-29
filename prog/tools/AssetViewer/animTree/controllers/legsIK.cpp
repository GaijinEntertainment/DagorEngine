// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "legsIK.h"
#include "../animTreeUtils.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

const Point3 DEFAULT_CRAWL_KNEE_OFFSET_VEC = Point3(0.0f, 0.2f, 0.1f);
const float DEFAULT_CRAWL_FOOT_OFFSET = 0.15f;
const float DEFAULT_CRAWL_FOOT_ANGLE = 130.0f;
const float DEFAULT_CRAWL_MAX_RAY = 0.5f;
const Point2 DEFAULT_FOOT_STEP_RANGE = Point2(0.0f, 0.1f);
const Point2 DEFAULT_FOOT_ROT_RANGE = Point2(-30.f, 30.f);
const float DEFAULT_MAX_FOOT_UP = 0.5f;
const Point3 DEFAULT_IK_FOOT_FWD = Point3(1.f, 0.f, 0.f);
const Point3 DEFAULT_IK_FOOT_SIDE = Point3(1.f, 0.f, 0.f);
const float DEFAULT_MAX_DY_RATE = 0.9f;
const float DEFAULT_MAX_DA_RATE = 60.f;

void legs_ik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_bool_if_not_exists(params, panel, field_idx, "alwaysSolve");
  add_edit_bool_if_not_exists(params, panel, field_idx, "isCrawl");
  add_edit_point3_if_not_exists(params, panel, field_idx, "crawlKneeOffsetVec", DEFAULT_CRAWL_KNEE_OFFSET_VEC);
  add_edit_float_if_not_exists(params, panel, field_idx, "crawlFootOffset", DEFAULT_CRAWL_FOOT_OFFSET);
  add_edit_float_if_not_exists(params, panel, field_idx, "crawlFootAngle", DEFAULT_CRAWL_FOOT_ANGLE);
  add_edit_float_if_not_exists(params, panel, field_idx, "crawlMaxRay", DEFAULT_CRAWL_MAX_RAY);
}

void legs_ik_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_bool(params, panel, "alwaysSolve");
  remove_param_if_default_bool(params, panel, "isCrawl");
  remove_param_if_default_point3(params, panel, "crawlKneeOffsetVec", DEFAULT_CRAWL_KNEE_OFFSET_VEC);
  remove_param_if_default_float(params, panel, "crawlFootOffset", DEFAULT_CRAWL_FOOT_OFFSET);
  remove_param_if_default_float(params, panel, "crawlFootAngle", DEFAULT_CRAWL_FOOT_ANGLE);
  remove_param_if_default_float(params, panel, "crawlMaxRay", DEFAULT_CRAWL_MAX_RAY);
}

void legs_ik_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> blockNames{String("leg"), String("def")};
  Tab<String> legNames;
  const int legNid = settings->getNameId("leg");
  const int defNid = settings->getNameId("def");
  for (int i = 0; i < settings->blockCount(); ++i)
    if (settings->getBlock(i)->getBlockNameId() == legNid)
      legNames.emplace_back(settings->getBlock(i)->getStr("leg", ""));
    else if (settings->getBlock(i)->getBlockNameId() == defNid)
      legNames.emplace_back("def");

  bool isEditable = !legNames.empty();
  bool defaultBlockSelected = isEditable && legNames[0] == "def";
  bool isBlockTypeEditable = defaultBlockSelected || (isEditable && !settings->blockExists("def"));
  const DataBlock *defaultBlock = settings->getBlockByNameEx("def");
  const DataBlock *selectedBlock = settings->getBlockByNameEx("leg");
  panel->createList(PID_CTRLS_NODES_LIST, "Legs", legNames, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add leg");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createCombo(PID_CTRLS_LEGS_IK_BLOCK_TYPE, "Block type", blockNames, defaultBlockSelected ? 1 : 0, isBlockTypeEditable);
  panel->createEditBox(PID_CTRLS_LEGS_IK_LEG, "leg", selectedBlock->getStr("leg", defaultBlock->getStr("leg", "")), isEditable);
  panel->createEditBox(PID_CTRLS_LEGS_IK_KNEE, "knee", selectedBlock->getStr("knee", defaultBlock->getStr("knee", "")), isEditable);
  panel->createEditBox(PID_CTRLS_LEGS_IK_FOOT, "foot", selectedBlock->getStr("foot", defaultBlock->getStr("foot", "")), isEditable);
  panel->createEditBox(PID_CTRLS_LEGS_IK_FOOT_STEP, "foot_step",
    selectedBlock->getStr("foot_step", defaultBlock->getStr("foot_step", "")), isEditable);
  panel->createPoint2(PID_CTRLS_LEGS_IK_FOOT_STEP_RANGE, "footStepRange",
    selectedBlock->getPoint2("footStepRange", defaultBlock->getPoint2("footStepRange", DEFAULT_FOOT_STEP_RANGE)), /*prec*/ 2,
    isEditable);
  panel->createPoint2(PID_CTRLS_LEGS_IK_FOOT_ROT_RANGE, "footRotRange",
    selectedBlock->getPoint2("footRotRange", defaultBlock->getPoint2("footRotRange", DEFAULT_FOOT_ROT_RANGE)), /*prec*/ 2, isEditable);
  panel->createEditFloat(PID_CTRLS_LEGS_IK_MAX_FOOT_UP, "maxFootUp",
    selectedBlock->getReal("maxFootUp", defaultBlock->getReal("maxFootUp", DEFAULT_MAX_FOOT_UP)),
    /*prec*/ 2, isEditable);
  panel->createPoint3(PID_CTRLS_LEGS_IK_FOOT_FWD, "footFwd",
    selectedBlock->getPoint3("footFwd", defaultBlock->getPoint3("footFwd", DEFAULT_IK_FOOT_FWD)), /*prec*/ 2, isEditable);
  panel->createPoint3(PID_CTRLS_LEGS_IK_FOOT_SIDE, "footSide",
    selectedBlock->getPoint3("footSide", defaultBlock->getPoint3("footSide", DEFAULT_IK_FOOT_SIDE)), /*prec*/ 2, isEditable);
  panel->createEditFloat(PID_CTRLS_LEGS_IK_MAX_DY_RATE, "maxDyRate",
    selectedBlock->getReal("maxDyRate", defaultBlock->getReal("maxDyRate", DEFAULT_MAX_DY_RATE)),
    /*prec*/ 2, isEditable);
  panel->createEditFloat(PID_CTRLS_LEGS_IK_MAX_DA_RATE, "maxDaRate",
    selectedBlock->getReal("maxDaRate", defaultBlock->getReal("maxDaRate", DEFAULT_MAX_DA_RATE)),
    /*prec*/ 2, isEditable);
  panel->createCheckBox(PID_CTRLS_LEGS_IK_USE_ANIMCHAR_UP_DIR, "useAnimcharUpDir",
    selectedBlock->getBool("useAnimcharUpDir", defaultBlock->getBool("useAnimcharUpDir", false)), isEditable);
  panel->createCheckBox(PID_CTRLS_LEGS_IK_REVERSE_FLEX_DIRECTION, "reverseFlexDirection",
    selectedBlock->getBool("reverseFlexDirection", defaultBlock->getBool("reverseFlexDirection", false)), isEditable);
}

void legs_ik_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  const DataBlock *defaultBlock = settings->getBlockByNameEx("def");
  DataBlock *selectedBlock = nullptr;
  if (listName == "def")
    selectedBlock = settings->getBlockByName("def");
  else
  {
    for (int i = 0; i < settings->blockCount(); ++i)
    {
      DataBlock *child = settings->getBlock(i);
      if (listName == child->getStr("leg", nullptr))
        selectedBlock = child;
    }
  }

  const SimpleString blockTypeValue = panel->getText(PID_CTRLS_LEGS_IK_BLOCK_TYPE);
  const SimpleString legValue = panel->getText(PID_CTRLS_LEGS_IK_LEG);
  const SimpleString kneeValue = panel->getText(PID_CTRLS_LEGS_IK_KNEE);
  const SimpleString footValue = panel->getText(PID_CTRLS_LEGS_IK_FOOT);
  const SimpleString footStepValue = panel->getText(PID_CTRLS_LEGS_IK_FOOT_STEP);
  const Point2 footStepRangeValue = panel->getPoint2(PID_CTRLS_LEGS_IK_FOOT_STEP_RANGE);
  const Point2 footRotRangeValue = panel->getPoint2(PID_CTRLS_LEGS_IK_FOOT_ROT_RANGE);
  const float maxFootUpValue = panel->getFloat(PID_CTRLS_LEGS_IK_MAX_FOOT_UP);
  const Point3 footFwdValue = panel->getPoint3(PID_CTRLS_LEGS_IK_FOOT_FWD);
  const Point3 footSideValue = panel->getPoint3(PID_CTRLS_LEGS_IK_FOOT_SIDE);
  const float maxDyRateValue = panel->getFloat(PID_CTRLS_LEGS_IK_MAX_DY_RATE);
  const float maxDaRateValue = panel->getFloat(PID_CTRLS_LEGS_IK_MAX_DA_RATE);
  const bool useAnimcharUpDirValue = panel->getBool(PID_CTRLS_LEGS_IK_USE_ANIMCHAR_UP_DIR);
  const bool reverseFlexDirectionValue = panel->getBool(PID_CTRLS_LEGS_IK_REVERSE_FLEX_DIRECTION);

  if (legValue.empty() && blockTypeValue != "def")
  {
    logerr("Can't save empty <leg> field for not def block type");
    return;
  }

  if (!selectedBlock)
    selectedBlock = settings->addNewBlock(blockTypeValue);

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  if (legValue != defaultBlock->getStr("leg", ""))
    selectedBlock->setStr("leg", legValue.c_str());
  if (kneeValue != defaultBlock->getStr("knee", ""))
    selectedBlock->setStr("knee", kneeValue.c_str());
  if (footValue != defaultBlock->getStr("foot", ""))
    selectedBlock->setStr("foot", footValue.c_str());
  if (footStepValue != defaultBlock->getStr("foot_step", ""))
    selectedBlock->setStr("foot_step", footStepValue.c_str());
  if (footStepRangeValue != defaultBlock->getPoint2("footStepRange", DEFAULT_FOOT_STEP_RANGE))
    selectedBlock->setPoint2("footStepRange", footStepRangeValue);
  if (footRotRangeValue != defaultBlock->getPoint2("footRotRange", DEFAULT_FOOT_ROT_RANGE))
    selectedBlock->setPoint2("footRotRange", footRotRangeValue);
  if (maxFootUpValue != defaultBlock->getReal("maxFootUp", DEFAULT_MAX_FOOT_UP))
    selectedBlock->setReal("maxFootUp", maxFootUpValue);
  if (footFwdValue != defaultBlock->getPoint3("footFwd", DEFAULT_IK_FOOT_FWD))
    selectedBlock->setPoint3("footFwd", footFwdValue);
  if (footSideValue != defaultBlock->getPoint3("footSide", DEFAULT_IK_FOOT_SIDE))
    selectedBlock->setPoint3("footSide", footSideValue);
  if (maxDyRateValue != defaultBlock->getReal("maxDyRate", DEFAULT_MAX_DY_RATE))
    selectedBlock->setReal("maxDyRate", maxDyRateValue);
  if (maxDaRateValue != defaultBlock->getReal("maxDaRate", DEFAULT_MAX_DA_RATE))
    selectedBlock->setReal("maxDaRate", maxDaRateValue);
  if (useAnimcharUpDirValue != defaultBlock->getBool("useAnimcharUpDir", false))
    selectedBlock->setBool("useAnimcharUpDir", useAnimcharUpDirValue);
  if (reverseFlexDirectionValue != defaultBlock->getBool("reverseFlexDirection", false))
    selectedBlock->setBool("reverseFlexDirection", reverseFlexDirectionValue);

  if (listName != legValue || blockTypeValue != selectedBlock->getBlockName())
    panel->setText(PID_CTRLS_NODES_LIST, blockTypeValue == "def" ? "def" : legValue.c_str());

  if (blockTypeValue != selectedBlock->getBlockName())
    selectedBlock->changeBlockName(blockTypeValue);
}

void legs_ik_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  const DataBlock *defaultBlock = settings->getBlockByNameEx("def");
  if (name == "def")
  {
    selectedBlock = settings->getBlockByName("def");
    panel->setText(PID_CTRLS_LEGS_IK_BLOCK_TYPE, "def");
  }
  else
  {
    for (int i = 0; i < settings->blockCount(); ++i)
    {
      const DataBlock *child = settings->getBlock(i);
      if (name == child->getStr("leg", nullptr))
        selectedBlock = child;
    }
    panel->setText(PID_CTRLS_LEGS_IK_BLOCK_TYPE, "leg");
  }

  panel->setText(PID_CTRLS_LEGS_IK_LEG, selectedBlock->getStr("leg", defaultBlock->getStr("leg", "")));
  panel->setText(PID_CTRLS_LEGS_IK_KNEE, selectedBlock->getStr("knee", defaultBlock->getStr("knee", "")));
  panel->setText(PID_CTRLS_LEGS_IK_FOOT, selectedBlock->getStr("foot", defaultBlock->getStr("foot", "")));
  panel->setText(PID_CTRLS_LEGS_IK_FOOT_STEP, selectedBlock->getStr("foot_step", defaultBlock->getStr("foot_step", "")));
  panel->setPoint2(PID_CTRLS_LEGS_IK_FOOT_STEP_RANGE,
    selectedBlock->getPoint2("footStepRange", defaultBlock->getPoint2("footStepRange", DEFAULT_FOOT_STEP_RANGE)));
  panel->setPoint2(PID_CTRLS_LEGS_IK_FOOT_ROT_RANGE,
    selectedBlock->getPoint2("footRotRange", defaultBlock->getPoint2("footRotRange", DEFAULT_FOOT_ROT_RANGE)));
  panel->setFloat(PID_CTRLS_LEGS_IK_MAX_FOOT_UP,
    selectedBlock->getReal("maxFootUp", defaultBlock->getReal("maxFootUp", DEFAULT_MAX_FOOT_UP)));
  panel->setPoint3(PID_CTRLS_LEGS_IK_FOOT_FWD,
    selectedBlock->getPoint3("footFwd", defaultBlock->getPoint3("footFwd", DEFAULT_IK_FOOT_FWD)));
  panel->setPoint3(PID_CTRLS_LEGS_IK_FOOT_SIDE,
    selectedBlock->getPoint3("footSide", defaultBlock->getPoint3("footSide", DEFAULT_IK_FOOT_SIDE)));
  panel->setFloat(PID_CTRLS_LEGS_IK_MAX_DY_RATE,
    selectedBlock->getReal("maxDyRate", defaultBlock->getReal("maxDyRate", DEFAULT_MAX_DY_RATE)));
  panel->setFloat(PID_CTRLS_LEGS_IK_MAX_DA_RATE,
    selectedBlock->getReal("maxDaRate", defaultBlock->getReal("maxDaRate", DEFAULT_MAX_DA_RATE)));
  panel->setBool(PID_CTRLS_LEGS_IK_USE_ANIMCHAR_UP_DIR,
    selectedBlock->getBool("useAnimcharUpDir", defaultBlock->getBool("useAnimcharUpDir", false)));
  panel->setBool(PID_CTRLS_LEGS_IK_REVERSE_FLEX_DIRECTION,
    selectedBlock->getBool("reverseFlexDirection", defaultBlock->getBool("reverseFlexDirection", false)));

  bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  int defNid = settings->getNameId("def");
  bool isBlockTypeEditable = isEditable && (selectedBlock->getBlockNameId() == defNid || !settings->blockExists("def"));
  panel->setEnabledById(PID_CTRLS_LEGS_IK_BLOCK_TYPE, isBlockTypeEditable);
  for (int i = PID_CTRLS_LEGS_IK_LEG; i <= PID_CTRLS_LEGS_IK_REVERSE_FLEX_DIRECTION; ++i)
    panel->setEnabledById(i, isEditable);
}

void legs_ik_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  if (removeName == "def")
    settings->removeBlock("def");
  else
  {
    for (int i = 0; i < settings->blockCount(); ++i)
      if (removeName == settings->getBlock(i)->getStr("leg", nullptr))
        settings->removeBlock(i);
  }
}

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "legLocker.h"
#include "../animTreeUtils.h"
#include "../animTreeDragListHandler.h"

#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathBase.h>
#include <propPanel/control/container.h>

// Defaults mirror LegLockerNode in active_matter/prog/scripts/game/dasAnimBlendNodes/anim_pbc_leg_locker.das
static const char *DEFAULT_PELVIS_NODE = "Bip01 Pelvis";
static const char *DEFAULT_LEFT_STEPPING_PARAM = "left_leg_stepping";
static const char *DEFAULT_RIGHT_STEPPING_PARAM = "right_leg_stepping";
static const char *DEFAULT_IS_FPV_PARAM = "is_fpv";
static const float DEFAULT_REPLANT_RADIUS = 0.13f;
static const float DEFAULT_REPLANT_YAW_DEG = 20.0f;
static const float DEFAULT_UNLOCK_YAW_DEG = 50.0f;
static const float DEFAULT_LOCK_FADE_WT = 0.5f;
static const float DEFAULT_LOCK_RELEASE_TIME = 0.22f;
static const float DEFAULT_LOCK_ATTACK_TIME = 0.04f;
static const float DEFAULT_SETTLE_TRACK_TIME = 0.05f;
static const float DEFAULT_PLANT_ABOVE_GROUND = 0.05f;
static const float DEFAULT_STEP_HEIGHT = 0.025f;
static const float DEFAULT_STEP_SPEED_BASE = 4.0f;
static const float DEFAULT_STEP_DUR_SCALE_MAX = 1.0f;
static const float DEFAULT_MAX_REACH_SCALE = 0.96f;
static const float DEFAULT_MAX_HIP_MOVE_DOWN = 0.2f;
static const float DEFAULT_HIP_MOVE_VISCOSITY = 0.1f;
static const float DEFAULT_COM_SHIFT_FACTOR = 0.16f;
static const float DEFAULT_COM_SHIFT_MAX = 0.065f;
static const float DEFAULT_COM_SHIFT_TIME = 0.41f;
static const float DEFAULT_FOOT_LIFT = 0.02f;
static const float DEFAULT_TRACE_UP = 0.5f;
static const float DEFAULT_TRACE_DOWN = 1.5f;
static const float DEFAULT_TELEPORT_DIST = 1.0f;
static const float DEFAULT_TIPTOE_KNEE_COS = 0.45f;
static const float DEFAULT_TIPTOE_MAX_ANGLE_DEG = RadToDeg(0.9f);
static const float DEFAULT_TIPTOE_SMOOTH_TIME = 0.135f;
static const int MAX_LEG_COUNT = 2;

void leg_locker_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "pelvisNode", DEFAULT_PELVIS_NODE);
  add_edit_box_if_not_exists(params, panel, field_idx, "leftSteppingParam", DEFAULT_LEFT_STEPPING_PARAM);
  add_edit_box_if_not_exists(params, panel, field_idx, "rightSteppingParam", DEFAULT_RIGHT_STEPPING_PARAM);
  add_edit_box_if_not_exists(params, panel, field_idx, "isFpvParam", DEFAULT_IS_FPV_PARAM);
  add_edit_float_if_not_exists(params, panel, field_idx, "replantRadius", DEFAULT_REPLANT_RADIUS);
  add_edit_float_if_not_exists(params, panel, field_idx, "replantYawDeg", DEFAULT_REPLANT_YAW_DEG);
  add_edit_float_if_not_exists(params, panel, field_idx, "unlockYawDeg", DEFAULT_UNLOCK_YAW_DEG);
  add_edit_float_if_not_exists(params, panel, field_idx, "lockFadeWt", DEFAULT_LOCK_FADE_WT);
  add_edit_float_if_not_exists(params, panel, field_idx, "lockReleaseTime", DEFAULT_LOCK_RELEASE_TIME);
  add_edit_float_if_not_exists(params, panel, field_idx, "lockAttackTime", DEFAULT_LOCK_ATTACK_TIME);
  add_edit_float_if_not_exists(params, panel, field_idx, "settleTrackTime", DEFAULT_SETTLE_TRACK_TIME);
  add_edit_float_if_not_exists(params, panel, field_idx, "plantAboveGround", DEFAULT_PLANT_ABOVE_GROUND);
  add_edit_float_if_not_exists(params, panel, field_idx, "stepHeight", DEFAULT_STEP_HEIGHT);
  add_edit_float_if_not_exists(params, panel, field_idx, "stepSpeedBase", DEFAULT_STEP_SPEED_BASE);
  add_edit_float_if_not_exists(params, panel, field_idx, "stepDurScaleMax", DEFAULT_STEP_DUR_SCALE_MAX);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxReachScale", DEFAULT_MAX_REACH_SCALE);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxHipMoveDown", DEFAULT_MAX_HIP_MOVE_DOWN);
  add_edit_float_if_not_exists(params, panel, field_idx, "hipMoveViscosity", DEFAULT_HIP_MOVE_VISCOSITY);
  add_edit_float_if_not_exists(params, panel, field_idx, "comShiftFactor", DEFAULT_COM_SHIFT_FACTOR);
  add_edit_float_if_not_exists(params, panel, field_idx, "comShiftMax", DEFAULT_COM_SHIFT_MAX);
  add_edit_float_if_not_exists(params, panel, field_idx, "comShiftTime", DEFAULT_COM_SHIFT_TIME);
  add_edit_float_if_not_exists(params, panel, field_idx, "footLift", DEFAULT_FOOT_LIFT);
  add_edit_float_if_not_exists(params, panel, field_idx, "traceUp", DEFAULT_TRACE_UP);
  add_edit_float_if_not_exists(params, panel, field_idx, "traceDown", DEFAULT_TRACE_DOWN);
  add_edit_float_if_not_exists(params, panel, field_idx, "teleportDist", DEFAULT_TELEPORT_DIST);
  add_edit_float_if_not_exists(params, panel, field_idx, "tiptoeKneeCos", DEFAULT_TIPTOE_KNEE_COS);
  add_edit_float_if_not_exists(params, panel, field_idx, "tiptoeMaxAngleDeg", DEFAULT_TIPTOE_MAX_ANGLE_DEG);
  add_edit_float_if_not_exists(params, panel, field_idx, "tiptoeSmoothTime", DEFAULT_TIPTOE_SMOOTH_TIME);
}

void leg_locker_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "pelvisNode", DEFAULT_PELVIS_NODE);
  remove_param_if_default_str(params, panel, "leftSteppingParam", DEFAULT_LEFT_STEPPING_PARAM);
  remove_param_if_default_str(params, panel, "rightSteppingParam", DEFAULT_RIGHT_STEPPING_PARAM);
  remove_param_if_default_str(params, panel, "isFpvParam", DEFAULT_IS_FPV_PARAM);
  remove_param_if_default_float(params, panel, "replantRadius", DEFAULT_REPLANT_RADIUS);
  remove_param_if_default_float(params, panel, "replantYawDeg", DEFAULT_REPLANT_YAW_DEG);
  remove_param_if_default_float(params, panel, "unlockYawDeg", DEFAULT_UNLOCK_YAW_DEG);
  remove_param_if_default_float(params, panel, "lockFadeWt", DEFAULT_LOCK_FADE_WT);
  remove_param_if_default_float(params, panel, "lockReleaseTime", DEFAULT_LOCK_RELEASE_TIME);
  remove_param_if_default_float(params, panel, "lockAttackTime", DEFAULT_LOCK_ATTACK_TIME);
  remove_param_if_default_float(params, panel, "settleTrackTime", DEFAULT_SETTLE_TRACK_TIME);
  remove_param_if_default_float(params, panel, "plantAboveGround", DEFAULT_PLANT_ABOVE_GROUND);
  remove_param_if_default_float(params, panel, "stepHeight", DEFAULT_STEP_HEIGHT);
  remove_param_if_default_float(params, panel, "stepSpeedBase", DEFAULT_STEP_SPEED_BASE);
  remove_param_if_default_float(params, panel, "stepDurScaleMax", DEFAULT_STEP_DUR_SCALE_MAX);
  remove_param_if_default_float(params, panel, "maxReachScale", DEFAULT_MAX_REACH_SCALE);
  remove_param_if_default_float(params, panel, "maxHipMoveDown", DEFAULT_MAX_HIP_MOVE_DOWN);
  remove_param_if_default_float(params, panel, "hipMoveViscosity", DEFAULT_HIP_MOVE_VISCOSITY);
  remove_param_if_default_float(params, panel, "comShiftFactor", DEFAULT_COM_SHIFT_FACTOR);
  remove_param_if_default_float(params, panel, "comShiftMax", DEFAULT_COM_SHIFT_MAX);
  remove_param_if_default_float(params, panel, "comShiftTime", DEFAULT_COM_SHIFT_TIME);
  remove_param_if_default_float(params, panel, "footLift", DEFAULT_FOOT_LIFT);
  remove_param_if_default_float(params, panel, "traceUp", DEFAULT_TRACE_UP);
  remove_param_if_default_float(params, panel, "traceDown", DEFAULT_TRACE_DOWN);
  remove_param_if_default_float(params, panel, "teleportDist", DEFAULT_TELEPORT_DIST);
  remove_param_if_default_float(params, panel, "tiptoeKneeCos", DEFAULT_TIPTOE_KNEE_COS);
  remove_param_if_default_float(params, panel, "tiptoeMaxAngleDeg", DEFAULT_TIPTOE_MAX_ANGLE_DEG);
  remove_param_if_default_float(params, panel, "tiptoeSmoothTime", DEFAULT_TIPTOE_SMOOTH_TIME);
}

void leg_locker_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> legNames;
  for (int position : collect_block_positions_by_name(*settings, "leg"))
    legNames.emplace_back(settings->getBlock(position)->getStr("hip", ""));

  panel->createList(PID_CTRLS_NODES_LIST, "Legs", legNames, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add leg");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_LEG_LOCKER_HIP, "hip");
  panel->createEditBox(PID_CTRLS_LEG_LOCKER_KNEE, "knee");
  panel->createEditBox(PID_CTRLS_LEG_LOCKER_ANKLE, "ankle");
  panel->createEditBox(PID_CTRLS_LEG_LOCKER_TOE, "toe");
  leg_locker_set_selected_node_list_settings(panel, settings);
}

void leg_locker_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (selectedIdx < 0)
    return;

  // By index, as Remove does: two legs can share a hip. An index past the leg blocks is a new entry from Add.
  const dag::Vector<int> positions = collect_block_positions_by_name(*settings, "leg");
  if (selectedIdx > positions.size())
  {
    logerr("leg_locker <%s>: can't save new leg, save the new leg above it first", settings->getStr("name", ""));
    return;
  }
  DataBlock *selectedBlock = selectedIdx < positions.size() ? settings->getBlock(positions[selectedIdx]) : nullptr;
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  const SimpleString hipValue = panel->getText(PID_CTRLS_LEG_LOCKER_HIP);
  const SimpleString kneeValue = panel->getText(PID_CTRLS_LEG_LOCKER_KNEE);
  const SimpleString ankleValue = panel->getText(PID_CTRLS_LEG_LOCKER_ANKLE);
  const SimpleString toeValue = panel->getText(PID_CTRLS_LEG_LOCKER_TOE);
  if (hipValue.empty() || kneeValue.empty() || ankleValue.empty() || toeValue.empty())
  {
    logerr("leg_locker <%s>: can't save leg with empty hip, knee, ankle or toe", settings->getStr("name", ""));
    return;
  }
  if (!selectedBlock)
  {
    if (positions.size() >= MAX_LEG_COUNT)
    {
      logerr("leg_locker <%s>: can't add leg <%s>, LegLockerNode reads only %d legs", settings->getStr("name", ""), hipValue.c_str(),
        MAX_LEG_COUNT);
      return;
    }
    selectedBlock = settings->addNewBlock("leg");
  }

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  selectedBlock->setStr("hip", hipValue.c_str());
  selectedBlock->setStr("knee", kneeValue.c_str());
  selectedBlock->setStr("ankle", ankleValue.c_str());
  selectedBlock->setStr("toe", toeValue.c_str());

  if (listName != hipValue)
    panel->setText(PID_CTRLS_NODES_LIST, hipValue.c_str());
}

void leg_locker_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  const dag::Vector<int> positions = collect_block_positions_by_name(*settings, "leg");
  const DataBlock *selectedBlock =
    selectedIdx >= 0 && selectedIdx < positions.size() ? settings->getBlock(positions[selectedIdx]) : &DataBlock::emptyBlock;
  panel->setText(PID_CTRLS_LEG_LOCKER_HIP, selectedBlock->getStr("hip", ""));
  panel->setText(PID_CTRLS_LEG_LOCKER_KNEE, selectedBlock->getStr("knee", ""));
  panel->setText(PID_CTRLS_LEG_LOCKER_ANKLE, selectedBlock->getStr("ankle", ""));
  panel->setText(PID_CTRLS_LEG_LOCKER_TOE, selectedBlock->getStr("toe", ""));
  bool isEditable = selectedIdx >= 0;
  for (int i = PID_CTRLS_LEG_LOCKER_HIP; i <= PID_CTRLS_LEG_LOCKER_TOE; ++i)
    panel->setEnabledById(i, isEditable);
}

void leg_locker_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const int removeIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  dag::Vector<int> positions = collect_block_positions_by_name(*settings, "leg");
  if (removeIdx >= 0 && removeIdx < positions.size())
    settings->removeBlock(positions[removeIdx]);
}

class LegLockerReorderHandler : public BaseCtrlReorderHandler
{
public:
  LegLockerReorderHandler(AnimTreeEditor &editor, dag::ConstSpan<AnimCtrlData> controllers,
    PropPanel::ContainerPropertyControl *panel) :
    BaseCtrlReorderHandler(editor, controllers, panel)
  {}

protected:
  void handleSpecificReorder(DataBlock &settings, int from, int to) override
  {
    dag::Vector<int> positions = collect_block_positions_by_name(settings, "leg");
    move_block_at_positions(settings, positions, from, to);
  }
};

IListReorderHandler *leg_locker_get_reorder_handler(AnimTreeEditor &editor, dag::ConstSpan<AnimCtrlData> controllers,
  PropPanel::ContainerPropertyControl *panel)
{
  return new LegLockerReorderHandler(editor, controllers, panel);
}

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "aim.h"
#include "../animTree.h"
#include "../animTreeDragListHandler.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>
#include <util/dag_lookup.h>
#include <ioSys/dag_dataBlockCommentsDef.h>

const float DEFAULT_YAW_ACCEL = -1.0f;
const float DEFAULT_PITCH_ACCEL = -1.0f;

#define DEPENDENT_PARAM_NAMES_LIST                                                                                                  \
  DECL_NAMES("paramTargetYaw", ":targetYaw"), DECL_NAMES("paramYaw", ":yaw"), DECL_NAMES("paramPrevYaw", ":prevYaw"),               \
    DECL_NAMES("paramTargetPitch", ":targetPitch"), DECL_NAMES("paramPitch", ":pitch"), DECL_NAMES("paramPrevPitch", ":prevPitch"), \
    DECL_NAMES("paramHasStab", ":hasStab"), DECL_NAMES("paramStabYaw", ":stabYaw"), DECL_NAMES("paramStabYawMult", ":stabYawMult"), \
    DECL_NAMES("paramStabPitch", ":stabPitch"), DECL_NAMES("paramStabPitchMult", ":stabPitchMult"),                                 \
    DECL_NAMES("paramStabError", ":stabError")

#define DECL_NAMES(x, y) String(x)
const Tab<String> dependent_param_names = {DEPENDENT_PARAM_NAMES_LIST};
#undef DECL_NAMES

#define DECL_NAMES(x, y) y
const char *dependent_param_postfixes[] = {DEPENDENT_PARAM_NAMES_LIST};
#undef DECL_NAMES

void aim_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "node");
  add_edit_float_if_not_exists(params, panel, field_idx, "defaultYaw");
  add_edit_float_if_not_exists(params, panel, field_idx, "defaultPitch");
  add_edit_float_if_not_exists(params, panel, field_idx, "yawSpeed");
  add_edit_float_if_not_exists(params, panel, field_idx, "pitchSpeed");
  add_edit_float_if_not_exists(params, panel, field_idx, "yawAccel", DEFAULT_YAW_ACCEL);
  add_edit_float_if_not_exists(params, panel, field_idx, "pitchAccel", DEFAULT_PITCH_ACCEL);
  add_edit_box_if_not_exists(params, panel, field_idx, "param");

  add_edit_box_if_not_exists(params, panel, field_idx, "paramYawSpeedMul");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramPitchSpeedMul");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramYawSpeed");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramPitchSpeed");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramYawAccel");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramPitchAccel");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramMinYawAngle");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramMaxYawAngle");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramMinPitchAngle");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramMaxPitchAngle");

  add_edit_box_if_not_exists(params, panel, field_idx, "paramTargetYaw");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramYaw");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramWorldYaw");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramPrevYaw");

  add_edit_box_if_not_exists(params, panel, field_idx, "paramTargetPitch");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramPitch");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramWorldPitch");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramPrevPitch");

  add_edit_box_if_not_exists(params, panel, field_idx, "paramHasStab");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramStabYaw");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramStabYawMult");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramStabPitch");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramStabPitchMult");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramStabError");

  add_edit_point4_if_not_exists(params, panel, field_idx, "limit");
}

void aim_set_dependent_defaults(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  set_str_param_by_name_if_default(params, panel, "param", nameValue.c_str());
  const SimpleString paramValue = get_str_param_by_name_optional(params, panel, "param");
  for (int i = 0; i < dependent_param_names.size(); ++i)
  {
    const String defaultValue(0, "%s%s", paramValue, dependent_param_postfixes[i]);
    set_str_param_by_name_if_default(params, panel, dependent_param_names[i], defaultValue.c_str());
  }
}

void update_dependent_field_by_name(dag::ConstSpan<AnimParamData> params, PropPanel::ContainerPropertyControl *panel, int pid,
  const char *name, const char *name_postfix)
{
  const AnimParamData *data = find_param_by_name(params, name);
  if (data != params.end() && data->dependent)
  {
    const SimpleString value = panel->getText(pid);
    panel->setText(data->pid, String(0, "%s%s", value, name_postfix));
  }
}

void aim_update_dependent_fields(AnimParamData &param, dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  int changedIdx = lup(param.name, dependent_param_names);
  if (changedIdx >= 0)
  {
    const SimpleString paramValue = get_str_param_by_name_optional(params, panel, "param");
    const String dependentValue(0, "%s%s", paramValue, dependent_param_postfixes[changedIdx]);
    param.dependent = panel->getText(param.pid) == dependentValue.c_str();
  }
  else if (param.name == "name")
  {
    const SimpleString nameValue = panel->getText(param.pid);
    AnimParamData *paramData = find_param_by_name(params, "param");
    if (paramData == params.end())
      return;

    if (paramData->dependent)
    {
      panel->setText(paramData->pid, nameValue);
      aim_update_dependent_fields(*paramData, params, panel);
    }
  }
  else if (param.name == "param")
  {
    for (int i = 0; i < dependent_param_names.size(); ++i)
      update_dependent_field_by_name(params, panel, param.pid, dependent_param_names[i], dependent_param_postfixes[i]);
    const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
    param.dependent = panel->getText(param.pid) == nameValue;
  }
}

void aim_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  set_str_param_by_name_if_default(params, panel, "param", nameValue.c_str());
  const SimpleString paramValue = get_str_param_by_name_optional(params, panel, "param");

  remove_param_if_default_float(params, panel, "defaultYaw");
  remove_param_if_default_float(params, panel, "defaultPitch");
  remove_param_if_default_float(params, panel, "yawSpeed");
  remove_param_if_default_float(params, panel, "pitchSpeed");
  remove_param_if_default_float(params, panel, "yawAccel", DEFAULT_YAW_ACCEL);
  remove_param_if_default_float(params, panel, "pitchAccel", DEFAULT_PITCH_ACCEL);
  remove_param_if_default_str(params, panel, "param", nameValue);

  remove_param_if_default_str(params, panel, "paramYawSpeedMul");
  remove_param_if_default_str(params, panel, "paramPitchSpeedMul");
  remove_param_if_default_str(params, panel, "paramYawSpeed");
  remove_param_if_default_str(params, panel, "paramPitchSpeed");
  remove_param_if_default_str(params, panel, "paramYawAccel");
  remove_param_if_default_str(params, panel, "paramPitchAccel");
  remove_param_if_default_str(params, panel, "paramMinYawAngle");
  remove_param_if_default_str(params, panel, "paramMaxYawAngle");
  remove_param_if_default_str(params, panel, "paramMinPitchAngle");
  remove_param_if_default_str(params, panel, "paramMaxPitchAngle");
  remove_param_if_default_str(params, panel, "paramWorldYaw");
  remove_param_if_default_str(params, panel, "paramWorldPitch");
  remove_param_if_default_point4(params, panel, "limit");

  for (int i = 0; i < dependent_param_names.size(); ++i)
  {
    const String defaultValue(0, "%s%s", paramValue, dependent_param_postfixes[i]);
    remove_param_if_default_str(params, panel, dependent_param_names[i], defaultValue.c_str());
  }
}

void aim_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> names;
  const DataBlock *table = settings->getBlockByName("limitsTable");
  if (table)
    fill_param_names_without_comments(names, *table);

  bool isEditable = !names.empty();
  const int firstIdx = isEditable ? get_param_name_idx_by_list_name(*table, SimpleString(names[0].c_str()), 0) : -1;
  panel->createList(PID_CTRLS_NODES_LIST, "Limits table", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_AIM_LIMIT_NAME, "name", firstIdx >= 0 ? table->getParamName(firstIdx) : "", isEditable);
  panel->createPoint4(PID_CTRLS_AIM_LIMIT_VALUE, "limit", firstIdx >= 0 ? table->getPoint4(firstIdx) : Point4::ZERO, /*prec*/ 2,
    isEditable);
}

void aim_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (selectedIdx == -1)
    return;

  DataBlock *table = settings->addBlock("limitsTable");
  const SimpleString name = panel->getText(PID_CTRLS_AIM_LIMIT_NAME);
  const Point4 value = panel->getPoint4(PID_CTRLS_AIM_LIMIT_VALUE);
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  // Skip commented fields
  int paramIdx = get_param_name_idx_by_list_name(*table, listName, selectedIdx);
  if (paramIdx >= 0)
  {
    table->changeParamName(paramIdx, name);
    table->setPoint4(paramIdx, value);
  }
  else
    table->addPoint4(name, value);

  SimpleString selectedName = panel->getText(PID_CTRLS_NODES_LIST);
  if (selectedName != name)
    panel->setText(PID_CTRLS_NODES_LIST, name.c_str());
}

void aim_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  const DataBlock *table = settings->getBlockByNameEx("limitsTable");
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  panel->setText(PID_CTRLS_AIM_LIMIT_NAME, name.c_str());
  int paramIdx = get_param_name_idx_by_list_name(*table, name, selectedIdx);
  if (paramIdx >= 0)
    panel->setPoint4(PID_CTRLS_AIM_LIMIT_VALUE, table->getPoint4(paramIdx));
  bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  for (int i = PID_CTRLS_AIM_LIMIT_NAME; i <= PID_CTRLS_AIM_LIMIT_VALUE; ++i)
    panel->setEnabledById(i, isEditable);
}

void aim_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  DataBlock *table = settings->getBlockByName("limitsTable");
  if (table)
  {
    // Skip commented fields
    int paramIdx = get_param_name_idx_by_list_name(*table, listName, selectedIdx);
    if (paramIdx >= 0)
      table->removeParam(paramIdx);
  }

  if (table && table->isEmpty())
    settings->removeBlock("limitsTable");
}

class AimReorderHandler : public BaseCtrlReorderHandler
{
public:
  AimReorderHandler(AnimTreePlugin &plugin, dag::ConstSpan<AnimCtrlData> controllers, PropPanel::ContainerPropertyControl *panel) :
    BaseCtrlReorderHandler(plugin, controllers, panel)
  {}

protected:
  void handleSpecificReorder(DataBlock &settings, int from, int to) override
  {
    DataBlock *table = settings.getBlockByName("limitsTable");
    if (!table)
      return;
    dag::Vector<int> positions;
    for (int i = 0; i < table->paramCount(); ++i)
      if (!CHECK_COMMENT_PREFIX(table->getParamName(i)))
        positions.push_back(i);
    if (from < 0 || from >= positions.size() || to < 0 || to >= positions.size())
      return;
    move_param_blk(*table, positions[from], positions[to]);
  }
};

IListReorderHandler *aim_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimCtrlData> controllers,
  PropPanel::ContainerPropertyControl *panel)
{
  return new AimReorderHandler(plugin, controllers, panel);
}

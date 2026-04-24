// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "paramsCtrlDragDropHandlers.h"
#include "paramsCtrl.h"
#include "../animTree.h"
#include "../animTreePanelPids.h"
#include "../animTreeUtils.h"
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>
#include <propPanel/control/dragAndDropHandler.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // GImGui, ImRect

static constexpr char IF_MATH_DRAG_PAYLOAD_TYPE[] = "PropPanelControlReorder";

struct FieldState
{
  SimpleString text;
  SimpleString tooltip;
  bool showTooltipAlways;
};

// Swaps texts, highlight and tooltip between two if/math editbox fields.
// Returns {srcIdx, insertIdx} for use in DataBlock reorder, or {-1, -1} if no reorder happened.
static eastl::pair<int, int> reorder_if_math_fields(PropPanel::ContainerPropertyControl *panel, int src_pid, int dst_pid,
  bool insert_before)
{
  Tab<int> pids;
  for (int pid = PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD; pid < PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD_SIZE; ++pid)
    if (panel->getById(pid) != nullptr)
      pids.push_back(pid);

  int srcIdx = -1, dstIdx = -1;
  for (int i = 0; i < pids.size(); ++i)
  {
    if (pids[i] == src_pid)
      srcIdx = i;
    if (pids[i] == dst_pid)
      dstIdx = i;
  }
  if (srcIdx < 0 || dstIdx < 0 || srcIdx == dstIdx)
    return {-1, -1};

  int insertIdx = insert_before ? dstIdx : dstIdx + 1;
  if (srcIdx < dstIdx)
    --insertIdx;

  Tab<FieldState> states;
  states.resize(pids.size());
  for (int i = 0; i < pids.size(); ++i)
  {
    states[i].text = panel->getText(pids[i]);
    const char *tip = panel->getTooltipById(pids[i]);
    states[i].tooltip = tip ? tip : "";
    states[i].showTooltipAlways = !states[i].tooltip.empty();
  }

  FieldState moved = states[srcIdx];
  states.erase(states.begin() + srcIdx);
  states.insert(states.begin() + insertIdx, moved);

  for (int i = 0; i < pids.size(); ++i)
  {
    panel->setText(pids[i], states[i].text.c_str());
    panel->setTooltipId(pids[i], states[i].tooltip.c_str());
    panel->setShowTooltipAlwaysById(pids[i], states[i].showTooltipAlways);
    const bool hasError = !states[i].tooltip.empty();
    panel->setValueHighlightById(pids[i],
      hasError ? PropPanel::ColorOverride::EDIT_BOX_WRONG_VALUE_BACKGROUND : PropPanel::ColorOverride::NONE);
  }
  return {srcIdx, insertIdx};
}

static void params_ctrl_reorder_if_math_blocks(DataBlock &settings, int src_idx, int insert_idx)
{
  if (src_idx == insert_idx)
    return;

  const int ifNid = settings.getNameId("if");
  const int mathNid = settings.getNameId("math");

  Tab<int> positions;
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    const int nid = settings.getBlock(i)->getBlockNameId();
    if (nid == ifNid || nid == mathNid)
      positions.push_back(i);
  }

  if (src_idx < 0 || insert_idx < 0 || src_idx >= positions.size() || insert_idx >= positions.size())
    return;

  move_block_blk(settings, positions[src_idx], positions[insert_idx]);
}

void AnimTreePlugin::saveParamsCtrlAfterIfMathReorder(PropPanel::ContainerPropertyControl *panel, int src_idx, int insert_idx)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;
  TLeafHandle parent = tree->getParentLeaf(leaf);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);
  if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
    if (DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf)))
    {
      params_ctrl_reorder_if_math_blocks(*settings, src_idx, insert_idx);
      save_if_math_fields(panel, *settings);
      saveProps(props, fullPath);
    }
}

void IfMathDragHandler::setPanel(PropPanel::ContainerPropertyControl *panel) { pluginPanel = panel; }

void IfMathDragHandler::onBeginDrag(int pid)
{
  if (pluginPanel)
  {
    const SimpleString text = pluginPanel->getText(pid);
    ImGui::TextUnformatted(text.empty() ? "(empty)" : text.c_str());
  }
  ImGui::SetDragDropPayload(IF_MATH_DRAG_PAYLOAD_TYPE, &pid, sizeof(int));
}

ImGuiDragDropFlags IfMathDragHandler::getDragDropFlags() { return ImGuiDragDropFlags_AcceptNoDrawDefaultRect; }

void IfMathDropHandler::setPlugin(AnimTreePlugin *plugin_arg, PropPanel::ContainerPropertyControl *plugin_panel)
{
  plugin = plugin_arg;
  pluginPanel = plugin_panel;
}

PropPanel::DragAndDropResult IfMathDropHandler::handleDropTarget(int target_pid, PropPanel::ContainerPropertyControl *panel)
{
  if (target_pid < PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD || target_pid >= PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD_SIZE)
    return PropPanel::DragAndDropResult::NOT_ALLOWED;

  const ImGuiPayload *payload = PropPanel::acceptDragDropPayloadBeforeDelivery(IF_MATH_DRAG_PAYLOAD_TYPE);
  if (!payload)
    return PropPanel::DragAndDropResult::NOT_ALLOWED;

  const ImRect &targetRect = GImGui->DragDropTargetRect;
  const float mouseY = ImGui::GetMousePos().y;
  const bool insertBefore = mouseY < (targetRect.Min.y + targetRect.Max.y) * 0.5f;
  const float lineY = insertBefore ? targetRect.Min.y : targetRect.Max.y;
  ImGui::GetWindowDrawList()->AddLine({targetRect.Min.x, lineY}, {targetRect.Max.x, lineY}, IM_COL32(0, 120, 215, 255), 2.0f);

  if (payload->IsDelivery())
  {
    int sourcePid;
    PropPanel::getDragAndDropPayloadData(payload, &sourcePid);
    auto [srcIdx, insertIdx] = reorder_if_math_fields(panel, sourcePid, target_pid, insertBefore);
    if (srcIdx >= 0 && plugin)
      plugin->saveParamsCtrlAfterIfMathReorder(pluginPanel, srcIdx, insertIdx);
    return PropPanel::DragAndDropResult::ACCEPTED;
  }
  return PropPanel::DragAndDropResult::NONE;
}

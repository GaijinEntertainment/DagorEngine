// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animTreeDragListHandler.h"
#include "animTreeUtils.h"
#include "animTree.h"
#include "animTreePanelPids.h"
#include "animStates/animStatesData.h"
#include <util/dag_string.h>
#include <propPanel/control/container.h>
#include <ioSys/dag_dataBlock.h>

void BaseAnimStatesReorderHandler::handleReorder(int from, int to)
{
  const AnimStatesData *data = find_data_by_type(states, getTargetAnimStateType());
  G_ASSERT(data);
  String fullPath;
  DataBlock props = plugin.getPropsAnimStates(panel, *data, fullPath);
  handleSpecificReorder(props, from, to);
  plugin.saveProps(props, fullPath);
}

void BaseCtrlReorderHandler::handleReorder(int from, int to)
{
  DataBlock props;
  String fullPath;
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;
  const AnimCtrlData *data = find_data_by_handle(controllers, leaf);
  if (data == controllers.end())
    return;
  DataBlock *settings = plugin.findCtrlSettings(tree, leaf, data->type, props, fullPath);
  if (!settings)
    return;
  handleSpecificReorder(*settings, from, to);
  plugin.saveProps(props, fullPath);
}

bool BaseCtrlReorderHandler::canReorder()
{
  Tab<String> values;
  panel->getStrings(PID_CTRLS_NODES_LIST, values);
  for (const String &v : values)
    if (v == "new_node")
      return false;
  return true;
}

void ResetRandomSwitchReorderHandler::handleReorder(int from, int to) { plugin.reorderResetRandomSwitchList(panel, from, to); }

void AnimTreeListDragHandler::onBeginDrag(int idx, const char *value)
{
  DragDropPayloadData payloadData;
  payloadData.idx = idx;
  payloadData.name = value;
  ImGui::Text("%s", value);
  ImGui::SetDragDropPayload(ASSET_DRAG_AND_DROP_TYPE, &payloadData, sizeof(DragDropPayloadData));
}

ImGuiDragDropFlags AnimTreeListDragHandler::getDragDropFlags() { return ImGuiDragDropFlags_AcceptNoDrawDefaultRect; }

PropPanel::DragAndDropResult AnimTreeListDropHandler::onDropTarget(int idx)
{
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 min = ImGui::GetItemRectMin();
  ImVec2 max = ImGui::GetItemRectMax();
  float mouseY = ImGui::GetMousePos().y;
  float midY = (min.y + max.y) * 0.5f;
  float y = mouseY < midY ? min.y : max.y;

  drawList->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), IM_COL32(0, 120, 215, 255), 2.0f);
  const ImGuiPayload *dragAndDropPayload = PropPanel::acceptDragDropPayloadBeforeDelivery("ListBoxReorder");
  if (reorderHandler && !reorderHandler->canReorder())
    return PropPanel::DragAndDropResult::NOT_ALLOWED;

  DragDropPayloadData dragData;
  PropPanel::getDragAndDropPayloadData<DragDropPayloadData>(dragAndDropPayload, &dragData);
  if (dragAndDropPayload && dragAndDropPayload->IsDelivery())
  {
    int offsetIdx = mouseY < midY ? 0 : 1;
    offsetIdx += dragData.idx < idx ? -1 : 0;
    String name(dragData.name);
    Tab<String> values;
    panel->getStrings(pid, values);
    values.erase(values.begin() + dragData.idx);
    values.insert(values.begin() + idx + offsetIdx, name);
    panel->setStrings(pid, values);
    panel->setInt(pid, idx + offsetIdx);
    if (reorderHandler)
      reorderHandler->handleReorder(dragData.idx, idx + offsetIdx);
    return PropPanel::DragAndDropResult::ACCEPTED;
  }

  return PropPanel::DragAndDropResult::NONE;
}
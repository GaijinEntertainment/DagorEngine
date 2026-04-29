//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/c_common.h> // TLeafHandle
#include <imgui/imgui.h>
#include <cstring>

namespace PropPanel
{

class ContainerPropertyControl;

enum class DragAndDropResult
{
  NONE,
  NOT_ALLOWED,
  ACCEPTED,
};

class IDragSourceHandler
{
public:
  virtual void onBeginDrag(int pcb_id) = 0;
  virtual ImGuiDragDropFlags getDragDropFlags() = 0;
};

class IDropTargetHandler
{
public:
  virtual DragAndDropResult handleDropTarget(int pcb_id, PropPanel::ContainerPropertyControl *panel) = 0;
};

class ITreeDragHandler
{
public:
  // Receives the the node data of the dragged tree-item.
  // Implementation should drag & drop payload (ImGui::SetDragDropPayload)
  // and prepare the imgui widgets ghost image following the cursor.
  virtual void onBeginDrag(TLeafHandle leaf) = 0;
};

class ITreeDropHandler
{
public:
  // Receives the the node data of the drop-target tree-item.
  // Implementation should handle payload validity and acceptence (ImGui::AcceptDragDropPayload)
  // and immediately execute the expected user (inter)action.
  virtual DragAndDropResult onDropTargetDirect(TLeafHandle leaf) = 0;

  // Receives parent node data and index of requested drop position
  // Implementation should handle payload validity and acceptence (ImGui::AcceptDragDropPayload)
  // and immediately execute the expected user (inter)action.
  virtual DragAndDropResult onDropTargetBetween([[maybe_unused]] TLeafHandle leaf, [[maybe_unused]] int idx)
  {
    return DragAndDropResult::NOT_ALLOWED;
  }
  virtual bool canDropBetween() { return false; }
};

class IListDragHandler
{
public:
  virtual void onBeginDrag(int idx, const char *value) = 0;
  virtual ImGuiDragDropFlags getDragDropFlags() = 0;
};

class IListDropHandler
{
public:
  virtual DragAndDropResult onDropTarget(int idx) = 0;
};


// Note: accepting the drag & drop payload with the matching type will high-light the target (hovered) widget.
// For dropping with in-between feature, call the function with no default rectangle flag
// Only do so if other paramaters for delivering on the target widget are correct!
inline const ImGuiPayload *acceptDragDropPayloadBeforeDelivery(const char *type, bool no_default_rec = false)
{
  return ImGui::AcceptDragDropPayload(type,
    ImGuiDragDropFlags_AcceptBeforeDelivery | (no_default_rec ? ImGuiDragDropFlags_AcceptNoDrawDefaultRect : 0));
}

template <typename DragData>
inline void getDragAndDropPayloadData(const ImGuiPayload *drag_and_drop_payload, DragData *drag_data)
{
  G_ASSERT(drag_and_drop_payload->DataSize == sizeof(DragData));
  memcpy(drag_data, drag_and_drop_payload->Data, drag_and_drop_payload->DataSize);
}

inline void handleDragAndDropNotAllowed(DragAndDropResult result)
{
  if (result == DragAndDropResult::NOT_ALLOWED)
    ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
}

} // namespace PropPanel

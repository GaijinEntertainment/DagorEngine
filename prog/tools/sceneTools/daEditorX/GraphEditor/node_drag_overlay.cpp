// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "node_drag_overlay.h"

#include "graph_node_render.h"
#include "node_drag_payload.h"
#include "plugin.h"

#include <graphEditor/graph_data.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // ImGuiContext::MouseViewport

namespace
{
constexpr ImVec2 GHOST_CURSOR_OFFSET(16.0f, 10.0f);
} // namespace

void NodeDragOverlay::resolveTemplate(GraphEditorPlg &plugin, const char *template_uid)
{
  // Same factory the drop uses, so the ghost's pin sides match the node that will land.
  uid = template_uid;
  name.clear();
  hasInputs = false;
  hasOutputs = false;

  GraphData::Node preview;
  if (!plugin.makeNodeFromBaseBlk(template_uid, 0.0f, 0.0f, preview))
  {
    return;
  }

  name = preview.descName;
  for (const GraphData::Pin &pin : preview.pins)
  {
    // What the node pass skips too: separators are rules, hidden pins are never drawn.
    if (pin.separator || pin.hidden)
    {
      continue;
    }
    (pin.isInput ? hasInputs : hasOutputs) = true;
  }
}

void NodeDragOverlay::draw(GraphEditorPlg &plugin, bool drop_target_hot)
{
  // Null on the frame the canvas takes the drop: EndDragDropTarget clears the drag right after
  // delivery, so the ghost never overlaps the node it just created.
  const ImGuiPayload *const payload = ImGui::GetDragDropPayload();
  if (!payload || !payload->IsDataType(BASE_NODE_DRAG_PAYLOAD))
  {
    uid.clear(); // a base-node reload can change what the same uid resolves to
    return;
  }

  BaseNodeDragUid templateUid;
  read_base_node_drag_payload(*payload, templateUid);
  if (uid != templateUid)
  {
    resolveTemplate(plugin, templateUid);
  }

  if (!drop_target_hot)
  {
    ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
  }

  // The viewport under the cursor, not the main one: with multi-viewport a panel can be floated onto
  // another monitor, and the ghost has to follow the drag there.
  ImGuiViewport *viewport = ImGui::GetCurrentContext()->MouseViewport;
  if (!viewport)
  {
    viewport = ImGui::GetMainViewport();
  }
  const ImVec2 boundsMax(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);

  // Offset off the cursor by what ImGui gives its own tooltips, so the ghost sits where the preview it
  // replaced would have. The node lands at the cursor, or below-forward of the pin when the drop is
  // on one; never under the ghost.
  const float cursorScale = ImGui::GetStyle().MouseCursorScale;
  const ImVec2 mouse = ImGui::GetMousePos();
  const ImVec2 anchor(mouse.x + GHOST_CURSOR_OFFSET.x * cursorScale, mouse.y + GHOST_CURSOR_OFFSET.y * cursorScale);
  draw_node_drag_ghost(ImGui::GetForegroundDrawList(viewport), name.c_str(), hasInputs, hasOutputs, anchor, viewport->Pos, boundsMax);
}

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_shortcuts_panel.h"

#include "command_definitions.h"
#include "graph_command_text.h"

#include <oldEditor/de_cm.h>

#include <EditorCore/ec_interface.h>

#include <imgui/imgui.h>

namespace
{
struct ShortcutRow
{
  const char *section;
  const char *commandId;
  const char *staticKey;
  const char *description;
};

const ShortcutRow SHORTCUT_ROWS[] = {
  {"General", SHOW_SHORTCUTS, nullptr, "Show / hide this panel"},
  {nullptr, nullptr, "RMB", "Context menu for the node, pin, edge or empty space under the cursor"},
  {nullptr, CANVAS_TOGGLE_AUTOUPDATE, nullptr, "Toggle auto-update"},
  {nullptr, FORCE_REBUILD, nullptr, "Force rebuild"},
  {nullptr, TOGGLE_GRAPH_PROPERTIES, nullptr,
    "Toggle Properties between the selected node and the graph (in a subgraph window: its interface)"},

  {"View and Navigation", nullptr, "Mouse Wheel", "Zoom in / out"},
  {nullptr, nullptr, "Middle Mouse", "Pan the canvas (drag)"},
  {nullptr, CANVAS_ZOOM_AND_CENTER, nullptr, "Center the view on the whole graph"},
  {nullptr, EditorCommandIds::ZOOM_AND_CENTER, nullptr, "Frame the selected nodes (Zoom and center)"},

  {"Selection", nullptr, "LMB", "Select; drag to move; drag from a pin to connect"},
  {nullptr, nullptr, "Ctrl+LMB", "Toggle selection"},
  {nullptr, nullptr, "Double Click", "Preview the node"},
  {nullptr, CANVAS_SELECT_NODES_NO_OUTPUTS, nullptr, "Select nodes with no connected outputs"},
  {nullptr, CANVAS_SHOW_NEXT_SELECTED, nullptr, "Show the next selected node"},
  {nullptr, EditorCommandIds::SELECT_ALL, nullptr, "Select all nodes"},
  {nullptr, EditorCommandIds::DESELECT_ALL, nullptr, "Deselect all"},
  {nullptr, EditorCommandIds::INVERT_SELECTION, nullptr, "Invert the node selection"},

  {"Editing", CANVAS_DELETE_SELECTED, nullptr, "Remove selected"},
  {nullptr, CANVAS_REMOVE_KEEP_CONNECTIONS, nullptr, "Remove but keep connections"},
  {nullptr, CANVAS_COPY, nullptr, "Copy"},
  {nullptr, CANVAS_CUT, nullptr, "Cut"},
  {nullptr, CANVAS_PASTE, nullptr, "Paste"},
  {nullptr, CANVAS_DUPLICATE, nullptr, "Duplicate selected"},

  {"Connections", CANVAS_REMOVE_EDGES_AT_PIN, nullptr, "Remove edges (cursor must be over a pin)"},
  {nullptr, CANVAS_MODIFY_EDGE_AT_PIN, nullptr, "Modify edge, several edges open a list (cursor must be over a pin)"},
  {nullptr, CANVAS_JUMP_OPPOSITE_PIN, nullptr, "Jump to the opposite pin, several edges open a list (cursor must be over a pin)"},
  {nullptr, CANVAS_COMMENT_PIN, nullptr, "Comment a pin (cursor must be over a pin)"},
  {nullptr, CANVAS_ADD_NODE_AT_PIN, nullptr, "Add a connected node (cursor must be over a pin or an edge)"},
  {nullptr, CANVAS_ADD_TRANSIT_NODE, nullptr, "Add a node into the connections (over a pin or an edge, or press and then aim)"},
  {nullptr, nullptr, "LMB Drag", "Drag a link to empty space to add a connected node"},

  {"Subgraphs", CANVAS_CREATE_SUBGRAPH, nullptr, "Create a subgraph from the selected nodes (2 or more)"},
  {nullptr, CANVAS_OPEN_SUBGRAPH, nullptr, "Open the selected subgraph, or focus its window (a pin under the cursor jumps instead)"},
  {nullptr, CANVAS_EXPLODE_SUBGRAPH, nullptr, "Explode the selected subgraph into the graph"},
  {nullptr, CANVAS_GO_TO_PARENT_GRAPH, nullptr, "Go to the parent graph (a subgraph window must be focused)"},
};

const char *row_key_text(const ShortcutRow &row)
{
  if (!row.commandId)
  {
    return row.staticKey ? row.staticKey : "";
  }
  return command_hint(row.commandId);
}
} // namespace

ShortcutsPanel::ShortcutsPanel() { panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Shortcuts"); }

ShortcutsPanel::~ShortcutsPanel() { IEditorCoreEngine::get()->deleteCustomPanel(panelWindow); }

void ShortcutsPanel::updateImgui()
{
  // Each section is its own 2-column table (key | action), so key columns size to the widest
  // chord within their group. The panel window provides scrolling when the list overflows.
  bool tableOpen = false;
  for (const ShortcutRow &row : SHORTCUT_ROWS)
  {
    if (row.section)
    {
      if (tableOpen)
      {
        ImGui::EndTable();
        tableOpen = false;
      }
      ImGui::SeparatorText(row.section);
      if (ImGui::BeginTable(row.section, 2, ImGuiTableFlags_SizingFixedFit))
      {
        ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthStretch);
        tableOpen = true;
      }
    }
    if (!tableOpen)
    {
      continue;
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(row_key_text(row));
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(row.description);
  }
  if (tableOpen)
  {
    ImGui::EndTable();
  }
}

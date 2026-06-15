// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_shortcuts_panel.h"

#include "command_definitions.h"

#include <EditorCore/ec_editorCommandSystem.h>
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

  {"View and Navigation", nullptr, "Mouse Wheel", "Zoom in / out"},
  {nullptr, nullptr, "Middle Mouse", "Pan the canvas (drag)"},
  {nullptr, CANVAS_ZOOM_AND_CENTER, nullptr, "Center the view on the whole graph"},
  {nullptr, CANVAS_FRAME_SELECTED, nullptr, "Frame the selected nodes"},
  {nullptr, CANVAS_FRAME_SELECTED_WITH_MARGIN, nullptr, "Frame the selected nodes (zoomed in)"},

  {"Selection", nullptr, "LMB", "Select; drag to move; drag from a pin to connect"},
  {nullptr, nullptr, "Ctrl+LMB", "Toggle selection"},
  {nullptr, nullptr, "Double Click", "Preview the node"},
  {nullptr, CANVAS_SELECT_NODES_NO_OUTPUTS, nullptr, "Select nodes with no connected outputs"},
  {nullptr, CANVAS_SHOW_NEXT_SELECTED, nullptr, "Show the next selected node"},

  {"Editing", CANVAS_DELETE_SELECTED, nullptr, "Remove selected"},
  {nullptr, CANVAS_REMOVE_KEEP_CONNECTIONS, nullptr, "Remove but keep connections"},
  {nullptr, CANVAS_COPY, nullptr, "Copy"},
  {nullptr, CANVAS_CUT, nullptr, "Cut"},
  {nullptr, CANVAS_PASTE, nullptr, "Paste"},

  {"Connections", CANVAS_REMOVE_EDGES_AT_PIN, nullptr, "Remove edges (cursor must be over a pin)"},
  {nullptr, CANVAS_MODIFY_EDGE_AT_PIN, nullptr, "Modify edge (cursor must be over a pin)"},
  {nullptr, CANVAS_JUMP_OPPOSITE_PIN, nullptr, "Jump to the opposite pin (cursor must be over a pin)"},
  {nullptr, CANVAS_COMMENT_PIN, nullptr, "Comment a pin (cursor must be over a pin)"},
};

const char *row_key_text(IEditorCommandSystem *command_system, const ShortcutRow &row)
{
  if (!row.commandId)
  {
    return row.staticKey ? row.staticKey : "";
  }
  if (command_system)
  {
    const char *text = command_system->getCommandKeyChordsAsText(row.commandId);
    if (text && *text)
    {
      return text;
    }
  }
  return "";
}
} // namespace

ShortcutsPanel::ShortcutsPanel() { panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Shortcuts"); }

ShortcutsPanel::~ShortcutsPanel() { IEditorCoreEngine::get()->deleteCustomPanel(panelWindow); }

void ShortcutsPanel::updateImgui()
{
  IEditorCommandSystem *commandSystem = EDITORCORE->queryEditorInterface<IEditorCommandSystem>();

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
    ImGui::TextUnformatted(row_key_text(commandSystem, row));
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(row.description);
  }
  if (tableOpen)
  {
    ImGui::EndTable();
  }
}

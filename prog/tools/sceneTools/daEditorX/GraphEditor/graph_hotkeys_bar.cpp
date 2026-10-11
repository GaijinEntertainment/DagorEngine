// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_hotkeys_bar.h"

#include "command_definitions.h"
#include "graph_command_text.h"

#include <de3_interface.h>
#include <propPanel/propPanelService.h>
#include <libTools/util/hdpiUtil.h>

#include <imgui/imgui_internal.h>

#include <EASTL/algorithm.h>

namespace
{
constexpr int HOTKEYS_ROW_HEIGHT = 20;      // row / key-cap / mouse-icon height
constexpr int HOTKEYS_MOUSE_ICON_SIZE = 20; // mouse_wheel / mouse_move are 20x20
constexpr int HOTKEYS_ENTRY_GAP = 12;       // gap between adjacent entries
constexpr int HOTKEYS_LABEL_GAP = 4;        // gap between an entry's icon / key-cap and its label
constexpr int HOTKEYS_KEYCAP_PAD_X = 6;     // key-cap inner horizontal padding (each side)
constexpr int HOTKEYS_KEYCAP_ROUNDING = 3;  // key-cap corner radius
constexpr int HOTKEYS_MARGIN = 10;          // inset from the canvas edges

constexpr ImU32 HOTKEYS_TEXT_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);          // white labels + cap text
constexpr ImU32 HOTKEYS_KEYCAP_BORDER_COLOR = IM_COL32(0x8C, 0x8C, 0x8C, 0xFF); // key-cap outline
constexpr ImU32 HOTKEYS_HINT_COLOR = IM_COL32(0x81, 0xC5, 0xFF, 0xFF);          // a contextual entry, cap and label alike

// A contextual entry draws only while its action is available, and is tinted to say so.
enum class HotkeyHint
{
  Always,
  PinJump,
  AddNode,
  AddTransitNode,
  CreateSubgraph,
  OpenSubgraph,
  FocusSubgraph,
  ExplodeSubgraph,
  ParentGraph,
};

struct HotkeyEntry
{
  bool isKeyCap;
  const char *iconName;
  const char *commandId; // a key-cap shows the command's current chord, so a rebind shows through
  const char *label;
  HotkeyHint hint;
};

// The icon name is pasted into the path, so the "dark/" prefix pins these to commonData/icons/dark
// whatever the active theme is -- the bar sits on the canvas, which never goes light.
// Left to right, the entries always shown first, so a contextual one never moves them.
const HotkeyEntry HOTKEY_ENTRIES[] = {
  {true, nullptr, SHOW_SHORTCUTS, "Tutorial", HotkeyHint::Always},
  {true, nullptr, CANVAS_ZOOM_AND_CENTER, "Zoom and Center", HotkeyHint::Always},
  {false, "dark/mouse_wheel", nullptr, "Zoom", HotkeyHint::Always},
  {false, "dark/mouse_move", nullptr, "Pan", HotkeyHint::Always},
  {true, nullptr, CANVAS_JUMP_OPPOSITE_PIN, "Jump to opposite pin", HotkeyHint::PinJump},
  {true, nullptr, CANVAS_ADD_TRANSIT_NODE, "Add transit node", HotkeyHint::AddTransitNode},
  {true, nullptr, CANVAS_ADD_NODE_AT_PIN, "Add node", HotkeyHint::AddNode},
  {true, nullptr, CANVAS_GO_TO_PARENT_GRAPH, "Go to parent graph", HotkeyHint::ParentGraph},
  {true, nullptr, CANVAS_EXPLODE_SUBGRAPH, "Explode selected subgraph", HotkeyHint::ExplodeSubgraph},
  {true, nullptr, CANVAS_OPEN_SUBGRAPH, "Focus to selected subgraph", HotkeyHint::FocusSubgraph},
  {true, nullptr, CANVAS_OPEN_SUBGRAPH, "Open selected subgraph", HotkeyHint::OpenSubgraph},
  {true, nullptr, CANVAS_CREATE_SUBGRAPH, "Create a subgraph", HotkeyHint::CreateSubgraph},
};
} // namespace

void draw_graph_hotkeys_bar(const ImVec2 &canvas_min, const ImVec2 &canvas_max, const GraphHotkeyContext &context, float end_width)
{
  ImDrawList *dl = ImGui::GetWindowDrawList();

  const float rowH = static_cast<float>(hdpi::_pxS(HOTKEYS_ROW_HEIGHT));
  const float iconSize = static_cast<float>(hdpi::_pxS(HOTKEYS_MOUSE_ICON_SIZE));
  const float entryGap = static_cast<float>(hdpi::_pxS(HOTKEYS_ENTRY_GAP));
  const float labelGap = static_cast<float>(hdpi::_pxS(HOTKEYS_LABEL_GAP));
  const float keycapPadX = static_cast<float>(hdpi::_pxS(HOTKEYS_KEYCAP_PAD_X));
  const float rounding = static_cast<float>(hdpi::_pxS(HOTKEYS_KEYCAP_ROUNDING));
  const float margin = static_cast<float>(hdpi::_pxS(HOTKEYS_MARGIN));
  const float borderThickness = static_cast<float>(eastl::max(1, hdpi::_pxS(1))); // never vanish at sub-96 DPI
  const float textH = ImGui::GetTextLineHeight();

  auto hidden = [&context](const HotkeyEntry &e, const char *cap) {
    if (e.isKeyCap && !*cap)
    {
      return true; // an unbound command has no key to press
    }
    switch (e.hint)
    {
      case HotkeyHint::PinJump: return !context.pinJumpAvailable;
      case HotkeyHint::AddNode: return !context.addNodeAvailable;
      case HotkeyHint::AddTransitNode: return !context.addTransitNodeAvailable;
      case HotkeyHint::CreateSubgraph: return !context.createSubgraphAvailable;
      case HotkeyHint::OpenSubgraph: return !context.openSubgraphAvailable;
      case HotkeyHint::FocusSubgraph: return !context.focusSubgraphAvailable;
      case HotkeyHint::ExplodeSubgraph: return !context.explodeSubgraphAvailable;
      case HotkeyHint::ParentGraph: return !context.parentGraphAvailable;
      default: return false;
    }
  };

  const float rowTop = canvas_max.y - margin - rowH;
  const float textY = rowTop + (rowH - textH) * 0.5f;
  const float iconY = rowTop + (rowH - iconSize) * 0.5f;
  const float maxX = canvas_max.x - margin - (end_width > 0.0f ? end_width + entryGap : 0.0f);

  float x = canvas_min.x + margin;
  for (const HotkeyEntry &e : HOTKEY_ENTRIES)
  {
    const char *cap = e.isKeyCap ? command_hint(e.commandId) : "";
    if (hidden(e, cap))
    {
      continue;
    }
    const float leadW = e.isKeyCap ? ImGui::CalcTextSize(cap).x + keycapPadX * 2.0f : iconSize;
    const float labelW = ImGui::CalcTextSize(e.label).x;
    if (x + leadW + labelGap + labelW > maxX)
    {
      break;
    }

    const bool contextual = e.hint != HotkeyHint::Always;
    const ImU32 textColor = contextual ? HOTKEYS_HINT_COLOR : HOTKEYS_TEXT_COLOR;
    if (e.isKeyCap)
    {
      const ImU32 capBorder = contextual ? HOTKEYS_HINT_COLOR : HOTKEYS_KEYCAP_BORDER_COLOR;
      dl->AddRect(ImVec2(x, rowTop), ImVec2(x + leadW, rowTop + rowH), capBorder, rounding, ImDrawFlags_None, borderThickness);
      dl->AddText(ImVec2(x + keycapPadX, textY), textColor, cap);
    }
    else
    {
      const ImTextureID tex = DAEDITOR3.getPropPanelService()->getIconTextureId(e.iconName, hdpi::_pxS(HOTKEYS_MOUSE_ICON_SIZE));
      if (tex)
      {
        dl->AddImage(tex, ImVec2(x, iconY), ImVec2(x + iconSize, iconY + iconSize));
      }
    }

    x += leadW + labelGap;
    dl->AddText(ImVec2(x, textY), textColor, e.label);
    x += labelW + entryGap;
  }
}

float graph_hotkeys_bar_height() { return static_cast<float>(hdpi::_pxS(HOTKEYS_MARGIN) + hdpi::_pxS(HOTKEYS_ROW_HEIGHT)); }

ImRect graph_hotkeys_bar_end(const ImVec2 &canvas_max, float width)
{
  const float margin = static_cast<float>(hdpi::_pxS(HOTKEYS_MARGIN));
  const float rowH = static_cast<float>(hdpi::_pxS(HOTKEYS_ROW_HEIGHT));
  return ImRect(canvas_max.x - margin - width, canvas_max.y - margin - rowH, canvas_max.x - margin, canvas_max.y - margin);
}

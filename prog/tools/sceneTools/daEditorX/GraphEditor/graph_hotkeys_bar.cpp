// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_hotkeys_bar.h"

#include <de3_interface.h>
#include <propPanel/propPanelService.h>
#include <libTools/util/hdpiUtil.h>

#include <imgui/imgui.h>

#include <EASTL/algorithm.h>

namespace
{
constexpr int HOTKEYS_ROW_HEIGHT = 20;      // row / key-cap / mouse-icon height
constexpr int HOTKEYS_MOUSE_ICON_SIZE = 20; // mouse_wheel / mouse_move are 20x20
constexpr int HOTKEYS_ENTRY_GAP = 12;       // gap between adjacent entries
constexpr int HOTKEYS_LABEL_GAP = 4;        // gap between an entry's icon / key-cap and its label
constexpr int HOTKEYS_KEYCAP_PAD_X = 6;     // key-cap inner horizontal padding (each side)
constexpr int HOTKEYS_KEYCAP_ROUNDING = 3;  // key-cap corner radius
constexpr int HOTKEYS_MARGIN = 10;          // inset from the canvas bottom and right edges

constexpr ImU32 HOTKEYS_TEXT_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);          // white labels + cap text
constexpr ImU32 HOTKEYS_KEYCAP_BORDER_COLOR = IM_COL32(0x8C, 0x8C, 0x8C, 0xFF); // key-cap outline

struct HotkeyEntry
{
  bool isKeyCap;
  const char *iconName;
  const char *capText;
  const char *label;
};

const HotkeyEntry HOTKEY_ENTRIES[] = {
  {false, "mouse_wheel", nullptr, "Zoom"},
  {false, "mouse_move", nullptr, "Pan"},
  {true, nullptr, "Ctrl+Space", "Zoom and Center"},
  {true, nullptr, "F1", "Tutorial"},
};
} // namespace

void draw_graph_hotkeys_bar(const ImVec2 &canvas_max)
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

  // Lead = the mouse icon (fixed) or the key-cap (text width + padding on both sides).
  auto leadWidth = [&](const HotkeyEntry &e) -> float {
    if (e.isKeyCap)
    {
      return ImGui::CalcTextSize(e.capText).x + keycapPadX * 2.0f;
    }
    return iconSize;
  };

  float totalWidth = 0.0f;
  bool first = true;
  for (const HotkeyEntry &e : HOTKEY_ENTRIES)
  {
    if (!first)
    {
      totalWidth += entryGap;
    }
    totalWidth += leadWidth(e) + labelGap + ImGui::CalcTextSize(e.label).x;
    first = false;
  }

  const float originX = canvas_max.x - margin - totalWidth;
  const float rowTop = canvas_max.y - margin - rowH;
  const float textY = rowTop + (rowH - textH) * 0.5f;
  const float iconY = rowTop + (rowH - iconSize) * 0.5f;

  float x = originX;
  first = true;
  for (const HotkeyEntry &e : HOTKEY_ENTRIES)
  {
    if (!first)
    {
      x += entryGap;
    }
    first = false;

    if (e.isKeyCap)
    {
      const float capW = ImGui::CalcTextSize(e.capText).x + keycapPadX * 2.0f;
      dl->AddRect(ImVec2(x, rowTop), ImVec2(x + capW, rowTop + rowH), HOTKEYS_KEYCAP_BORDER_COLOR, rounding, ImDrawFlags_None,
        borderThickness);
      dl->AddText(ImVec2(x + keycapPadX, textY), HOTKEYS_TEXT_COLOR, e.capText);
      x += capW;
    }
    else
    {
      const ImTextureID tex = DAEDITOR3.getPropPanelService()->getIconTextureId(e.iconName, hdpi::_pxS(HOTKEYS_MOUSE_ICON_SIZE));
      if (tex)
      {
        dl->AddImage(tex, ImVec2(x, iconY), ImVec2(x + iconSize, iconY + iconSize));
      }
      x += iconSize;
    }

    x += labelGap;
    dl->AddText(ImVec2(x, textY), HOTKEYS_TEXT_COLOR, e.label);
    x += ImGui::CalcTextSize(e.label).x;
  }
}

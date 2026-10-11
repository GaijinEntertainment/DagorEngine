// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_subgraph_window.h"

#include "graph_node_render.h"
#include "graph_tab.h"
#include "graph_theme.h"

#include <de3_interface.h>
#include <libTools/util/hdpiUtil.h>
#include <propPanel/propPanelService.h>

#include <EASTL/algorithm.h>

#include <imgui/imgui_internal.h>

#include <math.h>

namespace
{
constexpr int WINDOW_DEFAULT_WIDTH = 719;
constexpr int WINDOW_DEFAULT_HEIGHT = 652;
constexpr int WINDOW_MIN_SIZE = 200;
constexpr int WINDOW_HEADER_HEIGHT = 36;
constexpr int WINDOW_FOOTER_HEIGHT = 44;
constexpr int WINDOW_PAD = 8;
constexpr int WINDOW_ROUNDING = 8;
constexpr int WINDOW_FOCUS_BORDER = 2;
constexpr int WINDOW_MARGIN = 8;
constexpr int WINDOW_STACK_STEP = 10;
constexpr int WINDOW_RESIZE_EDGE = 8;  // the band of the design's content padding
constexpr int WINDOW_RESIZE_GRIP = 18; // the design's corner grip
constexpr int WINDOW_RESIZE_LINE = 2;
constexpr int WINDOW_BUTTON_SIZE = 16;
constexpr int WINDOW_BUTTON_GAP = 8;
constexpr int WINDOW_FOOTER_BUTTON_HEIGHT = 28;
constexpr int WINDOW_FOOTER_BUTTON_PAD = 12;
constexpr int WINDOW_CLOSE_GLYPH = 8;
constexpr int WINDOW_CLOSE_GLYPH_HOT = 10;
constexpr int WINDOW_CLOSE_STROKE = 2;
constexpr int WINDOW_CLOSE_ROUNDING = 1;
constexpr int WINDOW_CRUMB_GAP = 4;
constexpr int WINDOW_ICON_ROUNDING = 2;
constexpr float WINDOW_FONT_SIZE = 16.0f; // ImGui scales a pushed size itself

constexpr float GLYPH_MINIMIZE_LENGTH = 6.4f;
constexpr float GLYPH_MAXIMIZE_SIZE = 9.6f;
constexpr float GLYPH_RESTORE_SIZE = 7.5f;
constexpr float GLYPH_RESTORE_STEP = 3.2f;
constexpr float GLYPH_STROKE = 1.5f;
constexpr float GLYPH_ROUNDING = 2.0f;

constexpr int CARD_WIDTH = 250;
constexpr int CARD_HEIGHT = 60;
constexpr int CARD_BORDER = 2;
constexpr int CARD_HEADER_HEIGHT = 20;
constexpr int CARD_GAP = 4;
constexpr int STACK_TILE = 20;
constexpr int STACK_TILE_ROUNDING = 3;
constexpr int STACK_BACK_STEP = 5;
constexpr int STACK_CARDS_GAP = 2;
constexpr float STACK_BACK_ALPHA = 0.2f;
constexpr const char *STACK_ICON = "dark/subgraph_min_active";
constexpr const char *STACK_CHANGED_ICON = "dark/subgraph_min_changed";
constexpr const char *STACK_BACK_ICON = "dark/subgraph_min";

constexpr ImGuiButtonFlags ALL_MOUSE_BUTTONS =
  ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle;
// ImGui would paint a child's background into the parent's list, under the root canvas drawn after it.
constexpr ImGuiWindowFlags CANVAS_CHILD_FLAGS =
  ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

enum : int
{
  SIDE_LEFT = 1,
  SIDE_RIGHT = 2,
  SIDE_TOP = 4,
  SIDE_BOTTOM = 8,
  SIDES_ALL = SIDE_LEFT | SIDE_RIGHT | SIDE_TOP | SIDE_BOTTOM, // a move
};

struct ResizeGrip
{
  int sides;
  ImGuiMouseCursor cursor;
};

const ResizeGrip RESIZE_GRIPS[] = {
  {SIDE_LEFT, ImGuiMouseCursor_ResizeEW},
  {SIDE_RIGHT, ImGuiMouseCursor_ResizeEW},
  {SIDE_TOP, ImGuiMouseCursor_ResizeNS},
  {SIDE_BOTTOM, ImGuiMouseCursor_ResizeNS},
  {SIDE_LEFT | SIDE_TOP, ImGuiMouseCursor_ResizeNWSE},
  {SIDE_RIGHT | SIDE_TOP, ImGuiMouseCursor_ResizeNESW},
  {SIDE_LEFT | SIDE_BOTTOM, ImGuiMouseCursor_ResizeNESW},
  {SIDE_RIGHT | SIDE_BOTTOM, ImGuiMouseCursor_ResizeNWSE},
};

float scaled(int px) { return static_cast<float>(hdpi::_pxS(px)); }

// A child clips to its own rect, which the shadow reaches past, so the clip is replaced, not intersected.
void draw_floating_shadow(ImDrawList &dl, const ImVec2 &min, const ImVec2 &max, const ImVec2 &clip_min, const ImVec2 &clip_max)
{
  dl.PushClipRect(clip_min, clip_max, false);
  draw_soft_shadow(&dl, min, max, scaled(WINDOW_ROUNDING));
  dl.PopClipRect();
}

enum class IconGlyph
{
  Minimize,
  Maximize,
  Restore,
};

void draw_icon_glyph(ImDrawList &dl, const ImVec2 &center, float scale, IconGlyph glyph)
{
  const float stroke = GLYPH_STROKE * scale;
  const float rounding = GLYPH_ROUNDING * scale;
  if (glyph == IconGlyph::Minimize)
  {
    const float half = GLYPH_MINIMIZE_LENGTH * 0.5f * scale;
    dl.AddLine(ImVec2(center.x - half, center.y), ImVec2(center.x + half, center.y), IM_COL32_WHITE, stroke);
    return;
  }
  if (glyph == IconGlyph::Maximize)
  {
    const float half = GLYPH_MAXIMIZE_SIZE * 0.5f * scale;
    dl.AddRect(ImVec2(center.x - half, center.y - half), ImVec2(center.x + half, center.y + half), IM_COL32_WHITE, rounding,
      ImDrawFlags_None, stroke);
    return;
  }
  const float half = GLYPH_RESTORE_SIZE * 0.5f * scale;
  const float shift = GLYPH_RESTORE_STEP * 0.5f * scale;
  const ImVec2 front(center.x - shift, center.y + shift);
  const ImVec2 back(center.x + shift, center.y - shift);
  dl.AddRect(ImVec2(front.x - half, front.y - half), ImVec2(front.x + half, front.y + half), IM_COL32_WHITE, rounding,
    ImDrawFlags_None, stroke);
  dl.PathLineTo(ImVec2(back.x - half, front.y - half));
  dl.PathArcToFast(ImVec2(back.x - half + rounding, back.y - half + rounding), rounding, 6, 9);
  dl.PathArcToFast(ImVec2(back.x + half - rounding, back.y - half + rounding), rounding, 9, 12);
  dl.PathArcToFast(ImVec2(back.x + half - rounding, back.y + half - rounding), rounding, 0, 3);
  dl.PathLineTo(ImVec2(front.x + half, back.y + half));
  dl.PathStroke(IM_COL32_WHITE, ImDrawFlags_None, stroke);
}

bool icon_button(ImDrawList &dl, const char *id, const ImVec2 &min, float size, IconGlyph glyph)
{
  ImGui::SetCursorScreenPos(min);
  const bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size));
  if (ImGui::IsItemHovered() || ImGui::IsItemActive())
  {
    dl.AddRectFilled(min, ImVec2(min.x + size, min.y + size), SUBGRAPH_WINDOW_BUTTON_COLOR, scaled(WINDOW_ICON_ROUNDING));
  }
  draw_icon_glyph(dl, ImVec2(min.x + size * 0.5f, min.y + size * 0.5f), size / WINDOW_BUTTON_SIZE, glyph);
  return clicked;
}

void draw_stack_tiles(ImDrawList &dl, const ImVec2 &min, bool several, bool hot, bool changed)
{
  const float tile = scaled(STACK_TILE);
  const float rounding = scaled(STACK_TILE_ROUNDING);
  auto drawTile = [&](const ImVec2 &tile_min, ImU32 fill, const char *icon_name, ImU32 icon_tint) {
    const ImVec2 tileMax(tile_min.x + tile, tile_min.y + tile);
    dl.AddRectFilled(tile_min, tileMax, fill, rounding);
    if (const ImTextureID icon = DAEDITOR3.getPropPanelService()->getIconTextureId(icon_name, hdpi::_pxS(STACK_TILE)))
    {
      dl.AddImage(icon, tile_min, tileMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), icon_tint);
    }
  };
  if (several)
  {
    drawTile(ImVec2(min.x + scaled(STACK_BACK_STEP), min.y), ImGui::GetColorU32(SUBGRAPH_CARD_HOVER_COLOR, STACK_BACK_ALPHA),
      STACK_BACK_ICON, ImGui::GetColorU32(IM_COL32_WHITE, STACK_BACK_ALPHA));
  }
  drawTile(min, hot ? SUBGRAPH_CARD_HOVER_COLOR : SUBGRAPH_CARD_COLOR, changed ? STACK_CHANGED_ICON : STACK_ICON, IM_COL32_WHITE);
}

// A bottom corner has the size of the design's grip, a top one keeps clear of the header's buttons; an edge runs
// between its corners.
ImRect grip_rect(int sides, const ImVec2 &min, const ImVec2 &max)
{
  const float edge = scaled(WINDOW_RESIZE_EDGE);
  const float grip = scaled(WINDOW_RESIZE_GRIP);
  const bool horizontal = (sides & (SIDE_LEFT | SIDE_RIGHT)) != 0;
  const bool vertical = (sides & (SIDE_TOP | SIDE_BOTTOM)) != 0;
  if (horizontal && vertical)
  {
    const float size = (sides & SIDE_TOP) ? edge : grip;
    const float x = (sides & SIDE_LEFT) ? min.x : max.x - size;
    const float y = (sides & SIDE_TOP) ? min.y : max.y - size;
    return ImRect(x, y, x + size, y + size);
  }
  if (horizontal)
  {
    const float x = (sides & SIDE_LEFT) ? min.x : max.x - edge;
    return ImRect(x, min.y + edge, x + edge, max.y - grip);
  }
  const float inset = (sides & SIDE_TOP) ? edge : grip;
  const float y = (sides & SIDE_TOP) ? min.y : max.y - edge;
  return ImRect(min.x + inset, y, max.x - inset, y + edge);
}

// False when the area stopped the end short.
bool grab_axis(float &lo, float &hi, bool move_lo, bool move_hi, float delta, float area_lo, float area_hi, float min_size)
{
  if (move_lo && move_hi)
  {
    const float size = hi - lo;
    const float wanted = lo + delta;
    lo = eastl::clamp(wanted, area_lo, eastl::max(area_lo, area_hi - size));
    hi = lo + size;
    return lo == wanted;
  }
  if (move_lo)
  {
    const float wanted = lo + delta;
    lo = eastl::clamp(wanted, area_lo, eastl::max(area_lo, hi - min_size));
    return lo == wanted;
  }
  if (move_hi)
  {
    const float wanted = hi + delta;
    hi = eastl::clamp(wanted, eastl::min(area_hi, lo + min_size), area_hi);
    return hi == wanted;
  }
  return true;
}

void store_axis(float &offset, float &size, float lo, float hi, bool move_lo, bool move_hi)
{
  if (move_lo || move_hi)
  {
    offset = lo;
  }
  if (move_lo != move_hi)
  {
    size = hi - lo;
  }
}

// The shape of ImGui's own resize grip.
void draw_corner_grip(ImDrawList &dl, const ImVec2 &corner, ImU32 color)
{
  const float size = scaled(WINDOW_RESIZE_GRIP);
  const float rounding = scaled(WINDOW_ROUNDING);
  dl.PathLineTo(ImVec2(corner.x - size, corner.y));
  dl.PathLineTo(ImVec2(corner.x, corner.y - size));
  dl.PathArcToFast(ImVec2(corner.x - rounding, corner.y - rounding), rounding, 0, 3);
  dl.PathFillConvex(color);
}

void draw_hot_sides(ImDrawList &dl, const ImVec2 &min, const ImVec2 &max, int sides)
{
  const float line = scaled(WINDOW_RESIZE_LINE);
  const float rounding = scaled(WINDOW_ROUNDING);
  if (sides & SIDE_LEFT)
  {
    dl.AddRectFilled(ImVec2(min.x, min.y + rounding), ImVec2(min.x + line, max.y - rounding), SUBGRAPH_WINDOW_RESIZE_HOT_COLOR);
  }
  if (sides & SIDE_RIGHT)
  {
    dl.AddRectFilled(ImVec2(max.x - line, min.y + rounding), ImVec2(max.x, max.y - rounding), SUBGRAPH_WINDOW_RESIZE_HOT_COLOR);
  }
  if (sides & SIDE_TOP)
  {
    dl.AddRectFilled(ImVec2(min.x + rounding, min.y), ImVec2(max.x - rounding, min.y + line), SUBGRAPH_WINDOW_RESIZE_HOT_COLOR);
  }
  if (sides & SIDE_BOTTOM)
  {
    dl.AddRectFilled(ImVec2(min.x + rounding, max.y - line), ImVec2(max.x - rounding, max.y), SUBGRAPH_WINDOW_RESIZE_HOT_COLOR);
  }
}
} // namespace

SubgraphWindow::SubgraphWindow(GraphEditorPlg &plugin, int tab_id, GraphTab &root, SubgraphWindow *opened_from, const char *file_stem,
  const SubgraphWindowLayout &last_layout) :
  parent(opened_from), stem(file_stem), tab(eastl::make_unique<GraphTab>(plugin, tab_id, &root, this)), layout(last_layout)
{}

SubgraphWindow::~SubgraphWindow() = default;

void SubgraphWindow::unminimize()
{
  if (minimizeStamp != 0)
  {
    minimizeStamp = 0;
    tab->onActivated();
  }
}

void SubgraphWindow::placeDefault(const ImVec2 &area_size, const eastl::vector<eastl::unique_ptr<SubgraphWindow>> &windows)
{
  const float margin = scaled(WINDOW_MARGIN);
  const float step = scaled(WINDOW_STACK_STEP);
  layout.size = ImVec2(scaled(WINDOW_DEFAULT_WIDTH), scaled(WINDOW_DEFAULT_HEIGHT));
  ImVec2 offset(area_size.x - margin - layout.size.x, area_size.y - margin - layout.size.y);
  // Moves up-left only, so it ends.
  for (bool stepped = true; stepped;)
  {
    stepped = false;
    for (const eastl::unique_ptr<SubgraphWindow> &w : windows)
    {
      if (w.get() != this && w->layout.valid && fabsf(w->layout.offset.x - offset.x) < step * 0.5f &&
          fabsf(w->layout.offset.y - offset.y) < step * 0.5f)
      {
        offset = ImVec2(offset.x - step, offset.y - step);
        stepped = true;
      }
    }
  }
  layout.offset = offset;
  layout.valid = true;
}

// Not stored: the dock window can shrink and grow back.
SubgraphWindowLayout SubgraphWindow::fitted(const ImVec2 &area_size) const
{
  const float margin = scaled(WINDOW_MARGIN);
  SubgraphWindowLayout fit = layout;
  const ImVec2 room(area_size.x - 2.0f * margin, area_size.y - 2.0f * margin);
  const ImVec2 wanted = layout.maximized ? room : layout.size;
  fit.size = ImVec2(eastl::max(1.0f, eastl::min(wanted.x, room.x)), eastl::max(1.0f, eastl::min(wanted.y, room.y)));
  fit.offset = ImVec2(eastl::clamp(layout.offset.x, margin, eastl::max(margin, area_size.x - margin - fit.size.x)),
    eastl::clamp(layout.offset.y, margin, eastl::max(margin, area_size.y - margin - fit.size.y)));
  return fit;
}

void SubgraphWindow::startGrab(int sides, const SubgraphWindowLayout &from)
{
  if (ImGui::IsItemActivated())
  {
    // A right or middle press activates the item too, and must not replay the last left drag.
    grabSides = ImGui::IsMouseClicked(ImGuiMouseButton_Left) ? sides : 0;
    grabId = ImGui::GetItemID();
    grabStart = from;
  }
}

bool SubgraphWindow::followGrab(const ImVec2 &delta, const ImVec2 &area_size)
{
  // A press alone stores nothing. Unlike the delta, the farthest distance stays above zero once the mouse moved.
  if (ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] == 0.0f)
  {
    return true;
  }
  const float margin = scaled(WINDOW_MARGIN);
  const float minSize = scaled(WINDOW_MIN_SIZE);
  float left = grabStart.offset.x;
  float right = left + grabStart.size.x;
  float top = grabStart.offset.y;
  float bottom = top + grabStart.size.y;
  const bool followedX = grab_axis(left, right, (grabSides & SIDE_LEFT) != 0, (grabSides & SIDE_RIGHT) != 0, delta.x, margin,
    area_size.x - margin, minSize);
  const bool followedY = grab_axis(top, bottom, (grabSides & SIDE_TOP) != 0, (grabSides & SIDE_BOTTOM) != 0, delta.y, margin,
    area_size.y - margin, minSize);
  // Only what the grab changes: the fitted geometry it starts from is smaller while the dock window is.
  store_axis(layout.offset.x, layout.size.x, left, right, (grabSides & SIDE_LEFT) != 0, (grabSides & SIDE_RIGHT) != 0);
  store_axis(layout.offset.y, layout.size.y, top, bottom, (grabSides & SIDE_TOP) != 0, (grabSides & SIDE_BOTTOM) != 0);
  return followedX && followedY;
}

SubgraphWindow::DrawResult SubgraphWindow::draw(const SubgraphWindowFrame &frame)
{
  DrawResult result;
  const ImVec2 areaSize(frame.areaMax.x - frame.areaMin.x, frame.areaMax.y - frame.areaMin.y);
  // Before the child takes its rect, so the window does not trail the mouse by a frame.
  const bool grabbed = grabSides != 0 && ImGui::GetActiveID() == grabId;
  const bool grabStopped = grabbed && !followGrab(ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f), areaSize);
  const SubgraphWindowLayout fit = fitted(areaSize);
  const ImVec2 winMin(frame.areaMin.x + fit.offset.x, frame.areaMin.y + fit.offset.y);
  const ImVec2 winMax(winMin.x + fit.size.x, winMin.y + fit.size.y);
  const float rounding = scaled(WINDOW_ROUNDING);

  ImGui::SetCursorScreenPos(winMin);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  const eastl::string childId(eastl::string::CtorSprintf(), "subgraph_window_%d", tab->getId());
  const bool visible = ImGui::BeginChild(childId.c_str(), fit.size, ImGuiChildFlags_None, CANVAS_CHILD_FLAGS);
  if (visible)
  {
    const float headerH = scaled(WINDOW_HEADER_HEIGHT);
    const float footerH = scaled(WINDOW_FOOTER_HEIGHT);
    const float pad = scaled(WINDOW_PAD);
    const float buttonSize = scaled(WINDOW_BUTTON_SIZE);
    const float buttonGap = scaled(WINDOW_BUTTON_GAP);
    const ImVec2 closeMin(winMax.x - pad - buttonSize, winMin.y + (headerH - buttonSize) * 0.5f);
    const ImVec2 maximizeMin(closeMin.x - buttonGap - buttonSize, closeMin.y);
    const ImVec2 minimizeMin(maximizeMin.x - buttonGap - buttonSize, closeMin.y);

    const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    // ImGui focuses on a left press only, and too late: the root canvas would take a middle drag.
    const bool pressed = hovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
                                      ImGui::IsMouseClicked(ImGuiMouseButton_Middle));
    // Minimize and close take the window away, so a press on them leaves the focus as it is.
    auto underMouse = [&](const ImVec2 &button_min) {
      return ImRect(button_min, ImVec2(button_min.x + buttonSize, button_min.y + buttonSize)).Contains(ImGui::GetMousePos());
    };
    const bool pressOnLeave = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && (underMouse(minimizeMin) || underMouse(closeMin));
    if (focusRequested || (pressed && !pressOnLeave && !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)))
    {
      ImGui::SetWindowFocus();
    }
    focusRequested = false;

    ImDrawList &dl = *ImGui::GetWindowDrawList();
    if (frame.active || hovered)
    {
      draw_floating_shadow(dl, winMin, winMax, frame.areaMin, frame.canvasMax);
    }
    dl.AddRectFilled(winMin, winMax, GRAPH_CANVAS_BG_COLOR, rounding);
    dl.AddRectFilled(winMin, ImVec2(winMax.x, winMin.y + headerH), SUBGRAPH_WINDOW_HEADER_COLOR, rounding,
      ImDrawFlags_RoundCornersTop);

    // The controls go before the header and window items under them: an item that allows overlap keeps the
    // hover for the frame the pointer reaches a later one, and would take a press made in it.
    ImRect nameRect;
    result.crumbClicked = drawCrumbs(dl, ImVec2(winMin.x + pad, winMin.y), minimizeMin.x - pad, frame, nameRect);
    if (icon_button(dl, "maximize", maximizeMin, buttonSize, fit.maximized ? IconGlyph::Restore : IconGlyph::Maximize))
    {
      layout.maximized = !fit.maximized;
    }
    ImGui::PushItemFlag(ImGuiItemFlags_NoFocus, true);
    result.minimizeClicked = icon_button(dl, "minimize", minimizeMin, buttonSize, IconGlyph::Minimize);
    result.closeClicked = drawCloseButton(dl, closeMin, buttonSize);
    ImGui::PopItemFlag();
    // Before the grips: the bottom-right one covers the corner of Save as. ne clips the canvas to its
    // rect, so the canvas pass after it paints nothing over the footer.
    drawFooter(dl, ImVec2(winMin.x, winMax.y - footerH), winMax, result);

    // Before the canvas pass, so that ne's items, submitted later, cannot take the hover from them.
    int hotSides = 0;
    ImGuiMouseCursor cursor = (grabbed && grabSides == SIDES_ALL) ? ImGuiMouseCursor_ResizeAll : ImGuiMouseCursor_COUNT;
    for (const ResizeGrip &grip : RESIZE_GRIPS)
    {
      const ImRect r = grip_rect(grip.sides, winMin, winMax);
      if (fit.maximized || r.GetWidth() <= 0.0f || r.GetHeight() <= 0.0f)
      {
        continue;
      }
      ImGui::SetCursorScreenPos(r.Min);
      ImGui::PushID(grip.sides);
      ImGui::InvisibleButton("grip", r.GetSize()); // left only, so a right press over the canvas still opens its menu
      ImGui::PopID();
      startGrab(grip.sides, fit);
      if (ImGui::IsItemHovered() || ImGui::IsItemActive())
      {
        hotSides = grip.sides;
        cursor = grip.cursor;
      }
    }

    ImGui::SetCursorScreenPos(winMin);
    ImGui::InvisibleButton("header", ImVec2(fit.size.x, headerH), ALL_MOUSE_BUTTONS);
    startGrab(fit.maximized ? 0 : SIDES_ALL, fit);
    drawNameTooltip(nameRect, false);
    // No empty space: a press there would move the dock window. It allows overlap, so the canvas after it takes the hover.
    ImGui::SetCursorScreenPos(winMin);
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("window", fit.size, ALL_MOUSE_BUTTONS);

    // Edge to edge, as the design's padding has the canvas color; the 4px floor keeps the size positive.
    const ImVec2 contentMin(winMin.x, winMin.y + headerH);
    const ImVec2 contentMax(eastl::max(contentMin.x + 4.0f, winMax.x), eastl::max(contentMin.y + 4.0f, winMax.y - footerH));
    tab->updateCanvas(contentMin, contentMax);

    const int cornerGrip = SIDE_RIGHT | SIDE_BOTTOM;
    if (!fit.maximized)
    {
      draw_corner_grip(dl, winMax, hotSides == cornerGrip ? SUBGRAPH_WINDOW_RESIZE_HOT_COLOR : SUBGRAPH_WINDOW_GRIP_COLOR);
    }
    // After the canvas pass, so these lie over the nodes.
    if (frame.active)
    {
      draw_focus_frame(dl, winMin, winMax, rounding, frame.failed);
    }
    else
    {
      if (!hovered)
      {
        dl.AddRectFilled(winMin, winMax, SUBGRAPH_WINDOW_DIM_COLOR, rounding);
      }
      const float border = static_cast<float>(eastl::max(1, hdpi::_pxS(1)));
      const float inset = border * 0.5f;
      dl.AddRect(ImVec2(winMin.x + inset, winMin.y + inset), ImVec2(winMax.x - inset, winMax.y - inset), SUBGRAPH_WINDOW_BORDER_COLOR,
        rounding, ImDrawFlags_None, border);
    }
    if (hotSides != cornerGrip)
    {
      draw_hot_sides(dl, winMin, winMax, hotSides);
    }
    // After the canvas pass too: ne::End sets its own.
    if (grabStopped)
    {
      cursor = ImGuiMouseCursor_NotAllowed;
    }
    if (cursor != ImGuiMouseCursor_COUNT)
    {
      ImGui::SetMouseCursor(cursor);
    }
    result.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  return result;
}

// Returns the parent crumb clicked, or -1. `name_rect`: where the name was drawn, empty if it was not.
int SubgraphWindow::drawCrumbs(ImDrawList &dl, const ImVec2 &min, float max_x, const SubgraphWindowFrame &frame,
  ImRect &name_rect) const
{
  // RenderTextEllipsis measures and draws with the current font, in ImGuiCol_Text.
  ImGui::PushFont(nullptr, WINDOW_FONT_SIZE);
  const float headerH = scaled(WINDOW_HEADER_HEIGHT);
  ImVec2 pos(min.x, min.y + (headerH - ImGui::GetFontSize()) * 0.5f);
  const ImVec2 textMax(max_x, min.y + headerH);
  // A narrow window ends the crumbs with an ellipsis, as the design's smallest one does. The size comes
  // measured, so the ellipsis call need not measure again.
  auto drawText = [&](const char *text, const ImVec2 &size, ImU32 color) {
    if (pos.x < textMax.x)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, color);
      ImGui::RenderTextEllipsis(&dl, pos, textMax, textMax.x, text, text + strlen(text), &size);
      ImGui::PopStyleColor();
    }
    pos.x += size.x;
  };
  const char *const separator = " / ";
  const ImVec2 separatorSize = ImGui::CalcTextSize(separator);
  int clicked = -1;
  for (int i = 0; i < static_cast<int>(frame.parentNames.size()) && pos.x < textMax.x; ++i)
  {
    // The name jumps to its level, the separator does not.
    const char *name = frame.parentNames[i].c_str();
    const ImVec2 nameSize = ImGui::CalcTextSize(name);
    ImGui::SetCursorScreenPos(pos);
    ImGui::PushID(i);
    if (ImGui::InvisibleButton("crumb", ImVec2(eastl::min(nameSize.x, textMax.x - pos.x), ImGui::GetFontSize())))
    {
      clicked = i;
    }
    ImGui::PopID();
    const bool hot = ImGui::IsItemHovered() || ImGui::IsItemActive();
    drawText(name, nameSize, hot ? SUBGRAPH_WINDOW_CRUMB_HOVER_COLOR : SUBGRAPH_WINDOW_PARENT_CRUMB_COLOR);
    drawText(separator, separatorSize, SUBGRAPH_WINDOW_PARENT_CRUMB_COLOR);
  }
  pos.x += scaled(WINDOW_CRUMB_GAP);
  name_rect = ImRect(pos, pos);
  if (pos.x < textMax.x)
  {
    const eastl::string name = shownName();
    const ImVec2 nameSize = ImGui::CalcTextSize(name.c_str(), name.c_str() + name.size());
    name_rect.Max = ImVec2(eastl::min(textMax.x, pos.x + nameSize.x), pos.y + ImGui::GetFontSize());
    drawText(name.c_str(), nameSize, IM_COL32_WHITE);
  }
  ImGui::PopFont();
  return clicked;
}

void SubgraphWindow::drawFooter(ImDrawList &dl, const ImVec2 &min, const ImVec2 &max, DrawResult &result) const
{
  // A Button paints when submitted, so the band goes first.
  dl.AddRectFilled(min, max, SUBGRAPH_WINDOW_FOOTER_COLOR, scaled(WINDOW_ROUNDING), ImDrawFlags_RoundCornersBottom);
  const float gap = scaled(WINDOW_BUTTON_GAP);
  const float textPad = scaled(WINDOW_FOOTER_BUTTON_PAD);
  const float buttonH = scaled(WINDOW_FOOTER_BUTTON_HEIGHT);
  const ImVec2 saveSize(ImGui::CalcTextSize("Save").x + 2.0f * textPad, buttonH);
  const ImVec2 saveAsSize(ImGui::CalcTextSize("Save as").x + 2.0f * textPad, buttonH);
  ImGui::SetCursorScreenPos(
    ImVec2(max.x - scaled(WINDOW_PAD) - saveAsSize.x - gap - saveSize.x, min.y + (max.y - min.y - buttonH) * 0.5f));
  ImGui::PushStyleColor(ImGuiCol_Button, SUBGRAPH_WINDOW_BUTTON_COLOR);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SUBGRAPH_WINDOW_BUTTON_HOVER_COLOR);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, SUBGRAPH_WINDOW_BUTTON_ACTIVE_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Border, SUBGRAPH_WINDOW_BUTTON_BORDER_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32_WHITE);
  result.saveClicked = ImGui::Button("Save", saveSize);
  ImGui::SameLine(0.0f, gap);
  result.saveAsClicked = ImGui::Button("Save as", saveAsSize);
  ImGui::PopStyleColor(5);
}

bool SubgraphWindow::drawCloseButton(ImDrawList &dl, const ImVec2 &min, float size) const
{
  ImGui::SetCursorScreenPos(min);
  const bool clicked = ImGui::InvisibleButton("close", ImVec2(size, size));
  const bool hot = ImGui::IsItemHovered() || ImGui::IsItemActive();
  if (hot)
  {
    dl.AddRectFilled(min, ImVec2(min.x + size, min.y + size), SUBGRAPH_WINDOW_CLOSE_HOVER_COLOR, scaled(WINDOW_CLOSE_ROUNDING));
  }
  const float half = scaled(hot ? WINDOW_CLOSE_GLYPH_HOT : WINDOW_CLOSE_GLYPH) * 0.5f;
  const ImVec2 c(min.x + size * 0.5f, min.y + size * 0.5f);
  const float stroke = scaled(WINDOW_CLOSE_STROKE);
  dl.AddLine(ImVec2(c.x - half, c.y - half), ImVec2(c.x + half, c.y + half), IM_COL32_WHITE, stroke);
  dl.AddLine(ImVec2(c.x - half, c.y + half), ImVec2(c.x + half, c.y - half), IM_COL32_WHITE, stroke);
  return clicked;
}

void SubgraphWindow::drawCard(ImDrawList &dl, const ImVec2 &min, bool hovered, SubgraphStackResult &result)
{
  const ImVec2 max(min.x + scaled(CARD_WIDTH), min.y + scaled(CARD_HEIGHT));
  const float rounding = scaled(WINDOW_ROUNDING);
  dl.AddRectFilled(min, max, hovered ? SUBGRAPH_CARD_HOVER_COLOR : SUBGRAPH_CARD_COLOR, rounding);
  const float border = scaled(CARD_BORDER);
  dl.AddRect(ImVec2(min.x + border * 0.5f, min.y + border * 0.5f), ImVec2(max.x - border * 0.5f, max.y - border * 0.5f),
    SUBGRAPH_CARD_BORDER_COLOR, rounding, ImDrawFlags_None, border);

  const float inset = border + scaled(WINDOW_PAD);
  const float rowH = scaled(CARD_HEADER_HEIGHT);
  const float buttonSize = scaled(WINDOW_BUTTON_SIZE);
  const float gap = scaled(WINDOW_BUTTON_GAP);
  const ImVec2 closeMin(max.x - inset - buttonSize, min.y + inset + (rowH - buttonSize) * 0.5f);
  const ImVec2 maximizeMin(closeMin.x - gap - buttonSize, closeMin.y);
  const ImVec2 restoreMin(maximizeMin.x - gap - buttonSize, closeMin.y);
  ImGui::PushID(tab->getId());
  if (icon_button(dl, "restore", restoreMin, buttonSize, IconGlyph::Restore))
  {
    layout.maximized = false;
    result.shown = this;
  }
  if (icon_button(dl, "maximize", maximizeMin, buttonSize, IconGlyph::Maximize))
  {
    layout.maximized = true;
    result.shown = this;
  }
  if (drawCloseButton(dl, closeMin, buttonSize))
  {
    result.closed = this;
  }

  ImGui::PushFont(nullptr, WINDOW_FONT_SIZE);
  const eastl::string name = shownName();
  const ImVec2 nameSize = ImGui::CalcTextSize(name.c_str(), name.c_str() + name.size());
  const ImVec2 namePos(min.x + inset, min.y + inset + (rowH - ImGui::GetFontSize()) * 0.5f);
  const ImVec2 nameMax(restoreMin.x - gap, min.y + inset + rowH);
  const ImRect nameRect(namePos, ImVec2(eastl::min(nameMax.x, namePos.x + nameSize.x), namePos.y + ImGui::GetFontSize()));
  ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32_WHITE);
  ImGui::RenderTextEllipsis(&dl, namePos, nameMax, nameMax.x, name.c_str(), name.c_str() + name.size(), &nameSize);
  ImGui::PopStyleColor();
  ImGui::PopFont();

  ImGui::SetCursorScreenPos(min);
  if (ImGui::InvisibleButton("card", ImVec2(max.x - min.x, max.y - min.y)))
  {
    result.shown = this;
  }
  drawNameTooltip(nameRect, namePos.x + nameSize.x > nameMax.x);
  ImGui::PopID();
}

eastl::string SubgraphWindow::shownName() const
{
  return eastl::string(eastl::string::CtorSprintf(), "%s%s", tab->isModified() ? "*" : "", stem.c_str());
}

void SubgraphWindow::drawNameTooltip(const ImRect &name_rect, bool cut) const
{
  if ((cut || tab->isModified()) && name_rect.Contains(ImGui::GetMousePos()) && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) &&
      ImGui::BeginTooltip())
  {
    ImGui::TextUnformatted(stem.c_str());
    if (tab->isModified())
    {
      ImGui::TextDisabled("UNSAVED CHANGES");
    }
    ImGui::EndTooltip();
  }
}

float subgraph_stack_width(int minimized_count) { return scaled(STACK_TILE) + (minimized_count > 1 ? scaled(STACK_BACK_STEP) : 0.0f); }

SubgraphStackResult draw_subgraph_stack(const ImRect &stack, const ImVec2 &canvas_min, const ImVec2 &canvas_max,
  dag::ConstSpan<SubgraphWindow *> minimized, bool &cards_shown)
{
  SubgraphStackResult result;
  const int count = static_cast<int>(minimized.size());
  const ImVec2 cardSize(scaled(CARD_WIDTH), scaled(CARD_HEIGHT));
  const float gap = scaled(CARD_GAP);
  const int perRow =
    eastl::clamp(static_cast<int>((stack.Max.x - canvas_min.x - scaled(WINDOW_MARGIN) + gap) / (cardSize.x + gap)), 1, count);
  const int rows = (count + perRow - 1) / perRow;
  const ImVec2 cardsMax(stack.Max.x, stack.Min.y - scaled(STACK_CARDS_GAP));
  const ImVec2 cardsSize(perRow * cardSize.x + (perRow - 1) * gap, rows * cardSize.y + (rows - 1) * gap);
  const ImVec2 cardsMin(cardsMax.x - cardsSize.x, cardsMax.y - cardsSize.y);
  const bool graphWindowHovered =
    ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

  // Children, so the canvas under them takes no hover; NoFocus items keep ImGui's focus on the surface the keys go to.
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::SetCursorScreenPos(stack.Min);
  const bool stackVisible = ImGui::BeginChild("subgraph_stack", stack.GetSize(), ImGuiChildFlags_None, CANVAS_CHILD_FLAGS);
  bool stackHovered = false;
  if (stackVisible)
  {
    ImGui::PushItemFlag(ImGuiItemFlags_NoFocus, true);
    ImGui::InvisibleButton("stack", stack.GetSize(), ALL_MOUSE_BUTTONS);
    ImGui::PopItemFlag();
    stackHovered = ImGui::IsItemHovered();
  }
  cards_shown = stackHovered ||
                (cards_shown && graphWindowHovered && ImGui::IsMouseHoveringRect(cardsMin, ImVec2(cardsMax.x, stack.Min.y), false));
  if (stackVisible)
  {
    const bool changed =
      eastl::any_of(minimized.begin(), minimized.end(), [](const SubgraphWindow *w) { return w->getTab().isModified(); });
    draw_stack_tiles(*ImGui::GetWindowDrawList(), stack.Min, count > 1, cards_shown, changed);
  }
  ImGui::EndChild();

  if (cards_shown)
  {
    ImGui::SetCursorScreenPos(cardsMin);
    if (ImGui::BeginChild("subgraph_cards", cardsSize, ImGuiChildFlags_None, CANVAS_CHILD_FLAGS))
    {
      ImGui::PushItemFlag(ImGuiItemFlags_NoFocus, true);
      ImDrawList &dl = *ImGui::GetWindowDrawList();
      auto cardRect = [&](int i) {
        const int fromNewest = count - 1 - i;
        const int col = fromNewest % perRow;
        const int row = fromNewest / perRow;
        const ImVec2 min(cardsMax.x - (col + 1) * cardSize.x - col * gap, cardsMax.y - (row + 1) * cardSize.y - row * gap);
        return ImRect(min, ImVec2(min.x + cardSize.x, min.y + cardSize.y));
      };
      int hoveredCard = -1;
      if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
      {
        for (int i = 0; i < count && hoveredCard < 0; ++i)
        {
          const ImRect card = cardRect(i);
          if (ImGui::IsMouseHoveringRect(card.Min, card.Max))
          {
            hoveredCard = i;
          }
        }
      }
      for (int i = 0; i < count; ++i)
      {
        if (i != hoveredCard)
        {
          minimized[i]->drawCard(dl, cardRect(i).Min, false, result);
        }
      }
      // Last, so its shadow falls on its neighbours.
      if (hoveredCard >= 0)
      {
        const ImRect card = cardRect(hoveredCard);
        draw_floating_shadow(dl, card.Min, card.Max, canvas_min, canvas_max);
        minimized[hoveredCard]->drawCard(dl, card.Min, true, result);
      }
      ImGui::SetCursorScreenPos(cardsMin);
      ImGui::InvisibleButton("cards", cardsSize, ALL_MOUSE_BUTTONS);
      ImGui::PopItemFlag();
    }
    ImGui::EndChild();
  }
  ImGui::PopStyleVar();
  return result;
}

void draw_focus_frame(ImDrawList &dl, const ImVec2 &min, const ImVec2 &max, float rounding, bool failed)
{
  const float border = scaled(WINDOW_FOCUS_BORDER);
  const float inset = border * 0.5f;
  dl.AddRect(ImVec2(min.x + inset, min.y + inset), ImVec2(max.x - inset, max.y - inset),
    failed ? GRAPH_ERROR_BORDER_COLOR : SUBGRAPH_WINDOW_FOCUS_COLOR, rounding, ImDrawFlags_None, border);
}

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>
#include <propPanel/focusHelper.h>
#include "control/spinEditStandalone.h"
#include "c_constants.h"
#include <drv/3d/dag_resId.h>
#include <gui/dag_imguiUtil.h>
#include <libTools/util/hdpiUtil.h>
#include <imgui/imgui_internal.h>
#include <drv/3d/dag_sampler.h>

using namespace ImGui;

namespace PropPanel
{

bool ImguiHelper::checkboxDragSelectionInProgress = false;
bool ImguiHelper::checkboxDragSelectionValue = false;

void ImguiHelper::afterNewFrame()
{
  if (checkboxDragSelectionInProgress && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    checkboxDragSelectionInProgress = false;
}

float ImguiHelper::getDefaultRightSideEditWidth()
{
  return hdpi::_pxS(DEFAULT_CONTROL_WIDTH) + SpinEditControlStandalone::getSpaceBeforeSpinButtons() +
         SpinEditControlStandalone::getSpinButtonsSize().x;
}

// Taken from imgui_widgets.cpp.
static void TreeNodeStoreStackData(ImGuiTreeNodeFlags flags)
{
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = g.CurrentWindow;

  g.TreeNodeStack.resize(g.TreeNodeStack.Size + 1);
  ImGuiTreeNodeStackData *tree_node_data = &g.TreeNodeStack.back();
  tree_node_data->ID = g.LastItemData.ID;
  tree_node_data->TreeFlags = flags;
  tree_node_data->InFlags = g.LastItemData.InFlags;
  tree_node_data->NavRect = g.LastItemData.NavRect;
  window->DC.TreeHasStackDataDepthMask |= (1 << window->DC.TreeDepth);
}

// Taken from imgui_widgets.cpp.
// clang-format off
static bool IsRootOfOpenMenuSet()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if ((g.OpenPopupStack.Size <= g.BeginPopupStack.Size) || (window->Flags & ImGuiWindowFlags_ChildMenu))
        return false;

    // Initially we used 'upper_popup->OpenParentId == window->IDStack.back()' to differentiate multiple menu sets from each others
    // (e.g. inside menu bar vs loose menu items) based on parent ID.
    // This would however prevent the use of e.g. PushID() user code submitting menus.
    // Previously this worked between popup and a first child menu because the first child menu always had the _ChildWindow flag,
    // making hovering on parent popup possible while first child menu was focused - but this was generally a bug with other side effects.
    // Instead we don't treat Popup specifically (in order to consistently support menu features in them), maybe the first child menu of a Popup
    // doesn't have the _ChildWindow flag, and we rely on this IsRootOfOpenMenuSet() check to allow hovering between root window/popup and first child menu.
    // In the end, lack of ID check made it so we could no longer differentiate between separate menu sets. To compensate for that, we at least check parent window nav layer.
    // This fixes the most common case of menu opening on hover when moving between window content and menu bar. Multiple different menu sets in same nav layer would still
    // open on hover, but that should be a lesser problem, because if such menus are close in proximity in window content then it won't feel weird and if they are far apart
    // it likely won't be a problem anyone runs into.
    const ImGuiPopupData* upper_popup = &g.OpenPopupStack[g.BeginPopupStack.Size];
    if (window->DC.NavLayerCurrent != upper_popup->ParentNavLayer)
        return false;
    return upper_popup->Window && (upper_popup->Window->Flags & ImGuiWindowFlags_ChildMenu) && ImGui::IsWindowChildOf(upper_popup->Window, window, true, false);
}
// clang-format on

static float getMenuCheckmarkSize(const ImGuiContext &context) { return context.FontSize * 0.866f; }
static float getMenuCheckmarkPadding(const ImGuiContext &context) { return context.FontSize * 0.40f; }

bool ImguiHelper::checkboxWithDragSelection(const char *label, bool *value)
{
  if (ImGui::GetCurrentWindow()->SkipItems)
    return false;

  const bool oldValue = *value;
  bool changed = ImGui::Checkbox(label, value);

  if (!checkboxDragSelectionInProgress && ImGui::IsItemActivated() && ImGui::IsItemClicked(ImGuiMouseButton_Left))
  {
    checkboxDragSelectionInProgress = true;
    checkboxDragSelectionValue = !oldValue;

    // Prevent double checking with drag selection. Without this dragging from checkbox A to B, and then back to A would
    // toggle A twice (first by the drag selection, then by ImGui::Checkbox on mouse release).
    ImGui::ClearActiveID();
  }

  if (checkboxDragSelectionInProgress && checkboxDragSelectionValue != *value && ImGui::IsItemHovered())
  {
    *value = checkboxDragSelectionValue;
    changed = true;
  }

  return changed;
}

bool ImguiHelper::inputTextWithEnterWorkaround(const char *label, String *str, bool focused, ImGuiInputTextFlags flags,
  ImGuiInputTextCallback callback, void *user_data)
{
  class ImGuiKeyDownHider
  {
  public:
    explicit ImGuiKeyDownHider(ImGuiKey key)
    {
      keyData = ImGui::GetKeyData(key);
      if (keyData->Down)
      {
        originalKeyData = *keyData;
        keyData->Down = false;
        keyData->DownDuration = -1.0f;
        keyData->DownDurationPrev = -1.0f;
      }
      else
      {
        originalKeyData.Down = false;
      }
    }

    ~ImGuiKeyDownHider()
    {
      if (originalKeyData.Down)
        *keyData = originalKeyData;
    }

  private:
    ImGuiKeyData *keyData;
    ImGuiKeyData originalKeyData;
  };

  if (!focused)
    return ImGuiDagor::InputText(label, str, flags, callback, user_data);

  ImGuiKeyDownHider enterKeyHider(ImGuiKey_Enter);
  ImGuiKeyDownHider keypadEnterHider(ImGuiKey_KeypadEnter);
  return ImGuiDagor::InputText(label, str, flags, callback, user_data);
}

// Based on ImGui::LabelText.
void ImguiHelper::labelOnly(const char *label, const char *label_end, bool use_text_width)
{
  ImGuiWindow *window = GetCurrentWindow();
  if (window->SkipItems)
    return;

  const ImGuiStyle &style = GImGui->Style;
  const ImVec2 pos = window->DC.CursorPos;
  const float width = CalcItemWidth();
  const ImVec2 size = CalcTextSize(label, label_end);
  const ImRect bb(pos, pos + ImVec2(use_text_width ? size.x : width, size.y + (style.FramePadding.y * 2)));
  ItemSize(bb, style.FramePadding.y);
  if (!ItemAdd(bb, 0))
    return;

  RenderTextClipped(bb.Min + ImVec2(0.0f, style.FramePadding.y), bb.Max, label, label_end, &size);
}

bool ImguiHelper::collapsingHeaderWidth(const char *label, float width, ImGuiTreeNodeFlags flags)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  const float originalCursorPosX = window->DC.CursorPos.x;
  const float originalWorkRectMinX = window->WorkRect.Min.x;
  const float originalWorkRectMaxX = window->WorkRect.Max.x;

  // Add the inverse of extrusion amount that TreeNodeBehavior use when ImGuiTreeNodeFlags_Framed is set.
  const float extrusionMinX = IM_TRUNC(window->WindowPadding.x * 0.5f - 1.0f);
  const float extrusionMaxX = IM_TRUNC(window->WindowPadding.x * 0.5f);
  window->WorkRect.Min.x = originalCursorPosX + extrusionMinX;
  window->WorkRect.Max.x = originalCursorPosX + width - extrusionMaxX;

  flags |= ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_SpanFullWidth;
  const bool returnValue = ImGui::TreeNodeBehavior(window->GetID(label), flags, label);

  window->DC.CursorPosPrevLine.x = originalCursorPosX + width;
  window->WorkRect.Min.x = originalWorkRectMinX;
  window->WorkRect.Max.x = originalWorkRectMaxX;

  return returnValue;
}

// Based on ImGui::TreeNodeBehavior. Taken from imgui_widgets.cpp. Modified parts are marked with GAIJIN.
// clang-format off
bool ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end, TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool allow_blocked_hover, bool show_arrow)
{
    // GAIJIN {
    using namespace ImGui;
    // GAIJIN }

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
    const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

    if (!label_end)
        label_end = FindRenderedTextEnd(label);
    const ImVec2 label_size = CalcTextSize(label, label_end, false);

    const float text_offset_x = g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2);   // Collapsing arrow width + Spacing
    const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset);            // Latch before ItemSize changes it
    const float text_width = g.FontSize + label_size.x + padding.x * 2;                         // Include collapsing arrow

    // We vertically grow up to current line height up the typical widget height.
    const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
    const bool span_all_columns = (flags & ImGuiTreeNodeFlags_SpanAllColumns) != 0 && (g.CurrentTable != NULL);
    ImRect frame_bb;
    frame_bb.Min.x = span_all_columns ? window->ParentWorkRect.Min.x : (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
    frame_bb.Min.y = window->DC.CursorPos.y;
    frame_bb.Max.x = span_all_columns ? window->ParentWorkRect.Max.x : (flags & ImGuiTreeNodeFlags_SpanTextWidth) ? window->DC.CursorPos.x + text_width + padding.x : window->WorkRect.Max.x;
    frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
    if (display_frame)
    {
        const float outer_extend = IM_TRUNC(window->WindowPadding.x * 0.5f); // Framed header expand a little outside of current limits
        frame_bb.Min.x -= outer_extend;
        frame_bb.Max.x += outer_extend;
    }

    ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
    ItemSize(ImVec2(text_width, frame_height), padding.y);

    // For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
    ImRect interact_bb = frame_bb;
    if ((flags & (ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanTextWidth | ImGuiTreeNodeFlags_SpanAllColumns)) == 0)
       interact_bb.Max.x = frame_bb.Min.x + text_width + (label_size.x > 0.0f ? style.ItemSpacing.x * 2.0f : 0.0f);

    // Compute open and multi-select states before ItemAdd() as it clear NextItem data.
    ImGuiID storage_id = (g.NextItemData.Flags & ImGuiNextItemDataFlags_HasStorageID) ? g.NextItemData.StorageId : id;
    bool is_open = TreeNodeUpdateNextOpen(storage_id, flags);

    bool is_visible;
    if (span_all_columns)
    {
        // Modify ClipRect for the ItemAdd(), faster than doing a PushColumnsBackground/PushTableBackgroundChannel for every Selectable..
        const float backup_clip_rect_min_x = window->ClipRect.Min.x;
        const float backup_clip_rect_max_x = window->ClipRect.Max.x;
        window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
        window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
        is_visible = ItemAdd(interact_bb, id);
        window->ClipRect.Min.x = backup_clip_rect_min_x;
        window->ClipRect.Max.x = backup_clip_rect_max_x;
    }
    else
    {
      // GAIJIN {
      // Always use the label's rect for navigation rect to prevent horizontal scrolling when navigation with the up or down keys.
      // The scrolling happens because when using ImGuiTreeNodeFlags_SpanAvailWidth ImGui::ScrollToRectEx detects the item's rect not fully
      // in view and tries to compansate for it.
      // is_visible = ItemAdd(interact_bb, id);
      ImRect nav_bb = frame_bb;
      nav_bb.Max.x = frame_bb.Min.x + text_width + (label_size.x > 0.0f ? style.ItemSpacing.x * 2.0f : 0.0f);
      is_visible = ItemAdd(interact_bb, id, &nav_bb);
      // GAIJIN }
    }
    g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
    g.LastItemData.DisplayRect = frame_bb;

    // If a NavLeft request is happening and ImGuiTreeNodeFlags_NavLeftJumpsBackHere enabled:
    // Store data for the current depth to allow returning to this node from any child item.
    // For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
    // It will become tempting to enable ImGuiTreeNodeFlags_NavLeftJumpsBackHere by default or move it to ImGuiStyle.
    bool store_tree_node_stack_data = false;
    if (!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
    {
        if ((flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && is_open && !g.NavIdIsAlive)
            if (g.NavMoveDir == ImGuiDir_Left && g.NavWindow == window && NavMoveRequestButNoResultYet())
                store_tree_node_stack_data = true;
    }

    const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
    if (!is_visible)
    {
        if (store_tree_node_stack_data && is_open)
            TreeNodeStoreStackData(flags); // Call before TreePushOverrideID()
        if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            TreePushOverrideID(id);
        IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
        return is_open;
    }

    if (span_all_columns)
    {
        TablePushBackgroundChannel();
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasClipRect;
        g.LastItemData.ClipRect = window->ClipRect;
    }

    ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
    if ((flags & ImGuiTreeNodeFlags_AllowOverlap) || (g.LastItemData.InFlags & ImGuiItemFlags_AllowOverlap))
        button_flags |= ImGuiButtonFlags_AllowOverlap;
    if (!is_leaf)
        button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

    // We allow clicking on the arrow section with keyboard modifiers held, in order to easily
    // allow browsing a tree while preserving selection with code implementing multi-selection patterns.
    // When clicking on the rest of the tree node we always disallow keyboard modifiers.
    const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
    const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
    const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);

    // Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
    // Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
    // - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
    // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
    // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
    // - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
    // - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
    // It is rather standard that arrow click react on Down rather than Up.
    // We set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
    if (is_mouse_x_over_arrow)
        button_flags |= ImGuiButtonFlags_PressedOnClick;
    else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
    else
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

    bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
    const bool was_selected = selected;

    // Multi-selection support (header)
    const bool is_multi_select = (g.LastItemData.InFlags & ImGuiItemFlags_IsMultiSelect) != 0;
    if (is_multi_select)
    {
        // Handle multi-select + alter button flags for it
        MultiSelectItemHeader(id, &selected, &button_flags);
        if (is_mouse_x_over_arrow)
            button_flags = (button_flags | ImGuiButtonFlags_PressedOnClick) & ~ImGuiButtonFlags_PressedOnClickRelease;

        // We absolutely need to distinguish open vs select so comes by default
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;
    }
    else
    {
        if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
            button_flags |= ImGuiButtonFlags_NoKeyModifiers;
    }

    bool hovered, held;
    bool pressed = ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);

    // GAIJIN {
    if (!hovered && allow_blocked_hover)
    {
      ImGuiHoveredFlags hoveredFlags = ImGuiHoveredFlags_AllowWhenOverlappedByItem;

      // We do not want hover highlight when another control is active (for example panning in the viewport), but we
      // need this flag for the image buttons. So using this condition is not perfect, there will be hover highlight on
      // other tree nodes when pressing the mouse button on an image button. The perfect condition would check if the
      // active item belongs to the current tree node.
      if (IsWindowFocused())
        hoveredFlags |= ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;

      hovered = IsItemHovered(hoveredFlags);
    }

    end_data.hovered = hovered;
    // GAIJIN }

    bool toggled = false;
    if (!is_leaf)
    {
        if (pressed && g.DragDropHoldJustPressedId != id)
        {
            if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 || (g.NavActivateId == id && !is_multi_select))
                toggled = true;
            if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
                toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
            if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseClickedCount[0] == 2)
                toggled = true;
        }
        else if (pressed && g.DragDropHoldJustPressedId == id)
        {
            IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
            if (!is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
                toggled = true;
            else
                pressed = false; // Cancel press so it doesn't trigger selection.
        }

        if (g.NavId == id && g.NavMoveDir == ImGuiDir_Left && is_open)
        {
            toggled = true;
            NavClearPreferredPosForAxis(ImGuiAxis_X);
            NavMoveRequestCancel();
        }
        if (g.NavId == id && g.NavMoveDir == ImGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
        {
            toggled = true;
            NavClearPreferredPosForAxis(ImGuiAxis_X);
            NavMoveRequestCancel();
        }

        if (toggled)
        {
            is_open = !is_open;
            window->DC.StateStorage->SetInt(storage_id, is_open);
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
        }
    }

    // Multi-selection support (footer)
    if (is_multi_select)
    {
        bool pressed_copy = pressed && !toggled;
        MultiSelectItemFooter(id, &selected, &pressed_copy);
        if (pressed)
            SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId, interact_bb);
    }

    if (selected != was_selected)
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

    // Render
    {
        const ImU32 text_col = GetColorU32(ImGuiCol_Text);
        ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_Compact;
        if (is_multi_select)
            nav_highlight_flags |= ImGuiNavHighlightFlags_AlwaysDraw; // Always show the nav rectangle
        if (display_frame)
        {
            // Framed type
            const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
            RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
            RenderNavHighlight(frame_bb, id, nav_highlight_flags);
            if (flags & ImGuiTreeNodeFlags_Bullet)
                RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f), text_col);
            else if (!is_leaf)
                RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y), text_col, is_open ? ((flags & ImGuiTreeNodeFlags_UpsideDownArrow) ? ImGuiDir_Up : ImGuiDir_Down) : ImGuiDir_Right, 1.0f);
            else // Leaf without bullet, left-adjusted text
                text_pos.x -= text_offset_x - padding.x;
            if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
                frame_bb.Max.x -= g.FontSize + style.FramePadding.x;
            if (g.LogEnabled)
                LogSetNextTextDecoration("###", "###");
        }
        else
        {
            // Unframed typed for tree nodes
            if (hovered || selected)
            {
                const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
                RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
            }
            RenderNavHighlight(frame_bb, id, nav_highlight_flags);
            if (flags & ImGuiTreeNodeFlags_Bullet)
                RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f), text_col);
            // GAIJIN {
            // else if (!is_leaf)
            else if (!is_leaf && show_arrow)
            // GAIJIN }
                RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f), text_col, is_open ? ((flags & ImGuiTreeNodeFlags_UpsideDownArrow) ? ImGuiDir_Up : ImGuiDir_Down) : ImGuiDir_Right, 0.70f);
            if (g.LogEnabled)
                LogSetNextTextDecoration(">", NULL);
        }

        if (span_all_columns)
            TablePopBackgroundChannel();

        // GAIJIN {
        // // Label
        // if (display_frame)
        //     RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
        // else
        //     RenderText(text_pos, label, label_end, false);
        // GAIJIN }
    }

    // GAIJIN {
    // if (store_tree_node_stack_data && is_open)
    //     TreeNodeStoreStackData(flags); // Call before TreePushOverrideID()
    // if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
    //     TreePushOverrideID(id); // Could use TreePush(label) but this avoid computing twice
    //
    // IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));

    end_data.draw = true;
    end_data.id = id;
    end_data.label = label;
    end_data.labelEnd = label_end;
    end_data.flags = flags;
    end_data.labelSize = label_size;
    end_data.textPos = text_pos;
    end_data.isOpen = is_open;
    end_data.storeTreeNodeStackData = store_tree_node_stack_data;
    // GAIJIN }

    return is_open;
}
// clang-format on

bool ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(const char *label, ImGuiTreeNodeFlags flags,
  TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool allow_blocked_hover, bool show_arrow)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;
  ImGuiID id = window->GetID(label);
  return treeNodeWithSpecialHoverBehaviorStart(id, flags, label, nullptr, end_data, allow_blocked_hover, show_arrow);
}

void ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(const TreeNodeWithSpecialHoverBehaviorEndData &end_data,
  const float *label_clip_max_x)
{
  G_ASSERT(end_data.draw);

  ImGuiContext &g = *GImGui;
  const float finalLabelClipMaxX = label_clip_max_x ? *label_clip_max_x : treeNodeWithSpecialHoverBehaviorGetLabelClipMaxX();
  const ImVec2 posMin = end_data.textPos;
  const ImVec2 posMax(finalLabelClipMaxX, end_data.textPos.y + end_data.labelSize.y);
  ImGui::RenderTextEllipsis(g.CurrentWindow->DrawList, posMin, posMax, posMax.x, posMax.x, end_data.label, end_data.labelEnd,
    &end_data.labelSize);

  if (end_data.storeTreeNodeStackData && end_data.isOpen)
    TreeNodeStoreStackData(end_data.flags); // Call before TreePushOverrideID()
  if (end_data.isOpen && !(end_data.flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
    ImGui::TreePushOverrideID(end_data.id); // Could use TreePush(label) but this avoid computing twice

  IMGUI_TEST_ENGINE_ITEM_INFO(end_data.id, end_data.label,
    g.LastItemData.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));

  if (end_data.labelSize.x > (posMax.x - posMin.x))
    set_previous_imgui_control_tooltip((const void *)ImGui::GetItemID(), end_data.label, end_data.labelEnd);
}

float ImguiHelper::treeNodeWithSpecialHoverBehaviorGetLabelClipMaxX()
{
  const ImGuiWindow *window = ImGui::GetCurrentWindowRead();
  return window->ContentRegionRect.Max.x + window->Scroll.x;
}

void ImguiHelper::renderArrowWithSize(ImDrawList *draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, ImVec2 size)
{
  size *= 0.5f;
  ImVec2 center = pos + size;

  ImVec2 a, b, c;
  switch (dir)
  {
    case ImGuiDir_Up:
    case ImGuiDir_Down:
      if (dir == ImGuiDir_Up)
        size = -size;
      a = ImVec2(+0.000f, +0.750f) * size;
      b = ImVec2(-0.866f, -0.750f) * size;
      c = ImVec2(+0.866f, -0.750f) * size;
      break;
    case ImGuiDir_Left:
    case ImGuiDir_Right:
      if (dir == ImGuiDir_Left)
        size = -size;
      a = ImVec2(+0.750f, +0.000f) * size;
      b = ImVec2(-0.750f, +0.866f) * size;
      c = ImVec2(-0.750f, -0.866f) * size;
      break;
    case ImGuiDir_None:
    case ImGuiDir_COUNT: IM_ASSERT(0); break;
  }
  draw_list->AddTriangleFilled(center + a, center + b, center + c, col);
}

// The changed parts are marked with GAIJIN.
// clang-format off
bool ImguiHelper::arrowButtonExWithSize(const char* str_id, ImGuiDir dir, ImVec2 size, ImVec2 arrow_size, ImGuiButtonFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(str_id);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const float default_size = GetFrameHeight();
    ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : -1.0f);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    const ImU32 text_col = GetColorU32(ImGuiCol_Text);
    RenderNavHighlight(bb, id);
    RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);

// GAIJIN {
    const ImVec2 arrowPos = (bb.Min + bb.Max - arrow_size) / 2.0f;
    renderArrowWithSize(window->DrawList, arrowPos, text_col, dir, arrow_size);
// GAIJIN }

    IMGUI_TEST_ENGINE_ITEM_INFO(id, str_id, g.LastItemData.StatusFlags);
    return pressed;
}
// clang-format on

// The changed parts are marked with GAIJIN.
// clang-format off
bool ImguiHelper::beginListBoxWithWindowFlags(const char* label, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    // Size default to hold ~7.25 items.
    // Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
    ImVec2 size = ImTrunc(CalcItemSize(size_arg, CalcItemWidth(), GetTextLineHeightWithSpacing() * 7.25f + style.FramePadding.y * 2.0f));
    ImVec2 frame_size = ImVec2(size.x, ImMax(size.y, label_size.y));
    ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    g.NextItemData.ClearFlags();

    if (!IsRectVisible(bb.Min, bb.Max))
    {
        ItemSize(bb.GetSize(), style.FramePadding.y);
        ItemAdd(bb, 0, &frame_bb);
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
        return false;
    }

    // FIXME-OPT: We could omit the BeginGroup() if label_size.x == 0.0f but would need to omit the EndGroup() as well.
    BeginGroup();
    if (label_size.x > 0.0f)
    {
        ImVec2 label_pos = ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y);
        RenderText(label_pos, label);
        window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, label_pos + label_size);
        AlignTextToFramePadding();
    }

// GAIJIN {
    // BeginChild(id, frame_bb.GetSize(), ImGuiChildFlags_FrameStyle);
    BeginChild(id, frame_bb.GetSize(), child_flags, window_flags);
// GAIJIN }

    return true;
}

// clang-format on
bool ImguiHelper::imageButtonFrameless(ImGuiID id, ImTextureID texture_id, const ImVec2 &image_size, const ImVec2 &uv0,
  const ImVec2 &uv1, const ImVec4 &tint_col)
{
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  const ImVec2 padding = ImVec2(0, 0);
  const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + image_size + padding * 2.0f);
  ImGui::ItemSize(bb);
  if (!ImGui::ItemAdd(bb, id))
    return false;

  bool hovered, held;
  const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

  ImGui::RenderNavHighlight(bb, id);

  if (held || hovered)
  {
    const ImU32 backgroundColor = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
    window->DrawList->AddRectFilled(bb.Min + padding, bb.Max - padding, backgroundColor);
  }

  window->DrawList->AddImage(texture_id, bb.Min + padding, bb.Max - padding, uv0, uv1, ImGui::GetColorU32(tint_col));

  return pressed;
}

bool ImguiHelper::imageButtonFrameless(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, const char *tooltip)
{
  const bool pressed = imageButtonFrameless(ImGui::GetCurrentWindow()->GetID(str_id), texture_id, image_size);
  set_previous_imgui_control_tooltip((const void *)ImGui::GetItemID(), tooltip, nullptr);
  return pressed;
}

bool ImguiHelper::imageButtonFramelessOrPlaceholder(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size,
  const char *tooltip, bool use_button)
{
  if (use_button)
    return imageButtonFrameless(str_id, texture_id, image_size, tooltip);

  ImGui::Dummy(getImageButtonFramelessFullSize(image_size));
  return false;
}

bool ImguiHelper::imageCheckButtonWithBackground(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, bool checked,
  const char *tooltip)
{
  // Not exactly the same as PropPanel::ToolbarToggleButtonPropertyControl.
  if (checked)
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
  else
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));

  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
  ImGui::PushStyleColor(ImGuiCol_Border, Constants::TOGGLE_BUTTON_CHECKED_BORDER_COLOR);

  setPointSampler();
  const bool pressed = ImGui::ImageButton(str_id, texture_id, image_size);
  setDefaultSampler();

  ImGui::PopStyleColor(3);

  set_previous_imgui_control_tooltip((const void *)ImGui::GetItemID(), tooltip, nullptr);

  return pressed;
}

ImVec2 ImguiHelper::getImageButtonWithDownArrowSizeInternal(const ImVec2 &image_size, float &default_height, ImVec2 &arrow_half_size)
{
  const ImGuiContext &g = *GImGui;
  const float arrowScale = 0.5f;

  default_height = ImGui::GetFrameHeight();
  arrow_half_size = ImVec2(g.FontSize * 0.5f * arrowScale, g.FontSize * 0.25f * arrowScale);

  const ImVec2 size(
    g.Style.FramePadding.x + image_size.x + g.Style.ItemInnerSpacing.x + (arrow_half_size.x * 2.0f) + g.Style.FramePadding.x,
    ImMax(image_size.y, default_height));

  return size;
}

ImVec2 ImguiHelper::getImageButtonWithDownArrowSize(const ImVec2 &image_size)
{
  float defaultHeight;
  ImVec2 arrowHalfSize;
  return getImageButtonWithDownArrowSizeInternal(image_size, defaultHeight, arrowHalfSize);
}

bool ImguiHelper::imageButtonWithArrow(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, bool checked,
  ImGuiButtonFlags flags)
{
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  float defaultHeight;
  ImVec2 arrowHalfSize;
  const ImVec2 size = getImageButtonWithDownArrowSizeInternal(image_size, defaultHeight, arrowHalfSize);
  const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
  const ImGuiID id = window->GetID(str_id);

  ImGui::ItemSize(size, (size.y >= defaultHeight) ? g.Style.FramePadding.y : -1.0f);
  if (!ImGui::ItemAdd(bb, id))
    return false;

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

  // For the checked state it uses the same colors as PropPanel::ToolbarToggleButtonPropertyControl.
  ImU32 bgColor;
  if (held && hovered)
    bgColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);
  else if (checked && hovered)
    bgColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);
  else if (checked)
    bgColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
  else if (hovered)
    bgColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
  else
    bgColor = ImGui::GetColorU32(ImGuiCol_Button);

  ImGui::RenderNavHighlight(bb, id);
  ImGui::RenderFrame(bb.Min, bb.Max, bgColor, true, g.Style.FrameRounding);

  const ImVec2 imagePos = bb.Min + ImVec2(g.Style.FramePadding.x, ImMax(0.0f, (size.y - g.FontSize) * 0.5f));
  window->DrawList->AddImage(texture_id, imagePos, imagePos + image_size);

  const ImVec2 arrowPos =
    ImVec2(imagePos.x + image_size.x + g.Style.ItemInnerSpacing.x, bb.Min.y) + ImVec2(arrowHalfSize.x, size.y * 0.5f);
  const ImU32 arrowColor = IM_COL32(0, 0, 0, 255);
  window->DrawList->AddTriangleFilled(ImVec2(arrowPos.x - arrowHalfSize.x, arrowPos.y - arrowHalfSize.y),
    ImVec2(arrowPos.x + arrowHalfSize.x, arrowPos.y - arrowHalfSize.y), ImVec2(arrowPos.x, arrowPos.y + arrowHalfSize.y), arrowColor);

  IMGUI_TEST_ENGINE_ITEM_INFO(id, str_id, g.LastItemData.StatusFlags);
  return pressed;
}

ImVec2 ImguiHelper::getButtonSize(const char *label, bool hide_text_after_double_hash, const ImVec2 &size_arg)
{
  const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, hide_text_after_double_hash);
  const ImVec2 buttonSize = ImGui::CalcItemSize(size_arg, labelSize.x + ImGui::GetStyle().FramePadding.x * 2.0f,
    labelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f);
  return buttonSize;
}

bool ImguiHelper::searchInput(const void *focus_id, const char *label, const char *hint, String &text_to_search,
  ImTextureID search_icon, ImTextureID clear_icon, bool *input_focused, ImGuiID *input_id)
{
  const ImVec2 fontSizedIconSize = getFontSizedIconSize();
  const float iconPaddingX = ImGui::GetStyle().ItemInnerSpacing.x;
  const float inputPaddingX = iconPaddingX + fontSizedIconSize.x;

  focus_helper.setFocusToNextImGuiControlIfRequested(focus_id);

  ImGui::SetNextItemAllowOverlap();

  // Extend padding so we can fit the icons in it. This is not perfect but better than copying the entire ImGui::InputTextEx.
  const ImVec2 originalFramePadding = ImGui::GetStyle().FramePadding;
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(originalFramePadding.x + inputPaddingX, originalFramePadding.y));
  bool inputChanged = ImGuiDagor::InputTextWithHint(label, hint, &text_to_search);
  ImGui::PopStyleVar();

  if (input_focused)
    *input_focused = ImGui::IsItemFocused();
  if (input_id)
    *input_id = ImGui::GetItemID();

  const ImVec2 inputRectMin = ImGui::GetItemRectMin();
  const ImVec2 inputRectMax = ImGui::GetItemRectMax();
  const ImVec2 iconPos = inputRectMin + ImVec2(iconPaddingX, ((inputRectMax.y - inputRectMin.y) - fontSizedIconSize.y) * 0.5f);
  ImGui::GetWindowDrawList()->AddImage(search_icon, iconPos, iconPos + fontSizedIconSize);

  // Show the clear button.
  if (!text_to_search.empty())
  {
    ImGui::SameLine();

    const ImVec2 originalCursorPos = ImGui::GetCursorScreenPos();
    const ImVec2 clearButtonPos = ImVec2(inputRectMax.x - iconPaddingX - fontSizedIconSize.x, iconPos.y);
    ImGui::SetCursorScreenPos(clearButtonPos);
    const bool clearButtonPressed = imageButtonFrameless("searchInputClear", clear_icon, fontSizedIconSize, "Clear");
    ImGui::SetCursorScreenPos(originalCursorPos + ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));

    // Ensure correct position for the next item after the search input (needed because the image button).
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    if (clearButtonPressed)
    {
      text_to_search.clear();
      inputChanged = true;
      focus_helper.requestFocus(focus_id);
    }
  }

  return inputChanged;
}

// The changed parts compared to ImGui::MenuItemEx are marked with GAIJIN.
// clang-format off
bool ImguiHelper::menuItemExWithLeftSideCheckmark(const char *label, const char *icon, const char *shortcut, bool selected, bool enabled, bool bullet)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 label_size = CalcTextSize(label, NULL, true);

    // See BeginMenuEx() for comments about this.
    const bool menuset_is_open = IsRootOfOpenMenuSet();
    if (menuset_is_open)
        PushItemFlag(ImGuiItemFlags_NoWindowHoverableCheck, true);

    // We've been using the equivalent of ImGuiSelectableFlags_SetNavIdOnHover on all Selectable() since early Nav system days (commit 43ee5d73),
    // but I am unsure whether this should be kept at all. For now moved it to be an opt-in feature used by menus only.
    bool pressed;
    PushID(label);
    if (!enabled)
        BeginDisabled();

    // We use ImGuiSelectableFlags_NoSetKeyOwner to allow down on one menu item, move, up on another.
    const ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SelectOnRelease | ImGuiSelectableFlags_NoSetKeyOwner | ImGuiSelectableFlags_SetNavIdOnHover;
    const ImGuiMenuColumns* offsets = &window->DC.MenuColumns;
    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
    {
        // Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
        // Note that in this situation: we don't render the shortcut, we render a highlight instead of the selected tick mark.
        float w = label_size.x;
        window->DC.CursorPos.x += IM_TRUNC(style.ItemSpacing.x * 0.5f);

        ImVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
        pressed = Selectable("", selected, selectable_flags, ImVec2(w, 0.0f));
        PopStyleVar();
        if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)
            RenderText(text_pos, label);
        window->DC.CursorPos.x += IM_TRUNC(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
    }
    else
    {
        // Menu item inside a vertical menu
        // (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
        //  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.
        float icon_w = (icon && icon[0]) ? CalcTextSize(icon, NULL).x : 0.0f;
        float shortcut_w = (shortcut && shortcut[0]) ? CalcTextSize(shortcut, NULL).x : 0.0f;

        // GAIJIN {
        // float checkmark_w = IM_TRUNC(g.FontSize * 1.20f);

        const float checkmarkSize = getMenuCheckmarkSize(g);
        const float checkmarkPadding = getMenuCheckmarkPadding(g);
        const float checkmark_w = IM_TRUNC(checkmarkPadding + checkmarkSize + checkmarkPadding);
        // GAIJIN }

        float min_w = window->DC.MenuColumns.DeclColumns(icon_w, label_size.x, shortcut_w, checkmark_w); // Feedback for next frame
        float stretch_w = ImMax(0.0f, GetContentRegionAvail().x - min_w);
        pressed = Selectable("", false, selectable_flags | ImGuiSelectableFlags_SpanAvailWidth, ImVec2(min_w, label_size.y));
        if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)
        {
            // GAIJIN {
            if (selected)
            {
                if (bullet)
                    RenderBullet(window->DrawList, pos + ImVec2(offsets->OffsetLabel + checkmarkPadding + g.FontSize * 0.5f, g.FontSize * 0.5f), GetColorU32(ImGuiCol_Text));
                else
                    RenderCheckMark(window->DrawList, pos + ImVec2(offsets->OffsetLabel + checkmarkPadding, g.FontSize * 0.134f * 0.5f), GetColorU32(ImGuiCol_Text), checkmarkSize);
            }

            pos.x += IM_TRUNC(checkmarkPadding + checkmarkSize + checkmarkPadding);
            // GAIJIN }

            RenderText(pos + ImVec2(offsets->OffsetLabel, 0.0f), label);
            if (icon_w > 0.0f)
                RenderText(pos + ImVec2(offsets->OffsetIcon, 0.0f), icon);
            if (shortcut_w > 0.0f)
            {
                PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
                RenderText(pos + ImVec2(offsets->OffsetShortcut + stretch_w, 0.0f), shortcut, NULL, false);
                PopStyleColor();
            }

            // GAIJIN {
            //if (selected)
            //    RenderCheckMark(window->DrawList, pos + ImVec2(offsets->OffsetMark + stretch_w + g.FontSize * 0.40f, g.FontSize * 0.134f * 0.5f), GetColorU32(ImGuiCol_Text), g.FontSize * 0.866f);
            // GAIJIN }
        }
    }
    IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (selected ? ImGuiItemStatusFlags_Checked : 0));
    if (!enabled)
        EndDisabled();
    PopID();
    if (menuset_is_open)
        PopItemFlag();

    return pressed;
}
// clang-format on

// The changed parts compared to ImGui::BeginMenuEx are marked with GAIJIN.
// clang-format off
bool ImguiHelper::beginMenuExWithLeftSideCheckmark(const char* label, const char* icon, bool enabled)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    bool menu_is_open = IsPopupOpen(id, ImGuiPopupFlags_None);

    // Sub-menus are ChildWindow so that mouse can be hovering across them (otherwise top-most popup menu would steal focus and not allow hovering on parent menu)
    // The first menu in a hierarchy isn't so hovering doesn't get across (otherwise e.g. resizing borders with ImGuiButtonFlags_FlattenChildren would react), but top-most BeginMenu() will bypass that limitation.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;
    if (window->Flags & ImGuiWindowFlags_ChildMenu)
        window_flags |= ImGuiWindowFlags_ChildWindow;

    // If a menu with same the ID was already submitted, we will append to it, matching the behavior of Begin().
    // We are relying on a O(N) search - so O(N log N) over the frame - which seems like the most efficient for the expected small amount of BeginMenu() calls per frame.
    // If somehow this is ever becoming a problem we can switch to use e.g. ImGuiStorage mapping key to last frame used.
    if (g.MenusIdSubmittedThisFrame.contains(id))
    {
        if (menu_is_open)
            menu_is_open = BeginPopupEx(id, window_flags); // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
        else
            g.NextWindowData.ClearFlags();          // we behave like Begin() and need to consume those values
        return menu_is_open;
    }

    // Tag menu as used. Next time BeginMenu() with same ID is called it will append to existing menu
    g.MenusIdSubmittedThisFrame.push_back(id);

    ImVec2 label_size = CalcTextSize(label, NULL, true);

    // Odd hack to allow hovering across menus of a same menu-set (otherwise we wouldn't be able to hover parent without always being a Child window)
    // This is only done for items for the menu set and not the full parent window.
    const bool menuset_is_open = IsRootOfOpenMenuSet();
    if (menuset_is_open)
        PushItemFlag(ImGuiItemFlags_NoWindowHoverableCheck, true);

    // The reference position stored in popup_pos will be used by Begin() to find a suitable position for the child menu,
    // However the final position is going to be different! It is chosen by FindBestWindowPosForPopup().
    // e.g. Menus tend to overlap each other horizontally to amplify relative Z-ordering.
    ImVec2 popup_pos, pos = window->DC.CursorPos;
    PushID(label);
    if (!enabled)
        BeginDisabled();
    const ImGuiMenuColumns* offsets = &window->DC.MenuColumns;
    bool pressed;

    // We use ImGuiSelectableFlags_NoSetKeyOwner to allow down on one menu item, move, up on another.
    const ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_NoSetKeyOwner | ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_NoAutoClosePopups;
    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
    {
        // Menu inside an horizontal menu bar
        // Selectable extend their highlight by half ItemSpacing in each direction.
        // For ChildMenu, the popup position will be overwritten by the call to FindBestWindowPosForPopup() in Begin()
        popup_pos = ImVec2(pos.x - 1.0f - IM_TRUNC(style.ItemSpacing.x * 0.5f), pos.y - style.FramePadding.y + window->MenuBarHeight);
        window->DC.CursorPos.x += IM_TRUNC(style.ItemSpacing.x * 0.5f);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
        float w = label_size.x;
        ImVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
        pressed = Selectable("", menu_is_open, selectable_flags, ImVec2(w, label_size.y));
        RenderText(text_pos, label);
        PopStyleVar();
        window->DC.CursorPos.x += IM_TRUNC(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
    }
    else
    {
        // Menu inside a regular/vertical menu
        // (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
        //  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.
        popup_pos = ImVec2(pos.x, pos.y - style.WindowPadding.y);
        float icon_w = (icon && icon[0]) ? CalcTextSize(icon, NULL).x : 0.0f;

        // GAIJIN {
        //float checkmark_w = IM_TRUNC(g.FontSize * 1.20f);

        const float checkmarkSize = getMenuCheckmarkSize(g);
        const float checkmarkPadding = getMenuCheckmarkPadding(g);
        const float checkmarkWidth = IM_TRUNC(checkmarkPadding + checkmarkSize + checkmarkPadding);
        const float rightSideSpace = IM_TRUNC(checkmarkSize + (g.FontSize * 0.30f)); // Leave space for the sub-menu open arrow.
        const float checkmark_w = checkmarkWidth + rightSideSpace;
        // GAIJIN }

        float min_w = window->DC.MenuColumns.DeclColumns(icon_w, label_size.x, 0.0f, checkmark_w); // Feedback to next frame
        float extra_w = ImMax(0.0f, GetContentRegionAvail().x - min_w);
        ImVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);

        // GAIJIN {
        extra_w += checkmarkWidth;
        text_pos.x += checkmarkWidth;
        // GAIJIN }

        pressed = Selectable("", menu_is_open, selectable_flags | ImGuiSelectableFlags_SpanAvailWidth, ImVec2(min_w, label_size.y));
        RenderText(text_pos, label);
        if (icon_w > 0.0f)
            RenderText(pos + ImVec2(offsets->OffsetIcon, 0.0f), icon);
        RenderArrow(window->DrawList, pos + ImVec2(offsets->OffsetMark + extra_w + g.FontSize * 0.30f, 0.0f), GetColorU32(ImGuiCol_Text), ImGuiDir_Right);
    }
    if (!enabled)
        EndDisabled();

    const bool hovered = (g.HoveredId == id) && enabled && !g.NavDisableMouseHover;
    if (menuset_is_open)
        PopItemFlag();

    bool want_open = false;
    bool want_open_nav_init = false;
    bool want_close = false;
    if (window->DC.LayoutType == ImGuiLayoutType_Vertical) // (window->Flags & (ImGuiWindowFlags_Popup|ImGuiWindowFlags_ChildMenu))
    {
        // Close menu when not hovering it anymore unless we are moving roughly in the direction of the menu
        // Implement http://bjk5.com/post/44698559168/breaking-down-amazons-mega-dropdown to avoid using timers, so menus feels more reactive.
        bool moving_toward_child_menu = false;
        ImGuiPopupData* child_popup = (g.BeginPopupStack.Size < g.OpenPopupStack.Size) ? &g.OpenPopupStack[g.BeginPopupStack.Size] : NULL; // Popup candidate (testing below)
        ImGuiWindow* child_menu_window = (child_popup && child_popup->Window && child_popup->Window->ParentWindow == window) ? child_popup->Window : NULL;
        if (g.HoveredWindow == window && child_menu_window != NULL)
        {
            const float ref_unit = g.FontSize; // FIXME-DPI
            const float child_dir = (window->Pos.x < child_menu_window->Pos.x) ? 1.0f : -1.0f;
            const ImRect next_window_rect = child_menu_window->Rect();
            ImVec2 ta = (g.IO.MousePos - g.IO.MouseDelta);
            ImVec2 tb = (child_dir > 0.0f) ? next_window_rect.GetTL() : next_window_rect.GetTR();
            ImVec2 tc = (child_dir > 0.0f) ? next_window_rect.GetBL() : next_window_rect.GetBR();
            const float pad_farmost_h = ImClamp(ImFabs(ta.x - tb.x) * 0.30f, ref_unit * 0.5f, ref_unit * 2.5f); // Add a bit of extra slack.
            ta.x += child_dir * -0.5f;
            tb.x += child_dir * ref_unit;
            tc.x += child_dir * ref_unit;
            tb.y = ta.y + ImMax((tb.y - pad_farmost_h) - ta.y, -ref_unit * 8.0f); // Triangle has maximum height to limit the slope and the bias toward large sub-menus
            tc.y = ta.y + ImMin((tc.y + pad_farmost_h) - ta.y, +ref_unit * 8.0f);
            moving_toward_child_menu = ImTriangleContainsPoint(ta, tb, tc, g.IO.MousePos);
            //GetForegroundDrawList()->AddTriangleFilled(ta, tb, tc, moving_toward_child_menu ? IM_COL32(0,128,0,128) : IM_COL32(128,0,0,128)); // [DEBUG]
        }

        // The 'HovereWindow == window' check creates an inconsistency (e.g. moving away from menu slowly tends to hit same window, whereas moving away fast does not)
        // But we also need to not close the top-menu menu when moving over void. Perhaps we should extend the triangle check to a larger polygon.
        // (Remember to test this on BeginPopup("A")->BeginMenu("B") sequence which behaves slightly differently as B isn't a Child of A and hovering isn't shared.)
        if (menu_is_open && !hovered && g.HoveredWindow == window && !moving_toward_child_menu && !g.NavDisableMouseHover && g.ActiveId == 0)
            want_close = true;

        // Open
        // (note: at this point 'hovered' actually includes the NavDisableMouseHover == false test)
        if (!menu_is_open && pressed) // Click/activate to open
            want_open = true;
        else if (!menu_is_open && hovered && !moving_toward_child_menu) // Hover to open
            want_open = true;
        else if (!menu_is_open && hovered && g.HoveredIdTimer >= 0.30f && g.MouseStationaryTimer >= 0.30f) // Hover to open (timer fallback)
            want_open = true;
        if (g.NavId == id && g.NavMoveDir == ImGuiDir_Right) // Nav-Right to open
        {
            want_open = want_open_nav_init = true;
            NavMoveRequestCancel();
            NavRestoreHighlightAfterMove();
        }
    }
    else
    {
        // Menu bar
        if (menu_is_open && pressed && menuset_is_open) // Click an open menu again to close it
        {
            want_close = true;
            want_open = menu_is_open = false;
        }
        else if (pressed || (hovered && menuset_is_open && !menu_is_open)) // First click to open, then hover to open others
        {
            want_open = true;
        }
        else if (g.NavId == id && g.NavMoveDir == ImGuiDir_Down) // Nav-Down to open
        {
            want_open = true;
            NavMoveRequestCancel();
        }
    }

    if (!enabled) // explicitly close if an open menu becomes disabled, facilitate users code a lot in pattern such as 'if (BeginMenu("options", has_object)) { ..use object.. }'
        want_close = true;
    if (want_close && IsPopupOpen(id, ImGuiPopupFlags_None))
        ClosePopupToLevel(g.BeginPopupStack.Size, true);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Openable | (menu_is_open ? ImGuiItemStatusFlags_Opened : 0));
    PopID();

    if (want_open && !menu_is_open && g.OpenPopupStack.Size > g.BeginPopupStack.Size)
    {
        // Don't reopen/recycle same menu level in the same frame if it is a different menu ID, first close the other menu and yield for a frame.
        OpenPopup(label);
    }
    else if (want_open)
    {
        menu_is_open = true;
        OpenPopup(label, ImGuiPopupFlags_NoReopen);// | (want_open_nav_init ? ImGuiPopupFlags_NoReopenAlwaysNavInit : 0));
    }

    if (menu_is_open)
    {
        ImGuiLastItemData last_item_in_parent = g.LastItemData;
        SetNextWindowPos(popup_pos, ImGuiCond_Always);                  // Note: misleading: the value will serve as reference for FindBestWindowPosForPopup(), not actual pos.
        PushStyleVar(ImGuiStyleVar_ChildRounding, style.PopupRounding); // First level will use _PopupRounding, subsequent will use _ChildRounding
        menu_is_open = BeginPopupEx(id, window_flags);                  // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
        PopStyleVar();
        if (menu_is_open)
        {
            // Implement what ImGuiPopupFlags_NoReopenAlwaysNavInit would do:
            // Perform an init request in the case the popup was already open (via a previous mouse hover)
            if (want_open && want_open_nav_init && !g.NavInitRequest)
            {
                FocusWindow(g.CurrentWindow, ImGuiFocusRequestFlags_UnlessBelowModal);
                NavInitWindow(g.CurrentWindow, false);
            }

            // Restore LastItemData so IsItemXXXX functions can work after BeginMenu()/EndMenu()
            // (This fixes using IsItemClicked() and IsItemHovered(), but IsItemHovered() also relies on its support for ImGuiItemFlags_NoWindowHoverableCheck)
            g.LastItemData = last_item_in_parent;
            if (g.HoveredWindow == window)
                g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
        }
    }
    else
    {
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    }

    return menu_is_open;
}
// clang-format on

float ImguiHelper::getCursorPosYWithoutLastItemSpacing()
{
  ImGuiWindow *window = ImGui::GetCurrentWindowRead();
  float cursorPosYWithoutItemSpacing = (window->DC.CursorPosPrevLine.y + window->DC.PrevLineSize.y); // See ImGui::ItemSize().
  return cursorPosYWithoutItemSpacing - window->Pos.y + window->Scroll.y;                            // See ImGui::GetCursorPosY().
}

void ImguiHelper::setPointSampler()
{
  d3d::SamplerInfo smpInfo;
  smpInfo.filter_mode = d3d::FilterMode::Point;
  static d3d::SamplerHandle smp = d3d::request_sampler(smpInfo);
  ImGuiDagor::Sampler(smp);
}

void ImguiHelper::setDefaultSampler()
{
  static d3d::SamplerHandle smp = d3d::request_sampler({});
  ImGuiDagor::Sampler(smp);
}

} // namespace PropPanel
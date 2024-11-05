// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "tooltipHelper.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

TooltipHelper tooltip_helper;

void TooltipHelper::afterNewFrame()
{
  // Prevent the tooltip if any mouse button is currently down.
  preventTooltips = ImGui::IsAnyMouseDown();

  // Prevent the tooltip for the currently hovered control if there was a mouse click or keyboard press till the next
  // re-hovering. (Handled in beforeEndFrame.)
  if (!preventTooltips)
  {
    ImGuiContext *context = ImGui::GetCurrentContext();
    for (const ImGuiInputEvent &event : context->InputEventsTrail)
    {
      if (event.Type == ImGuiInputEventType_MouseButton || event.Type == ImGuiInputEventType_MouseWheel ||
          event.Type == ImGuiInputEventType_Key)
      {
        preventTooltips = true;
        break;
      }
    }
  }

  lastHoveredControl = nullptr;
}

void TooltipHelper::beforeEndFrame()
{
  if (preventTooltips)
    preventTooltipForControl = lastHoveredControl;
  else if (lastHoveredControl != preventTooltipForControl)
    preventTooltipForControl = nullptr;
}

void TooltipHelper::setPreviousImguiControlTooltip(const void *control, const char *text, const char *text_end)
{
  if (!text || *text == 0)
    return;

  // The ImGuiHoveredFlags_AllowWhenBlockedByActiveItem flag is needed becase we really want to know if it is actually hovered.
  if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    return;

  G_ASSERT(control);
  lastHoveredControl = control;

  if (preventTooltips || control == preventTooltipForControl)
    return;

  if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal))
    return;

  if (!ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_None))
    return;

  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (!window->SkipItems)
    ImGui::TextUnformatted(text, text_end);

  ImGui::EndTooltip();
}

} // namespace PropPanel
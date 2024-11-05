//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

class FocusHelper
{
public:
  // show_focus_rectangle: display the navigation rectangle just like when navigation between controls with keyboard.
  //   If the focus rectangle is displayed then buttons can be pressed with the Space key, just like in Windows.
  void requestFocus(const void *control, bool show_focus_rectangle = false)
  {
    requestedControlToFocus = control;
    showFocusRectangle = show_focus_rectangle;
  }

  // Only needed to call this when you manually handle the focus setting outside of FocusHelper.
  void clearRequest() { requestedControlToFocus = nullptr; }

  void setFocusToNextImGuiControlIfRequested(const void *control, int offset = 0)
  {
    if (!control || control != requestedControlToFocus)
      return;

    ImGui::SetKeyboardFocusHere(offset);
    ImGui::FocusWindow(ImGui::GetCurrentWindow()); // Bring window to the front.
    if (showFocusRectangle)
    {
      // ImGui::NavRestoreHighlightAfterMove() almost does what we need, but we do not want hover highlight on the
      // focused button because that would cause a color change (changing back to the non-hovered color) when the user
      // moved the mouse. To avoid that we do not set NavDisableMouseHover to true.
      ImGui::GetCurrentContext()->NavDisableHighlight = false;
      ImGui::GetCurrentContext()->NavDisableMouseHover = false;
      ImGui::GetCurrentContext()->NavMousePosDirty = true;
    }

    clearRequest();
  }

  // If you handle the request outside of FocusHelper then you have to remove the request from FocusHelper with
  // clearRequest.
  const void *getRequestedControlToFocus() const { return requestedControlToFocus; }

private:
  const void *requestedControlToFocus = nullptr;
  bool showFocusRectangle = false;
};

extern FocusHelper focus_helper;

} // namespace PropPanel
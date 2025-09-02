//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace PropPanel
{

// What is this and why is this needed?
//
// Problem: when editing a text edit control (anything with an edit control, including the text edit component of spin
// edits) where the changes are only applied when pressing Enter then clicking in a tree the uncommitted (with the Enter
// key) edits made to the edit control might not be saved if the edit control's display is dependent on the selection in
// the tree. For example the Composites Tree in Asset Viewer and The Elements tree in Mission Editor are such trees.
//
// The issue happens because the selection change event in the tree is immediate, while the focus loss in the edit box
// is only detected in the next frame. Thus the edit box gets no chance to detect its focus loss (the condition for its
// value change event) before the selection changes.
//
// To fix this issue this interface and the functions below can be used to send an immediate notification before the
// change of the selection in the tree to the focused edit box.
class IImmediateFocusLossHandler
{
public:
  virtual void onImmediateFocusLoss() = 0;
};

// Private, do not touch it.
extern IImmediateFocusLossHandler *focused_immediate_focus_loss_handler;

// Returns with the currently focused control that will receive the immediate focus loss notification.
inline IImmediateFocusLossHandler *get_focused_immediate_focus_loss_handler() { return focused_immediate_focus_loss_handler; }

// Call this if a control that needs immediate focus loss notification has focus.
// Call it with null if the control matches get_focused_immediate_focus_loss_handler() and had lost focus or destroyed.
//
// So the recommended usage is like this:
// From the destructor:
//   if (get_focused_immediate_focus_loss_handler() == this)
//     set_focused_immediate_focus_loss_handler(nullptr);
//
// After ImGui::InputText():
//   if (textInputFocused)
//     set_focused_immediate_focus_loss_handler(textInput);
//   else if (get_focused_immediate_focus_loss_handler() == textInput)
//     set_focused_immediate_focus_loss_handler(nullptr);
inline void set_focused_immediate_focus_loss_handler(IImmediateFocusLossHandler *control = nullptr)
{
  focused_immediate_focus_loss_handler = control;
}

// Call this if you immediately take away the focus from another control. Immediately means that in a manner that ImGui
// cannot react to it in the next frame. Generally you need to call this if you use mouse button down to focus on an
// control instead of mouse button release (like ImGui mostly does).
void send_immediate_focus_loss_notification();

} // namespace PropPanel
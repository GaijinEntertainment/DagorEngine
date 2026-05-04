// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace darg
{

class GuiScene;
class Element;

class KbFocus
{
public:
  KbFocus(GuiScene *gui_scene) : scene(gui_scene) {}
  void reset();

  void onElementDetached(Element *elem);
  void setFocus(Element *elem, bool capture = false);
  void captureFocus(Element *elem) { setFocus(elem, true); }
  bool hasCapturedFocus() const { return focus && isCaptured; }
  void applyNextFocus();

public:
  Element *focus = nullptr;
  bool isCaptured = false;
  Element *nextFocus = nullptr;
  bool nextFocusQueued = false;
  bool nextFocusNeedCapture = false;

  GuiScene *scene = nullptr;

  // ImmAssociateContextEx() called from WinKeyboardDevice::enableIme()
  // hangs when called after window close button was clicked.
  // It doesn't seem that there is a good way to handle that,
  // so we listen to WM_SYSCOMMAND+SC_CLOSE message and stop using IME API
  // assuming that the app window will close and no one stops that.
  // If they do stop closing, this flag has to be reset to false, too.
  bool sysCloseRequested = false;
};

} // namespace darg
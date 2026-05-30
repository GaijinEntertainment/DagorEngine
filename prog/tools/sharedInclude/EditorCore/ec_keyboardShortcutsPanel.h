// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class KeyboardShortcutsPanel
{
public:
  KeyboardShortcutsPanel();
  ~KeyboardShortcutsPanel();

  void updateImgui();

private:
  struct State;
  State *state;
};

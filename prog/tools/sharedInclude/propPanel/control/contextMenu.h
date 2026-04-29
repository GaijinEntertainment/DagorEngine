//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/control/menuImplementation.h>

namespace PropPanel
{

// Generally you do not need this use this.
// You are likely looking for create_context_menu() instead. It can be found in propPanel/control/menu.h.
class ContextMenu : public Menu
{
public:
  // See the comment at PropPanel::render_context_menu() regarding the return value.
  bool updateImgui();

private:
  bool isContextMenu() const override { return true; }
  void onImguiDelayedCallback(void *user_data) override;

  bool calledOpenPopup = false;
  bool waitingForDelayedCallback = false;
};

} // namespace PropPanel
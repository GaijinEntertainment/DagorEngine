// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "menuInternal.h"

namespace PropPanel
{

class ContextMenu : public Menu
{
public:
  // See the comment at PropPanel::render_context_menu() regarding the return value.
  bool updateImgui();

private:
  virtual bool isContextMenu() const override { return true; }
  virtual void onImguiDelayedCallback(void *user_data) override;

  bool calledOpenPopup = false;
  bool waitingForDelayedCallback = false;
};

} // namespace PropPanel
// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_modelessWindowController.h>
#include <util/dag_string.h>

class ViewportWindow;

class ViewportWindowStatSettingsDialogController final : public IModelessWindowController
{
public:
  ViewportWindowStatSettingsDialogController(ViewportWindow &viewport_window, int viewport_index);

  const char *getWindowId() const override;
  void releaseWindow() override;
  void showWindow(bool show = true) override;
  bool isWindowShown() const override;

private:
  bool isOwningDialog() const;

  const String windowId;
  ViewportWindow &viewportWindow;
};

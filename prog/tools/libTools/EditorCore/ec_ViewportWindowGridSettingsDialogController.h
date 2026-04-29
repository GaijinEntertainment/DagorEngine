// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_modelessWindowController.h>
#include <util/dag_string.h>

class GridEditDialog;

class ViewportWindowGridSettingsDialogController final : public IModelessWindowController
{
public:
  explicit ViewportWindowGridSettingsDialogController(int viewport_index);

  const char *getWindowId() const override;
  void releaseWindow() override;
  void showWindow(bool show = true) override;
  bool isWindowShown() const override;

  static void onGridVisibilityChanged(int viewport_index);
  static void onSnapSettingChanged();

private:
  bool isOwningDialog() const;

  const String windowId;
  const int viewportIndex;
};

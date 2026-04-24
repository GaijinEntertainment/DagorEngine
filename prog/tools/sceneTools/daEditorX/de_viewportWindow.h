// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_ViewportWindow.h>

class BaseTexture;

class DagorEdViewportWindow : public ViewportWindow
{
private:
  int onMenuItemClick(unsigned id) override;
  bool onDropFiles(const dag::Vector<String> &files) override;
  bool canStartInteractionWithViewport() override;
  BaseTexture *getDepthBuffer() override;
  void fillStatSettingsDialog(ViewportWindowStatSettingsDialog &dialog, bool include_camera_distance) override;
};

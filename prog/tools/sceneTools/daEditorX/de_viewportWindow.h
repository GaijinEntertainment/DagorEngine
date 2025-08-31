// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_ViewportWindow.h>

class BaseTexture;

class DagorEdViewportWindow : public ViewportWindow
{
private:
  bool onDropFiles(const dag::Vector<String> &files) override;
  bool canStartInteractionWithViewport() override;
  BaseTexture *getDepthBuffer() override;
};

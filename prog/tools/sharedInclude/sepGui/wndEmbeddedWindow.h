//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class IWndEmbeddedWindow
{
public:
  virtual void onWmEmbeddedResize(int width, int height) = 0;
  virtual bool onWmEmbeddedMakingMovable(int &width, int &height) = 0;
};

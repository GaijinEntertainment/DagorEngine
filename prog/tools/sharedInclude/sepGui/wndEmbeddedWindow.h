//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class IWndEmbeddedWindow
{
public:
  virtual void onWmEmbeddedResize(int width, int height) = 0;
  virtual bool onWmEmbeddedMakingMovable(int &width, int &height) = 0;
};

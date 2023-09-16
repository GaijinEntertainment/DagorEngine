//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <render/genericLUT/genericLUT.h>

class CommonLUT
{
public:
  bool isInited() const;
  void requestUpdate(); // call it each time you think shader variables had changed. It WILL force rebuilding of LUT on next render()
  bool render();        // call it each frame, always, as early in frame as possible, but after requestUpdate().
                        // it won't render anything at all, unless update is called/device resetted
protected:
  CommonLUT();

  GenericTonemapLUT lutBuilder;
  bool frameReady = 0;
  int driverResetCounter = 0;
};

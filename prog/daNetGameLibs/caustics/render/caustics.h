// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/daFG.h>

eastl::array<dafg::NodeHandle, 2> makeCausticsNode();
struct CausticsSetting
{
  float causticsScrollSpeed = 0.75;
  float causticsWorldScale = 0.25;
};

void queryCausticsSettings(CausticsSetting &settings);

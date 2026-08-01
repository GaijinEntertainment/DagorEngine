// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/daFG.h>

dafg::NodeHandle makeCausticsPerCameraResNode();
dafg::NodeHandle makeCausticsRenderNode(const char *view_ns, bool is_main_view);
struct CausticsSetting
{
  float causticsScrollSpeed = 0.75;
  float causticsWorldScale = 0.25;
};

void queryCausticsSettings(CausticsSetting &settings);

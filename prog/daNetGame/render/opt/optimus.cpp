// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <Windows.h>
// The Optimus driver looks for the existence and value of the export.
// A value of 0x00000001 indicates that rendering should be performed using High Performance Graphics.
// Same for Enduro technology on AMD A-series APUs.
extern "C"
{
  __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
  __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

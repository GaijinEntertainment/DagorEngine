//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if !_TARGET_PC_WIN
#error PC Win32/Win64 only.
#endif

#if SUPPORT_PC_KINECT
#include "NuiApi.h"
#else
#define NUI_SKELETON_DATA  int
#define NUI_SKELETON_FRAME int
#endif

class Kinect
{
public:
  Kinect();
  ~Kinect();

  void update();
  NUI_SKELETON_DATA *getSkeleton();
  bool isConnected() { return connected; }
  unsigned int isTracked() { return currentSkeletonNo != 0xFFFFFFFF; }

protected:
  NUI_SKELETON_FRAME skeletonFrame;
  unsigned int currentSkeletonNo;
  unsigned int currentTrackingId;
  bool initialized;
  bool connected;

  void init();
};

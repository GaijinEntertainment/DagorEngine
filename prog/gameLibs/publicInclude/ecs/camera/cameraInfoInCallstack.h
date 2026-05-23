//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_stackHlp.h>
#include <ecs/camera/getActiveCameraSetup.h>

struct CameraInfoInCallstack : stackhelp::ext::ScopedCallStackContext
{
  CameraInfoInCallstack(const CameraSetup &cam, int w, int h);

private:
  struct Data
  {
    Point3 pos = Point3::ZERO;
    Point3 dir = Point3::ZERO;
    float fov = 0;
    int w = 0;
    int h = 0;
    int padding = 0; // to have size mod 8 == 0
  };
  Data data;
  static constexpr size_t DATA_SIZE_IN_POINTERS = (sizeof(Data) + sizeof(void *) - 1) / sizeof(void *);

  static stackhelp::ext::CallStackResolverCallbackAndSizePair saveCameraInfo(stackhelp::CallStackInfo stack, void *context);

  static stackhelp::ext::ResolvedRecord logCameraInfo(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack);
};

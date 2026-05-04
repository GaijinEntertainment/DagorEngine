//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_stackHlp.h>
#include <ecs/camera/getActiveCameraSetup.h>

struct CameraPosInCallstack : stackhelp::ext::ScopedCallStackContext
{
  CameraPosInCallstack(const CameraSetup &cam);

private:
  struct Data
  {
    Point3 pos = Point3::ZERO;
    Point3 dir = Point3::ZERO;
  };
  Data data;
  static constexpr size_t DATA_SIZE_IN_POINTERS = (sizeof(Data) + sizeof(void *) - 1) / sizeof(void *);

  static stackhelp::ext::CallStackResolverCallbackAndSizePair saveCameraPos(stackhelp::CallStackInfo stack, void *context);

  static stackhelp::ext::ResolvedRecord logCameraPos(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack);
};

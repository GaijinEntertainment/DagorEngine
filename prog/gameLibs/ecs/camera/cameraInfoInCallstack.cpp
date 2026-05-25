// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/camera/cameraInfoInCallstack.h>

#include <stdio.h>
#include <EASTL/algorithm.h>


CameraInfoInCallstack::CameraInfoInCallstack(const CameraSetup &cam, int w, int h) :
  ScopedCallStackContext{saveCameraInfo, this}, data{cam.transform.getcol(3), cam.transform.getcol(2), cam.fov, w, h}
{}

stackhelp::ext::CallStackResolverCallbackAndSizePair CameraInfoInCallstack::saveCameraInfo(stackhelp::CallStackInfo stack,
  void *context)
{
  CameraInfoInCallstack *self = static_cast<CameraInfoInCallstack *>(context);

  if (stack.stackSize < DATA_SIZE_IN_POINTERS)
    return self->invokePrev(stack);

  memcpy(&stack.stack[0], &self->data, sizeof(Data));

  return self->invokeChain<logCameraInfo>(stack, DATA_SIZE_IN_POINTERS);
}

stackhelp::ext::ResolvedRecord CameraInfoInCallstack::logCameraInfo(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack)
{
  if (stack.stackSize < DATA_SIZE_IN_POINTERS)
    return {};

  const Data &data = *reinterpret_cast<const Data *>(stack.stack);

  int ret = snprintf(buf, max_buf, "Camera pos=%.3f,%.3f,%.3f dir=%.3f,%.3f,%.3f fov=%.2f res=%dx%d\n", data.pos.x, data.pos.y,
    data.pos.z, data.dir.x, data.dir.y, data.dir.z, data.fov, data.w, data.h);

  return {static_cast<unsigned>(ret < 0 ? 0 : ret), DATA_SIZE_IN_POINTERS};
}
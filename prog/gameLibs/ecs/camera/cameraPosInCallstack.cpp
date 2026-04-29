// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/camera/cameraPosInCallstack.h>

#include <stdio.h>
#include <EASTL/algorithm.h>


CameraPosInCallstack::CameraPosInCallstack(const CameraSetup &cam) :
  ScopedCallStackContext{saveCameraPos, this}, data{cam.transform.getcol(3), cam.transform.getcol(2)}
{}

stackhelp::ext::CallStackResolverCallbackAndSizePair CameraPosInCallstack::saveCameraPos(stackhelp::CallStackInfo stack, void *context)
{
  CameraPosInCallstack *self = static_cast<CameraPosInCallstack *>(context);

  if (stack.stackSize < DATA_SIZE_IN_POINTERS)
    return self->invokePrev(stack);

  memcpy(&stack.stack[0], &self->data, sizeof(Data));

  return self->invokeChain<logCameraPos>(stack, DATA_SIZE_IN_POINTERS);
}

stackhelp::ext::ResolvedRecord CameraPosInCallstack::logCameraPos(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack)
{
  if (stack.stackSize < DATA_SIZE_IN_POINTERS)
    return {};

  Data data;
  memcpy(&data, stack.stack, sizeof(data));

  int ret = snprintf(buf, max_buf, "Camera pos=%.3f,%.3f,%.3f dir=%.3f,%.3f,%.3f\n", data.pos.x, data.pos.y, data.pos.z, data.dir.x,
    data.dir.y, data.dir.z);

  return {static_cast<unsigned>(ret < 0 ? 0 : ret), DATA_SIZE_IN_POINTERS};
}
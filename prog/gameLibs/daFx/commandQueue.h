// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>

namespace dafx
{
struct CommandQueue
{
  struct CreateInstance
  {
    SystemId sid;
    InstanceId iid;
  };

  struct InstancePos
  {
    InstanceId iid;
    Point4 pos;
  };

  struct InstanceWarmup
  {
    InstanceId iid;
    float time;
  };

  struct InstanceEmissionRate
  {
    InstanceId iid;
    float rate;
  };

  struct InstanceStatus
  {
    InstanceId iid;
    bool enabled;
  };

  struct InstanceVisibility
  {
    InstanceId iid;
    uint32_t visibility;
  };

  struct InstanceValue
  {
    InstanceId iid;
    bool isOpt;
    int size;
    eastl::string name;
    int srcOffset;
  };

  struct InstanceValueDirect
  {
    InstanceId iid;
    int subIdx;
    int size;
    int srcOffset;
    int dstOffset;
  };

  struct InstanceValueFromSystemScaled
  {
    InstanceId iid;
    SystemId sysid;
    int subIdx;
    int size;
    int vecOffset;
    int dstOffset;
  };

  eastl::vector<CreateInstance> createInstance;
  eastl::vector<InstanceId> destroyInstance;
  eastl::vector<InstanceId> resetInstance;

  eastl::vector<InstancePos> instancePos;
  eastl::vector<InstanceStatus> instanceStatus;
  eastl::vector<InstanceVisibility> instanceVisibility;
  eastl::vector<InstanceWarmup> instanceWarmup;
  eastl::vector<InstanceEmissionRate> instanceEmissionRate;
  eastl::vector<InstanceValue> instanceValue;
  eastl::vector<InstanceValueDirect> instanceValueDirect;
  eastl::vector<InstanceValueFromSystemScaled> instanceValueFromSystemScaled;

  dag::Vector<unsigned char, EASTLAllocatorType, /*init*/ false> instanceValueData;
};
} // namespace dafx
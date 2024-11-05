// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ADT/uniqueResHandle.h>
#include <render/clusteredLights.h>

struct LightHandleDeleter
{
  void operator()(int h) const;
};
typedef UniqueResHandle<LightHandleDeleter, unsigned, ClusteredLights::INVALID_LIGHT> LightHandle;

//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "projective_decals.hlsli"
#include <3d/dag_resPtr.h>
#include <EASTL/string.h>

struct DecalsBuffer
{
  eastl::string decalBufferName, culledBufferName;
  BufPtr decalInstances;
  UniqueBuf culledInstances;
  uint32_t bufferCapacity;

  DecalsBuffer() = default;
  DecalsBuffer(uint32_t size, const char *decal_buffer_name, const char *culled_buffer_name);
  void resize_buffers(unsigned int size);
};

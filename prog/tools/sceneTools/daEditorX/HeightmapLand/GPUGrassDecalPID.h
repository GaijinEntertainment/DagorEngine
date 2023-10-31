#pragma once
#include <math/dag_hlsl_floatx.h>
#include <render/grassInstance.hlsli>

enum class GPUGrassDecalPID
{
  NAME,
  ID,
  ADD_TYPE,
  REMOVE_TYPE,
  TYPES_START,
  TYPES_END = TYPES_START + MAX_TYPES_PER_CHANNEL,
  COUNT
};
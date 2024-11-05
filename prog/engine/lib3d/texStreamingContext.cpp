// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texStreamingContext.h>
#include <util/dag_convar.h>
#include <math/dag_TMatrix4.h>

#define PROFILE_TEX_STREAMING_MEMORY 0

#if PROFILE_TEX_STREAMING_MEMORY
CONSOLE_INT_VAL("tex_streaming_level", override, TexStreamingContext::minLevel, -1, TexStreamingContext::maxLevel);
#endif

TexStreamingContext::TexStreamingContext(const Driver3dPerspective &persp, int w)
{
  multiplicator = persp.wk * w;
  multiplicator *= multiplicator;
}

int TexStreamingContext::getTexLevel(float texScale, float distSq) const
{
  G_ASSERT_RETURN(!isnan(multiplicator), maxLevel);
  if (multiplicator == 0.0f)
    return minLevel;
  if (texScale == 0.0f || distSq == 0.0f || multiplicator == FLT_MAX)
    return maxLevel;
  int desiredTexLevel;
  // acutually computes ceil(log2(sqrt( multiplicator * texScale / (perspective ? distSq : 1.0f) )))
  frexp(multiplicator * texScale / distSq, &desiredTexLevel);
  desiredTexLevel = max(desiredTexLevel, minLevel);
  desiredTexLevel = (desiredTexLevel >> 1) + (desiredTexLevel & 1);
#if PROFILE_TEX_STREAMING_MEMORY
  if (override.get() >= 0)
    desiredTexLevel = override.get();
#endif
  return min(maxLevel, desiredTexLevel);
}

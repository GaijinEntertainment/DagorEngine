// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texStreamingContext.h>
#include <drv/3d/dag_decl.h>
#include <util/dag_convar.h>
#include <math/dag_TMatrix4.h>

#define PROFILE_TEX_STREAMING_MEMORY 0

#if PROFILE_TEX_STREAMING_MEMORY
CONSOLE_INT_VAL("tex_streaming_level", override, TexStreamingContext::MIN_TEX_LEVEL, -1, TexStreamingContext::MAX_TEX_LEVEL);
#endif

TexStreamingContext::TexStreamingContext(const Driver3dPerspective &persp, int w)
{
  multiplicator = persp.wk * w;
  multiplicator *= multiplicator;
}

int TexStreamingContext::getTexLevel(float texScale, float distSq) const
{
  G_ASSERT(multiplicator >= 0.0f);
  if (multiplicator == 0.0f)
    return MIN_TEX_LEVEL;
  if (texScale == 0.0f || distSq == 0.0f || multiplicator == FLT_MAX)
    return MAX_TEX_LEVEL;
  int desiredTexLevel;
  // acutually computes ceil(log2(sqrt( multiplicator * texScale / (perspective ? distSq : 1.0f) )))
  frexp(multiplicator * texScale / distSq, &desiredTexLevel);
  desiredTexLevel = max(desiredTexLevel, MIN_TEX_LEVEL);
  desiredTexLevel = (desiredTexLevel >> 1) + (desiredTexLevel & 1);

  // MIN_LEVEL(1) is also a default value for LdLev for resources that are stubbed, streaming waits until it's
  // changed to > 1 to start loading data for them so here to get an actually meaningful level we need to return
  // at least (MIN_LEVEL + 1)
  desiredTexLevel = max(desiredTexLevel, MIN_NON_STUB_LEVEL);
#if PROFILE_TEX_STREAMING_MEMORY
  if (override.get() >= 0)
    desiredTexLevel = override.get();
#endif
  return min(MAX_TEX_LEVEL, desiredTexLevel);
}

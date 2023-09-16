#ifndef _DAGOR_GAMELIB_RENDER_ESMAODECALS_HLSLI_
#define _DAGOR_GAMELIB_RENDER_ESMAODECALS_HLSLI_

#define MAX_ESM_AO_DECALS 128

struct EsmAoDecal
{
  float3 tmRow0; uint param0;
  float3 tmRow1; uint param1;
  float3 tmRow2; uint param2;
  float3 center; uint texSliceId;
};

#endif
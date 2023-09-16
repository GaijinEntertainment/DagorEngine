#ifndef _DAGOR_GAMELIB_RENDER_CABLES_HLSL_
#define _DAGOR_GAMELIB_RENDER_CABLES_HLSL_

struct CableData
{
  float4 point1_rad;
  float4 point2_sag;
};

#define TRIANGLES_PER_CABLE 20 //must be even

#endif
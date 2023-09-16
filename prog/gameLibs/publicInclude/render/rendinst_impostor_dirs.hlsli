#ifndef __RENDINST_IMPOSTOR_DIRS_H__
#define __RENDINST_IMPOSTOR_DIRS_H__

#define IMPOSTOR_D 0.7071067811

static const float3 IMPOSTOR_SLICE_DIRS[9] =
{
  float3( 1, 0, 0 ),
  float3( IMPOSTOR_D, 0, IMPOSTOR_D ),
  float3( 0, 0, 1 ),
  float3( -IMPOSTOR_D, 0, IMPOSTOR_D ),
  float3( -1, 0, 0 ),
  float3( -IMPOSTOR_D, 0, -IMPOSTOR_D ),
  float3( 0, 0, -1 ),
  float3( IMPOSTOR_D, 0, -IMPOSTOR_D ),
  float3( 0, -1, 0 )
};

static const float3 IMPOSTOR_VIEW_DIRS[9] =
{
  float3( 0, 0, -1 ),
  float3( IMPOSTOR_D, 0, -IMPOSTOR_D ),
  float3( 1, 0, 0 ),
  float3( IMPOSTOR_D, 0, IMPOSTOR_D ),
  float3( 0, 0, 1 ),
  float3( -IMPOSTOR_D, 0, IMPOSTOR_D ),
  float3( -1, 0, 0 ),
  float3( -IMPOSTOR_D, 0, -IMPOSTOR_D ),
  float3( 0, 0, 0 )
};

#undef IMPOSTOR_D

#endif // __RENDINST_IMPOSTOR_DIRS_H__

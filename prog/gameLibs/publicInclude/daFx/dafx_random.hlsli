#ifndef DAFX_RANDOM_HLSL
#define DAFX_RANDOM_HLSL

#define DAFX_RND_MAX (0x7fff)
#define DAFX_RND_MAX_INV ( 1.f / DAFX_RND_MAX )

#define rnd_seed_t uint

#ifdef __cplusplus
  #define rnd_seed_ref rnd_seed_t&
#else
  #define rnd_seed_ref in out rnd_seed_t
#endif

DAFX_INLINE
uint dafx_fastrnd( rnd_seed_ref seed )
{
  seed = ( 214013 * ( seed % 0xffffff ) + 2531011 );
  return ( seed >> 16 ) & DAFX_RND_MAX;
};

DAFX_INLINE
uint dafx_uirnd( rnd_seed_ref seed )
{
  return dafx_fastrnd( seed );
}

DAFX_INLINE
float dafx_frnd( rnd_seed_ref seed )
{
  return float( dafx_fastrnd( seed ) ) * DAFX_RND_MAX_INV;
}

DAFX_INLINE
float dafx_srnd( rnd_seed_ref seed )
{
  return dafx_frnd( seed ) * 2.f - 1.f;
}

DAFX_INLINE
float2 dafx_srnd_vec2( rnd_seed_ref seed )
{
  float2 v;
  v.x = dafx_srnd( seed );
  v.y = dafx_srnd( seed );
  return v;
}

DAFX_INLINE
float3 dafx_srnd_vec3( rnd_seed_ref seed )
{
  float3 v;
  v.x = dafx_srnd( seed );
  v.y = dafx_srnd( seed );
  v.z = dafx_srnd( seed );
  return v;
}

DAFX_INLINE
float2 dafx_frnd_vec2( rnd_seed_ref seed )
{
  float2 v;
  v.x = dafx_frnd( seed );
  v.y = dafx_frnd( seed );
  return v;
}

DAFX_INLINE
float3 dafx_frnd_vec3( rnd_seed_ref seed )
{
  float3 v;
  v.x = dafx_frnd( seed );
  v.y = dafx_frnd( seed );
  v.z = dafx_frnd( seed );
  return v;
}

DAFX_INLINE
float4 dafx_frnd_vec4( rnd_seed_ref seed )
{
  float4 v;
  v.x = dafx_frnd( seed );
  v.y = dafx_frnd( seed );
  v.z = dafx_frnd( seed );
  v.w = dafx_frnd( seed );
  return v;
}

DAFX_INLINE
rnd_seed_t dafx_calc_instance_rnd_seed( uint gid, uint dispatch_seed )
{
  // avalanche hash (lowbias32): an affine seed (A*gid+c) makes every LCG draw
  // affine in gid too, collapsing spawn positions to a ~3-step lattice per axis
  rnd_seed_t h = gid * 0x9E3779B9u + dispatch_seed;
  h ^= h >> 16;
  h *= 0x7feb352d;
  h ^= h >> 15;
  h *= 0x846ca68b;
  h ^= h >> 16;
  return h;
}

#endif
#ifndef DAGI_VOLMAP_CB_INCLUDED
#define DAGI_VOLMAP_CB_INCLUDED 1

#define LAST_VOLMAP_CASCADE 1
#define MAX_VOLMAP_CASCADES (LAST_VOLMAP_CASCADE+1)

#ifdef __cplusplus
  #define INITIALIZER(...)  = {__VA_ARGS__}
  //for PS4 PSSL1
  #define INITIALIZER2(...) = {__VA_ARGS__}
  #define INITIALIZER3(...) = {__VA_ARGS__}
  #define INITIALIZER4(...) = {__VA_ARGS__}
#else
  #define INITIALIZER(a)
  #define INITIALIZER2(a,b)
  #define INITIALIZER3(a,b,c)
  #define INITIALIZER4(a,b,c,d)
#endif

struct VolmapCB
{
  float3 box0 INITIALIZER3(1,1,1); uint xz_dim INITIALIZER(0);
  float3 box1 INITIALIZER3(0,0,0); uint y_dim INITIALIZER(0);
  float4 tcclamp INITIALIZER4(0,0,1,1);
  float2 voxelSize INITIALIZER2(0.5, 1);
  float2 cascade_z_tc_mul_add INITIALIZER2(1, 0);
  float3 alignedOrigin INITIALIZER3(0,-10000,0); uint cascade_z_ofs INITIALIZER(0);
  float3 coord_to_world_add INITIALIZER3(0,0,0); uint use_floor INITIALIZER(0);
  int2 crd_ofs_xz INITIALIZER2(0,0); int2 resv1 INITIALIZER2(0,0);
  uint4 max_voxels_per_bin INITIALIZER4(64, 32, 4096, 64+32);

  float2 tc_ofs INITIALIZER2(0,0);//crd_ofs_xz/xz_dim
  uint2 crd_pofs_uint2 INITIALIZER2(0,0);//(crd_ofs_xz+xz_dim)
  uint2 crd_nofs_uint2 INITIALIZER2(0,0);//(-crd_ofs_xz+xz_dim)
  float2 tcClampDouble INITIALIZER2(1,1);
  float4 fullVolmapResolution INITIALIZER4(64, 64, (32+16)*2, -0.5);
  float4 vignetteConsts INITIALIZER4((32-0.5)/10.0f,(16.-0.5)/4.f,-((32-2)/10.f - 1),-((16-2)/4.f - 1));
  float3 invFullVolmapResolution INITIALIZER3(1.0 / 64, 1.0 / 64, 1.0 / ((32+16)*2));
  uint resv2 INITIALIZER(0);
};

#undef INITIALIZER

#endif
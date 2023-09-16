#define MAX_XRAY_PARTS 512

#ifdef __cplusplus
using float4x4 = TMatrix4;
#endif

struct XrayPartParams
{
  float is_simulated;
  float explosion_scale;
  float pad0;
  float dissolve_rad;
  float4 hatching_color;
  float4 hatching_fresnel;
  float4 hatching_type;
  float4x4 world_local;
};

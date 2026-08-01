static const int DEBUG_MESH_COLORS__MAX = 6;

#ifndef __cplusplus
half3 debug_mesh_get_color(int value)
{
  static const half3 DEBUG_MESH_COLORS[DEBUG_MESH_COLORS__MAX + 1] = {
    half3(1, 1, 1), // 0  : white
    half3(1, 0, 0), // 1  : red
    half3(1, 1, 0), // 2  : yellow
    half3(0, 1, 0), // 3  : green
    half3(0, 1, 1), // 4  : cyan
    half3(0, 0, 1), // 5  : blue
    half3(1, 0, 1), // 6+ : magenta
  };
  return DEBUG_MESH_COLORS[clamp(value, 0, DEBUG_MESH_COLORS__MAX)];
}

#define LOD_DIFFUSE_WEIGHT 0.1
#define LOD_AMBIENT_WEIGHT 0.5
#define LOD_BACKGROUND_TINT 0.6

half3 debug_mesh_shade(half3 lod_color, half3 albedo, float3 normal, float3 light_dir)
{
  half3 diffuse = lerp(lod_color, albedo, LOD_DIFFUSE_WEIGHT);
  float NoL = dot(normal, light_dir);
  return diffuse * lerp(max(NoL, 0), 1, LOD_AMBIENT_WEIGHT);
}

half3 debug_mesh_shade_lod(int lod, half3 albedo, float3 normal, float3 light_dir)
{
  return debug_mesh_shade(debug_mesh_get_color(lod), albedo, normal, light_dir);
}
#endif

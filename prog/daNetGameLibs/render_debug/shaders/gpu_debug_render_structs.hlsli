#ifndef GPU_DEBUG_RENDER_STRUCTS_HLSLI
#define GPU_DEBUG_RENDER_STRUCTS_HLSLI

#define GPU_DEBUG_RENDER_MAX_ELEMS_PER_TYPE (128 * 1024)

struct GpuDrivenDebugRenderFlatSphere
{
  float3 centerWpos;
  float radius;
  float4 color;
};

struct GpuDrivenDebugRenderLine
{
  float3 startWpos;
  float startSize;
  float3 endWpos;
  float endSize;
  float4 startColor;
  float4 endColor;
};

#endif
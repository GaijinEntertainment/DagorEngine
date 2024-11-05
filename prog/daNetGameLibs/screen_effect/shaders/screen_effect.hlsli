#define MAX_SCREEN_EFFECTS 5

struct ScreenEffect
{
  float4 diffuse;
  float2 uv_scale;
  float2 padding;
  float roughness;
  float opacity;
  float intensity;
  float weight;
  float border_offset; // [0,1] add const intensity to effect.
  float border_saturation; // [0,1] determine thickness of attenuation

  float2 _align; // Struct should have a size divisible by sizeof(float4), because the cbuffer is a set of float4 registers.
};
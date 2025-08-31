#ifndef WETNESS_FUNCTIONS
#define WETNESS_FUNCTIONS 1
float clamp_range(float v, float minV, float maxV) {return saturate((v-minV)/(maxV-minV));}
void wetness_reflectance(half wetness, inout half reflectance)
{
  reflectance = lerp(reflectance, 0.3535, clamp_range(wetness, 0.25, 0.5));//sqrt(0.02/0.16)
}
void wetness_specular(half wetness, inout half3 specular)
{
  specular = lerp(specular, 0.02, clamp_range(wetness, 0.25, 0.5));
}
void wetness_metalness(half wetness, inout half metalness)
{
  metalness = lerp(metalness, 0, clamp_range(wetness, 0.25, 0.5));
}
void wetness_ao(half wetness, inout half ao)
{
  ao = lerp(ao, 1, clamp_range(wetness, 0.45, 0.95));
}
void wetness_diffuse(half wetness, half porosity, inout half3 diffuse)
{
  half3 diffuseSq = diffuse*diffuse;
  diffuse = lerp(diffuse, diffuseSq, clamp_range(wetness, 0, 0.35)*porosity);
}
void wetness_roughness(half wetness, inout half linearRoughness)
{
  linearRoughness = lerp(linearRoughness, 0.1, clamp_range(wetness, 0.2, 1));
}
void wetness_smoothness(half wetness, inout half smoothness)
{
  smoothness = lerp(smoothness, 0.99, clamp_range(wetness, 0.2, 1));
}
void wetness_normal_ts(half wetness, inout half2 normalTS)
{
  normalTS = lerp(normalTS, half2(0.5,0.5), clamp_range(wetness, 0.45, 0.95));
}
void wetness_normal_ws(half wetness, inout half3 normalWS)
{
  normalWS = lerp(normalWS, float3(0,1,0 ), clamp_range(wetness, 0.98, 1.0));
}

void apply_wetness(half wetness, inout half3 diffuse, inout half linearRoughness, inout half ao, inout half3 normalWS)
{
  wetness_diffuse(wetness, linearRoughness, diffuse);
  wetness_roughness(wetness, linearRoughness);
  wetness_ao(wetness, ao);
  wetness_normal_ws(wetness, normalWS);
}

#endif
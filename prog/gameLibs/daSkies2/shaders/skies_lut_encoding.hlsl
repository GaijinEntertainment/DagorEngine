#ifndef SKIES_LUT_ENCODING_HLSL_INCLUDED
#define SKIES_LUT_ENCODING_HLSL_INCLUDED 1

#define SKIES_LUT_ENCODING 1
float skies_view_to_tc_y(float texcoordY, float norm_view_y, float4 params)
{
  #if !SKIES_LUT_ENCODING
  return texcoordY;
  #endif
  //return texcoordY;
  float retY;
  if (norm_view_y < params.y)
  {
    float lerpParam = (norm_view_y - params.x)/(params.y-params.x);
    retY = params.w*(1-sqrt(saturate(1-lerpParam)));
  } else
  {
    float lerpParam = (params.z - norm_view_y)/(params.z-params.y);
    retY = params.w + (1-params.w)*(sqrt(saturate(1-lerpParam)));
  }
  retY = 1-retY;
  if (params.w < 0)
    retY = texcoordY;
  return retY;
}
float3 skies_tc_to_view(float texcoordY, float3 view, float4 params)
{
  #if !SKIES_LUT_ENCODING
  return normalize(view);
  #endif
  texcoordY = 1-texcoordY;
  float viewVectY;
  if (texcoordY < params.w)
  {
    float lerpParam = (params.w - texcoordY)/params.w;
    viewVectY = lerp(params.y, params.x, pow2(lerpParam));
  } else
  {
    float lerpParam = (texcoordY-params.w)/(1-params.w);
    viewVectY = lerp(params.y, params.z, pow2(lerpParam));
  }
  if (params.w >= 0)
    return float3(normalize(view.xz)*sqrt(saturate(1-viewVectY*viewVectY)), viewVectY).xzy;
  return normalize(view);
}

#endif
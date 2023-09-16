#ifndef GET_CUBE_VECTOR_HLSL
#define GET_CUBE_VECTOR_HLSL 1
  float3 GetCubemapVector(float2 uv, int cubeFace)//uv = uv*2-1
  {
    return (cubeFace < 2) ? (cubeFace < 1 ? float3(1, uv.y, -uv.x) : float3(-1, uv.y, uv.x)) :
           (cubeFace < 4) ? (cubeFace < 3 ? float3(uv.x, 1, -uv.y) : float3(uv.x, -1, uv.y)) :
                            (cubeFace < 5 ? float3(uv.x, uv.y, 1) : float3(-uv.x, uv.y, -1));
  }

  float3 GetCubemapVector2(float2 uv, int cubeFace)
  {
    return GetCubemapVector((uv * 2 - 1) * float2(1, -1), cubeFace);
  }
#endif
struct VsOut
{
  float4 pos : SV_POSITION;
  float2 coord : TEXCOORD0;
};

// only need to modify coords for source rect transform
// dest rect transform is done with viewport
float4 coordOffsetScale : register(c0);

#if __XBOX_ONE || __XBOX_SCARLETT
#include "blit.signature.hlsl"
[RootSignature(BLIT_SIGNATURE)]
#endif
VsOut main(uint vid : SV_VertexID)
{
  VsOut result;
  result.coord = float2((vid << 1) & 2, vid & 2);
  result.pos = float4(result.coord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
  result.coord = coordOffsetScale.xy + (result.coord * coordOffsetScale.zw);

  return result;
}
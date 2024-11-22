struct VsOut
{
  float4 pos : SV_POSITION;
};

#if __XBOX_ONE || __XBOX_SCARLETT
#include "clear.signature.hlsl"
[RootSignature(CLEAR_SIGNATURE)]
#endif
VsOut main(uint vid : SV_VertexID)
{
  VsOut result;
  float2 coord = float2((vid << 1) & 2, vid & 2);
  result.pos = float4(coord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
  return result;
}

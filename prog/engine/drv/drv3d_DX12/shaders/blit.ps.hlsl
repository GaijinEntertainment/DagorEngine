struct VsOut
{
  float4 pos : SV_POSITION;
  float2 coord : TEXCOORD0;
};

SamplerState smpl : register(s0, space0);
Texture2D tex : register(t0, space0);

#if __XBOX_ONE || __XBOX_SCARLETT
#include "blit.signature.hlsl"
[RootSignature(BLIT_SIGNATURE)]
#endif
float4 main(VsOut input) : SV_Target
{
  return tex.Sample(smpl, input.coord);
}
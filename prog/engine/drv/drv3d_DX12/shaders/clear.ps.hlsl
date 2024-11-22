struct VsOut
{
  float4 pos : SV_POSITION;
};

struct PSConstants
{
  float4 clearValue;
};
ConstantBuffer<PSConstants> constants : register(b0, space0);

#if __XBOX_ONE || __XBOX_SCARLETT
#include "clear.signature.hlsl"
[RootSignature(CLEAR_SIGNATURE)]
#endif
float4 main(VsOut input) : SV_Target
{
  return constants.clearValue;
}

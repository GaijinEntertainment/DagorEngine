struct VsOut
{
  float4 pos : SV_POSITION;
};

struct PSConstants
{
  uint4 clearValue;
};
ConstantBuffer<PSConstants> constants : register(b0, space0);

#if __XBOX_ONE || __XBOX_SCARLETT
#include "clear.signature.hlsl"
[RootSignature(CLEAR_SIGNATURE)]
#endif
uint4 main(VsOut input) : SV_Target
{
  return constants.clearValue;
}

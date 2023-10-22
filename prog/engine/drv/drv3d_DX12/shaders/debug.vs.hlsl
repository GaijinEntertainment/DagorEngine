#define __XBOX_DISABLE_PRECOMPILE 1
struct v2p
{
  float4 pos : SV_POSITION;
  float4 col : TEXCOORD0;
};

float4x4 globtm : register(c0);

v2p main(float4 pos : POSITION, float4 col : COLOR0)
{
  v2p o;
  o.pos = mul(pos, globtm);
  o.col = col;
  return o;
}
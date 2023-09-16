#define __XBOX_DISABLE_PRECOMPILE 1
struct v2p
{
  float4 pos : SV_POSITION;
  float4 col : TEXCOORD0;
};

float4 main(v2p i) : SV_Target
{
  return i.col;
}

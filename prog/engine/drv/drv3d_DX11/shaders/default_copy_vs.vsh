//default_copy_vs
struct v2p { float4 pos : SV_POSITION; float2 texcrd : TEXCOORD0; };
v2p main(float4 pos : POSITION)
{
  v2p o;
  o.pos = float4(pos.xy, 0, 1);
  o.texcrd = pos.zw * float2(0.5, -0.5) + float2(0.500000119, 0.500000119); // 0.5 + FLT_EPSILON is good enough for range -2..2
  return o;
}

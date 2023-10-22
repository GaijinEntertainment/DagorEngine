struct v2p { float4 pos : SV_POSITION; float4 color : TEXCOORD0; };
v2p main(float4 pos : POSITION, float4 color : TEXCOORD0) 
{
  v2p o; o.pos = pos;
  o.color = color;
  return o;
}

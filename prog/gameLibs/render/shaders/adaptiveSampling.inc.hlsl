  float2 dx = ddx(pos), dy = ddy(pos);
  float2 maxDXDY = max(abs(dx), abs(dy));
  float4 fxaaConsolePosPos = float4(pos - invSize, pos + invSize);

  half4 Nw = h4tex2D(tex,fxaaConsolePosPos.xy);
  half4 Sw = h4tex2D(tex,fxaaConsolePosPos.xw);
  half4 Ne = h4tex2D(tex,fxaaConsolePosPos.zy);
  half4 Se = h4tex2D(tex,fxaaConsolePosPos.zw);
  #ifndef LUMA
    half3 lumaC = normalize(half3(0.299,0.587,0.114));
    #define LUMA(a) dot((a).rgb, lumaC)
    #define DEF_LUMA 1
  #endif

  half lumaNw = LUMA(Nw);
  half lumaSw = LUMA(Sw);
  half lumaNe = LUMA(Ne);
  half lumaSe = LUMA(Se);

  half4 centerTap = h4tex2D(tex, pos);
  half4 rgbyM = centerTap;

  half dirSwMinusNe = lumaSw - lumaNe;
  half dirSeMinusNw = lumaSe - lumaNw;

  float2 dir;
  float2 halfInvSize = invSize;
  dir.x = dirSwMinusNe + dirSeMinusNw;
  dir.y = dirSwMinusNe - dirSeMinusNw;
  dir *= invSize;
  //dir = clamp(dir, -invSize*0.5, invSize*0.5);//reduces 'halo'

  float2 dir1 = dir;
  //dir1 *= 2.0;//can improve detail, but produces 'halo'
  half4 rgbyN1 = h4tex2D(tex,pos.xy - dir1);
  half4 rgbyP1 = h4tex2D(tex,pos.xy + dir1);
  #if DEF_LUMA
    #undef DEF_LUMA
    #undef LUMA
  #endif


  half4 rgbyA = (rgbyN1 + rgbyP1) * 0.5;
  //half weight = 0.5*saturate(max(maxDXDY.x, maxDXDY.y)/invSize.x)+0.5;
  half weight = saturate(max(maxDXDY.x, maxDXDY.y)/invSize.x);
  return lerp(rgbyA, centerTap, weight);

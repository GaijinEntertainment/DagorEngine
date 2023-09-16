float whiteTemp = 6500;
float whiteTint = 0;
macro WHITE_BALANCE_PARAMS(code)
  (code) {whiteBlance@f2 = (whiteTemp, whiteTint, 0,0);}
  hlsl(code) {
      #include <tonemapHelpers/whiteBalance.hlsl>
      float3 applyWhiteBalance(float3 linearColor)
      {
        float3 balancedColor = applyWhiteBalance( linearColor, whiteBlance.x, whiteBlance.y );
        return balancedColor;
      }
    }
endmacro


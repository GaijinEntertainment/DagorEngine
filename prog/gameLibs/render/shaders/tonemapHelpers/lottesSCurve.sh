float tonemapContrast = 1.28;
float tonemapShoulder = 0.85;
float tonemapMidIn= 0.06;
float tonemapMidOut = 0.18;
float tonemapHdrMax = 4;

macro LOTTES_TONEMAP_PARAMS(code)
  (code) {
    lottesParams @f4 = (tonemapContrast, tonemapShoulder, tonemapMidIn, tonemapMidOut);
    tonemapHdrMax @f1 = (tonemapHdrMax);
  }
  hlsl(code) {
    #include <tonemapHelpers/lottesSCurve.hlsl>
    float3 applyLottesCurve(float3 gradedColor)
    {
      //todo: use LottesSCurve_optimized with precomputed b,c!
      return LottesSCurve( gradedColor, lottesParams.x, lottesParams.y, lottesParams.z, lottesParams.w, tonemapHdrMax ) ;
      //return LottesSCurve( gradedColor, 1., 0.9, 0.5, 0.18, 4 ) ;
    }
  }
endmacro


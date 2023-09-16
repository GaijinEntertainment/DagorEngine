include "whiteBalance.sh"
include "lottesSCurve.sh"
include "colorGrading.sh"

macro FULL_TONEMAP(code)
  LOTTES_TONEMAP_PARAMS(code)
  COLOR_GRADING_PARAMS(code)
  WHITE_BALANCE_PARAMS(code)

  hlsl(code) {
    float3 applyTonemap(float3 gradedColor) { return applyLottesCurve(gradedColor); }
    //sample
    float3 colorGradedTonemap(float3 linearColor, out float3 gradedColor)
    {
      float3 balancedColor = applyWhiteBalance( linearColor );
      gradedColor = applyColorGrading(balancedColor);
      float3 tonemapedColor = applyTonemap(gradedColor);
      return tonemapedColor;
    }
  }
endmacro

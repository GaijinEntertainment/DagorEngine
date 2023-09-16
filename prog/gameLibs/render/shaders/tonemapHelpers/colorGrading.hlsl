#ifndef COLOR_GRADING_HLSL_INCLUDE
#define COLOR_GRADING_HLSL_INCLUDE 1
//like daVinci and others, works in linear HDR space.
//exact space doesn't matter for anything except luma calculations, that's why we pass luma as parameters

float3 colorCorrect( float3 workingColor,
  float4 colorSaturation,
  float4 colorContrast,
  float4 colorGamma,
  float4 colorGain,
  float4 colorOffset,
  float luma )
{
  workingColor = max( 0, lerp( luma.xxx, workingColor, colorSaturation.xyz*colorSaturation.w ) );
  workingColor = pow( workingColor * (1.0 / 0.18), colorContrast.xyz*colorContrast.w ) * 0.18;
  workingColor = pow( workingColor, 1.0 / (colorGamma.xyz*colorGamma.w) );
  workingColor = workingColor * (colorGain.xyz * colorGain.w) + (colorOffset.xyz + colorOffset.w);
  return workingColor;
}

//float3 sRGB_2_Luma = float3(0.2126729, 0.7151522, 0.0721750);//ACES one
//float3 sRGB_2_Luma = float3(0.2627066, 0.6779996, 0.0592938);//http://www.jacobstrom.com/publications/DCC_Strom_et_al.pdf
//float3 AP1_RGB_2_Luma = float3( 0.2722287168, 0.6740817658, 0.0536895174);//ACES AP1 space
//perceptedLuma = dot(workingColor, *2_Luma)
float3 colorCorrectSMH( float3 workingColor, float perceptedLuma,
  float4 colorSaturation,
  float4 colorContrast,
  float4 colorGamma,
  float4 colorGain,
  float4 colorOffset,

  float4 colorSaturationShadows,
  float4 colorContrastShadows,
  float4 colorGammaShadows,
  float4 colorGainShadows,
  float4 colorOffsetShadows,

  float4 colorSaturationMidtones,
  float4 colorContrastMidtones,
  float4 colorGammaMidtones,
  float4 colorGainMidtones,
  float4 colorOffsetMidtones,

  float4 colorSaturationHighlights,
  float4 colorContrastHighlights,
  float4 colorGammaHighlights,
  float4 colorGainHighlights,
  float4 colorOffsetHighlights,

  float colorCorrectionShadowsMax,
  float colorCorrectionHighlightsMin)
{
  float luma = perceptedLuma;

  float3 gradedColorShadows = colorCorrect(workingColor,
    colorSaturationShadows*colorSaturation,
    colorContrastShadows*colorContrast,
    colorGammaShadows*colorGamma,
    colorGainShadows*colorGain,
    colorOffsetShadows+colorOffset, luma);
  float weightShadows = 1- smoothstep(0, colorCorrectionShadowsMax, luma);

  float3 gradedColorHighlights = colorCorrect(workingColor,
    colorSaturationHighlights*colorSaturation,
    colorContrastHighlights*colorContrast,
    colorGammaHighlights*colorGamma,
    colorGainHighlights*colorGain,
    colorOffsetHighlights+colorOffset, luma);
  float weightHighlights = smoothstep(colorCorrectionHighlightsMin, 1, luma);

  float3 gradedColorMidtones = colorCorrect(workingColor,
    colorSaturationMidtones*colorSaturation,
    colorContrastMidtones*colorContrast,
    colorGammaMidtones*colorGamma,
    colorGainMidtones*colorGain,
    colorOffsetMidtones+colorOffset, luma);
  float weightMidtones = 1 - weightShadows - weightHighlights;

  float3 workingColorSMH = gradedColorShadows*weightShadows + gradedColorMidtones*weightMidtones + gradedColorHighlights*weightHighlights;

  return workingColorSMH;
}

#endif
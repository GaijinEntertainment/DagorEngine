float4 colorSaturation = (1,1,1,1);
float4 colorContrast   = (1,1,1,1);
float4 colorGamma      = (1,1,1,1);
float4 colorGain       = (1,1,1,1);
float4 colorOffset     = (0,0,0,0);

float4 shadows_colorSaturation = (1,1,1,1);
float4 shadows_colorContrast   = (1,1,1,1);
float4 shadows_colorGamma      = (1,1,1,1);
float4 shadows_colorGain       = (1,1,1,1);
float4 shadows_colorOffset     = (0,0,0,0);
float4 midtones_colorSaturation = (1,1,1,1);
float4 midtones_colorContrast   = (1,1,1,1);
float4 midtones_colorGamma      = (1,1,1,1);
float4 midtones_colorGain       = (1,1,1,1);
float4 midtones_colorOffset     = (0,0,0,0);
float4 highlights_colorSaturation = (1,1,1,1);
float4 highlights_colorContrast   = (1,1,1,1);
float4 highlights_colorGamma      = (1,1,1,1);
float4 highlights_colorGain       = (1,1,1,1);
float4 highlights_colorOffset     = (0,0,0,0);

float colorCorrectionShadowsMax = 0.1;
float colorCorrectionHighlightsMin = 0.8;

float4 hueToSaturationKey = (1,0,0,0);
float hueToSaturationKeyFalloff = 0.001;
float4 hueToSaturationValueMul = (1,1,1,1);
float4 hueToSaturationValueAdd = (0,0,0,0);

macro COLOR_GRADING_PARAMS(code)
  (code) {
    colorSaturation @f4  = colorSaturation;
    colorContrast   @f4  = colorContrast;
    colorGamma      @f4  = colorGamma;
    colorGain       @f4  = colorGain;
    colorOffset     @f4  = colorOffset;
    hueToSaturation @f4  = (hueToSaturationKey.r,hueToSaturationKey.g,hueToSaturationKey.b,hueToSaturationKeyFalloff);
    hueToSaturationValueMul @f4 = hueToSaturationValueMul;
    hueToSaturationValueAdd @f4 = hueToSaturationValueAdd;

    shadows_colorSaturation    @f4  = shadows_colorSaturation;
    shadows_colorContrast      @f4  = shadows_colorContrast;
    shadows_colorGamma         @f4  = shadows_colorGamma;
    shadows_colorGain          @f4  = shadows_colorGain;
    shadows_colorOffset        @f4  = shadows_colorOffset;
    midtones_colorSaturation   @f4  = midtones_colorSaturation;
    midtones_colorContrast     @f4  = midtones_colorContrast;
    midtones_colorGamma        @f4  = midtones_colorGamma;
    midtones_colorGain         @f4  = midtones_colorGain;
    midtones_colorOffset       @f4  = midtones_colorOffset;
    highlights_colorSaturation @f4  = highlights_colorSaturation;
    highlights_colorContrast   @f4  = highlights_colorContrast;
    highlights_colorGamma      @f4  = highlights_colorGamma;
    highlights_colorGain       @f4  = highlights_colorGain;
    highlights_colorOffset     @f4  = highlights_colorOffset;
    color_correction_smh  @f2 = (colorCorrectionShadowsMax, colorCorrectionHighlightsMin, 0,0);
  }
  hlsl(code) {
    #include <tonemapHelpers/colorGrading.hlsl>
    float hueFromColor(float3 color)
    {
      //https://en.wikipedia.org/wiki/HSL_and_HSV
      float rgbMax = max3(color.r, color.g, color.b);
      float rgbMaxMin = rgbMax-min3(color.r, color.g, color.b);
      float hue = 0;
      FLATTEN
      if (rgbMax==color.r)
        hue = 60*(color.g-color.b)/rgbMaxMin;
      FLATTEN
      if (rgbMax==color.g)
        hue = 60*(2 + (color.b-color.r)/rgbMaxMin);
      FLATTEN
      if (rgbMax==color.b)
        hue = 60*(4 + (color.r-color.g)/rgbMaxMin);
      if (hue<0)
        hue = hue+360;
      hue /= 360;
      FLATTEN
      if (rgbMax<1e-9)
        hue = 0;
      return hue;
    }

    float3 applyColorGrading(float3 balancedColor)
    {
      float3 sRGB_2_Luma = float3(0.2627066, 0.6779996, 0.0592938);//http://www.jacobstrom.com/publications/DCC_Strom_et_al.pdf
      float luma = dot(balancedColor, sRGB_2_Luma);
      float hueSource = hueFromColor(balancedColor);
      float hueKey = hueFromColor(hueToSaturation.rgb);
      float hueDist = min(abs(hueSource-hueKey), abs(hueSource-(1-hueKey)));
      float hueSaturationEffect = hueToSaturation.a > 0 ? exp2(-pow2(hueDist)/(max(hueToSaturation.a,1e-6))) : 0;

      float3 gradedColor =  colorCorrectSMH(balancedColor,luma,
        max(0, colorSaturation*lerp(float4(1,1,1,1), hueToSaturationValueMul, hueSaturationEffect) + lerp(float4(0,0,0,0),hueToSaturationValueAdd,hueSaturationEffect)),
        colorContrast  ,
        colorGamma     ,
        colorGain      ,
        colorOffset    ,

        shadows_colorSaturation    ,
        shadows_colorContrast      ,
        shadows_colorGamma         ,
        shadows_colorGain          ,
        shadows_colorOffset        ,

        midtones_colorSaturation  ,
        midtones_colorContrast    ,
        midtones_colorGamma       ,
        midtones_colorGain        ,
        midtones_colorOffset      ,

        highlights_colorSaturation,
        highlights_colorContrast  ,
        highlights_colorGamma     ,
        highlights_colorGain      ,
        highlights_colorOffset    ,

        color_correction_smh.x, color_correction_smh.y);

       return gradedColor;
    }
  }
endmacro

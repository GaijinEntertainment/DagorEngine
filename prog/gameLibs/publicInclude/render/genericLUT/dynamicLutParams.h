#pragma once

class Point4;

struct TonemappingParam
{
  float tonemapContrast;
  float tonemapShoulder;
  float tonemapMidIn;
  float tonemapMidOut;
  float tonemapHdrMax;
};

struct ColorGradingParam
{
  Point4 colorSaturation;
  Point4 colorContrast;
  Point4 colorGamma;
  Point4 colorGain;
  Point4 colorOffset;
  Point4 shadows_colorSaturation;
  Point4 shadows_colorContrast;
  Point4 shadows_colorGamma;
  Point4 shadows_colorGain;
  Point4 shadows_colorOffset;
  Point4 midtones_colorSaturation;
  Point4 midtones_colorContrast;
  Point4 midtones_colorGamma;
  Point4 midtones_colorGain;
  Point4 midtones_colorOffset;
  Point4 highlights_colorSaturation;
  Point4 highlights_colorContrast;
  Point4 highlights_colorGamma;
  Point4 highlights_colorGain;
  Point4 highlights_colorOffset;
  Point4 hueToSaturationKey;
  Point4 hueToSaturationValueMul;
  Point4 hueToSaturationValueAdd;
  float colorCorrectionHighlightsMin;
  float colorCorrectionShadowsMax;
  float hueToSaturationKeyFalloff;
  float baseWhiteTemp;
};

struct DynamicLutParams
{
  TonemappingParam tonemappingParams;
  ColorGradingParam colorGradingParams;
  float at;
};

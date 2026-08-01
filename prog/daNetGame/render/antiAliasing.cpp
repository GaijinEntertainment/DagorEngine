// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "antiAliasing_legacy.h"

#include <math/random/dag_halton.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_driver.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_parseResolution.h>
#include <ioSys/dag_dataBlock.h>

AntiAliasing::AntiAliasing(const IPoint2 &inputResolution, const IPoint2 &outputResolution) :
  inputResolution(inputResolution), outputResolution(outputResolution), lodBias(-log2(getUpsamplingRatio().y))
{}

Point2 AntiAliasing::getUpsamplingRatio() const
{
  return Point2(float(outputResolution.x) / float(inputResolution.x), float(outputResolution.y) / float(inputResolution.y));
}

void AntiAliasing::setInputResolution(const IPoint2 &input_res)
{
  lodBias = -log2(getUpsamplingRatio().y);
  inputResolution = input_res;
}

IPoint2 AntiAliasing::computeInputResolution(const IPoint2 &outputResolution)
{
  const char *resStr = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("maxRenderingResolution", nullptr);
  IPoint2 maxRenderingResolution;
  float upsamplingRatio = 100.0f;
  if (resStr && get_resolution_from_str(resStr, maxRenderingResolution.x, maxRenderingResolution.y))
  {
    float maxWidthRatio = min(1.f, float(maxRenderingResolution.x) / outputResolution.x);
    float maxHeightRatio = min(1.f, float(maxRenderingResolution.y) / outputResolution.y);
    upsamplingRatio = 100.0f * min(maxWidthRatio, maxHeightRatio);
  }
  else
    upsamplingRatio = ::dgs_get_settings()->getBlockByNameEx("video")->getReal("temporalUpsamplingRatio", 100.0f);

  IPoint2 inputResolution(round(outputResolution.x * (upsamplingRatio / 100.0f)),
    round(outputResolution.y * (upsamplingRatio / 100.0f)));
  inputResolution.x &= ~1;
  inputResolution.y &= ~1;

  if (inputResolution.x < 32 || inputResolution.y < 32)
  {
    logerr("specified temporalUpsamplingRatio %f can't be used at resolution %ix%i", upsamplingRatio, outputResolution.x,
      outputResolution.y);
    return outputResolution;
  }

  return inputResolution;
}

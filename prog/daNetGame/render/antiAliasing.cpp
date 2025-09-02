// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "antiAliasing.h"

#include <math/random/dag_halton.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_driver.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_parseResolution.h>
#include <ioSys/dag_dataBlock.h>

CONSOLE_INT_VAL("render", halton_sequence_length, 8, 1, 1024);
CONSOLE_FLOAT_VAL_MINMAX("render", jitter_scale, 1.f, 0.f, 100.f);

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

int AntiAliasing::getTemporalFrameCount() const
{
  Point2 upsamplingRatio = getUpsamplingRatio();
  return halton_sequence_length.get() * upsamplingRatio.x * upsamplingRatio.y;
}

Point2 AntiAliasing::update(Driver3dPerspective &perspective)
{
  // To keep sample coverage of each pixel uniform the number of samples
  // have to be proportional to the area covered by each pixel.

  int sequenceLength = getTemporalFrameCount();
  int phase = frameCounter % sequenceLength + 1;
  jitterOffset = Point2(halton_sequence(phase, 2) - 0.5f, halton_sequence(phase, 3) - 0.5f) * jitter_scale.get();

  Point2 result = Point2(jitterOffset.x * (2.0f / inputResolution.x), -jitterOffset.y * (2.0f / inputResolution.y));

  perspective.ox += result.x;
  perspective.oy += result.y;

  frameCounter++;

  return result;
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

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER
#include "private_worldRenderer.h"
#include <perfMon/dag_statDrv.h>
#include <render/deferredRenderer.h>

void WorldRenderer::applyFXAASettings()
{
  auto gfxBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  const char *qualityStr = gfxBlk->getStr("fxaaQuality", "medium");

  int fxaaQualityVal;
  if (strcmp(qualityStr, "low") == 0)
    fxaaQualityVal = 1;
  else if (strcmp(qualityStr, "medium") == 0)
    fxaaQualityVal = 12;
  else if (strcmp(qualityStr, "high") == 0)
    fxaaQualityVal = 30;
  else
    fxaaQualityVal = 12;
  ShaderGlobal::set_int(get_shader_variable_id("fxaa_quality_lev"), fxaaQualityVal);
}

bool WorldRenderer::isFXAAEnabled() { return currentAntiAliasingMode == AntiAliasingMode::FXAA; }
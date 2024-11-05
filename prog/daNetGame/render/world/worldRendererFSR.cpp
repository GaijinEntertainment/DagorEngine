// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER
#include "private_worldRenderer.h"
#include <render/hdrRender.h>
#include <perfMon/dag_statDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_computeShaders.h>

#ifdef _MSC_VER
#pragma warning(disable : 4505) // unreferenced local function has been removed for FSR includes
#endif

#define A_CPU
#include <fsr/ffx_a.h>
#include <fsr/ffx_fsr1.h>

void WorldRenderer::loadFsrSettings()
{
  fsrEnabled = false;
  const char *quality = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("fsr", nullptr);
  if (quality)
  {
    fsrEnabled = true;
    if (!strcmp(quality, "off"))
    {
      fsrEnabled = false;
    }
    else if (!strcmp(quality, "ultraquality"))
    {
      fsrScale = 1.3f;
      fsrMipBias = -0.3f;
    }
    else if (!strcmp(quality, "quality"))
    {
      fsrScale = 1.5f;
      fsrMipBias = -0.5f;
    }
    else if (!strcmp(quality, "balanced"))
    {
      fsrScale = 1.7f;
      fsrMipBias = -0.7f;
    }
    else if (!strcmp(quality, "performance"))
    {
      fsrScale = 2.0f;
      fsrMipBias = -1.0f;
    }
    else
    {
      G_ASSERTF(0, "Incorrect quality value for FSR");
    }
  }
}

IPoint2 WorldRenderer::getFsrScaledResolution(const IPoint2 &orig_resolution) const
{
  return IPoint2(round(float(orig_resolution.x) / fsrScale), round(float(orig_resolution.y) / fsrScale));
}

void WorldRenderer::closeFsr()
{
  superResolutionUpscalingConsts.close();
  superResolutionSharpeningConsts.close();
}

void WorldRenderer::initFsr(const IPoint2 &postfx_resolution, const IPoint2 &display_resolution)
{
  superResolutionUpscalingConsts = dag::buffers::create_persistent_cb(5, "superResolutionUpscalingConsts");
  superResolutionSharpeningConsts = dag::buffers::create_persistent_cb(5, "superResolutionSharpeningConsts");
  struct FSRConstants
  {
    uint32_t Const0[4];
    uint32_t Const1[4];
    uint32_t Const2[4];
    uint32_t Const3[4];
    uint32_t Sample[4];
  };
  {
    FSRConstants consts = {};
    FsrEasuCon(reinterpret_cast<AU1 *>(consts.Const0), reinterpret_cast<AU1 *>(consts.Const1), reinterpret_cast<AU1 *>(consts.Const2),
      reinterpret_cast<AU1 *>(consts.Const3), (AF1)postfx_resolution.x, (AF1)postfx_resolution.y, (AF1)postfx_resolution.x,
      (AF1)postfx_resolution.y, (AF1)display_resolution.x, (AF1)display_resolution.y);
    superResolutionUpscalingConsts->updateDataWithLock(0, sizeof(FSRConstants), &consts, VBLOCK_DISCARD);
  }
  {
    FSRConstants consts = {};
    FsrRcasCon(reinterpret_cast<AU1 *>(consts.Const0), 0.25f);
    consts.Sample[0] = (hdrrender::is_hdr_enabled() ? 1 : 0);
    superResolutionSharpeningConsts->updateDataWithLock(0, sizeof(FSRConstants), &consts, VBLOCK_DISCARD);
  }
}

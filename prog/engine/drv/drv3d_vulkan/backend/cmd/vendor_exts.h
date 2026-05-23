// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <3d/dag_nvFeatures.h>
#include "fsr_args.h"

namespace drv3d_vulkan
{

struct CmdExecuteFSR
{
  FSRUpscalingArgs params;
};

struct CmdExecuteDLSS
{
  nv::DlssParams<Image> params;
};

struct CmdInitializeDLSS
{
  int mode;
  int width;
  int height;
  bool use_rr;
  bool use_legacy_model;
};

struct CmdInitializeStreamlineDLSS
{
  int width;
  int height;
};

struct CmdReleaseDLSS
{};

struct CmdReleaseStreamlineDLSS
{
  bool stereo_render;
};

struct CmdExecuteStreamlineDLSS
{
  nv::DlssParams<Image> dlssParams;
  int viewIndex;
};

struct CmdExecuteStreamlineDLSSG
{
  nv::DlssGParams<Image> dlssGParams;
  int viewIndex;
};

struct CmdExecuteXESS
{
  Image *inColor;
  Image *inDepth;
  Image *inMotionVectors;
  float inJitterOffsetX;
  float inJitterOffsetY;
  uint32_t inInputWidth;
  uint32_t inInputHeight;
  uint32_t inColorDepthOffsetX;
  uint32_t inColorDepthOffsetY;
  bool inReset;

  Image *outColor;
};

} // namespace drv3d_vulkan

// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <3d/dag_nvFeatures.h>
#include "fsr_args.h"

namespace drv3d_vulkan
{

struct CmdExecuteFSR
{
  amd::FSR *fsr;
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

} // namespace drv3d_vulkan

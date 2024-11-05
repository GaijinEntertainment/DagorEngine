//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_simpleString.h>
#include <daScript/daScript.h>
#include <util/dag_string.h>
#include <dasModules/dasDataBlock.h>

struct VideoInfo
{
  das::string desktopResolution;
  das::string videoCard;
  das::string videoVendor;
};

struct CpuInfo
{
  das::string cpu;
  das::string cpuFreq;
  das::string cpuVendor;
  das::string cpuSeriesCores;
  int numCores;
};

namespace bind_dascript
{
void get_video_info(const das::TBlock<void, das::TTemporary<const VideoInfo>> &block, das::Context *context, das::LineInfoArg *at);

void get_cpu_info(const das::TBlock<void, das::TTemporary<const CpuInfo>> &block, das::Context *context, das::LineInfoArg *at);
} // namespace bind_dascript

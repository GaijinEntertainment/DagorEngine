// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_amdFsr.h>
#include <osApiWrappers/dag_dynLib.h>
#include <util/dag_string.h>
#include <stdint.h>

struct ffxApiHeader;
struct ffxAllocationCallbacks;

using ffxContext = void *;
using ffxReturnCode_t = uint32_t;
using ffxCreateContextDescHeader = ffxApiHeader;
using ffxConfigureDescHeader = ffxApiHeader;
using ffxQueryDescHeader = ffxApiHeader;
using ffxDispatchDescHeader = ffxApiHeader;

using PfnFfxCreateContext = ffxReturnCode_t (*)(ffxContext *context, ffxCreateContextDescHeader *desc,
  const ffxAllocationCallbacks *memCb);
using PfnFfxDestroyContext = ffxReturnCode_t (*)(ffxContext *context, const ffxAllocationCallbacks *memCb);
using PfnFfxConfigure = ffxReturnCode_t (*)(ffxContext *context, const ffxConfigureDescHeader *desc);
using PfnFfxQuery = ffxReturnCode_t (*)(ffxContext *context, ffxQueryDescHeader *desc);
using PfnFfxDispatch = ffxReturnCode_t (*)(ffxContext *context, const ffxDispatchDescHeader *desc);

namespace amd
{

class FSRVulkan
{
public:
  FSRVulkan();
  ~FSRVulkan();

  static FSRVulkan &getInstance();
  static FSRVulkan *getExistingInstance();

  bool initUpscaling(const FSR::ContextArgs &args);
  void teardownUpscaling();

  Point2 getNextJitter(uint32_t render_width, uint32_t output_width);
  bool doApplyUpscaling(const FSR::UpscalingPlatformArgs &args, void *command_list) const;

  IPoint2 getRenderingResolution(FSR::UpscalingMode mode, const IPoint2 &target_resolution) const;
  FSR::UpscalingMode getUpscalingMode() const;

  bool isLoaded() const;
  bool isUpscalingSupported() const;
  String getVersionString() const;

  void beforeReset();
  void afterReset();

private:
  void loadLib();
  bool isLoadedImpl() const;

  DagorDynLibHolder fsrModule;

  PfnFfxCreateContext createContext = nullptr;
  PfnFfxDestroyContext destroyContext = nullptr;
  PfnFfxConfigure configure = nullptr;
  PfnFfxQuery query = nullptr;
  PfnFfxDispatch dispatch = nullptr;

  ffxContext upscalingContext = nullptr;

  FSR::ContextArgs contextArgs = {};

  int32_t jitterIndex = 0;
  String upscalingVersionString = String(8, "");
};

} // namespace amd

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amdFsr.h"

namespace amd
{

namespace
{
FSRVulkan *fsr_vulkan_instance = nullptr;
}

FSRVulkan &FSRVulkan::getInstance()
{
  static FSRVulkan instance;
  fsr_vulkan_instance = &instance;
  return instance;
}

FSRVulkan *FSRVulkan::getExistingInstance() { return fsr_vulkan_instance; }

FSRVulkan::FSRVulkan() = default;

FSRVulkan::~FSRVulkan()
{
  if (fsr_vulkan_instance == this)
    fsr_vulkan_instance = nullptr;
}

void FSRVulkan::loadLib() {}

bool FSRVulkan::isLoadedImpl() const { return false; }

bool FSRVulkan::initUpscaling(const FSR::ContextArgs &) { return false; }

void FSRVulkan::teardownUpscaling() {}

Point2 FSRVulkan::getNextJitter(uint32_t, uint32_t) { return Point2::ZERO; }

bool FSRVulkan::doApplyUpscaling(const FSR::UpscalingPlatformArgs &, void *) const { return false; }

IPoint2 FSRVulkan::getRenderingResolution(FSR::UpscalingMode, const IPoint2 &) const { return IPoint2::ZERO; }

FSR::UpscalingMode FSRVulkan::getUpscalingMode() const { return FSR::UpscalingMode::Off; }

bool FSRVulkan::isLoaded() const { return false; }

bool FSRVulkan::isUpscalingSupported() const { return false; }

String FSRVulkan::getVersionString() const { return String{}; }

void FSRVulkan::beforeReset() {}

void FSRVulkan::afterReset() {}

} // namespace amd

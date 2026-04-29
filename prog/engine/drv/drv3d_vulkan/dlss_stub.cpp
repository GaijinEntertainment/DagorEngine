// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dlss.h"

namespace drv3d_vulkan
{

DLSSSuperResolutionDirect::DLSSSuperResolutionDirect() {}
DLSSSuperResolutionDirect::~DLSSSuperResolutionDirect() {}

bool DLSSSuperResolutionDirect::Initialize(VkInstance, VkPhysicalDevice, VkDevice) { return false; }

void DLSSSuperResolutionDirect::Teardown(VkDevice) {}

bool DLSSSuperResolutionDirect::evaluate(const nv::DlssParams<void> &, void *) { return false; }

eastl::optional<nv::DLSS::OptimalSettings> DLSSSuperResolutionDirect::getOptimalSettings(Mode, IPoint2) const
{
  return eastl::nullopt;
}

bool DLSSSuperResolutionDirect::setOptions(Mode, IPoint2, bool, bool) { return false; }

void DLSSSuperResolutionDirect::DeleteFeature() {}

nv::DLSS::State DLSSSuperResolutionDirect::getState() { return nv::DLSS::State::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE; }

bool DLSSSuperResolutionDirect::supportRayReconstruction() { return false; }

dag::Expected<eastl::string, nv::SupportState> DLSSSuperResolutionDirect::getVersion() const
{
  return dag::Unexpected(nv::SupportState::NotSupported);
}

bool DLSSSuperResolutionDirect::setOptionsBackend(VkDevice, VkCommandBuffer, Mode, IPoint2, bool, bool) { return false; }

const eastl::vector<eastl::string> &DLSSSuperResolutionDirect::getRequiredDeviceExtensions(VkInstance, VkPhysicalDevice) const
{
  return requiredDeviceExtensions;
}

const eastl::vector<eastl::string> &DLSSSuperResolutionDirect::getRequiredInstanceExtensions() const
{
  return requiredInstanceExtensions;
}

} // namespace drv3d_vulkan

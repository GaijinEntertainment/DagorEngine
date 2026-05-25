// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "xess.h"

namespace drv3d_vulkan
{

eastl::string Xess::errorKindToString(ErrorKind) { return {}; }

void Xess::load() {}

void Xess::unload() {}

bool Xess::init() { return false; }

bool Xess::createFeature(int, uint32_t, uint32_t) { return false; }

void Xess::failDeviceExt() {}

void Xess::failInstanceExt() {}

bool Xess::shutdown() { return false; }

XessState Xess::getState() const { return XessState::DISABLED; }

void Xess::getRenderResolution(int &, int &, int &, int &, int &, int &) const {}

bool Xess::evaluate(const CmdExecuteXESS &) { return false; }

void Xess::setVelocityScale(float, float) {}

bool Xess::isQualityAvailableAtResolution(uint32_t, uint32_t, int) const { return false; }

dag::Expected<eastl::string, Xess::ErrorKind> Xess::getVersion() const { return dag::Unexpected(ErrorKind::Unknown); }

Xess::Xess() = default;

Xess::~Xess() = default;

void Xess::startDump(const char *, uint32_t) {}

bool Xess::isFrameGenerationSupported() const { return false; }

bool Xess::isFrameGenerationEnabled() const { return false; }

void Xess::enableFrameGeneration(bool) {}

void Xess::suppressFrameGeneration(bool) {}

void Xess::doScheduleGeneratedFrames(const XessFgParamsVulkan &, const XessFgParamsVulkanResourceStates &) {}

int Xess::getPresentedFrameCount() { return 1; }

uint64_t Xess::getMemoryUsage() const { return 0; }

const eastl::vector<eastl::string> &Xess::getRequiredDeviceExtensions() const { return requiredDeviceExtensions; }

const eastl::vector<eastl::string> &Xess::getRequiredInstanceExtensions() const { return requiredInstanceExtensions; }

void Xess::injectRequiredFeatures(VkPhysicalDeviceFeatures2KHR &) const {}

void Xess::patchApiVersion(uint32_t &) {}

} // namespace drv3d_vulkan

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/dag_vr.h>
#include "shaders/dag_postFxRenderer.h"

void VRDevice::setAndCallInputInitializationCallback(HumanInput::VrInput::InitializationCallback) {}

void VRDevice::create(RenderingAPI, const ApplicationData &) {}

VRDevice *VRDevice::getInstance() { return nullptr; }

VRDevice *VRDevice::getInstanceIfActive() { return nullptr; }

void VRDevice::deleteInstance() {}

bool VRDevice::hasActiveSession() { return false; }

VRDevice::VRDevice() {}
VRDevice::~VRDevice() {}

void VRDevice::setMirrorMode(MirrorMode) {}

void VRDevice::renderMirror(FrameData &) {}

void VRDevice::forceDisableScreenMask(bool) {}

bool VRDevice::renderScreenMask(const TMatrix4 &, int, float, int) { return false; }

bool VRDevice::shouldBeEnabled() { return false; }

void VRDevice::applyAllFromSettings() {}

VRDevice::MirrorMode VRDevice::getMirrorModeFromSettings() { return MirrorMode::Both; }

void VRDevice::setEnabled(bool) {}

bool VRDevice::setRenderingDevice() { return false; }

void VRDevice::prepareVrsMask(FrameData &) {}

void VRDevice::enableVrsMask(StereoIndex, bool) {}

void VRDevice::disableVrsMask() {}

const ManagedTex *VRDevice::getVrsMask(StereoIndex stereo_index, bool combined) const { return nullptr; }

// static
void VRDevice::calcViewTransforms(VRDevice::FrameData::ViewData &, float, float, float) {}

// static
float VRDevice::calcBoundingView(VRDevice::FrameData &) { return 1.f; }

// static
int VRDevice::getConfiguredRefreshRate() { return 0; }
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/volumetricLights/volumetricLights.h>

#include <shaders/dag_computeShaders.h>
#include <render/nodeBasedShader.h>
#include <math/dag_frustum.h>


bool VolumeLight::IS_SUPPORTED = false;

VolumeLight::VolfogMediaInjectionGuard::VolfogMediaInjectionGuard(BaseTexture *) : tex(nullptr) {}
VolumeLight::VolfogMediaInjectionGuard::~VolfogMediaInjectionGuard() {}

VolumeLight::VolumeLight() {}
VolumeLight::~VolumeLight() {}
void VolumeLight::close() {}
void VolumeLight::init() {}
void VolumeLight::setResolution(int, int, int, int, int) {}
void VolumeLight::setRange(float) {}
void VolumeLight::setCurrentView(int) {}
bool VolumeLight::perform(const TMatrix4 &, const TMatrix4 &, const TMatrix4_vec4 &, const Point3 &) { return false; }
void VolumeLight::performIntegration() {}
void VolumeLight::switchOff() {}
void VolumeLight::switchOn() {}
void VolumeLight::closeShaders() {}
void VolumeLight::beforeReset() {}
void VolumeLight::afterReset() {}
void VolumeLight::invalidate() {}

bool VolumeLight::updateShaders(const String &, const DataBlock &, String &) { return false; }
void VolumeLight::initShaders(const DataBlock &) {}
void VolumeLight::enableOptionalShader(const String &, bool) {}

VolumeLight::VolfogMediaInjectionGuard VolumeLight::StartVolfogMediaInjection() { return VolfogMediaInjectionGuard(nullptr); }

void VolumeLight::onSettingsChange(VolfogQuality, VolfogShadowCasting, DistantFogQuality) {}

Frustum VolumeLight::calcFrustum(const TMatrix4 &, const Driver3dPerspective &) const { return Frustum(); }

bool VolumeLight::isDistantFogEnabled() const { return false; }

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/volumetricLights/volumetricLights.h>

#include <shaders/dag_computeShaders.h>
#include <render/nodeBasedShader.h>
#include <math/dag_frustum.h>


bool VolumeLight::IS_SUPPORTED = false;

VolumeLight::VolumeLight() {}
VolumeLight::~VolumeLight() {}
void VolumeLight::close() {}
void VolumeLight::init() {}
void VolumeLight::setResolution(int, int, int, int, int) {}
void VolumeLight::setRange(float) {}
void VolumeLight::setCurrentView(int) {}
void VolumeLight::switchOff() {}
void VolumeLight::switchOn() {}
void VolumeLight::closeShaders() {}
void VolumeLight::beforeReset() {}
void VolumeLight::afterReset() {}
void VolumeLight::invalidate() {}

bool VolumeLight::performStartFrame(const TMatrix4 &view_tm, const TMatrix4 &proj_tm, const TMatrix4_vec4 &glob_tm,
  const Point3 &camera_pos)
{
  return false;
}
void VolumeLight::performFroxelFogOcclusion() {}
void VolumeLight::performFroxelFogFillMedia() {}
void VolumeLight::performVolfogShadow() {}
void VolumeLight::performDistantFogRaymarch() {}
void VolumeLight::performFroxelFogPropagate() {}
void VolumeLight::performDistantFogReconstruct() {}

void VolumeLight::volfogMediaInjectionStart(uint32_t shader_stage) {}
void VolumeLight::volfogMediaInjectionEnd(uint32_t shader_stage) {}

bool VolumeLight::updateShaders(const String &, const DataBlock &, String &) { return false; }
void VolumeLight::initShaders(const DataBlock &) {}
void VolumeLight::enableOptionalShader(const String &, bool) {}

void VolumeLight::onSettingsChange(VolfogQuality, VolfogShadowCasting, DistantFogQuality) {}

Frustum VolumeLight::calcFrustum(const TMatrix4 &, const Driver3dPerspective &) const { return Frustum(); }

bool VolumeLight::isDistantFogEnabled() const { return false; }

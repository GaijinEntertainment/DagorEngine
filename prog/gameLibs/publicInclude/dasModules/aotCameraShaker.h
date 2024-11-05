//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcs.h>
#include <camera/cameraShaker.h>

MAKE_TYPE_FACTORY(CameraShaker, CameraShaker)

namespace bind_dascript
{
inline void camera_shaker_update(CameraShaker &camera_shaker, float dt) { camera_shaker.update(dt); }

inline void camera_shaker_shakeMatrix(CameraShaker &camera_shaker, TMatrix &transform) { camera_shaker.shakeMatrix(transform); }

inline void camera_shaker_setShake(CameraShaker &camera_shaker, float wishShake) { camera_shaker.setShake(wishShake); }

inline void camera_shaker_setShakeHiFreq(CameraShaker &camera_shaker, float wishShake) { camera_shaker.setShakeHiFreq(wishShake); }

inline void camera_shaker_setSmoothShakeHiFreq(CameraShaker &camera_shaker, float wishShake)
{
  camera_shaker.setSmoothShakeHiFreq(wishShake);
}
} // namespace bind_dascript

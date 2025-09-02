//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <effectManager/effectManager.h>
#include "dafxCompound_decl.h"

struct EffectManager::SoundEffect
{
  AcesEffect::FxId fxId;
  bool loopSound = false;
  bool paused = false;
  float delay = 0;
  float intensity = 0;
  Point3 velocity = Point3(0, 0, 0);
  Point3 pos;
  SoundExEvent *sfx = nullptr;

  SoundEffect() = delete;
  SoundEffect(AcesEffect::FxId fx_id, const Point3 &eff_pos) : fxId(fx_id), pos(eff_pos) {}
};

struct EffectManager::LightEffect
{
  AcesEffect::FxId fxId;
  int extLightId = -1;
  BaseEffectObject *obj;
  float radiusMultiplier = 1.0f;
  Point3 pos = {0, 0, 0};
  Point4 params = {0, 0, 0, 0};
  float intensity = 1.0f;
  TMatrix box = TMatrix::ZERO;
  const TMatrix *getBox() const { return box.getcol(0).lengthSq() > 0 ? &box : nullptr; }
  LightfxShadowParams shadowParams = {};

  ShadowEnabled shadowEnabled = ShadowEnabled::NOT_INITED;
  void activateShadowExt() const;
  void releaseShadowExt() const;

  LightEffect() = delete;
  LightEffect(AcesEffect::FxId fx_id, BaseEffectObject *base_obj) : fxId(fx_id), obj(base_obj) {}
};

struct EffectManager::PendingData
{
  AcesEffect::FxId fxId;
  bool isEmitterTm = true;
  bool unsetEmitter = false;
  bool velocitySet = false;
  bool velocityScaleMinMaxSet = false;
  float spawnRate = 1.0f;
  Color4 colorMult = Color4(1, 1, 1, 1);
  Point3 velocity = Point3(0, 0, 0);
  Point2 velocityScaleMinMax = Point2(1, 1);
  float lightRadiusMultiplier = 1.0f;
  float lightIntensity = 1.0f;
  float windScale = -1.0f;
  Matrix3 gravityTm = Matrix3::IDENT;
  TMatrix lightBox = TMatrix::ZERO;
  Point3 fakeBrightnessBackgroundPos = Point3(0, 0, 0);

  PendingData() = delete;
  explicit PendingData(AcesEffect::FxId fx_id) : fxId(fx_id) {}
};
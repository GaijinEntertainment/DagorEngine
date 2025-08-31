// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "effectManager/effectManager.h"

AcesEffect::~AcesEffect() {}
void AcesEffect::setFxTm(const TMatrix &) {}
void AcesEffect::setEmitterTm(const TMatrix &) {}
void AcesEffect::setFakeBrightnessBackgroundPos(const Point3 &) {}
void AcesEffect::setVelocity(const Point3 &) {}
void AcesEffect::setVelocityScaleMinMax(const Point2 &) {}
void AcesEffect::setSpawnRate(float) {}
void AcesEffect::setLightRadiusMultiplier(float) {}
void AcesEffect::setColorMult(const Color4 &) {}
void AcesEffect::setVisibility(uint32_t) {}
void AcesEffect::hide(bool) {}
void AcesEffect::unlock() {}
void AcesEffect::unsetEmitter() {}
bool AcesEffect::isAlive() const { return true; }
bool AcesEffect::isActive() const { return false; }
void AcesEffect::enableActiveQuery() {}
const char *AcesEffect::getName() const { return ""; }
float AcesEffect::soundEnableDistanceSq() const { return 0; }
const Point3 &AcesEffect::getWorldPos() const { return ZERO<Point3>(); }
float AcesEffect::lifeTime() const { return 0; }
void AcesEffect::pauseSound(bool) {}
bool AcesEffect::hasSound() const { return false; }
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <billboardDecals/billboardDecals.h>
#include "render/fx/fx.h"
#include <effectManager/effectManager.h>

int acesfx::get_type_by_name(const char *) { return -1; }
FxQuality acesfx::get_fx_target() { G_ASSERT_RETURN(false, FX_QUALITY_LOW); }
float acesfx::get_effect_life_time(int) { G_ASSERT_RETURN(false, 0.0f); }
bool acesfx::prefetch_effect(int) { G_ASSERT_RETURN(false, false); }
const char *acesfx::get_name_by_type(int) { return nullptr; }

void acesfx::start_effect(int, const TMatrix &, const TMatrix &, bool, float, AcesEffect **, FxErrorType *) {}
void acesfx::start_effect_pos(int, const Point3 &, bool, float, AcesEffect **) {}
void acesfx::start_effect_pos_norm(int, const Point3 &, const Point3 &, bool, float, AcesEffect **) {}

void acesfx::stop_effect(AcesEffect *&fx) { fx = NULL; }
void acesfx::reset() {}
void acesfx::setWindParams(float, Point2) {}

bool acesfx::thermal_vision_on() { G_ASSERT_RETURN(false, false); }

void acesfx::wait_fx_managers_update_and_allow_accum_cmds() {}

// effectManager.cpp
AcesEffect::~AcesEffect() {}
void AcesEffect::setFxScale(float) {}
void AcesEffect::setFxTm(const TMatrix &) {}
void AcesEffect::setEmitterTm(const TMatrix &) {}
void AcesEffect::setColorMult(struct Color4 const &) {}
void AcesEffect::unlock() {}

void ri_destr_start_effect(int, const TMatrix &, const TMatrix &, int, bool, AcesEffect **, const char *) {}

struct Color4 __cdecl get_blood_color(void) { return ZERO<Color4>(); }

bool is_blood_enabled() { return false; }

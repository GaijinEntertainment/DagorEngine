// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_assert.h>

#include <blood_puddles/public/render/bloodPuddles.h>

BloodPuddles *get_blood_puddles_mgr() { G_ASSERT_RETURN(false, nullptr); }
void init_blood_puddles_mgr() { G_ASSERT(0); }
void close_blood_puddles_mgr() { G_ASSERT(0); }
void add_hit_blood_effect(const Point3 &, const Point3 &) { G_ASSERT(0); }
void create_blood_puddle_emitter(const ecs::EntityId, const int) { G_ASSERT(0); }
void DecalsMatrices::clearItems() { G_ASSERT(0); }
void BloodPuddles::update() { G_ASSERT(0); }
void BloodPuddles::beforeRender() { G_ASSERT(0); }
void BloodPuddles::addSplashEmitter(
  const Point3 &, const Point3 &, const Point3 &, const Point3 &, float, rendinst::riex_handle_t, const Point3 &)
{
  G_ASSERT(0);
}
void BloodPuddles::putDecal(int, const Point3 &, const Point3 &, float, rendinst::riex_handle_t, const Point3 &, bool) { G_ASSERT(0); }
void BloodPuddles::putDecal(
  int, const Point3 &, const Point3 &, const Point3 &, float, rendinst::riex_handle_t, const Point3 &, bool, int, float)
{
  G_ASSERT(0);
}

bool BloodPuddles::tryPlacePuddle(PuddleCtx &) { G_ASSERT_RETURN(false, false); }

void BloodPuddles::addPuddleAt(const PuddleCtx &, int) { G_ASSERT(0); }

void BloodPuddles::addFootprint(Point3, const Point3 &, const Point3 &, float, bool, int) { G_ASSERT(0); }

void BloodPuddles::addSplash(const Point3 &, const Point3 &, const TMatrix &, int, float) { G_ASSERT(0); }

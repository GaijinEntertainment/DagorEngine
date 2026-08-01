//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <math/dag_bounds3.h>
#include <generic/dag_span.h>

// forward declarations for external classes
struct RoDataBlock;
struct ObjectsToPlace;
struct Frustum;
class BaseEffectObject;
class Occlusion;

struct Effect
{
  BaseEffectObject *fx;
#if DAGOR_DBGLEVEL > 0
  static constexpr int NAME_LEN = 39;
  char name[NAME_LEN + 1];
#endif
  BSphere3 sph;
  bool visible;
  bool updateWhenInvisible;
  int bindumpIndex;
  bool bindumpRenderable;
};


namespace StaticFxObjects
{
void init(const RoDataBlock &blk, unsigned bindump_id = -1, bool renderable = true);
void init(const ObjectsToPlace &o, unsigned bindump_id = -1, bool renderable = true);

void on_bindump_unload(unsigned bindump_id);
void on_bindump_renderable_change(unsigned bindump_id, bool renderable);

void clear();

// culls objects[] by the frustum/occlusion of the view being prepared and issues FX_RENDER_BEFORE
void before_render(const Frustum &culling_frustum, const TMatrix &view_itm, const Occlusion *occlusion);
// renders objects marked visible by the last before_render(); render_type is FX_RENDER_SOLID/TRANS/...
void render(int render_type, const TMatrix &view_itm);
void update(real dt);

void on_device_reset();

dag::ConstSpan<Effect> get_effects_list();
}; // namespace StaticFxObjects

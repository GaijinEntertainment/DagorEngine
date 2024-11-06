//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <phys/dag_physDecl.h>
#include <phys/dag_physObject.h>
#include <render/dynmodelRenderer.h>

class DynamicRenderableSceneInstance;

namespace gamephys
{
class DestructableObject;
}
namespace destructables
{
struct DestrRendData
{
  struct RendData
  {
    DynamicRenderableSceneInstance *inst;
    dynrend::InitialNodes *initialNodes;
  };
  SmallTab<RendData, MidmemAlloc> rendData;
  int deformationId;
};

DestrRendData *init_rend_data(DynamicPhysObjectClass<PhysWorld> *phys_obj);
void clear_rend_data(DestrRendData *data);

struct DestrRendDataDeleter
{
  void operator()(DestrRendData *rd);
};

void before_render(const Point3 &view_pos, bool has_motion_vectors);
// Objects with a bounding box radius < min_bbox_radius will be skipped.
void render(dynrend::ContextId inst_ctx, const Frustum &frustum, float min_bbox_radius);

typedef int (*deform_create_instance_cb_type)(const DestrRendData *src, bool fully_deformed);
typedef void (*deform_destroy_instance_cb_type)(const DestrRendData *src);
typedef void (*deform_before_render_cb_type)(dag::ConstSpan<gamephys::DestructableObject *> list);
typedef bool (*deform_per_draw_cb_type)(int deformation_id, Point4 &v);

void set_visual_deform_cb(deform_create_instance_cb_type create_cb, deform_destroy_instance_cb_type destroy_cb,
  deform_before_render_cb_type before_render_cb, deform_per_draw_cb_type per_draw_cb);
} // namespace destructables

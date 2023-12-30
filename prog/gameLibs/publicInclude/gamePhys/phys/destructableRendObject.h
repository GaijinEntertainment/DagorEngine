//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <phys/dag_physDecl.h>
#include <phys/dag_physObject.h>
#include <render/dynmodelRenderer.h>

class DynamicRenderableSceneInstance;

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
};

DestrRendData *init_rend_data(DynamicPhysObjectClass<PhysWorld> *phys_obj);
void clear_rend_data(DestrRendData *data);

struct DestrRendDataDeleter
{
  void operator()(DestrRendData *rd) { rd ? clear_rend_data(rd) : (void)0; }
};

void before_render(const Point3 &view_pos, bool has_motion_vectors);
// Objects with a bounding box radius < min_bbox_radius will be skipped.
void render(dynrend::ContextId inst_ctx, const Frustum &frustum, float min_bbox_radius);
} // namespace destructables

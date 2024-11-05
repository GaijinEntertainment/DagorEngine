// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/physDebugDraw.h>
#include <gameMath/traceUtils.h>
#include <debug/dag_textMarks.h>
#include <util/dag_string.h>
#include <math/dag_Point4.h>
#include <math/dag_mathUtils.h>
#include <gameRes/dag_collisionResource.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstAccess.h>


void gamephys::draw_trace_handle(const TraceMeshFaces *trace_handle)
{
  Point3 boxCenter = trace_handle->box.center();
  Point3 boxSize = trace_handle->box.width();
  BBox3 box(boxCenter - boxSize * 0.5f, boxCenter + boxSize * 0.5f);
  draw_debug_box(box, E3DCOLOR_MAKE(trace_handle->hasObjectGroups ? 0 : 255, 0, 0, 255));
  for (int i = 0; i < trace_handle->trianglesCount; ++i)
  {
    Point4 vert, edge0, edge1;

    v_stu(&vert.x, trace_handle->triangles[i * 3 + 0]);
    v_stu(&edge0.x, trace_handle->triangles[i * 3 + 1]);
    v_stu(&edge1.x, trace_handle->triangles[i * 3 + 2]);

    draw_debug_line(Point3::xyz(vert), Point3::xyz(vert + edge0), E3DCOLOR_MAKE(255, 0, 255, 255));
    draw_debug_line(Point3::xyz(vert), Point3::xyz(vert + edge1), E3DCOLOR_MAKE(255, 0, 255, 255));
    draw_debug_line(Point3::xyz(vert + edge0), Point3::xyz(vert + edge1), E3DCOLOR_MAKE(255, 0, 255, 255));
  }
  if (trace_handle->isRendinstsValid)
  {
    trace_handle->rendinstCache.foreachValid(rendinst::GatherRiTypeFlag::RiGenTmAndExtra,
      [&](const rendinst::RendInstDesc &ri_desc, bool is_ri_extra) {
        CollisionResource *collRes = nullptr;
        if (!is_ri_extra)
        {
          collRes = rendinst::getRIGenCollInfo(ri_desc);
        }
        else
        {
          rendinst::riex_handle_t handle = ri_desc.getRiExtraHandle();
          rendinst::getRIExtraCollInfo(handle, &collRes, nullptr);
        }
        if (!collRes)
          return;
        TMatrix tm = rendinst::getRIGenMatrix(ri_desc);
        CollisionResource::DebugDrawData debugData;
        debugData.localNodeTree = true;
        collRes->drawDebug(tm, nullptr, debugData);
      });
  }
}

void gamephys::draw_trace_handle_debug_stats(const TraceMeshFaces *trace_handle, const Point3 &wpos)
{
#if TRACE_HANDLE_DEBUG_STATS_ENABLED
  String text1(128, "inv: %d/ri inv: %d", trace_handle->stats.numFullInvalidates, trace_handle->stats.numRIInvalidates);
  String text2(128, "casts: %d/miss: %d/ri miss: %d", trace_handle->stats.numCasts, trace_handle->stats.numFullMisses,
    trace_handle->stats.numRIMisses);
  float missFullPrc = 0.0f;
  float missRIPrc = 0.0f;
  if (trace_handle->stats.numCasts > 0)
  {
    missFullPrc = 100.0f * (float)trace_handle->stats.numFullMisses / trace_handle->stats.numCasts;
    missRIPrc = 100.0f * (float)trace_handle->stats.numRIMisses / trace_handle->stats.numCasts;
  }
  String text3(128, "miss: %.3f%%/ri miss: %.3f%%", missFullPrc, missRIPrc);
  add_debug_text_mark(wpos + Point3(0.f, 1.3f, 0.f), text1.str(), -1, 0);
  add_debug_text_mark(wpos + Point3(0.f, 1.3f, 0.f), text2.str(), -1, 1);
  add_debug_text_mark(wpos + Point3(0.f, 1.3f, 0.f), text3.str(), -1, 2);
#else
  G_UNUSED(trace_handle);
  G_UNUSED(wpos);
#endif
}

void gamephys::draw_trace_handle_debug_invalidate(const TraceMeshFaces *trace_handle, bool is_for_ri)
{
  if (trace_handle->debugDrawInvalidates)
    draw_debug_box_buffered(trace_handle->box, (is_for_ri ? E3DCOLOR_MAKE(0, 255, 0, 255) : E3DCOLOR_MAKE(255, 0, 0, 255)), 600);
}

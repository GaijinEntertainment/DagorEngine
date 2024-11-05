// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/render/updateStageRender.h>
#include <debug/dag_debug3d.h>
#include <vecmath/dag_vecMath_common.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <EASTL/numeric.h>
#include <math/random/dag_random.h>
#include <ecs/delayedAct/actInThread.h>
#include <EASTL/array.h>
#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <landMesh/heightmapQuery.h>


CONSOLE_BOOL_VAL("hmap_query_debug", debug_draw, false);
CONSOLE_BOOL_VAL("hmap_query_debug", enable_query, true);
CONSOLE_BOOL_VAL("hmap_query_debug", use_jitter, true);
CONSOLE_BOOL_VAL("hmap_query_debug", use_shuffle, true);
CONSOLE_INT_VAL("hmap_query_debug", query_type, 2, 0, 2);
CONSOLE_FLOAT_VAL_MINMAX("hmap_query_debug", radius, 0.2, 0.01, 10.0);
CONSOLE_FLOAT_VAL_MINMAX("hmap_query_debug", spacing, 3.0, 0.01, 20.0);


struct CachedDebugElem
{
  int queryIndex;
  Point3 pos;
};

static constexpr int MAX_DEBUG_ELEMS_ROW_EXTEND = 7;
static constexpr int MAX_DEBUG_ELEMS_ROW = MAX_DEBUG_ELEMS_ROW_EXTEND * 2 + 1;
static constexpr int MAX_DEBUG_ELEMS = MAX_DEBUG_ELEMS_ROW * MAX_DEBUG_ELEMS_ROW;
eastl::array<CachedDebugElem, MAX_DEBUG_ELEMS> cachedDebugElems;
eastl::array<int, MAX_DEBUG_ELEMS> indexArr;


ECS_NO_ORDER
ECS_TAG(render, dev)
static void debug_draw_heightmap_queries_es(const ecs::UpdateStageInfoRenderDebug &,
  // static void debug_draw_heightmap_queries_es(const ParallelUpdateFrameDelayed&, // for perf measurements (remove debug lines before
  // use)
  const TMatrix &transform,
  const bool camera__active)
{
  TIME_PROFILE(debug_draw_heightmap_queries);

  if (!(debug_draw && camera__active))
    return;

  Point2 centerPos2d = Point2(transform.getcol(3).x, transform.getcol(3).z);
  Point3 offsetPos = Point3(0, radius, 0);

  {
    TIME_PROFILE(prepare);

    int rndSeed = 666;
    static bool bInitIndexArr = true;
    if (bInitIndexArr)
    {
      bInitIndexArr = false;
      eastl::iota(indexArr.begin(), indexArr.end(), 0);
      if (use_shuffle)
        eastl::random_shuffle(indexArr.begin(), indexArr.end(), [&](uint32_t n) { return static_cast<uint32_t>(_rnd(rndSeed) % n); });
    }

    for (int y = -MAX_DEBUG_ELEMS_ROW_EXTEND; y <= MAX_DEBUG_ELEMS_ROW_EXTEND; ++y)
      for (int x = -MAX_DEBUG_ELEMS_ROW_EXTEND; x <= MAX_DEBUG_ELEMS_ROW_EXTEND; ++x)
      {
        int i = (y + MAX_DEBUG_ELEMS_ROW_EXTEND) * MAX_DEBUG_ELEMS_ROW + (x + MAX_DEBUG_ELEMS_ROW_EXTEND);
        int index = indexArr[i];

        Point2 pos2d = Point2(x, y);
        if (use_jitter)
        {
          pos2d.x += _rnd_float(rndSeed, 0, 1);
          pos2d.y += _rnd_float(rndSeed, 0, 1);
        }
        pos2d = centerPos2d + pos2d * spacing;
        CachedDebugElem &elem = cachedDebugElems[index];
        elem.pos.x = pos2d.x;
        elem.pos.z = pos2d.y;
      }
  }

  {
    TIME_PROFILE(process);
    begin_draw_cached_debug_lines();
    for (auto &elem : cachedDebugElems)
    {
      if (elem.queryIndex >= 0)
      {
        HeightmapQueryResult queryResult;
        GpuReadbackResultState queryResultState = heightmap_query::get_query_result(elem.queryIndex, queryResult);
        if (::is_gpu_readback_query_successful(queryResultState))
        {
          elem.pos.y = (query_type == 0) ? queryResult.heightNoOffset
                                         : ((query_type == 1) ? queryResult.heightWithOffset : queryResult.heightWithOffsetDeform);
          elem.queryIndex = -1;
        }
        else if (::is_gpu_readback_query_failed(queryResultState))
          elem.queryIndex = -1;
      }

      draw_cached_debug_line_twocolored(elem.pos, elem.pos + offsetPos, E3DCOLOR(255, 0, 255), E3DCOLOR(255, 255, 0));

      if (enable_query && elem.queryIndex < 0)
        elem.queryIndex = heightmap_query::query(Point2(elem.pos.x, elem.pos.z));
    }
    end_draw_cached_debug_lines();
  }
}

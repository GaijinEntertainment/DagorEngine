//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_mathBase.h>
#include <math/dag_math3d.h>
#include <math/dag_bounds2.h>
#include <vecmath/dag_vecMath.h>
#include <rendInst/rendInstDesc.h>
#include <rendInst/constants.h>
#include <debug/dag_debug3d.h>
#include <EASTL/sort.h>

struct Trace
{
  struct alignas(16) PosAndT : public Point3
  {
    float outT;
    __forceinline PosAndT() : outT(0.f) {}
    __forceinline PosAndT &operator=(const Point3 &b)
    {
      Point3::operator=(b);
      return *this;
    }
    __forceinline PosAndT(const Point3 &b) : Point3(b), outT(0.f) {}
    __forceinline PosAndT(const Point3 &b, float out_t) : Point3(b), outT(out_t) {}
  };
  PosAndT pos;
  Point3_vec4 outNorm;
  Point3_vec4 dir;
  int outMatId;
  float hmapHeight;
  void *userPtr;

  Trace() : outMatId(-1), dir(ZERO<Point3_vec4>()), userPtr(nullptr) {}

  Trace(const Point3 &from, const Point3 &dir_to, float t, void *user_ptr)
  {
    pos = from;
    dir = dir_to;
    dir.resv = 0.f;
    pos.outT = t;
    outMatId = -1;
    hmapHeight = -100000;
    userPtr = user_ptr;
  }
};

struct TraceMeshFaces
{
  enum
  {
    MAX_TRIANGLES_PER_SYSTEM = 20,
    MAX_TRIANGLES = MAX_TRIANGLES_PER_SYSTEM * 2
  };

  BBox3 box;
  carray<vec4f, MAX_TRIANGLES * 3> triangles;
  int trianglesCount;
  struct HeightmapCache
  {
    static constexpr int MAX_HEIGHTMAP_VERTICES = 15 * 15;
    carray<float, MAX_HEIGHTMAP_VERTICES> heights;
    Point2 gridOffset;
    float gridElemSize;
    int8_t width, height;
    // heightmap will start at trianglesCount*3 vertices and will consist of (width+1) * (height+1) vertices
    HeightmapCache() : width(-1), height(-1) {}
  } heightmap;

  class RendinstCache
  {
  public:
    static constexpr size_t MAX_ENTRIES = 128;

    mutable int version = -1;

    template <typename callback_t>
    __forceinline void foreachValid(rendinst::GatherRiTypeFlags ri_types, callback_t callback) const
    {
      for (const CacheEntry &entry : cachedData)
      {
        bool isRiExtra = (entry.type & rendinst::GatherRiTypeFlag::RiExtraOnly).asInteger() != 0;
        if ((entry.type & ri_types) && rendinst::isRiGenDescValid(entry.desc))
          callback(entry.desc, isRiExtra);
      }
    }
    bool isFull() const { return cachedData.size() == cachedData.capacity(); }
    int size() const { return cachedData.size(); }
    void add(const rendinst::RendInstDesc &desc, bool is_pos)
    {
      rendinst::GatherRiTypeFlags type =
        desc.isRiExtra() ? rendinst::GatherRiTypeFlag::RiExtraOnly
                         : (is_pos ? rendinst::GatherRiTypeFlag::RiGenPosOnly : rendinst::GatherRiTypeFlag::RiGenTmOnly);
      cachedData.emplace_back(CacheEntry{type, desc});
    }
    void clear() { cachedData.clear(); }
    void sort()
    {
      auto cmp = [](const CacheEntry &lhs, const CacheEntry &rhs) {
        return lhs.desc.getRiExtraHandle() < rhs.desc.getRiExtraHandle();
      };
      eastl::sort(cachedData.begin(), cachedData.end(), cmp);
    }

  private:
    struct CacheEntry
    {
      rendinst::GatherRiTypeFlags type;
      rendinst::RendInstDesc desc;
    };
    StaticTab<CacheEntry, MAX_ENTRIES> cachedData;
  } rendinstCache;

  struct MaterialMapCache
  {
    Tab<uint8_t> matIds;
    uint8_t width;
    uint8_t height;
    BBox2 box;
    Point2 mul;
    Point2 add;
    bool valid;

    MaterialMapCache()
    {
      width = 0;
      height = 0;
      box.setempty();
      mul.zero();
      add.zero();
      valid = false;
    }

    void setBox(uint8_t wd, uint8_t ht, const BBox2 &in_box)
    {
      box = in_box;
      add = -box.lim[0];
      Point2 boxWidth = box.width();
      width = wd;
      height = ht;
      mul.x = safediv(width - 1, boxWidth.x);
      mul.y = safediv(height - 1, boxWidth.y);
      valid = true;
    }

    int getMatAt(const Point2 &pos) const
    {
      Point2 localPos = pos + add;
      localPos.x *= mul.x;
      localPos.y *= mul.y;

      IPoint2 posi = IPoint2(localPos.x, localPos.y);
      if (posi.x < 0 || posi.y < 0 || posi.x >= width || posi.y >= height)
        return -1;

      static const uint8_t invalid_mat_id = 0xFF;

      const uint8_t res = matIds[posi.y * width + posi.x];
      return res == invalid_mat_id ? -1 : res;
    }
  } matMapCache;

#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
  struct Stats
  {
    int numFullInvalidates = 0;    // Full invalidate happens when new aabb no longer fits in inflated aabb.
    int numRIInvalidates = 0;      // RI invalidate happens when new aabb fits, but RI world has changed.
    mutable int numCasts = 0;      // Total number of casts (ray, sphere, whatever) through this cache.
    mutable int numFullMisses = 0; // Number of aabb misses.
    mutable int numRIMisses = 0;   // Number of times when aabb didn't miss, but RI world has moved and, thus, we had a cache miss.

    void reset()
    {
      numFullInvalidates = 0;
      numRIInvalidates = 0;
      numCasts = 0;
      numFullMisses = 0;
      numRIMisses = 0;
    }
  } stats;
#define TRACE_HANDLE_DEBUG_STATS_ENABLED        1
#define TRACE_HANDLE_DEBUG_STAT(handle, member) handle->stats.member
#define TRACE_HANDLE_DEBUG_STATS_RESET(handle)  handle->stats.reset()
#else
#define TRACE_HANDLE_DEBUG_STATS_ENABLED 0
#define TRACE_HANDLE_DEBUG_STAT(handle, member) \
  do                                            \
  {                                             \
  } while (false)
#define TRACE_HANDLE_DEBUG_STATS_RESET(handle) \
  do                                           \
  {                                            \
  } while (false)
#endif

  bool isLandmeshValid;
  bool isStaticValid;
  bool isRendinstsValid;
  bool hasRendinsts;          // 1bit cache for rendinsts.
  bool hasTraceableRendinsts; // Another 1bit cache for rendinsts.
  bool hasObjectGroups;       // 1bit cache for object groups
  bool debugDrawInvalidates;  // red - full invalidate, green - RI invalidate
  bool debugDrawCasts;        // white - all casts
  bool debugDrawMisses;       // red - full miss, green - RI miss
  bool isDirty;

  TraceMeshFaces() :
    trianglesCount(0),
    isLandmeshValid(false),
    isStaticValid(false),
    hasRendinsts(true),
    hasTraceableRendinsts(true),
    isRendinstsValid(false),
    hasObjectGroups(true),
    debugDrawInvalidates(false),
    debugDrawCasts(false),
    debugDrawMisses(false),
    isDirty(false)
  {}

  void markDirty() { isDirty = true; }
};


namespace trace_utils
{
inline void prepare_traces(dag::Span<Trace> traces, bbox3f &box, dag::Span<vec4f> ray_start_pos, dag::Span<vec4f> out_norm,
  bool expand_down = true)
{
#ifdef _DEBUG_TAB_
  G_ASSERTF(traces.size() <= ray_start_pos.size() && ray_start_pos.size() == out_norm.size(), "%d %d %d", traces.size(),
    ray_start_pos.size(), out_norm.size());
#endif
  v_bbox3_init(box, ray_start_pos[0] = v_ld(&traces[0].pos.x));
  v_bbox3_add_pt(box, v_madd(v_ld(&traces[0].dir.x), v_splat_w(ray_start_pos[0]), ray_start_pos[0]));
  for (int i = 1; i < traces.size(); ++i)
  {
    v_bbox3_add_pt(box, ray_start_pos[i] = v_ld(&traces[i].pos.x));
    v_bbox3_add_pt(box, v_madd(v_ld(&traces[i].dir.x), v_splat_w(ray_start_pos[i]), ray_start_pos[i]));
  }
  if (expand_down)
  {
    // FIXME: We keep this for WT only, this is how thing were done there, we probably
    // can get rid of it later.
    box.bmin = v_sub(box.bmin, v_and(v_splat_w(box.bmax), v_cast_vec4f(V_CI_MASK0100)));
  }
  for (int rc = traces.size(); (rc & 3) != 0; ++rc)
    ray_start_pos[rc] = ray_start_pos[traces.size() - 1];

  for (int i = 0; i < traces.size(); ++i)
    out_norm[i] = v_perm_xyzd(v_ld(&traces[i].outNorm.x), v_cast_vec4f(v_splatsi(0x80000000))); // -0.0 in float and magic number we
                                                                                                // can detect
}

inline void prepare_traces_box(dag::Span<Trace> traces, bbox3f &box)
{
  vec4f posT = v_ld(&traces[0].pos.x);
  v_bbox3_init(box, posT);
  v_bbox3_add_pt(box, v_madd(v_ld(&traces[0].dir.x), v_splat_w(posT), posT));
  for (int i = 1; i < traces.size(); ++i)
  {
    posT = v_ld(&traces[i].pos.x);
    v_bbox3_add_pt(box, posT);
    v_bbox3_add_pt(box, v_madd(v_ld(&traces[i].dir.x), v_splat_w(posT), posT));
  }
}

inline void store_traces(dag::Span<Trace> traces, dag::Span<vec4f> ray_start_pos, dag::Span<vec4f> norm)
{
  for (int i = 0; i < traces.size(); ++i)
  {
    v_st(&traces[i].pos.x, ray_start_pos[i]);
    v_st(&traces[i].outNorm, norm[i]);
  }
}

inline void draw_trace_handle_debug_cast_result(const TraceMeshFaces *trace_handle, dag::Span<Trace> traces, bool res, bool is_for_ri)
{
#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
  if (res)
  {
    if (trace_handle->debugDrawCasts)
      for (const auto &trace : traces)
        draw_debug_line_buffered(trace.pos, trace.pos + trace.dir * trace.pos.outT, E3DCOLOR_MAKE(255, 255, 255, 255), 60);
  }
  else if (trace_handle->debugDrawMisses)
  {
    for (const auto &trace : traces)
      draw_debug_line_buffered(trace.pos, trace.pos + trace.dir * trace.pos.outT,
        (is_for_ri ? E3DCOLOR_MAKE(0, 255, 0, 255) : E3DCOLOR_MAKE(255, 0, 0, 255)), 60);
  }
#else
  G_UNUSED(trace_handle);
  G_UNUSED(traces);
  G_UNUSED(res);
  G_UNUSED(is_for_ri);
#endif
}

inline void draw_trace_handle_debug_cast_result(const TraceMeshFaces *trace_handle, const BBox3 &wbox, bool res, bool is_for_ri)
{
#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
  if (res)
  {
    if (trace_handle->debugDrawCasts)
      draw_debug_box_buffered(wbox, E3DCOLOR_MAKE(255, 255, 255, 255), 60);
  }
  else if (trace_handle->debugDrawMisses)
    draw_debug_box_buffered(wbox, (is_for_ri ? E3DCOLOR_MAKE(0, 255, 0, 255) : E3DCOLOR_MAKE(255, 0, 0, 255)), 60);
#else
  G_UNUSED(trace_handle);
  G_UNUSED(wbox);
  G_UNUSED(res);
  G_UNUSED(is_for_ri);
#endif
}

inline void draw_trace_handle_debug_cast_result(const TraceMeshFaces *trace_handle, const TMatrix &tm, const BBox3 &box, bool res,
  bool is_for_ri)
{
#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
  if (res)
  {
    if (trace_handle->debugDrawCasts)
      draw_debug_box_buffered(tm * box, E3DCOLOR_MAKE(255, 255, 255, 255), 60);
  }
  else if (trace_handle->debugDrawMisses)
    draw_debug_box_buffered(tm * box, (is_for_ri ? E3DCOLOR_MAKE(0, 255, 0, 255) : E3DCOLOR_MAKE(255, 0, 0, 255)), 60);
#else
  G_UNUSED(trace_handle);
  G_UNUSED(tm);
  G_UNUSED(box);
  G_UNUSED(res);
  G_UNUSED(is_for_ri);
#endif
}

const float defaultHeight = -1000000.0f;
} // namespace trace_utils

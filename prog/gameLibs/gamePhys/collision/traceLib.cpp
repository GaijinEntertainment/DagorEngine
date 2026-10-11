// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/collisionLib.h>
#include <gameLibs/gamePhys/bundleAllHits_api.h>
#include <gameLibs/gamePhys/bundleTerrain_api.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <rendInst/rendInstCollision.h>
#include <landMesh/lmeshManager.h>
#include <heightmap/heightmapHandler.h>
#include <gameRes/dag_collisionResource.h>
#include <rendInst/traceUtils.h>
#include <fftWater/fftWater.h>
#include <perfMon/dag_statDrv.h>
#include <gamePhys/collision/contactData.h>
#include <math/dag_mathUtils.h>
#include <gamePhys/collision/collisionCache.h>
#include <math/dag_traceRayTriangle.h>

#include "collisionGlobals.h"
#include <supp/dag_alloca.h>

static const float invalid_water_height = dacoll::INVALID_WATER_HEIGHT;
static void first_mirroring_transform(float &in_out_x, float &in_out_z, float &out_x_k, float &out_z_k);

// The RI tracer always adds Destructible|Meshes; these are the caller-selected extras.
static rendinst::TraceFlags etf_to_ri_additional_flags(int flags)
{
  rendinst::TraceFlags f = {};
  if (flags & dacoll::ETF_RI_TREES)
    f |= rendinst::TraceFlag::Trees;
  if (flags & dacoll::ETF_RI_PHYS)
    f |= rendinst::TraceFlag::Phys;
  return f;
}

// Gate a warmed trace-mesh cache for a ray batch: if it can't serve this box, null the
// handle so the caller falls back to a full trace. Emits the cache-hit debug draw.
static void gate_trace_cache(dag::Span<Trace> traces, const TraceMeshFaces *&handle)
{
  if (!handle)
    return;
  bbox3f rayBox;
  trace_utils::prepare_traces_box(traces, rayBox);
  bool res = dacoll::try_use_trace_cache(rayBox, handle);
  trace_utils::draw_trace_handle_debug_cast_result(handle, traces, res, false);
  if (!res)
    handle = nullptr;
}


static bool trace_world_resource(const CollisionResource *res, const Point3 &p, const Point3 &dir, real &t, int *out_pmid,
  Point3 *out_norm, bool force_cull)
{
  if (!res)
    return false;
  int matId = PHYSMAT_INVALID;
  mat44f identTm;
  v_mat44_ident(identTm);
  if (
    !res->traceRay(identTm, p, dir, t, out_norm, matId, /*ray_mat_id*/ -1, CollisionNode::TRACEABLE, /*tm_cache*/ nullptr, force_cull))
    return false;
  if (out_pmid)
    *out_pmid = matId;
  return true;
}

bool dacoll::traceray_normalized_frt(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm)
{
  return trace_world_resource(get_static_collision_resource(), p, dir, t, out_pmid, out_norm, /*force_cull*/ false);
}

bool dacoll::tracedown_normalized_frt(const Point3 &p, real &t, int *out_pmid, Point3 *out_norm)
{
  return trace_world_resource(get_static_collision_resource(), p, Point3(0.f, -1.f, 0.f), t, out_pmid, out_norm, /*force_cull*/ true);
}

bool dacoll::traceray_water(const Point3 &p, const Point3 &dir, real &t, Point3 *out_norm)
{
  return trace_world_resource(get_water_collision(), p, dir, t, /*out_pmid*/ nullptr, out_norm, /*force_cull*/ false);
}

bool dacoll::tracedown_water(const Point3 &p, real &t, Point3 *out_norm)
{
  return trace_world_resource(get_water_collision(), p, Point3(0.f, -1.f, 0.f), t, /*out_pmid*/ nullptr, out_norm,
    /*force_cull*/ true);
}

// constraint: the handle's material map (collisionCache.cpp) holds the phys map with decals, and gives -1 off its grid
static bool traceray_lmesh(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm, const TraceMeshFaces *handle)
{
  if (!dacoll::get_lmesh())
    return false;
  Point3 transPos(p);
  float xk, zk;
  first_mirroring_transform(transPos.x, transPos.z, xk, zk);
  Point3 transDir = Point3(dir.x * xk, dir.y, dir.z * zk);
  bool res = dacoll::get_lmesh()->traceray(transPos, transDir, t, out_norm);
  if (res && out_pmid)
  {
    const int cacheMat = handle ? handle->matMapCache.getMatAt(Point2::xz(p + dir * t)) : -1;
    *out_pmid = cacheMat >= 0 ? cacheMat : dacoll::get_lmesh_mat_id_at_point(Point2::xz(transPos + transDir * t));
  }
  if (res && out_norm)
  {
    out_norm->x *= xk;
    out_norm->z *= zk;
  }
  return res;
}

bool dacoll::traceray_normalized_lmesh(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm)
{
  return traceray_lmesh(p, dir, t, out_pmid, out_norm, /*handle*/ nullptr);
}

bool dacoll::rayhit_normalized_frt(const Point3 &p, const Point3 &dir, real t)
{
  const CollisionResource *res = dacoll::get_static_collision_resource();
  if (!res)
    return false;
  mat44f identTm;
  v_mat44_ident(identTm);
  int outMat;
  return res->rayHit(identTm, p, dir, t, PHYSMAT_INVALID, outMat, CollisionNode::TRACEABLE);
}

bool dacoll::rayhit_normalized_lmesh(const Point3 &p, const Point3 &dir, real t)
{
  Point3 transPos(p);
  float xk, zk;
  first_mirroring_transform(transPos.x, transPos.z, xk, zk);
  Point3 transDir = Point3(dir.x * xk, dir.y, dir.z * zk);
  return get_lmesh() ? get_lmesh()->rayhitNormalized(transPos, transDir, t) : false;
}

bool dacoll::rayhit_normalized_ri(const Point3 &p, const Point3 &dir, real t, rendinst::TraceFlags additional_trace_flags,
  int ray_mat_id, const TraceMeshFaces *handle, rendinst::riex_handle_t skip_riex_handle)
{
  Trace traceData(p, dir, t, nullptr);
  dag::Span<Trace> traceDataSlice(&traceData, 1);
  rendinst::TraceFlags traceFlags = rendinst::TraceFlag::Destructible | rendinst::TraceFlag::Meshes | additional_trace_flags;
  gate_trace_cache(traceDataSlice, handle);
  return rendinst::rayhitRIGenNormalized(traceDataSlice, traceFlags, ray_mat_id, nullptr, handle, skip_riex_handle);
}

bool dacoll::rayhit_normalized(const Point3 &p, const Point3 &dir, real t, int flags, int ray_mat_id, const TraceMeshFaces *handle,
  rendinst::riex_handle_t skip_riex_handle)
{
  TIME_PROFILE_NAME(rayhit, handle ? "rayhit_handle" : "rayhit");

  if (flags & (ETF_FRT))
    if (rayhit_normalized_frt(p, dir, t))
      return true;

  if (flags & (ETF_LMESH | ETF_HEIGHTMAP))
    if (rayhit_normalized_lmesh(p, dir, t))
      return true;

  if (flags & ETF_RI)
    if (rayhit_normalized_ri(p, dir, t, etf_to_ri_additional_flags(flags), ray_mat_id, handle, skip_riex_handle))
      return true;

  return false;
}

bool dacoll::rayhit_normalized_transparency(const Point3 &p, const Point3 &dir, float t, float threshold, int ray_mat_id)
{
  return rendinst::traceTransparencyRayRIGenNormalized(p, dir, t, threshold, ray_mat_id);
}

bool dacoll::rayhit_normalized_sound_occlusion(const Point3 &p, const Point3 &dir, float t, int ray_mat_id,
  float &accumulated_occlusion, float &max_occlusion)
{
  return rendinst::traceSoundOcclusionRayRIGenNormalized(p, dir, t, ray_mat_id, accumulated_occlusion, max_occlusion);
}

bool dacoll::traceray_normalized_ri(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm,
  rendinst::TraceFlags additional_trace_flags, rendinst::RendInstDesc *out_desc, int ray_mat_id, const TraceMeshFaces *handle,
  rendinst::riex_handle_t skip_riex_handle)
{
  Trace traceData(p, dir, t, nullptr);
  dag::Span<Trace> traceDataSlice(&traceData, 1);
  if (out_norm)
    traceData.outNorm = *out_norm;
  rendinst::TraceFlags traceFlags = rendinst::TraceFlag::Destructible | rendinst::TraceFlag::Meshes | additional_trace_flags;
  gate_trace_cache(traceDataSlice, handle);
  bool res = rendinst::traceRayRIGenNormalized(traceDataSlice, traceFlags, ray_mat_id, out_desc, handle, skip_riex_handle);
  if (res)
  {
    if (out_norm)
      *out_norm = traceData.outNorm;
    if (out_pmid)
      *out_pmid = traceData.outMatId;
    t = traceData.pos.outT;
  }
  return res;
}

bool dacoll::traceray_normalized(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm, int flags,
  rendinst::RendInstDesc *out_desc, int ray_mat_id, const TraceMeshFaces *handle, rendinst::riex_handle_t skip_riex_handle)
{
  return dacoll::traceray_normalized_coll_type(p, dir, t, out_pmid, out_norm, flags, out_desc, nullptr, ray_mat_id, handle,
    skip_riex_handle);
}

bool dacoll::traceray_normalized_coll_type(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm, int flags,
  rendinst::RendInstDesc *out_desc, int *out_coll_type, int ray_mat_id, const TraceMeshFaces *handle,
  rendinst::riex_handle_t skip_riex_handle)
{
  G_ASSERTF(!check_nan(p) && p.lengthSq() < 1e11f && !check_nan(dir) && !check_nan(t), "%@ %@ %f", p, dir, t);

  TIME_PROFILE_NAME(traceray, handle ? "traceray_handle" : "traceray");

  bool res = false;
  if (flags & ETF_FRT)
  {
    bool trace_res = traceray_normalized_frt(p, dir, t, out_pmid, out_norm);
    if (trace_res && out_coll_type)
      *out_coll_type = ETF_FRT;
    res |= trace_res;
  }
  if (flags & (ETF_LMESH | ETF_HEIGHTMAP))
  {
    bool trace_res = traceray_lmesh(p, dir, t, out_pmid, out_norm, handle);
    if (trace_res && out_coll_type)
      *out_coll_type = ETF_LMESH;
    res |= trace_res;
  }
  if (flags & ETF_RI)
  {
    bool trace_res = traceray_normalized_ri(p, dir, t, out_pmid, out_norm, etf_to_ri_additional_flags(flags), out_desc, ray_mat_id,
      handle, skip_riex_handle);
    if (trace_res && out_coll_type)
      *out_coll_type = ETF_RI;
    res |= trace_res;
  }
  return res;
}

bool dacoll::traceray_normalized_multiray(dag::Span<Trace> traces, dag::Span<rendinst::RendInstDesc> ri_desc, int flags,
  int ray_mat_id, const TraceMeshFaces *handle)
{
  G_ASSERT(traces.size() == ri_desc.size());
  if (traces.empty())
    return false;

  bool res = false;
  // FRT and LMESH have no batch API, so run them per ray; each shrinks the ray's outT that
  // the RI stage then uses as its max distance (same closest-hit composition as single-ray).
  for (Trace &trace : traces)
  {
    if (flags & ETF_FRT)
      res |= traceray_normalized_frt(trace.pos, trace.dir, trace.pos.outT, &trace.outMatId, &trace.outNorm);
    if (flags & (ETF_LMESH | ETF_HEIGHTMAP))
      res |= traceray_lmesh(trace.pos, trace.dir, trace.pos.outT, &trace.outMatId, &trace.outNorm, handle);
  }

  // RI is genuinely batched: one gather + grid traversal over the union box for the whole
  // span, filling one RendInstDesc per ray. The cache is gated once on the union box.
  if (flags & ETF_RI)
  {
    rendinst::TraceFlags riFlags = rendinst::TraceFlag::Destructible | rendinst::TraceFlag::Meshes | etf_to_ri_additional_flags(flags);
    // Batching amortizes the RI gather only while the rays are spatially coherent; with
    // far-apart origins the union box covers a huge region and testing every gathered
    // instance against every ray loses to per-ray traces. Real callers (animchar legs,
    // wheels, gear) stay within meters; the threshold only guards pathological batches.
    const float maxBatchedOriginSpread = 256.f;
    BBox3 originsBox;
    for (const Trace &trace : traces)
      originsBox += trace.pos;
    if (traces.size() > 1 && originsBox.width().length() > maxBatchedOriginSpread)
    {
      int i = 0;
      for (Trace &trace : traces)
      {
        if (!traceray_normalized_ri(trace.pos, trace.dir, trace.pos.outT, &trace.outMatId, &trace.outNorm,
              etf_to_ri_additional_flags(flags), &ri_desc[i], ray_mat_id, handle))
          ri_desc[i].invalidate();
        else
          res = true;
        i++;
      }
      return res;
    }
    const TraceMeshFaces *riHandle = handle;
    gate_trace_cache(traces, riHandle);
    if (rendinst::traceRayRIGenNormalizedMultiRay(traces, riFlags, ri_desc, ray_mat_id, riHandle))
      res = true;
  }
  return res;
}

bool dacoll::tracedown_normalized(const Point3 &p, real &t, int *out_pmid, Point3 *out_norm, int flags,
  rendinst::RendInstDesc *out_desc, int ray_mat_id, const TraceMeshFaces *handle)
{
  Trace trace(p, Point3(0.f, -1.f, 0.f), t, nullptr);
  rendinst::RendInstDesc desc;
  if (!tracedown_normalized_multiray(dag::Span<Trace>(&trace, 1), dag::Span<rendinst::RendInstDesc>(&desc, 1), flags, ray_mat_id,
        handle))
    return false;
  t = trace.pos.outT;
  if (out_pmid)
    *out_pmid = trace.outMatId;
  if (out_norm)
    *out_norm = trace.outNorm;
  if (out_desc)
    *out_desc = desc;
  return true;
}

// the cached down kernels return the hit triangle's v0.w in the SoA normal's w lane
static void resolve_down_matids(dag::Span<Trace> traces, const TraceMeshFaces *handle, const vec4f *v_out_norm)
{
  for (int i = 0; i < traces.size(); ++i)
  {
    const int m = dacoll::resolve_static_cache_tri_mat(v_extract_wi(v_cast_vec4i(v_out_norm[i])));
    traces[i].outMatId = m >= 0 ? m : handle->matMapCache.getMatAt(Point2::xz(traces[i].pos));
  }
}

// RI stage of a cached down trace. traceDownMultiRay omits Trees/Meshes on purpose (perf).
static bool tracedown_ri_multiray(dag::Span<Trace> traces, bbox3f_cref ray_box, dag::Span<rendinst::RendInstDesc> ri_desc,
  const TraceMeshFaces *handle, int ray_mat_id, int flags, bool rendinsts_valid)
{
  if (!rendinsts_valid)
  {
    TRACE_HANDLE_DEBUG_STAT(handle, numRIMisses++);
    trace_utils::draw_trace_handle_debug_cast_result(handle, traces, false, true);
    return false;
  }
  rendinst::TraceFlags traceFlags = rendinst::TraceFlag::Destructible;
  if ((flags & dacoll::ETF_RI_PHYS) != 0)
    traceFlags |= rendinst::TraceFlag::Phys;
  return rendinst::traceDownMultiRay(traces, ray_box, ri_desc, handle, ray_mat_id, traceFlags);
}

// Every arm of a down multiray answers the static scene through the culled down entry, never the
// two-sided ray walk, so all of them keep to the same faces.
static bool tracedown_static_scene(dag::Span<Trace> traces, int flags)
{
  bool res = false;
  if (flags & dacoll::ETF_FRT)
    for (Trace &trace : traces)
      res |= dacoll::tracedown_normalized_frt(trace.pos, trace.pos.outT, &trace.outMatId, &trace.outNorm);
  return res;
}

bool dacoll::tracedown_normalized_multiray(dag::Span<Trace> traces, dag::Span<rendinst::RendInstDesc> ri_desc, int flags,
  int ray_mat_id, const TraceMeshFaces *handle)
{
  unsigned trcnt = traces.size();
  if (!trcnt)
    return false;

  if (!handle)
  {
    const bool res = tracedown_static_scene(traces, flags);
    return dacoll::traceray_normalized_multiray(traces, ri_desc, flags & ~ETF_FRT, ray_mat_id, handle) || res;
  }

  TIME_PROFILE_DEV(tracedown_handle);

  vec4f rayStartStor[4], *rayStartPosVec = rayStartStor, outNormStor[4], *v_outNorm = outNormStor;
  mat44f tempSOAStor, *tempSOA = &tempSOAStor;
  unsigned trcnt4a = (trcnt + 3) & ~3;
  if (trcnt > 4)
  {
    rayStartPosVec = (vec4f *)alloca(sizeof(vec4f) * trcnt4a * 2 + sizeof(mat44f) * (trcnt4a / 4));
#ifdef _DEBUG_TAB_
    G_FAST_ASSERT(uintptr_t(rayStartPosVec) % 16 == 0);
#endif
    v_outNorm = rayStartPosVec + trcnt4a;
    tempSOA = (mat44f *)(v_outNorm + trcnt4a);
  }

  bbox3f rayBox;
  trace_utils::prepare_traces(traces, rayBox, dag::Span<vec4f>(rayStartPosVec, trcnt4a), dag::Span<vec4f>(v_outNorm, trcnt4a),
    /*expand_down*/ false);

  bool traceRes = try_use_trace_cache(rayBox, handle);
  trace_utils::draw_trace_handle_debug_cast_result(handle, traces, traceRes, false);
  if (!traceRes)
  {
    const bool res = tracedown_static_scene(traces, flags);
    return dacoll::traceray_normalized_multiray(traces, ri_desc, flags & ~ETF_FRT, ray_mat_id) || res;
  }

  bool rendinstsValid = rendinst::checkCachedRiData(handle);

  bool res = false;

  // Down-specialized cached pipeline over the SoA scratch: heightmap cache, then static
  // triangles, then resolve per-ray materials, then rendinsts. Each stage only shrinks outT.
  res |= dacoll::tracedown_hmap_cache_multiray(traces, handle, reinterpret_cast<Point3_vec4 *>(rayStartPosVec),
    reinterpret_cast<Point3_vec4 *>(v_outNorm));
  res |= traceDownTrianglesMultiRay(handle->triangles.data(), handle->trianglesCount, // array of vert0, edge1, edge2
    rayStartPosVec, trcnt4a / 4, v_outNorm, tempSOA);

  trace_utils::store_traces(traces, dag::Span<vec4f>(rayStartPosVec, trcnt), dag::Span<vec4f>(v_outNorm, trcnt));
  resolve_down_matids(traces, handle, v_outNorm);

  if (flags & ETF_RI)
    res |= tracedown_ri_multiray(traces, rayBox, ri_desc, handle, ray_mat_id, flags, rendinstsValid);

  // Fall back to a full trace only for the subsystems the cache did not already cover.
  int fallbackFlags = flags;
  if (handle->isLandmeshValid)
    fallbackFlags &= ~ETF_LMESH;
  if (handle->isStaticValid)
    fallbackFlags &= ~ETF_FRT;
  if (rendinstsValid)
    fallbackFlags &= ~(ETF_RI | ETF_RI_TREES | ETF_RI_PHYS);
  if (handle->heightmap.width >= 0)
    fallbackFlags &= ~ETF_HEIGHTMAP;
  if (!handle->hasObjectGroups)
    fallbackFlags &= ~ETF_OBJECTS_GROUP;
  // A cache that could not hold the static scene falls back here, and the down rule has to survive
  // that: the ordinary ray walk below would answer a face the cached kernel culled.
  res |= tracedown_static_scene(traces, fallbackFlags);
  res |= dacoll::traceray_normalized_multiray(traces, ri_desc, fallbackFlags & ~ETF_FRT, ray_mat_id, handle);

  return res;
}

bool dacoll::tracedown_hmap_cache_multiray(dag::Span<Trace> traces, const TraceMeshFaces *handle, Point3_vec4 *ray_start_pos_vec,
  Point3_vec4 *v_out_norm)
{
  if (handle->heightmap.width <= 0)
    return false;

  const float *height = handle->heightmap.heights.data();
  float cellSize = handle->heightmap.gridElemSize;
  float invElemSize = 1.0 / cellSize;
  float halfCellSize = cellSize * 0.5f;
  int lastX = handle->heightmap.width - 1, lastY = handle->heightmap.height - 1;
  Point3 VC00(-halfCellSize, 0, -halfCellSize); // h0-hmid
  Point3 VC01(-halfCellSize, 0, halfCellSize);  // hy-hmid
  Point3 VC11(halfCellSize, 0, halfCellSize);   // hxy-hmid
  Point3 VC10(halfCellSize, 0, -halfCellSize);  // hx-hmid
  Point3_vec4 *posT = ray_start_pos_vec;
  Point3_vec4 *norm = v_out_norm;
  int i = 0;
  bool res = false;
  LandMeshHolesManager *holes = get_lmesh() ? get_lmesh()->getHolesManager() : nullptr;
  for (Point3_vec4 *endPos = posT + traces.size(); posT != endPos; ++posT, ++norm, ++i)
  {
    vec4f vpt = v_mul(v_sub(v_perm_xzxz(v_ld(&posT->x)), v_ldu_half(&handle->heightmap.gridOffset.x)), v_splats(invElemSize));
    vec4f vptf = v_floor(vpt);
    vec4i xy = v_cvt_vec4i(vptf);
    const int x = v_extract_xi(xy), y = v_extract_yi(xy);
    if (x < 0 || x >= lastX || y < 0 || y >= lastY)
      continue;
    alignas(16) struct
    {
      Point2 pt, ptf;
    } pt_ptf;
    v_st(&pt_ptf.pt.x, v_perm_xyab(vpt, vptf));
    const Point2 pt = pt_ptf.pt, ptf = pt_ptf.ptf;
    int index = x + y * handle->heightmap.width;
    float h0 = height[index];
    float hx = height[index + 1];
    float hy = height[index + handle->heightmap.width];
    float hxy = height[index + handle->heightmap.width + 1];
    float hmid = (h0 + hx + hy + hxy) * 0.25f;
    VC00.y = h0 - hmid;
    VC01.y = hy - hmid;
    VC11.y = hxy - hmid;
    VC10.y = hx - hmid;
    //  hy .-------. hxy
    //     | \ 2 / |
    //     |  \ /  |
    //     | 1 * 3 |
    //     |  / \  |
    //     | / 4 \ |
    //  h0 .-------. hx
    Point2 dpt(pt.x - ptf.x - 0.5f, pt.y - ptf.y - 0.5f);
    float dptx_m_y = dpt.x - dpt.y, dptx_p_y = dpt.x + dpt.y;

    float ht;
    Point3 nrm;
    if (dptx_m_y >= 0.0f)
    {
      if (dptx_p_y >= 0.0f)
      {
        ht = dptx_m_y * VC10.y + dptx_p_y * VC11.y;
        nrm = VC11 % VC10;
      }
      else
      {
        ht = dptx_m_y * VC10.y - dptx_p_y * VC00.y;
        nrm = VC10 % VC00;
      }
    }
    else
    {
      if (dptx_p_y >= 0.0f)
      {
        ht = -dptx_m_y * VC01.y + dptx_p_y * VC11.y;
        nrm = VC01 % VC11;
      }
      else
      {
        ht = -dptx_m_y * VC01.y - dptx_p_y * VC00.y;
        nrm = VC00 % VC01;
      }
    }
    if (!holes || !holes->check(Point2(posT->x, posT->z)))
    {
      traces[i].hmapHeight = hmid + ht;
      float t = posT->y - (hmid + ht);
      if (t > 0.f && t < posT->resv)
      {
        res = true;
        posT->resv = t;
        *norm = normalize(nrm);
      }
    }
  }
  return res;
}

bool dacoll::traceray_normalized_contact(const Point3 &from, const Point3 &to, Tab<gamephys::CollisionContactData> &out_contacts,
  int ray_mat_id, int flags)
{
  Point3 dir = to - from;
  float len = length(dir);
  float originalLen = len;
  dir *= safeinv(len);
  Point3 norm;
  int matId;
  if (dacoll::traceray_normalized(from, dir, len, &matId, &norm, flags, nullptr, ray_mat_id))
  {
    gamephys::CollisionContactData traceCont;
    traceCont.wpos = from + dir * len;
    traceCont.wnormB = norm;
    traceCont.depth = len - originalLen;
    traceCont.userPtrA = NULL;
    traceCont.userPtrB = NULL;
    traceCont.matId = matId;

    out_contacts.push_back(traceCont);
    return true;
  }
  return false;
}

static dacoll::trace_game_objects_cb trace_game_objs_cb = nullptr;

dacoll::trace_game_objects_cb dacoll::set_trace_game_objects_cb(trace_game_objects_cb cb)
{
  auto prev_cb = trace_game_objs_cb;
  trace_game_objs_cb = cb;
  return prev_cb;
}

bool dacoll::trace_game_objects(const Point3 &from, const Point3 &dir, float &t, Point3 &out_vel, int ignore_game_obj_id,
  int ray_mat_id)
{
  if (!trace_game_objs_cb)
    return false;
  return trace_game_objs_cb(from, dir, t, out_vel, ignore_game_obj_id, ray_mat_id, main_grid_types_span);
}

bool dacoll::trace_game_objects_with_grid_filter(const Point3 &from, const Point3 &dir, float &t, Point3 &out_vel,
  int ignore_game_obj_id, int ray_mat_id, const dag::ConstSpan<uint32_t> grid_types)
{
  if (!trace_game_objs_cb)
    return false;
  return trace_game_objs_cb(from, dir, t, out_vel, ignore_game_obj_id, ray_mat_id, grid_types);
}

// Five ints of an instance folded into one key. The low bit is forced so no instance can share the terrain's.
static uint32_t ri_object_key(const rendinst::RendInstDesc &desc)
{
  uint32_t h = uint32_t(desc.cellIdx) * 2654435761u;
  h ^= uint32_t(desc.idx) * 2246822519u;
  h ^= uint32_t(desc.pool) * 3266489917u;
  h ^= desc.offs * 668265263u;
  h ^= uint32_t(desc.layer) * 374761393u;
  return (h ^ (h >> 16)) | 1u;
}

// A probe must select the same geometry as the walk it certifies, or it answers another question.
static rendinst::TraceRendInstParams bundle_walk_params(int ray_mat_id)
{
  rendinst::TraceRendInstParams params;
  params.tracePhysMatId = ray_mat_id;
  params.traceMeshes = true;
  params.triangleHitsOnly = true;
  return params;
}

// pos.outT carries the length; a degenerate segment keeps its slot at zero length, so the ray ids stay the segment indices
static void init_trace(Trace &trace, vec4f from, vec4f to)
{
  const vec4f dir = v_sub(to, from);
  const vec4f len = v_length3(dir);
  const vec4f degenerate = v_cmp_lt(len, v_splats(1e-6f));
  v_st(&trace.pos.x, v_perm_xyzd(from, v_andnot(degenerate, len)));
  v_st(&trace.dir.x, v_sel(v_perm_xyzd(v_div(dir, len), v_zero()), V_C_UNIT_0100, degenerate));
}

// One resource bound a segment reached with an endpoint inside it: a question that object's own hits answer once the
// segment's hit list is complete.
struct EndpointBound
{
  mat44f tm;     // the pose the bound was taken in: a probe that meets another one cannot answer for it
  vec4f probeTo; // out of the bound, for a segment it holds whole that never crossed the object
  uint32_t object;
  uint32_t ray;
  int next;     // the ray's chain
  uint8_t ends; // bit 0 the from end, bit 1 the to end
};

int dacoll::trace_bundle_all_hits(dag::ConstSpan<BundleSegment> segments, int flags, int ray_mat_id, dag::Span<BundleHit> out_hits,
  dag::Span<BundleRange> out_ranges, uint32_t hits_per_segment, bool certify_endpoints)
{
  TIME_PROFILE(trace_bundle_all_hits);
  FRAMEMEM_REGION;
  const int segCount = segments.size();
  if (!segCount)
    return 0;
  G_ASSERT_RETURN(hits_per_segment > 0 && out_ranges.size() >= segCount, 0);
  hits_per_segment = min(hits_per_segment, 0xffffu); // BundleRange::count

  dag::Vector<Trace, framemem_allocator> traces(segCount);
  for (int i = 0; i < segCount; ++i)
    init_trace(traces[i], v_ldu(&segments[i].from.x), v_ldu_p3_safe(&segments[i].to.x)); // to ends the array, from does not

  dag::Vector<EndpointBound, framemem_allocator> bounds;
  dag::Vector<int, framemem_allocator> firstBound;
  if (certify_endpoints)
  {
    bounds.reserve(segCount * 2);
    firstBound.assign(segCount, -1);
  }
  // The world box, not the mesh: deciding the side from a bound would make every source of a dense level read enclosed.
  const auto recordBound = [&](int ray, const rendinst::RendInstDesc &desc, bbox3f_cref resource_bbox,
                             mat44f_cref tm) DAGOR_FORCE_INLINE_LAMBDA {
    bbox3f box;
    v_bbox3_init(box, tm, resource_bbox);
    const vec4f from = v_ldu(&traces[ray].pos.x);
    const vec4f to = v_madd(v_ldu(&traces[ray].dir.x), v_splat_w(from), from);
    const uint8_t ends = uint8_t((v_bbox3_test_pt_inside(box, from) ? 1 : 0) | (v_bbox3_test_pt_inside(box, to) ? 2 : 0));
    if (!ends)
      return;
    EndpointBound &bound = bounds.push_back();
    bound.tm = tm;
    bound.probeTo = ends == 3 ? bundle_bound_exit(box, from) : v_zero();
    bound.object = ri_object_key(desc);
    bound.ray = uint32_t(ray);
    bound.next = firstBound[ray];
    bound.ends = ends;
    firstBound[ray] = int(bounds.size() - 1);
  };

  rendinst::RendInstsIntersectionsListExt riHits;
  if (flags & ETF_RI)
  {
    rendinst::TraceRendInstParams params = bundle_walk_params(ray_mat_id);
    if (certify_endpoints)
      params.visitedBoundsCb = recordBound;
    rendinst::traceRayRendInstsRayBatchAllIntersections(make_span(traces), riHits, params);
  }

  dag::Vector<uint32_t, framemem_allocator> probeRequests;
  uint32_t written = 0, riCursor = 0;
  { // constraint: a hole rewrite on the main thread waits on the holes lock, so it ends with this pass
    // The heightfield alone, never the landmesh ray tracer, so a game and the harness ask the same ground; a hole is no surface.
    const LandMeshManager *lmesh = (flags & ETF_HEIGHTMAP) ? get_lmesh() : nullptr;
    const HeightmapHandler *hmap = lmesh ? lmesh->getHmapHandler() : nullptr;
    const LandMeshHolesManager *holes = lmesh ? lmesh->getHolesManager() : nullptr;
    ScopedLockReadTemplate<OSReadWriteLock> holesGuard(holes ? &holes->rewriteLock() : nullptr); // a game rewrites them in play
    const auto holeAt = [&](const Point2 &pos) { return holes && holes->check(pos); };
    const auto heightAt = [&](const Point2 &pos, float &ht) { return !holeAt(pos) && hmap->getHeight(pos, ht, nullptr); };

    for (int i = 0; i < segCount; ++i)
    {
      BundleHitSink sink;
      sink.hits = out_hits.data() + written;
      const uint32_t remaining = uint32_t(out_hits.size()) - written, reserved = uint32_t(segCount - 1 - i);
      sink.capacity = remaining > reserved ? min(remaining - reserved, hits_per_segment) : 0; // a slot kept for every segment to come
      const Trace &trace = traces[i];

      for (; riCursor < riHits.size() && riHits[riCursor].rayId == unsigned(i); riCursor++) // sorted by ray id, then by t
      {
        const rendinst::TraceRayRendInstDataExt &hit = riHits[riCursor];
        sink.add(hit.tIn, hit.matId, ri_object_key(hit.riDesc), bundle_hit_facing(hit.normIn, trace.dir));
      }
      const float len = trace.pos.outT;
      bool fromUnderGround = false;
      if (hmap && len > 0.f) // a degenerate segment measures nothing, so it opens with no synthetic enter either
        fromUnderGround = trace_terrain_span(*hmap, trace.pos, trace.dir, len, heightAt, holeAt,
          [&](float at, int8_t facing, const Point2 &pos) DAGOR_FORCE_INLINE_LAMBDA {
            sink.add(at, sink.count < sink.capacity ? get_lmesh_mat_id_at_point(pos) : -1, BUNDLE_TERRAIN_OBJECT, facing);
          });

      BundleRange &range = out_ranges[i];
      range.start = written;
      range.count = uint16_t(sink.count);
      range.dropped = uint8_t(min(sink.dropped, 255u));
      range.cutoff = sink.cutoff;
      range.endpoints = BUNDLE_ENDPOINTS_UNKNOWN;
      written += sink.count;
      if (!certify_endpoints || len <= 0.f || sink.dropped) // a lost hit can reverse an endpoint
        continue;
      uint32_t endpoints = BUNDLE_ENDPOINTS_KNOWN | (fromUnderGround ? BUNDLE_ENDPOINT_FROM_INSIDE : 0) |
                           (hmap && terrain_point_inside(segments[i].to, heightAt) ? BUNDLE_ENDPOINT_TO_INSIDE : 0);
      const dag::ConstSpan<BundleHit> segHits(sink.hits, sink.count);
      for (int b = firstBound[i]; b >= 0; b = bounds[b].next)
        // only a complete hit list says which bounds are still open, so the probe out of one is traced after this pass
        bundle_certify_endpoints(endpoints, segHits, bounds[b].object, (bounds[b].ends & 1) != 0, (bounds[b].ends & 2) != 0,
          [&](uint32_t) {
            probeRequests.push_back(uint32_t(b));
            return BundleEndpointSide::Outside;
          });
      range.endpoints = uint8_t(endpoints);
    }
  }
  if (probeRequests.empty())
    return int(written);

  dag::Vector<Trace, framemem_allocator> probes(probeRequests.size());
  for (uint32_t k = 0; k < probeRequests.size(); ++k)
  {
    const EndpointBound &bound = bounds[probeRequests[k]];
    init_trace(probes[k], v_ldu(&traces[bound.ray].pos.x), bound.probeTo);
  }
  // An riExtra instance can move between the two batches, and then the probe end may lie inside it too.
  dag::Vector<uint8_t, framemem_allocator> moved(probeRequests.size(), 0);
  rendinst::TraceRendInstParams probeParams = bundle_walk_params(ray_mat_id);
  const auto checkPose = [&](int k, const rendinst::RendInstDesc &desc, bbox3f_cref, mat44f_cref tm) {
    const EndpointBound &bound = bounds[probeRequests[k]];
    if (ri_object_key(desc) == bound.object && memcmp(&tm, &bound.tm, sizeof(mat44f)) != 0)
      moved[k] = 1;
  };
  probeParams.visitedBoundsCb = checkPose;
  rendinst::RendInstsIntersectionsListExt probeHits;
  rendinst::traceRayRendInstsRayBatchAllIntersections(make_span(probes), probeHits, probeParams);
  // The probe ends outside the bound, so a start inside the mesh has to leave it: the nearest hit of the object is a leave.
  for (uint32_t k = 0, cursor = 0; k < probeRequests.size(); ++k)
  {
    const EndpointBound &bound = bounds[probeRequests[k]];
    const rendinst::TraceRayRendInstDataExt *nearest = nullptr;
    for (; cursor < probeHits.size() && probeHits[cursor].rayId == k; ++cursor)
      if (!nearest && ri_object_key(probeHits[cursor].riDesc) == bound.object)
        nearest = &probeHits[cursor];
    if (moved[k])
      out_ranges[bound.ray].endpoints = BUNDLE_ENDPOINTS_UNKNOWN;
    else if (nearest && bundle_hit_facing(nearest->normIn, probes[k].dir) < 0)
      out_ranges[bound.ray].endpoints |= BUNDLE_ENDPOINT_FROM_INSIDE | BUNDLE_ENDPOINT_TO_INSIDE;
  }
  return int(written);
}

bool dacoll::bundle_segment_any_hit(const Point3 &from, const Point3 &to, int flags, int ray_mat_id)
{
  const BundleSegment seg = {from, to};
  BundleHit hit;
  BundleRange range;
  return trace_bundle_all_hits(dag::ConstSpan<BundleSegment>(&seg, 1), flags, ray_mat_id, dag::Span<BundleHit>(&hit, 1),
           dag::Span<BundleRange>(&range, 1), 1, /*certify_endpoints*/ false) > 0;
}

bool dacoll::trace_sphere_cast_ex(const Point3 &from, const Point3 &to, float rad, int num_casts, dacoll::ShapeQueryOutput &out,
  int cast_mat_id, int ignore_game_obj_id, const TraceMeshFaces *handle, int flags)
{
  TIME_PROFILE_DEV(trace_sphere_cast);

  Point3 dir = to - from;
  float t = length(dir);
  if (t < 1e-5f)
    return false;

  dir *= safeinv(t);

  float len = t;
  t += rad;

  const Point3 axis2 = normalize(dir % (fabsf(dir.y) > 0.99f ? Point3(1.f, 0.f, 0.f) : Point3(0.f, 1.f, 0.f)));
  const Point3 axis1 = dir % axis2;

  float cosPhi = 1.f, sinPhi = 0.f;
  // NOTE: use negative number of casts to re-orient circular traces by PI/2 (tweak)
  if (num_casts < 0)
  {
    num_casts = -num_casts;
    cosPhi = 0.f;
    sinPhi = 1.f;
  }
  const float invNumCasts = safeinv(float(num_casts));
  float sinStep, cosStep;
  sincos(TWOPI * invNumCasts, sinStep, cosStep);

  Tab<Trace> traces(framemem_ptr());
  traces.reserve(num_casts + 1);
  traces.emplace_back(from, dir, t, nullptr);
  for (int i = 0; i < num_casts; ++i)
  {
    traces.emplace_back(from + (axis1 * cosPhi + axis2 * sinPhi) * rad, dir, t, nullptr);
    const float nextCos = cosPhi * cosStep - sinPhi * sinStep;
    sinPhi = sinPhi * cosStep + cosPhi * sinStep;
    cosPhi = nextCos;
  }
  Tab<rendinst::RendInstDesc> riDescs(framemem_ptr());
  riDescs.resize(traces.size());
  dacoll::traceray_normalized_multiray(make_span(traces), make_span(riDescs), flags, cast_mat_id, handle);

  bool res = false;

  out.norm.zero();
  out.vel.zero();
  out.res = to;
  if (traces[0].pos.outT < t)
  {
    t = traces[0].pos.outT;
    out.norm = traces[0].outNorm;
    out.res = from + dir * t;
    res = true;
  }
  if (dacoll::trace_game_objects(from, dir, t, out.vel, ignore_game_obj_id, cast_mat_id))
  {
    out.norm = -dir;
    out.res = from + dir * t;
    res = true;
  }

  float minT = t;
  float minNorm = res ? out.norm * dir : 1.f;
  for (int i = 1; i < traces.size(); ++i)
  {
    const float outT = traces[i].pos.outT;
    if (outT >= t)
      continue;
    if (outT < minT)
    {
      minT = outT;
      out.res = traces[i].pos + dir * outT;
      res = true;
    }
    const float normProj = traces[i].outNorm * dir;
    // check if this normal is better, or if we're out of reach of previous results
    if (normProj < minNorm || (t - outT > rad && outT - minT < rad))
    {
      out.norm = traces[i].outNorm;
      minNorm = normProj;
    }
  }
  out.t = minT / len; // yes, it can be slightly above 1.0, that's in order to make it useful for actual distance measuring.

  return res;
}


float dacoll::traceht_lmesh(const Point2 &pos)
{
  float height = 0.f;
  if (get_lmesh())
  {
    LandMeshHolesManager *holes = get_lmesh()->getHolesManager();
    Point2 transPos(pos);
    float xk, zk;
    first_mirroring_transform(transPos.x, transPos.y, xk, zk);
    if ((!holes || !holes->check(pos)) && get_lmesh()->getHeight(transPos, height, nullptr))
      return height;
  }
  return -1e5f;
}

float dacoll::traceht_hmap(const Point2 &pos)
{
  float height = 0.f;
  const LandMeshHolesManager *holes = get_lmesh() ? get_lmesh()->getHolesManager() : nullptr;
  ScopedLockReadTemplate<OSReadWriteLock> holesGuard(holes ? &holes->rewriteLock() : nullptr); // worker callers race a rewrite
  if (get_lmesh() && get_lmesh()->getHmapHandler() && (!holes || !holes->check(pos)) &&
      get_lmesh()->getHmapHandler()->getHeight(pos, height, nullptr))
    return height;
  return -1e5f;
}

bool dacoll::traceht_water(const Point3 &pos, float &t)
{
  FFTWater *water = get_water();
  if (!water || check_nan(pos))
    return false;
  float res = 1e9f;
  if (!fft_water::getHeightAboveWater(water, pos, res))
    return false;
  t = pos.y - res;
  return true;
}

float dacoll::traceht_water_at_time(const Point3 &pos, float time, Point3 *out_displacement)
{
  FFTWater *water = get_water();
  if (!water)
    return 0.f;
  float res = 1e9f;
  if (fft_water::getHeightAboveWaterAtTime(water, time, pos, res, out_displacement))
    return pos.y - res;
  return 0.f;
}

bool dacoll::traceray_water_at_time(const Point3 &start, const Point3 &end, float time, float &t)
{
  FFTWater *water = get_water();
  if (!water)
    return false;
  return (bool)fft_water::intersect_segment_at_time(water, time, start, end, t);
}

float dacoll::getSignificantWaterWaveHeight()
{
  FFTWater *water = get_water();
  return water != nullptr ? fft_water::get_significant_wave_height(water) : 0.0f;
}

static void first_mirroring_transform(float &in_out_x, float &in_out_z, float &out_x_k, float &out_z_k)
{
  if (!dacoll::get_lmesh())
  {
    out_x_k = out_z_k = 1.f;
    return;
  }

  int numBorderCellsXPos = 0;
  int numBorderCellsXNeg = 0;
  int numBorderCellsZPos = 0;
  int numBorderCellsZNeg = 0;
  dacoll::get_landmesh_mirroring(numBorderCellsXPos, numBorderCellsXNeg, numBorderCellsZPos, numBorderCellsZNeg);

  const BBox3 &box = dacoll::get_lmesh()->getBBox();

  if (in_out_x < box[0].x && numBorderCellsXNeg)
  {
    in_out_x = box[0].x - (in_out_x - box[0].x);
    out_x_k = -1.f;
  }
  else if (in_out_x > box[1].x && numBorderCellsXPos)
  {
    in_out_x = box[1].x - (in_out_x - box[1].x);
    out_x_k = -1.f;
  }
  else
  {
    out_x_k = 1.f;
  }

  if (in_out_z < box[0].z && numBorderCellsZNeg)
  {
    in_out_z = box[0].z - (in_out_z - box[0].z);
    out_z_k = -1.f;
  }
  else if (in_out_z > box[1].z && numBorderCellsZPos)
  {
    in_out_z = box[1].z - (in_out_z - box[1].z);
    out_z_k = -1.f;
  }
  else
  {
    out_z_k = 1.f;
  }
}

bool dacoll::get_min_max_hmap_in_circle(const Point2 &center, float rad, float &min_ht, float &max_ht)
{
  Tab<Point2> list(framemem_ptr());
  get_min_max_hmap_list_in_circle(center, rad, list);
  if (list.empty())
    return false;
  // init to first element
  min_ht = list[0].x;
  max_ht = list[0].y;
  for (const Point2 &minMax : list)
  {
    min_ht = min(min_ht, minMax.x);
    max_ht = max(max_ht, minMax.y);
  }
  return true;
}

void dacoll::get_min_max_hmap_list_in_circle(const Point2 &center, float rad, Tab<Point2> &list)
{
  HeightmapHandler *hmap = get_lmesh() ? get_lmesh()->getHmapHandler() : nullptr;
  if (!hmap)
    return;

  float maxHt = hmap->getHeightMin();
  float minHt = hmap->getHeightMin() + hmap->getHeightScale();

  int gridSize = hmap->getMinMaxHtGridSize();
  float cellSz = hmap->getHeightmapCellSize();
  Point3 hmapOffs = hmap->getHeightmapOffset();
  int gridRad = ceil(ceilf(rad / cellSz) / gridSize);
  IPoint2 gridCenter = IPoint2(((center.x - hmapOffs.x) / cellSz) / gridSize, ((center.y - hmapOffs.z) / cellSz) / gridSize);
  // we cannot calculate it throught gridCenter/Rad, as it'll be too wide
  int region[4] = {int(((center.x - rad - hmapOffs.x) / cellSz) / gridSize), int(((center.y - rad - hmapOffs.z) / cellSz) / gridSize),
    int(((center.x + rad - hmapOffs.x) / cellSz) / gridSize), int(((center.y + rad - hmapOffs.z) / cellSz) / gridSize)};
  for (int y = region[1]; y <= region[3]; ++y)
    for (int x = region[0]; x <= region[2]; ++x)
    {
      if (sqr(x - gridCenter.x) + sqr(y - gridCenter.y) > sqr(gridRad))
        continue;
      Point2 minMaxHt(minHt, maxHt);
      if (hmap->getMinMaxHtInGrid(IPoint2(x, y), minMaxHt.x, minMaxHt.y))
        list.push_back() = minMaxHt;
    }
}

void dacoll::tracemultiray_lmesh(dag::Span<Trace> &traces)
{
  if (!get_lmesh())
    return;

  for (int i = 0; i < traces.size(); ++i)
  {
    Point3 fromPt = traces[i].pos;
    float xk, zk;
    first_mirroring_transform(fromPt.x, fromPt.z, xk, zk);
    float ht = fromPt.y - traces[i].pos.outT;
    float minht = ht - 1000.0f;

    if (get_lmesh()->getHeightBelow(fromPt, minht, &traces[i].outNorm))
    {
      if (minht >= ht)
      {
        traces[i].outNorm.x *= xk;
        traces[i].outNorm.z *= zk;
        traces[i].pos.outT = fromPt.y - minht;
        traces[i].outMatId = get_lmesh_mat_id();
      }
    }
    else if (get_lmesh()->getHeight(Point2::xz(fromPt), ht, &traces[i].outNorm))
    {
      traces[i].outNorm.x *= xk;
      traces[i].outNorm.z *= zk;
      traces[i].pos.outT = max(fromPt.y - ht, 0.0001f);
      traces[i].outMatId = get_lmesh_mat_id();
    }
  }
}

static bool get_heightmap_at_point(float x, float z, float &out_height)
{
  out_height = 0.f;

  if (dacoll::get_lmesh())
  {
    float xk, zk;
    first_mirroring_transform(x, z, xk, zk);

    if (dacoll::get_lmesh()->getHeight(Point2(x, z), out_height, NULL))
      return true;
    else
    {
      out_height = 0.f;
      return false;
    }
  }

  return false;
}

bool dacoll::is_valid_heightmap_pos(const Point2 &pos)
{
  float height = 0.f;
  return get_lmesh() && get_lmesh()->getHmapHandler() && get_lmesh()->getHmapHandler()->getHeight(pos, height, nullptr);
}

bool dacoll::is_valid_water_height(float height) { return height > invalid_water_height; }

float dacoll::get_water_height_2d(const Point3 &pos, float t, bool &underwater)
{
  float t2 = t * 2.f;
  Point3 startPos = pos + Point3(0.f, t, 0.f);
  if (dacoll::tracedown_water(startPos, t2, nullptr))
  {
    const float ht = startPos.y - t2;
    underwater = ht > pos.y;
    return ht;
  }
  underwater = false;
  return invalid_water_height;
}

static float traceht_water_at_time_internal(const Point3 &pos, float t, float time, bool &underwater, bool &is_fft_water_height_above)
{
  is_fft_water_height_above = false;
  if (dacoll::has_only_water2d())
    return dacoll::get_water_height_2d(pos, t, underwater);

  FFTWater *water = dacoll::get_water();
  if (water)
  {
    float toY = pos.y - t;
    if (min(pos.y, toY) < fft_water::get_max_level(water) + fft_water::get_max_wave_height(water))
    {
      float res = 1e9f;
      if (fft_water::getHeightAboveWaterAtTime(water, time, pos, res))
      {
        is_fft_water_height_above = true;
        return res;
      }
    }
  }
  return dacoll::get_water_height_2d(pos, t, underwater);
}


float dacoll::traceht_water_at_time(const Point3 &pos, float t, float time, bool &underwater, float minWaterCoastDist)
{
  bool isFftWaterHeightAbove;
  float waterDist = traceht_water_at_time_internal(pos, t, time, underwater, isFftWaterHeightAbove);
  if (!isFftWaterHeightAbove)
    return waterDist;
  underwater = waterDist < 0.f;
  float landHt;
  float curWaterHt = pos.y - waterDist;
  if (!get_heightmap_at_point(pos.x, pos.z, landHt))
    return curWaterHt;
  float waterLevel = fft_water::get_level(get_water());
  float coastFadeFactor = cvt(waterLevel - landHt, 0.0f, minWaterCoastDist, 0.0f, 1.0f);
  float resWaterHt = (curWaterHt - waterLevel) * sqr(coastFadeFactor) + waterLevel;
  return resWaterHt;
}


float dacoll::traceht_water_at_time_no_ground(const Point3 &pos, float t, float time, bool &underwater)
{
  bool isFftWaterHeightAbove;
  float res = traceht_water_at_time_internal(pos, t, time, underwater, isFftWaterHeightAbove);
  if (isFftWaterHeightAbove)
  {
    underwater = res < 0.f;
    return pos.y - res;
  }
  return res;
}

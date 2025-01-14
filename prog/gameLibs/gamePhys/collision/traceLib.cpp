// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/collisionLib.h>
#include <memory/dag_framemem.h>
#include <rendInst/rendInstCollision.h>
#include <landMesh/lmeshManager.h>
#include <heightmap/heightmapHandler.h>
#include <sceneRay/dag_sceneRay.h>
#include <gameMath/traceUtils.h>
#include <fftWater/fftWater.h>
#include <perfMon/dag_statDrv.h>
#include <gamePhys/collision/contactData.h>
#include <math/dag_mathUtils.h>
#include <gamePhys/collision/collisionCache.h>
#include <math/dag_traceRayTriangle.h>

#include "collisionGlobals.h"
#include <supp/dag_alloca.h>

static const float invalid_water_height = -1e10f;
static void first_mirroring_transform(float &in_out_x, float &in_out_z, float &out_x_k, float &out_z_k);


bool dacoll::traceray_normalized_frt(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm)
{
  if (!dacoll::get_frt())
    return false;

  int faceNo = dacoll::get_frt()->tracerayNormalized(p, dir, t);
  if (faceNo >= 0)
  {
    G_ASSERT(faceNo < get_pmid().size());

    if (out_pmid)
      *out_pmid = get_pmid()[faceNo];

    if (out_norm)
      *out_norm = dacoll::get_frt()->facebounds(faceNo).n;
  }
  return faceNo >= 0;
}

bool dacoll::traceray_normalized_lmesh(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm)
{
  if (!get_lmesh())
    return false;
  Point3 transPos(p);
  float xk, zk;
  first_mirroring_transform(transPos.x, transPos.z, xk, zk);
  Point3 transDir = Point3(dir.x * xk, dir.y, dir.z * zk);
  bool res = get_lmesh()->traceray(transPos, transDir, t, out_norm);
  if (res && out_pmid)
    *out_pmid = get_lmesh_mat_id_at_point(Point2::xz(transPos + transDir * t)); // reset mid
  if (res && out_norm)
  {
    out_norm->x *= xk;
    out_norm->z *= zk;
  }
  return res;
}

bool dacoll::rayhit_normalized_frt(const Point3 &p, const Point3 &dir, real t)
{
  return get_frt() ? get_frt()->rayhitNormalized(p, dir, t) : false;
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
  // we don't have proper rayhit with handle and ray mat support yet in rendinst lib, but we'll surely add it in future.
  // TODO: add rayhit with handles and ray mat to rensinst library
  rendinst::TraceFlags traceFlags = rendinst::TraceFlag::Destructible | rendinst::TraceFlag::Meshes | additional_trace_flags;
  if (handle)
  {
    bbox3f rayBox;
    trace_utils::prepare_traces_box(traceDataSlice, rayBox);
    bool res = try_use_trace_cache(rayBox, handle);
    trace_utils::draw_trace_handle_debug_cast_result(handle, traceDataSlice, res, false);
    if (!res)
      handle = nullptr;
  }
  return rendinst::traceRayRIGenNormalized(traceDataSlice, traceFlags, ray_mat_id, nullptr, handle, skip_riex_handle);
}

bool dacoll::rayhit_normalized(const Point3 &p, const Point3 &dir, real t, int flags, int ray_mat_id, const TraceMeshFaces *handle,
  rendinst::riex_handle_t skip_riex_handle)
{
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
  auto do_rayhit = [&]() {
#endif
    if (flags & (ETF_FRT))
      if (rayhit_normalized_frt(p, dir, t))
        return true;

    if (flags & (ETF_LMESH | ETF_HEIGHTMAP))
      if (rayhit_normalized_lmesh(p, dir, t))
        return true;

    if (flags & ETF_RI)
    {
      rendinst::TraceFlags additionalTraceFlags = {};
      if ((flags & ETF_RI_TREES) != 0)
        additionalTraceFlags |= rendinst::TraceFlag::Trees;
      if ((flags & ETF_RI_PHYS) != 0)
        additionalTraceFlags |= rendinst::TraceFlag::Phys;
      if (rayhit_normalized_ri(p, dir, t, additionalTraceFlags, ray_mat_id, handle, skip_riex_handle))
        return true;
    }
    return false;
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
  };
  if (handle)
  {
    TIME_PROFILE(rayhit_handle);
    return do_rayhit();
  }
  else
  {
    TIME_PROFILE(rayhit);
    return do_rayhit();
  }
#endif
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
  if (handle)
  {
    bbox3f rayBox;
    trace_utils::prepare_traces_box(traceDataSlice, rayBox);
    bool res = try_use_trace_cache(rayBox, handle);
    trace_utils::draw_trace_handle_debug_cast_result(handle, traceDataSlice, res, false);
    if (!res)
      handle = nullptr;
  }
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
  rendinst::RendInstDesc *out_desc, int ray_mat_id, const TraceMeshFaces *handle)
{
  return dacoll::traceray_normalized_coll_type(p, dir, t, out_pmid, out_norm, flags, out_desc, nullptr, ray_mat_id, handle);
}

bool dacoll::traceray_normalized_coll_type(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm, int flags,
  rendinst::RendInstDesc *out_desc, int *out_coll_type, int ray_mat_id, const TraceMeshFaces *handle)
{
  G_ASSERTF(!check_nan(p) && p.lengthSq() < 1e11f && !check_nan(dir) && !check_nan(t), "%@ %@ %f", p, dir, t);

  bool res = false;
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
  auto do_traceray = [&]() {
#endif
    if (flags & ETF_FRT)
    {
      bool trace_res = traceray_normalized_frt(p, dir, t, out_pmid, out_norm);
      if (trace_res && out_coll_type)
        *out_coll_type = ETF_FRT;
      res |= trace_res;
    }
    if (flags & (ETF_LMESH | ETF_HEIGHTMAP))
    {
      bool trace_res = traceray_normalized_lmesh(p, dir, t, out_pmid, out_norm);
      if (trace_res && out_coll_type)
        *out_coll_type = ETF_LMESH;
      if (trace_res && handle && out_pmid)
        *out_pmid = handle->matMapCache.getMatAt(Point2::xz(p));
      res |= trace_res;
    }
    if (flags & ETF_RI)
    {
      rendinst::TraceFlags additionalTraceFlags = {};
      if ((flags & ETF_RI_TREES) != 0)
        additionalTraceFlags |= rendinst::TraceFlag::Trees;
      if ((flags & ETF_RI_PHYS) != 0)
        additionalTraceFlags |= rendinst::TraceFlag::Phys;
      bool trace_res = traceray_normalized_ri(p, dir, t, out_pmid, out_norm, additionalTraceFlags, out_desc, ray_mat_id, handle);
      if (trace_res && out_coll_type)
        *out_coll_type = ETF_RI;
      res |= trace_res;
    }
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
  };
  if (handle)
  {
    TIME_PROFILE(traceray_handle);
    do_traceray();
  }
  else
  {
    TIME_PROFILE(traceray);
    do_traceray();
  }
#endif
  return res;
}

bool dacoll::traceray_normalized_multiray(dag::Span<Trace> traces, dag::Span<rendinst::RendInstDesc> ri_desc, int flags,
  int ray_mat_id, const TraceMeshFaces *handle)
{
  // TODO: add multiray where possible
  G_ASSERT(traces.size() == ri_desc.size());
  bool res = false;
  int i = 0;
  for (auto &trace : traces)
  {
    if (dacoll::traceray_normalized(trace.pos, trace.dir, trace.pos.outT, &trace.outMatId, &trace.outNorm, flags, &ri_desc[i],
          ray_mat_id, handle))
      res = true;
    else if (flags & ETF_RI)
      ri_desc[i].invalidate();
    i++;
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

bool dacoll::tracedown_normalized_multiray(dag::Span<Trace> traces, dag::Span<rendinst::RendInstDesc> ri_desc, int flags,
  int ray_mat_id, const TraceMeshFaces *handle)
{
  unsigned trcnt = traces.size();
  if (!trcnt)
    return false;

  if (!handle)
    return dacoll::traceray_normalized_multiray(traces, ri_desc, flags, ray_mat_id, handle);

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
    return dacoll::traceray_normalized_multiray(traces, ri_desc, flags, ray_mat_id);

  bool rendinstsValid = rendinst::checkCachedRiData(handle);

  bool res = false;

  res |= dacoll::tracedown_hmap_cache_multiray(traces, handle, reinterpret_cast<Point3_vec4 *>(rayStartPosVec),
    reinterpret_cast<Point3_vec4 *>(v_outNorm));
  res |= traceDownTrianglesMultiRay(handle->triangles.data(), handle->trianglesCount, // array of vert0, edge1, edge2
    rayStartPosVec, trcnt4a / 4, v_outNorm, tempSOA);

  trace_utils::store_traces(traces, dag::Span<vec4f>(rayStartPosVec, trcnt), dag::Span<vec4f>(v_outNorm, trcnt));
  for (int i = 0; i < trcnt; ++i)
  {
    int faceNo = v_extract_wi(v_cast_vec4i(v_outNorm[i]));
    if (faceNo < 0 || faceNo >= get_pmid().size())
    {
      traces[i].outMatId = handle->matMapCache.getMatAt(Point2::xz(traces[i].pos));
      continue;
    }

    traces[i].outMatId = get_pmid()[faceNo];
  }
  if (flags & ETF_RI)
  {
    if (rendinstsValid)
    {
      // In general traceDownMultiRay is a bit inconsistent with traceray_normalized_multiray,
      // it doesn't support TraceFlag::Trees and TraceFlag::Meshes, but we don't care for the
      // sake of perf.
      rendinst::TraceFlags traceFlags = rendinst::TraceFlag::Destructible;
      if ((flags & ETF_RI_PHYS) != 0)
        traceFlags |= rendinst::TraceFlag::Phys;
      res |= rendinst::traceDownMultiRay(traces, rayBox, ri_desc, handle, ray_mat_id, traceFlags);
    }
    else
    {
      TRACE_HANDLE_DEBUG_STAT(handle, numRIMisses++);
      trace_utils::draw_trace_handle_debug_cast_result(handle, traces, false, true);
    }
  }

  int fallbackFlags = flags & (handle->isLandmeshValid ? ~ETF_LMESH : 0xffffffff) & (handle->isStaticValid ? ~ETF_FRT : 0xffffffff) &
                      (rendinstsValid ? ~(ETF_RI | ETF_RI_TREES | ETF_RI_PHYS) : 0xffffffff) &
                      (handle->heightmap.width >= 0 ? ~ETF_HEIGHTMAP : 0xffffffff) &
                      (!handle->hasObjectGroups ? ~ETF_OBJECTS_GROUP : 0xffffffff);
  res |= dacoll::traceray_normalized_multiray(traces, ri_desc, fallbackFlags, ray_mat_id, handle);

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
  LandMeshHolesManager *holes = get_lmesh()->getHolesManager();
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

void dacoll::set_trace_game_objects_cb(trace_game_objects_cb cb) { trace_game_objs_cb = cb; }

bool dacoll::trace_game_objects(const Point3 &from, const Point3 &dir, float &t, Point3 &out_vel, int ignore_game_obj_id,
  int ray_mat_id)
{
  if (!trace_game_objs_cb)
    return false;
  return trace_game_objs_cb(from, dir, t, out_vel, ignore_game_obj_id, ray_mat_id);
}

bool dacoll::trace_sphere_cast_ex(const Point3 &from, const Point3 &to, float rad, int num_casts, dacoll::ShapeQueryOutput &out,
  int cast_mat_id, int ignore_game_obj_id, const TraceMeshFaces *handle, int flags)
{
  TIME_PROFILE_DEV(trace_sphere_cast);

  // simulate with 5 traces
  Point3 dir = to - from;
  float t = length(dir);
  if (t < 1e-5f)
    return false;

  dir *= safeinv(t);

  float len = t;
  t += rad;

  TMatrix transform = TMatrix::IDENT;
  transform.setcol(0, dir);
  if (fabsf(dir.y) > sqrt(2) * 0.5f) // more than 45 degrees up/down, choose another basis
    transform.setcol(2, normalize(dir % Point3(1.f, 0.f, 0.f)));
  else
    transform.setcol(2, normalize(dir % Point3(0.f, 1.f, 0.f)));

  transform.setcol(1, normalize(dir % transform.getcol(2)));

  int matId = -1;
  Point3 norm = {};
  bool res = false;

  out.norm.zero();
  out.vel.zero();
  out.res = to;
  if (dacoll::traceray_normalized(from, dir, t, &matId, &norm, flags, nullptr, cast_mat_id, handle))
  {
    out.norm = norm;
    out.res = from + dir * t;
    res = true;
  }
  if (dacoll::trace_game_objects(from, dir, t, out.vel, ignore_game_obj_id, cast_mat_id))
  {
    out.norm = -dir;
    out.res = from + dir * t;
    res = true;
  }

  const float invNumCasts = safeinv(float(num_casts));
  float minT = t;
  float minNorm = res ? out.norm * dir : 1.f;
  for (int i = 0; i < num_casts; ++i)
  {
    Point3 start = from + transform % Point3(0.f, cosf(TWOPI * invNumCasts * i), sinf(TWOPI * invNumCasts * i)) * rad;
    float outT = t;
    if (dacoll::traceray_normalized(start, dir, outT, &matId, &norm, flags, nullptr, cast_mat_id, handle))
    {
      if (outT < minT)
      {
        minT = outT;
        out.res = start + dir * outT;
        res = true;
      }
      float normProj = norm * dir;
      // check if this normal is better, or if we're out of reach of previous results
      if (normProj < minNorm || (t - outT > rad && outT - minT < rad))
      {
        out.norm = norm;
        minNorm = normProj;
      }
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
  if (get_lmesh() && get_lmesh()->getHmapHandler() &&
      (!get_lmesh()->getHolesManager() || !get_lmesh()->getHolesManager()->check(pos)) &&
      get_lmesh()->getHmapHandler()->getHeight(pos, height, nullptr))
    return height;
  return -1e5f;
}

bool dacoll::traceht_water(const Point3 &pos, float &t)
{
  FFTWater *water = get_water();
  if (!water)
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

float get_water_height_2d(const Point3 &pos, float t, bool &underwater)
{
  if (!dacoll::get_water_tracer())
  {
    underwater = false;
    return invalid_water_height;
  }
  float ht = invalid_water_height;
  float t2 = t * 2.f;
  Point3 startPos = pos + Point3(0.f, t, 0.f);
  int faceNo = dacoll::get_water_tracer()->traceDown(startPos, t2);
  if (faceNo >= 0)
  {
    ht = startPos.y - t2;
    underwater = ht > pos.y;
    return ht;
  }
  underwater = false;
  return ht;
}

static float traceht_water_at_time_internal(const Point3 &pos, float t, float time, bool &underwater, bool &is_fft_water_height_above)
{
  is_fft_water_height_above = false;
  if (dacoll::has_only_water2d())
    return get_water_height_2d(pos, t, underwater);

  FFTWater *water = dacoll::get_water();
  if (water)
  {
    const float waterLevel = fft_water::get_level(water);
    float toY = pos.y - t;
    if (min(pos.y, toY) < waterLevel + fft_water::get_max_wave(water))
    {
      float res = 1e9f;
      if (fft_water::getHeightAboveWaterAtTime(water, time, pos, res))
      {
        is_fft_water_height_above = true;
        return res;
      }
    }
  }
  return get_water_height_2d(pos, t, underwater);
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

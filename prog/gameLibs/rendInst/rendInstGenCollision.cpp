// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtraAccess.h>
#include "riGen/riGenData.h"
#include "riGen/genObjUtil.h"
#include "riGen/riGenExtra.h"
#include "riGen/riUtil.h"
#include "riGen/riRotationPalette.h"

#include <gameRes/dag_collisionResource.h>
#include <math/dag_mathUtils.h>
#include <math/dag_wooray2d.h>
#include <EASTL/fixed_vector.h>
#include <memory/dag_framemem.h>
#include <gameMath/traceUtils.h>
#include <util/dag_bitArray.h>

#define debug(...) logmessage(_MAKE4C('RGEN'), __VA_ARGS__)

namespace rendinst
{
static constexpr int SUBCELL_DIV = RendInstGenData::SUBCELL_DIV;

static float treeTrunkOpacity = 1.f;
static float treeCanopyOpacity = 0.5f;

#if !_TARGET_PC_TOOLS_BUILD
static uint32_t max_number_of_rendinst_collision_callbacks = 10000;
uint32_t setMaxNumRiCollisionCb(uint32_t n) { return eastl::exchange(max_number_of_rendinst_collision_callbacks, n); }
#else
static constexpr uint32_t max_number_of_rendinst_collision_callbacks = ~0u; // Unrestricted for tools (De3X, AV2)
uint32_t setMaxNumRiCollisionCb(uint32_t) { return max_number_of_rendinst_collision_callbacks; }
#endif

template <typename T>
static __forceinline T roll_bit(T bit)
{
  return (bit << 1) | (bit >> ((sizeof(bit) * CHAR_BIT) - 1));
}

#if DAGOR_DBGLEVEL > 0
static inline void verify_trace(const Trace &trace)
{
  vec3f vtpos = v_ld(&trace.pos.x);
  G_ASSERTF(!check_nan(v_extract_x(v_length3_sq_x(vtpos))) && v_test_vec_x_lt(v_length3_sq_x(vtpos), v_set_x(1e11f)) &&
              !check_nan(v_extract_x(v_length3_sq_x(v_ld(&trace.dir.x)))) && (trace.pos.outT >= 0.f && trace.pos.outT < 1e5f),
    "trace.{pos,dir,outT}=%@,%@,%f", trace.pos, trace.dir, trace.pos.outT);
  G_UNUSED(vtpos);
}
#else
static inline void verify_trace(const Trace &) {}
#endif

struct TraceRayStrat : public MaterialRayStrat
{
  TraceFlags traceFlags = {};
  uint8_t behaviorFlag = 0;

  TraceRayStrat(PhysMat::MatID ray_mat, TraceFlags trace_flags = TraceFlag::Destructible) :
    MaterialRayStrat(ray_mat, bool(trace_flags & TraceFlag::Trees)),
    traceFlags(trace_flags),
    behaviorFlag(trace_flags & TraceFlag::Phys ? CollisionNode::PHYS_COLLIDABLE : CollisionNode::TRACEABLE)
  {}

  bool isCheckBBoxAll() { return false; }

  bool executeForMesh(CollisionResource *coll_res, mat44f_cref tm, const Point3 &pos, const Point3 &dir, float &out_t,
    Point3 &out_norm, rendinst::RendInstDesc *ri_desc, bool &have_collision, int layer_idx, int idx, int pool, int offs,
    int &out_mat_id, int cell_idx)
  {
    bool meshCollision = coll_res->traceRay(tm, pos, dir, out_t, &out_norm, out_mat_id, rayMatId, behaviorFlag);
    if (meshCollision)
    {
      if (ri_desc)
      {
        ri_desc->layer = layer_idx;
        ri_desc->idx = idx;
        ri_desc->pool = pool;
        ri_desc->offs = offs;
      }
      if (out_mat_id == PHYSMAT_INVALID || out_mat_id == PHYSMAT_DEFAULT)
      {
        int poolRef = cell_idx == -1 ? rendinst::riExtra[pool].riPoolRef : pool;
        RendInstGenData *rgl = rendinst::getRgLayer(cell_idx == -1 ? rendinst::riExtra[pool].riPoolRefLayer : layer_idx);
        if (poolRef >= 0 && rgl)
          out_mat_id = rgl->rtData->riProperties[poolRef].matId;
      }
    }
    have_collision |= meshCollision;
    return false;
  }

  bool executeForPos(CollisionResource *coll_res, mat44f_cref tm, const BBox3 &, const Point3 &pos, const Point3 &dir, float &out_t,
    Point3 &out_norm, rendinst::RendInstDesc *ri_desc, bool &have_collision, int layer_idx, int idx, int pool, int offs,
    int &out_mat_id, int cell_idx, const BBox3 & /*bbox_all*/)
  {
    if (!processPosRI)
      return false;
    bool meshCollision = coll_res->traceRay(tm, pos, dir, out_t, &out_norm, out_mat_id, rayMatId, behaviorFlag);
    if (meshCollision)
    {
      if (ri_desc)
      {
        ri_desc->layer = layer_idx;
        ri_desc->idx = idx;
        ri_desc->pool = pool;
        ri_desc->offs = offs;
      }
      if (out_mat_id == PHYSMAT_INVALID || out_mat_id == PHYSMAT_DEFAULT)
      {
        int poolRef = cell_idx == -1 ? rendinst::riExtra[pool].riPoolRef : pool;
        RendInstGenData *rgl = rendinst::getRgLayer(cell_idx == -1 ? rendinst::riExtra[pool].riPoolRefLayer : layer_idx);
        if (poolRef >= 0 && rgl)
          out_mat_id = rgl->rtData->riProperties[poolRef].matId;
      }
    }
    have_collision |= meshCollision;
    return false;
  }

  bool executeForCell(bool /*cell_result*/)
  {
    return false; // do not exit on first cell
  }

  // Like MaterialRayStrat::shouldIgnoreRendinst() but with additional check for immortal/processDestrtuctibleRI
  bool shouldIgnoreRendinst(bool is_pos, bool is_immortal, PhysMat::MatID mat_id) const
  {
    if (is_pos && !processPosRI)
      return true;

    if (!(traceFlags & TraceFlag::Destructible) && !is_immortal)
      return true;

    if (rayMatId == PHYSMAT_INVALID)
      return false;

    return !PhysMat::isMaterialsCollide(rayMatId, mat_id);
  }
};

struct TraceRayListStrat : public TraceRayStrat
{
  RendInstsIntersectionsList list;

  TraceRayListStrat(PhysMat::MatID ray_mat) : TraceRayStrat(ray_mat) {}

  bool executeForMesh(CollisionResource *coll_res, mat44f_cref tm, const Point3 &pos, const Point3 &dir, float &out_t,
    Point3 &out_norm, rendinst::RendInstDesc *ri_desc, bool &have_collision, int layer_idx, int idx, int pool, int offs,
    int &out_mat_id, int cell_idx)
  {
    float dist = out_t;
    bool meshCollision = coll_res->traceRay(tm, pos, dir, out_t, &out_norm, out_mat_id);
    if (meshCollision)
    {
      rendinst::TraceRayRendInstData &data = list.push_back();
      data.riDesc.reset();
      data.riDesc.layer = layer_idx;
      data.riDesc.idx = idx;
      data.riDesc.pool = pool;
      data.riDesc.offs = offs;
      data.riDesc.cellIdx = cell_idx;
      data.normIn = out_norm;
      data.tIn = out_t;
      if (ri_desc)
      {
        ri_desc->layer = layer_idx;
        ri_desc->idx = idx;
        ri_desc->pool = pool;
        ri_desc->offs = offs;
      }
      if (out_mat_id == PHYSMAT_INVALID || out_mat_id == PHYSMAT_DEFAULT)
      {
        int poolRef = cell_idx == -1 ? rendinst::riExtra[pool].riPoolRef : pool;
        RendInstGenData *rgl = rendinst::getRgLayer(cell_idx == -1 ? rendinst::riExtra[pool].riPoolRefLayer : layer_idx);
        if (poolRef >= 0 && rgl)
          out_mat_id = rgl->rtData->riProperties[poolRef].matId;
      }
      data.matId = out_mat_id;
    }
    out_t = dist;
    have_collision |= meshCollision;
    return false;
  }
};

template <typename ListT>
struct TraceRayAllUnsortedListStrat : public TraceRayStrat
{
  ListT &list;
  CollResIntersectionsType intersectedNodes;
  float extendRay = 0.0f;

  TraceRayAllUnsortedListStrat(PhysMat::MatID ray_mat, ListT &list) : TraceRayStrat(ray_mat), list(list) {}

  bool executeForMesh(CollisionResource *coll_res, mat44f_cref tm, const Point3 &pos, const Point3 &dir, float &in_out_t,
    Point3 & /* out_norm */, rendinst::RendInstDesc * /* ri_desc */, bool &have_collision, int layer_idx, int idx, int pool, int offs,
    int &out_mat_id, int cell_idx)
  {
    if (coll_res->traceRay(tm,
          /* geom_node_tree */ nullptr, pos, dir, /* in_t */ in_out_t + extendRay, intersectedNodes,
          /* sort */ true, /* behavior_filter */ CollisionNode::TRACEABLE,
          /* collision_node_mask */ nullptr, /* no_cull */ true))
    {
      have_collision = true;

      int defPhysMatId = -1;
      const int poolRef = cell_idx == -1 ? rendinst::riExtra[pool].riPoolRef : pool;
      const RendInstGenData *rgl = rendinst::getRgLayer(cell_idx == -1 ? rendinst::riExtra[pool].riPoolRefLayer : layer_idx);
      if (poolRef >= 0 && rgl != nullptr)
        out_mat_id = defPhysMatId = rgl->rtData->riProperties[poolRef].matId;

      for (const IntersectedNode &node : intersectedNodes)
      {
        int physMatId = coll_res->getNode(node.collisionNodeId)->physMatId;
        if (physMatId == PHYSMAT_INVALID || physMatId == PHYSMAT_DEFAULT)
          physMatId = defPhysMatId;
        out_mat_id = physMatId;

        typename ListT::value_type &data = list.push_back();
        data.riDesc.reset();
        data.riDesc.layer = layer_idx;
        data.riDesc.idx = idx;
        data.riDesc.pool = pool;
        data.riDesc.offs = offs;
        data.riDesc.cellIdx = cell_idx;
        data.normIn = node.normal;
        data.collisionNodeId = node.collisionNodeId;
        data.tIn = node.intersectionT;
        data.matId = physMatId;
        if constexpr (eastl::is_same_v<typename ListT::value_type, TraceRayRendInstSolidData>)
        {
          data.tOut = data.tIn;
          data.normOut = data.normIn;
        }
        // in_out_t = eastl::max(in_out_t, node.intersectionT);
      }
    }
    return false;
  }
};

struct RayHitStrat : public MaterialRayStrat
{
  RayHitStrat(PhysMat::MatID ray_mat) : MaterialRayStrat(ray_mat) {}

  bool isCheckBBoxAll() { return false; }

  bool executeForMesh(CollisionResource *coll_res, mat44f_cref tm, const Point3 &pos, const Point3 &dir, float in_t,
    Point3 & /*out_norm*/, rendinst::RendInstDesc *ri_desc, bool &have_collision, int layer_idx, int idx, int pool, int offs,
    int &out_mat_id, int /*cell_idx*/)
  {
    if (coll_res->rayHit(tm, pos, dir, in_t, rayMatId, out_mat_id))
    {
      if (ri_desc)
      {
        ri_desc->layer = layer_idx;
        ri_desc->idx = idx;
        ri_desc->pool = pool;
        ri_desc->offs = offs;
      }
      have_collision = true;
      return true;
    }
    return false;
  }

  bool executeForPos(CollisionResource *coll_res, mat44f_cref tm, const BBox3 & /*box*/, const Point3 &pos, const Point3 &dir,
    float in_t, Point3 & /*out_norm*/, rendinst::RendInstDesc *ri_desc, bool &have_collision, int layer_idx, int idx, int pool,
    int offs, int &out_mat_id, int /*cell_idx*/, const BBox3 & /*bbox_all*/)
  {
    if (coll_res->rayHit(tm, pos, dir, in_t, rayMatId, out_mat_id))
    {
      if (ri_desc)
      {
        ri_desc->layer = layer_idx;
        ri_desc->idx = idx;
        ri_desc->pool = pool;
        ri_desc->offs = offs;
      }
      have_collision = true;
      return true;
    }
    return false;
  }

  bool executeForCell(bool cell_result)
  {
    return cell_result; // exit of first hit cell
  }
};


static int does_line_intersect_vert_cone_two_points(float r, float h, Point3 vertex, Point3 line_start, Point3 line_end,
  float (&roots)[2], bool (&valid)[2])
{
  if (r < FLT_EPSILON || fabs(h) < FLT_EPSILON) // negative height means the cone vertex is below
    return 0;

  const Point3 diff = line_end - line_start;
  const float rSqrInv = 1 / sqr(r);
  const float hSqrInv = 1 / sqr(h);

  const Point3 p = line_start - vertex;

  const double A = (sqr(diff.x) + sqr(diff.z)) * rSqrInv - sqr(diff.y) * hSqrInv;
  const double B = 2 * ((p.x * diff.x + p.z * diff.z) * rSqrInv - p.y * diff.y * hSqrInv);
  const double C = (sqr(p.x) + sqr(p.z)) * rSqrInv - sqr(p.y) * hSqrInv;
  const int rootCount = solve_square_equation(A, B, C, roots);
  int intersectionCount = 0;
  valid[0] = valid[1] = false;

  for (int i = 0; i < rootCount; ++i)
  {
    const float root = roots[i];

    if (root < 0 || root > 1)
      continue;

    const float intersectionY = sign(h) * (p.y + root * diff.y);
    if (intersectionY > 0 || intersectionY < -h)
      continue;

    valid[i] = true;
    ++intersectionCount;
  }

  return intersectionCount;
}

struct RayTransparencyStrat
{
  PhysMat::MatID rayMatId = PHYSMAT_INVALID;
  float accumulatedTransparency = 0.0f;
  const float transparencyThreshold;
  const bool checkCanopy;
  float secondLayerMinCapsuleHeightSq = FLT_MAX;

  RayTransparencyStrat(float transparency_threshold, bool check_canopy, PhysMat::MatID ray_mat) :
    rayMatId(ray_mat), checkCanopy(check_canopy), transparencyThreshold(transparency_threshold)
  {}

  bool isCheckBBoxAll() { return transparencyThreshold > 0 && accumulatedTransparency < transparencyThreshold; }

  bool executeForMesh(CollisionResource * /*coll_res*/, mat44f_cref /*tm*/, const Point3 & /*pos*/, const Point3 & /*dir*/,
    float & /*out_t*/, Point3 & /*out_norm*/, rendinst::RendInstDesc * /*ri_desc*/, bool & /*have_collision*/, int /*layer_idx*/,
    int /*idx*/, int /*pool*/, int /*offs*/, int & /*out_mat_id*/, int /*cell_idx*/)
  {
    return false;
  }

  bool executeForPos(CollisionResource *coll_res, mat44f_cref tm, const BBox3 &box_collision, const Point3 &pos, const Point3 &dir,
    float &out_t, Point3 & /*out_norm*/, rendinst::RendInstDesc *ri_desc, bool &have_collision, int layer_idx, int idx, int pool,
    int offs, int &out_mat_id, int cell_idx, const BBox3 &bbox_all)
  {
    RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
    const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[pool];
    float at = 1.f;
    bool traceToCanopy = checkCanopy && riProp.canopyOpacity > 0.f;
    if (does_line_intersect_box(box_collision, pos, pos + dir * out_t, at))
    {
      // check coll resource for more then 1 capsule
      bool traceToCollRes =
        coll_res->boxNodesHead || coll_res->meshNodesHead || (coll_res->capsuleNodesHead && coll_res->capsuleNodesHead->nextNode);
      bool haveCollision = !traceToCollRes;
      if (!traceToCollRes && layer_idx >= rendinst::rgPrimaryLayers)
        haveCollision =
          coll_res->capsuleNodesHead &&
          secondLayerMinCapsuleHeightSq < (coll_res->capsuleNodesHead->capsule.a - coll_res->capsuleNodesHead->capsule.b).lengthSq();
      if (traceToCollRes)
      {
        int outMatId;
        haveCollision = coll_res->traceRay(tm, pos, dir, out_t, nullptr /*norm*/, outMatId);
      }
      if (haveCollision)
      {
        accumulatedTransparency += treeTrunkOpacity;
        traceToCanopy = false;
      }
    }
    if (traceToCanopy && !bbox_all.isempty())
    {
      float exitAt = 0.f;

      BBox3 canopyBox; // synthetical tree canopy box
      getRIGenCanopyBBox(riProp, bbox_all, canopyBox);

      if (riProp.canopyTriangle)
      {
        Point3 canopyBoxCenter = canopyBox.center();
        float canopyHeight = canopyBox.boxMax().y - canopyBox.boxMin().y;
        Point3 canopyVertex = {canopyBoxCenter.x, canopyBox.boxMax().y, canopyBoxCenter.z};
        float ats[2] = {0};
        bool valid[2];
        float canopyWidth = canopyBox.width().x * 0.5f;
        int intersectionCount = does_line_intersect_vert_cone_two_points(canopyWidth, canopyHeight, canopyVertex, pos,
          pos + dir * out_t, ats, valid); // note: if intersectionCount = 2 then ats[0] > ats[1]
        if (intersectionCount > 0)
        {
          at = valid[0] ? ats[0] : ats[1];
          float dist = out_t * (intersectionCount > 1 ? ats[0] - ats[1] : (valid[0] ? ats[0] : 1 - ats[1]));
          float opacity = dist * out_t / (2 * canopyBox.width().length()) * riProp.canopyOpacity;
          accumulatedTransparency += opacity * treeCanopyOpacity;
        }
      }
      else if (does_line_intersect_box_side_two_points(canopyBox, pos, pos + dir * out_t, at, exitAt) >= 0)
      {
        bool isStartInside = false;
        if (exitAt < at + 1.0e-6f)
        {
          if (canopyBox & pos)
          {
            isStartInside = true;
            at = 0.0f; // starting point is inside
          }
          else
            exitAt = 1.0f; // final point is inside
        }
        Point3 absDir = Point3(fabsf(dir.x), fabsf(dir.y), fabsf(dir.z));
        float opacity = (exitAt - at) * out_t / (absDir * canopyBox.width()) * riProp.canopyOpacity;
        accumulatedTransparency += opacity * treeCanopyOpacity;
        if (isStartInside)
          at = exitAt; // in this case we want to get the distance to the canopy's border
      }
    }
    if (accumulatedTransparency >= transparencyThreshold)
    {
      have_collision = true;
      out_t *= at;
      if (ri_desc)
      {
        ri_desc->idx = idx;
        ri_desc->pool = pool;
        ri_desc->offs = offs;
        ri_desc->layer = layer_idx;
      }
      if (out_mat_id == PHYSMAT_INVALID || out_mat_id == PHYSMAT_DEFAULT)
      {
        int poolRef = cell_idx == -1 ? rendinst::riExtra[pool].riPoolRef : pool;
        RendInstGenData *rgl = rendinst::getRgLayer(cell_idx == -1 ? rendinst::riExtra[pool].riPoolRefLayer : layer_idx);
        if (poolRef >= 0 && rgl)
          out_mat_id = rgl->rtData->riProperties[poolRef].matId;
      }
    }
    G_ASSERTF(at >= 0.f && at <= 1.001f, "Get full dump please (at=%f)", at);
    return accumulatedTransparency >= transparencyThreshold;
  }

  bool executeForCell(bool cell_result)
  {
    return cell_result; // exit of first hit cell
  }

  // ignores non-pos rendinst and works for pos rendinst (trees)
  bool shouldIgnoreRendinst(bool is_pos, bool /* is_immortal */, PhysMat::MatID mat_id) const
  {
    if (rayMatId != PHYSMAT_INVALID && !PhysMat::isMaterialsCollide(rayMatId, mat_id))
      return true;

    return !is_pos;
  }
};

struct RaySoundOcclusionStrat
{
  const PhysMat::MatID rayMatId;
  float accumulatedOcclusion = 0.f;
  float maxOcclusion = 0.f;

  RaySoundOcclusionStrat(PhysMat::MatID ray_mat) : rayMatId(ray_mat) {}

  bool isCheckBBoxAll() { return accumulatedOcclusion < 1.f; }

  bool executeForMesh(CollisionResource *coll_res, mat44f_cref tm, const Point3 &pos, const Point3 &dir, float &out_t, //-V669
    Point3 & /*out_norm*/, rendinst::RendInstDesc * /*ri_desc*/, bool &have_collision, int layer_idx, int /*idx*/, int pool,
    int /*offs*/, int &out_mat_id, int cell_idx)
  {
    const RendInstGenData *rgl = rendinst::getRgLayer(cell_idx == -1 ? rendinst::riExtra[pool].riPoolRefLayer : layer_idx);
    const int poolRef = cell_idx == -1 ? rendinst::riExtra[pool].riPoolRef : pool;
    if (!rgl || poolRef < 0)
      return false;

    const auto &riProperties = rgl->rtData->riProperties[poolRef];
    if (riProperties.soundOcclusion <= 0.f)
      return false;

    constexpr auto behaviorFlag = CollisionNode::PHYS_COLLIDABLE;
    if (coll_res->rayHit(tm, pos, dir, out_t, rayMatId, out_mat_id, behaviorFlag))
    {
      accumulatedOcclusion += riProperties.soundOcclusion;
      maxOcclusion = max(maxOcclusion, riProperties.soundOcclusion);
      have_collision = true;
    }
    return accumulatedOcclusion >= 1.f;
  }

  bool executeForPos(CollisionResource *coll_res, mat44f_cref tm, const BBox3 &, const Point3 &pos, const Point3 &dir, //-V669
    float &out_t,                                                                                                      //-V669
    Point3 & /*out_norm*/, rendinst::RendInstDesc * /*ri_desc*/, bool &have_collision, int layer_idx, int /*idx*/, int pool,
    int /*offs*/, int &out_mat_id, int cell_idx, const BBox3 & /*bbox_all*/)
  {
    const RendInstGenData *rgl = rendinst::getRgLayer(cell_idx == -1 ? rendinst::riExtra[pool].riPoolRefLayer : layer_idx);
    const int poolRef = cell_idx == -1 ? rendinst::riExtra[pool].riPoolRef : pool;
    if (!rgl || poolRef < 0)
      return false;

    const auto &riProperties = rgl->rtData->riProperties[poolRef];
    if (riProperties.soundOcclusion <= 0.f)
      return false;

    constexpr auto behaviorFlag = CollisionNode::PHYS_COLLIDABLE;
    if (coll_res->rayHit(tm, pos, dir, out_t, rayMatId, out_mat_id, behaviorFlag))
    {
      accumulatedOcclusion += riProperties.soundOcclusion;
      maxOcclusion = max(maxOcclusion, riProperties.soundOcclusion);
      have_collision = true;
    }
    return accumulatedOcclusion >= 1.f;
  }

  bool executeForCell(bool /*cell_result*/) { return accumulatedOcclusion >= 1.f; }

  bool shouldIgnoreRendinst(bool /*is_pos*/, bool /* is_immortal */, PhysMat::MatID mat_id) const
  {
    if (rayMatId != PHYSMAT_INVALID && !PhysMat::isMaterialsCollide(rayMatId, mat_id))
      return true;
    return false;
  }
};

template <typename Strategy>
static bool traverseRayCell(RendInstGenData::Cell &cell, bbox3f_cref rayBox, dag::Span<Trace> traces, bool /*trace_meshes*/,
  int layer_idx, rendinst::RendInstDesc *ri_desc, Strategy &strategy, int cell_idx)
{
  const RendInstGenData::CellRtData *crt_ptr = cell.isReady();
  if (DAGOR_UNLIKELY(!crt_ptr))
    return false;
  const RendInstGenData::CellRtData &crt = *crt_ptr;

  if (DAGOR_UNLIKELY(!v_bbox3_test_box_intersect(crt.bbox[0], rayBox)))
    return false;

  static const IBBox2 cell_leaf_lim(IPoint2(0, 0), IPoint2(SUBCELL_DIV - 1, SUBCELL_DIV - 1));
  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  float cellSz = rgl->grid2world * rgl->cellSz;
  float subcellSz = cellSz / SUBCELL_DIV;

  int pcnt = crt.pools.size();
  float cell_x0 = v_extract_x(crt.cellOrigin), cell_z0 = v_extract_z(crt.cellOrigin);
  vec3f v_cell_add = crt.cellOrigin;
  vec3f v_cell_mul = v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(cellSz, crt.cellHeight, cellSz, 0));

  carray<uint16_t, SUBCELL_DIV * SUBCELL_DIV> subcellIndices;
  // carray<uint64_t, SUBCELL_DIV*SUBCELL_DIV> subcellIndicesTraces(framemem_ptr());// if we want to mask which traces in which
  // subCell. However MultiRay are CLOSE and subcells are BIG G_ASSERT(traces.size()<=64);
  int64_t subcellIndicesCnt = 0;
  uint64_t alreadyHas = 0;
  G_STATIC_ASSERT(is_pow2(SUBCELL_DIV));
  G_STATIC_ASSERT(SUBCELL_DIV * SUBCELL_DIV <= 64);
  bool haveCollision = false;
  const int tracesCnt = traces.size();
  for (int i = 0; i < tracesCnt; ++i)
  {
    vec3f rayStart = v_ldu(&traces[i].pos.x);
    vec3f rayDir = v_ldu(&traces[i].dir.x);
    if (!v_test_ray_box_intersection(rayStart, rayDir, v_set_x(traces[i].pos.outT), crt.bbox[0]))
      continue;

    WooRay2d ray(Point2(traces[i].pos.x - cell_x0, traces[i].pos.z - cell_z0), Point2::xz(traces[i].dir), traces[i].pos.outT,
      Point2(subcellSz, subcellSz), cell_leaf_lim);
    double nextT = 0;
    bool cellIntersected = false;
    do
    {
      uint64_t outOfCellMask = uint32_t(~(SUBCELL_DIV - 1));
      outOfCellMask |= outOfCellMask << 32;
      uint64_t xy = uint64_t(ray.currentCell().x) | (uint64_t(ray.currentCell().y) << 32);
      if (DAGOR_LIKELY((xy & outOfCellMask) == 0))
      {
        cellIntersected = true;
        int idx = ray.currentCell().y * SUBCELL_DIV + ray.currentCell().x;
        uint64_t mask = 1ULL << idx;
        if (alreadyHas & mask)
          continue;

        if (v_test_ray_box_intersection(rayStart, rayDir, v_set_x(traces[i].pos.outT), crt.bbox[idx + 1]))
        {
          subcellIndices[subcellIndicesCnt] = idx;
          alreadyHas |= mask;
          subcellIndicesCnt++;
        }
      }
      else if (cellIntersected) // ray tail is out of cell
        break;
    } while (ray.nextCell(nextT));
  }
  const eastl::BitvectorWordType *riPosInstData = rgl->rtData->riPosInst.data();
  const eastl::BitvectorWordType *riPaletteRotationData = rgl->rtData->riPaletteRotation.data();
  for (int i = 0; i < subcellIndicesCnt; ++i)
  {
    int idx = subcellIndices[i];
    eastl::BitvectorWordType riPosInstBit = 1;
    for (int p = 0; DAGOR_LIKELY(p < pcnt); p++, riPosInstBit = roll_bit(riPosInstBit))
    {
      const RendInstGenData::CellRtData::SubCellSlice &scs = crt.getCellSlice(p, idx);
      if (DAGOR_LIKELY(!scs.sz)) // very hot place, hits is ~2% of 1000+
        continue;
      v_prefetch(crt.sysMemData.get() + scs.ofs);
      CollisionResource *collRes = rgl->rtData->riCollRes[p].collRes;
      if (DAGOR_UNLIKELY(!collRes))
        continue;
      bool isPosInst = (riPosInstData[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0;
      const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[p];
      if (strategy.shouldIgnoreRendinst(isPosInst, riProp.immortal, riProp.matId))
        continue;
      if (DAGOR_UNLIKELY(!isPosInst))
      {
        const int16_t *data_s = (int16_t *)(crt.sysMemData.get() + scs.ofs);
        int stride_w =
          RIGEN_TM_STRIDE_B((rgl->rtData->riZeroInstSeeds.data()[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0,
            rgl->perInstDataDwords) /
          2;
        for (const int16_t *__restrict data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
        {
          mat44f tm;
          if (!rendinst::gen::unpack_tm_full(tm, data, v_cell_add, v_cell_mul))
            continue;
#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS // ignore riZeroInstSeeds here since we use this code for tools only
          if (is_rendinst_marked_collision_ignored(data, rgl->perInstDataDwords, RIGEN_TM_STRIDE_B(false, 0) / 2))
            continue;
#endif

          bbox3f transformedBox;
          v_bbox3_init(transformedBox, tm, collRes->vFullBBox);
          // globalBoxCollided = (tm * collRes->boundingBox) & _objWorldBB;
          if (!v_bbox3_test_box_intersect(transformedBox, rayBox))
            continue;
          for (int rayId = 0; rayId < tracesCnt; ++rayId)
          {
            bool stop =
              strategy.executeForMesh(collRes, tm, traces[rayId].pos, traces[rayId].dir, traces[rayId].pos.outT, traces[rayId].outNorm,
                ri_desc, haveCollision, layer_idx, idx, p, int(intptr_t(data) - intptr_t(data_s)), traces[rayId].outMatId, cell_idx);
            if (stop)
              return haveCollision;
          }
        }
      }
      else if (bool paletteRotation = (riPaletteRotationData[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0)
      {
        bool checkBBoxAll = riProp.canopyOpacity > 0.f && strategy.isCheckBBoxAll();
        rendinst::gen::RotationPaletteManager::Palette palette =
          rendinst::gen::get_rotation_palette_manager()->getPalette({layer_idx, p});
        vec4f posBoundingRad = v_add_x(v_length3_x(collRes->vBoundingSphere), v_set_x(collRes->getBoundingSphereRad()));

        const int16_t *data_s = (int16_t *)(crt.sysMemData.get() + scs.ofs);
        int stride_w =
          RIGEN_POS_STRIDE_B((riZeroInstSeeds[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0, rgl->perInstDataDwords) / 2;
        for (const int16_t *__restrict data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
        {
          vec3f v_pos, v_scale;
          vec4i paletteId;
          if (!rendinst::gen::unpack_tm_pos(v_pos, v_scale, data, v_cell_add, v_cell_mul, paletteRotation, &paletteId))
            continue;

          if (!checkBBoxAll)
          {
            // unprecise, but fast and it filter most of instances
            vec4f posBoundingRadSq = v_sqr_x(v_mul_x(posBoundingRad, v_hmax3(v_scale)));
            if (tracesCnt == 1)
            {
              vec3f rayStart = v_ldu(&traces[0].pos.x);
              vec3f rayDir = v_ldu(&traces[0].dir.x);
              vec3f rayLen = v_splats(traces[0].pos.outT);
              if (DAGOR_LIKELY(!v_test_ray_sphere_intersection(rayStart, rayDir, rayLen, v_pos, posBoundingRadSq)))
                continue;
            }
            else if (!v_bbox3_test_sph_intersect(rayBox, v_pos, posBoundingRadSq)) // usually trace down, so we can use rayBox here
              continue;
          }

          quat4f v_rot = rendinst::gen::RotationPaletteManager::get_quat(palette, v_extract_xi(paletteId));
          mat44f tm;
          v_mat44_compose(tm, v_pos, v_rot, v_scale);

          bbox3f riBBox;
          v_bbox3_init(riBBox, tm, collRes->vFullBBox);
          BBox3 riWorldCollisionBox, treeWithCanopyWorldCollisionBox;
          v_stu_bbox3(riWorldCollisionBox, riBBox);
          if (checkBBoxAll)
          {
            bbox3f allBBox;
            v_bbox3_init(allBBox, tm, rgl->rtData->riResBb[p]);
            v_stu_bbox3(treeWithCanopyWorldCollisionBox, allBBox);

            if (traces.size() == 1)
            {
              vec3f rayStart = v_ldu(&traces[0].pos.x);
              vec3f rayDir = v_ldu(&traces[0].dir.x);
              vec3f rayLen = v_splats(traces[0].pos.outT);
              if (DAGOR_LIKELY(!v_test_ray_box_intersection(rayStart, rayDir, rayLen, allBBox)))
                continue;
            }
            else if (DAGOR_LIKELY(!v_bbox3_test_box_intersect(allBBox, rayBox)))
              continue;
          }

          for (int rayId = 0; rayId < tracesCnt; ++rayId)
          {
            bool stop = strategy.executeForPos(collRes, tm, riWorldCollisionBox, traces[rayId].pos, traces[rayId].dir,
              traces[rayId].pos.outT, traces[rayId].outNorm, ri_desc, haveCollision, layer_idx, idx, p,
              int(intptr_t(data) - intptr_t(data_s)), traces[rayId].outMatId, cell_idx, treeWithCanopyWorldCollisionBox);
            if (stop)
              return haveCollision;
          }
        }
      }
      else
      {
        vec3f v_tree_min = collRes->vFullBBox.bmin;
        vec3f v_tree_max = collRes->vFullBBox.bmax;

        const int16_t *data_s = (int16_t *)(crt.sysMemData.get() + scs.ofs);
        int stride_w =
          RIGEN_POS_STRIDE_B((riZeroInstSeeds[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0, rgl->perInstDataDwords) / 2;
        for (const int16_t *data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
        {
          vec3f v_pos, v_scale;
          if (!rendinst::gen::unpack_tm_pos(v_pos, v_scale, data, v_cell_add, v_cell_mul, false /*paletteRotation*/))
            continue;

          bbox3f treeBBox;
          treeBBox.bmin = v_add(v_pos, v_mul(v_scale, v_tree_min));
          treeBBox.bmax = v_add(v_pos, v_mul(v_scale, v_tree_max));

          bool isIntersec = v_bbox3_test_box_intersect(treeBBox, rayBox);
          bool checkBBoxAll = riProp.canopyOpacity > 0.f && strategy.isCheckBBoxAll();

          BBox3 worldBoxCollision;
          BBox3 worldBoxAll;
          if (isIntersec || checkBBoxAll)
          {
            v_stu_bbox3(worldBoxCollision, treeBBox);
            if (checkBBoxAll)
            {
              bbox3f &box = rgl->rtData->riResBb[p];
              treeBBox.bmin = v_add(v_pos, v_mul(v_scale, box.bmin));
              treeBBox.bmax = v_add(v_pos, v_mul(v_scale, box.bmax));
              isIntersec = isIntersec || v_bbox3_test_box_intersect(treeBBox, rayBox);
              if (!isIntersec)
                continue;
              v_stu_bbox3(worldBoxAll, treeBBox);
            }
          }
          else
            continue;

          mat44f tm;
          v_mat44_compose(tm, v_pos, V_C_UNIT_0001, v_scale);
          for (int rayId = 0; rayId < tracesCnt; ++rayId)
          {
            bool stop = strategy.executeForPos(collRes, tm, worldBoxCollision, traces[rayId].pos, traces[rayId].dir,
              traces[rayId].pos.outT, traces[rayId].outNorm, ri_desc, haveCollision, layer_idx, idx, p,
              int(intptr_t(data) - intptr_t(data_s)), traces[rayId].outMatId, cell_idx, worldBoxAll);
            if (stop)
              return haveCollision;
          }
        }
      }
    }
  }
  return haveCollision;
}

extern void getRIGenExtra44NoLock(riex_handle_t id, mat44f &out_tm);

template <typename Strategy, bool CHK_BOX>
bool rayTestRiExtraInstanceBase(dag::Span<Trace> traces, bbox3f_cref ray_box, bbox3f_cref coll_res_bbox,
  rendinst::riex_handle_t handle, mat44f_cref tm, CollisionResource *res, bool &have_collision, Strategy &strategy,
  dag::Span<rendinst::RendInstDesc> out_ri_desc)
{
  if (CHK_BOX)
  {
    bbox3f transformedFullBox;
    v_bbox3_init(transformedFullBox, tm, coll_res_bbox);
    if (!v_bbox3_test_box_intersect(transformedFullBox, ray_box))
      return false;
  }

  for (int rayId = 0; rayId < traces.size(); ++rayId)
  {
    uint32_t pool = rendinst::handle_to_ri_type(handle);
    int layer = pool < rendinst::riex_max_type() ? rendinst::riExtra[pool].riPoolRefLayer : -1;
    bool haveCollision = false;
    bool shouldExit =
      strategy.executeForMesh(res, tm, traces[rayId].pos, traces[rayId].dir, traces[rayId].pos.outT, traces[rayId].outNorm,
        safe_at(out_ri_desc, rayId), haveCollision, layer, rendinst::handle_to_ri_inst(handle), pool, 0, traces[rayId].outMatId, -1);
    have_collision |= haveCollision;
    if (haveCollision && rayId < out_ri_desc.size())
      out_ri_desc[rayId].setRiExtra();
    if (shouldExit)
      return true;
  }
  return false;
}


template <typename Strategy, bool CHK_BOX, bool READ_LOCK = true>
bool rayTestRiExtraInstance(dag::Span<Trace> traces, bbox3f_cref ray_box, bbox3f_cref coll_res_bbox, rendinst::riex_handle_t handle,
  CollisionResource *res, bool &have_collision, Strategy &strategy, dag::Span<rendinst::RendInstDesc> out_ri_desc)
{
  mat44f tm;
  if (READ_LOCK)
    rendinst::getRIGenExtra44(handle, tm);
  else
    rendinst::getRIGenExtra44NoLock(handle, tm);

  return rayTestRiExtraInstanceBase<Strategy, CHK_BOX>(traces, ray_box, coll_res_bbox, handle, tm, res, have_collision, strategy,
    out_ri_desc);
}

static inline void init_raybox_from_trace(bbox3f &box, Trace &trace)
{
  verify_trace(trace);
  vec4f vFrom = v_ld(&trace.pos.x);
  v_bbox3_init(box, vFrom);
  v_bbox3_add_pt(box, v_madd(v_ld(&trace.dir.x), v_max(v_splat_w(vFrom), v_zero()), vFrom));
}

static inline void init_raybox_from_traces(bbox3f &box, dag::Span<Trace> traces)
{
  init_raybox_from_trace(box, traces[0]);
  for (int i = 1; i < traces.size(); ++i)
  {
    verify_trace(traces[i]);
    vec4f vFrom = v_ld(&traces[i].pos.x);
    v_bbox3_add_pt(box, vFrom);
    v_bbox3_add_pt(box, v_madd(v_ld(&traces[i].dir.x), v_max(v_splat_w(vFrom), v_zero()), vFrom));
  }
}

static bool rayHit1RiExtra(Trace &trace, rendinst::RendInstDesc *ri_desc, MaterialRayStrat &strategy, float min_r)
{
  rendinst::RendInstDesc temp_ri_desc;
  rendinst::RendInstDesc &out_ri_desc = ri_desc ? *ri_desc : temp_ri_desc;
  out_ri_desc.layer = -1;
  if (rendinst::rayHitRIGenExtraCollidable(trace.pos, trace.dir, trace.pos.outT, out_ri_desc, strategy, min_r))
    return true;
  return false;
}

template <typename Strategy>
static bool rayTraverseRiExtra(bbox3f_cref ray_box, dag::Span<Trace> traces, rendinst::RendInstDesc *ri_desc, Strategy &strategy,
  bool &haveCollision, riex_handle_t skip_riex_handle = rendinst::RIEX_HANDLE_NULL) // pos bbox here!
{
  TIME_PROFILE_DEV(rayTraverseRiExtra);

  riex_collidable_t ri_h;
  dag::Vector<mat44f, framemem_allocator> ri_tm;

  // RI can be destroyed after gatherRIGenExtraCollidable and before actual ray trace, causing OOB
  // so we save matrices to guarantee valid data (this also save as a read lock in loop later)
  rendinst::ccExtra.lockRead();
  if (traces.size() == 1)
    rendinst::gatherRIGenExtraCollidable(ri_h, traces[0].pos, traces[0].dir, traces[0].pos.outT, false /*read_lock*/);
  else
  {
    BBox3 rayBox;
    v_stu_bbox3(rayBox, ray_box);
    rendinst::gatherRIGenExtraCollidable(ri_h, rayBox, false /*read_lock*/);
  }

  ri_tm.resize_noinit(ri_h.size());
  for (int i = 0; i < ri_h.size(); ++i)
    getRIGenExtra44NoLock(ri_h[i], ri_tm[i]);
  rendinst::ccExtra.unlockRead();

  if (ri_h.size())
  {
    int res_idx = -1;
    CollisionResource *collRes = nullptr;
    bbox3f v_collResBox;
    dag::RelocatableFixedVector<rendinst::RendInstDesc, 4, true, framemem_allocator> descriptions;
    descriptions.push_back_uninitialized(traces.size());
    for (int i = 0; i < ri_h.size(); i++)
    {
      if (DAGOR_UNLIKELY(ri_h[i] == skip_riex_handle))
        continue;

      uint32_t res_idx2 = rendinst::handle_to_ri_type(ri_h[i]);
      if (res_idx != res_idx2)
      {
        res_idx = res_idx2;
        collRes = rendinst::riExtra[res_idx].collRes;
        v_collResBox = collRes->vFullBBox;
      }

      int poolRef = rendinst::riExtra[res_idx].riPoolRef;
      if (RendInstGenData *rgl = (poolRef >= 0) ? rendinst::getRgLayer(rendinst::riExtra[res_idx].riPoolRefLayer) : nullptr)
      {
        const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[poolRef];
        if (strategy.shouldIgnoreRendinst(/*isPos*/ false, riProp.immortal, riProp.matId))
          continue;
      }

      for (auto &desc : descriptions)
        desc.reset();
      bool shouldReturn = rayTestRiExtraInstanceBase<Strategy, false>(traces, ray_box, v_collResBox, ri_h[i], ri_tm[i], collRes,
        haveCollision, strategy, make_span(descriptions));
      if (ri_desc)
        for (int j = 0; j < descriptions.size(); ++j)
          if (descriptions[j].pool >= 0)
          {
            *ri_desc = descriptions[0]; // just first one, should be good enough
            break;
          }

      if (shouldReturn)
        return true;
    }
    if (haveCollision)
    {
      if (ri_desc)
        ri_desc->setRiExtra();
      if (strategy.executeForCell(true))
        return true;
    }
  }

  return false;
}

template <typename Strategy>
static bool rayTraverseRendinst(bbox3f_cref rayBox, dag::Span<Trace> traces, bool trace_meshes, int layer_idx,
  rendinst::RendInstDesc *ri_desc, Strategy &strategy, bool &haveCollision) // pos bbox here!
{
  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  vec4f worldBboxXZ = v_perm_xzac(rayBox.bmin, rayBox.bmax);
  vec4f regionV = v_sub(worldBboxXZ, rgl->world0Vxz);
  regionV = v_max(v_mul(regionV, rgl->invGridCellSzV), v_zero());
  regionV = v_min(regionV, rgl->lastCellXZXZ);
  vec4i regionI = v_cvt_floori(regionV);
  DECL_ALIGN16(int, regions[4]);
  v_sti(regions, regionI);

  ScopedLockRead lock(rgl->rtData->riRwCs);
  rgl->rtData->loadedCellsBBox.clip(regions[0], regions[1], regions[2], regions[3]);
  dag::ConstSpan<int> ld = rgl->rtData->loaded.getList();
  if ((regions[2] - regions[0] + 1) * (regions[3] - regions[1] + 1) < ld.size())
  {
    int cellXStride = rgl->cellNumW - (regions[2] - regions[0] + 1);
    for (int z = regions[1], cellI = regions[1] * rgl->cellNumW + regions[0]; z <= regions[3]; z++, cellI += cellXStride)
      for (int x = regions[0]; x <= regions[2]; x++, cellI++)
      {
        RendInstGenData::Cell &cell = rgl->cells[cellI];
        if (traverseRayCell(cell, rayBox, traces, trace_meshes, layer_idx, ri_desc, strategy, cellI))
        {
          if (ri_desc)
            ri_desc->cellIdx = cellI;
          haveCollision = true;
          if (strategy.executeForCell(true))
            return true;
        }
      }
  }
  else
  {
    for (auto ldi : ld)
    {
      RendInstGenData::Cell &cell = rgl->cells[ldi];
      if (traverseRayCell(cell, rayBox, traces, trace_meshes, layer_idx, ri_desc, strategy, ldi))
      {
        if (ri_desc)
          ri_desc->cellIdx = ldi;
        haveCollision = true;
        if (strategy.executeForCell(true))
          return true;
      }
    }
  }

  return false;
}

template <typename Strategy>
static bool rayTraverse(dag::Span<Trace> traces, bool trace_meshes, rendinst::RendInstDesc *ri_desc, Strategy &strategy,
  riex_handle_t skip_riex_handle = rendinst::RIEX_HANDLE_NULL) // pos bbox here!
{
  bool haveCollision = false;
  bbox3f rayBox;
  init_raybox_from_traces(rayBox, traces);
  if (rayTraverseRiExtra(rayBox, traces, ri_desc, strategy, haveCollision, skip_riex_handle))
    return true;
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
  {
    if (rayTraverseRendinst(rayBox, traces, trace_meshes, _layer, ri_desc, strategy, haveCollision))
    {
      if (ri_desc)
        ri_desc->layer = _layer;
      return true;
    }
  }
  return haveCollision;
}


bool traceTransparencyRayRIGenNormalizedWithDist(const Point3 &pos, const Point3 &dir, float &t, float transparency_threshold,
  PhysMat::MatID ray_mat, rendinst::RendInstDesc *ri_desc, int *out_mat_id, float *out_transparency, bool check_canopy)
{
  RayTransparencyStrat rayTranspStrategy(transparency_threshold, check_canopy, ray_mat);
  Trace traceData(pos, dir, t, nullptr);

  bbox3f rayBox;
  init_raybox_from_trace(rayBox, traceData);
  bool haveCollision = false;
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    rayTraverseRendinst(rayBox, dag::Span<Trace>(&traceData, 1), false, _layer, ri_desc, rayTranspStrategy, haveCollision);
  t = traceData.pos.outT;
  if (haveCollision && out_mat_id)
    *out_mat_id = traceData.outMatId;
  if (out_transparency)
    *out_transparency = rayTranspStrategy.accumulatedTransparency;
  return haveCollision;
}

bool traceTransparencyRayRIGenNormalized(const Point3 &pos, const Point3 &dir, float mint, float transparency_threshold,
  PhysMat::MatID ray_mat, rendinst::RendInstDesc *ri_desc, int *out_mat_id, float *out_transparency, bool check_canopy)
{
  return traceTransparencyRayRIGenNormalizedWithDist(pos, dir, mint, transparency_threshold, ray_mat, ri_desc, out_mat_id,
    out_transparency, check_canopy);
}

bool traceTransparencyRayRIGenNormalizedAllLayers(const Point3 &pos, const Point3 &dir, float mint, float transparency_threshold,
  PhysMat::MatID ray_mat, rendinst::RendInstDesc *ri_desc, int *out_mat_id, float *out_transparency, bool check_canopy,
  float min_height_second_layer)
{
  RayTransparencyStrat rayTranspStrategy(transparency_threshold, check_canopy, ray_mat);
  rayTranspStrategy.secondLayerMinCapsuleHeightSq = sqr(min_height_second_layer);
  Trace traceData(pos, dir, mint, nullptr);

  bbox3f rayBox;
  init_raybox_from_trace(rayBox, traceData);
  bool haveCollision = false;
  FOR_EACH_RG_LAYER_DO (rgl)
    rayTraverseRendinst(rayBox, dag::Span<Trace>(&traceData, 1), false, _layer, ri_desc, rayTranspStrategy, haveCollision);
  if (haveCollision && out_mat_id)
    *out_mat_id = traceData.outMatId;
  if (out_transparency)
    *out_transparency = rayTranspStrategy.accumulatedTransparency;
  return haveCollision;
}

void initTraceTransparencyParameters(float tree_trunk_opacity, float tree_canopy_opacity)
{
  treeTrunkOpacity = tree_trunk_opacity;
  treeCanopyOpacity = tree_canopy_opacity;
}

bool traceSoundOcclusionRayRIGenNormalized(const Point3 &p, const Point3 &dir, float t, int ray_mat_id, float &accumulated_occlusion,
  float &max_occlusion)
{
  RaySoundOcclusionStrat strategy(ray_mat_id);

  Trace traceData(p, dir, t, nullptr);
  bbox3f rayBox;
  init_raybox_from_trace(rayBox, traceData);

  bool haveCollision = false;
  rayTraverseRiExtra(rayBox, dag::Span<Trace>(&traceData, 1), nullptr, strategy, haveCollision, rendinst::RIEX_HANDLE_NULL);
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    rayTraverseRendinst(rayBox, dag::Span<Trace>(&traceData, 1), true /*trace_meshes*/, _layer, nullptr, strategy, haveCollision);

  accumulated_occlusion = strategy.accumulatedOcclusion;
  max_occlusion = strategy.maxOcclusion;
  return haveCollision;
}

bool checkCachedRiData(const TraceMeshFaces *ri_cache)
{
  return ri_cache->isRendinstsValid && riutil::world_version_check(ri_cache->rendinstCache.version, ri_cache->box);
}

static bool check_cached_ri_data(const TraceMeshFaces *ri_cache)
{
  bool res = checkCachedRiData(ri_cache);
  if (!res)
    TRACE_HANDLE_DEBUG_STAT(ri_cache, numRIMisses++);
  return res;
}

template <typename Strategy>
static bool rayTestIndividualNoLock(dag::Span<Trace> traces, const rendinst::RendInstDesc &ri_desc,
  dag::Span<rendinst::RendInstDesc> out_ri_desc, Strategy &strategy, bbox3f_cref rayBox,
  riex_handle_t skip_riex_handle = rendinst::RIEX_HANDLE_NULL)
{
  bool haveCollision = false;
  if (!ri_desc.isValid())
    return false;

  if (ri_desc.isRiExtra())
  {
    if (ri_desc.pool >= rendinst::riExtra.size())
      return false;
    rendinst::RiExtraPool *pool = rendinst::riExtra.data() + ri_desc.pool;
    if (!pool->isInGrid(ri_desc.idx))
      return false;

    if (pool->isPosInst())
      return false;

    if (RendInstGenData *rgl = (pool->riPoolRef >= 0) ? rendinst::getRgLayer(pool->riPoolRefLayer) : nullptr)
    {
      const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[pool->riPoolRef];
      if (strategy.shouldIgnoreRendinst(/*isPos*/ false, riProp.immortal, riProp.matId))
        return false;
    }

    CollisionResource *collRes = pool->collRes;

    if (!collRes)
      return false;

    auto rh = rendinst::make_handle(ri_desc.pool, ri_desc.idx);
    if (DAGOR_UNLIKELY(rh == skip_riex_handle))
      return false;

    rayTestRiExtraInstance<Strategy, /*CHK_BOX*/ true, /*READ_LOCK*/ false>(traces, rayBox, collRes->vFullBBox, rh, collRes,
      haveCollision, strategy, out_ri_desc);
    return haveCollision;
  }

  RendInstGenData::Cell *cell = nullptr;
  int16_t *data = riutil::get_data_by_desc(ri_desc, cell);
  if (!data)
    return false;

  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(ri_desc);
  mat44f tm;
  if (!riutil::get_rendinst_matrix(ri_desc, rgl, data, cell, tm) || !rgl)
    return false;

  CollisionResource *collRes = rgl->rtData->riCollRes[ri_desc.pool].collRes;
  if (!collRes || rgl->rtData->riPosInst[ri_desc.pool])
    return false;

  bbox3f transformedBox;
  v_bbox3_init(transformedBox, tm, collRes->vFullBBox);
  if (!v_bbox3_test_box_intersect(transformedBox, rayBox))
    return false;

  for (int rayId = 0; rayId < traces.size(); ++rayId)
  {
    bool localHaveCollision = false;
    bool shouldExit = strategy.executeForMesh(collRes, tm, traces[rayId].pos, traces[rayId].dir, traces[rayId].pos.outT,
      traces[rayId].outNorm, safe_at(out_ri_desc, rayId), localHaveCollision, ri_desc.layer, ri_desc.idx, ri_desc.pool, ri_desc.offs,
      traces[rayId].outMatId, 0);
    haveCollision |= localHaveCollision;
    if (localHaveCollision && rayId < out_ri_desc.size())
      out_ri_desc[rayId] = ri_desc;
    if (shouldExit)
      return true;
  }

  return haveCollision;
}

bool traceRayRIGenNormalized(dag::Span<Trace> traces, TraceFlags trace_flags, int ray_mat_id, rendinst::RendInstDesc *out_ri_descs,
  const TraceMeshFaces *ri_cache, rendinst::riex_handle_t skip_riex_handle)
{
  TraceRayStrat traceRayStrategy(ray_mat_id, trace_flags);
  bool ret = false;
  if (ri_cache)
  {
    AutoLockReadPrimaryAndExtra lockRead;

    if (check_cached_ri_data(ri_cache))
    {
      bbox3f rayBox;
      trace_utils::prepare_traces_box(traces, rayBox);
      ri_cache->rendinstCache.foreachValid(rendinst::GatherRiTypeFlag::RiGenTmAndExtra,
        [&](const rendinst::RendInstDesc &ri_desc, bool) {
          if (rendinst::isRgLayerPrimary(ri_desc.layer))
            ret |= rayTestIndividualNoLock(traces, ri_desc, {}, traceRayStrategy, rayBox, skip_riex_handle);
        });

      return ret;
    }
    trace_utils::draw_trace_handle_debug_cast_result(ri_cache, traces, false, true);
  }
  return rayTraverse(traces, bool(trace_flags & TraceFlag::Meshes), out_ri_descs, traceRayStrategy, skip_riex_handle);
}


DECL_ALIGN16(static float, v_SUBCELL_DIV_MAX[4]) = {
  SUBCELL_DIV - 0.01f, SUBCELL_DIV - 0.01f, SUBCELL_DIV - 0.01f, SUBCELL_DIV - 0.01f};

static bool traceDownMultiRayCell(int layer_idx, int cell_idx, dag::Span<Trace> traces, dag::Span<rendinst::RendInstDesc> ri_desc,
  bbox3f_cref ray_box, int ray_mat_id, uint8_t behaviorFlags, Bitarray *filter_pools)
{
  const RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  const RendInstGenData::Cell &cell = rgl->cells[cell_idx];
  const RendInstGenData::CellRtData *crt_ptr = cell.isReady();
  if (!crt_ptr)
    return false;
  const RendInstGenData::CellRtData &crt = *crt_ptr;

  if (!v_bbox3_test_box_intersect(crt.bbox[0], ray_box))
    return false;

  float cellSz = rgl->grid2world * rgl->cellSz;
  vec4f v_invSubcellSz = v_splats(SUBCELL_DIV / cellSz);

  int pcnt = crt.pools.size();
  vec3f v_cell_add = crt.cellOrigin;
  vec3f v_cell_mul = v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(cellSz, crt.cellHeight, cellSz, 0));

  bool haveCollision = false;
  // crt.bbox[0] and rayBox intersects, so we can merge them (isectBox2d = rayBox2d & cellBox2d)
  vec4f rayBox2d = v_perm_xzac(ray_box.bmin, ray_box.bmax);
  vec4f cellBox2d = v_perm_xzac(crt.bbox[0].bmin, crt.bbox[0].bmax);
  vec4f isectBox2d = v_perm_xycd(v_max(rayBox2d, cellBox2d), v_min(rayBox2d, cellBox2d));

  isectBox2d = v_sub(isectBox2d, v_perm_xzxz(v_cell_add));
  isectBox2d = v_mul(isectBox2d, v_invSubcellSz);
  isectBox2d = v_max(isectBox2d, v_zero());
  isectBox2d = v_min(isectBox2d, v_ld(v_SUBCELL_DIV_MAX));
  vec4i vSubCell = v_cvt_vec4i(isectBox2d); // it is the same as floor, since it is always positive!
  DECL_ALIGN16(int, subCell[4]);            // xzXZ
  v_sti(subCell, vSubCell);

  // Possible small optimization: check that each ray intersect isectBox2d
  eastl::fixed_vector<CollisionTrace, 20, true, framemem_allocator> collisionTraces(traces.size());
  for (int rayId = 0; rayId < traces.size(); rayId++)
  {
    collisionTraces[rayId].vFrom = v_ldu(&traces[rayId].pos.x);
    collisionTraces[rayId].vDir = v_ldu(&traces[rayId].dir.x);
    collisionTraces[rayId].norm = Point3(0, 1, 0);
    collisionTraces[rayId].t = traces[rayId].pos.outT;
    collisionTraces[rayId].outMatId = PHYSMAT_INVALID;
  }
  dag::Span<CollisionTrace> tracesSlice(collisionTraces.data(), collisionTraces.size());

  const eastl::BitvectorWordType *riPosInstData = rgl->rtData->riPosInst.data();
  for (int stride_subCell = SUBCELL_DIV - (subCell[2] - subCell[0] + 1), idx = subCell[1] * SUBCELL_DIV + subCell[0];
       subCell[1] <= subCell[3]; subCell[1]++, idx += stride_subCell)
  {
    for (int x2 = subCell[0]; x2 <= subCell[2]; x2++, idx++)
    {
      if (!v_bbox3_test_box_intersect(crt.bbox[idx + 1], ray_box))
        continue;
      eastl::BitvectorWordType riPosInstBit = 1;
      for (int p = 0; DAGOR_LIKELY(p < pcnt); p++, riPosInstBit = roll_bit(riPosInstBit))
      {
        const RendInstGenData::CellRtData::SubCellSlice &scs = crt.getCellSlice(p, idx);
        if (DAGOR_LIKELY(!scs.sz))
          continue;
        bool isPosInst = (riPosInstData[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0;
        if (DAGOR_LIKELY(isPosInst))
          continue;

        CollisionResource *collRes = rgl->rtData->riCollRes[p].collRes;
        if (!collRes)
          continue;

        if (filter_pools && filter_pools->get(p))
          continue;

        const int16_t *data_s = (int16_t *)(crt.sysMemData.get() + scs.ofs);
        int stride_w =
          RIGEN_TM_STRIDE_B((rgl->rtData->riZeroInstSeeds.data()[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0,
            rgl->perInstDataDwords) /
          2;
        for (const int16_t *data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
        {
          mat44f tm;
          if (!rendinst::gen::unpack_tm_full(tm, data, v_cell_add, v_cell_mul))
            continue;
#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS // ignore riZeroInstSeeds here since we use this code for tools only
          if (is_rendinst_marked_collision_ignored(data, rgl->perInstDataDwords, RIGEN_TM_STRIDE_B(false, 0) / 2))
            continue;
#endif

          haveCollision |= collRes->traceMultiRay(tm, tracesSlice, ray_mat_id, behaviorFlags);
          for (int rayId = 0; rayId < traces.size(); rayId++)
          {
            if (collisionTraces[rayId].isHit)
            {
              ri_desc[rayId].cellIdx = cell_idx;
              ri_desc[rayId].idx = idx;
              ri_desc[rayId].pool = p;
              ri_desc[rayId].offs = int(intptr_t(data) - intptr_t(data_s));
              ri_desc[rayId].layer = layer_idx;

              traces[rayId].pos.outT = collisionTraces[rayId].t;
              traces[rayId].outNorm = collisionTraces[rayId].norm;
              if (collisionTraces[rayId].outMatId == PHYSMAT_INVALID || collisionTraces[rayId].outMatId == PHYSMAT_DEFAULT)
                traces[rayId].outMatId = getRIGenMaterialId(ri_desc[rayId]);
              else
                traces[rayId].outMatId = collisionTraces[rayId].outMatId;
            }
          }
        }
      }
    }
  }
  return haveCollision;
}

bool traceDownMultiRay(dag::Span<Trace> traces, bbox3f_cref ray_box, dag::Span<rendinst::RendInstDesc> ri_descs,
  const TraceMeshFaces *ri_cache, int ray_mat_id, TraceFlags trace_flags, Bitarray *filter_pools, IgnoreFunc ignore_func)
{
  G_ASSERT(traces.size() == ri_descs.size());
  G_ASSERT(!(trace_flags & (TraceFlag::Meshes | TraceFlag::Trees)));

  if (traces.empty())
    return false;

  bool haveCollision = false;
  if (ri_cache)
  {
    AutoLockReadPrimaryAndExtra lockRead;

    if (check_cached_ri_data(ri_cache))
    {
      TraceRayStrat traceRayStrat(ray_mat_id, trace_flags);

      ri_cache->rendinstCache.foreachValid(rendinst::GatherRiTypeFlag::RiGenTmAndExtra, [&](const rendinst::RendInstDesc &desc, bool) {
        if (rendinst::isRgLayerPrimary(desc.layer) && !ignoreTraceRiExtra(desc, ignore_func))
          haveCollision |= rayTestIndividualNoLock(traces, desc, ri_descs, traceRayStrat, ray_box);
      });

      return haveCollision;
    }
    trace_utils::draw_trace_handle_debug_cast_result(ri_cache, traces, false, true);
  }

  uint8_t behaviorFlags = trace_flags & TraceFlag::Phys ? CollisionNode::PHYS_COLLIDABLE : CollisionNode::TRACEABLE;

  BBox3 rayBox;
  v_stu_bbox3(rayBox, ray_box);
  riex_collidable_t ri_h;
  dag::Vector<mat44f, framemem_allocator> ri_tm;
  rendinst::ccExtra.lockRead();
  rendinst::gatherRIGenExtraCollidable(ri_h, rayBox, false /*read_lock*/);
  ri_tm.resize_noinit(ri_h.size());
  for (int i = 0; i < ri_h.size(); ++i)
    getRIGenExtra44NoLock(ri_h[i], ri_tm[i]);
  rendinst::ccExtra.unlockRead();

  eastl::fixed_vector<CollisionTrace, 20, true, framemem_allocator> collisionTraces(traces.size());
  for (int rayId = 0; rayId < traces.size(); rayId++)
  {
    collisionTraces[rayId].vFrom = v_ldu(&traces[rayId].pos.x);
    collisionTraces[rayId].vDir = v_ldu(&traces[rayId].dir.x);
    collisionTraces[rayId].norm = Point3(0, 1, 0);
    collisionTraces[rayId].t = traces[rayId].pos.outT;
    collisionTraces[rayId].outMatId = PHYSMAT_INVALID;
  }

  for (int i = 0; i < ri_h.size(); i++)
  {
    uint32_t resIdx = rendinst::handle_to_ri_type(ri_h[i]);
    const CollisionResource *collRes = rendinst::riExtra[resIdx].collRes;
    if (!collRes)
      continue;

    if (filter_pools && filter_pools->get(rendinst::riExtra[resIdx].riPoolRef))
      continue;

    if (ignoreTraceRiExtra(RendInstDesc(ri_h[i]), ignore_func))
      continue;

    dag::Span<CollisionTrace> tracesSlice(collisionTraces.data(), collisionTraces.size());
    haveCollision |= collRes->traceMultiRay(ri_tm[i], tracesSlice, ray_mat_id, behaviorFlags);
    for (int rayId = 0; rayId < traces.size(); rayId++)
    {
      if (collisionTraces[rayId].isHit)
      {
        ri_descs[rayId].setRiExtra();
        ri_descs[rayId].idx = rendinst::handle_to_ri_inst(ri_h[i]);
        ri_descs[rayId].pool = resIdx;
        ri_descs[rayId].offs = 0;
        ri_descs[rayId].layer = rendinst::riExtra[resIdx].riPoolRefLayer;

        traces[rayId].pos.outT = collisionTraces[rayId].t;
        traces[rayId].outNorm = collisionTraces[rayId].norm;
        if (collisionTraces[rayId].outMatId == PHYSMAT_INVALID || collisionTraces[rayId].outMatId == PHYSMAT_DEFAULT)
          traces[rayId].outMatId = getRIGenMaterialId(ri_descs[rayId]);
        else
          traces[rayId].outMatId = collisionTraces[rayId].outMatId;
      }
    }
  }

  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
  {
    vec4f worldBboxXZ = v_perm_xzac(ray_box.bmin, ray_box.bmax);
    vec4f regionV = v_sub(worldBboxXZ, rgl->world0Vxz);
    regionV = v_max(v_mul(regionV, rgl->invGridCellSzV), v_zero());
    regionV = v_min(regionV, rgl->lastCellXZXZ);
    vec4i regionI = v_cvt_floori(regionV);
    DECL_ALIGN16(int, regions[4]);
    v_sti(regions, regionI);

    ScopedLockRead lock(rgl->rtData->riRwCs);
    rgl->rtData->loadedCellsBBox.clip(regions[0], regions[1], regions[2], regions[3]);

    int cellXStride = rgl->cellNumW - (regions[2] - regions[0] + 1);
    for (int z = regions[1], cellI = regions[1] * rgl->cellNumW + regions[0]; z <= regions[3]; z++, cellI += cellXStride)
    {
      for (int x = regions[0]; x <= regions[2]; x++, cellI++)
      {
        if (traceDownMultiRayCell(_layer, cellI, traces, ri_descs, ray_box, ray_mat_id, behaviorFlags, filter_pools))
          haveCollision = true;
      }
    }
  }

  return haveCollision;
}

bool rayhitRendInstNormalized(const Point3 &from, const Point3 &dir, float t, int ray_mat_id, const rendinst::RendInstDesc &ri_desc)
{
  RayHitStrat rayHitStrategy(ray_mat_id);
  Trace traceData(from, dir, t, nullptr);
  bbox3f rayBox;
  init_raybox_from_trace(rayBox, traceData);
  RendInstDesc desc;
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(ri_desc);
  ScopedLockRead lock(ri_desc.isRiExtra() ? rendinst::ccExtra : rgl->rtData->riRwCs);
  bool ret =
    rayTestIndividualNoLock(dag::Span<Trace>(&traceData, 1), ri_desc, dag::Span<RendInstDesc>(&desc, 1), rayHitStrategy, rayBox);
  return ret;
}

// optimized version
bool rayhitRendInstsNormalized(const Point3 &from, const Point3 &dir, float t, float min_size, int ray_mat_id,
  rendinst::RendInstDesc *ri_desc)
{
  Trace traceData(from, dir, t, nullptr);

  RayHitStrat rayHitStrategy(ray_mat_id);
  if (rayHit1RiExtra(traceData, ri_desc, rayHitStrategy, min_size))
    return true;
  bool haveCollision = false;
  bbox3f rayBox;
  init_raybox_from_trace(rayBox, traceData);
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    rayTraverseRendinst(rayBox, dag::Span<Trace>(&traceData, 1), true, _layer, ri_desc, rayHitStrategy, haveCollision);
  return haveCollision;
}

bool traceRayRendInstsNormalized(const Point3 &from, const Point3 &dir, float &tout, Point3 &norm, bool /*extend_bbox*/,
  bool trace_meshes, rendinst::RendInstDesc *ri_desc, bool trace_trees, int ray_mat_id, int *out_mat_id)
{
  TraceFlags traceFlags = TraceFlag::Destructible;
  if (trace_meshes)
    traceFlags |= TraceFlag::Meshes;
  if (trace_trees)
    traceFlags |= TraceFlag::Trees;
  Trace traceData(from, dir, tout, nullptr);
  traceData.outNorm = norm;
  if (!traceRayRIGenNormalized(dag::Span<Trace>(&traceData, 1), traceFlags, ray_mat_id, ri_desc))
    return false;
  tout = traceData.pos.outT;
  norm = traceData.outNorm;
  if (out_mat_id)
    *out_mat_id = traceData.outMatId;
  return true;
}

bool traceRayRendInstsListNormalized(const Point3 &from, const Point3 &dir, float dist, RendInstsIntersectionsList &ri_data,
  bool trace_meshes)
{
  Trace traceData(from, dir, dist, nullptr);
  TraceRayListStrat traceRayStrategy(PHYSMAT_INVALID);
  bool ret = false;
  if (rayTraverse(dag::Span<Trace>(&traceData, 1), trace_meshes, nullptr, traceRayStrategy))
    ret = true;
  if (!ret || traceRayStrategy.list.empty())
    return false;
  ri_data = eastl::move(traceRayStrategy.list);
  return true;
}

template <typename ListT>
bool traceRayRendInstsListAllNormalizedImpl(const Point3 &from, const Point3 &dir, float dist, ListT &ri_data, bool trace_meshes,
  float extend_ray)
{
  Trace traceData(from, dir, dist, nullptr);
  TraceRayAllUnsortedListStrat traceRayStrategy(PHYSMAT_INVALID, ri_data);
  traceRayStrategy.extendRay = extend_ray;
  bool ret = false;
  if (rayTraverse(dag::Span<Trace>(&traceData, 1), trace_meshes, nullptr, traceRayStrategy))
    ret = true;
  if (!ret || traceRayStrategy.list.empty())
    return false;
  stlsort::sort_branchless(ri_data.begin(), ri_data.end());
  return true;
}

bool traceRayRendInstsListAllNormalized(const Point3 &from, const Point3 &dir, float dist, RendInstsIntersectionsList &ri_data,
  bool trace_meshes, float extend_ray)
{
  return traceRayRendInstsListAllNormalizedImpl(from, dir, dist, ri_data, trace_meshes, extend_ray);
}

bool traceRayRendInstsListAllNormalized(const Point3 &from, const Point3 &dir, float dist, RendInstsSolidIntersectionsList &ri_data,
  bool trace_meshes, float extend_ray)
{
  return traceRayRendInstsListAllNormalizedImpl(from, dir, dist, ri_data, trace_meshes, extend_ray);
}

template <typename T, typename IdxT, int StackSize>
struct LocalIntersectionIdRemap
{
  dag::RelocatableFixedVector<T, StackSize, true, framemem_allocator> data;
  IdxT add(const T &item)
  {
    auto it = eastl::find(data.begin(), data.end(), item);
    if (it == data.end())
    {
      data.push_back(item);
      it = data.end() - 1;
    }
    return IdxT(it - data.begin());
  }

  const T &get(IdxT id) const { return data[id]; }
};

void computeRiIntersectedSolids(RendInstsSolidIntersectionsList &intersected, const Point3 &from, const Point3 &dir,
  SolidSectionsMerge merge_mode)
{
  if (DAGOR_UNLIKELY(merge_mode == SolidSectionsMerge::NONE))
    return;
  if (intersected.size() < 2)
    return;
  const int intersectionCount = int(intersected.size());

  if (intersectionCount > 1024 || intersectionCount < 0)
  {
    eastl::string dbgNames;
    dbgNames.reserve(300);
    for (const auto &data : intersected)
    {
      dbgNames += " ";
      dbgNames += getRIGenResName(data.riDesc);
      if (dbgNames.length() > 200)
        break;
    }
    logerr("computeRiIntersectedSolids has received too big intersection list (%i), this is probably a bug, it will not be processed, "
           "ray=(%.9f,%.9f,%.9f),(%.9f,%.9f,%.9f) printing first rendinsts: %s",
      intersectionCount, from.x, from.y, from.z, dir.x, dir.y, dir.z, dbgNames.c_str());
    intersected.clear();
    return;
  }

  union LocalIntersectionId
  {
    struct
    {
      uint8_t collNodeId;
      uint8_t matId;
      uint8_t riId;
      bool removed : 1;
      bool direction : 1;
    };
    uint32_t data;
    int outIdx;

    bool matches(LocalIntersectionId rhs) const { return collNodeId == rhs.collNodeId && matId == rhs.matId && riId == rhs.riId; }
  };

  dag::RelocatableFixedVector<LocalIntersectionId, 128u, true, framemem_allocator> intersectionData;
  LocalIntersectionIdRemap<unsigned, uint8_t, 16> localCollNodeRemap;
  LocalIntersectionIdRemap<int, uint8_t, 16> localMatIdRemap;
  LocalIntersectionIdRemap<RendInstDesc, uint8_t, 4> localRiRemap;

  // [0, size) - coll node index and flags
  // [size, size * 2) - out intersection index
  intersectionData.resize(size_t(intersectionCount) * 2u);

  const vec3f v_ray_dir = v_ldu(&dir.x);
  for (int i = 0; i < intersectionCount; i++)
  {
    const auto &point = intersected[i];
    LocalIntersectionId id;
    id.data = 0;
    id.riId = localRiRemap.add(point.riDesc);
    id.matId = localMatIdRemap.add(point.matId);
    id.collNodeId = localCollNodeRemap.add(point.collisionNodeId);
    id.removed = false;
    id.direction = v_test_vec_x_le_0(v_dot3_x(v_ray_dir, v_ldu(&point.normIn.x)));
    intersectionData[i] = id;
    intersectionData[i + intersectionCount].outIdx = i;
  }

  for (int i = 0; i < intersectionCount; i++)
  {
    if (intersectionData[i].removed)
      continue;

    int outPointIndex = i;
    if (intersectionData[i].direction)
    {
      int entries = 1;
      for (int j = i + 1; j < intersectionCount; j++)
      {
        if (intersectionData[j].removed)
          continue;

        if (intersectionData[i].matches(intersectionData[j]))
        {
          if (intersectionData[j].direction)
            entries++;
          else
          {
            entries--;
            if (entries == 0)
            {
              outPointIndex = j;
              break;
            }
          }
        }
      }
      if (entries == 0)
      {
        for (int j = i + 1; j <= outPointIndex; j++)
        {
          if (intersectionData[i].matches(intersectionData[j]))
            intersectionData[j].removed = true;
        }
      }
    }
    intersectionData[i + intersectionCount].outIdx = outPointIndex;
  }

  if (merge_mode != SolidSectionsMerge::COLL_NODE)
  {
    for (int i = 0; i < intersectionCount; i++)
    {
      switch (merge_mode)
      {
        case SolidSectionsMerge::ALL:
          intersectionData[i].collNodeId = 0;
          intersectionData[i].riId = 0;
          intersectionData[i].matId = 0;
          break;
        case SolidSectionsMerge::RENDINST:
          intersectionData[i].collNodeId = 0;
          intersectionData[i].matId = 0;
          break;
        case SolidSectionsMerge::RI_MAT: intersectionData[i].collNodeId = 0; break;
        case SolidSectionsMerge::MATERIAL:
          intersectionData[i].collNodeId = 0;
          intersectionData[i].riId = 0;
          break;
        case SolidSectionsMerge::COLL_NODE:
        case SolidSectionsMerge::NONE: break;
      }
    }

    for (int i = 0; i < intersectionCount; i++)
    {
      if (intersectionData[i].removed)
        continue;
      int outIdx = intersectionData[i + intersectionCount].outIdx;
      for (int j = i + 1; j < outIdx; j++)
      {
        if (intersectionData[j].removed)
          continue;
        if ((intersectionData[i]).matches(intersectionData[j]))
        {
          outIdx = eastl::max(outIdx, intersectionData[j + intersectionCount].outIdx);
          intersectionData[j].removed = true;
        }
      }
      intersectionData[i + intersectionCount].outIdx = outIdx;
    }
  }

  const auto it = eastl::remove_if(intersected.begin(), intersected.end(), [&](TraceRayRendInstSolidData &point) {
    const int idx = int(&point - intersected.data());
    const int outIdx = intersectionData[idx + intersectionCount].outIdx;
    point.tOut = intersected[outIdx].tOut;
    point.normOut = intersected[outIdx].normOut;
    return intersectionData[idx].removed;
  });
  intersected.erase(it, intersected.end());
}

static bool testObjToRIGenIntersectionVel(int layer_idx, CollisionResource *obj_res, const CollisionNodeFilter &filter,
  const TMatrix &obj_tm, const Point3 &obj_vel, Point3 *intersected_obj_pos, bool *tree_sphere_intersected, Point3 *collisionPoint)
{
  mat44f vObjTm;
  v_mat44_make_from_43cu_unsafe(vObjTm, obj_tm.array);
  vec3f vObjSphereCenter = v_mat44_mul_vec3p(vObjTm, obj_res->vBoundingSphere);

  bbox3f vObjBb;
  v_bbox3_init(vObjBb, vObjTm, obj_res->vFullBBox);

  mat44f vInvTm, vInvOldTm;
  bool invReady = false;

  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  float cellSz = rgl->grid2world * rgl->cellSz;
  float invsubcellSz = SUBCELL_DIV / cellSz;
  float testRad = obj_vel.length() * 2.0f + 50.0f;

  vec4f worldBboxXZ = v_perm_xzac(vObjBb.bmin, vObjBb.bmax);
  vec4f regionV = v_sub(worldBboxXZ, rgl->world0Vxz);
  regionV = v_max(v_mul(regionV, rgl->invGridCellSzV), v_zero());
  regionV = v_min(regionV, rgl->lastCellXZXZ);
  vec4i regionI = v_cvt_floori(regionV);
  DECL_ALIGN16(int, regions[4]);
  v_sti(regions, regionI);

  ScopedLockRead lock(rgl->rtData->riRwCs);
  const eastl::BitvectorWordType *riPosInstData = rgl->rtData->riPosInst.data();
  const eastl::BitvectorWordType *riPaletteRotation = rgl->rtData->riPaletteRotation.data();
  int cellXStride = rgl->cellNumW - (regions[2] - regions[0] + 1);
  for (int z = regions[1], cellI = regions[1] * rgl->cellNumW + regions[0]; z <= regions[3]; z++, cellI += cellXStride)
    for (int x = regions[0]; x <= regions[2]; x++, cellI++)
    {
      RendInstGenData::Cell &cell = rgl->cells[cellI];
      const RendInstGenData::CellRtData *crt_ptr = cell.isReady();
      if (!crt_ptr)
        continue;
      const RendInstGenData::CellRtData &crt = *crt_ptr;

      if (!v_bbox3_test_box_intersect(crt.bbox[0], vObjBb))
        continue;

      int pcnt = crt.pools.size();
      float cell_h0 = v_extract_y(crt.cellOrigin), cell_dh = crt.cellHeight;
      float cell_x0 = v_extract_x(crt.cellOrigin), cell_z0 = v_extract_z(crt.cellOrigin);

      vec4i cellsRange = v_cvt_floori(v_mul(
        v_sub(v_add(v_perm_xxzz(vObjTm.col3), v_make_vec4f(-testRad, +testRad, -testRad, +testRad)), v_perm_xxzz(crt.cellOrigin)),
        v_splats(invsubcellSz)));
      cellsRange = v_clampi(cellsRange, v_zeroi(), v_splatsi(SUBCELL_DIV - 1));
      int x0 = v_extract_xi(cellsRange), x1 = v_extract_yi(cellsRange);
      int z0 = v_extract_zi(cellsRange), z1 = v_extract_wi(cellsRange);
      for (; z0 <= z1; z0++)
        for (int x2 = x0; x2 <= x1; x2++)
        {
          int idx = z0 * SUBCELL_DIV + x2 + 1;

          if (!v_bbox3_test_box_intersect(crt.bbox[idx], vObjBb))
            continue;
          idx--;

          eastl::BitvectorWordType riPosInstBit = 1;
          for (int p = 0; p < pcnt; p++, riPosInstBit = roll_bit(riPosInstBit))
          {
            const RendInstGenData::CellRtData::SubCellSlice &scs = crt.getCellSlice(p, idx);
            bool isPosInst = (riPosInstData[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0;
            if (!isPosInst || !scs.sz)
              continue;
            bool paletteRotation = (riPaletteRotation[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0;

            CollisionResource *riCollRes = rgl->rtData->riCollRes[p].collRes;
            if (!riCollRes)
              continue;
            float max_y = v_extract_y(riCollRes->vFullBBox.bmax);
            int stride_w =
              RIGEN_POS_STRIDE_B((rgl->rtData->riZeroInstSeeds.data()[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0,
                rgl->perInstDataDwords) /
              2;
            if (paletteRotation)
            {
              rendinst::gen::RotationPaletteManager::Palette palette =
                rendinst::gen::get_rotation_palette_manager()->getPalette({layer_idx, p});

              for (const int16_t *data = (int16_t *)(crt.sysMemData.get() + scs.ofs), *data_e = data + scs.sz / 2; data < data_e;
                   data += stride_w)
              {
                int paletteId;
                TMatrix riTm;
                if (!rendinst::gen::unpack_tm_pos_fast(riTm, data, cell_x0, cell_h0, cell_z0, cellSz, cell_dh, paletteRotation,
                      &paletteId))
                  continue;
                TMatrix rotTm = rendinst::gen::RotationPaletteManager::get_tm(palette, paletteId);
                mat44f vRiTm, vRotTm;
                v_mat44_make_from_43cu_unsafe(vRiTm, riTm.array);
                v_mat44_make_from_43cu_unsafe(vRotTm, rotTm.array);
                v_mat44_mul43(vRiTm, vRiTm, vRotTm);
                vec4f maxRiTmScale = v_mat44_max_scale43_x(vRiTm);
                float maxMulY = abs(v_extract_y(vRiTm.col0)) + abs(v_extract_y(vRiTm.col1)) + abs(v_extract_y(vRiTm.col2));

                float scaledRiSphereRadius = riCollRes->getBoundingSphereRad() * v_extract_x(maxRiTmScale);
                float sumRad = obj_res->getBoundingSphereRad() + scaledRiSphereRadius;

                vec3f vRiSphereCenter = v_mat44_mul_vec3p(vRiTm, riCollRes->vBoundingSphere);
                vec4f vSphereDist = v_length3_sq_x(v_sub(vObjSphereCenter, vRiSphereCenter));
                if (v_extract_x(vSphereDist) > sumRad * sumRad)
                  continue;

                if (intersected_obj_pos)
                  v_stu_p3(&intersected_obj_pos->x, vRiTm.col3);
                if (tree_sphere_intersected)
                  *tree_sphere_intersected = true;

                if (!invReady)
                {
                  vec3f vObjVel = v_ldu(&obj_vel.x);
                  mat44f vOldTm = vObjTm;
                  vOldTm.col3 = v_sub(vOldTm.col3, v_add(vObjVel, vObjVel));
                  v_mat44_inverse43(vInvTm, vObjTm);
                  v_mat44_inverse43(vInvOldTm, vOldTm);
                  invReady = true;
                }

                float intersectionT = max_y * maxMulY;
                vec3f vFrom = vRiTm.col3;
                vec3f vDir = V_C_UNIT_0100;
                vec3f vTo = v_madd(vDir, v_splats(intersectionT), vFrom);

                vec3f a00 = v_mat44_mul_vec3p(vInvOldTm, vFrom);
                vec3f a01 = v_mat44_mul_vec3p(vInvOldTm, vTo);
                vec3f a10 = v_mat44_mul_vec3p(vInvTm, vFrom);
                vec3f a11 = v_mat44_mul_vec3p(vInvTm, vTo);

                Point3 point;
                int nodeIndex = -1;
                if (obj_res->traceQuad(filter, a00, a01, a10, a11, point, nodeIndex))
                {
                  if (collisionPoint)
                    *collisionPoint = point;
                  return true;
                }
              }
            }
            else
            {
              for (const int16_t *data = (int16_t *)(crt.sysMemData.get() + scs.ofs), *data_e = data + scs.sz / 2; data < data_e;
                   data += stride_w)
              {
                TMatrix riTm;
                if (!rendinst::gen::unpack_tm_pos_fast(riTm, data, cell_x0, cell_h0, cell_z0, cellSz, cell_dh, paletteRotation))
                  continue;

                mat44f vRiTm;
                v_mat44_make_from_43cu_unsafe(vRiTm, riTm.array);
                float scaledRiSphereRadius = riCollRes->getBoundingSphereRad() * riTm.m[0][0];
                float sumRad = obj_res->getBoundingSphereRad() + scaledRiSphereRadius;

                vec3f vRiSphereCenter = v_mat44_mul_vec3p(vRiTm, riCollRes->vBoundingSphere);
                vec4f vSphereDist = v_length3_sq_x(v_sub(vObjSphereCenter, vRiSphereCenter));
                if (v_extract_x(vSphereDist) > sumRad * sumRad)
                  continue;

                if (intersected_obj_pos)
                  *intersected_obj_pos = riTm.getcol(3);
                if (tree_sphere_intersected)
                  *tree_sphere_intersected = true;

                if (!invReady)
                {
                  vec3f vObjVel = v_ldu(&obj_vel.x);
                  mat44f vOldTm = vObjTm;
                  vOldTm.col3 = v_sub(vOldTm.col3, v_add(vObjVel, vObjVel));
                  v_mat44_inverse43(vInvTm, vObjTm);
                  v_mat44_inverse43(vInvOldTm, vOldTm);
                  invReady = true;
                }

                float intersectionT = max_y * riTm.m[1][1];
                vec3f vFrom = vRiTm.col3;
                vec3f vDir = V_C_UNIT_0100;
                vec3f vTo = v_madd(vDir, v_splats(intersectionT), vFrom);

                vec3f a00 = v_mat44_mul_vec3p(vInvOldTm, vFrom);
                vec3f a01 = v_mat44_mul_vec3p(vInvOldTm, vTo);
                vec3f a10 = v_mat44_mul_vec3p(vInvTm, vFrom);
                vec3f a11 = v_mat44_mul_vec3p(vInvTm, vTo);

                Point3 point;
                int nodeIndex = -1;
                if (obj_res->traceQuad(filter, a00, a01, a10, a11, point, nodeIndex))
                {
                  if (collisionPoint)
                    *collisionPoint = point;
                  return true;
                }
              }
            }
          }
        }
    }
  return false;
}
bool testObjToRIGenIntersection(CollisionResource *obj_res, const CollisionNodeFilter &filter, const TMatrix &obj_tm,
  const Point3 &obj_vel, Point3 *intersected_obj_pos, bool *tree_sphere_intersected, Point3 *collisionPoint)
{
  bool ret = false;
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    if (testObjToRIGenIntersectionVel(_layer, obj_res, filter, obj_tm, obj_vel, intersected_obj_pos, tree_sphere_intersected,
          collisionPoint))
      ret = true;
  return ret;
}

static void fill_collision_info(RendInstGenData *rgl, const rendinst::RendInstDesc &ri_desc, const TMatrix &tm, const BBox3 &bbox,
  rendinst::CollisionInfo &out_coll_info)
{
  bool isRiExtra = ri_desc.isRiExtra();
  out_coll_info.riPoolRef = isRiExtra ? rendinst::riExtra[ri_desc.pool].riPoolRef : ri_desc.pool;
  if (isRiExtra && out_coll_info.riPoolRef >= 0)
    rgl = rendinst::getRgLayer(rendinst::riExtra[ri_desc.pool].riPoolRefLayer);
  // debug("fill_collision_info(%d, %d, %d, %d) -> %d, %d",
  //   ri_desc.cellIdx, ri_desc.idx, ri_desc.pool, ri_desc.offs, ri_desc.isRiExtra(), pool);

  out_coll_info.handle = ri_desc.isRiExtra() ? rendinst::riExtra[ri_desc.pool].collHandle
                                             : (rgl ? rgl->rtData->riCollRes[out_coll_info.riPoolRef].handle : nullptr); //-V781
  out_coll_info.collRes = ri_desc.isRiExtra() ? rendinst::riExtra[ri_desc.pool].collRes
                                              : (rgl ? rgl->rtData->riCollRes[out_coll_info.riPoolRef].collRes : nullptr);
  out_coll_info.tm = tm;
  out_coll_info.localBBox = bbox;
  out_coll_info.isImmortal = (rgl && out_coll_info.riPoolRef >= 0)
                               ? rgl->rtData->riProperties[out_coll_info.riPoolRef].immortal //-V781
                               : (isRiExtra && rendinst::riExtra[ri_desc.pool].immortal);
  out_coll_info.stopsBullets =
    (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riProperties[out_coll_info.riPoolRef].stopsBullets : true;
  out_coll_info.isDestr =
    (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riDestr[out_coll_info.riPoolRef].destructable : !isRiExtra;
  out_coll_info.destrImpulse =
    (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riDestr[out_coll_info.riPoolRef].destructionImpulse : 0;
  out_coll_info.hp = isRiExtra ? rendinst::riExtra[ri_desc.pool].getHp(ri_desc.idx) : 0.f;
  out_coll_info.initialHp = isRiExtra ? rendinst::riExtra[ri_desc.pool].initialHP : 0.f;
  out_coll_info.bushBehaviour =
    (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riProperties[out_coll_info.riPoolRef].bushBehaviour : false;
  out_coll_info.treeBehaviour =
    (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riProperties[out_coll_info.riPoolRef].treeBehaviour : false;
  out_coll_info.isParent = (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riDestr[out_coll_info.riPoolRef].isParent : false;
  out_coll_info.destructibleByParent =
    (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riDestr[out_coll_info.riPoolRef].destructibleByParent : false;
  out_coll_info.destroyNeighbourDepth =
    (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riDestr[out_coll_info.riPoolRef].destroyNeighbourDepth : 1;
  out_coll_info.destrFxId = (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riDestr[out_coll_info.riPoolRef].destrFxId : -1;
  out_coll_info.destrFxTemplate =
    (rgl && out_coll_info.riPoolRef >= 0) ? rgl->rtData->riDestr[out_coll_info.riPoolRef].destrFxTemplate : SimpleString();

  if (isRiExtra && out_coll_info.hp <= 0.f && !out_coll_info.isDestr)
    out_coll_info.isImmortal = true;
}

const char *get_rendinst_res_name_from_col_info(const CollisionInfo &col_info)
{
  RendInstGenData *rgl = rendinst::getRgLayer(col_info.desc.layer);

  if (
    rgl && col_info.riPoolRef >= 0 && col_info.riPoolRef < rgl->rtData->riResName.size() && rgl->rtData->riResName[col_info.riPoolRef])
    return rgl->rtData->riResName[col_info.riPoolRef];

  return "?";
}

CollisionInfo getRiGenDestrInfo(const RendInstDesc &desc)
{
  CollisionInfo ret(desc);
  G_ASSERT(desc.isValid());
  fill_collision_info(RendInstGenData::getGenDataByLayer(desc), desc, rendinst::getRIGenMatrixNoLock(desc),
    rendinst::getRIGenBBox(desc), ret);
  return ret;
}

struct CallbackAddStrat : public MaterialRayStrat
{
  rendinst::RendInstCollisionCB *callback;
  dag::RelocatableFixedVector<eastl::pair<bool, rendinst::CollisionInfo>, TraceMeshFaces::RendinstCache::MAX_ENTRIES> collInfoQueue;
  bool useQueue;

  // set use_queue to true if you want unlock ri mutexes before callback processing
  CallbackAddStrat(rendinst::RendInstCollisionCB *cb, bool use_queue, PhysMat::MatID ray_mat = PHYSMAT_INVALID) :
    MaterialRayStrat(ray_mat, true), callback(cb), useQueue(use_queue)
  {}

  ~CallbackAddStrat() { processQueue(); }

  CheckBoxRIResultFlags executeForPos(RendInstGenData *rgl, const rendinst::RendInstDesc &ri_desc, const TMatrix &tm,
    const BBox3 &bbox)
  {
    collInfoQueue.emplace_back(eastl::make_pair(false /*is_tm*/, rendinst::CollisionInfo(ri_desc)));
    fill_collision_info(rgl, ri_desc, tm, bbox, collInfoQueue.back().second);
    if (!useQueue)
    {
      callback->addTreeCheck(collInfoQueue.back().second);
      collInfoQueue.clear();
    }
    return {};
  }

  CheckBoxRIResultFlags executeForTm(RendInstGenData *rgl, const rendinst::RendInstDesc &ri_desc, mat44f_cref v_tm, const BBox3 &bbox,
    const CollisionResource *)
  {
    DECL_ALIGN16(TMatrix, tm);
    v_mat_43ca_from_mat44(&tm[0][0], v_tm);
    collInfoQueue.emplace_back(eastl::make_pair(true /*is_tm*/, rendinst::CollisionInfo(ri_desc)));
    fill_collision_info(rgl, ri_desc, tm, bbox, collInfoQueue.back().second);
    if (!useQueue)
    {
      callback->addCollisionCheck(collInfoQueue.back().second);
      collInfoQueue.clear();
    }
    return {};
  }

  CheckBoxRIResultFlags executeForPos(RendInstGenData *rgl, const rendinst::RendInstDesc &ri_desc, mat44f_cref v_tm, const BBox3 &bbox,
    const CollisionResource *)
  {
    DECL_ALIGN16(TMatrix, tm);
    v_mat_43ca_from_mat44(&tm[0][0], v_tm);
    return executeForPos(rgl, ri_desc, tm, bbox);
  }

  void processQueue()
  {
    for (auto &q : collInfoQueue)
    {
      if (q.first)
        callback->addCollisionCheck(q.second);
      else
        callback->addTreeCheck(q.second);
    }
    collInfoQueue.clear();
  }
};

struct CheckIntersectionStrat : public MaterialRayStrat
{
  CheckIntersectionStrat(PhysMat::MatID ray_mat = PHYSMAT_INVALID) : MaterialRayStrat(ray_mat, true) {}

  CheckBoxRIResultFlags executeForTm(RendInstGenData * /*rgl*/, const rendinst::RendInstDesc & /*ri_desc*/, mat44f_cref /*tm*/,
    const BBox3 & /*bbox*/, const CollisionResource *)
  {
    return CheckBoxRIResultFlag::HasTraceableRi | CheckBoxRIResultFlag::HasCollidableRi;
  }
  CheckBoxRIResultFlags executeForPos(RendInstGenData * /*rgl*/, const rendinst::RendInstDesc & /*ri_desc*/, mat44f_cref /*tm*/,
    const BBox3 & /*bbox*/, const CollisionResource *)
  {
    return CheckBoxRIResultFlag::HasCollidableRi;
  }
};

struct CacheAddStrat : public MaterialRayStrat
{
  TraceMeshFaces *riCache;
  bool valid;

  explicit CacheAddStrat(TraceMeshFaces *ri_cache) : MaterialRayStrat(PHYSMAT_INVALID, true), riCache(ri_cache), valid(true) {}

  CheckBoxRIResultFlags executeForTm(RendInstGenData * /*rgl*/, const rendinst::RendInstDesc &ri_desc, mat44f_cref v_tm,
    const BBox3 &bbox, const CollisionResource *)
  {
    return addToCacheInBox(ri_desc, v_tm, bbox, riCache->rendinstCache, false, CheckBoxRIResultFlag::All);
  }

  CheckBoxRIResultFlags executeForPos(RendInstGenData * /*rgl*/, const rendinst::RendInstDesc &ri_desc, mat44f_cref v_tm,
    const BBox3 &bbox, const CollisionResource *)
  {
    return addToCacheInBox(ri_desc, v_tm, bbox, riCache->rendinstCache, true, CheckBoxRIResultFlag::All);
  }

private:
  CheckBoxRIResultFlags addToCacheInBox(const rendinst::RendInstDesc &ri_desc, mat44f_cref v_tm, const BBox3 &bbox,
    TraceMeshFaces::RendinstCache &cache, bool is_pos, CheckBoxRIResultFlags overflow_result)
  {
    if (cache.isFull())
    {
      valid = false; // we don't have enough place for another one, we cannot use cache
      return overflow_result;
    }
    mat44f invTm;
    v_mat44_inverse43(invTm, v_tm);
    bbox3f v_bbox = v_ldu_bbox3(bbox);
    bbox3f invBox;
    v_bbox3_init(invBox, invTm, v_ldu_bbox3(riCache->box));
    if (!v_bbox3_test_box_intersect(v_bbox, invBox))
      return {};

    cache.add(ri_desc, is_pos);
    return {};
  }
};

template <typename T>
class VerifyReadLock
{
public:
  VerifyReadLock(T &_lock) : lock(_lock) {}
  ~VerifyReadLock() { G_ASSERT(!locked); }
  void lockRead() DAG_TS_ACQUIRE_SHARED(lock)
  {
    G_ASSERT(!locked);
    lock.lockRead();
    locked = true;
  }
  void unlockRead() DAG_TS_RELEASE_SHARED(lock)
  {
    G_ASSERT(locked);
    lock.unlockRead();
    locked = false;
  }
  bool isLocked() const { return locked; }

private:
  T &lock;
  bool locked = false;
};

template <typename bounding_type_t>
class BoundingsChecker
{
public:
  __forceinline BoundingsChecker(const bounding_type_t &bounding_a, bbox3f world_box_a) : //-V730
    boundingA(bounding_a), worldBoxA(world_box_a), isSmallA(isSmallBox(world_box_a)), itmInitedA(false)
  {}

  __forceinline bool testIntersection(mat44f tm_b, bbox3f local_box_b)
  {
    bbox3f worldBoxB;
    v_bbox3_init(worldBoxB, tm_b, local_box_b);

    if constexpr (eastl::is_same_v<bounding_type_t, BSphere3>)
    {
      vec3f centerA = v_ldu(&boundingA.c.x);
      if (!v_bbox3_test_sph_intersect(worldBoxB, centerA, v_set_x(boundingA.r2)))
        return false;

      mat44f itmB;
      v_mat44_inverse43(itmB, tm_b);
      vec3f lcA = v_mat44_mul_vec3p(itmB, centerA);
      vec4f maxScale = v_mat44_max_scale43_x(tm_b);
      bbox3f extB = local_box_b;
      v_bbox3_extend(extB, v_mul(v_splats(boundingA.r), maxScale));
      return v_bbox3_test_pt_inside(extB, lcA);
    }

    if constexpr (eastl::is_same_v<bounding_type_t, BBox3>)
    {
      if (!v_bbox3_test_box_intersect(worldBoxA, worldBoxB))
        return false;

      if (bool isSmallB = isSmallBox(worldBoxB))
        return true;
      if (isSmallA)
      {
        bbox3f aInB;
        mat44f itmB;
        v_mat44_inverse43(itmB, tm_b);
        v_bbox3_init(aInB, itmB, worldBoxA);
        return v_bbox3_test_box_intersect(aInB, local_box_b);
      }

      if (v_bbox3_test_trasformed_box_intersect(worldBoxA, local_box_b, tm_b))
        return true;

      mat44f itmB;
      v_mat44_inverse43(itmB, tm_b);
      if (v_bbox3_test_trasformed_box_intersect(local_box_b, worldBoxA, itmB))
        return true;
    }

    if constexpr (eastl::is_same_v<bounding_type_t, OrientedObjectBox>)
    {
      if (!v_bbox3_test_box_intersect(worldBoxA, worldBoxB))
        return false;

      if (DAGOR_UNLIKELY(!itmInitedA) && !isSmallA)
      {
        itmInitedA = true;
        v_mat44_inverse43(itmA, boundingA.tm);
      }
      bool isSmallB = isSmallBox(worldBoxB);
      if (isSmallA != isSmallB)
      {
        if (isSmallB)
        {
          bbox3f bInA;
          v_bbox3_init(bInA, itmA, worldBoxB);
          return v_bbox3_test_box_intersect(bInA, boundingA.bbox);
        }
        else
        {
          bbox3f aInB;
          mat44f itmB;
          v_mat44_inverse43(itmB, tm_b);
          v_bbox3_init(aInB, itmB, worldBoxA);
          return v_bbox3_test_box_intersect(aInB, local_box_b);
        }
      }
      if (isSmallA) // two small objects
        return true;

      mat44f tmBtoA;
      v_mat44_mul43(tmBtoA, itmA, tm_b);
      if (v_bbox3_test_trasformed_box_intersect(boundingA.bbox, local_box_b, tmBtoA))
        return true;

      mat44f tmAtoB, itmB;
      v_mat44_inverse43(itmB, tm_b);
      v_mat44_mul43(tmAtoB, itmB, boundingA.tm);
      if (v_bbox3_test_trasformed_box_intersect(local_box_b, boundingA.bbox, tmAtoB))
        return true;
    }
    if constexpr (eastl::is_same_v<bounding_type_t, Capsule>)
    {
      if (!v_bbox3_test_box_intersect(boundingA.getBoundingBox(), worldBoxB))
        return false;
      bbox3f extB = worldBoxB;
      v_bbox3_extend(extB, v_splats(boundingA.r));
      return v_test_segment_box_intersection(v_ldu(&boundingA.a.x), v_ldu(&boundingA.b.x), extB);
    }
    return false;
  }

private:
  static bool isSmallBox(bbox3f bbox)
  {
    const float smallObjWidth = 1.6f;
    return (v_signmask(v_cmp_lt(v_bbox3_size(bbox), v_splats(smallObjWidth))) & 0b101) == 0b101;
  }

  const bounding_type_t &boundingA;
  mat44f itmA;
  bbox3f worldBoxA;
  bool isSmallA;
  bool itmInitedA;
};

// NOTE: the looped lockign logic is insanely complicated, way too much even for humans, static analysis has no chance.
template <typename Strategy, typename bounding_type_t>
static CheckBoxRIResultFlags testObjToRIGenIntersectionNoCache(int layer, const bounding_type_t &obj_bounding,
  const BBox3 &obj_world_aabb, GatherRiTypeFlags ri_types, Strategy &strategy, bool unlock_in_cb) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  G_ASSERT(!v_bbox3_is_empty(v_ldu_bbox3(obj_world_aabb)));
#if !_TARGET_PC_TOOLS_BUILD
  // it shouldn't be significantly larger than phys size of tested object if use test collision by sphere cast (perf reasons)
  G_ASSERTF(!unlock_in_cb || obj_world_aabb.width().lengthSq() <= sqr(500.f), "Test intersection with oversize box width %f",
    obj_world_aabb.width().length());
#endif
  RendInstGenData *rgl = rendinst::rgLayer[layer];
  if (DAGOR_UNLIKELY(!rgl || !rgl->rtData))
    return {};

  TIME_PROFILE_DEV(testObjToRIGenIntersection_NoCache);

  CheckBoxRIResultFlags result;
  unsigned testedNum = 0, trianglesNum = 0;
  G_UNUSED(testedNum + trianglesNum);
  BoundingsChecker objectBounding(obj_bounding, v_ldu_bbox3(obj_world_aabb));

  rendinst::riex_collidable_t riexHandles;
  if (ri_types & GatherRiTypeFlag::RiExtraOnly && rendinst::isRgLayerPrimary(layer))
  {
    VerifyReadLock lock(rendinst::ccExtra);
    lock.lockRead();
    if constexpr (eastl::is_same_v<bounding_type_t, BSphere3>)
      gatherRIGenExtraCollidable(riexHandles, obj_bounding, false /*read_lock*/);
    if constexpr (eastl::is_same_v<bounding_type_t, BBox3> || eastl::is_same_v<bounding_type_t, OrientedObjectBox>)
      gatherRIGenExtraCollidable(riexHandles, obj_world_aabb, false /*read_lock*/);
    if constexpr (eastl::is_same_v<bounding_type_t, Capsule>)
      gatherRIGenExtraCollidable(riexHandles, obj_bounding, false /*read_lock*/);
    bool ignoreRi = false;
    int prevRiType = -1;

    for (rendinst::riex_handle_t handle : riexHandles)
    {
      if (!lock.isLocked()) // lock again if was unlocked during callback
        lock.lockRead();

      uint32_t riIdx = rendinst::handle_to_ri_inst(handle);
      uint32_t riType = rendinst::handle_to_ri_type(handle);
      const RiExtraPool &pool = riExtra[riType];
      if (riType != prevRiType)
      {
        prevRiType = riType;
        int poolRef = pool.riPoolRef;
        if (RendInstGenData *rgl = (poolRef >= 0) ? rendinst::getRgLayer(pool.riPoolRefLayer) : nullptr)
        {
          const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[poolRef];
          ignoreRi = strategy.shouldIgnoreRendinst(/*isPos*/ false, riProp.immortal, riProp.matId);
        }
        else
          ignoreRi = false;
      }
      if (ignoreRi)
        continue;

      mat44f riTm;
      v_mat43_transpose_to_mat44(riTm, pool.riTm[riIdx]);
      if (!objectBounding.testIntersection(riTm, pool.collRes->vFullBBox))
        continue;

      rendinst::RendInstDesc riDesc(-1, riIdx, riType, 0, layer);
      if (unlock_in_cb)
        lock.unlockRead();
      result |= strategy.executeForTm(rgl, riDesc, riTm, pool.collRes->boundingBox, pool.collRes);
      testedNum++;
#if DA_PROFILER_ENABLED
      trianglesNum += pool.collRes->getTrianglesCount(CollisionNode::PHYS_COLLIDABLE);
#endif
      if (result & CheckBoxRIResultFlag::HasTraceableRi)
        break;
    }
    if (lock.isLocked())
      lock.unlockRead();
  }

  G_ASSERTF(testedNum <= max_number_of_rendinst_collision_callbacks, "Tested (%d) more then allowed limit of %d @ pos=%@ wabb=(%@,%@)",
    testedNum, max_number_of_rendinst_collision_callbacks, obj_world_aabb.center(), obj_world_aabb[0], obj_world_aabb[1]);

  if (result & CheckBoxRIResultFlag::HasTraceableRi || !(ri_types & GatherRiTypeFlag::RiGenOnly))
  {
#if DA_PROFILER_ENABLED
    if constexpr (eastl::is_same_v<bounding_type_t, BSphere3>)
      DA_PROFILE_TAG(coll_tests, ": sph r %.1f; tested %u; %u tris", obj_bounding.r, testedNum, trianglesNum);
    if constexpr (eastl::is_same_v<bounding_type_t, BBox3> || eastl::is_same_v<bounding_type_t, OrientedObjectBox>)
      DA_PROFILE_TAG(coll_tests, ": box r %.1f; tested %u; %u tris", obj_world_aabb.width().length() / 2.f, testedNum, trianglesNum);
    if constexpr (eastl::is_same_v<bounding_type_t, Capsule>)
      DA_PROFILE_TAG(coll_tests, ": cap %.1f r %.1f; tested %u; %u tris", length(obj_bounding.b - obj_bounding.a), obj_bounding.r,
        testedNum, trianglesNum);
#endif
    return result; //-V1020 The function exited without calling the 'lock.unlockRead' function.
  }

  vec4f objBox2d = v_perm_xzac(v_ldu(&obj_world_aabb.boxMin().x), v_ldu(&obj_world_aabb.boxMax().x));
  vec4f regionV = v_sub(objBox2d, rgl->world0Vxz);

  // negate bbox
  vec4f expandCellsRange = v_neg(v_perm_xzac(v_max(v_sub(rgl->rtData->maxCellBbox.bmax, v_splats(rgl->cellSz)), v_zero()),
    v_min(rgl->rtData->maxCellBbox.bmin, v_zero())));
  regionV = v_add(regionV, expandCellsRange);

  // add -1, -1, 1, 1 to expand to neighbours. If it doesn't intersect we will get early exits from
  // test of bbox against the cell itself
  regionV = v_max(v_add(v_mul(regionV, rgl->invGridCellSzV), v_make_vec4f(-1, -1, 1, 1)), v_zero());
  regionV = v_min(regionV, rgl->lastCellXZXZ);
  vec4i regionI = v_cvt_floori(regionV);
  DECL_ALIGN16(int, regions[4]);
  v_sti(regions, regionI);

  float cellSz = (rgl->grid2world * rgl->cellSz);
  float invSubcellSz = SUBCELL_DIV / cellSz;
  vec4f v_invSubcellSz = v_splats(invSubcellSz);
#if DAGOR_DBGLEVEL > 0
  if (regions[2] >= rgl->cellNumW || regions[3] >= rgl->cellNumH || regions[0] < 0 || regions[1] < 0)
  {
    DAG_FATAL("invalid region (%d,%d)-(%d,%d), build from (%f,%f)-(%f,%f) box, cellNum=(%d,%d)", regions[0], regions[1], regions[2],
      regions[3], obj_world_aabb.boxMin().x, obj_world_aabb.boxMin().z, obj_world_aabb.boxMax().x, obj_world_aabb.boxMax().z,
      rgl->cellNumW, rgl->cellNumH);
  }
#endif

  VerifyReadLock lock(rgl->rtData->riRwCs);
  lock.lockRead();

  const eastl::BitvectorWordType *riPosInstData = rgl->rtData->riPosInst.data();
  const eastl::BitvectorWordType *riPaletteRotationData = rgl->rtData->riPaletteRotation.data();
  int cellXStride = rgl->cellNumW - (regions[2] - regions[0] + 1);
  if (!rgl->rtData->riProperties.empty()) // we cannot run on empty properties. In most cases it's just a second layer
  {
    for (int z = regions[1], cellI = regions[1] * rgl->cellNumW + regions[0]; z <= regions[3]; z++, cellI += cellXStride)
    {
      for (int x = regions[0]; x <= regions[2]; x++, cellI++)
      {
        RendInstGenData::Cell &cell = rgl->cells[cellI];
        const RendInstGenData::CellRtData *crt_ptr = cell.isReady();
        if (!crt_ptr)
          continue;
        const RendInstGenData::CellRtData &crt = *crt_ptr;

        if (!v_bbox3_test_box_intersect(crt.bbox[0], v_ldu_bbox3(obj_world_aabb)))
          continue;
        vec3f v_cell_add = crt.cellOrigin;
        vec3f v_cell_mul = v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(cellSz, crt.cellHeight, cellSz, 0));

        int pcnt = crt.pools.size();
        // crt.bbox[0] and obj_world_aabb intersects, so we can merge them
        vec4f cellBox2d = v_perm_xzac(crt.bbox[0].bmin, crt.bbox[0].bmax);
        vec4f isectBox2d = v_perm_xycd(v_max(objBox2d, cellBox2d), v_min(objBox2d, cellBox2d));

        isectBox2d = v_sub(isectBox2d, v_perm_xzxz(v_cell_add));
        isectBox2d = v_add(v_mul(isectBox2d, v_invSubcellSz), v_make_vec4f(-1, -1, 1, 1));
        isectBox2d = v_max(isectBox2d, v_zero());
        isectBox2d = v_min(isectBox2d, v_ld(v_SUBCELL_DIV_MAX));
        vec4i isectBox2di = v_cvt_vec4i(isectBox2d); // it is the same as floor, since it is always positive!
        DECL_ALIGN16(int, subCell[4]);               // xzXZ
        v_sti(subCell, isectBox2di);

        for (int stride_subCell = SUBCELL_DIV - (subCell[2] - subCell[0] + 1), idx = subCell[1] * SUBCELL_DIV + subCell[0];
             subCell[1] <= subCell[3]; subCell[1]++, idx += stride_subCell)
        {
          for (int x2 = subCell[0]; x2 <= subCell[2]; x2++, idx++)
          {
            if (!v_bbox3_test_box_intersect(crt.bbox[idx + 1], v_ldu_bbox3(obj_world_aabb)))
              continue;

            eastl::BitvectorWordType riPosInstBit = 1;
            for (int p = 0; DAGOR_LIKELY(p < pcnt); p++, riPosInstBit = roll_bit(riPosInstBit))
            {
              const RendInstGenData::CellRtData::SubCellSlice &scs = crt.getCellSlice(p, idx);
              if (DAGOR_LIKELY(!scs.sz))
                continue;
              const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[p];
              bool isPosInst = (riPosInstData[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0;
              if (strategy.shouldIgnoreRendinst(isPosInst, riProp.immortal, riProp.matId) || !scs.sz)
                continue;
              CollisionResource *collRes = rgl->rtData->riCollRes[p].collRes;
              if (!collRes && strategy.isCollisionRequired())
                continue;
              bool paletteRotation = (riPaletteRotationData[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0;

              const bbox3f v_collResBox = collRes ? collRes->vFullBBox : rgl->rtData->riCollResBb[p];

              if (!isPosInst)
              {
                const int16_t *data_s = (const int16_t *)(crt.sysMemData.get() + scs.ofs);
                int stride_w =
                  RIGEN_TM_STRIDE_B((rgl->rtData->riZeroInstSeeds.data()[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0,
                    rgl->perInstDataDwords) /
                  2;
                for (const int16_t *data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
                {
                  mat44f riTm;
                  if (!rendinst::gen::unpack_tm_full(riTm, data, v_cell_add, v_cell_mul))
                    continue;
                  if (!objectBounding.testIntersection(riTm, v_collResBox))
                    continue;

                  BBox3 localBBox;
                  v_stu_bbox3(localBBox, v_collResBox);

                  rendinst::RendInstDesc riDesc(cellI, idx, p, int(intptr_t(data) - intptr_t(data_s)), layer);
                  result |= strategy.executeForTm(rgl, riDesc, riTm, localBBox, collRes);
                  testedNum++;
#if DA_PROFILER_ENABLED
                  trianglesNum += collRes ? collRes->getTrianglesCount(CollisionNode::PHYS_COLLIDABLE) : 0;
#endif
                  if (result & CheckBoxRIResultFlag::HasTraceableRi)
                    goto done;
                }
              }
              else if (!(result & CheckBoxRIResultFlag::HasCollidableRi))
              {
                BBox3 localBBox;
                v_stu_bbox3(localBBox, v_collResBox);

                const int16_t *data_s = (int16_t *)(crt.sysMemData.get() + scs.ofs);
                int stride_w =
                  RIGEN_POS_STRIDE_B((rgl->rtData->riZeroInstSeeds.data()[p / (sizeof(riPosInstBit) * CHAR_BIT)] & riPosInstBit) != 0,
                    rgl->perInstDataDwords) /
                  2;
                if (paletteRotation)
                {
                  rendinst::gen::RotationPaletteManager::Palette palette =
                    rendinst::gen::get_rotation_palette_manager()->getPalette({layer, p});

                  for (const int16_t *data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
                  {
                    vec3f v_pos, v_scale;
                    vec4i paletteId;
                    if (!rendinst::gen::unpack_tm_pos(v_pos, v_scale, data, v_cell_add, v_cell_mul, paletteRotation, &paletteId))
                      continue;
                    quat4f v_rot = rendinst::gen::RotationPaletteManager::get_quat(palette, v_extract_xi(paletteId));
                    mat44f tm;
                    v_mat44_compose(tm, v_pos, v_rot, v_scale);
                    bbox3f treeBBox;
                    v_bbox3_init(treeBBox, tm, v_collResBox);

                    if (!v_bbox3_test_box_intersect(treeBBox, v_ldu_bbox3(obj_world_aabb)))
                      continue;

                    rendinst::RendInstDesc riDesc(cellI, idx, p, int(intptr_t(data) - intptr_t(data_s)), layer);
                    result |= strategy.executeForPos(rgl, riDesc, tm, localBBox, collRes);
                    testedNum++;
#if DA_PROFILER_ENABLED
                    trianglesNum += collRes ? collRes->getTrianglesCount(CollisionNode::PHYS_COLLIDABLE) : 0;
#endif
                    if (result & CheckBoxRIResultFlag::HasTraceableRi)
                      goto done;
                  }
                }
                else
                {
                  vec3f v_tree_min = v_collResBox.bmin;
                  vec3f v_tree_max = v_collResBox.bmax;

                  for (const int16_t *data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
                  {
                    vec3f v_pos, v_scale;
                    if (!rendinst::gen::unpack_tm_pos(v_pos, v_scale, data, v_cell_add, v_cell_mul, paletteRotation))
                      continue;
                    bbox3f treeBBox;
                    treeBBox.bmin = v_add(v_pos, v_mul(v_scale, v_tree_min));
                    treeBBox.bmax = v_add(v_pos, v_mul(v_scale, v_tree_max));

                    if (!v_bbox3_test_box_intersect(treeBBox, v_ldu_bbox3(obj_world_aabb)))
                      continue;

                    rendinst::RendInstDesc riDesc(cellI, idx, p, int(intptr_t(data) - intptr_t(data_s)), layer);
                    mat44f tm;
                    v_mat44_compose(tm, v_pos, v_scale);
                    result |= strategy.executeForPos(rgl, riDesc, tm, localBBox, collRes);
                    testedNum++;
#if DA_PROFILER_ENABLED
                    trianglesNum += collRes ? collRes->getTrianglesCount(CollisionNode::PHYS_COLLIDABLE) : 0;
#endif
                    if (result & CheckBoxRIResultFlag::HasTraceableRi)
                      goto done;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
done:
  lock.unlockRead();
  G_ASSERTF(testedNum <= max_number_of_rendinst_collision_callbacks, "Tested (%d) more then allowed limit of %d @ pos=%@ wabb=(%@,%@)",
    testedNum, max_number_of_rendinst_collision_callbacks, obj_world_aabb.center(), obj_world_aabb[0], obj_world_aabb[1]);
#if DA_PROFILER_ENABLED
  DA_PROFILE_TAG(coll_tests, ": r %.1f; tested %u; %u tris", obj_world_aabb.width().length() / 2.f, testedNum, trianglesNum);
#endif
  return result;
}

struct GatherCollidableInSphereStrat
{
  Point3 center;
  float radius;
  rigen_collidable_data_t &collidableData;

  GatherCollidableInSphereStrat(const Point3 &pos, float rad, rigen_collidable_data_t &data) :
    center(pos), radius(rad), collidableData(data)
  {}
  bool shouldIgnoreRendinst(bool /*is_pos*/, bool /*is_immortal */, PhysMat::MatID /*mat_id*/) const { return false; }
  inline bool isCollisionRequired() const { return true; }

  CheckBoxRIResultFlags add(RendInstGenData *rgl, const rendinst::RendInstDesc &desc, mat44f_cref tm, const BBox3 &bbox,
    const CollisionResource *collres)
  {
    if (v_bbox3_test_sph_intersect(v_ldu_bbox3(bbox), v_ldu(&center.x), v_set_x(sqr(radius))))
    {
      RiGenCollidableData *data = (RiGenCollidableData *)collidableData.push_back_uninitialized();
      v_mat_44cu_from_mat44(data->tm.array, tm);
      data->collres = collres;
      data->desc = desc;
      data->immortal = rgl->rtData->riProperties[desc.pool].immortal;
      data->dist = v_extract_x(v_length3_x(v_sub(tm.col3, v_ldu(&center.x))));
      G_ASSERTF(!desc.isRiExtra(), "Fix code above for riExtra support");
    }
    return {};
  }

  CheckBoxRIResultFlags executeForTm(RendInstGenData *rgl, const rendinst::RendInstDesc &desc, mat44f_cref tm, const BBox3 &bbox,
    const CollisionResource *collres)
  {
    return add(rgl, desc, tm, bbox, collres);
  }

  CheckBoxRIResultFlags executeForPos(RendInstGenData *rgl, const rendinst::RendInstDesc &desc, mat44f_cref tm, const BBox3 &bbox,
    const CollisionResource *collres)
  {
    return add(rgl, desc, tm, bbox, collres);
  }
};

void gatherRIGenCollidableInRadius(rigen_collidable_data_t &out_data, const Point3 &pos, float radius, GatherRiTypeFlags flags)
{
  if (flags & GatherRiTypeFlag::RiExtraOnly)
    rendinst::gatherRIGenExtraCollidable(out_data, pos, radius, true /*lock*/);
  GatherCollidableInSphereStrat strat(pos, radius, out_data);
  BSphere3 sph(pos, radius);
  BBox3 bbox(sph);
  if (flags & GatherRiTypeFlag::RiGenOnly)
  {
    FOR_EACH_RG_LAYER_DO (rgl)
      testObjToRIGenIntersectionNoCache(_layer, sph, bbox, GatherRiTypeFlag::RiGenOnly, strat, false);
  }
}

struct RiClipCapsuleCallback : public rendinst::RendInstCollisionCB
{
  const Capsule &c;
  Point3 &lpt, &wpt;
  real &md;
  const Point3 &movedirNormalized;
  RiClipCapsuleCallback(const Capsule &c, Point3 &lpt, Point3 &wpt, real &md, const Point3 &movedirNormalized) :
    c(c), lpt(lpt), wpt(wpt), md(md), movedirNormalized(movedirNormalized)
  {}

  void addCollisionCheck(const CollisionInfo &coll_info) override
  {
    if (coll_info.collRes)
      coll_info.collRes->clipCapsule(coll_info.tm, c, lpt, wpt, md, movedirNormalized);
  }
  void addTreeCheck(const CollisionInfo &) override {}
};

void clipCapsuleRI(const ::Capsule &c, Point3 &lpt, Point3 &wpt, real &md, const Point3 &movedirNormalized,
  const TraceMeshFaces *ri_cache)
{
  if (ri_cache && ri_cache->isRendinstsValid)
  {
    ri_cache->rendinstCache.foreachValid(rendinst::GatherRiTypeFlag::RiGenTmAndExtra,
      [&](const rendinst::RendInstDesc &ri_desc, bool) {
        RendInstGenData *v = RendInstGenData::getGenDataByLayer(ri_desc);
        if (!v)
          return;
        CollisionResource *collRes = v->rtData->riCollRes[ri_desc.pool].collRes;
        if (!collRes)
          return;

        TMatrix tm = getRIGenMatrixNoLock(ri_desc);
        collRes->clipCapsule(tm, c, lpt, wpt, md, movedirNormalized);
      });
  }
  else
  {
    RiClipCapsuleCallback callback(c, lpt, wpt, md, movedirNormalized);
    CallbackAddStrat strat(&callback, false /*use_queue*/, -1);

    BBox3 wbb = c.getBoundingBoxScalar();
    FOR_EACH_RG_LAYER_DO (rgl)
      testObjToRIGenIntersectionNoCache(_layer, wbb, wbb, GatherRiTypeFlag::RiGenAndExtra, strat, false);
  }
}

template <typename bounding_type_t>
static void testObjToRIGenIntersectionInternal(const bounding_type_t &bounding, rendinst::RendInstCollisionCB &callback,
  GatherRiTypeFlags ri_types, const TraceMeshFaces *ri_cache, PhysMat::MatID ray_mat, bool unlock_in_cb)
{
  if (!rendinst::rgLayer.size() || !rendinst::rgLayer[0])
    return;

  bbox3f objFullBBox;
  v_bbox3_init_empty(objFullBBox);
  if constexpr (eastl::is_same_v<bounding_type_t, BSphere3>)
    v_bbox3_init_by_bsph(objFullBBox, v_ldu(&bounding.c.x), v_splats(bounding.r));
  if constexpr (eastl::is_same_v<bounding_type_t, BBox3>)
    objFullBBox = v_ldu_bbox3(bounding);
  if constexpr (eastl::is_same_v<bounding_type_t, OrientedObjectBox>)
    v_bbox3_init(objFullBBox, bounding.tm, bounding.bbox);
  if constexpr (eastl::is_same_v<bounding_type_t, Capsule>)
    objFullBBox = bounding.getBoundingBox();
  G_ASSERT(!v_bbox3_is_empty(objFullBBox));

  // To consider: extract this code path to separate function
  if (ri_cache && v_bbox3_test_box_inside(v_ldu_bbox3(ri_cache->box), objFullBBox))
  {
    TIME_PROFILE_DEV(testObjToRIGenIntersection_Cache);
    StaticTab<rendinst::CollisionInfo, TraceMeshFaces::RendinstCache::MAX_ENTRIES> collInfoQueue;
    bool isCacheValid = false;
    {
      AutoLockReadPrimaryAndExtra lockRead;
      if (check_cached_ri_data(ri_cache))
      {
        isCacheValid = true;
        BoundingsChecker objectBounding(bounding, objFullBBox);
#if DA_PROFILER_ENABLED
        int testedNum = 0, trianglesNum = 0;
#endif
        bool isRiCollide = false;
        int prevRiType = -1;
        ri_cache->rendinstCache.foreachValid(ri_types, [&](const rendinst::RendInstDesc &ri_desc, bool is_ri_extra) {
          if (ray_mat != PHYSMAT_INVALID)
          {
            if (ri_desc.pool != prevRiType)
            {
              prevRiType = ri_desc.pool;
              int riMat = rendinst::getRIGenMaterialId(ri_desc, false /*lock*/);
              isRiCollide = riMat == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat, riMat);
            }
            if (!isRiCollide)
              return;
          }

          BBox3 riBox = getRIGenBBox(ri_desc);
          TMatrix tm = getRIGenMatrixNoLock(ri_desc);

          mat44f riTm;
          v_mat44_make_from_43cu_unsafe(riTm, tm.array);
          if (!objectBounding.testIntersection(riTm, v_ldu_bbox3(riBox)))
            return;

          rendinst::CollisionInfo &collInfo = collInfoQueue.emplace_back(ri_desc);
          RendInstGenData *rgl = rendinst::rgLayer[ri_desc.layer];
          fill_collision_info(rgl, ri_desc, tm, riBox, collInfo);

          if (!unlock_in_cb)
          {
            bool isPosRi = !is_ri_extra && rgl->rtData->riPosInst[ri_desc.pool];
            if (!isPosRi)
              callback.addCollisionCheck(collInfo);
            else
              callback.addTreeCheck(collInfo);
            collInfoQueue.clear();
          }

#if DA_PROFILER_ENABLED
          testedNum++;
          trianglesNum += collInfo.collRes->getTrianglesCount(CollisionNode::PHYS_COLLIDABLE);
#endif
        });
#if DA_PROFILER_ENABLED
        if constexpr (eastl::is_same_v<bounding_type_t, BSphere3>)
          DA_PROFILE_TAG(coll_tests, ": sph r %.1f; tested %u/%u; %u tris", bounding.r, testedNum, ri_cache->rendinstCache.size(),
            trianglesNum);
        if constexpr (eastl::is_same_v<bounding_type_t, BSphere3> || eastl::is_same_v<bounding_type_t, OrientedObjectBox>)
          DA_PROFILE_TAG(coll_tests, ": box r %.1f; tested %u/%u; %u tris", v_extract_x(v_length3_x(v_bbox3_size(objFullBBox))) / 2.f,
            testedNum, ri_cache->rendinstCache.size(), trianglesNum);
        if constexpr (eastl::is_same_v<bounding_type_t, Capsule>)
          DA_PROFILE_TAG(coll_tests, ": cap %.1f r %.1f; tested %u/%u; %u tris", length(bounding.b - bounding.a), bounding.r,
            testedNum, ri_cache->rendinstCache.size(), trianglesNum);
#endif
      }
      else
      {
        BBox3 bb;
        v_stu_bbox3(bb, objFullBBox);
        trace_utils::draw_trace_handle_debug_cast_result(ri_cache, bb, false, true);
      }
      // end of lock scope
    }

    // Execute delayed callbacks with released ri locks
    if (isCacheValid)
    {
      for (const rendinst::CollisionInfo &collInfo : collInfoQueue)
      {
        RendInstGenData *rgl = rendinst::rgLayer[collInfo.desc.layer];
        bool isPosRi = !collInfo.desc.isRiExtra() && rgl->rtData->riPosInst[collInfo.desc.pool];
        if (!isPosRi)
          callback.addCollisionCheck(collInfo);
        else
          callback.addTreeCheck(collInfo);
      }
      return;
    }
  }

  BBox3 aabb;
  v_stu_bbox3(aabb, objFullBBox);
  CallbackAddStrat strat(&callback, unlock_in_cb, ray_mat); // strat callback should be always executed with lock
  FOR_EACH_RG_LAYER_DO (rgl)
    testObjToRIGenIntersectionNoCache(_layer, bounding, aabb, ri_types, strat, false /*unlock_in_cb*/);
}

void testObjToRIGenIntersection(const BSphere3 &obj_sph, RendInstCollisionCB &callback, GatherRiTypeFlags ri_types,
  const TraceMeshFaces *ri_cache, PhysMat::MatID ray_mat, bool unlock_in_cb)
{
  testObjToRIGenIntersectionInternal(obj_sph, callback, ri_types, ri_cache, ray_mat, unlock_in_cb);
}

void testObjToRIGenIntersection(const BBox3 &obj_box, RendInstCollisionCB &callback, GatherRiTypeFlags ri_types,
  const TraceMeshFaces *ri_cache, PhysMat::MatID ray_mat, bool unlock_in_cb)
{
  testObjToRIGenIntersectionInternal(obj_box, callback, ri_types, ri_cache, ray_mat, unlock_in_cb);
}

void testObjToRIGenIntersection(const BBox3 &obj_box, const TMatrix &obj_tm, RendInstCollisionCB &callback, GatherRiTypeFlags ri_types,
  const TraceMeshFaces *ri_cache, PhysMat::MatID ray_mat, bool unlock_in_cb)
{
  OrientedObjectBox obb;
  v_mat44_make_from_43cu_unsafe(obb.tm, obj_tm.array);
  obb.bbox = v_ldu_bbox3(obj_box);
  testObjToRIGenIntersectionInternal(obb, callback, ri_types, ri_cache, ray_mat, unlock_in_cb);
}

void testObjToRIGenIntersection(const Capsule &obj_capsule, RendInstCollisionCB &callback, GatherRiTypeFlags ri_types,
  const TraceMeshFaces *ri_cache, PhysMat::MatID ray_mat, bool unlock_in_cb)
{
  testObjToRIGenIntersectionInternal(obj_capsule, callback, ri_types, ri_cache, ray_mat, unlock_in_cb);
}

CheckBoxRIResultFlags checkBoxToRIGenIntersection(const BBox3 &box)
{
  CheckIntersectionStrat strat;
  CheckBoxRIResultFlags testRes = {};
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    CheckBoxRIResultFlags res =
      testObjToRIGenIntersectionNoCache(_layer, box, box, GatherRiTypeFlag::RiGenAndExtra, strat, false /*unlock_in_cb*/);
    if (rendinst::isRgLayerPrimary(_layer))
    {
      if (res & CheckBoxRIResultFlag::HasTraceableRi)
        return res;
    }
    else
      res &= CheckBoxRIResultFlag::HasCollidableRi;
    testRes |= res;
  }
  return testRes;
}

struct ForeachRIGenStrat : public MaterialRayStrat
{
  ForeachCB &cb;

  ForeachRIGenStrat(ForeachCB &_cb) : MaterialRayStrat(PHYSMAT_INVALID, true), cb(_cb) {}

  inline bool isCollisionRequired() const { return false; }

  CheckBoxRIResultFlags executeForTm(RendInstGenData *rgl, const rendinst::RendInstDesc &ri_desc, mat44f_cref v_tm, const BBox3 &,
    const CollisionResource *)
  {
    DECL_ALIGN16(TMatrix, tm);
    v_mat_43ca_from_mat44(&tm[0][0], v_tm);
    cb.executeForTm(rgl, ri_desc, tm);
    return {};
  }

  CheckBoxRIResultFlags executeForPos(RendInstGenData *rgl, const rendinst::RendInstDesc &ri_desc, mat44f_cref v_tm, const BBox3 &,
    const CollisionResource *)
  {
    DECL_ALIGN16(TMatrix, tm);
    v_mat_43ca_from_mat44(&tm[0][0], v_tm);
    cb.executeForPos(rgl, ri_desc, tm);
    return {};
  }
};

void foreachRIGenInBox(const BBox3 &box, GatherRiTypeFlags ri_types, ForeachCB &cb)
{
  ForeachRIGenStrat strat(cb);
  FOR_EACH_RG_LAYER_DO (rgl)
    testObjToRIGenIntersectionNoCache(_layer, box, box, ri_types, strat, false /*unlock_in_cb*/);
}

bool initializeCachedRiData(TraceMeshFaces *ri_cache)
{
  if (!rendinst::rgPrimaryLayers || !rendinst::rgLayer[0])
    return false;

  TIME_PROFILE(initializeCachedRiData);
  ri_cache->rendinstCache.clear();

  // World version is updated separately (riRwCs is NOT held), so
  // we can't be 100% sure that RIs we collect here are of 'ver', but
  // they're AT LEAST of 'ver'. This means if the world gets invalidated
  // we'll know that in 'world_version_check' later and act properly.
  int ver = riutil::world_version_get();

  CacheAddStrat strat(ri_cache);
  mat44f identTm;
  v_mat44_ident(identTm);
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    testObjToRIGenIntersectionNoCache(_layer, ri_cache->box, ri_cache->box, GatherRiTypeFlag::RiGenAndExtra, strat,
      false /*unlock_in_cb*/);
  if (strat.valid)
  {
    // only update cache's version if it fully succeeded, partial caches
    // don't count, we can't use them!
    ri_cache->rendinstCache.version = ver;
    ri_cache->rendinstCache.sort();
  }
  else
  {
    // clean it up just in case, we should never use these anyway.
    ri_cache->rendinstCache.clear();
  }
  return strat.valid;
}

} // namespace rendinst

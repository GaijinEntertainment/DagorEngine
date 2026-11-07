// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameMath/collisionSurfaceGrid.h>

#include <EASTL/bitvector.h>
#include <EASTL/sort.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_geomTree.h>
#include <math/dag_mathUtils.h>
#include <vecmath/dag_vecMath.h>

static constexpr float BARYCENTRIC_EPSILON = 1e-5f;
static constexpr float DUPLICATE_CROSSING_DT = 1e-5f;
static constexpr float MIN_COVERAGE_WEIGHT = 0.5f;
static constexpr int MAX_TRIANGLE_COUNT = 0xFFFF;

static unsigned sample_block_lanes(int block, int i0, int i1)
{
  const int first = max(i0 - block * 4, 0);
  const int last = min(i1 - block * 4, 3);
  return ((1u << (last + 1)) - 1u) & ~((1u << first) - 1u);
}

// getCollisionNodeTm adds the world translation before the ray origin can be taken off, which rounds each node at world scale, so
// the tree branch of CollisionResource::getPosedNodeWtmInline is repeated here with the origin as its offset: keep the two equal.
static void get_origin_relative_node_tm(const CollisionResource &coll_res, const CollisionNode &node, const mat44f &origin_relative_tm,
  const GeomNodeTree *geom_node_tree, vec3f origin, mat44f &out_tm)
{
  if (!geom_node_tree || node.geomNodeId.index() >= geom_node_tree->nodeCount())
  {
    coll_res.getCollisionNodeTm(&node, origin_relative_tm, geom_node_tree, out_tm);
    return;
  }
  out_tm = geom_node_tree->getNodeWtmRel(node.geomNodeId);
  out_tm.col3 = v_add(out_tm.col3, v_sub(geom_node_tree->getWtmOfs(), origin));
  TMatrix relGeomNodeTm;
  if (coll_res.getRelGeomNodeTms(node.nodeIndex, relGeomNodeTm))
  {
    mat44f relTm;
    v_mat44_make_from_43cu_unsafe(relTm, relGeomNodeTm.array);
    v_mat44_mul43(out_tm, out_tm, relTm);
  }
}

VECTORCALL void make_ray_side_axes(vec3f dir, vec3f &out_side_x, vec3f &out_side_y)
{
  vec3f sideX = v_make_vec4f(v_extract_z(dir), 0.f, -v_extract_x(dir), 0.f);
  if (v_extract_x(v_length3_sq_x(sideX)) == 0.f)
    sideX = v_make_vec4f(v_extract_y(dir), -v_extract_x(dir), 0.f, 0.f);
  out_side_x = v_norm3(sideX);
  out_side_y = v_cross3(dir, out_side_x);
}

bool CollisionSurfaceGrid::supports(const CollisionResource &coll_res)
{
  return coll_res.boxNodes().empty() && coll_res.sphereNodes().empty() && coll_res.capsuleNodes().empty();
}

void CollisionSurfaceGrid::build(const CollisionResource &coll_res, const mat44f &tm, const GeomNodeTree *geom_node_tree, vec3f from,
  vec3f dir, float len, float half_width, const CollisionNodeMask &collision_node_mask, const CollisionNodeMask *no_cull_node_mask,
  TraceCollisionResourceStats *out_stats)
{
  surfaces.clear();
  triangles.clear();
  // reserved before the first crossing: while they fit, crossings stays the last framemem block and grows in place
  surfaces.reserve(64);
  triangles.reserve(256);

  vec3f v1, v2;
  make_ray_side_axes(dir, v1, v2);
  origin = v_perm_xyzd(from, v_zero());
  axisDir = v_perm_xyzd(dir, v_zero());
  sideX = v_perm_xyzd(v1, v_zero());
  sideY = v_perm_xyzd(v2, v_zero());
  halfWidth = half_width;
  sampleStep = 2.f * half_width / float(GRID_SIZE - 1);
  axisLength = len;
  for (int block = 0; block < SAMPLE_BLOCK_COUNT; ++block)
    sampleX[block] =
      v_make_vec4f(sampleCoord(block * 4), sampleCoord(block * 4 + 1), sampleCoord(block * 4 + 2), sampleCoord(block * 4 + 3));

  mat44f originRelativeTm = tm;
  originRelativeTm.col3 = v_sub(tm.col3, origin);
  const CollisionResourceInstance &defaultPose = coll_res.getDefaultInstance();
  dag::Vector<Crossing, framemem_allocator> crossings;
  for (uint16_t nodeId : coll_res.meshNodes())
  {
    if (triangles.size() >= MAX_TRIANGLE_COUNT)
      break;
    const CollisionNode &node = coll_res.getAllNodes()[nodeId];
    if (!coll_res.nodeChunkPtr(node))
      continue;
    const CollisionResourceInstance::PoseMeta &poseMeta = defaultPose.getPoseMeta(nodeId);
    if (poseMeta.isDisabled() || !poseMeta.isTraceable() || !poseMeta.checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    if (!collision_node_mask.test(nodeId, true))
      continue;
    NodeTraits traits;
    traits.solid = poseMeta.checkBehaviorFlags(CollisionNode::SOLID);
    traits.keepBackFaces = traits.solid || (node.flags & CollisionNode::TRACE_TWO_SIDED) != 0 ||
                           (no_cull_node_mask && no_cull_node_mask->test(nodeId, true));

    alignas(EA_CACHE_LINE_SIZE) mat44f nodeTm;
    get_origin_relative_node_tm(coll_res, node, originRelativeTm, geom_node_tree, origin, nodeTm);
    if (rasterizeNode(coll_res, node, nodeTm, traits, crossings) && out_stats && nodeId < out_stats->size())
      (*out_stats)[nodeId]++;
  }
}

Point3 CollisionSurfaceGrid::getRayStart(const Point2 &offset) const
{
  Point3 start;
  v_stu_p3(&start.x, v_madd(v_madd(sideX, v_splats(offset.x), v_mul(sideY, v_splats(offset.y))), v_splats(halfWidth), origin));
  return start;
}

Point2 CollisionSurfaceGrid::sampleOffset(int sample_no)
{
  const float step = 2.f / float(GRID_SIZE - 1);
  return Point2(-1.f + float(sample_no % GRID_SIZE) * step, -1.f + float(sample_no / GRID_SIZE) * step);
}

CollisionSurfaceGrid::SamplePoint CollisionSurfaceGrid::makeSamplePoint(const Point2 &offset) const
{
  SamplePoint point;
  point.x = offset.x * halfWidth;
  point.y = offset.y * halfWidth;
  const float u = (point.x + halfWidth) / sampleStep;
  const float v = (point.y + halfWidth) / sampleStep;
  const int i0 = clamp((int)floorf(u), 0, GRID_SIZE - 2);
  const int j0 = clamp((int)floorf(v), 0, GRID_SIZE - 2);
  const float fu = clamp(u - float(i0), 0.f, 1.f);
  const float fv = clamp(v - float(j0), 0.f, 1.f);
  point.sampleNo[0] = j0 * GRID_SIZE + i0;
  point.sampleNo[1] = point.sampleNo[0] + 1;
  point.sampleNo[2] = point.sampleNo[0] + GRID_SIZE;
  point.sampleNo[3] = point.sampleNo[2] + 1;
  point.weight[0] = (1.f - fu) * (1.f - fv);
  point.weight[1] = fu * (1.f - fv);
  point.weight[2] = (1.f - fu) * fv;
  point.weight[3] = fu * fv;
  return point;
}

void CollisionSurfaceGrid::rasterizeTriangle(const NodeRaster &raster, vec3f p0, vec3f p1, vec3f p2, tri_ref_t tri_ref,
  dag::Vector<Crossing, framemem_allocator> &crossings)
{
  const vec3f a = v_mat44_mul_vec3p(raster.gridTm, p0);
  const vec3f b = v_mat44_mul_vec3p(raster.gridTm, p1);
  const vec3f c = v_mat44_mul_vec3p(raster.gridTm, p2);
  const vec3f lo = v_min(v_min(a, b), c);
  const vec3f hi = v_max(v_max(a, b), c);
  const vec3f limitLo = v_make_vec4f(-halfWidth, -halfWidth, 0.f, 0.f);
  const vec3f limitHi = v_make_vec4f(halfWidth, halfWidth, raster.length, 0.f);
  if (v_check_xyz_any_true(v_or(v_cmp_gt(lo, limitHi), v_cmp_lt(hi, limitLo))))
    return;

  alignas(16) float pa[4], pb[4], pc[4];
  v_st(pa, a);
  v_st(pb, b);
  v_st(pc, c);
  const float abx = pb[0] - pa[0], aby = pb[1] - pa[1];
  const float acx = pc[0] - pa[0], acy = pc[1] - pa[1];
  const float area = abx * acy - aby * acx;
  if (fabsf(area) <= 1e-6f * (abx * abx + aby * aby + acx * acx + acy * acy))
    return;
  const Facing facing = area * raster.detSign < 0.f ? Facing::FRONT : Facing::BACK;
  if (facing == Facing::BACK && !raster.keepBackFaces)
    return;

  const vec4f gridOffset = v_splats(halfWidth);
  const vec4f gridScale = v_splats(1.f / sampleStep);
  const vec4f tolerance = v_splats(1e-3f);
  const vec4i firstSample = v_maxi(v_cvt_ceili(v_sub(v_mul(v_add(lo, gridOffset), gridScale), tolerance)), v_zeroi());
  const vec4i lastSample = v_mini(v_cvt_floori(v_add(v_mul(v_add(hi, gridOffset), gridScale), tolerance)), v_splatsi(GRID_SIZE - 1));
  const int i0 = v_extract_xi(firstSample), j0 = v_extract_yi(firstSample);
  const int i1 = v_extract_xi(lastSample), j1 = v_extract_yi(lastSample);
  if (i0 > i1 || j0 > j1)
    return;
  const int firstBlock = i0 >> 2, lastBlock = i1 >> 2;
  unsigned blockLanes[SAMPLE_BLOCK_COUNT] = {};
  for (int block = firstBlock; block <= lastBlock; ++block)
    blockLanes[block] = sample_block_lanes(block, i0, i1);

  const float invArea = 1.f / area;
  const float abt = pb[2] - pa[2], act = pc[2] - pa[2];
  const vec4f kbx = v_splats(acy * invArea), kby = v_splats(-acx * invArea);
  const vec4f kcx = v_splats(-aby * invArea), kcy = v_splats(abx * invArea);
  const vec4f ta = v_splats(pa[2]), dtb = v_splats(abt), dtc = v_splats(act);
  const vec4f ax = v_splats(pa[0]);
  const vec4f minBarycentric = v_splats(-BARYCENTRIC_EPSILON);
  const vec4f maxT = v_splats(raster.length);

  const int triangleNo = (int)triangles.size();
  const uint32_t firstCrossing = crossings.size();
  uint32_t crossingCount = firstCrossing;
  crossings.resize_noinit(firstCrossing + SAMPLE_COUNT);
  for (int j = j0; j <= j1; ++j)
  {
    const vec4f dy = v_splats(sampleCoord(j) - pa[1]);
    const vec4f lbRow = v_mul(dy, kby);
    const vec4f lcRow = v_mul(dy, kcy);
    for (int block = firstBlock; block <= lastBlock; ++block)
    {
      const vec4f dx = v_sub(sampleX[block], ax);
      const vec4f lb = v_madd(dx, kbx, lbRow);
      const vec4f lc = v_madd(dx, kcx, lcRow);
      const vec4f la = v_sub(v_sub(V_C_ONE, lb), lc);
      const vec4f t = v_madd(lc, dtc, v_madd(lb, dtb, ta));
      const vec4f inside = v_and(v_and(v_cmp_ge(la, minBarycentric), v_cmp_ge(lb, minBarycentric)),
        v_and(v_cmp_ge(lc, minBarycentric), v_and(v_cmp_ge(t, v_zero()), v_cmp_ge(maxT, t))));
      const unsigned lanes = (unsigned)v_truemask(inside) & blockLanes[block];
      alignas(16) float tLanes[4];
      v_st(tLanes, t);
      const int blockSampleNo = j * GRID_SIZE + block * 4;
      for (int lane = 0; lane < 4; ++lane)
      {
        crossings[crossingCount] = {tLanes[lane], (uint16_t)triangleNo, (uint8_t)(blockSampleNo + lane), facing};
        crossingCount += (lanes >> lane) & 1u;
      }
    }
  }
  crossings.resize(crossingCount);
  if (crossingCount == firstCrossing)
    return;

  SurfaceTriangle &triangle = triangles.push_back();
  const vec3f n = v_cross3(v_sub(b, a), v_sub(c, a));
  const vec3f world = v_madd(sideX, v_splat_x(n), v_madd(sideY, v_splat_y(n), v_mul(axisDir, v_splat_z(n))));
  v_stu_p3(&triangle.normal.x, v_mul(v_norm3(world), v_splats(raster.detSign)));
  triangle.tSlopeX = (acy * abt - aby * act) * invArea;
  triangle.tSlopeY = (abx * act - acx * abt) * invArea;
  triangle.tAtAxis = pa[2] - triangle.tSlopeX * pa[0] - triangle.tSlopeY * pa[1];
  triangle.tMin = v_extract_z(lo);
  triangle.tMax = v_extract_z(hi);
  triangle.triRef = tri_ref;
}

// shortcut: a layer is the crossing rank in its sample, so an edge, a silhouette or a separate part of the node in front of another
// sheet of the same facing puts two sheets into one layer - group by connectivity then.
void CollisionSurfaceGrid::buildNodeSurfaces(dag::Vector<Crossing, framemem_allocator> &crossings)
{
  constexpr int BUCKET_COUNT = 2 * SAMPLE_COUNT;
  auto bucketOf = [](const Crossing &crossing) { return int(crossing.facing) * SAMPLE_COUNT + crossing.sampleNo; };
  uint32_t bucketEnd[BUCKET_COUNT] = {};
  for (const Crossing &crossing : crossings)
    bucketEnd[bucketOf(crossing)]++;
  for (uint32_t bucket = 0, begin = 0; bucket < BUCKET_COUNT; ++bucket)
  {
    const uint32_t count = bucketEnd[bucket];
    bucketEnd[bucket] = begin;
    begin += count;
  }
  const uint32_t crossingCount = crossings.size();
  crossings.resize_noinit(2 * crossingCount);
  Crossing *sorted = crossings.data() + crossingCount;
  for (uint32_t i = 0; i < crossingCount; ++i)
    sorted[bucketEnd[bucketOf(crossings[i])]++] = crossings[i];

  uint32_t begin = 0;
  for (int facing = 0; facing < 2; ++facing)
  {
    // A facing's layers are created in rank order, so its layer k is surface firstSurface + k.
    const uint32_t firstSurface = surfaces.size();
    for (int sampleNo = 0; sampleNo < SAMPLE_COUNT; ++sampleNo)
    {
      const uint32_t end = bucketEnd[facing * SAMPLE_COUNT + sampleNo];
      if (begin == end)
        continue;
      eastl::insertion_sort(sorted + begin, sorted + end, [](const Crossing &a, const Crossing &b) { return a.t < b.t; });

      uint32_t surfaceNo = firstSurface;
      float prevT = -VERY_BIG_NUMBER;
      for (uint32_t i = begin; i < end; ++i)
      {
        if (sorted[i].t - prevT <= DUPLICATE_CROSSING_DT)
          continue;
        prevT = sorted[i].t;
        if (surfaceNo == surfaces.size())
          surfaces.push_back();
        Surface &surface = surfaces[surfaceNo++];
        surface.coverage[sampleNo >> 5] |= 1u << (sampleNo & 31);
        surface.triangleNo[sampleNo] = sorted[i].triangleNo;
      }
      begin = end;
    }
  }
}

bool CollisionSurfaceGrid::rasterizeNode(const CollisionResource &coll_res, const CollisionNode &node,
  const mat44f &origin_relative_node_tm, const NodeTraits &traits, dag::Vector<Crossing, framemem_allocator> &crossings)
{
  const float scale = v_extract_x(v_mat44_spectral_norm43_bound_x(origin_relative_node_tm));
  const float det = v_extract_x(v_mat44_det43(origin_relative_node_tm));
  if (fabsf(det) <= 1e-6f * scale * scale * scale)
    return false;
  const float detSign = det < 0.f ? -1.f : 1.f;

  const bbox3f box = coll_res.getNodeGeometryBBox(node);
  const vec3f center = v_mat44_mul_vec3p(origin_relative_node_tm, v_bbox3_center(box));
  const float boundRadius = v_extract_x(v_length3_x(v_mul(v_sub(box.bmax, box.bmin), V_C_HALF))) * scale;
  const float axial = v_extract_x(v_dot3_x(center, axisDir));
  if (axial < -boundRadius || axial > axisLength + boundRadius)
    return false;
  if (v_extract_x(v_length3_sq_x(v_nmsub(axisDir, v_splats(axial), center))) > sqr(boundRadius + halfWidth * sqrtf(2.f)))
    return false;
  const float nodeLength = traits.solid ? max(axisLength, axial + boundRadius) : axisLength;

  mat44f nodeItm;
  v_mat44_inverse43(nodeItm, origin_relative_node_tm);
  const vec3f localFrom = v_mat44_mul_vec3p(nodeItm, v_zero());
  const vec3f localDir = v_mat44_mul_vec3v(nodeItm, axisDir);
  const float localDirLength = v_extract_x(v_length3_x(localDir));
  const float localLength = nodeLength * localDirLength;

  const vec3f localSideX = v_mat44_mul_vec3v(nodeItm, sideX);
  const vec3f localSideY = v_mat44_mul_vec3v(nodeItm, sideY);
  const vec3f localGridPad = v_mul(v_add(v_abs(localSideX), v_abs(localSideY)), v_splats(halfWidth));

  NodeRaster raster;
  mat44f basis = {sideX, sideY, axisDir, v_zero()};
  v_mat44_transpose(basis, basis);
  v_mat44_mul43(raster.gridTm, basis, origin_relative_node_tm);
  raster.detSign = detSign;
  raster.length = nodeLength;
  raster.keepBackFaces = traits.keepBackFaces;

  crossings.clear();
  coll_res.visitNodeTrianglesNearRay(node, localFrom, v_div(localDir, v_splats(localDirLength)), localLength, localGridPad,
    [this, &raster, &crossings](vec3f p0, vec3f p1, vec3f p2, tri_ref_t tri_ref) {
      rasterizeTriangle(raster, p0, p1, p2, tri_ref, crossings);
      return triangles.size() >= MAX_TRIANGLE_COUNT;
    });
  if (!crossings.empty())
    buildNodeSurfaces(crossings);
  return true;
}

bool CollisionSurfaceGrid::interpolateSurface(const Surface &surface, const SamplePoint &point, float &out_t, Point3 &out_normal,
  tri_ref_t &out_tri_ref) const
{
  float weightSum = 0.f, tSum = 0.f, heaviestWeight = 0.f;
  Point3 normalSum = Point3(0.f, 0.f, 0.f);
  for (int k = 0; k < 4; ++k)
  {
    const int sampleNo = point.sampleNo[k];
    if (!surface.covers(sampleNo))
      continue;
    const float weight = point.weight[k];
    const SurfaceTriangle &triangle = triangles[surface.triangleNo[sampleNo]];
    weightSum += weight;
    tSum += weight * clamp(triangle.tAtAxis + triangle.tSlopeX * point.x + triangle.tSlopeY * point.y, triangle.tMin, triangle.tMax);
    normalSum += triangle.normal * weight;
    if (weight > heaviestWeight)
    {
      heaviestWeight = weight;
      out_tri_ref = triangle.triRef;
    }
  }
  if (weightSum < MIN_COVERAGE_WEIGHT)
    return false;
  out_t = tSum / weightSum;
  out_normal = normalize(normalSum);
  return true;
}

bool CollisionSurfaceGrid::traceRay(const Point2 &offset, float in_t, CollResIntersectionsType &intersected_nodes_list,
  SurfaceNoList &out_surface_nos) const
{
  intersected_nodes_list.clear();
  out_surface_nos.clear();
  const SamplePoint point = makeSamplePoint(offset);
  const Point3 from = getRayStart(offset);
  Point3 dir;
  v_stu_p3(&dir.x, axisDir);
  for (int surfaceNo = 0; surfaceNo < surfaces.size(); ++surfaceNo)
  {
    float t;
    Point3 normal;
    tri_ref_t triRef;
    if (!interpolateSurface(surfaces[surfaceNo], point, t, normal, triRef) || t < 0.f || t >= in_t)
      continue;
    out_surface_nos.push_back(surfaceNo);
    IntersectedNode &hit = intersected_nodes_list.push_back();
    hit.normal = normal;
    hit.intersectionT = t;
    hit.intersectionPos = from + dir * t;
    hit.triRef = triRef;
  }
  return !intersected_nodes_list.empty();
}

bool CollisionSurfaceGrid::traceSample(int sample_no, float in_t, CollResIntersectionsType &intersected_nodes_list,
  SurfaceNoList &out_surface_nos) const
{
  intersected_nodes_list.clear();
  out_surface_nos.clear();
  const float x = sampleCoord(sample_no % GRID_SIZE);
  const float y = sampleCoord(sample_no / GRID_SIZE);
  const Point3 from = getRayStart(sampleOffset(sample_no));
  Point3 dir;
  v_stu_p3(&dir.x, axisDir);
  for (int surfaceNo = 0; surfaceNo < surfaces.size(); ++surfaceNo)
  {
    const Surface &surface = surfaces[surfaceNo];
    if (!surface.covers(sample_no))
      continue;
    const SurfaceTriangle &triangle = triangles[surface.triangleNo[sample_no]];
    const float t = clamp(triangle.tAtAxis + triangle.tSlopeX * x + triangle.tSlopeY * y, triangle.tMin, triangle.tMax);
    if (t < 0.f || t >= in_t)
      continue;
    out_surface_nos.push_back(surfaceNo);
    IntersectedNode &hit = intersected_nodes_list.push_back();
    hit.normal = triangle.normal;
    hit.intersectionT = t;
    hit.intersectionPos = from + dir * t;
    hit.triRef = triangle.triRef;
  }
  return !intersected_nodes_list.empty();
}

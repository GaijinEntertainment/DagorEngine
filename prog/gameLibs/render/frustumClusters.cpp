// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/frustumClusters.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_occlusionTest.h>

#define SHRINK_SPHERE     1
#define VALIDATE_CLUSTERS 0

static constexpr int get_target_mip(int srcW, int destW) { return srcW <= destW ? 0 : 1 + get_target_mip(srcW / 2, destW); }
bool get_max_occlusion_depth(vec4f *destDepth) // from occlusion
{
  if (OCCLUSION_W / OCCLUSION_H != CLUSTERS_W / CLUSTERS_H)
    return false;
  constexpr int occlusionMip = get_target_mip(OCCLUSION_W, CLUSTERS_W);
  G_STATIC_ASSERT((OCCLUSION_H >> occlusionMip) == CLUSTERS_H);
  vec4f *occlusionZ = reinterpret_cast<vec4f *>(OcclusionTest<OCCLUSION_W, OCCLUSION_H>::getZbuffer(occlusionMip));
  for (int i = 0; i < CLUSTERS_W * CLUSTERS_H / 4; ++i, destDepth++, occlusionZ++)
    *destDepth = occlusion_convert_from_internal_zbuffer(*occlusionZ);
  return true;
}

static __forceinline vec3f v_dist3_sq_x(vec3f a, vec3f b) { return v_length3_sq_x(v_sub(a, b)); }

void FrustumClusters::prepareFrustum(mat44f_cref view_, mat44f_cref proj_, float zn, float minDist, float maxDist,
  bool use_occlusion) // from occlusion
{
  depthSliceScale = CLUSTERS_D / log2f(maxDist / minDist);
  depthSliceBias = -log2f(minDist) * depthSliceScale;

  minSliceDist = minDist;
  maxSliceDist = maxDist;
  znear = zn;
  view = view_;
  proj = proj_;

  // constant for fixed min/max dists
  for (int z = 0; z <= CLUSTERS_D; ++z)
    sliceDists[z] = getDepthAtSlice(z, depthSliceScale, depthSliceBias);

  {
    vec4f destDepthW[(CLUSTERS_W * CLUSTERS_H + 3) / 4];
    if (use_occlusion && get_max_occlusion_depth(destDepthW))
    {
      vec4f *sliceDepth = destDepthW;
      uint32_t *slicesNo = (uint32_t *)maxSlicesNo.data();
      vec4f depthSliceScaleBias = v_make_vec4f(depthSliceScale, depthSliceBias, 255, 0);
      for (int y = 0; y < CLUSTERS_H; ++y)
      {
        vec4f maxRowSlice = v_splat_z(depthSliceScaleBias);
        for (int x = 0; x < CLUSTERS_W / 4; ++x, sliceDepth++, slicesNo++)
          *slicesNo = getVecMaxSliceAtDepth(*sliceDepth, depthSliceScaleBias, maxRowSlice);
        maxRowSlice = v_min(maxRowSlice, v_rot_2(maxRowSlice));
        sliceNoRowMax[y] = v_extract_xi(v_cvt_ceili(v_min(maxRowSlice, v_rot_1(maxRowSlice))));
      }
      //
    }
    else
    {
      mem_set_ff(maxSlicesNo);
      mem_set_ff(sliceNoRowMax);
    }
  }
  //-constant for fixed min/max dists

  // constant for fixed projection
  Point4 viewClip(((TMatrix4 &)proj)[0][0], -((TMatrix4 &)proj)[1][1], ((TMatrix4 &)proj)[2][0], ((TMatrix4 &)proj)[2][1]);
  for (int x = 0; x <= CLUSTERS_W; ++x)
  {
    static const float tileScaleX = 0.5f * CLUSTERS_W;
    float tileBiasX = tileScaleX - x;
    x_planes[x] = normalize(Point3(viewClip.x * tileScaleX, 0.0f, viewClip.z * tileScaleX + tileBiasX));
    x_planes[x].resv = 0;
    x_planes2[x] = Point2(x_planes[x].x, x_planes[x].z);
  }
  for (int y = 0; y <= CLUSTERS_H; ++y)
  {
    static const float tileScaleY = 0.5f * CLUSTERS_H;
    float tileBiasY = tileScaleY - y;
    y_planes[y] = normalize(Point3(0, viewClip.y * tileScaleY, viewClip.w * tileScaleY + tileBiasY));
    y_planes[y].resv = 0;
    y_planes2[y] = Point2(y_planes[y].y, y_planes[y].z);
  }
  {
    TIME_PROFILE(allPoints);
    vec4f *points = (vec4f *)&frustumPoints[0].x;
    for (int z = 0; z <= CLUSTERS_D; ++z)
    {
      vec4f z_plane = v_make_vec4f(0, 0, 1, z == 0 ? -zn : -sliceDists.data()[z]);
      for (int y = 0; y <= CLUSTERS_H; ++y)
      {
        vec3f point, dir;
        v_unsafe_two_plane_intersection(z_plane, v_ld(&y_planes.data()[y].x), point, dir);
        for (int x = 0; x <= CLUSTERS_W; ++x, points++)
          *points = v_unsafe_ray_intersect_plane(point, dir, v_ld(&x_planes.data()[x].x));
      }
    }
  }
#if HAS_FROXEL_SPHERES
  vec4f *spheres = (vec4f *)&frustumSpheres[0].x;
  for (int z = 0; z < CLUSTERS_D; ++z)
  {
    for (int y = 0; y < CLUSTERS_H; ++y)
    {
      vec4f *points = (vec4f *)&frustumPoints[z * (CLUSTERS_W + 1) * (CLUSTERS_H + 1) + y * (CLUSTERS_W + 1)].x;
      for (int x = 0; x < CLUSTERS_W; ++x, spheres++, points++)
      {
        bbox3f box;
        v_bbox3_init(box, points[0]);
        v_bbox3_add_pt(box, points[1]);
        v_bbox3_add_pt(box, points[(CLUSTERS_W + 1)]);
        v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) + 1]);
        v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + 0]);
        v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + 1]);
        v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + (CLUSTERS_W + 1)]);
        v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + (CLUSTERS_W + 1) + 1]);
        vec3f center = v_bbox3_center(box);
        vec3f dist2 = v_dist3_sq_x(points[0], center);
        dist2 = v_max(dist2, v_dist3_sq_x(points[1], center));
        dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1)], center));
        dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) + 1], center));
        dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + 0], center));
        dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + 1], center));
        dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + (CLUSTERS_W + 1)], center));
        dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + (CLUSTERS_W + 1) + 1], center));
        *spheres = v_perm_xyzd(center, v_splat_x(v_sqrt_x(dist2)));
      }
    }
  }
#endif
  //-constant for fixed projection
}

uint32_t FrustumClusters::getSpheresClipSpaceRects(const vec4f *pos_radius, int aligned_stride, int count,
  StaticTab<ItemRect3D, ClusterGridItemMasks::MAX_ITEM_COUNT> &rects3d,
  StaticTab<vec4f, ClusterGridItemMasks::MAX_ITEM_COUNT> &spheresViewSpace)
{
  float clusterW = CLUSTERS_W, clusterH = CLUSTERS_H;
  for (int i = 0; i < count; ++i, pos_radius += aligned_stride)
  {
    vec4f wpos = *pos_radius;
    vec4f vpos = v_mat44_mul_vec3p(view, wpos);
    DECL_ALIGN16(Point4, lightViewSpace);
    v_st(&lightViewSpace.x, v_perm_xyzd(vpos, wpos));

    FrustumScreenRect rect =
      findScreenSpaceBounds(((TMatrix4 &)proj)[0][0], ((TMatrix4 &)proj)[1][1], lightViewSpace, clusterW, clusterH, znear);
    if (rect.min_x > rect.max_x || rect.min_y > rect.max_y)
      continue;
    float zMinW = lightViewSpace.z - lightViewSpace.w, zMaxW = lightViewSpace.z + lightViewSpace.w;
    if (zMinW >= maxSliceDist) // todo: move to frustum culling
      continue;

    uint32_t zMin = getSliceAtDepth(max(zMinW, 1e-6f), depthSliceScale, depthSliceBias);
    uint32_t zMax = getSliceAtDepth(max(zMaxW, 1e-6f), depthSliceScale, depthSliceBias);
    rects3d.push_back(ItemRect3D(rect, min(zMin, uint32_t(CLUSTERS_D - 1)), min(zMax, uint32_t(CLUSTERS_D - 1)), i));
    spheresViewSpace.push_back(v_ld(&lightViewSpace.x));
  }
  return rects3d.size();
}

uint32_t FrustumClusters::fillItemsSpheresGrid(ClusterGridItemMasks &items,
  const StaticTab<vec4f, ClusterGridItemMasks::MAX_ITEM_COUNT> &lightsViewSpace, uint32_t *result_mask, uint32_t word_count)
{
  if (!items.rects3d.size())
    return 0;

  uint32_t currentMasksStart = 0, totalItemsCount = 0;
  const ItemRect3D *grid = items.rects3d.data();
  for (int i = 0; i < items.rects3d.size(); ++i, grid++)
  {
    items.sliceMasksStart[i] = currentMasksStart;

    const uint32_t itemId = items.rects3d[i].itemId;
    uint32_t *resultMasksUse = result_mask + (itemId >> 5);
    const uint32_t itemMask = 1 << (itemId & 31);

    // int x_center = (grid->rect.max_x+grid->rect.min_x)/2;
    int z0 = grid->zmin, z1 = grid->zmax;
    int y0 = grid->rect.min_y, y1 = grid->rect.max_y + 1;
    int x0 = grid->rect.min_x, x1 = grid->rect.max_x + 1;

#if !SHRINK_SPHERE
    const MaskType x_mask = ((MaskType(1) << MaskType(grid->rect.max_x)) | ((MaskType(1) << MaskType(grid->rect.max_x)) - 1)) &
                            (~((MaskType(1) << MaskType(x0)) - 1));
    totalItemsCount += (z1 - z0 + 1) * (y1 - y0) * (x1 - x0);
    (void)lightsViewSpace;
#else
    const Point4 &lightViewSpace = *reinterpret_cast<const Point4 *>(lightsViewSpace.data() + i);
    vec4f pt = v_mat44_mul_vec3p(proj, lightsViewSpace[i]);
    float radiusSq = lightViewSpace.w * lightViewSpace.w;
    int center_z = lightViewSpace.z <= znear ? -1 : getSliceAtDepth(lightViewSpace.z, depthSliceScale, depthSliceBias);
    int center_y = (v_extract_w(v_abs(pt)) > 0.001f)
                     ? floorf(CLUSTERS_H * (v_extract_x(v_div_x(v_splat_y(pt), v_splat_w(pt))) * -0.5f + 0.5))
                     : grid->rect.center_y; //(grid->rect.max_y+grid->rect.min_y)/2;
#endif
    for (int z = z0; z <= z1; z++)
    {
#if SHRINK_SPHERE
      DECL_ALIGN16(Point4, z_light_pos) = lightViewSpace;
      float z_lightRadiusSq = radiusSq;
      if (z != center_z)
      {
        float planeSign = z < center_z ? 1.0f : -1.0f;
        int zPlaneId = z < center_z ? z + 1 : z;
        const float sliceDist = center_z < 0 ? znear : sliceDists[zPlaneId];
        float zPlaneDist = planeSign * (lightViewSpace.z - sliceDist);
        z_light_pos.z = z_light_pos.z - zPlaneDist * planeSign;
        z_lightRadiusSq = max(0.f, radiusSq - zPlaneDist * zPlaneDist);
        z_light_pos.w = sqrtf(z_lightRadiusSq);
        // debug("%d: z = %d lightViewSpace = %@ z_light_pos= %@ zPlaneDist = %@ plane = %f dist %f",
        //   i, z, lightViewSpace, z_light_pos, zPlaneDist, planeSign, sliceDists[zPlaneId]);
      }
      Point2 z_light_pos2(z_light_pos.y, z_light_pos.z);
#endif
      uint8_t *sliceNoRow = maxSlicesNo.data() + y0 * CLUSTERS_W;
      for (int y = y0; y < y1; y++, currentMasksStart++, sliceNoRow += CLUSTERS_W)
      {
        if (sliceNoRowMax[y] < z)
          continue;
#if SHRINK_SPHERE
        // if (shrinkSphere.get())
        {
          DECL_ALIGN16(Point4, y_light_pos) = z_light_pos;
          if (y != center_y)
          { // Use original in the middle, shrunken sphere otherwise
            // project to plane
            // y_light = project_to_plane(y_light, plane);
            // const Point3 &plane = (y < center_y) ? y_planes.data()[y + 1] : -y_planes.data()[y];
            // float yPlaneT = dot(plane, *(Point3*)&y_light_pos);
            // y_light_pos += yPlaneT*plane.xyz;
            // y_light_pos.y -= yPlaneT*plane.y; y_light_pos.z -= yPlaneT*plane.z;

            const Point2 &plane2 = (y < center_y) ? y_planes2.data()[y + 1] : -y_planes2.data()[y];
            float yPlaneT = dot(plane2, z_light_pos2);
            y_light_pos.y -= yPlaneT * plane2.x;
            y_light_pos.z -= yPlaneT * plane2.y;
            float y_lightRadiusSq = z_lightRadiusSq - yPlaneT * yPlaneT;
            if (y_lightRadiusSq < 0)
            {
              // debug("z=%d y=%d: plane=%@, dist = %@ z_light = %@ skip",z,y, plane, yPlaneT,
              //   z_light_pos);
              items.sliceMasks.data()[currentMasksStart] = 0;
              continue;
            }
            y_light_pos.w = sqrtf(y_lightRadiusSq);
            // debug("z=%d y=%d: plane=%@, dist = %@ z_light = %@ y_light = %@",z,y, plane, yPlaneT,
            //   z_light_pos, y_light_pos);
          }
          Point2 y_light_pos2(y_light_pos.x, y_light_pos.z);
          int x = x0;
          do
          { // Scan from left until with hit the sphere
            ++x;
          } while (x < x1 && (sliceNoRow[x] < z || dot(x_planes2.data()[x], y_light_pos2) >= y_light_pos.w));
          // while (x < x1 && dot(x_planes.data()[x], *(Point3*)&y_light_pos) >= y_light_pos.w);

          int xs = x1;
          do
          { // Scan from right until with hit the sphere
            --xs;
          } while (xs >= x && (sliceNoRow[x] < z || -dot(x_planes2.data()[xs], y_light_pos2) >= y_light_pos.w));
          // while (xs >= x && -dot(x_planes.data()[xs], *(Point3*)&y_light_pos) >= y_light_pos.w);

          --x;
          uint32_t *resultMaskAt = resultMasksUse + (x + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H) * word_count;

#if NO_OCCLUSION
          items.sliceMasks.data()[currentMasksStart] =
            ((MaskType(1) << MaskType(xs)) | ((MaskType(1) << MaskType(xs)) - 1)) & (~((MaskType(1) << MaskType(x)) - 1));

          totalItemsCount += xs - x + 1;

          for (; x <= xs; x++, resultMaskAt += word_count)
            *resultMaskAt |= itemMask;
#else
          uint32_t mask = 0, bit = 1 << x;
          for (; x <= xs; x++, bit <<= 1, resultMaskAt += word_count)
          {
            if (z <= sliceNoRow[x])
            {
              totalItemsCount++;
              *resultMaskAt |= itemMask;
              mask |= bit;
            }
          }
          items.sliceMasks.data()[currentMasksStart] = mask;
#endif
          // if (lightsSliceMasks[currentMasksStart]!=x_mask)
          //   debug("save lights");
          // continue;
        }
#else
        items.sliceMasks[currentMasksStart] = x_mask;
        uint32_t *resultMaskAt = resultMasksUse + (x0 + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H) * word_count;
        for (int x = x0; x < x1; x++, resultMaskAt += word_count)
          *resultMaskAt |= itemMask;
#endif
      }
    }
    // currentMasksStart += (grid->zmax-grid->zmin+1) * (grid->rect.max_y-grid->rect.min_y+1);
  }
  G_ASSERT(currentMasksStart <= items.sliceMasks.size());
  items.itemsListCount = totalItemsCount;
  return totalItemsCount;
}

uint32_t FrustumClusters::fillItemsSpheres(const vec4f *pos_radius, int aligned_stride, int count, ClusterGridItemMasks &items,
  uint32_t *result_mask, uint32_t word_count)
{
  TIME_PROFILE(fillItemsSphere);
  G_STATIC_ASSERT(sizeof(items.sliceMasks[0]) * 8 >= CLUSTERS_W);
  items.reset();
  if (!count)
    return 0;
  TIME_PROFILE(fillRects)
  StaticTab<vec4f, ClusterGridItemMasks::MAX_ITEM_COUNT> lightsViewSpace;
  items.itemsListCount = 0;
  if (!getSpheresClipSpaceRects(pos_radius, aligned_stride, count, items.rects3d, lightsViewSpace))
    return 0;

  return fillItemsSpheresGrid(items, lightsViewSpace, result_mask, word_count);
}

#define V4D(a) v_extract_x(a), v_extract_y(a), v_extract_z(a), v_extract_w(a)
static inline int is_point_out(vec3f pt, mat44f_cref plane03_XYZW)
{
  vec4f res03;
  res03 = v_madd(v_splat_x(pt), plane03_XYZW.col0, plane03_XYZW.col3);
  res03 = v_madd(v_splat_y(pt), plane03_XYZW.col1, res03);
  res03 = v_madd(v_splat_z(pt), plane03_XYZW.col2, res03);
  int result = v_signmask(res03);
  return result;
}

static inline int are_points_out(vec3f *pt, mat44f_cref plane03_XYZW)
{
  vec4f res03, ret03;
  res03 = v_madd(v_splat_x(pt[0]), plane03_XYZW.col0, plane03_XYZW.col3);
  res03 = v_madd(v_splat_y(pt[0]), plane03_XYZW.col1, res03);
  res03 = v_madd(v_splat_z(pt[0]), plane03_XYZW.col2, res03);
  // debug("0=%f %f %f %f",V4D(res03));
  ret03 = res03;
  res03 = v_madd(v_splat_x(pt[1]), plane03_XYZW.col0, plane03_XYZW.col3);
  res03 = v_madd(v_splat_y(pt[1]), plane03_XYZW.col1, res03);
  res03 = v_madd(v_splat_z(pt[1]), plane03_XYZW.col2, res03);
  ret03 = v_and(res03, ret03);
  // debug("1=%f %f %f %f",V4D(res03));

  pt += CLUSTERS_W + 1;

  res03 = v_madd(v_splat_x(pt[0]), plane03_XYZW.col0, plane03_XYZW.col3);
  res03 = v_madd(v_splat_y(pt[0]), plane03_XYZW.col1, res03);
  res03 = v_madd(v_splat_z(pt[0]), plane03_XYZW.col2, res03);
  ret03 = v_and(res03, ret03);
  // debug("2=%f %f %f %f",V4D(res03));
  res03 = v_madd(v_splat_x(pt[1]), plane03_XYZW.col0, plane03_XYZW.col3);
  res03 = v_madd(v_splat_y(pt[1]), plane03_XYZW.col1, res03);
  res03 = v_madd(v_splat_z(pt[1]), plane03_XYZW.col2, res03);
  ret03 = v_and(res03, ret03);
  // debug("3=%f %f %f %f",V4D(res03));

  pt += (CLUSTERS_W + 1) * (CLUSTERS_H + 1) - (CLUSTERS_W + 1);

  res03 = v_madd(v_splat_x(pt[0]), plane03_XYZW.col0, plane03_XYZW.col3);
  res03 = v_madd(v_splat_y(pt[0]), plane03_XYZW.col1, res03);
  res03 = v_madd(v_splat_z(pt[0]), plane03_XYZW.col2, res03);
  ret03 = v_and(res03, ret03);
  // debug("4=%f %f %f %f",V4D(res03));
  res03 = v_madd(v_splat_x(pt[1]), plane03_XYZW.col0, plane03_XYZW.col3);
  res03 = v_madd(v_splat_y(pt[1]), plane03_XYZW.col1, res03);
  res03 = v_madd(v_splat_z(pt[1]), plane03_XYZW.col2, res03);
  ret03 = v_and(res03, ret03);
  // debug("5=%f %f %f %f",V4D(res03));

  pt += CLUSTERS_W + 1;

  res03 = v_madd(v_splat_x(pt[0]), plane03_XYZW.col0, plane03_XYZW.col3);
  res03 = v_madd(v_splat_y(pt[0]), plane03_XYZW.col1, res03);
  res03 = v_madd(v_splat_z(pt[0]), plane03_XYZW.col2, res03);
  ret03 = v_and(res03, ret03);
  // debug("6=%f %f %f %f",V4D(res03));
  res03 = v_madd(v_splat_x(pt[1]), plane03_XYZW.col0, plane03_XYZW.col3);
  res03 = v_madd(v_splat_y(pt[1]), plane03_XYZW.col1, res03);
  res03 = v_madd(v_splat_z(pt[1]), plane03_XYZW.col2, res03);
  ret03 = v_and(res03, ret03);
  // debug("7=%f %f %f %f",V4D(res03));

  int result = v_signmask(ret03);
  return result;
}


inline bool test_cone_vs_sphere(vec4f pos_radius, vec4f dir_cos_angle, vec4f testSphere, const vec3f &sinAngle)
{
  vec3f V = v_sub(testSphere, pos_radius);
  vec3f VlenSq = v_dot3_x(V, V);            //_x
  vec3f V1len = v_dot3_x(V, dir_cos_angle); //_x
  /*float tanHalf = v_extract_w(dir_angle);
  float cosHalfAngle = 1./sqrtf(1 + tanHalf*tanHalf);
  vec3f  distanceClosestPoint = v_sub_x(v_mul_x(v_splats(cosHalfAngle), v_sqrt_x(v_sub_x(VlenSq, v_mul_x(V1len, V1len)))),
                                        v_mul_x(V1len, v_splats(sqrtf(1-cosHalfAngle*cosHalfAngle))));//_x*/

  vec3f distanceClosestPoint =
    v_sub_x(v_mul_x(v_splat_w(dir_cos_angle), v_sqrt_x(v_sub_x(VlenSq, v_mul_x(V1len, V1len)))), v_mul_x(V1len, sinAngle)); //_x

  vec4f sphereRad = v_splat_w(testSphere);

  vec3f nangleCull = v_cmp_ge(sphereRad, distanceClosestPoint);
  vec3f result = nangleCull;
  vec3f nfrontCull = v_cmp_ge(v_add_x(sphereRad, v_splat_w(pos_radius)), V1len);
  result = v_and(result, nfrontCull);
  vec3f nbackCull = v_cmp_ge(V1len, v_neg(sphereRad));
  result = v_and(result, nbackCull);
  return v_signmask(result) & 1;
}

uint32_t FrustumClusters::cullSpot(ClusterGridItemMasks &items, int i, vec4f pos_radius, vec4f dir_angle, uint32_t *result_mask,
  uint32_t word_count)
{
  G_ASSERT(HAS_FROXEL_SPHERES);
  G_UNUSED(pos_radius);
  G_UNUSED(dir_angle);
  G_UNUSED(i);
  G_UNUSED(result_mask);
  G_UNUSED(word_count);
#if HAS_FROXEL_SPHERES
  const ItemRect3D *grid = &items.rects3d[i];

  const uint32_t itemId = items.rects3d[i].itemId;
  uint32_t *resultMasksUse = result_mask + (itemId >> 5);
  const uint32_t itemMask = 1 << (itemId & 31);

  int z0 = grid->zmin, z1 = grid->zmax;
  int y0 = grid->rect.min_y, y1 = grid->rect.max_y + 1;
  int x0 = grid->rect.min_x, x1 = grid->rect.max_x + 1;

  if (z0 == z1 && y1 - y0 == 1 && x1 - x0 == 1 && x0 != 0 && y0 != 0 && x1 != CLUSTERS_W && y1 != CLUSTERS_H && z0 != 0 &&
      z1 != CLUSTERS_D - 1) // it is very small, inside one cluster and so can't culled
    return items.itemsListCount;

  float tanHalf = v_extract_w(dir_angle);
  float cosHalfAngle = 1.f / sqrtf(1.f + tanHalf * tanHalf);
  dir_angle = v_perm_xyzd(dir_angle, v_splats(cosHalfAngle));
  vec3f sinAngle = v_splats(sqrtf(1 - cosHalfAngle * cosHalfAngle));

  uint32_t currentMasksStart = items.sliceMasksStart[i];
  // uint8_t *cPlanes = planeBits.data();
  for (int z = z0; z <= z1; z++) //, cPlanes+=planeYStride)
  {
    for (int y = y0; y < y1; y++, currentMasksStart++) //, cPlanes++)
    {
      const MaskType mask = items.sliceMasks[currentMasksStart];
      MaskType x_mask = (MaskType(1) << MaskType(x0));
      const vec4f *spheres = (const vec4f *)&frustumSpheres[x0 + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H].x;
      uint32_t *resultMaskAt = resultMasksUse + (x0 + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H) * word_count;
      for (int x = x0; x < x1; x++, x_mask <<= MaskType(1), spheres++, resultMaskAt += word_count)
      {
        if (!(mask & x_mask))
          continue;
        if (!test_cone_vs_sphere(pos_radius, dir_angle, *spheres, sinAngle))
        {
          *resultMaskAt &= ~itemMask;
          items.sliceMasks[currentMasksStart] &= ~x_mask;
          items.itemsListCount--;
        }
      }
    }
  }
#endif
  return items.itemsListCount;
}

uint32_t FrustumClusters::cullFrustum(ClusterGridItemMasks &items, int i, mat44f_cref plane03_XYZW, mat44f_cref plane47_XYZW,
  uint32_t *result_mask, uint32_t word_count)
{
  const ItemRect3D *grid = &items.rects3d[i];

  const uint32_t itemId = items.rects3d[i].itemId;
  uint32_t *resultMasksUse = result_mask + (itemId >> 5);
  const uint32_t itemMask = 1 << (itemId & 31);

  int z0 = grid->zmin, z1 = grid->zmax;
  int y0 = grid->rect.min_y, y1 = grid->rect.max_y + 1;
  int x0 = grid->rect.min_x, x1 = grid->rect.max_x + 1;

  if (z0 == z1 && y1 - y0 == 1 && x1 - x0 == 1 && x0 != 0 && y0 != 0 && x1 != CLUSTERS_W && y1 != CLUSTERS_H && z0 != 0 &&
      z1 != CLUSTERS_D - 1) // it is very small, inside one cluster and so can't culled
    return items.itemsListCount;

  carray<uint8_t, (CLUSTERS_W + 1) * (CLUSTERS_H + 1) * (CLUSTERS_D + 1)> planeBits;
  const int ez1 = min(z1 + 1, CLUSTERS_D);
  const int ey1 = min(y1 + 1, CLUSTERS_H + 1);
  const int ex1 = min(x1 + 1, CLUSTERS_W + 1);
  const int planeZStride = (CLUSTERS_W + 1) * (CLUSTERS_H + 1); // todo: replace with tight
  const int planeYStride = (CLUSTERS_W + 1);                    // todo: replace with tight
  // uint8_t *planes = planeBits.data();

  // todo: we should optimize it. we check too much of points, which are definetly visible, and a lot of points which won't be tested
  // at all
  //  each plane can directly solve planes equasion, so we can know valid line for each z,y without checking each point. it is always
  //  00011111000 case, we only need boundaries
  for (int z = z0; z <= ez1; z++)
  {
    uint8_t *planes = planeBits.data() + z * planeZStride + y0 * planeYStride + x0;
    vec4f *point =
      reinterpret_cast<vec4f *>(frustumPoints.data()) + z * (CLUSTERS_W + 1) * (CLUSTERS_H + 1) + y0 * (CLUSTERS_W + 1) + x0;
    for (int y = y0; y < ey1; y++, planes += planeYStride - (ex1 - x0), point += (CLUSTERS_W + 1) - (ex1 - x0)) //
      for (int x = x0; x < ex1; x++, point++, planes++)
      {
        // debug_it = ((x == 1 || x==2) && (y==1 || y==2) && (z==0||z==1) && i==0);
        *planes = is_point_out(*point, plane03_XYZW) | (is_point_out(*point, plane47_XYZW) << 4);
        // if (debug_it)
        //   debug("*planes = 0x%X %dx%dx%d: index= %d",*planes,x,y,z, planes-planeBits.data());
      }
  }
  // const int planeYStride = (ex1-x0);//todo: replace with tight
  // const int planeZStride = planeYStride*(ey1-y0);

  uint32_t currentMasksStart = items.sliceMasksStart[i];
  // uint8_t *cPlanes = planeBits.data();
  for (int z = z0; z <= z1; z++) //, cPlanes+=planeYStride)
  {
    for (int y = y0; y < y1; y++, currentMasksStart++) //, cPlanes++)
    {
      const MaskType mask = items.sliceMasks[currentMasksStart];
      MaskType x_mask = (MaskType(1) << MaskType(x0));
      uint8_t *cPlanes = planeBits.data() + z * planeZStride + y * planeYStride + x0;
      uint32_t *resultMaskAt = resultMasksUse + (x0 + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H) * word_count;
      for (int x = x0; x < x1; x++, x_mask <<= MaskType(1), cPlanes++, resultMaskAt += word_count)
      {
        if (!(mask & x_mask))
          continue;
        uint8_t planeOut = cPlanes[0] & cPlanes[1] & cPlanes[planeYStride] & cPlanes[planeYStride + 1];

        planeOut &= cPlanes[planeZStride + 0] & cPlanes[planeZStride + 1] & cPlanes[planeZStride + planeYStride] &
                    cPlanes[planeZStride + planeYStride + 1];
// #define CHECK_OPTIMIZATION 1
#if CHECK_OPTIMIZATION
        uint32_t index = z * (CLUSTERS_W + 1) * (CLUSTERS_H + 1) + y * (CLUSTERS_W + 1) + x;
        vec4f *point = (vec4f *)frustumPoints.data() + index;
#endif
        if (planeOut)
        {
#if CHECK_OPTIMIZATION
          if (!are_points_out(point, plane03_XYZW) && !are_points_out(point, plane47_XYZW))
          {
            debug("planeOut = %d=%d&%d&%d&%d & %d&%d&%d&%d index = %d + %d+%d", planeOut, cPlanes[0], cPlanes[1],
              cPlanes[planeYStride], cPlanes[planeYStride + 1], cPlanes[planeZStride], cPlanes[planeZStride + 1],
              cPlanes[planeZStride + planeYStride], cPlanes[planeZStride + planeYStride + 1], cPlanes - planeBits.data(), planeZStride,
              planeYStride);
            // debug("i=%d pt =%dx%dx%d", i, x,y,z);
            // if (z == z0)
            //   debug("%dx%d pt = %d %f %f %f", x, y, pt, v_extract_x(*point), v_extract_y(*point), v_extract_z(*point));
            // items.gridCount.data()[x+y*CLUSTERS_W + z*CLUSTERS_W*CLUSTERS_H]--;
            // items.sliceMasks[currentMasksStart] &= ~x_mask;
            // items.itemsListCount --;
          }
#endif
          *resultMaskAt &= ~itemMask;
          items.sliceMasks[currentMasksStart] &= ~x_mask;
          items.itemsListCount--;
        }
        else
        {
#if CHECK_OPTIMIZATION
          if (are_points_out(point, plane03_XYZW) || are_points_out(point, plane47_XYZW))
            debug("false");
#endif
        }
      }
    }
  }

  return items.itemsListCount;
}

uint32_t FrustumClusters::cullSpots(const vec4f *pos_radius, int pos_aligned_stride, const vec4f *dir_angle, int dir_aligned_stride,
  ClusterGridItemMasks &items, uint32_t *result_mask, uint32_t word_count)
{
  if (!items.itemsListCount)
    return 0;
  const ItemRect3D *grid = items.rects3d.data();
  vec4f v_c099 = v_splats(0.999f);
  for (int i = 0; i < items.rects3d.size(); ++i, grid++)
  {
    if (grid->zmin == grid->zmax && grid->rect.min_y == grid->rect.max_y && grid->rect.min_x == grid->rect.max_x && grid->zmin != 0 &&
        grid->rect.min_x != 0 && grid->rect.min_y != 0 && grid->rect.min_x != CLUSTERS_W - 1 &&
        grid->rect.min_y != CLUSTERS_H - 1) // it is very small, inside one cluster and so can't culled
      return items.itemsListCount;

    uint32_t id = grid->itemId;
    vec4f wpos = pos_radius[id * pos_aligned_stride], wdir = dir_angle[id * dir_aligned_stride];
    // float cosHalfAngle = 1./sqrtf(1 + dir_angle[id].w*dir_angle[id].w);
    vec4f tanHalf = v_splat_w(wdir);
    vec4f vpos = v_mat44_mul_vec3p(view, wpos);
    vec4f vdir = v_mat44_mul_vec3v(view, wdir);

// cullSpot(items, i, v_perm_xyzd(vpos, wpos), v_perm_xyzd(vdir, tanHalf));// a bit faster
// continue;

// vec3f vposDist = v_mul(vdir, v_splat_w(pos_radius));
// Point3 up0 = fabsf(tangentZ.z) < 0.999 ? Point3(0,0,1) : Point3(1,0,0);
#define TWO_MORE_PLANES 1
    vec4f up0 = v_sel(V_C_UNIT_0010, V_C_UNIT_1000, v_cmp_gt(v_abs(v_splat_z(vdir)), v_c099));
    vec3f left = v_norm3(v_cross3(up0, vdir)), up = v_cross3(vdir, left);
    vec3f vFar2 = v_mul(tanHalf, v_add(left, up)), vFar1 = v_mul(tanHalf, v_sub(left, up)), vFar3 = v_neg(vFar1), vFar0 = v_neg(vFar2);
#if TWO_MORE_PLANES
    vec3f leftrot = v_mul(vFar2, v_splats(0.7071067811865476f)), uprot = v_mul(vFar1, v_splats(0.7071067811865476f));
#endif
    vFar0 = v_add(vFar0, vdir);
    vFar1 = v_add(vFar1, vdir);
    vFar2 = v_add(vFar2, vdir);
    vFar3 = v_add(vFar3, vdir);

    mat44f plane03;
    plane03.col0 = v_make_plane_dir(vpos, vFar0, vFar1);
    plane03.col1 = v_make_plane_dir(vpos, vFar1, vFar2);
    plane03.col2 = v_make_plane_dir(vpos, vFar2, vFar3);
    plane03.col3 = v_make_plane_dir(vpos, vFar3, vFar0);
    v_mat44_transpose(plane03, plane03);


    plane3f planeNear = v_perm_xyzd(vdir, v_neg(v_dot3(vdir, vpos))),
            planeFar = v_perm_xyzd(v_neg(vdir), v_splat_w(v_sub(wpos, planeNear)));
    mat44f plane47;
    plane47.col0 = planeNear;
    plane47.col1 = planeFar;

#if TWO_MORE_PLANES
    vFar2 = v_add(leftrot, uprot);
    vFar1 = v_sub(leftrot, uprot);
    vFar0 = v_neg(vFar2); // vFar3 = v_neg(vFar1);
    vFar0 = v_add(vFar0, vdir);
    vFar1 = v_add(vFar1, vdir);
    vFar2 = v_add(vFar2, vdir); // vFar3 = v_add(vFar3, vdir);
    plane47.col2 = v_make_plane_dir(vpos, vFar1, vFar0);
    plane47.col3 = v_make_plane_dir(vpos, vFar2, vFar1);
    // we can add even more planes, but it will be at a cost of additional 4 planes..
#else
    plane47.col2 = planeNear;
    plane47.col3 = planeFar;
#endif
    v_mat44_transpose(plane47, plane47);

    cullFrustum(items, i, plane03, plane47, result_mask, word_count);
    // v_mat44_transpose(plane03, plane03);
    // vec3f tp = v_make_vec4f(-3.7626, 1.98745, 4.89138, 0);//v_add(vpos, v_mul(vdir, v_mul(V_C_HALF, v_splat_w(wpos))));
    // debug("dist %g %g %g %g | %g %g ", v_extract_x(v_plane_dist_x(plane03.col0, tp)),
    //       v_extract_x(v_plane_dist_x(plane03.col1, tp)), v_extract_x(v_plane_dist_x(plane03.col2, tp)),
    //       v_extract_x(v_plane_dist_x(plane03.col3, tp)),
    //       v_extract_x(v_plane_dist_x(planeNear, tp)), v_extract_x(v_plane_dist_x(planeFar, tp)));

    // static inline uint32_t
    // int x_center = (grid->rect.max_x+grid->rect.min_x)/2;
    // currentMasksStart += (grid->zmax-grid->zmin+1) * (grid->rect.max_y-grid->rect.min_y+1);
  }
  return items.itemsListCount;
}

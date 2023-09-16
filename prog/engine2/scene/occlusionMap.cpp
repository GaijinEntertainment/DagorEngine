#include <vecmath/dag_vecMath.h>
#include <math/integer/dag_IPoint2.h>
#include <scene/dag_occlusionMap.h>
#include <scene/dag_occlusion.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_tab.h>
#include <math/dag_frustum.h>
#include <util/dag_stlqsort.h>
#include <memory/dag_framemem.h>


void OcclusionMap::load(IGenLoad &cb)
{
  int occludersCount = cb.readInt();
  G_ASSERT(occludersCount < 65536);
  SmallTab<OccluderInfo, TmpmemAlloc> occludersInfo;
  SmallTab<Occluder, TmpmemAlloc> occluders;
  clear_and_resize(occluders, occludersCount);
  clear_and_resize(occludersInfo, occludersCount);
  cb.read(occluders.data(), data_size(occluders));
  cb.read(occludersInfo.data(), data_size(occludersInfo));
  int boxCnt = 0, quadCnt = 0;

  for (int i = 0; i < occludersInfo.size(); ++i)
  {
    if (occludersInfo[i].occType == occludersInfo[i].BOX)
      boxCnt++;
    else if (occludersInfo[i].occType == occludersInfo[i].QUAD)
      quadCnt++;
  }
  clear_and_resize(boxOccludersBoxes, boxCnt);
  clear_and_resize(boxOccluders, boxCnt);
  clear_and_resize(boxRadiusSq, boxCnt);

  clear_and_resize(quadOccluders, quadCnt);
  clear_and_resize(quadOccludersBoxes, quadCnt);
  clear_and_resize(quadNormAndArea, quadCnt);
  quadCnt = boxCnt = 0;
  bbox3f unitbox;
  unitbox.bmax = V_C_HALF;
  unitbox.bmin = v_neg(unitbox.bmax);
  for (int i = 0; i < occludersInfo.size(); ++i)
  {
    bbox3f box;
    if (occludersInfo[i].occType == occludersInfo[i].BOX)
    {
      mat44f mat44;
      v_mat44_make_from_43cu(mat44, occluders[i].box.boxSpace()[0]);
      v_mat44_inverse43(mat44, mat44);
      boxOccluders[boxCnt] = mat44;
      v_bbox3_init(box, mat44, unitbox);
      vec3f center = v_mul(v_add(box.bmax, box.bmin), V_C_HALF);
      boxOccludersBoxes[boxCnt].bmin = center;
      boxOccludersBoxes[boxCnt].bmax = v_sub(box.bmax, box.bmin); // extent2
      boxRadiusSq[boxCnt] = v_extract_x(v_length3_sq_x(v_sub(box.bmax, center)));
      boxCnt++;
    }
    else if (occludersInfo[i].occType == occludersInfo[i].QUAD)
    {
      quadOccluders[quadCnt].col0 = v_ldu(&occluders[i].quad.corners()[0].x);
      quadOccluders[quadCnt].col1 = v_ldu(&occluders[i].quad.corners()[1].x);
      quadOccluders[quadCnt].col2 = v_ldu(&occluders[i].quad.corners()[2].x);
      quadOccluders[quadCnt].col3 = v_ldu(&occluders[i].quad.corners()[3].x);
      v_bbox3_init(box, quadOccluders[quadCnt].col0);
      v_bbox3_add_pt(box, quadOccluders[quadCnt].col1);
      v_bbox3_add_pt(box, quadOccluders[quadCnt].col2);
      v_bbox3_add_pt(box, quadOccluders[quadCnt].col3);
      quadOccludersBoxes[quadCnt].bmin = v_mul(v_add(box.bmax, box.bmin), V_C_HALF);
      quadOccludersBoxes[quadCnt].bmax = v_sub(box.bmax, box.bmin);
      vec3f tri1 = v_cross3(v_sub(quadOccluders[quadCnt].col1, quadOccluders[quadCnt].col0),
        v_sub(quadOccluders[quadCnt].col2, quadOccluders[quadCnt].col0));
      vec3f tri2 = v_cross3(v_sub(quadOccluders[quadCnt].col2, quadOccluders[quadCnt].col3),
        v_sub(quadOccluders[quadCnt].col1, quadOccluders[quadCnt].col3));
      G_ASSERT(v_test_vec_x_gt_0(v_dot3_x(tri1, tri2)));
      vec3f area1 = v_length3(tri1);
      vec3f area2 = v_length3(tri2);
      vec3f area = v_add(area1, area2);
      quadNormAndArea[quadCnt] = v_mul(v_div(tri1, area1), area);
      quadCnt++;
    }
  }
}

struct SortByYunsigned
{
  bool operator()(const IPoint2 &a, const IPoint2 &b) const { return unsigned(a.y) < unsigned(b.y); }
};

static void change_occluders_metric(int occluders_count, int max_count, float &cmetric, const float minimal_metric)
{
  if (occluders_count > max_count)
    cmetric = min(1.0f, cmetric * 1.03f);
  else if (occluders_count < max_count / 4)
    cmetric = max(minimal_metric, cmetric * 0.97f);
}

static void select_best_occluders(Tab<IPoint2> &occluders, const int max_count)
{
  if (occluders.size() > max_count)
  {
    stlsort::nth_element(occluders.data(), occluders.data() + max_count, occluders.data() + occluders.size(), SortByYunsigned());
    occluders.resize(max_count);
  }
  stlsort::sort(occluders.data(), occluders.data() + occluders.size(), SortByYunsigned());
}

bool OcclusionMap::prepare(Occlusion &occlusion)
{
  vec3f viewPos = occlusion.getCurViewPos();
  mat44f globtm = occlusion.getCurViewProj();
  Frustum viewFrustum(globtm);
  // adding new occluders

  vec4f minAproxSize = v_splats(minQuadMetric * minQuadMetric); // todo: use current FOV!
  Tab<IPoint2> closestBoxOccluders(framemem_ptr()), closestQuadOccluders(framemem_ptr());

  for (int i = 0; i < quadOccluders.size(); ++i)
  {
    vec3f center = quadOccludersBoxes[i].bmin;
    vec3f centerVec = v_sub(center, viewPos);
    vec3f dist2 = v_length3_sq(centerVec);

    // vec3f invDist2 = v_rcp(dist2);
    // vec4f approxSolidAngle = v_mul(v_abs(v_dot3(centerVec, quadNormAndArea[i])), invDist2);
    // if (v_test_vec_x_lt(approxSolidAngle, minAproxSize))
    if (v_test_vec_x_lt(v_abs(v_dot3_x(centerVec, quadNormAndArea[i])), v_mul_x(minAproxSize, dist2)))
    {
      bbox3f box; // twice bigger box
      box.bmin = v_sub(center, quadOccludersBoxes[i].bmax);
      box.bmax = v_add(center, quadOccludersBoxes[i].bmax);
      if (!v_bbox3_test_pt_inside(box, viewPos))
        continue;
    }

    if (!viewFrustum.testBoxExtentB(v_add(center, center), quadOccludersBoxes[i].bmax)) // todo: check if needed at all. We actually
                                                                                        // perform visibility test within addOccluder
      continue;
    closestQuadOccluders.push_back(IPoint2(i, v_extract_xi(v_cast_vec4i(dist2))));
  }
  for (int i = 0; i < dynamicQuadOccluders.size(); ++i)
  {
    bbox3f box;
    v_bbox3_init(box, dynamicQuadOccluders[i].p0);
    v_bbox3_add_pt(box, dynamicQuadOccluders[i].p1);
    v_bbox3_add_pt(box, dynamicQuadOccluders[i].p2);
    v_bbox3_add_pt(box, dynamicQuadOccluders[i].p3);
    if (!viewFrustum.testBoxB(box.bmin, box.bmax))
      continue;

    vec3f center = v_bbox3_center(box);
    vec3f tri1 = v_cross3(v_sub(dynamicQuadOccluders[i].p1, dynamicQuadOccluders[i].p0),
      v_sub(dynamicQuadOccluders[i].p2, dynamicQuadOccluders[i].p0));
    // vec3f tri2 = v_cross3(v_sub(dynamicQuadOccluders[i].p2, dynamicQuadOccluders[i].p3),
    //                       v_sub(dynamicQuadOccluders[i].p1, dynamicQuadOccluders[i].p3));
    // G_ASSERT(v_test_vec_x_gt_0(v_dot3_x(tri1, tri2)));
    vec3f area1 = v_length3_est_x(tri1);
    // vec3f area2 = v_length3_est_x(tri2);
    vec3f area = v_splat_x(v_add_x(area1, area1)); // multiply area of first tri by 2
    vec4f quadNormAndArea1 = v_mul(v_div(tri1, area1), area);

    vec3f centerVec = v_sub(center, viewPos);
    vec3f dist2 = v_length3_sq(centerVec);

    // vec3f invDist2 = v_rcp(dist2);
    // vec4f approxSolidAngle = v_mul(v_abs(v_dot3(centerVec, quadNormAndArea[i])), invDist2);
    // if (v_test_vec_x_lt(approxSolidAngle, minAproxSize))
    if (v_test_vec_x_lt(v_abs(v_dot3_x(centerVec, quadNormAndArea1)), v_mul_x(minAproxSize, dist2)))
      continue;
    closestQuadOccluders.push_back(IPoint2(i | (1 << 31), v_extract_xi(v_cast_vec4i(dist2))));
  }

  vec4f minBoxApprox2 = v_splats(minBoxMetric * minBoxMetric); // todo: use FOV!
  for (int i = 0; i < boxOccluders.size(); ++i)
  {
    vec3f center = boxOccludersBoxes[i].bmin;
    vec3f centerVec = v_sub(center, viewPos);
    vec3f dist2 = v_length3_sq_x(centerVec);
    // vec3f invDist2 = v_rcp(dist2);
    // vec4f approxSolidAngle = v_mul(v_splat4(&boxRadiusSq[i]), invDist2);
    // if (v_test_vec_x_lt(approxSolidAngle, minBoxApprox2))
    if (v_test_vec_x_lt(v_splats(boxRadiusSq[i]), v_mul_x(dist2, minBoxApprox2)))
      continue;

    if (!viewFrustum.testBoxExtentB(v_add(center, center), boxOccludersBoxes[i].bmax)) // todo: check if needed at all. We actually
                                                                                       // perform visibility test within addOccluder
      continue;
    closestBoxOccluders.push_back(IPoint2(i, v_extract_xi(v_cast_vec4i(dist2))));
  }
  for (int i = 0; i < dynamicBoxOccluders.size(); ++i)
  {
    bbox3f box;
    v_bbox3_init(box, dynamicBoxOccluders[i].tm, dynamicBoxOccluders[i].box);
    if (!viewFrustum.testBoxB(box.bmin, box.bmax))
      continue;
    vec3f center = v_bbox3_center(box);

    vec3f centerVec = v_sub(center, viewPos);
    vec3f dist2 = v_length3_sq(centerVec);
    if (v_test_vec_x_lt(v_length3_sq_x(v_sub(box.bmax, center)), v_mul_x(dist2, minBoxApprox2)))
      continue;
    closestBoxOccluders.push_back(IPoint2(i | (1 << 31), v_extract_xi(v_cast_vec4i(dist2))));
  }

  enum
  {
    MAXIMUM_BOXES = 24,
    MAXIMUM_QUADS = 32
  };
  const int maxBoxCount = occlusion.hasGPUFrame() ? 16 : MAXIMUM_BOXES;
  const int maxQuadCount = occlusion.hasGPUFrame() ? 20 : MAXIMUM_QUADS;
  change_occluders_metric(closestBoxOccluders.size(), maxBoxCount, minBoxMetric, occlusion.hasGPUFrame() ? 0.06 : 0.01);
  change_occluders_metric(closestQuadOccluders.size(), maxQuadCount, minQuadMetric, occlusion.hasGPUFrame() ? 0.06 : 0.01);
  if (!closestQuadOccluders.size() && !closestBoxOccluders.size())
    return false;

  occlusion.startSWOcclusion(0.005f); // todo: change zn

  if (closestBoxOccluders.size())
  {
    select_best_occluders(closestBoxOccluders, maxBoxCount);
    StaticTab<mat44f, MAXIMUM_BOXES> occluderTM;
    StaticTab<bbox3f, MAXIMUM_BOXES> occluderBox;
    vec3f bmax = V_C_HALF;
    vec3f bmin = v_neg(bmax);
    occluderTM.resize(closestBoxOccluders.size());
    occluderBox.resize(closestBoxOccluders.size());
    for (int i = 0; i < closestBoxOccluders.size(); ++i)
    {
      int id = closestBoxOccluders[i].x & ~(1 << 31);
      if (closestBoxOccluders[i].x & (1 << 31))
      {
        occluderTM[i] = dynamicBoxOccluders[id].tm;
        occluderBox[i] = dynamicBoxOccluders[id].box;
      }
      else
      {
        occluderBox[i].bmin = bmin;
        occluderBox[i].bmax = bmax;
        occluderTM[i] = boxOccluders[id];
      }
    }
    occlusion.finalizeBoxes(occluderBox.data(), occluderTM.data(), occluderBox.size());
  }
  if (closestQuadOccluders.size())
  {
    StaticTab<vec4f, MAXIMUM_QUADS * 4> quads;
    select_best_occluders(closestQuadOccluders, maxQuadCount);
    quads.resize(closestQuadOccluders.size() * 4);
    for (int i = 0; i < closestQuadOccluders.size(); ++i)
    {
      int id = closestQuadOccluders[i].x & ~(1 << 31);
      if (closestQuadOccluders[i].x & (1 << 31))
        memcpy(&quads[i * 4], &dynamicQuadOccluders[id], sizeof(vec3f) * 4);
      else
        memcpy(&quads[i * 4], &quadOccluders[id], sizeof(vec3f) * 4);
    }
    occlusion.finalizeQuads(quads.data(), quads.size() / 4);
  }
  return true;
}

#include <debug/dag_debug3d.h>
void OcclusionMap::debugRender()
{
  if (!quadOccluders.size() && !boxOccluders.size() && !dynamicBoxOccluders.size() && !dynamicQuadOccluders.size())
    return;
  begin_draw_cached_debug_lines(true, false);
  for (int i = 0; i < quadOccluders.size(); ++i)
  {
    mat44f tm = quadOccluders[i];
    draw_cached_debug_quad(tm.col0, tm.col1, tm.col3, tm.col2, E3DCOLOR(100, 120, 255, 100));
  }
  bbox3f unitbox;
  unitbox.bmax = V_C_HALF;
  unitbox.bmin = v_neg(unitbox.bmax);
  for (int i = 0; i < boxOccluders.size(); ++i)
  {
    mat43f mat44;
    v_mat44_transpose_to_mat43(mat44, boxOccluders[i]);
    draw_cached_debug_solid_box(&mat44, &unitbox, 1, E3DCOLOR(100, 200, 200, 100));
  }
  for (int i = 0; i < dynamicBoxOccluders.size(); ++i)
  {
    mat43f wtm;
    v_mat44_transpose_to_mat43(wtm, dynamicBoxOccluders[i].tm);
    draw_cached_debug_solid_box(&wtm, &dynamicBoxOccluders[i].box, 1, E3DCOLOR(50, 200, 50, 100));
  }
  for (int i = 0; i < dynamicQuadOccluders.size(); ++i)
    draw_cached_debug_quad(dynamicQuadOccluders[i].p0, dynamicQuadOccluders[i].p1, dynamicQuadOccluders[i].p2,
      dynamicQuadOccluders[i].p3, E3DCOLOR(150, 220, 55, 100));
  end_draw_cached_debug_lines();
}

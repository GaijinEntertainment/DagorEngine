// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_vecMathCompatibility.h>
#include <render/renderShadowObject.h>
#include <render/psm.h>
#include <math/dag_plane3.h>
#include <math/dag_frustum.h>
#include <debug/dag_debug.h>

#define ALMOST_ZERO(F) (fabsf(F) <= 2.0f * FLT_EPSILON)

static Point3 transform(const TMatrix4 &tm, const Point3 &pt)
{
  Point4 out;
  tm.transform(pt, out);
  out.unify();
  return Point3(out.x, out.y, out.z);
}
static void XFormBoundingBox(BBox3 &result, const BBox3 &src, const TMatrix4 &matrix)
{
  BBox3 res;

  if (src.isempty())
  {
    result.setempty();
    return;
  }
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
    {
      Point4 out;
      matrix.transform(Point3(src.lim[i].x, src.lim[j].y, src.lim[0].z), out);
      res += Point3(out.x, out.y, out.z) / out.w;
      matrix.transform(Point3(src.lim[i].x, src.lim[j].y, src.lim[1].z), out);
      res += Point3(out.x, out.y, out.z) / out.w;
    }
  result = res;
}

static void transform_with_wclip(const Point3 &in, const TMatrix4 &transform, float wThreshold, BBox3 &add)
{
  float x, y, z, w;

  x = in.x * transform.m[0][0] + in.y * transform.m[1][0] + in.z * transform.m[2][0] + transform.m[3][0];
  y = in.x * transform.m[0][1] + in.y * transform.m[1][1] + in.z * transform.m[2][1] + transform.m[3][1];
  z = in.x * transform.m[0][2] + in.y * transform.m[1][2] + in.z * transform.m[2][2] + transform.m[3][2];
  w = in.x * transform.m[0][3] + in.y * transform.m[1][3] + in.z * transform.m[2][3] + transform.m[3][3];

  float wtest = fabsf(w) - 1e-6f;
  float wtest2 = w - wThreshold;

  add += Point3(fsel(wtest, x / w, 0.f), fsel(wtest, y / w, 0.f), fsel(wtest2, z / w, 0.f));
}

static bool intersect_AABB2D(BBox3 &c, const BBox3 &a, const BBox3 &b)
{
  c[0].z = c[1].z = 0.0f;

  if (a[1].x < b[0].x || a[0].x > b[1].x)
    return false;

  if (a[1].y < b[0].y || a[0].y > b[1].y)
    return false;

  // x
  c[1].x = min(a[1].x, b[1].x);
  c[0].x = max(a[0].x, b[0].x);
  // y
  c[1].y = min(a[1].y, b[1].y);
  c[0].y = max(a[0].y, b[0].y);
  return true;
}

static TMatrix4 light_space(const Point3 &viewLightDir)
{

  // build light space lookAt matrix
  const Point3 zAxis(0.f, 0.f, 1.f);
  const Point3 yAxis(0.f, 1.f, 0.f);
  Point3 axis;

  if (fabsf(viewLightDir * yAxis) > 0.99f)
    axis = zAxis;
  else
    axis = yAxis;

  return matrix_look_at_lh(Point3(0, 0, 0), viewLightDir, axis);
}


static void GetTerrainBoundingBox(Tab<BBox3> &shadowReceiversLocal, const Frustum &sceneFrustum, const BBox3 &ground)
{
  Point3 smq_start = ground[0];
  Point3 smq_altitude = Point3(ground[0].x, ground[1].y, ground[0].z);
  Point3 smq_width(ground.width().x, 0.f, 0.f);
  Point3 smq_height(0.f, 0.f, ground.width().z);
  float altitude = ground[1].y - ground[0].y;

  for (int k = 0; k < 4 * 4; k++)
  {
    float kx = float(k & 0x3);
    float ky = float((k >> 2) & 0x3);
    BBox3 hugeBox;
    hugeBox[0] = smq_start + (kx / 4.f) * smq_width + (ky / 4.f) * smq_height;
    hugeBox[1] = smq_altitude + ((kx + 1.f) / 4.f) * smq_width + ((ky + 1.f) / 4.f) * smq_height;
    int hugeResult = sceneFrustum.testBox(hugeBox);
    if (hugeResult != 2) //  2 requires more testing...  0 is fast reject, 1 is fast accept
    {
      if (hugeResult == 1)
      {
        shadowReceiversLocal.push_back(hugeBox);
      }
      continue;
    }


    vec3f big_box_center2 = v_make_vec4f(smq_start.x + (kx * (4.f / 16.0f) + 0.5f / 16.0f) * smq_width.x,
      smq_start.y + altitude * 0.5f, smq_start.z + (ky * (4.f / 16.0f) + 0.5f / 16.0f) * smq_height.z, 0);
    vec3f big_box_extent = v_make_vec4f(smq_width.x / 16.0f, altitude, smq_height.z / 16.0f, 0);

    vec3f big_box_step = v_make_vec4f(smq_width.x / 8.0f, 0, 0, smq_height.z / 8.0f);

    big_box_center2 = v_add(big_box_center2, big_box_center2);

    vec3f big_box_center_start2 = big_box_center2;
    float jy = ky * 4.f;
    for (int jjy = 0; jjy < 4; jjy++, jy += 1.0f)
    {
      big_box_center2 = big_box_center_start2;
      float jx = kx * 4.f;
      for (int jjx = 0; jjx < 4; jjx++, jx += 1.0f)
      {
        int bigResult = sceneFrustum.testBoxExtent(big_box_center2, big_box_extent);
        big_box_center2 = v_add(big_box_center2, big_box_step);
        if (bigResult != 2)
        {
          if (bigResult == 1)
          {
            BBox3 bigBox;
            bigBox[0] = smq_start + (jx / 16.f) * smq_width + (jy / 16.f) * smq_height;
            bigBox[1] = smq_altitude + ((jx + 1.f) / 16.f) * smq_width + ((jy + 1.f) / 16.f) * smq_height;
            shadowReceiversLocal.push_back(bigBox);
          }
          continue;
        }

        vec3f small_box_center2 = v_make_vec4f(smq_start.x + (jx * (4.f / 64.0f) + 0.5f / 64.0f) * smq_width.x,
          smq_start.y + altitude * 0.5f, smq_start.z + (jy * (4.f / 64.0f) + 0.5f / 64.0f) * smq_height.z, 0);
        vec3f small_box_extent = v_make_vec4f(smq_width.x / 64.0f, altitude, smq_height.z / 64.0f, 0);

        vec3f small_box_step = v_make_vec4f(smq_width.x / 32.0f, 0, 0, smq_height.z / 32.0f);

        small_box_center2 = v_add(small_box_center2, small_box_center2);

        vec3f small_box_center_start2 = small_box_center2;
        for (int q = 0; q < 4; q++)
        {
          int stack = 0;
          small_box_center2 = small_box_center_start2;
          for (int r = 0; r < 4; r++)
          {
            if (sceneFrustum.testBoxExtentB(small_box_center2, small_box_extent))
              stack |= (1 << r);
            small_box_center2 = v_add(small_box_center2, small_box_step);
          }
          small_box_center_start2 = v_add(small_box_center_start2, v_perm_yzwx(small_box_step));

          if (stack)
          {
            float iy = jy * 4.f + float(q);
            float firstX = 0, lastX = 0;
            int i;
            for (i = 0; i < 4; i++)
            {
              if ((stack & (1 << i)) != 0)
              {
                firstX = float(i);
                break;
              }
            }
            for (i = 3; i >= 0; i--)
            {
              if ((stack & (1 << i)) != 0)
              {
                lastX = float(i);
                break;
              }
            }
            firstX += jx * 4.f;
            lastX += jx * 4.f;

            BBox3 coalescedBox;
            coalescedBox[0] = smq_start + (firstX / 64.f) * smq_width + (iy / 64.f) * smq_height;
            coalescedBox[1] = smq_altitude + ((lastX + 1.f) / 64.f) * smq_width + ((iy + 1.f) / 64.f) * smq_height;
            shadowReceiversLocal.push_back(coalescedBox);
          }
        }
      }
      big_box_center_start2 = v_add(big_box_center_start2, v_perm_yzwx(big_box_step));
    }
  }
}


TMatrix4 TSM::BuildClippedMatrix(const TMatrix4 &PPLSFinal, float &minZ, float &maxZ, bool trapezoidal) const
{
  TMatrix4 PPLSFinalLc = m_View * PPLSFinal;
  float eW = m_XPSM_EpsilonW;
  // find PPLS AABB's for caster*s and receivers
  BBox3 PPLSCastersAABB;
  for (int i = 0; i < m_ShadowCasterPointsLocal.size(); i++)
    for (int j = 0; j < 8; j++)
      transform_with_wclip(m_ShadowCasterPointsLocal[i].point(j), PPLSFinalLc, eW, PPLSCastersAABB);

  BBox3 PPLSReceiversAABB;
  for (int i = 0; i < m_ShadowReceiverPointsLocal.size(); i++)
    for (int j = 0; j < 8; j++)
      transform_with_wclip(m_ShadowReceiverPointsLocal[i].point(j), PPLSFinalLc, eW, PPLSReceiversAABB);
  // find post projection space focus region
  Frustum eyeFrustum(m_View * m_Projection); // autocomputes all the extrema points
  vec4f pntList[8];
  eyeFrustum.generateAllPointFrustm(pntList);
  BBox3 frustumBox;
  for (int i = 0; i < 8; i++)
    transform_with_wclip(as_point3(&pntList[i]), PPLSFinalLc, eW, frustumBox);

  if (!PPLSReceiversAABB.isempty())
    PPLSReceiversAABB = PPLSReceiversAABB.getIntersection(frustumBox);
  else
    PPLSReceiversAABB = frustumBox;
  if (PPLSCastersAABB.isempty())
    PPLSCastersAABB = frustumBox;

  BBox3 PPLSFocusRegionAABB;
  if (!intersect_AABB2D(PPLSFocusRegionAABB, PPLSCastersAABB, PPLSReceiversAABB))
  {
    PPLSFocusRegionAABB = PPLSReceiversAABB;
  }
  else
  {
    BBox3 PPLSFocusRegionAABB2;
    if (intersect_AABB2D(PPLSFocusRegionAABB2, frustumBox, PPLSFocusRegionAABB))
      PPLSFocusRegionAABB = PPLSFocusRegionAABB2;
  }

  // build focus region linear basis
  if (trapezoidal)
  {
    PPLSFocusRegionAABB[1].x = min(1.f, PPLSFocusRegionAABB[1].x);
    PPLSFocusRegionAABB[0].x = max(-1.f, PPLSFocusRegionAABB[0].x);
    PPLSFocusRegionAABB[1].y = min(1.f, PPLSFocusRegionAABB[1].y);
    PPLSFocusRegionAABB[0].y = max(-1.f, PPLSFocusRegionAABB[0].y);
    maxZ = 1;
    minZ = 0;
  }
  else
  {
    minZ = PPLSCastersAABB[0].z;
    maxZ = PPLSReceiversAABB[1].z;
    if (max_z_dispertion < MAX_REAL)
      minZ = maxZ - max_z_dispertion;
    if (!m_ShadowCasterPointsLocal.size())
      minZ -= 350;
  }
  Point3 minPtBox = PPLSFocusRegionAABB[0];
  Point3 maxPtBox = PPLSFocusRegionAABB[1];

  // find focusRegionBasis^-1
  TMatrix4 unitCubeBasis(safeinv(maxPtBox.x - minPtBox.x), 0.0f, 0.0f, 0.0f, 0.0f, safeinv(maxPtBox.y - minPtBox.y), 0.0f, 0.0f, 0.0f,
    0.0f, safeinv(maxZ - minZ), 0.0f, safediv(-minPtBox.x, (maxPtBox.x - minPtBox.x)), safediv(-minPtBox.y, (maxPtBox.y - minPtBox.y)),
    safediv(-minZ, (maxZ - minZ)), 1.0f);

  // transform from a unit cube into d3d normalized space
  const TMatrix4 normalizedSpaceD3D(2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f);

  // finally multiply all transformations
  return PPLSFinalLc * unitCubeBasis * normalizedSpaceD3D;
}

//-----------------------------------------------------------------------------
// Name: BuildTSMProjectionMatrix
// Desc: Builds a trapezoidal shadow transformation matrix
//-----------------------------------------------------------------------------

void TSM::buildTSMMatrix(const BBox3 &frustumAABB2D, const BBox3 &frustumBox, const Point3 *frustumPnts, float xi, float delta,
  const Point2 &scale, TMatrix4 &trapezoid_space)
{
  float lambda = frustumAABB2D[1].x - frustumAABB2D[0].x;

  float delta_proj = delta * lambda;

  float eta = (lambda * delta_proj * (1.f + xi)) / (lambda * (1.f - xi) - 2.f * delta_proj);

  //  compute the projection point a distance eta from the top line.  this point is on the center line, y=0
  Point2 projectionPtQ(frustumAABB2D[1].x + eta, 0.f);

  //  find the maximum slope from the projection point to any point in the frustum.  this will be the
  //  projection field-of-view
  float max_slope = -1e32f;
  float min_slope = 1e32f;

  for (int i = 0; i < 8; i++)
  {
    Point2 tmp(frustumPnts[i].x * scale.x, frustumPnts[i].y * scale.y);
    float x_dist = tmp.x - projectionPtQ.x;
    if (!(ALMOST_ZERO(tmp.y) || ALMOST_ZERO(x_dist)))
    {
      max_slope = max(max_slope, tmp.y / x_dist);
      min_slope = min(min_slope, tmp.y / x_dist);
    }
  }

  float xn = eta;
  float xf = lambda + eta;

  TMatrix4 ptQ_xlate(-1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, projectionPtQ.x, 0.f, 0.f, 1.f);
  trapezoid_space = trapezoid_space * ptQ_xlate;

  //  this shear balances the "trapezoid" around the y=0 axis (no change to the projection pt position)
  //  since we are redistributing the trapezoid, this affects the projection field of view (shear_amt)
  float shear_amt = (max_slope + fabsf(min_slope)) * 0.5f - max_slope;
  max_slope = max_slope + shear_amt;

  TMatrix4 trapezoid_shear(1.f, shear_amt, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

  trapezoid_space = trapezoid_space * trapezoid_shear;


  float z_aspect = (frustumBox[1].z - frustumBox[0].z) / (frustumAABB2D[1].y - frustumAABB2D[0].y);

  //  perform a 2DH projection to 'unsqueeze' the top line.
  TMatrix4 trapezoid_projection(xf / (xf - xn), 0.f, 0.f, 1.f, 0.f, 1.f / max_slope, 0.f, 0.f, 0.f, 0.f, 1.f / (z_aspect * max_slope),
    0.f, -xn * xf / (xf - xn), 0.f, 0.f, 0.f);

  trapezoid_space = trapezoid_space * trapezoid_projection;
  //  the x axis is compressed to [0..1] as a result of the projection, so expand it to [-1,1]
  TMatrix4 biasedScaleX(2.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, -1.f, 0.f, 0.f, 1.f);
  trapezoid_space = trapezoid_space * biasedScaleX;
}

void TSM::BuildTSMProjectionMatrix()
{
  //  this isn't strictly necessary for TSMs; however, my 'light space' matrix has a
  //  degeneracy when view==light, so this avoids the problem.
  if (fabsf(m_fCosGamma) >= 0.95f) //
  {
    BuildOrthoShadowProjectionMatrix();
    return;
  }

  //  get the near and the far plane (points) in eye space.
  Point3 frustumPnts[8];

  Frustum eyeFrustum(m_Projection); // autocomputes all the extrema points
  vec4f pntList[8];
  eyeFrustum.generateAllPointFrustm(pntList);

  for (int i = 0; i < 4; i++)
  {
    frustumPnts[i] = as_point3(&pntList[(i << 1)]);           // far plane
    frustumPnts[i + 4] = as_point3(&pntList[(i << 1) | 0x1]); // near plane
  }

  //   we need to transform the eye into the light's post-projective space.
  //   however, the sun is a directional light, so we first need to find an appropriate
  //   rotate/translate matrix, before constructing an ortho projection.
  //   this matrix is a variant of "light space" from LSPSMs, with the Y and Z axes permuted

  Point3 leftVector, upVector, viewVector;
  const Point3 eyeVector(0.f, 0.f, -1.f); //  eye is always -Z in eye space

  //  code copied straight from BuildLSPSMProjectionMatrix
  upVector = m_lightDir % m_View; // lightDir is defined in eye space, so xform it
  leftVector = normalize(upVector % eyeVector);
  viewVector = upVector % leftVector;

  TMatrix4 lightSpaceBasis;
  lightSpaceBasis._11 = leftVector.x;
  lightSpaceBasis._12 = viewVector.x;
  lightSpaceBasis._13 = -upVector.x;
  lightSpaceBasis._14 = 0.f;
  lightSpaceBasis._21 = leftVector.y;
  lightSpaceBasis._22 = viewVector.y;
  lightSpaceBasis._23 = -upVector.y;
  lightSpaceBasis._24 = 0.f;
  lightSpaceBasis._31 = leftVector.z;
  lightSpaceBasis._32 = viewVector.z;
  lightSpaceBasis._33 = -upVector.z;
  lightSpaceBasis._34 = 0.f;
  lightSpaceBasis._41 = 0.f;
  lightSpaceBasis._42 = 0.f;
  lightSpaceBasis._43 = 0.f;
  lightSpaceBasis._44 = 1.f;

  // debug_matrix("lightspacebasis", lightSpaceBasis);
  BBox3 frustumBox;
  for (int i = 0; i < 8; i++)
  {
    frustumPnts[i] = frustumPnts[i] * lightSpaceBasis;
    frustumBox += frustumPnts[i];
  }
  //  build an off-center ortho projection that translates and scales the eye frustum's 3D AABB to the unit cube

  //  also - transform the shadow caster bounding boxes into light projective space.  we want to translate along the Z axis so that
  //  all shadow casters are in front of the near plane.
  BBox3 casterBox, rcvrBox;
  TMatrix4 lightSpaceBasisLc = m_View * lightSpaceBasis;
  for (int i = 0; i < m_ShadowCasterPointsLocal.size(); i++)
  {
    for (int j = 0; j < 8; j++)
      casterBox += m_ShadowCasterPointsLocal[i].point(j) * lightSpaceBasisLc;
  }
  if (!m_ShadowCasterPointsLocal.size())
    casterBox = frustumBox;

  if (m_ShadowReceiverPointsLocal.size())
  {
    for (int i = 0; i < m_ShadowReceiverPointsLocal.size(); i++)
      for (int j = 0; j < 8; j++)
        rcvrBox += m_ShadowReceiverPointsLocal[i].point(j) * lightSpaceBasisLc;
    // XFormBoundingBox(rcvrBox, m_ShadowReceiver, lightSpaceBasis);
    BBox3 intersected = rcvrBox.getIntersection(frustumBox);
    if (intersected.isempty())
      intersected = frustumBox;
    else
    {
      frustumBox[0].x = intersected[0].x;
      frustumBox[0].y = intersected[0].y;
      frustumBox[1].x = intersected[1].x;
      frustumBox[1].y = intersected[1].y;
    }
  }
  else
    rcvrBox = frustumBox;

  float min_z = min(casterBox[0].z, frustumBox[0].z);
  float max_z = max(rcvrBox[1].z, frustumBox[1].z);
  if (!m_ShadowCasterPointsLocal.size())
    min_z -= 350;

  if (min_z <= 0.f)
  {
    for (int i = 0; i < 8; i++)
      frustumPnts[i].z += -min_z + 1.f;
    frustumBox[0].z += -min_z + 1.f;
    frustumBox[1].z += -min_z + 1.f;
    lightSpaceBasis._43 += -min_z + 1.f;
    max_z = -min_z + max_z + 1.f;
    min_z = 1.f;
  }

  if (max_z_dispertion > max_z - min_z && max_z_dispertion < MAX_REAL)
    max_z = min_z + max_z_dispertion;
  TMatrix4 lightSpaceOrtho =
    matrix_ortho_off_center_lh(frustumBox[0].x, frustumBox[1].x, frustumBox[0].y, frustumBox[1].y, min_z, max_z);
  TMatrix4 lightSpace = lightSpaceBasis * lightSpaceOrtho;

  //  transform the view frustum by the new matrix
  for (int i = 0; i < 8; i++)
    frustumPnts[i] = transform(lightSpaceOrtho, frustumPnts[i]);
  // frustumPnts[i] = frustumPnts[i] * lightSpaceOrtho;


  Point2 centerPts[2];
  //  near plane
  centerPts[0].x = 0.25f * (frustumPnts[4].x + frustumPnts[5].x + frustumPnts[6].x + frustumPnts[7].x);
  centerPts[0].y = 0.25f * (frustumPnts[4].y + frustumPnts[5].y + frustumPnts[6].y + frustumPnts[7].y);
  //  far plane
  centerPts[1].x = 0.25f * (frustumPnts[0].x + frustumPnts[1].x + frustumPnts[2].x + frustumPnts[3].x);
  centerPts[1].y = 0.25f * (frustumPnts[0].y + frustumPnts[1].y + frustumPnts[2].y + frustumPnts[3].y);
  float delta = m_fTSM_Delta;
  if (fabsf(m_fCosGamma) >= 0.95f)
    delta = 0.5;

  Point2 centerOrig = (centerPts[0] + centerPts[1]) * 0.5f;
  float half_center_len = length(centerPts[1] - centerOrig);

  if (half_center_len < FLT_EPSILON)
  {
    BuildOrthoShadowProjectionMatrix(); //==FIXME: should not happen!
    return;
  }

  TMatrix4 trapezoid_space;

  TMatrix4 xlate_center(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, -centerOrig.x, -centerOrig.y, 0.f, 1.f);

  float x_len = centerPts[1].x - centerOrig.x;
  float y_len = centerPts[1].y - centerOrig.y;

  float cos_theta = x_len / half_center_len;
  float sin_theta = y_len / half_center_len;

  TMatrix4 rot_center(cos_theta, -sin_theta, 0.f, 0.f, sin_theta, cos_theta, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

  //  this matrix transforms the center line to y=0.
  //  since Top and Base are orthogonal to Center, we can skip computing the convex hull, and instead
  //  just find the view frustum X-axis extrema.  The most negative is Top, the most positive is Base
  //  Point Q (trapezoid projection point) will be a point on the y=0 line.
  trapezoid_space = xlate_center * rot_center;
  BBox3 frustumAABB2D;
  for (int i = 0; i < 8; i++)
  {
    frustumPnts[i] = transform(trapezoid_space, frustumPnts[i]);
    // frustumPnts[i] = frustumPnts[i] * trapezoid_space;
    frustumAABB2D += frustumPnts[i];
  }

  float x_scale = max(fabsf(frustumAABB2D[1].x), fabsf(frustumAABB2D[0].x));
  float y_scale = max(fabsf(frustumAABB2D[1].y), fabsf(frustumAABB2D[0].y));
  x_scale = 1.f / x_scale;
  y_scale = 1.f / y_scale;

  //  maximize the area occupied by the bounding box
  TMatrix4 scale_center(x_scale, 0.f, 0.f, 0.f, 0.f, y_scale, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

  trapezoid_space = trapezoid_space * scale_center;

  //  scale the frustum AABB up by these amounts (keep all values in the same space)
  frustumAABB2D[0].x *= x_scale;
  frustumAABB2D[1].x *= x_scale;
  frustumAABB2D[0].y *= y_scale;
  frustumAABB2D[1].y *= y_scale;
  //  compute eta.
  // float delta_proj = m_fTSM_Delta * lambda; //focusPt.x - frustumAABB2D[0].x;

  // const float xi = -0.6f;  // 80% line
  TMatrix4 ts = trapezoid_space;
  float bestXi = -0.6;

  buildTSMMatrix(frustumAABB2D, frustumBox, frustumPnts, bestXi, delta, Point2(x_scale, y_scale), trapezoid_space);

  if (!boxOfInterest.isempty())
  {
    BBox3 box;
    XFormBoundingBox(box, boxOfInterest, m_View * lightSpace * trapezoid_space);
    float stArea = box.width().y * box.width().x;

    TMatrix4 tspace2 = ts;
    buildTSMMatrix(frustumAABB2D, frustumBox, frustumPnts, bestXi, 0.52, Point2(x_scale, y_scale), tspace2);
    XFormBoundingBox(box, boxOfInterest, m_View * lightSpace * tspace2);

    // debug("0.4area = %g*%g=%g",
    //   box.width().x*1024,box.width().y*1024,box.width().y*box.width().x*1024*1024);
    if (box.width().y * box.width().x > stArea)
    {
      delta = 0.52;
      trapezoid_space = tspace2;
      stArea = box.width().y * box.width().x;
    } // else
    {
      int total_it = 15;
      float area = stArea;
      for (int i = 0; i < total_it; ++i)
      {
        float xi = -0.67 * float(i + 1) / (total_it + 1);
        tspace2 = ts;
        buildTSMMatrix(frustumAABB2D, frustumBox, frustumPnts, xi, delta, Point2(x_scale, y_scale), tspace2);

        XFormBoundingBox(box, boxOfInterest, m_View * lightSpace * tspace2);
        // float carea = min(box.width().y,box.width().x);
        float carea = box.width().y * box.width().x;
        // debug("area = %g, xi=%g", carea*1024*1024, xi);
        if (carea > area)
        {
          area = carea;
          trapezoid_space = tspace2;
        }
      }
    }
  }
  trapezoid_space = lightSpace * trapezoid_space;
  float minZ, maxZ;
  m_LightViewProj = BuildClippedMatrix(trapezoid_space, minZ, maxZ, true);
  m_XPSM_ZBias = 10.0f * m_XPSM_Bias / (maxZ - minZ);
  isOrthoNow = false;
}

//-----------------------------------------------------------------------------
// Name: BuildOrthoShadowProjectionMatrix
// Desc: Builds an orthographic shadow transformation matrix
//-----------------------------------------------------------------------------
void TSM::BuildOrthoShadowProjectionMatrix()
{
  // project view vector into xy plane and find
  // unit projection vector
  // final, combined PPLS transformation: LightSpace * PPLS Transform
  float minZ, maxZ;
  m_LightViewProj = BuildClippedMatrix(light_space(-(m_lightDir % m_View)), minZ, maxZ);
  // compute correct PPLS Z bias
  m_XPSM_ZBias = 10 * m_XPSM_Bias / (maxZ - minZ);
  isOrthoNow = true;
}

void TSM::BuildXPSMProjectionMatrix()
{
  if (fabsf(m_fCosGamma) >= 0.85f) //
  {
    BuildOrthoShadowProjectionMatrix();
    return;
  }
  // coef - constant tuned for near/far objects trade off and
  // scene scale
  const float coef = 0.05f * m_XPSM_Coef;
  // epsilonW - define how far to "infinity" warping can be pushed
  // in critical case
  const float epsilonW = m_XPSM_EpsilonW;

  // shadow casters and receivers all defined in VIEW SPACE
  // copy shadow casters and receivers
  // transform light direction into view space

  TMatrix4 lightSpace = light_space(-(m_lightDir % m_View));


  // transform view vector into light space
  Point3 V(lightSpace.m[2][0], lightSpace.m[2][1], lightSpace.m[2][2]);
  //  (0, 0, 1) * LightSpace

  // project view vector into xy plane and find
  // unit projection vector
  Point2 unitP(V.x, V.y);

  float unitPLength = unitP.length();

  // PPLS (post projection light space) transformation
  TMatrix4 PPLSTransform;
  // D3DXMATRIX PPLSTransform;

  // check projection vector length
  if (unitPLength > 0.15f)
  {
    // use perspective reparameterization
    // normalize projection vector
    unitP *= 1.0f / unitPLength;

    // transform shadow casters and receivers to light space
    // transform shadow casters into light space
    float minCastersProj = 1.0f;
    float minReceiversProj = 1.0f;
    // transform shadow receivers into light space
    // project casters and receivers points into unit projection vector and
    // find minimal value
    TMatrix4 lightSpaceLc = m_View * lightSpace;
    for (int i = 0; i < m_ShadowCasterPointsLocal.size(); i++)
      for (int j = 0; j < 8; j++)
      {
        Point3 pt = m_ShadowCasterPointsLocal[i].point(j) * lightSpaceLc;
        float proj = pt.x * unitP.x + pt.y * unitP.y;
        minCastersProj = min(minCastersProj, proj);
      }

    for (int i = 0; i < m_ShadowReceiverPointsLocal.size(); i++)
      for (int j = 0; j < 8; j++)
      {
        Point3 pt = m_ShadowReceiverPointsLocal[i].point(j) * lightSpaceLc;
        float proj = pt.x * unitP.x + pt.y * unitP.y;
        minReceiversProj = min(minReceiversProj, proj);
      }

    if (!m_ShadowReceiverPointsLocal.size())
    {
      Frustum frustum(m_View * m_Projection); // autocomputes all the extrema points
      vec4f pntList[8];
      frustum.generateAllPointFrustm(pntList);
      for (int j = 0; j < 8; j++)
      {
        Point3 pt = as_point3(&pntList[j]) * lightSpace;
        float proj = pt.x * unitP.x + pt.y * unitP.y;
        minReceiversProj = min(minReceiversProj, proj);
      }
    }
    if (!m_ShadowCasterPointsLocal.size())
      minCastersProj = minReceiversProj;

    // find focus region interval minimal point
    // max(min Casters, min Receivers)
    float minFocusRegionProj = max(minCastersProj, minReceiversProj);

    // find maximum projection vector length
    // 1 / (minProj * maxProjeVectorLength  + 1) != 0
    // minProj * maxProjVectorLength + 1 = thresholdW
    // !!!! TEST + RECEIVERS && CASTERS CODE
    float maxLengthP = fabsf(minFocusRegionProj) > FLT_EPSILON ? (epsilonW - 1.0f) / minFocusRegionProj : 0.0f;

    // find optimal (fixed warping) projection vector length
    float cosGamma = V.x * unitP.x + V.y * unitP.y;
    float lengthP = coef / cosGamma;

    // clip projection vector length
    if (maxLengthP > 0.0f && lengthP > maxLengthP)
    {
      lengthP = maxLengthP;
    }

    // calculate Projection matrix
    TMatrix4 Projection(1.0f, 0.0f, 0.0f, unitP.x * lengthP, 0.0f, 1.0f, 0.0f, unitP.y * lengthP, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f);
    // to maximize shadow map resolution
    // calculate Z rotation basis
    TMatrix4 ZRotation(unitP.x, unitP.y, 0.0f, 0.0f, unitP.y, -unitP.x, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    //
    // combine transformations
    PPLSTransform = Projection * ZRotation;
    isOrthoNow = false;
  }
  else
  {
    // miner lamp case, use orthogonal shadows
    PPLSTransform = TMatrix4::IDENT;
    isOrthoNow = true;
  }

  // final, combined PPLS transformation: LightSpace * PPLS Transform
  float minZ, maxZ;
  m_LightViewProj = BuildClippedMatrix(lightSpace * PPLSTransform, minZ, maxZ);
  // compute correct PPLS Z bias
  m_XPSM_ZBias = safediv(10.0f * m_XPSM_Bias, (maxZ - minZ));
}

//-----------------------------------------------------------------------------
//  TSM::ComputeVirtualCameraParameters( )
//    computes the near & far clip planes for the virtual camera based on the
//    scene.
//
//    bounds the field of view for the virtual camera based on a swept-sphere/frustum
//    intersection.  if the swept sphere representing the extrusion of an object's bounding
//    sphere along the light direction intersects the view frustum, the object is added to
//    a list of interesting shadow casters.  the field of view is the minimum cone containing
//    all eligible bounding spheres.
//-----------------------------------------------------------------------------

void TSM::ComputeVirtualCameraParameters(RenderShadowObject *scenes, int count, const BBox3 &ground)
{
  //  frustum is in world space, so that bounding boxes are minimal size (xforming an AABB
  //  generally increases its volume).
  TMatrix4 modelViewProjection = m_View * m_Projection;
  Point3 sweepDir = -m_lightDir;

  Frustum sceneFrustum(modelViewProjection);

  m_ShadowCasterPointsLocal.clear();
  m_ShadowCasterObjects.clear();
  m_ShadowReceiverObjects.clear();
  m_ShadowReceiverPointsLocal.clear();

  for (unsigned int i = 0; i < count; i++)
  {
    BBox3 instanceBox = scenes[i].box;
    int inFrustum = (scenes[i].flags & RenderShadowObject::SHADOW_BOX_IS_SPHERE)
                      ? sceneFrustum.testSphereB(scenes[i].sphc, scenes[i].rad)
                      : sceneFrustum.testBoxB(instanceBox); //  0 = outside.  1 = inside.   2 = intersection
    switch (inFrustum)
    {
      case 0:
        // outside frustum -- test swept sphere for potential shadow caster
        if ((scenes[i].flags & scenes[i].SHADOW_VISIBLE_CASTER) == scenes[i].SHADOW_CASTER)
        {
          if (sceneFrustum.testSweptSphere(scenes[i].sphc, scenes[i].rad, sweepDir))
          {
            m_ShadowCasterPointsLocal.push_back(instanceBox);
            m_ShadowCasterObjects.push_back(&scenes[i]);
          }
        }
        break;
        //  big box intersects frustum.  test sub-boxen.  this improves shadow quality, since it allows
      case 1:
        //  fully inside frustum.  so store large bounding box
        {
          if (scenes[i].flags & scenes[i].SHADOW_CASTER)
          {
            m_ShadowCasterPointsLocal.push_back(instanceBox);
            m_ShadowCasterObjects.push_back(&scenes[i]);
          }
          if (scenes[i].flags & scenes[i].SHADOW_RECEIVER)
          {
            m_ShadowReceiverPointsLocal.push_back(instanceBox);
            m_ShadowReceiverObjects.push_back(&scenes[i]);
          }
          break;
        }
    }
  }

  //  add the biggest shadow receiver -- the ground!
  // m_ShadowReceiver += GetTerrainBoundingBox(m_ShadowReceiverPoints, modelView, sceneFrustum);
  if (!ground.isempty())
    GetTerrainBoundingBox(m_ShadowReceiverPointsLocal, sceneFrustum, ground);

  //  these are the limits specified by the physical camera
  //  gamma is the "tilt angle" between the light and the view direction.
  m_fCosGamma = m_lightDir.x * m_View._13 + m_lightDir.y * m_View._23 + m_lightDir.z * m_View._33;
}


void TSM::close() {}

TSM::TSM() :
  m_ShadowReceiverPointsLocal(tmpmem_ptr()),
  m_ShadowReceiverObjects(tmpmem_ptr()),
  m_ShadowCasterPointsLocal(tmpmem_ptr()),
  m_ShadowCasterObjects(tmpmem_ptr())
{
  m_bUnitCubeClip = true;
  m_bSlideBack = true;
  m_lightDir = Point3(0.f, 0.f, 0.f);

  m_XPSM_Coef = 0.05f;
  m_XPSM_Bias = 0.055f;
  m_XPSM_EpsilonW = 0.085f;

  m_fTSM_Delta = 0.1f;
  m_iShadowType = (int)SHADOWTYPE_TSM;

  max_z_dispertion = MAX_REAL;

  view_angle_wk = 1;
  view_angle_hk = 1;

  m_View.identity();
  m_Projection.identity();
}

TMatrix4 TSM::buildMatrix(RenderShadowObject *scenes, int count, const BBox3 &ground)
{
  ComputeVirtualCameraParameters(scenes, count, ground);
  switch (m_iShadowType)
  {
    case (int)SHADOWTYPE_TSM: BuildTSMProjectionMatrix(); break;
    case (int)SHADOWTYPE_ORTHO: BuildOrthoShadowProjectionMatrix(); break;
    case (int)SHADOWTYPE_XPSM: BuildXPSMProjectionMatrix(); break;
  }
  return m_LightViewProj;
}

#include <render/xcsm.h>
#include <math/dag_bounds3.h>
#include <math/dag_math3d.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>

// #define FIGHT_ALIASING 1
#define FIGHT_ALIASING 0

// D3D NDC cube: (-1, -1, 0) - (1, 1, 1)
TMatrix4 ComputedShadowSplit::NDCtoUC(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f);

TMatrix4 ComputedShadowSplit::UCtoNDC(2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, -1.0, 0.0, 1.0);

static TMatrix4 UniversalUCtoNDC(2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, -1.0, 0.0, 1.0);

XCSM::XCSM() {}

void XCSM::init(int width2, int height2, int count, float *zbias)
{
  smapWidth = width2;
  smapHeight = height2;
  splitNum = count;


  // ZBIAS
  float zbias_def[4] = {0.04f, 0.055f, 0.065f, 0.06f};
  if (!zbias)
    zbias = zbias_def;
  bool warp[4] = {true, true, true, true};
  bool align[4] = {true, true, true, false};
  // warp[0]= warp[1]=warp[2]=warp[3] = false;
  // align[0]= align[1]=align[2]=align[3] = false;
  // const float zbias[4] = {0.02f, 0.07f, 0.25f, 0.8f};
  // float zbias[4] = {0.05f,0.15f, 0.15f, 0.08f};
  int maxWDHT = width2 > height2 ? width2 : height2;
  float texelSize = float_nonzero(maxWDHT) ? 2048.0f / maxWDHT : 0.0f;

  int splitWidth = splitNum > 1 ? width2 / 2 : width2;
  int splitHeight = splitNum > 2 ? height2 / 2 : height2;

  G_ASSERT(splitNum <= 4);

  for (int i = 0; i < splitNum; ++i)
  {
    int w = ((splitNum == 3 && i == 2) ? splitWidth * 2 : splitWidth);
    float splitK;
    if (splitNum > 1)
      splitK = 0.85f + i * (1.0f - 0.85f) / (splitNum - 1);
    else
      splitK = 1;

    splitSet[i].init((i & 1) * splitWidth, (i >> 1) * splitHeight, w, splitHeight, splitK, zbias[i] * texelSize, 0.06 + 0.12 * i,
      align[i], warp[i]);
  }
}

void XCSM::close() {}

void XCSM::precomputeSplit(int i)
{
  float x = ((float)splitSet[i].x) / (float)smapWidth;
  float y = ((float)splitSet[i].y) / (float)smapHeight;
  float w = ((float)splitSet[i].width) / (float)smapWidth;
  float h = ((float)splitSet[i].height) / (float)smapHeight;
  float halfTexelX = 0.5f / ((float)smapWidth);
  float halfTexelY = 0.5f / ((float)smapHeight);

  splitOut[i].texTM =
    TMatrix4(w, 0.0f, 0.0f, 0.0f, 0.0f, -h, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, x + halfTexelX, h + y + halfTexelY, 0.0f, 1.0f);
}

void XCSM::computeFocusTM(const BBox3 &box, TMatrix4 &tm)
{
  tm = TMatrix4(1.0f / (box.lim[1].x - box.lim[0].x), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f / (box.lim[1].y - box.lim[0].y), 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f / (box.lim[1].z - box.lim[0].z), 0.0f, -box.lim[0].x / (box.lim[1].x - box.lim[0].x),
    -box.lim[0].y / (box.lim[1].y - box.lim[0].y), -box.lim[0].z / (box.lim[1].z - box.lim[0].z), 1.0f);
}

inline void view_matrix_from_look(const Point3 &look, TMatrix &tm)
{
  tm.setcol(0, Point3(0, 1, 0) % look);
  float xAxisLength = length(tm.getcol(0));
  if (xAxisLength <= 1.192092896e-06F)
  {
    tm.setcol(0, normalize(Point3(0, 0, look.y / fabsf(look.y)) % look));
    tm.setcol(1, normalize(look % tm.getcol(0)));
  }
  else
  {
    tm.setcol(0, tm.getcol(0) / xAxisLength);
    tm.setcol(1, normalize(look % tm.getcol(0)));
  }
  tm.setcol(2, look);
}


void XCSM::prepare(float wk, float hk, float znear, float zfar, const TMatrix4 &view, const Point3 &sun_dir, float shadow_dist)
// wk, hk, znear,zfar - perspective projection parameters
// view - view matrix
// sun_dir - direction of the sun
// lambda scales between log (1) and uniform (0) (unused now)
// shadow_dist - maximum shadow distance
{
  // compute camera projection/inverse projection
  float q = zfar / (zfar - znear);
  TMatrix4 cameraProj(wk, 0.0f, 0.0f, 0.0f, 0.0f, hk, 0.0f, 0.0f, 0.0f, 0.0f, q, 1.0f, 0.0f, 0.0f, -q * znear, 0.0f);

  TMatrix4 cameraInvProj(znear / wk, 0.0f, 0.0f, 0.0f, 0.0f, znear / hk, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f / q, 0.0f, 0.0f, znear,
    1.0f);

  float splitZ[MAX_SPLIT_NUM + 1]; // split Z ranges at camera post-projective space
                                   // format: [0, split[0].splitProjZ, split[1].splitProjZ,.., 1]
                                   // N's split Z range: (splitZ[N], splitZ[N + 1])
  splitZ[0] = 0.0f;
  for (int i = 0; i < splitNum; ++i)
  {
    precomputeSplit(i);
    splitZ[i + 1] = splitSet[i].splitZ;
  }
  splitZ[splitNum] = 1.0f;

  // assumed: sun_dir look from (0,0,0) AT light source
  // transform light direction into VIEW SPACE
  Point4 lightDir4 = Point4(sun_dir.x, sun_dir.y, sun_dir.z, 0.0f) * view;

  // invert light direction to get correct Z ordering (near objects z < far objects z)
  Point3 lightDirAlign(-lightDir4.x, -lightDir4.y, -lightDir4.z);

  // compute transformation that align light rays with Z axis (light space)
  static const float epsilon = 0.95;
  TMatrix4 alignZ;
  if (fabs(lightDirAlign.y) < epsilon)
  {
    alignZ = matrix_look_at_lh(Point3(0.0f, 0.0f, 0.0f), lightDirAlign, Point3(0.0f, 1.0f, 0.0f));
  }
  else
  {
    alignZ = matrix_look_at_lh(Point3(0.0f, 0.0f, 0.0f), lightDirAlign, Point3(-1.0f, 0.0f, 0.0f));
  }

  // transform view direction into light space
  Point4 viewDir = Point4(0.0f, 0.0f, 1.0f, 0.0f) * alignZ;

  TMatrix4 alignX;
  TMatrix4 invAlignX;

  // view vector projection length at XY plane
  // gamma - angle between view vector and they XY projection
  float cosGamma = sqrtf(viewDir.x * viewDir.x + viewDir.y * viewDir.y);
  bool isWarp = true;
  if (cosGamma > 0.15f)
  {
    isWarp = true;

    // normalize XY projection of view vector
    float x = viewDir.x / cosGamma;
    float y = viewDir.y / cosGamma;

    // compute transformation that rotate around Z
    // and align view vector XY projection with X axis
    alignX = TMatrix4(x, -y, 0.0f, 0.0f, y, x, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    // and inverse
    invAlignX = TMatrix4(x, y, 0.0f, 0.0f, -y, x, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  }
  else
  {
    isWarp = false;
    alignX.identity();
    invAlignX.identity();
  }
  //        isWarp = false;
  //    debug("isWarp %s cosGamma=%g", isWarp? "true" : "false", cosGamma);

  // transform from view space into light space
  TMatrix4 lightSpace = alignZ * alignX;

  // compute frustum splits
  Point4 unitBox[8] = {Point4(0.0f, 0.0f, 0.0f, 1.0f), Point4(0.0f, 1.0f, 0.0f, 1.0f), Point4(1.0f, 1.0f, 0.0f, 1.0f),
    Point4(1.0f, 0.0f, 0.0f, 1.0f), Point4(0.0f, 0.0f, 1.0f, 1.0f), Point4(0.0f, 1.0f, 1.0f, 1.0f), Point4(1.0f, 1.0f, 1.0f, 1.0f),
    Point4(1.0f, 0.0f, 1.0f, 1.0f)};

  // compute projection into unit cube for each frustum split
  for (int j = 0; j < splitNum; ++j)
  {
    unitBox[0].z = unitBox[1].z = unitBox[2].z = unitBox[3].z = splitZ[j];
    unitBox[4].z = unitBox[5].z = unitBox[6].z = unitBox[7].z = splitZ[j + 1];
    ;

    // transform frustum split from post-projective camera NDC space into light space
    // and compute bounding box of this frustum chunk at light space
    BBox3 splitBox;
    // TMatrix4 frustumTM = ComputedShadowSplit::getUCtoNDC() * cameraInvProj * lightSpace;
    TMatrix4 frustumTM = UniversalUCtoNDC * cameraInvProj * lightSpace;
    for (int i = 0; i < 8; ++i)
    {
      Point4 p = unitBox[i] * frustumTM;
      float invW = 1.0f / p.w;
      Point3 p3(p.x * invW, p.y * invW, p.z * invW);
      splitBox += p3;
    }

    // shift frustum split to origin
    Point3 shiftPt = Point3(splitBox.lim[0].x, splitBox.center().y, splitBox.center().z);

    TMatrix4 shiftTM(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -shiftPt.x, -shiftPt.y, -shiftPt.z, 1.0f);

    TMatrix4 splitWarp;
    if (splitSet[j].isWarp == true && isWarp == true)
    {
      float warpDist = splitSet[j].warpDist * zfar;
      TMatrix4 warpTM = TMatrix4(1.0f, 0.0f, 0.0f, (1.0f / warpDist) * (cosGamma), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
      splitWarp = shiftTM * warpTM;
    }
    else
    {
      splitWarp = shiftTM;
    }

    // to reduce aliasing  align screen pixel / shadow map texel ratio
    // NOTE: questionable usability. TEST IT.
    // sometimes reduce projective aliasing
    // sometimes heavily distort ZBias especially for last(farthest) split
    if (splitSet[j].isAlign == true)
    {
      splitWarp *= invAlignX;
    }

    // transform from camera post projection space
    // into warped shadow map post projection space
    TMatrix4 splitTM = frustumTM * splitWarp;

    // transform frustum split into post-projective light space (shadow map)
    BBox3 splitFocusBox;
    for (int i = 0; i < 8; ++i)
    {
      Point4 p = unitBox[i] * splitTM;
      float invW = 1.0f / p.w;
      splitFocusBox += Point3(p.x * invW, p.y * invW, p.z * invW);
    }

    // compute point of shadow casters Z bound
    Point4 zboundPt(splitBox.lim[0].x, splitBox.center().y, splitBox.lim[0].z - shadow_dist, 1.0f);
    Point4 zboundWarpPt = zboundPt * splitWarp;
    float shadowZbound = zboundWarpPt.z / zboundWarpPt.w;
    splitFocusBox.lim[0].z = shadowZbound;

    // make focus TM from focus box for current frustum split
    TMatrix4 splitFocus;
    computeFocusTM(splitFocusBox, splitFocus);

    // compute correct splitZBias
    // splitOut[j].splitZBias = splitFocus._33 * splitSet[j].ZBias * shadow_dist/10;
    splitOut[j].splitZBias = splitFocus._33 * splitSet[j].ZBias;

    // transform this split from world space
    // to post projection light space unit cube
    splitOut[j].proj = view * lightSpace * splitWarp * splitFocus;

    ////-----
    if (FIGHT_ALIASING)
    {
      Point4 origin;
      splitOut[j].getProj().transform(Point3(0, 0, 0), origin);
      origin.unify();

      float halfW = 0.5f * (float)splitSet[j].width;
      float halfH = 0.5f * (float)splitSet[j].height;

      float texCoordX = origin.x * halfW;
      float texCoordY = origin.y * halfH;

      float dx = (floor(texCoordX) - texCoordX) / halfW;
      float dy = (floor(texCoordY) - texCoordY) / halfH;
      TMatrix4 tRound;
      tRound.identity();
      tRound._41 = dx;
      tRound._42 = dy;

      splitOut[j].proj = splitOut[j].getProj() * tRound * splitOut[j].getNDCtoUC();
    }
    ////-----

    // compute light space TM for visibility test
    TMatrix4 splitLightSpaceFocus;
    splitBox.lim[0].z = splitBox.lim[0].z - shadow_dist;
    computeFocusTM(splitBox, splitLightSpaceFocus);
    splitOut[j].visTM = view * lightSpace * splitLightSpaceFocus;
    splitOut[j].splitBoxWS = inverse(tmatrix(view * lightSpace)) * splitBox; // fixme:: comput more precise
    splitOut[j].splitBoxLocal = splitBox;
  }

  /*for (int i = 0; i < objects_count; ++i)
  {
      processObjects(*objects[i].objects, objects[i].count);
  }*/
}

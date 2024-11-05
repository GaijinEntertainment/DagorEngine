// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/ambientOcclusion/ambientOcclusion.h>
#include <math/dag_mesh.h>
#include <math/dag_SHmath.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point4.h>
#include <util/dag_bitArray.h>
#include <math/random/dag_random.h>
#include <sceneRay/dag_sceneRay.h>

struct PrtSamples
{
  struct Sample
  {
    float prtWeights[SPHHARM_NUM3];
    Point3 pt;
  };
  float ledge;
  SmallTab<Sample, TmpmemAlloc> samples;
};

static void getAxisFromNormal(const Point3 &norm, Point3 &xAxis, Point3 &yAxis)
{
  if (fabsf(norm.x) >= .9f)
    xAxis = Point3(0, 1, 0) % norm;
  else
    xAxis = norm % Point3(1, 0, 0);
  xAxis.normalize();
  yAxis = normalize(xAxis % norm);
}

static void getPrtSample(float prtWeights[SPHHARM_NUM3], const Point3 &n, float w)
{

  prtWeights[SPHHARM_00] += w * 1.0f;
  prtWeights[SPHHARM_1m1] += w * n.y;
  prtWeights[SPHHARM_10] += w * n.z;
  prtWeights[SPHHARM_1p1] += w * n.x;
  prtWeights[SPHHARM_2m2] += w * n.x * n.y;
  prtWeights[SPHHARM_2m1] += w * n.y * n.z;
  prtWeights[SPHHARM_2p1] += w * n.x * n.z;
  prtWeights[SPHHARM_20] += w * (3 * n.z * n.z - 1);
  prtWeights[SPHHARM_2p2] += w * (n.x * n.x - n.y * n.y);
}

static void packPRTWeights(float prtWeights[SPHHARM_NUM3], float w)
{
  // full lighting
  //  prtWeights[SPHHARM_00]  *= (SPHHARM_COEF_0*PI)/w;
  //  prtWeights[SPHHARM_1m1] *= (SPHHARM_COEF_1*PI)/w;
  //  prtWeights[SPHHARM_10]  *= (SPHHARM_COEF_1*PI)/w;
  //  prtWeights[SPHHARM_1p1] *= (SPHHARM_COEF_1*PI)/w;
  //  prtWeights[SPHHARM_2m2] *= (SPHHARM_COEF_21*PI)/w;
  //  prtWeights[SPHHARM_2m1] *= (SPHHARM_COEF_21*PI)/w;
  //  prtWeights[SPHHARM_2p1] *= (SPHHARM_COEF_21*PI)/w;
  //  prtWeights[SPHHARM_20]  *= (SPHHARM_COEF_20*PI)/w;
  //  prtWeights[SPHHARM_2p2] *= (SPHHARM_COEF_22*PI)/w;
  // return;
  // low-bit pack.
  // decoding: SPHHARM_COEF_0*PI, SPHHARM_COEF_1, SPHHARM_COEF_1, SPHHARM_COEF_1
  //           0.5*SPHHARM_COEF_21, 0.5*SPHHARM_COEF_21, 0.5*SPHHARM_COEF_21
  //           SPHHARM_COEF_20*2, SPHHARM_COEF_22
  prtWeights[SPHHARM_00] *= 1.0f / w;

  prtWeights[SPHHARM_1m1] = prtWeights[SPHHARM_1m1] / w;
  prtWeights[SPHHARM_10] = prtWeights[SPHHARM_10] / w;
  prtWeights[SPHHARM_1p1] = prtWeights[SPHHARM_1p1] / w;
  prtWeights[SPHHARM_2m2] = prtWeights[SPHHARM_2m2] * 2 / w;

  prtWeights[SPHHARM_2m1] = prtWeights[SPHHARM_2m1] * 2 / w;

  prtWeights[SPHHARM_20] = prtWeights[SPHHARM_20] * 0.5 / w;

  prtWeights[SPHHARM_2p1] = prtWeights[SPHHARM_2p1] * 2 / w;
  prtWeights[SPHHARM_2p2] = prtWeights[SPHHARM_2p2] / w;
}

static void getPrt(float prtWeights[SPHHARM_NUM3], int maxRays, const Point3 &pt, const Point3 &n, const Point3 &xAxis,
  const Point3 &yAxis, const TMatrix &fromWorld, float offset, float maxDist, float w, StaticSceneRayTracer *theRayTracer)
{
  for (int j = 0; j < maxRays; ++j)
  {
    real cosineTeta = sqrtf(float(j) / (maxRays - 1)); // very bad. 8 rays are going forward!
    real sineTeta = sqrtf(1.0f - cosineTeta * cosineTeta);
    for (int k = 0; k < maxRays; ++k)
    {
      real phi = (TWOPI * k) / maxRays;

      Point3 norm = (cosf(phi) * sineTeta) * xAxis + (sinf(phi) * sineTeta) * yAxis + cosineTeta * n;
      if (!theRayTracer->rayhitNormalized(pt + norm * offset, norm, maxDist))
        getPrtSample(prtWeights, normalize(fromWorld % norm), w);
    }
  }
}

void calculatePRT(MeshData &meshData, const TMatrix &toWorld, StaticSceneRayTracer *theRayTracer, int rays_per_point,
  float points_per_sq_meter, int maxPointsPerFace, float maxDist, int prt1, int prt2, int prt3)
{
  MeshData::ExtraChannel &prt1e = meshData.extra[prt1];
  MeshData::ExtraChannel &prt2e = meshData.extra[prt2];
  MeshData::ExtraChannel &prt3e = meshData.extra[prt3];
  SmallTab<Point3, TmpmemAlloc> verts;
  SmallTab<float, TmpmemAlloc> faceArea;
  clear_and_resize(verts, meshData.vert.size());
  for (int i = 0; i < verts.size(); ++i)
    verts[i] = toWorld * meshData.vert[i];
  clear_and_resize(faceArea, meshData.face.size());
  for (int i = 0; i < faceArea.size(); ++i)
    faceArea[i] = 0.5 * length((verts[meshData.face[i].v[1]] - verts[meshData.face[i].v[0]]) %
                               (verts[meshData.face[i].v[2]] - verts[meshData.face[i].v[0]]));
  Tab<Tabint> map(tmpmem);
  meshData.get_vert2facemap(map, 0, meshData.facengr.size());

  Tab<PrtSamples> prtmap(tmpmem);
  prtmap.resize(meshData.facengr.size());

  const int maxRays = rays_per_point;
  TMatrix fromWorld = inverse(toWorld);
  Matrix3 toWorldNorm = transpose(Matrix3((float *)fromWorld.m));

  for (int i = 0; i < meshData.facengr.size(); ++i)
  {
    float offset = 0.03f;
    int seed1 = 115791111, seed2 = 12413137;
    int fid = i;
    Point3 edge1(verts[meshData.face[fid].v[1]] - verts[meshData.face[fid].v[0]]);
    Point3 edge2(verts[meshData.face[fid].v[2]] - verts[meshData.face[fid].v[0]]);
    Point3 nedge1 = meshData.vertnorm[meshData.facengr[fid][1]] - meshData.vertnorm[meshData.facengr[fid][0]];
    Point3 nedge2 = meshData.vertnorm[meshData.facengr[fid][2]] - meshData.vertnorm[meshData.facengr[fid][0]];
    float ledge1 = edge1.length();
    float ledge2 = edge2.length();
    prtmap[i].ledge = ledge1 > ledge2 ? ledge1 : ledge2;
    float area = faceArea[fid];
    int maxPts = ceilf(area * points_per_sq_meter);
    maxPts = min(maxPointsPerFace, maxPts);
    clear_and_resize(prtmap[i].samples, maxPts);
    for (int pti = 0; pti < maxPts; ++pti)
    {
      float a, b;
      do
      {
        a = _frnd(seed1), b = _frnd(seed2);
        if (a + b > 1)
        {
          b = 1 - b;
          a = 1 - a;
        }
      } while (a + b < 0.01 || 1 - a - b < 0.01); // not in vertex
      Point3 pt;
      pt = edge1 * a + edge2 * b + verts[meshData.face[fid].v[0]];
      prtmap[i].samples[pti].pt = pt;

      Point3 cnorm = normalize(meshData.vertnorm[meshData.facengr[fid][0]] + nedge1 * a + nedge2 * b);
      Point3 n = toWorld * cnorm;
      Point3 norm, xAxis, yAxis;
      getAxisFromNormal(n, xAxis, yAxis);
      memset(prtmap[i].samples[pti].prtWeights, 0, sizeof(float) * SPHHARM_NUM3);
      getPrt(prtmap[i].samples[pti].prtWeights, maxRays, pt, n, xAxis, yAxis, fromWorld, offset, maxDist, 1, theRayTracer);
    }
  }

  Bitarray cvertDone;
  cvertDone.resize(meshData.vertnorm.size());
  cvertDone.reset();
  //__int64 refTime = ref_time_ticks();
  for (int i = 0; i < meshData.facengr.size(); ++i)
  {
    for (int vi = 0; vi < 3; ++vi)
    {
      int cvi = meshData.facengr[i][vi];
      if (cvertDone[cvi])
        continue;
      Point3 p = verts[meshData.face[i].v[vi]];
      // Point3 n = normalize(meshData.vertnorm[meshData.facengr[i][vi]]);
      Point3 n = toWorldNorm * meshData.vertnorm[meshData.facengr[i][vi]];
      cvertDone.set(cvi);
      float prtWeights[SPHHARM_NUM3];
      memset(prtWeights, 0, sizeof(float) * SPHHARM_NUM3);
      Point3 norm, xAxis, yAxis;
      getAxisFromNormal(n, xAxis, yAxis);
      float offset = 0.025f;
      float maxDist = theRayTracer->getSphere().r * 2;
      float totalWeight = 0;
      float startW = 1;
      float startRaysMul = 2;
      totalWeight = startW;
      getPrt(prtWeights, maxRays * startRaysMul, p, n, xAxis, yAxis, fromWorld, offset, maxDist,
        startW / (startRaysMul * startRaysMul), theRayTracer);
      int seed1 = 115791111, seed2 = 12413137;

      for (int fi = 0; fi < map[meshData.face[i].v[vi]].size(); ++fi)
      {
        int fid = map[meshData.face[i].v[vi]][fi];
        if (!(meshData.face[i].smgr & meshData.face[fid].smgr))
          continue;
        float ledge = prtmap[fid].ledge;
        for (int pti = 0; pti < prtmap[fid].samples.size(); ++pti)
        {
          Point3 pt = prtmap[fid].samples[pti].pt;
          float w = (1 - length(pt - p) / ledge);
          if (w < 0)
            continue;
          for (int pi = 0; pi < SPHHARM_NUM3; ++pi)
            prtWeights[pi] += w * prtmap[fid].samples[pti].prtWeights[pi];
          totalWeight += w;
        }
      }

      packPRTWeights(prtWeights, totalWeight * (maxRays * maxRays));
      ((Point4 *)&prt1e.vt[0])[cvi] = Point4(prtWeights[0], prtWeights[1], prtWeights[2], prtWeights[3]);
      ((Point4 *)&prt2e.vt[0])[cvi] = Point4((prtWeights[4]), (prtWeights[5]), (prtWeights[6]), (prtWeights[7]));
      ((float *)&prt3e.vt[0])[cvi] = prtWeights[8];
    }
  }
  for (int i = 0; i < meshData.facengr.size(); ++i)
  {
    memcpy(&prt1e.fc[i].t[0], &meshData.facengr[i][0], sizeof(TFace));
    memcpy(&prt2e.fc[i].t[0], &meshData.facengr[i][0], sizeof(TFace));
    memcpy(&prt3e.fc[i].t[0], &meshData.facengr[i][0], sizeof(TFace));
  }
}
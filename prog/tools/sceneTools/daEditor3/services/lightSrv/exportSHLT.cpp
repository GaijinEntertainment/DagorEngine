#include "ltMgr.h"
#include <de3_lightService.h>
#include <de3_lightProps.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/staticGeom/matFlags.h>
#include <sceneBuilder/nsb_StdTonemapper.h>
#include <generic/dag_sort.h>
#include <libTools/util/makeBindump.h>
#include <math/dag_SHlight.h>
#include <math/dag_mathAng.h>
#include <math/dag_boundingSphere.h>
#include <libTools/math/viewMatrix.h>
#include <libTools/util/sHmathUtil.h>
#include <util/dag_bitArray.h>
#include <ioSys/dag_genIo.h>
#include <debug/dag_debug.h>

namespace dagoredexpshlt
{
struct DirLight
{
  Color3 lt;
  float maxDist;
  Point3 dir;
  float coneCosAlpha;
};

struct OmniLight
{
  Point3 pt;
  float maxDist2;
  int cubeIdx, cubeCnt;
  float r2, r;
};

struct OmniLightCube
{
  Point3 norm;
  int _resv;
  Color3 lt;
  float maxDist2;
};
} // namespace dagoredexpshlt

using namespace dagoredexpshlt;

#define MIN_INTENS 0.16

static void eraseMesh(Mesh &mesh)
{
  mesh.setnumfaces(0);
  mesh.vert.clear();
  mesh.tvert[0].clear();
  mesh.tvert[1].clear();
  mesh.cvert.clear();
  mesh.vertnorm.clear();
}

static void addVerticesToMesh(MeshData &mesh, const Mesh &object)
{
  append_items(mesh.vert, object.vert.size(), object.vert.data());
  append_items(mesh.vertnorm, object.vertnorm.size(), object.vertnorm.data());
  append_items(mesh.tvert[0], object.tvert[0].size(), object.tvert[0].data());
  append_items(mesh.tvert[1], object.tvert[1].size(), object.tvert[1].data());
}

static BSphere3 calculateBoundingSphere(const Bitarray &used, const Point3 *verts, BBox3 &bbox, real expand_rad)
{
  Tab<Point3> cache_verts(tmpmem);
  cache_verts.reserve(used.size());
  bbox.setempty();

  for (int v = 0; v < used.size(); ++v)
  {
    if (used.get(v))
    {
      cache_verts.push_back(verts[v]);
      bbox += verts[v];
    }
  }

  if (!bbox.isempty())
  {
    bbox[0] -= Point3(expand_rad, expand_rad, expand_rad);
    bbox[1] += Point3(expand_rad, expand_rad, expand_rad);
  }

  BSphere3 bsph = ::mesh_bounding_sphere(&cache_verts[0], cache_verts.size());
  bsph.r += expand_rad;
  bsph.r2 = bsph.r * bsph.r;

  return bsph;
}

static float squared_dist_to_light(float area, float emissionArea, float cosPower, float sphereR2, float min_intens)
{
  if (emissionArea < 1e-6)
    return 0;
  if (cosPower < 1)
    cosPower = 1;
  float sphereSquareDiv = area / (sphereR2 * PI);
  float emissionPerArea = (emissionArea / area);
  float emissionAtStart = (emissionPerArea * TWOPI / (cosPower + 1));
  if (emissionAtStart < min_intens)
    min_intens = emissionAtStart / 20;
  float emissionLost = 1.0 - min_intens / emissionAtStart;
  float visR2 = (sphereR2 * sphereSquareDiv / (1 - pow(emissionLost, 2.0f / (cosPower + 1))));
  return visR2;
}

static void getMeshFaceDistance(const PtrTab<StaticGeometryMaterial> &mats, const Mesh &useMesh, const TMatrix &directionTm,
  float distance2[6], float r2, float min_intens, Color3 norm_intense[6])
{
  float minCosPower[6];
  float currentFaceArea[6];
  float currentAmbEmission[6];
  float currentDirEmission[6];
  memset(minCosPower, 0, sizeof(minCosPower));
  memset(currentFaceArea, 0, sizeof(currentFaceArea));
  memset(currentAmbEmission, 0, sizeof(currentAmbEmission));
  memset(currentDirEmission, 0, sizeof(currentDirEmission));
  memset(norm_intense, 0, sizeof(Color3) * 6);
  for (int fi = 0; fi < useMesh.getFaceCount(); ++fi)
  {
    const ::Face &face1 = useMesh.getFace()[fi];
    Point3 faceNorm1 = (useMesh.vert[face1.v[1]] - useMesh.vert[face1.v[0]]) % (useMesh.vert[face1.v[2]] - useMesh.vert[face1.v[0]]);
    int matNo = useMesh.getFaceSMGR(fi); //== we stored matNo here earlier in extractLightMesh()
    for (int di = 0; di < 6; ++di)
    {
      Point3 direction = directionTm.getcol(di % 3);
      if (di >= 3)
        direction = -direction;
      float projectedArea = faceNorm1 * direction;
      if (projectedArea <= 0)
      {
        if (mats[matNo] && mats[matNo]->flags & MatFlags::FLG_2SIDED)
          projectedArea = -projectedArea;
        else
          continue;
      }

      currentFaceArea[di] += projectedArea;

      float emission = brightness(mats[matNo]->emis);
      float orig_emission = emission;
      float cosPower = mats[matNo]->cosPower;
      if (cosPower > 1.0f)
      {
        if (minCosPower[di] < cosPower || minCosPower[di] == 0)
          minCosPower[di] = cosPower;
        currentAmbEmission[di] += projectedArea * brightness(mats[matNo]->amb);
        norm_intense[di] += projectedArea * color3(mats[matNo]->amb);
        float cosWeight = 0;
        for (int vi = 0; vi < 3; ++vi)
        {
          float current = (useMesh.vertnorm[useMesh.facengr[fi][vi]] * direction);
          if (current > 0)
            cosWeight += powf(current, cosPower + 1);
        }
        if (cosWeight < 0)
          cosWeight = 0;
        emission *= cosWeight / 3;
      }
      currentDirEmission[di] += projectedArea * emission;
      norm_intense[di] += (projectedArea * emission / orig_emission) * color3(mats[matNo]->emis);
    }
  }
  for (int di = 0; di < 6; ++di)
  {
    float visR2 = squared_dist_to_light(currentFaceArea[di], currentDirEmission[di], minCosPower[di], r2, min_intens);

    float ambVisR2 = visR2;
    if (currentAmbEmission[di] > 0)
      ambVisR2 = squared_dist_to_light(currentFaceArea[di], currentAmbEmission[di], 0, r2, min_intens);
    if (visR2 < ambVisR2)
      visR2 = ambVisR2;
    if (visR2 < 0.1)
      visR2 = 0.1;
    distance2[di] = visR2;
    norm_intense[di] *= 6.0 / (4.0 * PI * r2);
  }
}

static void extractLightMesh(MeshData &lightMesh, Bitarray &useVertices, const StaticGeometryMesh &gm)
{
  for (int f = gm.mesh.face.size() - 1; f >= 0; --f)
  {
    const Face &face = gm.mesh.face[f];
    int matNo = face.mat % gm.mats.size();

    if (!gm.mats[matNo] || lengthSq(gm.mats[matNo]->emis) <= REAL_EPS)
      continue;

    if (!lightMesh.face.size())
    {
      addVerticesToMesh(lightMesh, gm.mesh);

      useVertices.resize(lightMesh.vert.size());
      useVertices.reset();
    }

    ::Face mf;
    mf.v[0] = face.v[0];
    mf.v[1] = face.v[1];
    mf.v[2] = face.v[2];
    useVertices.set(mf.v[0]);
    useVertices.set(mf.v[1]);
    useVertices.set(mf.v[2]);
    mf.mat = matNo;
    mf.smgr = matNo;
    lightMesh.face.push_back(mf);

    if (gm.mesh.facengr.size())
    {
      FaceNGr fngr;
      fngr[0] = gm.mesh.facengr[f][0];
      fngr[1] = gm.mesh.facengr[f][1];
      fngr[2] = gm.mesh.facengr[f][2];
      G_VERIFY(fngr[0] < lightMesh.vertnorm.size());
      G_VERIFY(fngr[1] < lightMesh.vertnorm.size());
      G_VERIFY(fngr[2] < lightMesh.vertnorm.size());
      lightMesh.facengr.push_back(fngr);
    }
    else
    {
      G_ASSERT(false);
    }

    if (gm.mesh.tface[0].size())
      lightMesh.tface[0].push_back(gm.mesh.tface[0][f]);
  }
}

static void buildOmniCube(Mesh &lightMesh, Bitarray &usedVertices, const StaticGeometryMesh &gm, Point3 cube_norm[6],
  float cube_max_dist2[6], int &out_maxdisti, Color3 out_intens[6], BSphere3 &bsphere, real hdr_scale)
{
  BBox3 bbox;
  bsphere = ::calculateBoundingSphere(usedVertices, lightMesh.vert.data(), bbox, 0);

  float emissionFaceArea = 0, ambFaceArea = 0, minCosPower = MAX_REAL;
  float emissionMax = 0, ambEmissionMax = 0;
  Tab<Point3> normals(tmpmem_ptr());
  Bitarray normalsUsed;

  normals.reserve(lightMesh.vertnorm.size());
  normalsUsed.resize(lightMesh.vertnorm.size());
  normalsUsed.reset();
  for (int fi = 0; fi < lightMesh.face.size(); ++fi)
  {
    for (int vi = 0; vi < 3; ++vi)
    {
      if (normalsUsed[lightMesh.facengr[fi][vi]])
        continue;
      Point3 n = lightMesh.vertnorm[lightMesh.facengr[fi][vi]];
      int ni;
      for (ni = 0; ni < normals.size(); ++ni)
        if (n * normals[ni] > 0.9999f)
          break;
      if (ni >= normals.size())
        normals.push_back(n);
      normalsUsed.set(lightMesh.facengr[fi][vi]);
    }
  }

  TMatrix resultTm;
  memset(cube_max_dist2, 0, sizeof(float) * 6);
  out_maxdisti = 0;
  resultTm.identity();

  for (int ni = 0; ni < normals.size(); ++ni)
  {
    float distance2[6];
    Color3 intens[6];
    memset(distance2, 0, sizeof(distance2));
    TMatrix tm;
    view_matrix_from_look(normals[ni], tm);

    getMeshFaceDistance(gm.mats, lightMesh, tm, distance2, bsphere.r2, MIN_INTENS / (hdr_scale > 0 ? hdr_scale : 1), intens);
    int cmaxDisti = 0;
    float maxDist = 0;
    for (int di = 0; di < 6; di++)
      if (distance2[di] > maxDist)
      {
        maxDist = distance2[di];
        cmaxDisti = di;
      }
    if (maxDist > cube_max_dist2[out_maxdisti])
    {
      memcpy(cube_max_dist2, distance2, sizeof(distance2));
      memcpy(out_intens, intens, sizeof(intens));
      out_maxdisti = cmaxDisti;
      resultTm = tm;
    }
  }
  for (int fi = 0; fi < lightMesh.face.size(); ++fi)
  {
    lightMesh.face[fi].smgr = 1;
  }

  debug("  sphere=(%g %g %g) rad=%g visRadius[%d]=%g dist=" FMT_P3 "", P3D(bsphere.c), bsphere.r, out_maxdisti,
    sqrtf(cube_max_dist2[out_maxdisti]), P3D((out_maxdisti > 2 ? -1.0 : 1.0) * resultTm.getcol(out_maxdisti % 3)));

  debug("  visRadius = %g, %g, %g  %g, %g, %g", sqrtf(cube_max_dist2[0]), sqrtf(cube_max_dist2[1]), sqrtf(cube_max_dist2[2]),
    sqrtf(cube_max_dist2[3]), sqrtf(cube_max_dist2[4]), sqrtf(cube_max_dist2[5]));

  for (int i = 0; i < 3; i++)
  {
    cube_norm[i] = resultTm.getcol(i);
    cube_norm[i + 3] = -resultTm.getcol(i);
  }
}

inline Color3 cvt_color(E3DCOLOR col, float brightness = 1.f)
{
  return Color3(col.r * brightness / 255.0, col.g * brightness / 255.0, col.b * brightness / 255.0);
}


static int sort_suns(const DirLight *a, const DirLight *b)
{
  real pa = brightness(a->lt);
  real pb = brightness(b->lt);
  if (pa > pb)
    return -1;
  return pa < pb ? 1 : 0;
}

void LtMgrForLightService::exportShlt(mkbindump::BinDumpSaveCB &cwr, StaticGeometryContainer &geom)
{
  Tab<DirLight> dirlt(tmpmem);
  Tab<OmniLight> omnilt(tmpmem);
  Tab<OmniLightCube> omniltcube(tmpmem);
  SH3Lighting skylt;

  srv.toneMapper().recalc();

  Color3 scale = Color3::xyz(srv.toneMapper().getScale());
  Color3 hdrScale = Color3::xyz(srv.toneMapper().getHdrScale());
  Color3 postScale = Color3::xyz(srv.toneMapper().getPostScale());
  Color3 invGamma = Color3::xyz(srv.toneMapper().getInvGamma());
  Point3 sunDir(0, 0, 0);
  Color3 sunCol(0, 0, 0), ambColScale(1, 1, 1);
  real sunConeCos = 1.0;

  debug_ctx("hdrScale=" FMT_P3 " ldrScale=" FMT_P3 " postScale=" FMT_P3 "", hdrScale.r, hdrScale.g, hdrScale.b, scale.r, scale.g,
    scale.b, postScale.r, postScale.g, postScale.b);

  scale = scale * hdrScale; // * (1.0/16.0);

  // make sky light
  srv.calcSHLighting(skylt, true, false, false);

  {
    Color4 col4 = srv.toneMapper().mapColor(::color4(srv.getSky().color));
    col4 = Color4(col4.r / postScale.r, col4.g / postScale.g, col4.b / postScale.b);

    Color3 sky = color3(col4 * srv.getSky().brightness / 4);
    Color3 dirSky = srv.getSky().ambCol;

    debug("real sky: scale=" FMT_P3 ", dirSky=" FMT_P3 "", sky.r, sky.g, sky.b, dirSky.r, dirSky.g, dirSky.b);
    sky.r = fabs(sky.r) > 1e-3 ? dirSky.r / sky.r : 1.0;
    sky.g = fabs(sky.g) > 1e-3 ? dirSky.g / sky.g : 1.0;
    sky.b = fabs(sky.b) > 1e-3 ? dirSky.b / sky.b : 1.0;
    debug("scale=" FMT_P3 "", sky.r, sky.g, sky.b);
    for (int i = 0; i < 9; i++)
      skylt.sh[i] *= sky;
  }

  Color3 ish[SPHHARM_NUM3];
  memcpy(ish, skylt.sh, sizeof(ish));

  ish[SPHHARM_00] *= PI;

  ish[SPHHARM_1m1] *= 2.094395f;
  ish[SPHHARM_10] *= 2.094395f;
  ish[SPHHARM_1p1] *= 2.094395f;

  ish[SPHHARM_2m2] *= 0.785398f;
  ish[SPHHARM_2m1] *= 0.785398f;
  ish[SPHHARM_20] *= 0.785398f;
  ish[SPHHARM_2p1] *= 0.785398f;
  ish[SPHHARM_2p2] *= 0.785398f;

  class ScaleTransformCB : public SH3Color3TransformCB
  {
  public:
    const Color3 scale;
    ScaleTransformCB(const Color3 &_c) : scale(_c) {}
    Color3 transform(const Color3 &c) { return c * scale; }
  } cb(scale);

  transform_sphharm(ish, skylt.sh, cb);

  skylt.sh[SPHHARM_00] /= PI;

  skylt.sh[SPHHARM_1m1] /= 2.094395f;
  skylt.sh[SPHHARM_10] /= 2.094395f;
  skylt.sh[SPHHARM_1p1] /= 2.094395f;

  skylt.sh[SPHHARM_2m2] /= 0.785398f;
  skylt.sh[SPHHARM_2m1] /= 0.785398f;
  skylt.sh[SPHHARM_20] /= 0.785398f;
  skylt.sh[SPHHARM_2p1] /= 0.785398f;
  skylt.sh[SPHHARM_2p2] /= 0.785398f;

  // gather directional sources
  for (int i = srv.getSunCount() - 1; i >= 0; i--)
  {
    SunLightProps &s = *srv.getSun(i);

    DirLight &dl = dirlt.push_back();
    dl.lt = ::color3(s.color) * s.brightness;
    debug_ctx_("sun[%d]: lt=" FMT_P3 " ", i, dl.lt.r, dl.lt.g, dl.lt.b);
    dl.lt = dl.lt * scale;
    dl.dir = sph_ang_to_dir(Point2(s.azimuth, s.zenith));
    debug("-> lt=" FMT_P3 " dir=" FMT_P3 "", dl.lt.r, dl.lt.g, dl.lt.b, P3D(dl.dir));
    dl.maxDist = 200;
    dl.coneCosAlpha = cos(s.angSize / 2 * 0.9); /// sun is not circle
  }
  sort(dirlt, &sort_suns);

  SunLightProps *sun = srv.getSun(0);
  sunConeCos = cos(2.5 / 180.0 * PI * 0.9);
  sunCol = sun ? 2 * sun->ltCol / postScale : Color3(0, 0, 0);
  sunCol.r = powf(sunCol.r, 1.0 / invGamma.r) * postScale.r;
  sunCol.g = powf(sunCol.g, 1.0 / invGamma.g) * postScale.g;
  sunCol.b = powf(sunCol.b, 1.0 / invGamma.b) * postScale.b;
  sunCol *= 1.0 / (1.0 - sunConeCos);
  sunDir = sun ? sun->ltDir : Point3(0, 0, 0);
  debug("primary sun: lt=" FMT_P3 ", dir=" FMT_P3 " coneCos=%.7f", sunCol.r, sunCol.g, sunCol.b, P3D(sunDir), sunConeCos);

  // gather omniscent sources
  Mesh lightMesh;
  Bitarray usedLightVertices;
  for (int i = 0; i < geom.getNodes().size(); i++)
  {
    StaticGeometryMesh &gm = *geom.getNodes()[i]->mesh;
    bool has_light = false;
    for (int j = 0; j < gm.mats.size(); j++)
      if (gm.mats[j] && lengthSq(gm.mats[j]->emis) > REAL_EPS)
      {
        has_light = true;
        break;
      }

    if (!has_light)
      continue;

    eraseMesh(lightMesh);

    Mesh mesh;

    mesh = gm.mesh;

    for (int j = 0; j < gm.mesh.vert.size(); ++j)
      gm.mesh.vert[j] = geom.getNodes()[i]->wtm * gm.mesh.vert[j];

    gm.mesh.calc_ngr();
    gm.mesh.calc_vertnorms();

    extractLightMesh(lightMesh.getMeshData(), usedLightVertices, gm);
    debug("\nobject <%s>: ligthmesh: %d verts, %d faces", (char *)geom.getNodes()[i]->name, lightMesh.vert.size(),
      lightMesh.face.size());

    if (lightMesh.face.size())
    {
      Point3 cube_norm[6];
      float cube_max_dist2[6];
      Color3 intens[6];
      int max_disti;
      BSphere3 bsph;

      buildOmniCube(lightMesh, usedLightVertices, gm, cube_norm, cube_max_dist2, max_disti, intens, bsph, brightness(hdrScale));

      OmniLight &ol = omnilt.push_back();

      ol.pt = bsph.c;
      ol.maxDist2 = cube_max_dist2[max_disti];
      ol.r2 = bsph.r2;
      ol.cubeCnt = 6;
      ol.cubeIdx = append_items(omniltcube, ol.cubeCnt);
      ol.r = bsph.r;

      for (int i = 0; i < ol.cubeCnt; i++)
      {
        OmniLightCube &olc = omniltcube[ol.cubeIdx + i];
        olc.norm = cube_norm[i];
        olc.lt = intens[i] * scale;
        olc.maxDist2 = cube_max_dist2[i];
        olc._resv = 0;
        debug("  dir=" FMT_P3 "  lt=" FMT_P3 "", P3D(olc.norm), olc.lt.r, olc.lt.g, olc.lt.b);
      }
    }
    gm.mesh = mesh;
  }
  //== to do

  // write dump
  mkbindump::PatchTabRef dirlt_pt, omnilt_pt, omniltcube_pt;
  mkbindump::PatchTabRef stk_pt;

  cwr.beginTaggedBlock(_MAKE4C('SH3L'));
  cwr.writeFourCC(_MAKE4C('v4'));
  cwr.beginBlock();
  cwr.setOrigin();

  cwr.write32ex(&skylt, sizeof(skylt));
  cwr.align8();

  cwr.write32ex(&invGamma, sizeof(Color3));
  cwr.write32ex(&postScale, sizeof(Color3));

  dirlt_pt.reserveTab(cwr);
  omnilt_pt.reserveTab(cwr);
  omniltcube_pt.reserveTab(cwr);
  stk_pt.reserveTab(cwr);

  cwr.write32ex(&sunDir, sizeof(sunDir));
  cwr.write32ex(&sunCol, sizeof(sunCol));
  cwr.write32ex(&ambColScale, sizeof(ambColScale));
  cwr.writeReal(brightness(sunCol) * (1 - sunConeCos));
  cwr.writeReal(sunConeCos);

  cwr.align8();
  stk_pt.startData(cwr, dirlt.size() + omnilt.size());
  cwr.writeZeroes((dirlt.size() + omnilt.size()) * 32);
  stk_pt.finishTab(cwr);

  dirlt_pt.startData(cwr, dirlt.size());
  cwr.writeTabData32ex(dirlt);
  omnilt_pt.startData(cwr, omnilt.size());
  cwr.writeTabData32ex(omnilt);
  omniltcube_pt.startData(cwr, omniltcube.size());
  cwr.writeTabData32ex(omniltcube);

  dirlt_pt.finishTab(cwr);
  omnilt_pt.finishTab(cwr);
  omniltcube_pt.finishTab(cwr);

  cwr.popOrigin();
  cwr.endBlock();
  cwr.align8();
  cwr.endBlock();
}

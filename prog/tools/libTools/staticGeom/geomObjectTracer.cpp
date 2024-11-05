// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_traceRayTriangle.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/matFlags.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <sceneRay/dag_sceneRay.h>
#include <math/dag_rayIntersectSphere.h>
#include <libTools/dtx/dtxSampler.h>
#include <libTools/util/de_TextureName.h>
#include <util/dag_texMetaData.h>
#include <osApiWrappers/dag_files.h>
// #include <debug/dag_debug.h>

class GeomObject::SceneRayTracer
{
public:
  DAG_DECLARE_NEW(midmem)

  StaticSceneRayTracer *tracer;
  bool hasCollision;
  bool hasShadow;
  SmallTab<unsigned, MidmemAlloc> indicesEnd;

  SmallTab<DTXTextureSampler *, MidmemAlloc> textureSamplers;
  SmallTab<unsigned short, MidmemAlloc> faceTextureSamplers;
  // SmallTab<int, MidmemAlloc> materialSamplers;
  SceneRayTracer() : tracer(NULL), hasCollision(false), hasShadow(false) {}
  void clear()
  {
    del_it(tracer);
    for (int i = 0; i < textureSamplers.size(); ++i)
      del_it(textureSamplers[i]);
    clear_and_shrink(textureSamplers);
    clear_and_shrink(indicesEnd);
  }
  ~SceneRayTracer() { clear(); }
};

StaticSceneRayTracer *GeomObject::getRayTracer()
{
  if (!rayTracer)
    initRayTracer();
  return rayTracer->tracer;
}

void GeomObject::delRayTracer() { del_it(rayTracer); }


bool GeomObject::reloadRayTracer()
{
  delRayTracer();
  return (bool)getRayTracer();
}


void GeomObject::initRayTracer()
{
  delRayTracer();
  if (!geom)
    return;

  rayTracer = new GeomObject::SceneRayTracer;
  clear_and_resize(rayTracer->indicesEnd, geom->nodes.size());

  int totalFCount = 0, totalVCount = 0;
  for (int j = 0; j < geom->nodes.size(); ++j)
  {
    G_ASSERT(geom->nodes[j] && geom->nodes[j]->mesh);

    //(geom->nodes[j]->checkBillboard()) ||
    if (!geom->nodes[j]->isNonTopLod())
    {
      totalVCount += geom->nodes[j]->mesh->mesh.getVert().size();
      totalFCount += geom->nodes[j]->mesh->mesh.getFace().size();
    }

    rayTracer->indicesEnd[j] = totalFCount;
  }

  // target heuristic: 20 faces per XZ cell
  BBox3 bbox = getBoundBox(true);
  if (bbox.isempty() || !totalFCount)
    return;

  Point3 cellSize = bbox.width() / 8;
  float xz_scale = sqrtf(64.0f * 20.0f / float(totalFCount + 1));
  if (xz_scale > 8)
    xz_scale = 8;
  cellSize.x *= xz_scale;
  cellSize.z *= xz_scale;
  cellSize.y *= 2;

  if (cellSize.x < 1)
    cellSize.x = 1;
  if (cellSize.y < 1)
    cellSize.y = 1;
  if (cellSize.z < 1)
    cellSize.z = 1;

  BuildableStaticSceneRayTracer *tracer = ::create_buildable_staticmeshscene_raytracer(cellSize, 7);

  tracer->reserve(totalFCount, totalVCount);
  Tab<Point3> vert(tmpmem);
  Tab<unsigned> flags(tmpmem);

  Tab<StaticGeometryMaterial *> materials(tmpmem);
  Tab<StaticGeometryTexture *> textures(tmpmem);
  Tab<unsigned short> textureSampled(tmpmem);
  Tab<unsigned short> materialSamplers(tmpmem);
  int totalSamplers = 0;

  for (int j = 0; j < geom->nodes.size(); ++j)
  {
    for (int nodeMat = 0; nodeMat < geom->nodes[j]->mesh->mats.size(); ++nodeMat)
    {
      StaticGeometryMaterial *material = geom->nodes[j]->mesh->mats[nodeMat];

      for (int matId = 0; matId < materials.size(); ++matId)
        if (materials[matId] == material)
        {
          material = NULL;
          break;
        }

      if (!material)
        continue;

      materials.push_back(material);
      unsigned short sampler = 0xFFFF;
      for (int textureIndex = 0; textureIndex < 1; ++textureIndex)
      {
        StaticGeometryTexture *texture = material->textures[textureIndex];
        if (!texture)
          continue;

        for (int texId = 0; texId < textures.size(); ++texId)

          if (textures[texId] == texture)
          {
            texture = NULL;
            sampler = textureSampled[texId];
            break;
          }

        if (!texture)
          continue;

        textures.push_back(texture);
        textureSampled.push_back(sampler = (material->atest > 0 ? totalSamplers : 0xFFFF));
        if (material->atest > 0)
          totalSamplers++;
      }
      materialSamplers.push_back(sampler);
    }
  }

  G_VERIFY(totalSamplers < 0xFFFF);

  clear_and_resize(rayTracer->textureSamplers, totalSamplers);
  for (int i = 0; i < textures.size(); ++i)
    if (textureSampled[i] != 0xFFFF)
    {
      int sampler = textureSampled[i];
      rayTracer->textureSamplers[sampler] = NULL;
      file_ptr_t file = ::df_open(TextureMetaData::decodeFileName(textures[i]->fileName), DF_READ);
      if (file)
      {
        int len = ::df_length(file);
        unsigned char *data = new unsigned char[len];
        df_read(file, data, len);
        ::df_close(file);
        rayTracer->textureSamplers[sampler] = new DTXTextureSampler;
        if (!rayTracer->textureSamplers[sampler]->construct(data, len))
        {
          del_it(rayTracer->textureSamplers[sampler]);
          delete[] data;
        }
        else
          rayTracer->textureSamplers[sampler]->ownData_ = true;
      }
    }

  // rayTracer->materialSamplers.resize(materialSamplers.size());
  // rayTracer->materialSamplers = materialSamplers;
  clear_and_resize(rayTracer->faceTextureSamplers, totalFCount);
  bool hasCollision = false, hasShadow = false;

  int faceNodeOffset = 0;
  for (int j = 0; j < geom->nodes.size(); ++j)
  {
    //(geom->nodes[j]->checkBillboard()) ||
    if (geom->nodes[j]->isNonTopLod())
      continue;
    vert.resize(geom->nodes[j]->mesh->mesh.getVert().size());
    for (int vi = 0; vi < vert.size(); ++vi)
      vert[vi] = geom->nodes[j]->wtm * geom->nodes[j]->mesh->mesh.getVert()[vi];
    flags.resize(geom->nodes[j]->mesh->mesh.getFace().size());
    for (int fi = 0; fi < geom->nodes[j]->mesh->mesh.getFace().size(); ++fi)
    {
      flags[fi] = StaticSceneRayTracer::CULL_CCW;
      int mat = geom->nodes[j]->mesh->mesh.getFaceMaterial(fi);
      if (mat >= geom->nodes[j]->mesh->mats.size())
        mat = geom->nodes[j]->mesh->mats.size() - 1;
      StaticGeometryMaterial *material = geom->nodes[j]->mesh->mats[mat];
      if (!material)
        continue;
      for (int matId = 0; matId < materials.size(); ++matId)
        if (materials[matId] == material)
        {
          rayTracer->faceTextureSamplers[fi + faceNodeOffset] = materialSamplers[matId];
        }

      if (geom->nodes[j]->mesh->mats[mat]->flags & MatFlags::FLG_2SIDED)
        flags[fi] |= StaticSceneRayTracer::CULL_BOTH;
      if ((geom->nodes[j]->flags & StaticGeometryNode::FLG_COLLIDABLE))
        flags[fi] |= StaticSceneRayTracer::USER_FLAG3;
      if ((geom->nodes[j]->flags & StaticGeometryNode::FLG_CASTSHADOWS) &&
          (geom->nodes[j]->flags & StaticGeometryNode::FLG_RENDERABLE) &&
          (geom->nodes[j]->mesh->mats[mat]->flags & MatFlags::FLG_USE_IN_GI))
        flags[fi] |= StaticSceneRayTracer::USER_FLAG4;
      if (flags[fi] & StaticSceneRayTracer::USER_FLAG3)
        hasCollision = true;
      if (flags[fi] & StaticSceneRayTracer::USER_FLAG4)
        hasShadow = true;
    }

    tracer->addmesh(&vert[0], vert.size(), (const unsigned int *)&geom->nodes[j]->mesh->mesh.getFace()[0].v[0],
      elem_size(geom->nodes[j]->mesh->mesh.getFace()), geom->nodes[j]->mesh->mesh.getFace().size(), &flags[0], false);
    faceNodeOffset += geom->nodes[j]->mesh->mesh.getFace().size();
  }

  tracer->rebuild();

  rayTracer->tracer = tracer;
  rayTracer->hasShadow = hasShadow;
  rayTracer->hasCollision = hasCollision;
}

bool GeomObject::shadowRayHitTest(const Point3 &p1, const Point3 &dir1, real maxt, int trace_flags)
{
  if (!geom)
    return false;

  StaticSceneRayTracer *tracer = getRayTracer();
  bool traceInvisible = trace_flags & TRACE_INVISIBLE;

  if (!tracer || (!traceInvisible && !rayTracer->hasShadow))
    return false;

  real lg = length(dir1);
  if (!lg)
    return false;

  bool useNodeVis = trace_flags & TRACE_USE_NODEVIS;

  Point3 dir = dir1 / lg;
  maxt *= lg;
  tracer->setSkipFlagMask(StaticSceneRayTracer::USER_INVISIBLE);
  tracer->setUseFlagMask(traceInvisible ? StaticSceneRayTracer::CULL_BOTH : StaticSceneRayTracer::USER_FLAG4);
  tracer->setCullFlags(StaticSceneRayTracer::CULL_BOTH);

  // no atest
  if (!useNodeVis)
  {
    if (!rayTracer->textureSamplers.size())
      if (tracer->rayhitNormalized(p1, dir, maxt))
        return true;
  }

  // atest
  int fi, fromFace = -1;
  real mint = maxt;
  Point3 p = p1;

  while ((fi = tracer->tracerayNormalized(p, dir, mint, fromFace)) >= 0)
  {
    unsigned short sampler = rayTracer->faceTextureSamplers[fi];
    if (!useNodeVis && sampler == 0xFFFF)
      return true;

    int nodei;
    for (nodei = 0; nodei < rayTracer->indicesEnd.size(); ++nodei) // linear search - can be binary
      if (rayTracer->indicesEnd[nodei] > fi)
        break;
    if (nodei == rayTracer->indicesEnd.size())
      return true; // !actually, error


    if (!useNodeVis || isNodeInVisRange(nodei))
    {
      if (
        useNodeVis && sampler == 0xFFFF || !rayTracer->textureSamplers[sampler] || !geom->nodes[nodei]->mesh->mesh.getTFace(0).size())
        return true;
      int nodeFi;
      if (nodei > 0)
        nodeFi = fi - rayTracer->indicesEnd[nodei - 1];
      else
        nodeFi = fi;

      const Mesh &nodeMesh = geom->nodes[nodei]->mesh->mesh;

      Point2 tvert[3] = {nodeMesh.getTVert(0)[nodeMesh.getTFace(0)[nodeFi].t[0]],
        nodeMesh.getTVert(0)[nodeMesh.getTFace(0)[nodeFi].t[1]], nodeMesh.getTVert(0)[nodeMesh.getTFace(0)[nodeFi].t[2]]};

      real u, v;
      {
        const auto &f = tracer->faces(fi);
        const auto &vert0 = tracer->verts(f.v[0]);
        const auto edge1 = tracer->verts(f.v[1]) - vert0;
        const auto edge2 = tracer->verts(f.v[2]) - vert0;
        float ft = maxt;
        if (!traceRayToTriangleNoCull(p, dir, ft, vert0, edge1, edge2, u, v))
          continue; // should not be happening
      }
      Point2 uv = (tvert[1] - tvert[0]) * u + (tvert[2] - tvert[0]) * v + tvert[0];
      E3DCOLOR color = rayTracer->textureSamplers[sampler]->e3dcolorValueAt(uv.x, uv.y);

      int mat = geom->nodes[nodei]->mesh->mesh.getFaceMaterial(nodeFi);
      if (geom->nodes[nodei]->mesh->mats.size() > mat)
        mat = geom->nodes[nodei]->mesh->mats.size() - 1;
      if (geom->nodes[nodei]->mesh->mats[mat] && color.a > geom->nodes[nodei]->mesh->mats[mat]->atest)
        return true;
    }

    fromFace = fi;
    p += dir * (mint + 0.001f);
    maxt -= mint;
    mint = maxt;
  }

  return false;
}

bool GeomObject::hasCollision()
{
  if (!geom)
    return false;
  StaticSceneRayTracer *tracer = getRayTracer();
  if (!tracer || !rayTracer->hasCollision)
    return false;

  return true;
}

bool GeomObject::traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
{
  if (!geom)
    return false;
  StaticSceneRayTracer *tracer = getRayTracer();
  if (!tracer || !rayTracer->hasCollision)
    return false;
  tracer->setSkipFlagMask(StaticSceneRayTracer::USER_INVISIBLE);
  tracer->setUseFlagMask(StaticSceneRayTracer::USER_FLAG3);
  tracer->setCullFlags(0);
  int fi;
  if ((fi = tracer->traceray(p, dir, maxt)) >= 0)
  {
    if (norm)
      *norm = tracer->facebounds(fi).n;
    return true;
  }
  return false;
}

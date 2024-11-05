// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sceneBuilder/nsb_LightmappedScene.h>
#include <libTools/staticGeom/matFlags.h>
#include <libTools/util/prepareBillboardMesh.h>
#include <libTools/util/de_TextureName.h>
#include <libTools/util/iLogWriter.h>
#include <util/dag_texMetaData.h>
#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>
#include "lmsPriv.h"

#define NO_SMOOTHING_IS_ERROR 1

bool StaticSceneBuilder::check_degenerates = true;
bool StaticSceneBuilder::check_no_smoothing = true;

StaticSceneBuilder::LightmappedScene::Object::Object() :
  flags(0),
  mats(tmpmem),
  unwrapScheme(-1),
  lmTc(tmpmem),
  lmFaces(tmpmem),
  lmIds(tmpmem),
  vlmIndex(0),
  vlmSize(0),
  vltmapFaces(tmpmem),
  lmSampleSize(0),
  vssDivCount(0),
  visRange(1000),
  topLodObjId(-1),
  lodRange(100),
  lodNearVisRange(-1),
  lodFarVisRange(-1)
{}


void StaticSceneBuilder::LightmappedScene::Object::initMesh(const TMatrix &wtm, int node_flags, const Point3 &normals_dir)
{
  int num = mats.size();
  if (!num)
    num = 1;

  mesh.clampMaterials(num - 1);

  mesh.sort_faces_by_mat();

  SmallTab<Point3, TmpmemAlloc> orgVerts(mesh.getVert());

  mesh.optimize_tverts();
  if (!(node_flags & StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS) || !mesh.facengr.size())
    mesh.calc_ngr();
  if (!(node_flags & StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS) || !mesh.vertnorm.size())
    mesh.calc_vertnorms();
  mesh.transform(wtm);

  if (node_flags & (StaticGeometryNode::FLG_FORCE_WORLD_NORMALS | StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS))
  {
    // This code was written to prevent useless doubling number of faces for node with forced normals (fake trees, etc.)
    // As a side effect it 'spoils' materials used by this node - removes real2sided flag from ALL materials in scene
    // If this causes some troubles (I can't imagine when), but if you discover that some real2sided materials became not real-2-sided
    // - this was done here. In that case, better not to comment or delete this code, but implement doubling of materials - for such
    // kind of nodes, and for all others.
    //   I am sure, it won't really happen.

    for (int fi = 0; fi < mesh.getFaceCount(); ++fi)
    {
      int mi = mesh.getFaceMaterial(fi);
      if ((mats[mi]->flags & (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED)) == (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED))
      {
        mats[mi]->flags &= ~MatFlags::FLG_REAL_2SIDED;
      }
    }
  }

  ::split_two_sided(mesh, mats);

  // custom normals
  if (node_flags & (StaticGeometryNode::FLG_FORCE_WORLD_NORMALS | StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS))
  {
    Point3 dir;

    if (node_flags & StaticGeometryNode::FLG_FORCE_WORLD_NORMALS)
      dir = normals_dir;
    else
      dir = wtm % normals_dir;

    dir.normalize();

    mesh.facengr.resize(mesh.getFaceCount());
    for (int i = 0; i < mesh.facengr.size(); ++i)
      for (int j = 0; j < 3; ++j)
        mesh.facengr[i][j] = 0;

    mesh.vertnorm.resize(1);
    mesh.vertnorm[0] = dir;
  }
  else if (node_flags & StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS)
  {
    Point3 center = wtm.getcol(3);

    mesh.facengr.resize(mesh.getFaceCount());
    for (int i = 0; i < mesh.facengr.size(); ++i)
    {
      mesh.facengr[i][0] = mesh.face[i].v[0];
      mesh.facengr[i][1] = mesh.face[i].v[1];
      mesh.facengr[i][2] = mesh.face[i].v[2];
    }

    mesh.vertnorm.resize(mesh.getVertCount());
    for (int i = 0; i < mesh.vertnorm.size(); ++i)
      mesh.vertnorm[i] = normalize(mesh.vert[i] - center);
  }

  // get billboard centers from mapping
  if (flags & ObjFlags::FLG_BILLBOARD_MESH)
    ::prepare_billboard_mesh(mesh, wtm, orgVerts);
}


StaticSceneBuilder::LightmappedScene::LightmappedScene() :
  textures(tmpmem),
  giTextures(tmpmem),
  materials(tmpmem),
  objects(tmpmem),
  lightmaps(tmpmem),
  vltmapSize(0),
  totalLmSize(0, 0),
  lmSampleSizeScale(1.0),
  totalFaces(0)
{
  uuid.clear(); // unset
}


void StaticSceneBuilder::LightmappedScene::clear()
{
  clear_and_shrink(textures);
  clear_and_shrink(giTextures);
  clear_and_shrink(materials);
  clear_and_shrink(objects);
  clear_and_shrink(lightmaps);
  vltmapSize = 0;
  totalLmSize = IPoint2(0, 0);
  totalFaces = 0;
  uuid.clear(); // unset
}


int StaticSceneBuilder::LightmappedScene::getTextureId(StaticGeometryTexture *tex)
{
  if (!tex)
    return -1;

  for (int i = 0; i < textures.size(); ++i)
    if (stricmp(textures[i]->fileName, tex->fileName) == 0)
      return i;

  return -1;
}


int StaticSceneBuilder::LightmappedScene::addTexture(StaticGeometryTexture *tex)
{
  if (!tex)
    return -1;

  int id = getTextureId(tex);
  if (id >= 0)
    return id;

  textures.push_back(tex);
  return textures.size() - 1;
}


int StaticSceneBuilder::LightmappedScene::getGiTextureId(StaticGeometryTexture *tex)
{
  if (!tex)
    return -1;

  for (int i = 0; i < giTextures.size(); ++i)
    if (stricmp(giTextures[i]->fileName, tex->fileName) == 0)
      return i;

  return -1;
}


int StaticSceneBuilder::LightmappedScene::addGiTexture(StaticGeometryTexture *tex)
{
  if (!tex)
    return -1;

  int id = getGiTextureId(tex);
  if (id >= 0)
    return id;

  giTextures.push_back(tex);
  return giTextures.size() - 1;
}


int StaticSceneBuilder::LightmappedScene::getMaterialId(StaticGeometryMaterial *mat)
{
  if (!mat)
    return -1;

  for (int i = 0; i < materials.size(); ++i)
    if (materials[i] == mat)
      return i;

  return -1;
}


int StaticSceneBuilder::LightmappedScene::addMaterial(StaticGeometryMaterial *mat)
{
  if (!mat)
    return -1;

  int id = getMaterialId(mat);
  if (id >= 0)
    return id;

  for (int i = 0; i < StaticGeometryMaterial::NUM_TEXTURES; ++i)
    addTexture(mat->textures[i]);

  addGiTexture(mat->textures[0]);

  materials.push_back(mat);
  return materials.size() - 1;
}


void StaticSceneBuilder::LightmappedScene::addObject(Mesh &mesh, PtrTab<StaticGeometryMaterial> &mats, const TMatrix &wtm,
  const char *name, real lm_sample_size, int flags, int unwrap_scheme, const Point3 &normals_dir, int use_vss, int vss_div_count,
  real visrange, int trans_sort_priority, int top_lod_obj_id, real lod_range, real lod_near_range, real lod_far_range)
{
  int oi = append_items(objects, 1);

  objects[oi].flags = 0;

  if (flags & StaticGeometryNode::FLG_CASTSHADOWS)
    objects[oi].flags |= ObjFlags::FLG_CAST_SHADOWS;
  if (flags & StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF)
    objects[oi].flags |= ObjFlags::FLG_CAST_SHADOWS_ON_SELF;
  if (flags & StaticGeometryNode::FLG_FADE)
    objects[oi].flags |= ObjFlags::FLG_OBJ_FADE;
  if (flags & StaticGeometryNode::FLG_FADENULL)
    objects[oi].flags |= ObjFlags::FLG_OBJ_FADENULL;

  if (flags & StaticGeometryNode::FLG_BILLBOARD_MESH)
    objects[oi].flags |= ObjFlags::FLG_BILLBOARD_MESH;
  if (flags & StaticGeometryNode::FLG_OCCLUDER)
    objects[oi].flags |= ObjFlags::FLG_OCCLUDER;
  if (flags & StaticGeometryNode::FLG_BACK_FACE_DYNAMIC_LIGHTING)
    objects[oi].flags |= ObjFlags::FLG_BACK_FACE_DYNAMIC_LIGHTING;
  if (flags & StaticGeometryNode::FLG_DO_NOT_MIX_LODS)
    objects[oi].flags |= ObjFlags::FLG_DO_NOT_MIX_LODS;

  if (use_vss == 1)
    objects[oi].flags |= ObjFlags::FLG_OBJ_USE_VSS;
  else if (use_vss == 0)
    objects[oi].flags |= ObjFlags::FLG_OBJ_DONT_USE_VSS;

  objects[oi].name = name;
  objects[oi].mats = mats;
  objects[oi].mesh = mesh;
  objects[oi].lmSampleSize = lm_sample_size;
  objects[oi].vssDivCount = vss_div_count;
  objects[oi].visRange = visrange;
  objects[oi].topLodObjId = top_lod_obj_id;
  objects[oi].lodRange = lod_range;
  objects[oi].lodNearVisRange = lod_near_range;
  objects[oi].lodFarVisRange = lod_far_range;
  objects[oi].transSortPriority = trans_sort_priority;
  objects[oi].initMesh(wtm, flags, normals_dir);
  objects[oi].unwrapScheme = unwrap_scheme;

  totalFaces += objects[oi].mesh.getFaceCount();
  for (int i = 0; i < mats.size(); ++i)
    addMaterial(mats[i]);
}

bool StaticSceneBuilder::LightmappedScene::checkGeometry(const StaticGeometryNode &node, ILogWriter *log)
{
  // returns:
  //   true, if geometry/texturex seems to be correct;
  //   false, if something wrong and export cannot continue.

  bool success = true;
  bool has_degenerated_face = false;
  bool has_no_smoothing = false;
  const Mesh &mesh = node.mesh->mesh;
  if (StaticSceneBuilder::check_no_smoothing)
    for (int f = 0; f < mesh.getFaceCount(); ++f)
      if (!mesh.getFaceSMGR(f))
      {
        has_no_smoothing = true;
        const_cast<Mesh &>(mesh).setFaceSMGR(f, 1 << 30);
      }
  if (StaticSceneBuilder::check_degenerates)
    for (int f = 0; f < mesh.getFaceCount(); ++f)
    {
      Point3 v0 = mesh.getVert()[mesh.face[f].v[0]];
      Point3 v1 = mesh.getVert()[mesh.face[f].v[1]];
      Point3 v2 = mesh.getVert()[mesh.face[f].v[2]];
      real ss = defLmSampleSize / node.ltMul;
      real e0_l2 = lengthSq(v1 - v0) / ss;
      real e1_l2 = lengthSq(v2 - v1) / ss;
      real e2_l2 = lengthSq(v0 - v2) / ss;
      real epsilon = 0.000001;
      if (e0_l2 < epsilon || e1_l2 < epsilon || e2_l2 < epsilon)
      {
        has_degenerated_face = true;
        break;
      }
    }
  for (unsigned t = 0; t < node.mesh->mats.size(); ++t)
  {
    StaticGeometryMaterial *mat = node.mesh->mats[t];
    if (!mat)
      continue;
    bool is_envi = !strnicmp(mat->className, "hdr_envi", 8);
    for (unsigned i = 0; i < StaticGeometryMaterial::NUM_TEXTURES; ++i)
    {
      StaticGeometryTexture *th = mat->textures[i];
      if (th)
      {
        const char *fn = TextureMetaData::decodeFileName(th->fileName);
        if (!trail_strcmp(fn, "*") && !dd_file_exist(fn))
        {
          String msg(256, "missing texture: '%s'", th->fileName.str());
          if (log)
            log->addMessage(ILogWriter::ERROR, msg);
          debug(msg);
        }
      }
    }
  }
  if (has_degenerated_face && log)
    log->addMessage(ILogWriter::WARNING, "Object '%s' has degenerative face(s)", node.name);
  if (has_no_smoothing && log)
    log->addMessage(NO_SMOOTHING_IS_ERROR ? ILogWriter::ERROR : ILogWriter::WARNING, "Object '%s' has face(s) with no smoothing",
      node.name);

  return success;
}


void StaticSceneBuilder::LightmappedScene::addGeom(dag::ConstSpan<StaticGeometryNode *> nodes, ILogWriter *log)
{
  String objName;

  int firstObj = objects.size();

  for (int i = 0; i < nodes.size(); ++i)
  {
    if (!nodes[i])
      continue;
    StaticGeometryNode &n = *nodes[i];
    if (!n.mesh)
      continue;
    if (!n.mesh->mats.size())
      continue;

    objName.printf(512, "[%d] %s", i, (const char *)(n.name));
    if (checkGeometry(n, log))
    {
      real lmSampleSize = defLmSampleSize;
      if (n.ltMul)
      {
        lmSampleSize = defLmSampleSize / n.ltMul;
        if (lmSampleSize <= 0)
          lmSampleSize = defLmSampleSize;
      }

      addObject(n.mesh->mesh, n.mesh->mats, n.wtm, objName, lmSampleSize, n.flags, n.unwrapScheme, n.normalsDir, n.vss, n.vltMul,
        n.visRange, n.transSortPriority, i, n.lodRange, n.lodNearVisRange, n.lodFarVisRange);
    }
    else
      break;
  }

  // find top lod objects
  for (int oi = firstObj; oi < objects.size(); ++oi)
  {
    int ni = objects[oi].topLodObjId;

    int ti = -1;

    if (nodes[ni]->topLodName.length())
    {
      for (ti = firstObj; ti < objects.size(); ++ti)
      {
        const char *name = strchr(objects[ti].name, ']');
        G_ASSERT(name);
        name += 2;

        if (stricmp(name, nodes[ni]->topLodName) == 0)
          break;
      }

      if (ti >= objects.size())
        ti = -1;
    }

    objects[oi].topLodObjId = ti;
  }
}


int StaticSceneBuilder::LightmappedScene::getTextureIndex(StaticGeometryTexture *texture) { return getTextureId(texture); }


StaticGeometryTexture *StaticSceneBuilder::LightmappedScene::getTexture(int id)
{
  if (id < 0 || id >= textures.size())
    return NULL;
  return textures[id];
}

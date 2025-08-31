// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sceneBuilder/nsb_LightmappedScene.h>
#include <sceneBuilder/nsb_LightingProvider.h>
#include <sceneBuilder/nsb_StdTonemapper.h>
#include <generic/dag_sort.h>
#include <libTools/staticGeom/matFlags.h>
#include <libTools/util/strUtil.h>
#include <libTools/shaderResBuilder/globalVertexDataConnector.h>
#include <libTools/shaderResBuilder/matSubst.h>
#include <libTools/util/prepareBillboardMesh.h>
#include <libTools/util/makeBindump.h>
#include <shaders/dag_bboxTreeRElem.h>
#include <shaders/dag_renderScene.h>
#include <3d/dag_materialData.h>
#include <math/dag_boundingSphere.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_zlibIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_bitArray.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <stdlib.h>
#include "sceneSplitObjects.h"
#include <image/dag_texPixel.h>

#define BBOX_TREE_LEAVES_X 70
#define BBOX_TREE_LEAVES_Y 20
#define BBOX_TREE_LEAVES_Z 70

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define STATIC_BUMP_SHININESS_SCALE 1.f
#define MIN_SHININESS               0.1f

unsigned short int float_to_half(float val);

namespace StaticSceneBuilder
{
int save_lightmap_files(const char *fname_base, TexImage32 *img, int scene_format, int target_code, mkbindump::BinDumpSaveCB &cwr,
  bool keep_ref_files);

int save_lightmap_bump(const char *fname_base, TexImage32 *flat_img, TexImage32 *sunbump_color_img, mkbindump::BinDumpSaveCB &cwr,
  bool keep_ref_files);
} // namespace StaticSceneBuilder

class StaticSceneBuilder::StaticVisualScene
{
public:
  DAG_DECLARE_NEW(midmem)

  StaticVisualScene();
  ~StaticVisualScene();

  bool buildAndSave1(LightmappedScene &from, LightingProvider &lighting, StdTonemapper &tonemapper, Splitter &splitter,
    const char *foldername, int scene_format, unsigned target_code, IGenericProgressIndicator &pbar, ILogWriter &log,
    bool should_make_reference);
  bool buildAndSave(LightmappedScene &from, LightingProvider &lighting, StdTonemapper &tonemapper, Splitter &splitter,
    const char *foldername, int scene_format, dag::Span<unsigned> add_target_code, IGenericProgressIndicator &pbar, ILogWriter &log,
    bool should_make_reference);

  bool canAttachDataElem(const Tab<ShaderMeshData::RElem> &elem, const Tab<ShaderMeshData::RElem> &other_elem, bool allow_32_bit);

protected:
  struct ObjectLodIndex
  {
    int objIndex;
    real lodRange;
    ObjectLodIndex() = default;
    ObjectLodIndex(int index, real range) : objIndex(index), lodRange(range) {}

    static int compare(const ObjectLodIndex *a, const ObjectLodIndex *b)
    {
      if (a->lodRange > b->lodRange)
        return 1;
      if (a->lodRange < b->lodRange)
        return -1;
      return 0;
    }
  };

  struct ObjectTmpSaveData
  {
    mkbindump::Ref lodsRef;
    int nameOfs;
  };

  struct ShaderMDRec
  {
    ShaderMeshData *mesh;
    int szPos;
    ShaderMDRec(ShaderMeshData *m, int pos) : mesh(m), szPos(pos) {}
  };

  struct Object
  {
    SimpleString name;

    ShaderMeshData *meshData;

    BSphere3 bsph;
    BBox3 bbox;

    unsigned short flags;
    unsigned char rsceneParams;
    unsigned char transSortPriority;

    real visRange;
    real maxSubRad;

    real lodNearVisRange, lodFarVisRange;

    real lodRange;
    int orgObjId;
    int topLodObjId;

    real expandRadius;

    bool visible;

    unsigned int lightingObjectDataNo; // Index in the list of passed to split_objects lights.


    Object() : meshData(NULL), visible(true), expandRadius(0), maxSubRad(0), lightingObjectDataNo(0xFFFFFFFF) {}

    Object(const Object &c) { DAG_FATAL("Implement properly copy constructor"); }

    ~Object() { del_it(meshData); }

    void copySavableParams(Object &right)
    {
      name = right.name;
      bsph = right.bsph;
      maxSubRad = right.maxSubRad;
      bbox = right.bbox;
      flags = right.flags;
      rsceneParams = right.rsceneParams;
      transSortPriority = right.transSortPriority;
      visRange = right.visRange;
      visible = right.visible;
      lodRange = right.lodRange;
      lodNearVisRange = right.lodNearVisRange;
      lodFarVisRange = right.lodFarVisRange;
      orgObjId = right.orgObjId;
      topLodObjId = right.topLodObjId;
      expandRadius = right.expandRadius;
      lightingObjectDataNo = right.lightingObjectDataNo;
    }

    float lodNearVr2() { return lodNearVisRange > 0 ? lodNearVisRange * lodNearVisRange : lodNearVisRange; };
    float lodFarVr2() { return lodFarVisRange > 0 ? lodFarVisRange * lodFarVisRange : lodFarVisRange; };
  };

  Tab<Object> objects;
  bool buildShadowSD;

  void connectVertexData(IGenericProgressIndicator &pbar, bool optimize_for_vertex_cache);

  void save(const char *scene_fname, unsigned target_code, int scene_format, LightingProvider &lighting, StdTonemapper &tonemapper,
    int ltmap_count, const char *ltmap_ref_base, IGenericProgressIndicator &pbar);

  /*
  void prebuildAndSaveSpacialDataForShadows(mkbindump::BinDumpSaveCB &cwr,
                                            ShaderMeshDataSaveCB &mdcb);
  void saveBBoxTreeNode(BBoxTreeNode *node, unsigned int current_depth, unsigned int tree_depth,
                        mkbindump::BinDumpSaveCB &cwr, ShaderMeshDataSaveCB &mdcb);
  */

  bool checkLostGeom();
  static Color4 bumpDirAndSizeToColor4(const Point3 &dir, float size);
};
DAG_DECLARE_RELOCATABLE(StaticSceneBuilder::StaticVisualScene::Object);


class StaticSceneBuilder::LightmappedMaterials
{
public:
  LightmappedMaterials() : materials(tmpmem) {}

  ShaderMaterial *getMaterial(ShaderMaterial *mat, int lmid)
  {
    if (!mat)
      return mat;

    for (int i = 0; i < materials.size(); ++i)
    {
      ShaderMaterial *m = materials[i].getMaterial(mat, lmid);
      if (m)
        return m;
    }

    int i = append_items(materials, 1);
    materials[i].orgMat = mat;
    return materials[i].getMaterial(mat, lmid);
  }

protected:
  struct MaterialData
  {
    Ptr<ShaderMaterial> orgMat;
    PtrTab<ShaderMaterial> lmMats;

    MaterialData() : lmMats(tmpmem) {}

    ShaderMaterial *getMaterial(ShaderMaterial *mat, int lmid)
    {
      if (orgMat != mat)
      {
        int i;
        for (i = 0; i < lmMats.size(); ++i)
          if (lmMats[i] == mat)
            break;
        if (i >= lmMats.size())
          return NULL;
      }

      if (lmid >= lmMats.size())
        lmMats.resize(lmid + 1);
      else if (lmMats[lmid])
        return lmMats[lmid];

      ShaderMaterial *sm = mat->clone();
      G_ASSERT(sm);

      lmMats[lmid] = sm;

      return sm;
    }
  };

  Tab<MaterialData> materials;
};


StaticSceneBuilder::StaticVisualScene::StaticVisualScene() : objects(midmem) { buildShadowSD = true; }


StaticSceneBuilder::StaticVisualScene::~StaticVisualScene() { clear_and_shrink(objects); }


Color4 StaticSceneBuilder::StaticVisualScene::bumpDirAndSizeToColor4(const Point3 &dir, float size)
{
  // Raw approximation of log(~0.5) / log(cos(size)).
  if (size > TWOPI)
    size = TWOPI;
  float angle = acosf(1.0f - size / TWOPI) * 2.0f;
  float shininess = (1. - angle) * STATIC_BUMP_SHININESS_SCALE;
  if (shininess < MIN_SHININESS)
    shininess = MIN_SHININESS;

  if (shininess > 1.f)
    shininess = 1.f;

  return Color4(dir.x * 0.5f + 0.5f, dir.y * 0.5f + 0.5f, dir.z * 0.5f + 0.5f, shininess);
}


bool StaticSceneBuilder::StaticVisualScene::canAttachDataElem(const Tab<ShaderMeshData::RElem> &elem,
  const Tab<ShaderMeshData::RElem> &other_elem, bool allow_32_bit)
{
  for (int i = 0; i < other_elem.size(); i++)
  {
    const ShaderMeshData::RElem &otherElem = other_elem[i];

    // find render element with same material & vertex data
    for (int j = 0; j < elem.size(); j++)
    {
      const ShaderMeshData::RElem &thisElem = elem[j];
      if (can_combine_elems(thisElem, otherElem, allow_32_bit))
        return true;
    }
  }

  return false;
}


bool StaticSceneBuilder::StaticVisualScene::buildAndSave1(LightmappedScene &scene, LightingProvider &lighting,
  StdTonemapper &tonemapper, Splitter &splitter, const char *foldername, int scene_format, unsigned target_code,
  IGenericProgressIndicator &pbar, ILogWriter &log, bool should_make_reference)
{
  buildShadowSD = splitter.buildShadowSD;

  objects.clear();
  objects.resize(scene.objects.size());

  LightmappedMaterials lmMats;

  // create materials
  PtrTab<ShaderMaterial> shmats(tmpmem);
  shmats.resize(scene.materials.size());

  pbar.setActionDesc("Building scene: creating materials");
  pbar.setDone(0);
  pbar.setTotal(shmats.size());

  for (int i = 0; i < shmats.size(); ++i)
  {
    StaticGeometryMaterial *gmPtr = scene.materials[i];
    if (!gmPtr)
      continue;
    StaticGeometryMaterial &gm = *gmPtr;

    pbar.incDone();

    Ptr<MaterialData> matNull = new MaterialData;

    matNull->matName = gm.name;
    matNull->matScript = gm.scriptText;
    if (!gm.className.empty())
      matNull->className = gm.className;
    else
    {
      matNull->className = "__default";
      log.addMessage(ILogWriter::WARNING, "Empty Shader class in material <%s> substituted to <%s>", gm.name.str(),
        matNull->className);
    }

    if ((gm.flags & (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED)) == (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED))
      matNull->flags = 0;
    else
      matNull->flags = (gm.flags & MatFlags::FLG_2SIDED) ? MATF_2SIDED : 0;

    matNull->mat.amb = gm.amb;
    matNull->mat.diff = gm.diff;
    matNull->mat.spec = gm.spec;
    matNull->mat.emis = gm.emis;
    matNull->mat.power = gm.power;
    static_geom_mat_subst.substMatClass(*matNull);

    for (int ti = 0; ti < StaticGeometryMaterial::NUM_TEXTURES && ti < MAXMATTEXNUM; ++ti) //-V560
    {
      if (!gm.textures[ti])
        continue;
      matNull->mtex[ti] = ::add_managed_texture(gm.textures[ti]->fileName);
    }

    shmats[i] = new_shader_material(*matNull, true);
    if (!shmats[i])
      log.addMessage(ILogWriter::WARNING, "Shader material [%d] not found: <%s> class=<%s>", i, matNull->matName, matNull->className);
  }

  // compile meshes

  pbar.setActionDesc("Building scene: building meshes...");
  pbar.setDone(0);
  pbar.setTotal(scene.totalFaces);

  Tab<String> wrongMeshes(tmpmem);

  for (int i = 0; i < objects.size(); ++i)
  {
    Object &obj = objects[i];
    LightmappedScene::Object &lmobj = scene.objects[i];

    pbar.incDone(lmobj.mesh.getFaceCount());

    obj.name = lmobj.name;

    PtrTab<ShaderMaterial> mat(tmpmem);
    mat.resize(lmobj.mats.size());

    for (int j = 0; j < mat.size(); ++j)
    {
      int id = scene.getMaterialId(lmobj.mats[j]);
      if (id >= 0)
        mat[j] = shmats[id];
    }

    for (int j = 0; j < mat.size(); ++j)
      if (!mat[j])
        for (int f = 0; f < lmobj.mesh.getFaceCount(); ++f)
          if (lmobj.mesh.getFaceMaterial(f) == j)
          {
            log.addMessage(ILogWriter::ERROR, "Missing shader mat[%d] for object <%s>, face %d", j, lmobj.name, f);
            log.endLog();
            return false;
          }

    Mesh mesh = lmobj.mesh;

    Tab<uint16_t> lmid(tmpmem);
    lmid = lmobj.lmIds;

    Bitarray faceNeedVlm(tmpmem);
    faceNeedVlm.resize(mesh.getFaceCount());
    for (int j = 0; j < mesh.getFaceCount(); ++j)
      faceNeedVlm.set(j, lmobj.mats[mesh.getFaceMaterial(j)] && (lmobj.mats[mesh.getFaceMaterial(j)]->flags & MatFlags::FLG_USE_VLM));

    // convert LM TC (if crop shr != 0,0)
    Tab<Point2> lmTc(tmpmem);
    lmTc = lmobj.lmTc;

    if (lmid.size() == mesh.getFaceCount())
    {
      for (int f = 0; f < mesh.getFaceCount(); f++)
      {
        int li = lmid[f];
        if (li == LightmappedScene::BAD_LMID)
          continue; // not lightmapped

        IPoint2 cropShr = lighting.getLightmapCropShr(li);
        if (cropShr == IPoint2(0, 0))
          continue; // not cropped, nothing to do

        cropShr.x = 1 << cropShr.x;
        cropShr.y = 1 << cropShr.y;

        for (int j = 0; j < 3; j++)
        {
          int ti = lmobj.lmFaces[f].t[j];
          lmTc[ti].x = lmobj.lmTc[ti].x * cropShr.x;
          lmTc[ti].y = lmobj.lmTc[ti].y * cropShr.y;
          // debug ( "%d,%d,%d: convert tc (%.3f,%.3f -> %.3f,%.3f)", i, f, ti, lmobj.lmTc[ti].x, lmobj.lmTc[ti].y, lmTc[ti].x,
          // lmTc[ti].y );
        }
      }
    }

    // split materials by lmIds
    if (lmid.size() == mesh.getFaceCount())
    {
      for (int f = 0; f < mesh.getFaceCount();)
      {
        int mi = mesh.getFaceMaterial(f);
        int ef;
        for (ef = f + 1; ef < mesh.getFaceCount(); ++ef)
          if (mesh.getFaceMaterial(ef) != mi)
            break;

        bool split = false;

        for (; f < ef; ++f)
        {
          int li = lmid[f];
          if (li == LightmappedScene::BAD_LMID)
            continue;

          ShaderMaterial *sm = lmMats.getMaterial(mat[mi], li);

          if (!split)
          {
            split = true;
            mat[mi] = sm;
            for (int j = f; j < ef; ++j)
              if (lmid[j] == li)
                lmid[j] = LightmappedScene::BAD_LMID;
          }
          else
          {
            int ni = mat.size();
            mat.push_back(sm);
            for (int j = f; j < ef; ++j)
              if (lmid[j] == li)
              {
                lmid[j] = LightmappedScene::BAD_LMID;
                mesh.setFaceMaterial(j, ni);
              }
          }
        }
      }
    }

    // put LM TC into the mesh
    if (lmobj.lmFaces.size())
    {
      int ci = mesh.add_extra_channel(MeshData::CHT_FLOAT2, SCUSAGE_LIGHTMAP, 0);
      G_ASSERT(ci >= 0);

      MeshData::ExtraChannel &c = mesh.extra[ci];

      c.resize_verts(lmTc.size());
      mem_copy_from(c.vt, lmTc.data());

      c.fc = lmobj.lmFaces;
    }
    clear_and_shrink(lmTc);

    // put VLM colors into the mesh
    if (lmobj.vltmapFaces.size())
    {
      if (mesh.cface.size() != mesh.getFaceCount())
      {
        mesh.cvert.resize(1);
        mesh.cvert[0] = Color4(1, 1, 1, 1);

        mesh.cface.resize(mesh.getFaceCount());
        mem_set_0(mesh.cface);
      }

      const int cvi = mesh.cvert.size();

      mesh.cvert.resize(cvi + lmobj.vlmSize);

      int sunBumpColorChannelId = -1;

      if (scene_format == SCENE_FORMAT_LdrTga || scene_format == SCENE_FORMAT_LdrDds || scene_format == SCENE_FORMAT_AO_DXT1 ||
          scene_format == SCENE_FORMAT_LdrTgaDds)
      {
        for (int i = 0; i < lmobj.vlmSize; ++i)
        {
          E3DCOLOR c = lighting.getVltmapColor(lmobj.vlmIndex + i);
          mesh.cvert[cvi + i] = Color4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1);
        }
      }
      else if (scene_format == SCENE_FORMAT_SunBump)
      {
        // Create bump channels.
        G_ASSERT(mesh.find_extra_channel(SCUSAGE_EXTRA, 52) < 0);
        sunBumpColorChannelId = mesh.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, 52);
        G_ASSERT(sunBumpColorChannelId >= 0);
        MeshData::ExtraChannel &sunBumpColorChannel = mesh.extra[sunBumpColorChannelId];
        sunBumpColorChannel.resize_verts(lmobj.vlmSize);
        sunBumpColorChannel.fc.resize(mesh.getFaceCount());
        Color4 *sunBumpColorData = (Color4 *)&sunBumpColorChannel.vt[0];
        for (int i = 0; i < lmobj.vlmSize; ++i)
        {
          E3DCOLOR flat = lighting.getVltmapColor(lmobj.vlmIndex + i);
          mesh.cvert[cvi + i] = Color4(flat.r / 255.0f, flat.g / 255.0f, flat.b / 255.0f, 1);
          // sunBumpColorData[i] = color4(lighting.getSunBump(lmobj.vlmIndex + i));
          mesh.cvert[cvi + i] = color4(lighting.getSunBump(lmobj.vlmIndex + i));
        }
      }
      else
      {
        pbar.setActionDesc(String(128, "Unknown scene format: %d", scene_format));
        debug("Unknown scene format: %d", scene_format);
        for (int i = 0; i < objects.size(); ++i)
          del_it(objects[i].meshData);
        return false;
      }
      for (int i = 0; i < mesh.getFaceCount(); ++i)
      {
        if (!faceNeedVlm[i])
          continue;

        mesh.cface[i] = lmobj.vltmapFaces[i];
        mesh.cface[i].t[0] += cvi;
        mesh.cface[i].t[1] += cvi;
        mesh.cface[i].t[2] += cvi;

        if (scene_format == SCENE_FORMAT_SunBump)
          mesh.extra[sunBumpColorChannelId].fc[i] = lmobj.vltmapFaces[i];
      }
    }
    // calc bounding sphere
    obj.bsph = ::mesh_fast_bounding_sphere(mesh.getVert().data(), mesh.getVertCount());
    obj.maxSubRad = obj.bsph.r;

    if (check_nan(obj.bsph.c.x) || check_nan(obj.bsph.c.y) || check_nan(obj.bsph.c.z))
    {
      log.addMessage(log.WARNING, "NaN vertices in object %d <%s>, skipped", i, obj.name.str());
      debug("object <%s> bsph=" FMT_P3 "/%.3f  verts=%d", obj.name.str(), P3D(obj.bsph.c), obj.bsph.r, mesh.getVertCount());
      for (int j = 0; j < mesh.getVertCount(); j++)
        debug("%d: " FMT_P3 "", j, P3D(mesh.getVert()[j]));

      erase_items(objects, i, 1);
      erase_items(scene.objects, i, 1);
      i--;
      continue;
    }

    obj.bbox.setempty();
    for (int j = 0; j < mesh.getVertCount(); j++)
      obj.bbox += mesh.getVert()[j];

    if (lmobj.flags & ObjFlags::FLG_BILLBOARD_MESH)
    {
      int chi = mesh.find_extra_channel(SCUSAGE_EXTRA, 0);
      if (chi < 0)
      {
        SmallTab<Point3, TmpmemAlloc> orgVerts(mesh.vert);
        ::prepare_billboard_mesh(mesh, TMatrix::IDENT, orgVerts);
      }

      chi = mesh.find_extra_channel(SCUSAGE_EXTRA, 0);
      G_ASSERT(chi >= 0);

      MeshData::ExtraChannel &ch = mesh.extra[chi];

      real rsq = 0;
      int numv = ch.numverts();
      Point3 *deltas = (Point3 *)&ch.vt[0];

      for (int i = 0; i < numv; ++i)
      {
        real d = lengthSq(deltas[i]);
        if (d > rsq)
          rsq = d;
      }

      obj.expandRadius = sqrtf(rsq);

      if (!obj.bbox.isempty())
      {
        obj.bbox[0] -= Point3(obj.expandRadius, obj.expandRadius, obj.expandRadius);
        obj.bbox[1] += Point3(obj.expandRadius, obj.expandRadius, obj.expandRadius);

        obj.bsph.r += obj.expandRadius;
        obj.maxSubRad += obj.expandRadius;
        obj.bsph.r2 = obj.bsph.r * obj.bsph.r;
      }
    }

    obj.rsceneParams = 0;
    if (lmobj.flags & ObjFlags::FLG_OBJ_FADE)
      obj.rsceneParams |= RenderScene::RenderObject::ROP_FADE;

    if (lmobj.flags & ObjFlags::FLG_OBJ_FADENULL)
      obj.rsceneParams |= RenderScene::RenderObject::ROP_FADENULL | RenderScene::RenderObject::ROP_FADE;

    if (lmobj.flags & ObjFlags::FLG_BACK_FACE_DYNAMIC_LIGHTING)
      obj.rsceneParams |= RenderScene::RenderObject::ROP_BACK_FACE_DYNAMIC_LIGHTING;

    if (lmobj.flags & ObjFlags::FLG_DO_NOT_MIX_LODS)
      obj.rsceneParams |= RenderScene::RenderObject::ROP_NOT_MIX_LODS;

    obj.flags = 0;
    if (lmobj.flags & ObjFlags::FLG_CAST_SHADOWS)
      obj.flags |= RenderScene::RenderObject::ROF_CastShadows;

    obj.transSortPriority = lmobj.transSortPriority;
    obj.visRange = lmobj.visRange;

    obj.orgObjId = i;
    obj.topLodObjId = lmobj.topLodObjId;
    obj.lodRange = lmobj.lodRange;

    obj.lodNearVisRange = lmobj.lodNearVisRange;
    obj.lodFarVisRange = lmobj.lodFarVisRange;

    for (int l = 0; l < mesh.facengr.size(); ++l)
      for (int j = 0; j < 3; ++j)
        if (mesh.facengr[l][j] >= mesh.vertnorm.size() || (mesh.facengr[l][j] >= mesh.ngr.size() && mesh.ngr.size()))
        {
          debug("facengr %d has invalid ngr %d", l, mesh.facengr[l][j]);
          pbar.setActionDesc(String(128, "Wrong mesh %s (center: %g, %g%, %g)", obj.name.str(), obj.bbox.center().x,
            obj.bbox.center().y, obj.bbox.center().z));
          wrongMeshes.push_back() = obj.name.str();
        }

    if (wrongMeshes.size())
      continue;

    obj.meshData = new (midmem) ShaderMeshData;
    obj.meshData->build(mesh, (ShaderMaterial **)mat.data(), mat.size(), IdentColorConvert::object);
  }


  for (int i = 0; i < wrongMeshes.size(); i++)
    pbar.setActionDesc(String(128, "Wrong mesh %s", (char *)wrongMeshes[i]));

  if (wrongMeshes.size())
  {
    for (int i = 0; i < objects.size(); ++i)
      del_it(objects[i].meshData);
    return false;
  }


  if (checkLostGeom())
    pbar.setActionDesc("Some geometry data is lost!");


  //
  // Remove empty objects.
  //

  debug("Empty objects removing started with %d objects.", objects.size());
  for (unsigned int objectNo = 0; objectNo < objects.size(); objectNo++)
  {
    if (objects[objectNo].meshData->elems.empty())
    {
      erase_items(objects, objectNo, 1);
      objectNo--;
    }
  }
  debug("Empty objects removing finished with %d objects.", objects.size());

  if (checkLostGeom())
    pbar.setActionDesc("Some geometry data is lost!");


  //
  // Combine small objects.
  //

  if (splitter.combineObjects)
  {
    pbar.setActionDesc("Combining objects...");
    pbar.setDone(0);
    pbar.setTotal(objects.size());

#ifdef TEST_BUILD
    for (unsigned int objectNo = 0; objectNo < objects.size(); objectNo++)
    {
      debug("objectNo=%d, elem.count=%d", objectNo, objects[objectNo].meshData->elems.size());

      for (unsigned int elemNo = 0; elemNo < objects[objectNo].meshData->elems.size(); elemNo++)
        debug("%s", (const char *)objects[objectNo].meshData->elems[elemNo].mat->getInfo());
    }
#endif

    debug("combine_objects started with %d objects.", objects.size());

    Tab<Point3> points(tmpmem);
    Tab<bool> objectIsSmall(tmpmem);
    objectIsSmall.resize(objects.size());
    for (unsigned int i = 0; i < objects.size(); i++)
      objectIsSmall[i] = (objects[i].maxSubRad <= splitter.smallRadThres);

    real maxSizeSq = splitter.maxCombinedSize * splitter.maxCombinedSize;

    for (unsigned int leftObjectNo = 0; leftObjectNo < objects.size(); leftObjectNo++)
    {
      bool hasAttached = false;
      // for (unsigned int rightObjectNo = leftObjectNo + 1; rightObjectNo < objects.size(); rightObjectNo++)
      for (unsigned int rightObjectNo = objects.size() - 1; rightObjectNo > leftObjectNo; rightObjectNo--)
      {

        BBox3 comboBox = objects[leftObjectNo].bbox;
        comboBox += objects[rightObjectNo].bbox;

        if (lengthSq(comboBox.width()) > maxSizeSq)
          continue; // Combined size is too big.

        if (splitter.trianglesPerObjectTargeted > 0 &&
            get_triangle_num(objects[leftObjectNo]) + get_triangle_num(objects[rightObjectNo]) > splitter.trianglesPerObjectTargeted)
          continue; // Combined object would be too fat.

        if (objects[leftObjectNo].rsceneParams != objects[rightObjectNo].rsceneParams ||
            objects[leftObjectNo].flags != objects[rightObjectNo].flags ||
            objects[leftObjectNo].transSortPriority != objects[rightObjectNo].transSortPriority ||
            !is_equal_float(objects[leftObjectNo].visRange, objects[rightObjectNo].visRange) ||
            objects[leftObjectNo].visible != objects[rightObjectNo].visible ||
            objects[leftObjectNo].topLodObjId != objects[rightObjectNo].topLodObjId ||
            !is_equal_float(objects[leftObjectNo].lodRange, objects[rightObjectNo].lodRange) ||
            objects[leftObjectNo].lodNearVisRange > 0 || objects[rightObjectNo].lodNearVisRange > 0 ||
            !is_equal_float(objects[leftObjectNo].lodFarVisRange, objects[rightObjectNo].lodFarVisRange))
        {
          continue; // Different flags.
        }

        bool doAttach = canAttachDataElem(objects[leftObjectNo].meshData->elems, objects[rightObjectNo].meshData->elems, false);

        if (doAttach)
        {
          if (objectIsSmall[leftObjectNo])
          {
            float lsr = objects[leftObjectNo].maxSubRad;
            float rsr = objects[rightObjectNo].maxSubRad;
            if (rsr > splitter.maxCombinedSize)
              doAttach = false; // don't combine small with large
            else if (rsr > lsr * splitter.maxCombineRadRatio || lsr > rsr * splitter.maxCombineRadRatio)
              doAttach = false; // don't combine when radius ratio is too large
          }
          else if (objectIsSmall[rightObjectNo])
          {
            float lsr = objects[leftObjectNo].maxSubRad;
            float rsr = objects[rightObjectNo].maxSubRad;
            if (lsr > splitter.maxCombinedSize)
              doAttach = false; // don't combine large with small
            else if (rsr > lsr * splitter.maxCombineRadRatio || lsr > rsr * splitter.maxCombineRadRatio)
              doAttach = false; // don't combine when radius ratio is too large
          }
        }

        if (doAttach)
        {
          if (objects[leftObjectNo].expandRadius < objects[rightObjectNo].expandRadius)
            objects[leftObjectNo].expandRadius = objects[rightObjectNo].expandRadius;

          inplace_max(objects[leftObjectNo].maxSubRad, objects[rightObjectNo].maxSubRad);
          objects[leftObjectNo].meshData->attachData(*objects[rightObjectNo].meshData, false);
          erase_items(objects, rightObjectNo, 1);
          erase_items(objectIsSmall, rightObjectNo, 1);
          objects[leftObjectNo].bbox = comboBox;
          hasAttached = true;
        }
      }

      if (hasAttached)
      {
        recalc_sphere_and_bbox(objects[leftObjectNo], points);

        const real r = objects[leftObjectNo].expandRadius;

        objects[leftObjectNo].bbox[0] -= Point3(r, r, r);
        objects[leftObjectNo].bbox[1] += Point3(r, r, r);

        objects[leftObjectNo].bsph.r += r;
        objects[leftObjectNo].bsph.r2 = objects[leftObjectNo].bsph.r * objects[leftObjectNo].bsph.r;
      }
      pbar.incDone();
    }

    debug("combine_objects finished with %d objects.", objects.size());

#ifdef TEST_BUILD
    for (unsigned int objectNo = 0; objectNo < objects.size(); objectNo++)
    {
      debug("objectNo=%d, elem.count=%d", objectNo, objects[objectNo].meshData->elems.size());

      for (unsigned int elemNo = 0; elemNo < objects[objectNo].meshData->elems.size(); elemNo++)
        debug("%s", (const char *)objects[objectNo].meshData->elems[elemNo].mat->getInfo());
    }
#endif

    clear_and_shrink(points);

    if (checkLostGeom())
      pbar.setActionDesc("Some geometry data is lost!");
  }


  // Check for index32.

  for (unsigned int objectNo = 0; objectNo < objects.size(); objectNo++)
  {
    dag::ConstSpan<ShaderMeshData::RElem> elems = objects[objectNo].meshData->elems;
    for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++)
    {
      if (elems[elemNo].vertexData->iData32.size() > 0)
        logerr("index32 created.");
      if (elems[elemNo].vertexData->numv > 0xFFFF)
        logerr("numv > 0xFFFF.");
      if (elems[elemNo].vertexData->numf * 3 > 0xFFFF)
        logerr("numf * 3 > 0xFFFF.");
    }
  }


  //
  // Split big fat objects.
  //
  if (splitter.splitObjects && splitter.trianglesPerObjectTargeted > 0)
  {
    pbar.setActionDesc("Splitting objects...");
    pbar.setDone(0);
    pbar.setTotal(1);

    split_objects(objects, (unsigned int)splitter.trianglesPerObjectTargeted, splitter.minRadiusToSplit, pbar, log);
    pbar.setDone(1);

    if (checkLostGeom())
      pbar.setActionDesc("Some geometry data is lost!");
  }


  connectVertexData(pbar, splitter.optimizeForVertexCache);

  if (checkLostGeom())
    pbar.setActionDesc("Some geometry data is lost!");


  // save
  debug("Save scene");

  pbar.setActionDesc("Saving scene");
  String scene_fname(foldername);

  uint64_t tc_storage = 0;
  const char *tc_str = mkbindump::get_target_str(target_code, tc_storage);
  if (scene_format == SCENE_FORMAT_LdrTga || scene_format == SCENE_FORMAT_LdrDds || scene_format == SCENE_FORMAT_LdrTgaDds ||
      scene_format == SCENE_FORMAT_AO_DXT1 || scene_format == SCENE_FORMAT_SunBump)
    scene_fname.aprintf(0, "/scene-%s.scn", tc_str);
  else
    scene_fname.aprintf(0, "/scene_XXX-%s.scn", tc_str);

  debug("lightmaps: scn=%d light=%d", scene.lightmaps.size(), lighting.getLightmapCount());

  save(scene_fname, target_code, scene_format, lighting, tonemapper, lighting.getLightmapCount(),
    should_make_reference ? foldername : NULL, pbar);

  for (int i = 0; i < objects.size(); ++i)
    del_it(objects[i].meshData);
  return true;
}
bool StaticSceneBuilder::StaticVisualScene::buildAndSave(LightmappedScene &scene, LightingProvider &lighting,
  StdTonemapper &tonemapper, Splitter &splitter, const char *foldername, int scene_format, dag::Span<unsigned> add_target_code,
  IGenericProgressIndicator &pbar, ILogWriter &log, bool should_make_reference)
{
  if (!buildAndSave1(scene, lighting, tonemapper, splitter, foldername, scene_format, _MAKE4C('PC'), pbar, log, should_make_reference))
    return false;
  // save .SCN files for other requested platforms
  for (int i = 0; i < add_target_code.size(); i++)
    if (!buildAndSave1(scene, lighting, tonemapper, splitter, foldername, scene_format, add_target_code[i], pbar, log, false))
      return false;
  return true;
}

void StaticSceneBuilder::StaticVisualScene::save(const char *scene_fname, unsigned target_code, int scene_format,
  LightingProvider &lighting, StdTonemapper &tonemapper, int ltmap_count, const char *ltmap_ref_base, IGenericProgressIndicator &pbar)
{
  ShaderMaterialsSaver matSaver;
  ShaderTexturesSaver texSaver;
  bool write_be = dagor_target_code_be(target_code);
  mkbindump::BinDumpSaveCB cwr(96 << 20, target_code, write_be);

  try
  {
    int objSzpos, lmCatPos, obj_sz;

    for (int i = 0; i < objects.size(); ++i)
    {
      ShaderMeshData &md = *objects[i].meshData;

      for (int j = 0; j < md.elems.size(); ++j)
        matSaver.addMaterial(md.elems[j].mat, texSaver, NULL);

      md.enumVertexData(matSaver);
    }

    // gather list of top-lods
    Tab<int> topObjInd(tmpmem), fadeObjIdx(tmpmem);
    topObjInd.reserve(objects.size());

    for (int i = 0; i < objects.size(); ++i)
    {
      Object &o = objects[i];
      if (o.topLodObjId >= 0 && o.topLodObjId != o.orgObjId)
        continue;
      topObjInd.push_back(i);

      if (o.rsceneParams & (RenderScene::RenderObject::ROP_FADE | RenderScene::RenderObject::ROP_FADENULL))
        fadeObjIdx.push_back(topObjInd.size() - 1);
    }

    pbar.setDone(0);
    pbar.setTotal(1 + topObjInd.size() * 2 + objects.size() + ltmap_count);

    //--- struct SceneHdr
    cwr.writeFourCC(_MAKE4C('scn2'));         // int version;
    cwr.writeInt32e(0x20130524);              // int rev;
    matSaver.writeMatVdataHdr(cwr, texSaver); // int texNum, matNum, vdataNum, mvhdrSz;
    cwr.writeInt32e(ltmap_count);             // int ltmapNum;

    // int ltmapFormat;
    if (!ltmap_count && scene_format != SCENE_FORMAT_SunBump)
      cwr.writeInt32e(0);
    else if (scene_format == SCENE_FORMAT_LdrTga)
      cwr.writeFourCC(_MAKE4C('TGA'));
    else if (scene_format == SCENE_FORMAT_LdrDds || scene_format == SCENE_FORMAT_AO_DXT1)
      cwr.writeFourCC(_MAKE4C('DDS'));
    else if (scene_format == SCENE_FORMAT_LdrTgaDds)
      cwr.writeFourCC(_MAKE4C('DUAL'));
    else if (scene_format == SCENE_FORMAT_SunBump)
      cwr.writeFourCC(_MAKE4C('SB2'));
    else
      cwr.writeFourCC(_MAKE4C('?'));

    cwr.writeReal(1.f / tonemapper.postScaleMul); // float lightmapScale;
    cwr.writeReal(1);                             // float directionalLightmapScale;

    cwr.writeInt32e(topObjInd.size());  // int objNum;
    cwr.writeInt32e(fadeObjIdx.size()); // int fadeObjNum;
    cwr.writeInt32e(0);                 // int ltobjNum;
    objSzpos = cwr.tell();
    cwr.writeInt32e(0); // int objDataSize;
    if (scene_format == SCENE_FORMAT_SunBump)
    {
      cwr.writeReal(lighting.getSunColor().r);
      cwr.writeReal(lighting.getSunColor().g);
      cwr.writeReal(lighting.getSunColor().b);
      cwr.writeReal(lighting.getSunDirection().x);
      cwr.writeReal(lighting.getSunDirection().y);
      cwr.writeReal(lighting.getSunDirection().z);
    }
    cwr.beginBlock(); // int restDataSize;
    //--------

    // save texture indices
    for (int i = 0; i < texSaver.textures.size(); ++i)
      cwr.writeInt32e(i);

    // reserve space for lightmaps catalog
    SmallTab<int, TmpmemAlloc> lm_catalog;
    if (scene_format == SCENE_FORMAT_LdrTgaDds)
      clear_and_resize(lm_catalog, ltmap_count * 2);
    else
      clear_and_resize(lm_catalog, ltmap_count);
    mem_set_ff(lm_catalog);

    lmCatPos = cwr.tell();
    cwr.writeTabData32ex(lm_catalog);

    // save materials and vertex data
    matSaver.writeMatVdata(cwr, texSaver);
    pbar.incDone();

    // save objects
    {
      mkbindump::BinDumpSaveCB cwr_o(8 << 20, target_code, write_be);
      SmallTab<ObjectTmpSaveData, TmpmemAlloc> obj_tmp;
      Tab<ShaderMDRec> obj_mesh_data(tmpmem);
      Tab<ObjectLodIndex> lods(tmpmem);
      clear_and_resize(obj_tmp, topObjInd.size());
      obj_mesh_data.reserve(objects.size());

      cwr_o.writeInt32e(0);
      cwr_o.writeInt32e(topObjInd.size());

      // first pass - write names & lods
      for (int ti = 0; ti < topObjInd.size(); ++ti)
      {
        pbar.incDone();

        Object &to = objects[topObjInd[ti]];
        ObjectTmpSaveData &tmp = obj_tmp[ti];
        if (to.name.length())
        {
          tmp.nameOfs = cwr_o.tell();
          cwr_o.writeRaw(to.name.str(), to.name.length() + 1);
        }
        else
          tmp.nameOfs = -1;
      }
      cwr_o.align8();
      for (int ti = 0; ti < topObjInd.size(); ++ti)
      {
        pbar.incDone();

        Object &to = objects[topObjInd[ti]];
        ObjectTmpSaveData &tmp = obj_tmp[ti];

        lods.resize(1);
        lods[0] = ObjectLodIndex(topObjInd[ti], 0.0);

        if (to.topLodObjId >= 0)
        {
          for (int i = 0; i < objects.size(); ++i)
          {
            if (i == topObjInd[ti])
              continue;

            Object &o = objects[i];
            if (o.topLodObjId != to.topLodObjId)
              continue;

            lods.push_back(ObjectLodIndex(i, o.lodRange));
          }
        }

        // sort lods
        if (lods.size() > 2)
          sort(lods, 1, lods.size() - 1, &ObjectLodIndex::compare);

        tmp.lodsRef.start = cwr_o.tell();
        tmp.lodsRef.count = lods.size();

        for (int i = 0; i < lods.size(); ++i)
        {
          Object &o = objects[lods[i].objIndex];
          obj_mesh_data.push_back(ShaderMDRec(o.meshData, cwr_o.tell()));

          // RenderScene::RenderObject::Lod
          cwr_o.writePtr64e(0);
          cwr_o.writeReal(lods[i].lodRange * lods[i].lodRange);
          cwr_o.writeInt32e(0);
        }
      }

      // second pass - write object data
      cwr_o.writeInt32eAt(cwr_o.tell(), 0);
      for (int ti = 0; ti < topObjInd.size(); ++ti)
      {
        pbar.incDone();

        Object &to = objects[topObjInd[ti]];
        ObjectTmpSaveData &tmp = obj_tmp[ti];

        // RenderScene::RenderObject
        cwr_o.writePtr64e(0);                       // PatchablePtr<PVobject> visobj;
        cwr_o.writePtr64e(0);                       // PatchablePtr<IRenderWrapperControl> rctrl;
        cwr_o.writeRef(tmp.lodsRef);                // PatchableTab<ObjectLod> lods;
        cwr_o.write32ex(&to.bsph, sizeof(to.bsph)); // BSphere3 bsph;
        cwr_o.writeReal(to.visRange);               // real visrange;
        cwr_o.write32ex(&to.bbox, sizeof(to.bbox)); // BBox3 bbox;
        cwr_o.writeReal(to.maxSubRad);              // float maxSubobjRad;
        cwr_o.writeInt16e(to.flags);                // uint16_t flags;
        cwr_o.writeInt16e(to.lightingObjectDataNo); // uint16_t lightingObjectDataNo;
        cwr_o.writeInt8e(0);                        // uint8_t curLod;
        cwr_o.writeInt8e(to.transSortPriority);     // uint8_t priority;
        cwr_o.writeInt8e(to.rsceneParams);          // uint8_t params;
        cwr_o.writeInt8e(0);                        // uint8_t _resv;
        cwr_o.writeReal(to.lodNearVr2());           // real lodNearVisRangeSq;
        cwr_o.writeReal(to.lodFarVr2());            // real lodFarVisRangeSq;
        cwr_o.writeReal(0);                         // real mixNextLod;
        cwr_o.writePtr64e(tmp.nameOfs);             // PatchablePtr<const char> name;
        if (to.maxSubRad < to.bsph.r)
          debug("robj[%d]: bsph.r=%.3f maxSubRad=%.3f", ti, to.bsph.r, to.maxSubRad);
      }
      cwr_o.align16();
      obj_sz = cwr_o.tell();

      // third pass - write mesh data
      for (int i = 0; i < obj_mesh_data.size(); ++i)
      {
        pbar.incDone();
        ShaderMDRec &tmp = obj_mesh_data[i];
        cwr_o.writeInt32eAt(tmp.mesh->save(cwr_o, matSaver, true), tmp.szPos);
      }

      // save fade obj indices
      cwr_o.writeTabData32ex(fadeObjIdx);

      ZlibSaveCB z_cwr(cwr.getRawWriter(), ZlibSaveCB::CL_BestCompression);
      cwr_o.copyDataTo(z_cwr);
      z_cwr.finish();
    }
    cwr.writeInt32eAt(obj_sz, objSzpos);

    /*if (buildShadowSD)
      prebuildAndSaveSpacialDataForShadows(cwr, matSaver);
    else*/
    {
      cwr.writeInt32e(0x20070401);
      cwr.writeInt32e(0);
    }

    for (int i = 0; i < ltmap_count; ++i)
    {
      pbar.incDone();
      bool make_ref = ltmap_ref_base != NULL;
      String ref_ltmap_name(260, "%s/ref_lm%04d", make_ref ? ltmap_ref_base : "", i);

      if (scene_format == SCENE_FORMAT_LdrTga || scene_format == SCENE_FORMAT_AO_DXT1 || scene_format == SCENE_FORMAT_LdrDds)
      {
        TexImage32 *img = lighting.getLightmapImage(i);
        lm_catalog[i] = save_lightmap_files(ref_ltmap_name, img, scene_format, target_code, cwr, make_ref);
      }
      else if (scene_format == SCENE_FORMAT_LdrTgaDds)
      {
        TexImage32 *img = lighting.getLightmapImage(i);
        lm_catalog[i * 2] = cwr.tell();
        lm_catalog[i * 2 + 1] = save_lightmap_files(ref_ltmap_name, img, scene_format, target_code, cwr, make_ref);

        // handle this differently
        lm_catalog[i * 2] -= lmCatPos;
        lm_catalog[i * 2 + 1] -= lmCatPos;
        continue;
      }
      else if (scene_format == SCENE_FORMAT_SunBump)
      {
        TexImage32 *flatImg = lighting.getLightmapImage(i);
        TexImage32 *sunBumpImageImg = lighting.getLightmapImageSunBump(i);
        lm_catalog[i] = cwr.tell();
        save_lightmap_bump(ref_ltmap_name, flatImg, sunBumpImageImg, cwr, make_ref);
      }

      lm_catalog[i] -= lmCatPos;
    }

    cwr.seekto(lmCatPos);
    cwr.writeTabData32ex(lm_catalog);
    cwr.seekto(cwr.getSize());
    cwr.endBlock();
  }
  catch (IGenSave::SaveException)
  {
    dd_erase(scene_fname);
    return;
  }

  FullFileSaveCB fcwr(scene_fname);
  if (fcwr.fileHandle)
  {
    fcwr.writeInt(_MAKE4C('scnf'));
    fcwr.beginBlock();

    texSaver.prepareTexNames({});
    fcwr.writeInt(texSaver.texNames.size());
    for (int i = 0; i < texSaver.texNames.size(); ++i)
    {
      String reduced_tfn = texSaver.texNames[i];

      simplify_fname(reduced_tfn);

      fcwr.writeIntP<1>(reduced_tfn.length());
      fcwr.write(&reduced_tfn[0], reduced_tfn.length() & 0xFF);
    }

    while (fcwr.tell() & 15)
      fcwr.writeIntP<1>(0);
    fcwr.endBlock();

    cwr.copyDataTo(fcwr);
  }
}


/*
void StaticSceneBuilder::StaticVisualScene::prebuildAndSaveSpacialDataForShadows(
  mkbindump::BinDumpSaveCB &cwr, ShaderMeshDataSaveCB &mdcb)
{
  // Get geometry bbox.

  BBox3 sceneBBox;

  sceneBBox.setempty();
  for (unsigned int objectNo = 0; objectNo < objects.size(); objectNo++)
  {
    Object &object = objects[objectNo];
    ShaderMeshData *mesh = object.meshData;
    sceneBBox += objects[objectNo].bbox;
  }



  // Create leaves.

  static BBoxTreeLeaf *leafArray[BBOX_TREE_LEAVES_X][BBOX_TREE_LEAVES_Y][BBOX_TREE_LEAVES_Z];
  memset(leafArray, 0, sizeof(leafArray));

  int atestVar = get_shader_variable_id("atest");

  for (unsigned int objectNo = 0; objectNo < objects.size(); objectNo++)
  {
//debug( "obj[%u] \"%s\"", objectNo, obj[objectNo].name );
    Object &object = objects[objectNo];

    // skip sub-LODs
    if (object.topLodObjId>=0 && object.topLodObjId!=object.orgObjId) continue;

    ShaderMeshData *mesh = object.meshData;
    for (unsigned int elemNo = 0; elemNo < mesh->elem.size(); elemNo++)
    {
      ShaderMeshData::RElem &faceRenderElement = mesh->elem[elemNo];
      //if (!faceRenderElement.e)
      //  continue;


      // Get flags.

      int atestValue;
      bool res = faceRenderElement.mat->getIntVariable(atestVar, atestValue);
      unsigned int flags = (res && atestValue != 0) ? BBoxTreeRenderElement::FLAG_ATEST : 0;


      // Get vertices.

      unsigned int vertexStart = faceRenderElement.sv;
      unsigned int vertexNum = faceRenderElement.numv;
      unsigned int stride = faceRenderElement.vertexData->stride;
      //VDECL vertexDeclaration = faceRenderElement.vertexData->getVDecl();

      uint8_t *vertexArray = faceRenderElement.vertexData->vData.data();


      // Get indices.

      unsigned int indexNum = 3 * faceRenderElement.numf;

      unsigned int *indexArray32 = NULL;
      unsigned short int *indexArray16 = NULL;
      if (!faceRenderElement.vertexData->iData.empty())
        indexArray16 = &faceRenderElement.vertexData->iData[0];
      else if (!faceRenderElement.vertexData->iData32.empty())
        indexArray32 = (unsigned int *)&faceRenderElement.vertexData->iData32[0];
      else
        continue;


      // Iterate over faces.

      for (unsigned int faceStartIndex = 0; faceStartIndex < indexNum; faceStartIndex += 3)
      {
        Point3 corner0, corner1, corner2;

        if (indexArray32)
        {
          corner0 = *((Point3*)(vertexArray + (indexArray32[faceRenderElement.si + faceStartIndex + 0]) * stride));
          corner1 = *((Point3*)(vertexArray + (indexArray32[faceRenderElement.si + faceStartIndex + 1]) * stride));
          corner2 = *((Point3*)(vertexArray + (indexArray32[faceRenderElement.si + faceStartIndex + 2]) * stride));
        }
        else
        {
          corner0 = *((Point3*)(vertexArray + (indexArray16[faceRenderElement.si + faceStartIndex + 0]) * stride));
          corner1 = *((Point3*)(vertexArray + (indexArray16[faceRenderElement.si + faceStartIndex + 1]) * stride));
          corner2 = *((Point3*)(vertexArray + (indexArray16[faceRenderElement.si + faceStartIndex + 2]) * stride));
        }

        int x = (int)floorf((corner0.x - sceneBBox.lim[0].x) / sceneBBox.width().x * BBOX_TREE_LEAVES_X);
        int y = (int)floorf((corner0.y - sceneBBox.lim[0].y) / sceneBBox.width().y * BBOX_TREE_LEAVES_Y);
        int z = (int)floorf((corner0.z - sceneBBox.lim[0].z) / sceneBBox.width().z * BBOX_TREE_LEAVES_Z);

        if ( x < 0 ) x = 0;
        else if ( x >= BBOX_TREE_LEAVES_X ) x = BBOX_TREE_LEAVES_X-1;

        if ( y < 0 ) y = 0;
        else if ( y >= BBOX_TREE_LEAVES_Y ) y = BBOX_TREE_LEAVES_Y-1;

        if ( z < 0 ) z = 0;
        else if ( z >= BBOX_TREE_LEAVES_Z ) z = BBOX_TREE_LEAVES_Z-1;

        if (!leafArray[x][y][z])
          leafArray[x][y][z] = new BBoxTreeLeaf;
        BBoxTreeLeaf* box = leafArray[x][y][z];

        BBoxTreeRenderElement *renderElement = NULL;
        for (unsigned int renderElementNo = 0; renderElementNo < box->relemList.size(); renderElementNo++)
        {
          if (box->relemList[renderElementNo].vDataSrc == faceRenderElement.vertexData
            && box->relemList[renderElementNo].flags == flags)
          {
            renderElement = &box->relemList[renderElementNo];
            break;
          }
        }

        if (!renderElement)
        {
          box->relemList.push_back(BBoxTreeRenderElement());
          renderElement = box->relemList.top();
          renderElement->vDataSrc = faceRenderElement.vertexData;
          renderElement->flags = flags;
        }

        renderElement->faceList_.push_back((faceRenderElement.si + faceStartIndex) / 3);

        *box += corner0;
        *box += corner1;
        *box += corner2;
      }
    }
  }



  Tab<BBox3*> childrenList(tmpmem);
  for (unsigned int x = 0; x < BBOX_TREE_LEAVES_X; x++)
  {
    for (unsigned int y = 0; y < BBOX_TREE_LEAVES_Y; y++)
    {
      for (unsigned int z = 0; z < BBOX_TREE_LEAVES_Z; z++)
      {
        if (leafArray[x][y][z])
          childrenList.push_back(leafArray[x][y][z]);
      }
    }
  }

  debug("prebuildAndSaveSpacialDataForShadows: childrenList.size=%d", childrenList.size());

  cwr.writeInt32e(0x20070401);

  if (childrenList.empty())
  {
    cwr.writeInt32e(0);
    return;
  }


  // Create bbox hierarchy.

  Tab<BBox3*> nodeList(tmpmem);

  unsigned int bboxTreeDepth = 0;
  for (;;)
  {
    bboxTreeDepth++;

    for (unsigned int currentChild = 0; currentChild < childrenList.size(); currentChild++)
    {
      if (!childrenList[currentChild])
        continue;

      BBoxTreeNode *node = new BBoxTreeNode;


      // First node child.

      node->childArray_[0] = childrenList[currentChild];
      childrenList[currentChild] = NULL;
      *node += *node->childArray_[0];


      // Find another BBOX_TREE_CHILDREN_PER_NODE - 1 children to be nearest to the first one.

      for (unsigned int cpi = 1; cpi < BBoxTreeNode::CHILDREN_PER_NODE; cpi++)
      {
        float minDistance = 1e10f;
        unsigned int childIndex;
        for (unsigned int childCandidateNo = currentChild + 1; childCandidateNo < childrenList.size(); childCandidateNo++)
        {
          if (!childrenList[childCandidateNo])
            continue;

          float distance = lengthSq(childrenList[childCandidateNo]->center() - node->childArray_[0]->center());
          if (distance < minDistance)
          {
            node->childArray_[cpi] = childrenList[childCandidateNo];
            childIndex = childCandidateNo;
            minDistance = distance;
          }
        }

        if (!node->childArray_[cpi])
          break;

        childrenList[childIndex] = NULL;
        *node += *node->childArray_[cpi];
      }

      nodeList.push_back(node);  // The only child may be promoted to upper level if no equal branch depth required.
    }

    if (nodeList.size() == 1)
      break;

    //childrenList.clear();
    //childrenList.swap(nodeList);

    childrenList = nodeList;
    nodeList.resize(0);
  }

  //bboxTreeRoot_ = (BBoxTreeNode*)nodeList[0];

  debug("prebuildAndSaveSpacialDataForShadows: bboxTreeDepth=%d", bboxTreeDepth);



  // Save.

  cwr.writeInt32e(bboxTreeDepth);
  saveBBoxTreeNode((BBoxTreeNode*)nodeList[0], 0, bboxTreeDepth, cwr, mdcb);

  ((BBoxTreeNode*)nodeList[0])->recursiveDelete(0, bboxTreeDepth);
  delete nodeList[0];
}



void StaticSceneBuilder::StaticVisualScene::saveBBoxTreeNode(BBoxTreeNode *node,
  unsigned int current_depth, unsigned int tree_depth, mkbindump::BinDumpSaveCB &cwr,
  ShaderMeshDataSaveCB &mdcb)
{
  cwr.write32ex((BBox3*)node, sizeof(BBox3));
  for (unsigned int childNo = 0; childNo < BBoxTreeNode::CHILDREN_PER_NODE; childNo++)
  {
    if (!node->childArray_[childNo])
    {
      cwr.writeInt32e(0);
      continue;
    }
    cwr.writeInt32e(1);

    if (current_depth < tree_depth - 1)
    {
      saveBBoxTreeNode((BBoxTreeNode*)node->childArray_[childNo], current_depth + 1, tree_depth, cwr, mdcb);
    }
    else
    {
      BBoxTreeLeaf *leaf = (BBoxTreeLeaf*)node->childArray_[childNo];
      cwr.write32ex((BBox3*)leaf, sizeof(BBox3));
      cwr.writeInt32e(leaf->relemList.size());
      for (unsigned int renderElementNo = 0; renderElementNo < leaf->relemList.size(); renderElementNo++)
      {
        cwr.writeInt32e(mdcb.getGlobVData(leaf->relemList[renderElementNo].vDataSrc));
        cwr.writeInt32e(leaf->relemList[renderElementNo].flags);
        cwr.writeInt32e(leaf->relemList[renderElementNo].faceList_.size());
        for (unsigned int faceNo = 0; faceNo < leaf->relemList[renderElementNo].faceList_.size(); faceNo++)
          cwr.writeInt32e(leaf->relemList[renderElementNo].faceList_[faceNo]);
      }
    }
  }
}
*/

bool StaticSceneBuilder::StaticVisualScene::checkLostGeom()
{
  // collect GlobalVertexDataSrc
  PtrTab<GlobalVertexDataSrc> vdata(tmpmem);

  for (int oi = 0; oi < objects.size(); ++oi)
  {
    Object &obj = objects[oi];
    ShaderMeshData *mdPtr = obj.meshData;
    if (!mdPtr)
      continue;
    ShaderMeshData &md = *mdPtr;

    for (int ei = 0; ei < md.elems.size(); ++ei)
    {
      ShaderMeshData::RElem &e = md.elems[ei];
      int i;
      for (i = 0; i < vdata.size(); ++i)
        if (vdata[i] == e.vertexData)
          break;

      if (i >= vdata.size())
        vdata.push_back(e.vertexData);
    }
  }

  // check for lost data
  bool isLost = false;

  for (int di = 0; di < vdata.size(); ++di)
  {
    GlobalVertexDataSrc *vd = vdata[di];

    int vsize = 0, isize = 0;

    for (int oi = 0; oi < objects.size(); ++oi)
    {
      Object &obj = objects[oi];
      ShaderMeshData *mdPtr = obj.meshData;
      if (!mdPtr)
        continue;
      ShaderMeshData &md = *mdPtr;

      for (int ei = 0; ei < md.elems.size(); ++ei)
      {
        ShaderMeshData::RElem &e = md.elems[ei];
        if (e.vertexData != vd)
          continue;

        vsize += e.numv * vd->stride;
        isize += e.numf * 3 * (vd->iData32.size() ? 4 : 2);
      }
    }

    vsize = data_size(vd->vData) - vsize;
    isize = data_size(vd->iData) + data_size(vd->iData32) - isize;
    if (vsize != 0 || isize != 0)
    {
      DEBUG_CTX("lost vdata=%d idata=%d", vsize, isize);
      isLost = true;
    }
  }

  return isLost;
}


void StaticSceneBuilder::StaticVisualScene::connectVertexData(IGenericProgressIndicator &pbar, bool optimize_for_vertex_cache)
{
  // connect vertex data with same structure
  pbar.setActionDesc("Building scene: connecting vertex data");
  GlobalVertexDataConnector gvd;
  for (int i = 0; i < objects.size(); ++i)
    if (objects[i].meshData)
      gvd.addMeshData(objects[i].meshData, objects[i].bsph.c, objects[i].bsph.r);

  class UpdateInfo : public GlobalVertexDataConnector::UpdateInfoInterface
  {
  public:
    IGenericProgressIndicator &pbar;

    UpdateInfo(IGenericProgressIndicator &w) : pbar(w) {}

    void beginConnect(int total_count) override
    {
      pbar.setDone(0);
      pbar.setTotal(total_count);
    }

    void update(int processed_count, int total_count) override { pbar.setDone(processed_count); }

    void endConnect(int before_count, int after_count) override {}
  };

  UpdateInfo uinfo(pbar);

  gvd.connectData(false, &uinfo);

  // vdata = gvd.getResult();

  // optimize for vertex cache
  if (optimize_for_vertex_cache)
  {
    pbar.setActionDesc("Building scene: optimizing for vertex cache");
    pbar.setDone(0);
    pbar.setTotal(objects.size());
    for (int i = 0; i < objects.size(); ++i)
    {
      pbar.setTotal(i);
      objects[i].meshData->optimizeForCache();
    }
  }
}


bool StaticSceneBuilder::build_and_save_scene1(LightmappedScene &scene, LightingProvider &lighting, StdTonemapper &tonemapper,
  Splitter &splitter, const char *foldername, int scene_format, unsigned target_code, IGenericProgressIndicator &pbar, ILogWriter &log,
  bool should_make_reference)
{
  StaticVisualScene *staticScene = new StaticVisualScene;
  bool res = staticScene->buildAndSave1(scene, lighting, tonemapper, splitter, foldername, scene_format, target_code, pbar, log,
    should_make_reference);
  del_it(staticScene);

  return res;
}
bool StaticSceneBuilder::build_and_save_scene(LightmappedScene &scene, LightingProvider &lighting, StdTonemapper &tonemapper,
  Splitter &splitter, const char *foldername, int scene_format, dag::Span<unsigned> add_target_code, IGenericProgressIndicator &pbar,
  ILogWriter &log, bool should_make_reference)
{
  StaticVisualScene *staticScene = new StaticVisualScene;
  bool res = staticScene->buildAndSave(scene, lighting, tonemapper, splitter, foldername, scene_format, add_target_code, pbar, log,
    should_make_reference);
  del_it(staticScene);

  return res;
}

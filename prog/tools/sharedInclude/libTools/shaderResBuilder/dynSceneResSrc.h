// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DAGOR_DYNSCENERESSRC_H
#define _GAIJIN_DAGOR_DYNSCENERESSRC_H
#pragma once

#include <libTools/shaderResBuilder/meshDataSave.h>
#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <libTools/shaderResBuilder/shSkinMeshData.h>
#include <libTools/shaderResBuilder/processMat.h>
#include <libTools/shaderResBuilder/validateLods.h>
#include <3d/dag_materialData.h>
#include <math/dag_bounds3.h>
#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <util/dag_oaHashNameMap.h>
#include <osApiWrappers/dag_direct.h>
#include <EASTL/functional.h>
#include <EASTL/string.h>

class LodsEqualMaterialGather;
class AScene;
class Node;
class Mesh;
class MeshBones;
class MaterialDataList;
class Bitarray;
class DataBlock;
class ILogWriter;


class DynamicRenderableSceneLodsResSrc
{
public:
  ShaderTexturesSaver texSaver;
  ShaderMaterialsSaver matSaver;
  Tab<ShaderMaterialsSaver> matSaverSkins;
  NameMap nodeNameMap;
  int drsHdrAdd = 0;

  BBox3 bbox;
  BBox3 lbbox;
  Tab<Point3> *wAllVert = nullptr;
  BSphere3 bsph;
  bool boundingPackUsed = false;

  static bool optimizeForCache;
  static int limitBonePerVertex;
  static int setBonePerVertex;
  static bool (*process_scene)(const char *fn, int lod_n, AScene &sc, float start_range, const DataBlock &props);
  static DataBlock *buildResultsBlk;
  static bool sepMatToBuildResultsBlk;

  struct RigidObj
  {
    int nodeId = -1, szPos = 0;
    int meshOfs = 0;
    ShaderMeshData meshData;
    BSphere3 sph;

    RigidObj() {}

    void presave(ShaderMeshDataSaveCB &);
  };

  struct SkinnedObj
  {
    Tab<ShaderSkinnedMeshData *> meshData;
    int nodeId = -1;
    String nodeName;

    SkinnedObj() : meshData(tmpmem) {}

    ~SkinnedObj();

    void addSkinVariant(ShaderSkinnedMeshData *new_data) { meshData.push_back(new_data); }

    void presave(int var, ShaderMeshDataSaveCB &);
  };

  struct Lod
  {
    real range = 0;
    int szPos = 0;
    float texScale = 0;
    Tab<RigidObj> rigids;
    Tab<SkinnedObj> skins;

    String fileName;

    Lod() : rigids(tmpmem), skins(tmpmem) {}

    int addRigid(int node_id);

    int addSkin(const char *node_name);

    int saveRigids(mkbindump::BinDumpSaveCB &, ShaderMaterialsSaver &);
    void saveSkins(mkbindump::BinDumpSaveCB &, int var, ShaderMaterialsSaver &);

    unsigned countFaces() const
    {
      unsigned faceCount = 0;
      for (const auto &r : rigids)
        faceCount += r.meshData.countFaces();
      for (const auto &s : skins)
        for (const auto *d : s.meshData)
          if (d)
            faceCount += d->countFaces();
      return faceCount;
    }
  };

  Tab<Lod> lods;
  ILogWriter *log = nullptr;


  DynamicRenderableSceneLodsResSrc(IProcessMaterialData *pm = nullptr);

  void addNode(Lod &, Node *, LodsEqualMaterialGather &mat_gather);

  bool addLod(const char *filename, real range, LodsEqualMaterialGather &mat_gather, Tab<AScene *> &scene_list, bool all_animated,
    const DataBlock &props, bool need_reset_nodes_tm_scale, const DataBlock &material_overrides);

  bool build(const DataBlock &blk);
  bool save(mkbindump::BinDumpSaveCB &cwr, const LodValidationSettings *lvs, ILogWriter &log);

  static void setupMatSubst(const DataBlock &blk);
  MaterialData *processMaterial(MaterialData *m, bool need_tex = true)
  {
    return processMat ? processMat->processMaterial(m, need_tex) : m;
  }

protected:
  Tab<String> textureSlots;
  IProcessMaterialData *processMat;
  bool useSymTex = false;
  bool forceLeftSideMatrices = false;
  bool doNotSplitLods = false;

  void presave();

private:
  void calcMeshNodeBbox(Node *n);
  void calcSkinNodeBbox(Node *n);

  // add mesh node, if mesh found
  void addMeshNode(Lod &lod, Node *n, LodsEqualMaterialGather &mat_gather);

  // add skin node, if skin found
  void addSkinNode(Lod &lod, Node *n, LodsEqualMaterialGather &mat_gather);

  void splitRealTwoSided(Mesh &m, Bitarray &is_material_real_two_sided_array, int side_channel_id);
};

DAG_DECLARE_RELOCATABLE(DynamicRenderableSceneLodsResSrc::Lod);
DAG_DECLARE_RELOCATABLE(DynamicRenderableSceneLodsResSrc::SkinnedObj);
DAG_DECLARE_RELOCATABLE(DynamicRenderableSceneLodsResSrc::RigidObj);

#endif

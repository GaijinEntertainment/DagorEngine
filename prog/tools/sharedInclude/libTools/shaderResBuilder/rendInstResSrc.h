// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <libTools/shaderResBuilder/processMat.h>
#include <libTools/shaderResBuilder/validateLods.h>
#include <3d/dag_materialData.h>
#include <generic/dag_tab.h>
#include <math/dag_bounds3.h>
#include <math/dag_mesh.h>
#include <util/dag_bitArray.h>
#include <osApiWrappers/dag_direct.h>
#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <sceneRay/dag_sceneRayDecl.h>

class LodsEqualMaterialGather;
class AScene;
class Node;
class MaterialDataList;
class DataBlock;
class Bitarray;
class ILogWriter;

extern bool generate_quads;
extern bool generate_strips;

struct DeleteParametersFromLod
{
  int lod = 0;
  eastl::string shader_name;
  eastl::vector<eastl::string> parameters;
};

class RenderableInstanceLodsResSrc
{
public:
  ShaderTexturesSaver texSaver;
  ShaderMaterialsSaver matSaver;

  static bool optimizeForCache;
  static float sideWallAreaThresForOccluder;
  static DataBlock *buildResultsBlk;
  static bool sepMatToBuildResultsBlk;
  static const DataBlock *warnTwoSided;
  static eastl::vector<DeleteParametersFromLod> deleteParameters;

  struct RigidObj
  {
    ShaderMeshData meshData;

    RigidObj() {}

    void presave(ShaderMeshDataSaveCB &);
  };

  struct Lod
  {
    real range;
    int szPos;
    RigidObj rigid;
    BBox3 bbox;
    float bs0rad2;
    BSphere3 bsph;
    float texScale;

    String fileName;

    Lod() : bs0rad2(0), range(0), szPos(0), texScale(0) {}

    int save(mkbindump::BinDumpSaveCB &, ShaderMeshDataSaveCB &);
    unsigned countFaces() const { return rigid.meshData.countFaces(); }
  };

  Tab<Lod> lods;
  ILogWriter *log = nullptr;


  RenderableInstanceLodsResSrc(IProcessMaterialData *pm = nullptr) :
    lods(tmpmem),
    bboxFromLod(-1),
    hasImpostor(false),
    processMat(pm),
    hasOccl(true),
    explicitOccl(false),
    occlQuad(false),
    useSymTex(false),
    forceLeftSideMatrices(false),
    textureSlots(tmpmem)
  {
    occlBb.setempty();
  }

  void addNode(Lod &, Node *, Node *key_node, LodsEqualMaterialGather &mat_gather, StaticSceneRayTracer *ao_tracer);

  bool addLod(int lod_index, const char *filename, real range, LodsEqualMaterialGather &mat_gather, Tab<AScene *> &scene_list,
    const DataBlock &material_overrides, const char *add_mat_script = nullptr);

  bool build(const DataBlock &blk);
  bool save(mkbindump::BinDumpSaveCB &cwr, const LodValidationSettings *lvs, ILogWriter &log);

  static void setupMatSubst(const DataBlock &blk);
  MaterialData *processMaterial(MaterialData *m, bool need_tex = true)
  {
    return processMat ? processMat->processMaterial(m, need_tex) : m;
  }

protected:
  Tab<String> textureSlots;
  int bboxFromLod;
  bool useSymTex;
  bool forceLeftSideMatrices;
  bool hasImpostor;
  bool hasOccl;
  bool explicitOccl;
  BBox3 occlBb;
  Mesh occlMesh;
  bool occlQuad;
  bool doNotSplitLods = false;
  bool hasFloatingBox = false;
  BBox3 floatingBox;

  Tab<Point3> impConvexPts;
  Tab<BBox3> impShadowPts;
  float impMinY = MAX_REAL, impMaxY = MIN_REAL;
  float impCylRadSq = 0.f;
  float impMaxFacingLeavesDelta = 0, impBboxAspectRatio = 1;
  bool buildImpostorDataNow = false;
  Bitarray impTmp_vmark, impTmp_vmark_fl;
  IProcessMaterialData *processMat;

  void presave();

private:
  // add mesh node, if mesh found
  void addMeshNode(Lod &lod, Node *n, Node *key_node, LodsEqualMaterialGather &mat_gather, StaticSceneRayTracer *ao_tracer);
  void processImpostorMesh(Mesh &m, dag::ConstSpan<ShaderMaterial *> mat);

  void splitRealTwoSided(Mesh &m, Bitarray &is_material_real_two_sided_array);
};

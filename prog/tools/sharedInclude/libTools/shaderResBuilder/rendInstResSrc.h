// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <libTools/shaderResBuilder/processMat.h>
#include <libTools/shaderResBuilder/validateLods.h>
#include <3d/dag_materialData.h>
#include <generic/dag_tab.h>
#include <math/dag_bounds3.h>
#include <math/dag_mesh.h>
#include <math/integer/dag_IPoint3.h>
#include <image/dag_texPixel.h>
#include <util/dag_bitArray.h>
#include <osApiWrappers/dag_direct.h>
#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <sceneRay/dag_sceneRayDecl.h>
#include <generic/dag_carray.h>

class LodsEqualMaterialGather;
class AScene;
class Node;
class MaterialDataList;
class DataBlock;
class Bitarray;
class ILogWriter;
class DagorMatShaderSubstitute;

namespace voxelcache
{
struct CacheDump;
}

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
    int firstVoxelMip, numVoxelMips;
    int voxelHullVerts;                // number of voxel hull vertices * stride = offset of voxel data in VB
    Ptr<VoxelSurfaceSrc> voxelSurface; // ptr to struct with compressed texture blocks

    String fileName;

    Lod() : bs0rad2(0), range(0), szPos(0), texScale(0), firstVoxelMip(-1), numVoxelMips(0), voxelHullVerts(0) {}

    int save(mkbindump::BinDumpSaveCB &, ShaderMeshDataSaveCB &);
    unsigned countFaces() const { return rigid.meshData.countFaces(); }
  };

  static constexpr uint32_t MAX_VOXEL_LAYERS = 4;
  static constexpr uint32_t VOXEL_DEPTH_BITS = 14;

  struct VoxelSurfaceIndex
  {
    uint32_t depth, rgba01, norm;
    VoxelSurfaceIndex() : depth(0), rgba01(0), norm(0) {}
    VoxelSurfaceIndex(uint32_t d, uint32_t i) : depth(d), rgba01(i), norm(i) {}
  };

  struct VoxelFace
  {
    Tab<carray<VoxelSurfaceIndex, MAX_VOXEL_LAYERS>> blockIndex;
    VoxelFace() : blockIndex(tmpmem) {}
    void init(uint32_t nx, uint32_t ny) { blockIndex.resize(nx * ny); }
  };

  struct VoxelMip
  {
    Tab<uint64_t> bitmap;
    carray<VoxelFace, 6> faces;
    Tab<carray<uint8_t, 4 * 4 * 2>> rgbaBlocks;
    Tab<carray<uint8_t, 4 * 4>> normBlocks;
    IPoint3 numBlocks, rawSize;
    float voxelSize;
    Point3 boxMin;

    VoxelMip() = default;
    VoxelMip(const IPoint3 nb, float vs);

    inline uint32_t isSolid(const IPoint3 p) const
    {
      const IPoint3 b = p >> 2;
      if (uint32_t(b.x) >= uint32_t(numBlocks.x))
        return 0;
      if (uint32_t(b.y) >= uint32_t(numBlocks.y))
        return 0;
      if (uint32_t(b.z) >= uint32_t(numBlocks.z))
        return 0;
      return (bitmap[(b.y * numBlocks.z + b.z) * numBlocks.x + b.x] >> ((p.y & 3) << 4 | (p.z & 3) << 2 | (p.x & 3))) & 1;
    }
  };

  Tab<Lod> lods;
  Tab<VoxelMip> voxelMips;
  Mesh voxelHull;
  int numVoxelLayers;
  SimpleString assetName;
  ILogWriter *log = nullptr;


  RenderableInstanceLodsResSrc(IProcessMaterialData *pm = nullptr) :
    lods(tmpmem),
    voxelMips(tmpmem),
    numVoxelLayers(0),
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

  bool addLod(const char *filename, real range, LodsEqualMaterialGather &mat_gather, Tab<AScene *> &scene_list,
    const DataBlock &material_overrides, const char *add_mat_script = nullptr);

  bool build(const DataBlock &blk, voxelcache::CacheDump *cache = nullptr, const DataBlock *default_voxel_params = nullptr);
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

  bool buildVoxels(const DataBlock &blk, const DataBlock &default_voxel_params, voxelcache::CacheDump *cache,
    LodsEqualMaterialGather &mat_gather, DagorMatShaderSubstitute &mat_subst);
  bool buildVoxelMips(const DataBlock &blk, const DataBlock &default_voxel_params, voxelcache::CacheDump *cache);
  bool buildVoxelHull(const DataBlock &blk, const DataBlock &default_voxel_params);
  bool addVoxelLod(int first_mip, int num_mips, real range, LodsEqualMaterialGather &mat_gather, DagorMatShaderSubstitute &mat_subst);
};

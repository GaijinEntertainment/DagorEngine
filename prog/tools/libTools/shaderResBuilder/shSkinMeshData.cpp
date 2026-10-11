// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/shaderResBuilder/shSkinMeshData.h>
#include <libTools/util/makeBindump.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_clothSimChannels.h>
#include <shaders/dag_corSkinningChannel.h>
#include <math/dag_mesh.h>
#include <math/dag_meshBones.h>
#include <generic/dag_tabUtils.h>
#include <ioSys/dag_genIo.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include "corSkinning.h"

//************************************************************************
//* common parameters
//************************************************************************
// if defined, use Point4 instead of E3DCOLOR in bone weights
// (for use this option change shader!)
// #define HI_PRECISION

// if defined, compress channel data
#define MINIMIZE_CHANNEL_DATA

// if defined, enable separate materials by hardware maximum constant register count
#define SEPARATE_BY_MAX_VPR_CONST

//************************************************************************
//* debug parameters
//************************************************************************
// output extra debug info (supports levels 0, 1, 2; 0=off, 1=some messages; 2=all messages)
#define OUTPUT_DEBUG_LEVEL 1

// if defined, limit bones count to this number
int dynmodel_max_bones_count = 24;
#define LIMIT_MAX_BONE_COUNT dynmodel_max_bones_count

bool dynmodel_use_direct_bones_array = false;
bool dynmodel_use_direct_bones_array_combined_lods = false;
bool dynmodel_optimize_direct_bones = false;

// if defined, limit bones count to this number
// #define LIMIT_BONE_NUMBER     22

// if defined, set bone-per-face number to this value
// #define SET_BONE_PER_FACE   3

#if !defined(OUTPUT_DEBUG_LEVEL)
#define OUTPUT_DEBUG_LEVEL 0
#endif

//************************************************************************
//* skin vertex
//************************************************************************
struct SkinVert
{
  real w[MAX_SKINBONES];    // bone weights
  int16_t b[MAX_SKINBONES]; // bone indices
  int boneCount;            // used bone count
  int channelIndex;         // index in channel
};

//************************************************************************
//* material description for build
//************************************************************************
struct MaterialBuildDesc
{
  Ptr<ShaderMaterial> shMat;
  ShaderSkinnedMeshData::BoneIndexTab boneIndices;
  Tab<int> channelIndices;
  int srcMatIdx = -1;

  inline MaterialBuildDesc() : boneIndices(midmem), channelIndices(midmem), shMat(NULL) {}

  inline ~MaterialBuildDesc() { shMat = NULL; }

  // return number of added bones or 0, if not added
  inline int addBone(const SkinVert &v, bool real_add)
  {
    if (real_add)
    {
      int bCount = 0;

      for (int i = 0; i < v.boneCount; i++)
      {
        const int16_t bone = v.b[i];

        if (tabutils::getIndex(boneIndices, bone) < 0)
        {
          boneIndices.push_back(bone);
          bCount++;
        }
      }

      return bCount;
    }
    else
    {
      static Tab<int16_t> newBones(midmem_ptr());
      newBones.clear();
      newBones.reserve(4);

      for (int i = 0; i < v.boneCount; i++)
      {
        const int16_t bone = v.b[i];

        if ((tabutils::getIndex(newBones, bone) < 0) && (tabutils::getIndex(boneIndices, bone) < 0))
        {
          newBones.push_back(bone);
        }
      }

      return newBones.size();
    }
  }

  /*  // return number of added bones or 0, if not added
    inline int addBone(uint8_t bone)
    {
      int bCount = 0;
      if (tabutils::getIndex(boneIndices, bone) < 0)
      {
        boneIndices.push_back(bone);
        bCount++;
      }

      return bCount;
    }
  */
  inline int remapBone(int16_t real_bone_index) const
  {
    int boneIndex = tabutils::getIndex(boneIndices, real_bone_index);
    return boneIndex;
  }

  inline void copyToFinal(ShaderSkinnedMeshData::MaterialDesc &out_md)
  {
    out_md.boneIndices = boneIndices;
    out_md.shMat = shMat;
    out_md.srcMatIdx = srcMatIdx;
  }
};
DAG_DECLARE_RELOCATABLE(MaterialBuildDesc);


/*********************************
 *
 * class ChannelData
 *
 *********************************/
class ChannelData
{
public:
  BoneIndices indexData[MAX_SKINBONES / 4];

#if defined(HI_PRECISION)
  Point4 weightData[MAX_SKINBONES / 4];
#else
  E3DCOLOR weightData[MAX_SKINBONES / 4];
#endif

  inline ChannelData() = default;

  inline ChannelData(const ChannelData &other) { operator=(other); }
  inline ChannelData &operator=(const ChannelData &other)
  {
    for (int i = 0; i < MAX_SKINBONES / 4; i++)
    {
      indexData[i] = other.indexData[i];
      weightData[i] = other.weightData[i];
    }
    return *this;
  }

  inline bool operator==(const ChannelData &other) const
  {
    for (int i = 0; i < MAX_SKINBONES / 4; i++)
    {
      if (indexData[i] != other.indexData[i] ||
#if defined(HI_PRECISION)
          weightData[i] != other.weightData[i])
#else
          ((unsigned int)weightData[i] != (unsigned int)other.weightData[i]))
#endif
        return false;
    }
    return true;
  }

#if !defined(HI_PRECISION)
  // convert int weight
  inline void convertWeight(const SkinVert &v)
  {
    int sum = 0;
    for (int i = 0; i < MAX_SKINBONES / 4; i++)
    {
      int offset = i * 4;
      weightData[i].r = real2int((v.w[offset + 0]) * 255);
      weightData[i].g = real2int((v.w[offset + 0] + v.w[offset + 1]) * 255);
      weightData[i].b = real2int((v.w[offset + 0] + v.w[offset + 1] + v.w[offset + 2]) * 255);
      weightData[i].a = real2int((v.w[offset + 0] + v.w[offset + 1] + v.w[offset + 2] + v.w[offset + 3]) * 255);

      weightData[i].a -= weightData[i].b;
      weightData[i].b -= weightData[i].g;
      weightData[i].g -= weightData[i].r;

      sum += weightData[i].r + weightData[i].g + weightData[i].b + weightData[i].a;
    }

#if OUTPUT_DEBUG_LEVEL >= 2
    debug("%d %d %d %d %d %d %d %dsum=%d", weightData[0].r, weightData[0].g, weightData[0].b, weightData[0].a, weightData[1].r,
      weightData[1].g, weightData[1].b, weightData[1].a, sum);
#endif
  }
#endif
}; // class ChannelData


// class ShaderSkinnedMeshData
//
static int sepMatCounter = 0;


/*********************************
 *
 * class ShaderSkinnedMeshData
 *
 *********************************/
// ctor/dtor
ShaderSkinnedMeshData::ShaderSkinnedMeshData() :
  boneTab(midmem), materialDescTab(midmem), maxVPRConst(0), boneLimit(-1), boneCount(-1), use16BitBoneIndices(false)
{}


ShaderSkinnedMeshData::~ShaderSkinnedMeshData()
{

  //  debug("materialDescTab.count=%d", materialDescTab.size());
  //
  //  for (int i = 0; i < materialDescTab.size(); i++)
  //  {
  //    debug("materialDescTab[%d].ref=%d", i, materialDescTab[i].shMat->getRefCount());
  //  }
}


ShaderSkinnedMeshData *ShaderSkinnedMeshData::clone() { return new ShaderSkinnedMeshData(*this); }


// Probe the picked variant: cloth_skinning only drives an interval, so getIntVariable cannot read it.
static bool material_uses_cloth_skinning(ShaderMaterial **shmat_tab, int mat_count)
{
  class ClothChanCB : public ShaderChannelsEnumCB
  {
  public:
    bool clothIndex = false, clothWeight = false;
    void enum_shader_channel(int u, int ui, int, int, int, ChannelModifier, int) override
    {
      if (u != SCUSAGE_EXTRA)
        return;
      clothIndex |= (ui == CLOTH_SKINNING_INDEX_EXTRA_CHANNEL);
      clothWeight |= (ui == CLOTH_SKINNING_WEIGHT_EXTRA_CHANNEL);
    }
  } cb;

  for (int i = 0; i < mat_count; i++)
    if (shmat_tab[i])
    {
      int flags = 0;
      shmat_tab[i]->enum_channels(cb, flags);
    }
  return cb.clothIndex && cb.clothWeight;
}

// Probes the variant each material selects now, so set num_bones before asking about a skin channel.
static bool material_uses_extra_channel(ShaderMaterial **shmat_tab, int mat_count, int usage_index)
{
  class ExtraChanCB : public ShaderChannelsEnumCB
  {
  public:
    int usageIndex = 0;
    bool found = false;
    void enum_shader_channel(int u, int ui, int, int, int, ChannelModifier, int) override
    {
      found |= (u == SCUSAGE_EXTRA && ui == usageIndex);
    }
  } cb;

  cb.usageIndex = usage_index;
  for (int i = 0; i < mat_count; i++)
    if (shmat_tab[i])
    {
      int flags = 0;
      shmat_tab[i]->enum_channels(cb, flags);
    }
  return cb.found;
}

// constraint: must match cor_offset_decode in cor_skinning.hlsl
static Point4 encode_cor_offset(const Point3 &offset)
{
  const float maxAbs = max(max(fabsf(offset.x), fabsf(offset.y)), fabsf(offset.z));
  const int w = maxAbs > 0 ? min((int)ceilf(8 * log2f(maxAbs / 127)) + 200, 255) : 0;
  if (w < 1)
    return Point4(0, 0, 0, 0);
  const float scale = exp2f((w - 200) / 8.f);
  auto signedByte = [scale](float v) { return ((int)roundf(v / scale) & 255) / 255.f; };
  return Point4(signedByte(offset.x), signedByte(offset.y), signedByte(offset.z), w / 255.f);
}

static bool add_cor_skinning_channel(Mesh &mesh, dag::ConstSpan<SkinVert> svert, int max_skin_bones)
{
  const int id = mesh.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, COR_SKINNING_EXTRA_CHANNEL);
  if (id < 0)
    return false;

  Tab<CorSkinWeights> skin(tmpmem);
  skin.resize(svert.size());
  for (int i = 0; i < svert.size(); i++)
  {
    CorSkinWeights &d = skin[i];
    memcpy(d.bone, svert[i].b, sizeof(d.bone));
    memcpy(d.weight, svert[i].w, sizeof(d.weight));
#if !defined(HI_PRECISION)
    ChannelData quantized;
    quantized.convertWeight(svert[i]);
    d.weight[0] = quantized.weightData[0].r / 255.f;
    d.weight[1] = quantized.weightData[0].g / 255.f;
    d.weight[2] = quantized.weightData[0].b / 255.f;
    d.weight[3] = max(1.f - d.weight[0] - d.weight[1] - d.weight[2], 0.f);
#endif
    d.count = 0;
    for (int j = 0; j < max_skin_bones; j++)
      if (d.weight[j] > 0)
      {
        d.bone[d.count] = d.bone[j];
        d.weight[d.count++] = d.weight[j];
      }
  }

  // splitRealTwoSided copies would weigh their area twice
  const int sideId = mesh.find_extra_channel(SCUSAGE_EXTRA, 55);
  dag::ConstSpan<Face> faces = mesh.getFace();
  MeshData::ExtraChannel &ch = mesh.getMeshData().extra[id];
  ch.resize_verts(svert.size());
  Tab<Face> oneSided(tmpmem);
  for (int i = 0; i < faces.size(); i++)
  {
    ch.fc[i] = {faces[i].v[0], faces[i].v[1], faces[i].v[2]};
    if (sideId < 0 || mesh.getExtra(sideId).fc[i].t[0] == 0)
      oneSided.push_back(faces[i]);
  }
  dag::Span<Point4> corOffsets = make_span((Point4 *)ch.vt.data(), svert.size());
  compute_centers_of_rotation(mesh.getVert(), oneSided, skin, corOffsets);
  for (Point4 &o : corOffsets)
    o = encode_cor_offset(Point3::xyz(o));
  return true;
}

// build mesh data
bool ShaderSkinnedMeshData::build(Mesh &mesh, MeshBones &mesh_bones, ShaderMaterial **shmat_tab, int mat_count, int max_hw_vpr_const,
  NameMap *name_map, bool pack_vcolor_to_bones, int uv_err_lod, const char *uv_err_node)
{
  if (!name_map || !shmat_tab || !mat_count)
    return false;

  const bool build_cloth_skinning_channels = material_uses_cloth_skinning(shmat_tab, mat_count);
  const bool build_cloth_sim_weight_channel = material_uses_extra_channel(shmat_tab, mat_count, CLOTH_SIM_WEIGHT_EXTRA_CHANNEL);

#if OUTPUT_DEBUG_LEVEL >= 2
  debug("SkinnedMeshDataImpl::build started");
#endif
  int i;

  int max_skin_bones = boneCount > 0 ? boneCount : 4;
  G_VERIFY(max_skin_bones <= MAX_SKINBONES);
  G_VERIFY(max_skin_bones <= 4 || boneCount > 0); // for extended bones we rely on all faces using 8 as num_bones

  if (boneLimit > 0)
    debug("SkinnedMeshDataImpl::build - set boneLimit to %d", boneLimit);
  if (boneCount > 0)
    debug("SkinnedMeshDataImpl::build - set boneCount to %d", boneCount);

  // update materials on mesh
  mesh.clampMaterials(mat_count - 1);

  G_VERIFY(mesh.sort_faces_by_mat());
  mesh.optimize_tverts();

  G_VERIFY(mesh.calc_ngr());
  G_VERIFY(mesh.calc_vertnorms());
  G_VERIFY(mesh.createTangentsData(SCUSAGE_EXTRA, 100));
  G_VERIFY(mesh.createTextureSpaceData(SCUSAGE_EXTRA, 50, 51));
  create_vertex_color_data(mesh, SCUSAGE_EXTRA, 53);
  // flip normals for skinned meshes
  for (i = 0; i < mesh.getVertNorm().size(); ++i)
    ((MeshData &)mesh).vertnorm[i] = -((MeshData &)mesh).vertnorm[i];

  const int maxSupportedBoneCount = use16BitBoneIndices ? MAXBONE_16BIT : MAXBONE_8BIT;
  if (mesh_bones.boneNames.size() >= maxSupportedBoneCount)
  {
    DAG_FATAL("Too many bones! Current bone count is %d, max allowed bone count is %d", mesh_bones.boneNames.size(),
      maxSupportedBoneCount);
    return false;
  }

  boneTab.resize(mesh_bones.boneNames.size());

  for (i = 0; i < boneTab.size(); ++i)
  {
    Bone &sb = boneTab[i];
    sb.orgtm = mesh_bones.orgtm[i];

    const char *nodeName = mesh_bones.boneNames[i];

    sb.nodeId = name_map->addNameId(nodeName);
  }

  Tab<bool> bone_used(tmpmem);
  bone_used.resize(boneTab.size());
  mem_set_0(bone_used);

  Tab<SkinVert> svert(tmpmem);
  svert.resize(mesh.getVert().size());
  mem_set_0(svert);
  Tab<int> numvb(tmpmem);
  numvb.resize(max_skin_bones + 1);

  Tab<real> boneinf(tmpmem);
  boneinf = mesh_bones.boneinf;

  memset(numvb.data(), 0, numvb.size() * sizeof(int));
  int adjacentVertexBone = 0;
  int notSkinnedVerticesNum = 0;
  for (i = 0; i < svert.size(); ++i)
  {
    SkinVert &v = svert[i];
    // v.vn=0;
    real sum = 0;
    int j;
    for (j = 0; j < max_skin_bones; ++j)
    {
      real bw = 0;
      int bb = -1;
      for (int bi = 0; bi < boneTab.size(); ++bi)
      {
        real w = boneinf[i + bi * svert.size()];
        if (w > bw)
        {
          bw = w;
          bb = bi;
        }
      }
      // limit bone-per-vertex
      if ((boneLimit > 0) && (boneLimit == j))
        break;

      if (bb < 0)
        break;

#if defined(LIMIT_BONE_NUMBER)
      if (bb < LIMIT_BONE_NUMBER)
      {
        v.w[j] = bw;
        v.b[j] = bb;
        sum += bw;
      }
      else
      {
        v.w[j] = 0;
        v.b[j] = 0;
      }
#else
      v.w[j] = bw;
      v.b[j] = bb;
      sum += bw;
#endif
      if (j == 0)
        adjacentVertexBone = bb;

      boneinf[i + bb * svert.size()] = 0;
    }

    // set bone count to specified number
    v.boneCount = (boneCount > 0) ? boneCount : j;
    numvb[v.boneCount]++;
    if (j == 0)
    {
      v.w[0] = 1;
      v.b[0] = adjacentVertexBone;
      ++j;
      sum = 1;
      notSkinnedVerticesNum++;
    }
    for (; j < max_skin_bones; ++j)
    {
      v.w[j] = 0;
      v.b[j] = maxSupportedBoneCount;
    }

    if (sum != 0)
      sum = 1 / sum;
    for (j = 0; j < max_skin_bones; ++j)
    {
      if (v.b[j] == maxSupportedBoneCount)
      {
        v.b[j] = j > 0 ? v.b[j - 1] : 0;
      }
      else
      {
        v.w[j] *= sum;
      }
      bone_used[v.b[j]] = true;
    }
  }

#if OUTPUT_DEBUG_LEVEL >= 1
  debug("  %d bones total", boneTab.size());
  for (i = 0; i <= max_skin_bones; ++i)
    debug("  %d bones: %5d verts (%3d%%)", i, numvb[i], numvb[i] * 100 / svert.size());
#endif

  if (notSkinnedVerticesNum > 0)
    logwarn("  not skinned vertices in skin (%d)", notSkinnedVerticesNum);

  Tab<ShaderMaterial *> tmpMatTab(tmpmem);
  Tab<int> origMatIdx(tmpmem);

  int varNumBonesId = VariableMap::getVariableId("num_bones", true);
  G_ASSERT(VariableMap::isVariablePresent(varNumBonesId));

#define MAT_OFS 1000

  // separate materials by bone count & remap indices
  Tab<Tab<int>> faceIndex;
  faceIndex.resize(max_skin_bones);
  SmallTab<int, TmpmemAlloc> facemats;

  clear_and_resize(facemats, mesh.getFace().size());
  for (int j = 0; j < mesh.getFace().size(); j++)
  {
    facemats[j] = mesh.getFace()[j].mat;
    G_ASSERT(facemats[j] < MAT_OFS);
  }

  for (i = 0; i < mat_count; i++)
  {
    //    debug("SRC shadermaterial %d = '%s'", i, (char*)shmat_tab[i]->getShaderClassName());
    int j;

    for (j = 0; j < max_skin_bones; j++)
      clear_and_shrink(faceIndex[j]);

    // check usage flags
    dag::ConstSpan<Face> faces = mesh.getFace();
    for (j = 0; j < faces.size(); j++)
    {
      const Face &face = faces[j];
      // debug("%d face.mat=%d", j, face.mat);
      if (facemats[j] != i)
        continue;

      SkinVert &sv0 = svert[face.v[0]];
      SkinVert &sv1 = svert[face.v[1]];
      SkinVert &sv2 = svert[face.v[2]];

      int bonePerFace = sv0.boneCount;
      if (bonePerFace < sv1.boneCount)
        bonePerFace = sv1.boneCount;
      if (bonePerFace < sv2.boneCount)
        bonePerFace = sv2.boneCount;

#if defined(SET_BONE_PER_FACE)
      bonePerFace = SET_BONE_PER_FACE;
#endif

      // debug("bonePerFace = %d", bonePerFace);
      if (bonePerFace == 0)
      {
        logerr("No bones specified for some vertices!");
      }
      else
      {
        faceIndex[bonePerFace - 1].push_back(j);
      }
    }

    // separate materials & remap indices
    // bool originalUsed = false;
    for (j = 0; j < max_skin_bones; j++)
    {
      Tab<int> &curTab = faceIndex[j];
      if (curTab.empty())
        continue;
      G_ASSERT_CONTINUE(shmat_tab[i]);

      tmpMatTab.push_back(shmat_tab[i]->clone());
      origMatIdx.push_back(i);

      int newIndex = tmpMatTab.size() - 1;
      tmpMatTab[newIndex]->set_int_param(varNumBonesId, j + 1);

      if (newIndex != i)
      {
        // remap indices
        for (int k : curTab)
        {
          facemats[k] = newIndex + MAT_OFS;
        }
      }
    }
  }

  for (i = 0; i < mesh.getFace().size(); i++)
  {
    if (facemats[i] >= MAT_OFS)
    {
      facemats[i] -= MAT_OFS;
      ((MeshData &)mesh).face[i].mat = facemats[i];
    }
  }

  if (material_uses_extra_channel(tmpMatTab.data(), tmpMatTab.size(), COR_SKINNING_EXTRA_CHANNEL))
  {
    if (max_skin_bones > 4)
      logwarn("cor skinning: node '%s' has %d bones per vertex and the centers need up to 4, its offsets stay zero",
        uv_err_node ? uv_err_node : "?", max_skin_bones);
    if (!add_cor_skinning_channel(mesh, svert, max_skin_bones > 4 ? 0 : max_skin_bones))
      logerr("cor skinning for '%s': the mesh already has extra[%d]", tmpMatTab[0]->getShaderClassName(), COR_SKINNING_EXTRA_CHANNEL);
  }

  // build material desc's; separate materials, if max bone count reached

  int maxBoneCount = maxSupportedBoneCount;
  maxVPRConst = max_hw_vpr_const;
  if (!dynmodel_use_direct_bones_array)
  {
    G_ASSERT(maxVPRConst > 0);

    maxBoneCount = maxVPRConst;
    debug("max avail vpr array size: %d", maxBoneCount);

    maxBoneCount /= 3;
  }

  // maxBoneCount--;
#if defined(LIMIT_MAX_BONE_COUNT)
  if (maxBoneCount > LIMIT_MAX_BONE_COUNT)
    maxBoneCount = LIMIT_MAX_BONE_COUNT;
#endif
  if (maxBoneCount < 0)
    maxBoneCount = 0;

  Tab<MaterialBuildDesc> matTab(tmpmem);

  // maxBoneCount = 24;
  debug("max bone count=%d", maxBoneCount);

  int maxUsedBoneCount = 0;
  // for (i = tmpMatTab.size() - 1; i >= 0; i--)
  for (i = 0; i < tmpMatTab.size(); i++)
  {
    matTab.push_back(MaterialBuildDesc());
    MaterialBuildDesc *materialDesc = &matTab.back();
    materialDesc->shMat = tmpMatTab[i]->clone();
    materialDesc->srcMatIdx = origMatIdx[i];
    int curBoneCount = 0;

    if (dynmodel_use_direct_bones_array)
    {
      materialDesc->boneIndices.reserve(bone_used.size());
      for (int j = 0; j < bone_used.size(); j++)
        if (!dynmodel_optimize_direct_bones || bone_used[j])
          materialDesc->boneIndices.push_back(j);
      curBoneCount = materialDesc->boneIndices.size();
      if (curBoneCount > maxBoneCount)
      {
        logerr("direct_bones_array: curBoneCount=%d > maxBoneCount=%d, build impossible", curBoneCount, maxBoneCount);
        return false;
      }
    }

    dag::ConstSpan<Face> faces = mesh.getFace();
    for (int j = 0; j < faces.size(); j++)
    {
      const Face &face = faces[j];
      if (facemats[j] != i)
        continue;

      int newBoneCount = curBoneCount;

      int k;
      for (k = 0; k < 3; k++)
      {
        SkinVert &sv = svert[face.v[k]];
        newBoneCount += materialDesc->addBone(sv, false);
      }

      if (maxBoneCount && (newBoneCount >= maxBoneCount))
      {
        // separate current material (maximum bone count reached)
        debug("separate material %d (%s): curcount=%d maxbonecount=%d", i, (char *)tmpMatTab[i]->getShaderClassName(), curBoneCount,
          maxBoneCount);
        matTab.push_back(MaterialBuildDesc());
        materialDesc = &matTab.back();
        materialDesc->shMat = tmpMatTab[i]->clone();
        materialDesc->srcMatIdx = origMatIdx[i];

        curBoneCount = 0;
      }

      for (k = 0; k < 3; k++)
      {
        SkinVert &sv = svert[face.v[k]];
        curBoneCount += materialDesc->addBone(sv, true);
      }

      // remap material
      facemats[j] = matTab.size() - 1 + MAT_OFS;
    }
    if (curBoneCount > maxUsedBoneCount)
      maxUsedBoneCount = curBoneCount;
  }
  debug("max used bone count=%d", maxUsedBoneCount);

  for (i = 0; i < mesh.getFace().size(); i++)
  {
    if (facemats[i] >= MAT_OFS)
    {
      facemats[i] -= MAT_OFS;
      ((MeshData &)mesh).face[i].mat = facemats[i];
    }
  }


  for (i = 0; i < tmpMatTab.size(); i++)
  {
    if (tmpMatTab[i]->getRefCount())
    {
      tmpMatTab[i]->delRef();
    }
    else
    {
      delete tmpMatTab[i];
    }
  }
  clear_and_shrink(tmpMatTab);


  // update materials on mesh
  int numMat = matTab.size();
  G_ASSERT_RETURN(numMat, false);
  // for(i=0;i< mesh.face.size(); ++i) if(mesh.face[i].mat >= numMat) mesh.face[i].mat %= numMat;
  mesh.clampMaterials(numMat - 1);

  // gather channel index data
  Tab<ChannelData> dataTable(tmpmem);
  const int boneDataVecCount = max_skin_bones / 4;

  ChannelData chData{};
  for (i = 0; i < matTab.size(); i++)
  {
    MaterialBuildDesc &materialDesc = matTab[i];
    materialDesc.channelIndices.resize(svert.size());

    // build channel data for material
    dag::ConstSpan<Face> faces = mesh.getFace();
    dag::ConstSpan<TFace> cfaces = mesh.getCFace();
    dag::ConstSpan<Color4> verts = mesh.getCVert();
    for (int j = 0; j < faces.size(); j++)
    {
      const Face &face = faces[j];
      if (face.mat != i)
        continue;

      for (int k = 0; k < 3; k++)
      {
        SkinVert &v = svert[face.v[k]];

        for (int n = 0; n < boneDataVecCount; n++)
        {
          int offset = n * 4;
          if (dynmodel_use_direct_bones_array)
          {
            chData.indexData[n] = BoneIndices(materialDesc.remapBone(v.b[offset + 0]), materialDesc.remapBone(v.b[offset + 1]),
              materialDesc.remapBone(v.b[offset + 2]), materialDesc.remapBone(v.b[offset + 3]));
          }
          else
          {
            chData.indexData[n] = BoneIndices(v.b[offset + 0], v.b[offset + 1], v.b[offset + 2], v.b[offset + 3]);
          }

#if defined(HI_PRECISION)
          chData.weightData[n] = Point4(v.w[offset + 0], v.w[offset + 1], v.w[offset + 2], v.w[offset + 3]);
#endif
        }

#if !defined(HI_PRECISION)
        chData.convertWeight(v);
#endif

        // Pack vcolor for cut bodies to 4-th weight for bones.
        // We know that sum of weights is 1, so we can restore fourth weight in shader.
        if (pack_vcolor_to_bones && cfaces.size())
        {
          const TFace &cface = cfaces[j];
          const Color4 &cvert = verts[cface.t[k]];
          int packedColor = (cvert.r > 0 ? 4 : 0) + (cvert.g > 0 ? 2 : 0) + (cvert.b > 0 ? 1 : 0);
#if defined(HI_PRECISION)
          chData.weightData[boneDataVecCount - 1].a = packedColor * 0.125;
#else
          chData.weightData[boneDataVecCount - 1].a = packedColor * 32;
#endif
        }

#if defined(MINIMIZE_CHANNEL_DATA)
        int channelIndex = tabutils::getIndex(dataTable, chData);

        if (channelIndex < 0)
        {
          channelIndex = dataTable.size();
          dataTable.push_back(chData);
        }
#else
        int channelIndex = dataTable.size();
        dataTable.push_back(chData);
#endif

        materialDesc.channelIndices[face.v[k]] = channelIndex;
      }
    }
  }

#if OUTPUT_DEBUG_LEVEL >= 2
  for (i = 0; i < svert.size(); ++i)
  {
    debug("svert[%d] w[0]=%.4f b[0]=%d", i, svert[i].w[0], svert[i].b[0]);
    debug("svert[%d] w[1]=%.4f b[1]=%d", i, svert[i].w[1], svert[i].b[1]);
    debug("svert[%d] w[2]=%.4f b[2]=%d", i, svert[i].w[2], svert[i].b[2]);
    debug("svert[%d] w[3]=%.4f b[3]=%d", i, svert[i].w[3], svert[i].b[3]);
  }
#endif

  for (int iBoneDataVec = 0; iBoneDataVec < boneDataVecCount; iBoneDataVec++)
  {
    // make channels
    int indexChannelId =
      mesh.add_extra_channel(use16BitBoneIndices ? MeshData::CHT_SHORT4 : MeshData::CHT_E3DCOLOR, SCUSAGE_EXTRA, 0 + iBoneDataVec * 2);

#if defined(HI_PRECISION)
    int weightChannelId = mesh.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, 1 + iBoneDataVec * 2);
#else
    int weightChannelId = mesh.add_extra_channel(MeshData::CHT_E3DCOLOR, SCUSAGE_EXTRA, 1 + iBoneDataVec * 2);
#endif

    MeshData::ExtraChannel &boneIndexChannel = mesh.getMeshData().extra[indexChannelId];
    MeshData::ExtraChannel &boneWeightChannel = mesh.getMeshData().extra[weightChannelId];

    boneIndexChannel.resize_verts(dataTable.size());
    boneWeightChannel.resize_verts(dataTable.size());

#if defined(HI_PRECISION)
    Point4 *boneWeightPtr = (Point4 *)&boneWeightChannel.vt[0];
#else
    E3DCOLOR *boneWeightPtr = (E3DCOLOR *)&boneWeightChannel.vt[0];
#endif
    // fill channels with values
    for (i = 0; i < dataTable.size(); i++)
    {
      const ChannelData &chData = dataTable[i];
      if (use16BitBoneIndices)
      {
        BoneIndices *boneIndexPtr = (BoneIndices *)&boneIndexChannel.vt[0];
        boneIndexPtr[i] = chData.indexData[iBoneDataVec];
      }
      else
      {
        E3DCOLOR *boneIndexPtr = (E3DCOLOR *)&boneIndexChannel.vt[0];
        boneIndexPtr[i] = E3DCOLOR(chData.indexData[iBoneDataVec].x, chData.indexData[iBoneDataVec].y,
          chData.indexData[iBoneDataVec].z, chData.indexData[iBoneDataVec].w);
      }
      boneWeightPtr[i] = chData.weightData[iBoneDataVec];
    }

    // map vertices to channels
    int channelIndex;
    dag::ConstSpan<Face> faces = mesh.getFace();
    for (i = 0; i < boneIndexChannel.fc.size(); i++)
    {
      const Face &face = faces[i];
      const MaterialBuildDesc &materialDesc = matTab[face.mat];

      for (int j = 0; j < 3; j++)
      {
        channelIndex = materialDesc.channelIndices[face.v[j]];

        boneIndexChannel.fc[i].t[j] = channelIndex;
        boneWeightChannel.fc[i].t[j] = channelIndex;
      }
    }
  }

  if (build_cloth_skinning_channels)
  {
    // Face corners index the channel space (dataTable), not mesh verts. Placeholders; skinProxyLod fills them.
    auto addClothSkinningChannels = [&mesh, &matTab, &dataTable]() {
      const int indexId = mesh.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, CLOTH_SKINNING_INDEX_EXTRA_CHANNEL);
      const int weightId = mesh.add_extra_channel(MeshData::CHT_E3DCOLOR, SCUSAGE_EXTRA, CLOTH_SKINNING_WEIGHT_EXTRA_CHANNEL);
      if (indexId < 0 || weightId < 0)
        return false;

      MeshData::ExtraChannel &indexCh = mesh.getMeshData().extra[indexId];
      MeshData::ExtraChannel &weightCh = mesh.getMeshData().extra[weightId];
      indexCh.resize_verts(dataTable.size());
      weightCh.resize_verts(dataTable.size());
      mem_set_0(indexCh.vt);
      mem_set_0(weightCh.vt);

      dag::ConstSpan<Face> faces = mesh.getFace();
      for (int i = 0; i < indexCh.fc.size(); i++)
        for (int j = 0; j < 3; j++)
          indexCh.fc[i].t[j] = weightCh.fc[i].t[j] = matTab[faces[i].mat].channelIndices[faces[i].v[j]];
      return true;
    };
    if (!addClothSkinningChannels())
      logerr("cloth skinning for '%s': the mesh already has extra[%d] or extra[%d]", shmat_tab[0]->getShaderClassName(),
        (int)CLOTH_SKINNING_INDEX_EXTRA_CHANNEL, (int)CLOTH_SKINNING_WEIGHT_EXTRA_CHANNEL);
  }

  if (build_cloth_sim_weight_channel)
  {
    // Moved out of vcol, which already means wind on a cloth render material. One value per MESH VERTEX: per
    // bone-channel entry gives a rigid region one colour (MINIMIZE_CHANNEL_DATA), per corner tears at a seam.
    auto addClothSimWeightChannel = [&mesh]() -> const char * {
      const int id = mesh.add_extra_channel(MeshData::CHT_E3DCOLOR, SCUSAGE_EXTRA, CLOTH_SIM_WEIGHT_EXTRA_CHANNEL);
      if (id < 0)
        return "cannot add the cloth sim weight extra channel (already present?)";
      MeshData::ExtraChannel &ch = mesh.getMeshData().extra[id];
      const int vertCount = mesh.getVert().size();
      ch.resize_verts(vertCount);
      mem_set_0(ch.vt);
      E3DCOLOR *dst = (E3DCOLOR *)&ch.vt[0];

      dag::ConstSpan<Face> faces = mesh.getFace();
      dag::ConstSpan<Color4> cverts = mesh.getCVert();
      dag::ConstSpan<TFace> cfaces = mesh.getCFace();
      // The channel must exist once the material declares it, painted or not.
      const bool painted = !cverts.empty() && cfaces.size() == faces.size();
      auto toByte = [](float v) { return (unsigned char)clamp((int)(v * 255.0f + 0.5f), 0, 255); };
      for (int i = 0; i < ch.fc.size(); i++)
        for (int j = 0; j < 3; j++)
        {
          ch.fc[i].t[j] = faces[i].v[j];
          if (painted)
          {
            const Color4 &c = cverts[cfaces[i].t[j]];
            dst[faces[i].v[j]] = E3DCOLOR(toByte(c.r), toByte(c.g), toByte(c.b), 255);
          }
        }
      if (!painted)
        return "the sim mesh carries no painted vertex colour (R inverse mass, G max distance, B backstop), so "
               "every weight reads as zero";

      // a==255 marks a face-referenced vertex; unreferenced slots keep the zero fill and read as kinematic.
      int kinematicCount = 0, leashedCount = 0, referencedCount = 0;
      for (int i = 0; i < vertCount; i++)
      {
        if (dst[i].a != 255)
          continue;
        referencedCount++;
        kinematicCount += dst[i].r == 0 || dst[i].g == 0;
        leashedCount += dst[i].g != 255;
      }
      // Both, not either: a trouser cuff is held by leashes alone.
      if (!kinematicCount && !leashedCount)
        return "no vertex is kinematic (R or G painted 0) and none is leashed (G below 255), so nothing attaches "
               "the garment to the character";
      if (kinematicCount == referencedCount)
        logwarn("cloth sim mesh is 100%% kinematic (%d vertices): every vertex is painted R=0 or G=0, so it will "
                "rigidly follow the skinned pose and never simulate",
          referencedCount);
      return nullptr;
    };
    if (const char *err = addClothSimWeightChannel())
      logerr("cloth sim weights for '%s': %s", shmat_tab[0]->getShaderClassName(), err);
  }

  // prepare materials
  //  for (i = 0; i < matTab.size(); i++)
  //  {
  //    MaterialBuildDesc& materialDesc = matTab[i];
  //    materialDesc.channelIndices.clear();
  //    //materialDesc.vprArray.resize(materialDesc.boneIndices.size() * 3);
  //  }

  // build shader mesh
  Tab<ShaderMaterial *> shmat(tmpmem);
  for (i = 0; i < matTab.size(); i++)
    shmat.push_back(matTab[i].shMat);

  meshData.build(mesh, shmat.data(), numMat, IdentColorConvert::object, false, 128, uv_err_lod, uv_err_node);

  // convert material data
  clear_and_shrink(materialDescTab);
  materialDescTab.resize(matTab.size());
  for (i = 0; i < matTab.size(); i++)
  {
    matTab[i].copyToFinal(materialDescTab[i]);

    debug("shadermaterial %d = '%s' (boneIndexCount=%d)", i, (char *)matTab[i].shMat->getShaderClassName(),
      matTab[i].boneIndices.size());
  }

  //  for (i = 0; i < materialDescTab.size(); i++)
  //  {
  //    const ShaderSkinnedMeshData::MaterialDesc& src = materialDescTab[i];
  //
  //    debug("count=%d", src.boneIndices.size());
  //    for (int j = 0; j < src.boneIndices.size(); j++)
  //    {
  //      debug("%4.4d mapped to %4.4d", (int)j, (int)src.boneIndices[j]);
  //    }
  //  }

  //  for (i = 0; i < matTab.size(); i++)
  //  {
  //    matTab[i].shMat->setVPRArray(&matTab[i].vprArray);
  //  }
  //
  // #if defined(NO_REALTIME_BONE_UPDATE)
  //  recalcBones();
  // #endif

  // skin_trans=skin_trans || skin[o]->trans_elems();
#if OUTPUT_DEBUG_LEVEL >= 2
  debug("SkinnedMeshDataImpl::build completed OK");
#endif

  return true;
}

void ShaderSkinnedMeshData::remap(const NameMap &old_nm, NameMap &new_nm)
{
  for (int i = 0; i < boneTab.size(); ++i)
    boneTab[i].nodeId = new_nm.addNameId(old_nm.getName(boneTab[i].nodeId));
}


bool check_all_in_a_row(dag::ConstSpan<int16_t> bone_idx, int bone_cnt)
{
  if (bone_idx.size() != bone_cnt)
    return false;
  for (int i = 0; i < bone_idx.size(); i++)
    if (bone_idx[i] != i)
      return false;
  return true;
}

// save mesh data
int ShaderSkinnedMeshData::save(mkbindump::BinDumpSaveCB &cwr, ShaderMeshDataSaveCB &mdcb)
{
  int start = cwr.tell();
  mkbindump::PatchTabRef bone_pt, mtab_pt;
  SmallTab<int, TmpmemAlloc> ofs;
  int pos;

  cwr.setOrigin();

  bone_pt.reserveTab(cwr);
  mtab_pt.reserveTab(cwr);

  pos = cwr.tell();
  cwr.writePtr64e(0);
  cwr.writePtr64e(0);
  cwr.writePtr64e(0); // workData

  meshData.save(cwr, mdcb, false);

  cwr.writePtr32eAt(cwr.tell(), pos);
  for (int i = 0; i < boneTab.size(); ++i)
    cwr.writeInt16e(boneTab[i].nodeId);
  cwr.align8();

  cwr.writePtr32eAt(cwr.tell(), pos + cwr.PTR_SZ);
  for (int i = 0; i < materialDescTab.size(); ++i)
    cwr.writePtr64e(mdcb.getmatindex(materialDescTab[i].shMat));

  cwr.align8();
  bone_pt.startData(cwr, boneTab.size());
  for (int i = 0; i < boneTab.size(); ++i)
    cwr.write32ex(&boneTab[i].orgtm, sizeof(boneTab[i].orgtm));

  clear_and_resize(ofs, materialDescTab.size());
  for (int i = 0; i < materialDescTab.size(); ++i)
  {
    ofs[i] = cwr.tell();
    if (!check_all_in_a_row(materialDescTab[i].boneIndices, boneTab.size()))
      cwr.writeTabDataRaw(materialDescTab[i].boneIndices);
  }

  cwr.align8();
  mtab_pt.startData(cwr, materialDescTab.size());
  for (int i = 0; i < materialDescTab.size(); ++i)
    if (!check_all_in_a_row(materialDescTab[i].boneIndices, boneTab.size()))
      cwr.writeRef(ofs[i], materialDescTab[i].boneIndices.size());
    else
      cwr.writeRef(0, 0);

  bone_pt.finishTab(cwr);
  mtab_pt.finishTab(cwr);

  cwr.popOrigin();
  return (cwr.tell() - start) | 0x40000000; // tight format
}

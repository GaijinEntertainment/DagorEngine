// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <libTools/shaderResBuilder/shSkinMeshData.h>
#include <libTools/util/makeBindump.h>
#include <shaders/dag_shaders.h>
#include <math/dag_mesh.h>
#include <math/dag_meshBones.h>
#include <generic/dag_tabUtils.h>
#include <ioSys/dag_genIo.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>

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
  uint8_t b[MAX_SKINBONES]; // bone indices
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
        const uint8_t bone = v.b[i];

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
      static Tab<uint8_t> newBones(midmem_ptr());
      newBones.clear();
      newBones.reserve(4);

      for (int i = 0; i < v.boneCount; i++)
      {
        const uint8_t bone = v.b[i];

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
  inline int remapBone(uint8_t real_bone_index) const { return tabutils::getIndex(boneIndices, real_bone_index); }

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
  E3DCOLOR indexData;

#if defined(HI_PRECISION)
  Point4 weightData;
#else
  E3DCOLOR weightData;
#endif

  inline ChannelData() {}

  inline ChannelData(const ChannelData &other) { operator=(other); };
  inline ChannelData &operator=(const ChannelData &other)
  {
    indexData = other.indexData;
    weightData = other.weightData;
    return *this;
  }

  inline bool operator==(const ChannelData &other) const
  {
#if defined(HI_PRECISION)
    return ((unsigned int)indexData == (unsigned int)other.indexData) && (weightData == other.weightData);
#else
    return ((unsigned int)indexData == (unsigned int)other.indexData) && ((unsigned int)weightData == (unsigned int)other.weightData);
#endif
  }

#if !defined(HI_PRECISION)
  // convert int weight
  inline void convertWeight(const SkinVert &v)
  {
    weightData.r = real2int((v.w[0]) * 255);
    weightData.g = real2int((v.w[0] + v.w[1]) * 255);
    weightData.b = real2int((v.w[0] + v.w[1] + v.w[2]) * 255);
    weightData.a = real2int((v.w[0] + v.w[1] + v.w[2] + v.w[3]) * 255);

    weightData.a -= weightData.b;
    weightData.b -= weightData.g;
    weightData.g -= weightData.r;

    int summ = weightData.r + weightData.g + weightData.b + weightData.a;

#if OUTPUT_DEBUG_LEVEL >= 2
    debug("%d %d %d %d summ=%d", weightData.r, weightData.g, weightData.b, weightData.a, summ);
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
ShaderSkinnedMeshData::ShaderSkinnedMeshData() : boneTab(midmem), materialDescTab(midmem), maxVPRConst(0), boneLimit(-1), boneCount(-1)
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


// build mesh data
bool ShaderSkinnedMeshData::build(Mesh &mesh, MeshBones &mesh_bones, ShaderMaterial **shmat_tab, int mat_count, int max_hw_vpr_const,
  NameMap *name_map, bool pack_vcolor_to_bones)
{
  if (!name_map || !shmat_tab || !mat_count)
    return false;

#if OUTPUT_DEBUG_LEVEL >= 2
  debug("SkinnedMeshDataImpl::build started");
#endif
  int i;

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

  if (mesh_bones.boneNames.size() >= MAXBONE)
  {
    fatal("Too many bones!");
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
  int numvb[MAX_SKINBONES + 1];

  Tab<real> boneinf(tmpmem);
  boneinf = mesh_bones.boneinf;

  memset(numvb, 0, sizeof(numvb));
  int adjacentVertexBone = 0;
  int notSkinnedVerticesNum = 0;
  for (i = 0; i < svert.size(); ++i)
  {
    SkinVert &v = svert[i];
    // v.vn=0;
    real sum = 0;
    int j;
    for (j = 0; j < MAX_SKINBONES; ++j)
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
    for (; j < MAX_SKINBONES; ++j)
    {
      v.w[j] = 0;
      v.b[j] = MAXBONE;
    }

    if (sum != 0)
      sum = 1 / sum;
    for (j = 0; j < MAX_SKINBONES; ++j)
    {
      if (v.b[j] == MAXBONE)
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
  for (i = 0; i <= MAX_SKINBONES; ++i)
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
  Tab<int> faceIndex[MAX_SKINBONES];
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

    for (j = 0; j < MAX_SKINBONES; j++)
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
    for (j = 0; j < MAX_SKINBONES; j++)
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

  // build material desc's; separate materials, if max bone count reached

  maxVPRConst = max_hw_vpr_const;
  G_ASSERT(maxVPRConst > 0);

  int maxBoneCount = maxVPRConst;
  debug("max avail vpr array size: %d", maxBoneCount);

  maxBoneCount /= 3;
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

  ChannelData chData;
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

        chData.indexData = E3DCOLOR(materialDesc.remapBone(v.b[0]), materialDesc.remapBone(v.b[1]), materialDesc.remapBone(v.b[2]),
          materialDesc.remapBone(v.b[3]));

#if defined(HI_PRECISION)
        chData.weightData = Point4(v.w[0], v.w[1], v.w[2], v.w[3]);
#else
        chData.convertWeight(v);
#endif

        // Pack vcolor for cut bodies to 4-th weight for bones.
        // We know that sum of weights is 1, so we can restore forth weight in shader.
        if (pack_vcolor_to_bones && cfaces.size())
        {
          const TFace &cface = cfaces[j];
          const Color4 &cvert = verts[cface.t[k]];
          int packedColor = (cvert.r > 0 ? 4 : 0) + (cvert.g > 0 ? 2 : 0) + (cvert.b > 0 ? 1 : 0);
#if defined(HI_PRECISION)
          chData.weightData.a = packedColor * 0.125;
#else
          chData.weightData.a = packedColor * 32;
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

  // make channels
  int indexChannelId = mesh.add_extra_channel(MeshData::CHT_E3DCOLOR, SCUSAGE_EXTRA, 0);

#if defined(HI_PRECISION)
  int weightChannelId = mesh.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, 1);
#else
  int weightChannelId = mesh.add_extra_channel(MeshData::CHT_E3DCOLOR, SCUSAGE_EXTRA, 1);
#endif

  MeshData::ExtraChannel &boneIndexChannel = mesh.getMeshData().extra[indexChannelId];

  MeshData::ExtraChannel &boneWeightChannel = mesh.getMeshData().extra[weightChannelId];

  boneIndexChannel.resize_verts(dataTable.size());
  boneWeightChannel.resize_verts(dataTable.size());

  E3DCOLOR *boneIndexPtr = (E3DCOLOR *)&boneIndexChannel.vt[0];

#if defined(HI_PRECISION)
  Point4 *boneWeightPtr = (Point4 *)&boneWeightChannel.vt[0];
#else
  E3DCOLOR *boneWeightPtr = (E3DCOLOR *)&boneWeightChannel.vt[0];
#endif

  // fill channels with values
  for (i = 0; i < dataTable.size(); i++)
  {
    const ChannelData &chData = dataTable[i];
    boneIndexPtr[i] = chData.indexData;
    boneWeightPtr[i] = chData.weightData;
    //    boneIndexPtr[i] = E3DCOLOR(7, 0, 0, 0);
    //    boneWeightPtr[i] = E3DCOLOR(1, 0, 0, 0);
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

  meshData.build(mesh, shmat.data(), numMat, IdentColorConvert::object);

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


bool check_all_in_a_row(dag::ConstSpan<unsigned char> bone_idx, int bone_cnt)
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

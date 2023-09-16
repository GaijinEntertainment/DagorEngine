#pragma once

#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <generic/dag_tab.h>
#include <memory/dag_mem.h>
#include <math/dag_TMatrix.h>
#include <util/dag_oaHashNameMap.h>

//************************************************************************
//* forwards
//************************************************************************
class Mesh;
class MeshBones;

class ShaderMeshData;


// maximum bones-per-vertex
#define MAX_SKINBONES 4

// total maximum bone count
#define MAXBONE 255

/*********************************
 *
 * class ShaderSkinnedMeshData
 *
 *********************************/
class ShaderSkinnedMeshData
{
public:
  //************************************************************************
  //* bone description
  //************************************************************************
  struct Bone
  {
    TMatrix orgtm;
    int nodeId;
  };

  typedef Tab<Bone> BoneTab;

  typedef Tab<unsigned char> BoneIndexTab;

  //************************************************************************
  //* material description
  //************************************************************************
  struct MaterialDesc
  {
    Ptr<ShaderMaterial> shMat;
    BoneIndexTab boneIndices;
    int srcMatIdx = -1;

    MaterialDesc() : boneIndices(midmem), shMat(NULL) {}
  };

  typedef Tab<MaterialDesc> MaterialDescTab;

  // ctor/dtor
  ShaderSkinnedMeshData();
  ~ShaderSkinnedMeshData();

  ShaderSkinnedMeshData *clone();

  // build mesh data
  bool build(Mesh &mesh, MeshBones &mesh_bones, ShaderMaterial **shmat_tab, int mat_count, int max_hw_vpr_const, NameMap *name_map,
    bool pack_vcolor_to_bones);

  void remap(const NameMap &old_nm, NameMap &new_nm);

  // save mesh data
  int save(mkbindump::BinDumpSaveCB &cwr, ShaderMeshDataSaveCB &mdcb);

  // get bone tab
  inline const BoneTab &getBoneTab() const { return boneTab; }

  // get material desc tab
  inline const MaterialDescTab &getMaterialDescTab() const { return materialDescTab; }

  // return max_hw_vpr_const for this mesh data
  inline int getMaxVPRConst() const { return maxVPRConst; }

  // return shader mesh data
  inline const ShaderMeshData &getShaderMeshData() const { return meshData; }
  inline ShaderMeshData &getShaderMeshData() { return meshData; };
  unsigned countFaces() const { return meshData.countFaces(); }

  // limit bone-per-vertex count to specified value (1..4)
  inline void limitBonesPerVertex(int v)
  {
    boneLimit = v;
    boneCount = -1;
  }

  // set bone-per-vertex count to specified value (1..4)
  inline void setBonesPerVertex(int v)
  {
    boneLimit = -1;
    boneCount = v;
  }

  // add all used vertex data to CB
  inline void enumVertexData(ShaderMeshDataSaveCB &mdcb) const { meshData.enumVertexData(mdcb); }

  inline void optimizeForCache() { meshData.optimizeForCache(); }

private:
  ShaderMeshData meshData;

  BoneTab boneTab;
  MaterialDescTab materialDescTab;

  int boneLimit;
  int boneCount;

  int maxVPRConst;
};

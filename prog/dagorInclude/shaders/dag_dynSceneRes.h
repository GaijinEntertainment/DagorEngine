//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shadersRes.h>
#include <shaders/dag_IRenderWrapperControl.h>
#include <generic/dag_smallTab.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include <math/dag_vecMathCompatibility.h>
#include <util/dag_roNameMap.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <EASTL/fixed_function.h>
#include <EASTL/array.h>
#include <shaders/dag_bindposeBufferManager.h>


class Sbuffer;
class DataBlock;

namespace shglobvars
{
extern int dynamic_pos_unpack_reg; // readonly
};


class DynamicRenderableSceneInstance;


typedef void (*on_dynsceneres_beforedrawmeshes_cb)(DynamicRenderableSceneInstance &inst, int node_id);
on_dynsceneres_beforedrawmeshes_cb set_dynsceneres_beforedrawmeshes_cb(on_dynsceneres_beforedrawmeshes_cb cb);


// Name maps for dynamic renderable scene
decl_dclass_and_id(DynSceneResNameMapResource, DObject, 0x5DEEEC3F)
public:
  /// creates resource by loading from stream
  static DynSceneResNameMapResource *loadResource(IGenLoad & crd, int res_sz = -1);

public:
  RoNameMapEx node;

protected:
  // ctor/dtor
  DynSceneResNameMapResource() {}
  DynSceneResNameMapResource(const DynSceneResNameMapResource &from);

  static inline int dumpStartOfs() { return offsetof(DynSceneResNameMapResource, node); }
  void *dumpStartPtr() { return &node; }

  // patches data after resource dump loading
  void patchData();
end_dclass_decl();


// Dynamic renderable scene contains dynamic rigid and skinned objects.
// Object positions are controlled by key nodes.
decl_dclass_and_id(DynamicRenderableSceneResource, DObject, 0x7AE6FD70)
public:
  DAG_DECLARE_NEW(midmem)

  static DynamicRenderableSceneResource *loadResource(IGenLoad & crd, int srl_flags);

  static DynamicRenderableSceneResource *loadResourceInternal(IGenLoad & crd, DynSceneResNameMapResource * nm, int srl_flags,
    ShaderMatVdata &smvd, int res_sz = -1);
  void loadSkins(IGenLoad & crd, int flags, ShaderMatVdata &skin_smvd);

  void render(DynamicRenderableSceneInstance &, real opacity);
  void renderTrans(DynamicRenderableSceneInstance &, real opacity);
  void renderDistortion(DynamicRenderableSceneInstance &);

  void gatherUsedTex(TextureIdSet & tex_id_list) const;
  void gatherUsedMat(Tab<ShaderMaterial *> & mat_list) const;

  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new);

  void duplicateMaterial(TEXTUREID tex_id, Tab<ShaderMaterial *> & old_mat, Tab<ShaderMaterial *> & new_mat);
  void duplicateMat(ShaderMaterial * prev_m, Tab<ShaderMaterial *> & old_mat, Tab<ShaderMaterial *> & new_mat);

  void cloneMeshes();
  void updateShaderElems();

  void updateBindposes();
  int getBindposeBufferIndex(int bone_idx) const;

  const DynSceneResNameMapResource &getNames() const { return *names; }

  void getMeshes(Tab<ShaderMeshResource *> & meshes, Tab<int> & nodeIds, Tab<BSphere3> * bsph);

  template <typename RigidsCB, typename SkinsCB>
  void getMeshes(RigidsCB && rigids_cb, SkinsCB && skins_cb) const
  {
    for (int rigidNo = 0; rigidNo < rigids.size(); rigidNo++)
    {
      const RigidObject &rigid = rigids[rigidNo];
      rigids_cb(rigid.mesh->getMesh(), rigid.nodeId, rigid.sph_r, rigidNo);
    }

    for (int skinNo = 0; skinNo < skins.size(); skinNo++)
      skins_cb(skins[skinNo]->getMesh(), skinNodes[skinNo], skinNo);
  }

  dag::ConstSpan<PatchablePtr<ShaderSkinnedMeshResource>> getSkinMeshes() const { return skins; }
  dag::ConstSpan<PatchablePtr<ShaderSkinnedMeshResource>> getSkins() const { return skins; }
  dag::ConstSpan<int> getSkinNodes() const { return skinNodes; }
  int getSkinsCount() const { return skins.size(); }

  void findRigidNodesWithMaterials(dag::ConstSpan<const char *> material_names,
    const eastl::fixed_function<2 * sizeof(void *), void(int)> &cb) const;
  void findSkinNodesWithMaterials(dag::ConstSpan<const char *> material_names,
    const eastl::fixed_function<2 * sizeof(void *), void(int)> &cb) const;

  template <typename T>
  void findRigidNodesWithMaterials(dag::ConstSpan<const char *> material_names, const T &cb) const
  {
    auto checkRigid = [&](const RigidObject &ro) {
      for (auto &elem : ro.mesh->getMesh()->getAllElems())
        for (auto name : material_names)
          if (strcmp(elem.mat->getShaderClassName(), name) == 0)
          {
            cb(ro.nodeId);
            return;
          }
    };

    for (auto &ro : rigids)
      checkRigid(ro);
  }

  struct RigidObject;
  dag::ConstSpan<RigidObject> getRigidsConst() const { return rigids; }
  dag::Span<RigidObject> getRigids() { return make_span(rigids); }

  bool hasRigidMesh(int node_id);

  DynamicRenderableSceneResource *clone() const;

  bool traceRayRigids(DynamicRenderableSceneInstance & scene, const Point3 &from, const Point3 &dir, float &currentT, Point3 &normal,
    bool *trans = NULL, Point2 *uv = NULL);

  void acquireTexRefs();
  void releaseTexRefs();

  inline BSphere3 getBoundingSphere(const TMatrix *__restrict ptr)
  {
    BSphere3 ret;
    for (int i = 0; i < rigids.size(); i++)
    {
      const TMatrix &tm = ptr[rigids[i].nodeId];
      float scale = sqrt(max(tm.getcol(0).lengthSq(), max(tm.getcol(1).lengthSq(), tm.getcol(2).lengthSq())));
      BSphere3 sph(rigids[i].sph_c * tm, rigids[i].sph_r * scale);
      ret += sph;
    }
    return ret;
  }

  static inline int dumpStartOfs() { return offsetof(DynamicRenderableSceneResource, rigids); }
  void *dumpStartPtr() { return &rigids; }
  bool matchBetterBoneIdxAndItmForPoint(const Point3 &pos, int &bone_idx, TMatrix &bone_itm, DynamicRenderableSceneInstance &) const;
  int getBoneForNode(int node_idx) const;
  int getNodeForBone(uint32_t bone_idx) const;

  struct RigidObject
  {
    PatchablePtr<ShaderMeshResource> mesh;
    Point3 sph_c; //< in local coords: bounding sphere center
    real sph_r;   //< in local coords: bounding sphere radius (can be scaled, but only x scale is used)
    int32_t nodeId;
    uint32_t __resv;
  };

protected:
  Ptr<DynSceneResNameMapResource> names;
  int resSize, _resv0 = 0;

  PATCHABLE_32BIT_PAD32(_resv);

  Tab<int> skinNodes;
  PtrTab<BindPoseElem> bindPoseElemPtrArr;

  PatchableTab<RigidObject> rigids;
  PatchableTab<PatchablePtr<ShaderSkinnedMeshResource>> skins;


  DynamicRenderableSceneResource() {} //-V730
  DynamicRenderableSceneResource(const DynamicRenderableSceneResource &);
  ~DynamicRenderableSceneResource() { clearData(); }

  // patches data after resource dump loading
  void patchAndLoadData(IGenLoad & crd, int flags, int res_sz, ShaderMatVdata &smvd);

  // explicit destructor
  void clearData();

  RigidObject *getRigidObject(int node_id) const;

end_dclass_decl();


// Container for LODs, each LOD is DynamicRenderableSceneResource.
decl_dclass_and_id(DynamicRenderableSceneLodsResource, DObject, 0xC03E9CC2)
public:
  DAG_DECLARE_NEW(midmem)

  static DynamicRenderableSceneLodsResource *loadResource(IGenLoad & crd, int srl_flags, int res_sz = -1,
    const DataBlock *desc = nullptr);
  static DynamicRenderableSceneLodsResource *makeStubRes();

  void gatherUsedTex(TextureIdSet & tex_id_list) const;
  void gatherUsedMat(Tab<ShaderMaterial *> & mat_list) const;
  void gatherLodUsedMat(Tab<ShaderMaterial *> & mat_list, int lod = 0) const;

  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new);

  inline float getTexScale(int lod) const { return lods[lod].texScale; }

  DynamicRenderableSceneResource *getLod(real range);
  inline int getLodNo(real range)
  {
    for (int i = 0; i < lods.size(); ++i)
      if (range <= lods[i].range)
      {
        updateReqLod(i);
        uint32_t qlBestLod = getQlBestLod();
        return i > qlBestLod ? i : qlBestLod;
      }
    return -1;
  }
  inline int getLodNoForDistSq(real dist_sq)
  {
    for (int i = 0; i < lods.size(); ++i)
      if (dist_sq <= sqr(lods[i].range))
      {
        updateReqLod(i);
        uint32_t qlBestLod = getQlBestLod();
        return i > qlBestLod ? i : qlBestLod;
      }
    return -1;
  }
  inline float getMaxDist() const { return lods.size() ? lods.back().range : 0; }

  const DynSceneResNameMapResource &getNames() const { return *names; }

  DynamicRenderableSceneLodsResource *clone() const;
  void updateShaderElems();
  void updateBindposes();
  static void closeBindposeBuffer();

  inline BBox3 getLocalBoundingBox() const { return bbox; }

  bool isBoundPackUsed() const
  {
    return (interlocked_relaxed_load(packedFields) & (BOUND_PACK_USED_MASK << BOUND_PACK_USED_SHIFT)) != 0;
  }
  void acquireTexRefs()
  {
    for (int i = 0; i < lods.size(); i++)
      lods[i].scene->acquireTexRefs();
  }
  void releaseTexRefs()
  {
    for (int i = 0; i < lods.size(); i++)
      lods[i].scene->releaseTexRefs();
  }

  void addInstanceRef();
  void delInstanceRef();

  void duplicateMaterial(TEXTUREID tex_id, Tab<ShaderMaterial *> & old_mat, Tab<ShaderMaterial *> & new_mat)
  {
    for (int i = 0; i < lods.size(); i++)
      lods[i].scene->duplicateMaterial(tex_id, old_mat, new_mat);
  }
  void duplicateMat(ShaderMaterial * prev_m, Tab<ShaderMaterial *> & old_mat, Tab<ShaderMaterial *> & new_mat)
  {
    for (int i = 0; i < lods.size(); i++)
      lods[i].scene->duplicateMat(prev_m, old_mat, new_mat);
  }
  static void finalizeDuplicateMaterial(dag::Span<ShaderMaterial *> new_mat);

  static const char *getStaticClassName() { return "dynModel"; }

  uint32_t getBvhId() const { return bvhId; }
  void setBvhId(uint32_t id) const { bvhId = id; }

private:
  mutable uint32_t bvhId = 0;

public:
  struct Lod
  {
    PatchablePtr<DynamicRenderableSceneResource> scene;
    real range;
    float texScale;
    int getAllElems(Tab<dag::ConstSpan<ShaderMesh::RElem>> &out_elems) const;
  };

  static inline int dumpStartOfs() { return offsetof(DynamicRenderableSceneLodsResource, lods); }
  void *dumpStartPtr() { return &lods; }

  dag::ConstSpan<Ptr<ShaderMatVdata>> getSmvd() const { return make_span_const(&smvdR, 2); }
  void setResLoadingFlag(bool is_loading)
  {
    if (is_loading)
      interlocked_or(packedFields, RES_LD_FLG_MASK << RES_LD_FLG_SHIFT);
    else
      interlocked_and(packedFields, ~(RES_LD_FLG_MASK << RES_LD_FLG_SHIFT));
  }
  bool getResLoadingFlag() const { return (interlocked_relaxed_load(packedFields) & (RES_LD_FLG_MASK << RES_LD_FLG_SHIFT)) != 0; }
  bool areLodsSplit() const { return (smvdR && smvdR->areLodsSplit()) || (smvdS && smvdS->areLodsSplit()); }
  dag::ConstSpan<Lod> getUsedLods() const
  {
    uint32_t qlBestLod = getQlBestLod();
    return make_span_const(lods.data() + qlBestLod, lods.size() - qlBestLod);
  }
  dag::Span<Lod> getUsedLods()
  {
    uint32_t qlBestLod = getQlBestLod();
    return make_span(lods.data() + qlBestLod, lods.size() - qlBestLod);
  }
  const DynamicRenderableSceneLodsResource *getFirstOriginal() const { return originalRes ? originalRes.get() : this; }
  const DynamicRenderableSceneLodsResource *getNextClone() const { return nextClonedRes; }
  DynamicRenderableSceneLodsResource *getFirstOriginal() { return originalRes ? originalRes.get() : this; }
  DynamicRenderableSceneLodsResource *getNextClone() { return nextClonedRes; }
  static void lockClonesList();
  static void unlockClonesList();
  static void (*on_higher_lod_required)(DynamicRenderableSceneLodsResource *res, unsigned req_lod, unsigned cur_lod);

  static constexpr short int qlReqLodInitialValue = 16;
  unsigned getQlReqLod() const { return interlocked_relaxed_load(qlReqLod); }
  unsigned getQlReqLodEff() const
  {
    const uint16_t reqLod = getQlReqLod();
    const uint16_t reqLodPrev = interlocked_relaxed_load(qlReqLodPrev);
    return reqLod < reqLodPrev ? reqLod : reqLodPrev;
  }
  int getQlReqLFU() const { return interlocked_relaxed_load(qlReqLFU); }
  void updateReqLod(unsigned lod)
  {
    const int reqLFU = getQlReqLFU();
    const uint16_t reqLod = getQlReqLod();
    if (reqLFU == ::dagor_frame_no() && lod < reqLod)
    {
      setQlReqLod(lod);
      if (!originalRes && on_higher_lod_required)
      {
        uint32_t qlBestLod = getQlBestLod();
        if (lod < qlBestLod)
          on_higher_lod_required(this, lod, qlBestLod);
      }
    }
    else if (reqLFU != ::dagor_frame_no())
    {
      uint16_t newPrevLod = reqLFU + 10 > ::dagor_frame_no() ? reqLod : lod;
      interlocked_release_store(qlReqLodPrev, newPrevLod);
      setQlReqLod(lod);
      setQlReqLFU(dagor_frame_no());
      if (!originalRes && on_higher_lod_required)
      {
        uint32_t qlBestLod = getQlBestLod();
        if (lod < qlBestLod || lod < newPrevLod)
          on_higher_lod_required(this, lod, qlBestLod);
      }
    }
    if (originalRes)
      originalRes->updateReqLod(lod);
  }
  int getQlBestLod() const { return (interlocked_relaxed_load(packedFields) >> QL_BEST_LOD_SHIFT) & QL_BEST_LOD_MASK; }
  void applyQlBestLod(int lod0)
  {
    const uint32_t m = ~(QL_BEST_LOD_MASK << QL_BEST_LOD_SHIFT), v = (lod0 & QL_BEST_LOD_MASK) << QL_BEST_LOD_SHIFT;
    uint32_t storedValue = interlocked_relaxed_load(packedFields);
    while (interlocked_compare_exchange(packedFields, (storedValue & m) | v, storedValue) != storedValue)
      storedValue = interlocked_acquire_load(packedFields);
  }
  int getInstanceRefCount() const { return interlocked_acquire_load(instanceRefCount); }

  unsigned incQlReloadCnt() { return interlocked_increment(qlReloadCnt); }
  unsigned getQlReloadCnt() const { return interlocked_acquire_load(qlReloadCnt); }
  unsigned incQlDiscardCnt() { return interlocked_increment(qlDiscardCnt); }
  unsigned getQlDiscardCnt() const { return interlocked_acquire_load(qlDiscardCnt); }

protected:
  void setQlReqLod(uint16_t new_lod) { interlocked_release_store(qlReqLod, new_lod); }
  void setQlReqLFU(int frame_no) { interlocked_release_store(qlReqLFU, frame_no); }

  Ptr<DynSceneResNameMapResource> names;
  Ptr<ShaderMatVdata> smvdR, smvdS;
  enum FieldsMasks : uint32_t
  {
    RES_SIZE_SHIFT = 0x0,
    RES_SIZE_SIZE = 24,
    RES_SIZE_MASK = (1 << RES_SIZE_SIZE) - 1,
    BOUND_PACK_USED_SHIFT = RES_SIZE_SHIFT + RES_SIZE_SIZE,
    BOUND_PACK_USED_SIZE = 0x1,
    BOUND_PACK_USED_MASK = (1 << BOUND_PACK_USED_SIZE) - 1,
    QL_BEST_LOD_SHIFT = BOUND_PACK_USED_SHIFT + BOUND_PACK_USED_SIZE,
    QL_BEST_LOD_SIZE = 0x3,
    QL_BEST_LOD_MASK = (1 << QL_BEST_LOD_SIZE) - 1,
    RES_LD_FLG_SHIFT = QL_BEST_LOD_SIZE + QL_BEST_LOD_SHIFT,
    RES_LD_FLG_SIZE = 0x1,
    RES_LD_FLG_MASK = (1 << RES_LD_FLG_SIZE) - 1,
    RESERVED_SHIFT = RES_LD_FLG_SIZE + RES_LD_FLG_SHIFT,
    RESERVED_SIZE = 0x3,
    RESERVED_MASK = (1 << RESERVED_SIZE) - 1
  };
  static_assert(RESERVED_SHIFT + RESERVED_SIZE == 32,
    "Since we use one dword for the bitfield, we shouldn't have unhandled bits there.");
  uint32_t packedFields;
  int instanceRefCount;
  volatile unsigned short qlReqLod = qlReqLodInitialValue, qlReqLodPrev = qlReqLodInitialValue;
  volatile unsigned short qlReloadCnt = 0, qlDiscardCnt = 0;
  int qlReqLFU = 0;
  mutable DynamicRenderableSceneLodsResource *nextClonedRes = nullptr;
  mutable Ptr<DynamicRenderableSceneLodsResource> originalRes;
  PATCHABLE_32BIT_PAD32(_resv[3]);
  PATCHABLE_64BIT_PAD32(_resv[2]);

public:
  PatchableTab<Lod> lods;
  BBox3 bbox;
  float bpC254[4], bpC255[4]; //< available only when boundPackUsed==1 (overlaps with other data in older bindump format)

protected:
  DynamicRenderableSceneLodsResource() //-V730
  {
    packedFields = 0;
    instanceRefCount = 0;
  }
  DynamicRenderableSceneLodsResource(const DynamicRenderableSceneLodsResource &);
  ~DynamicRenderableSceneLodsResource()
  {
    G_ASSERT(getInstanceRefCount() == 0);
    clearData();
  }
  void addToCloneList(const DynamicRenderableSceneLodsResource &from);

  // patches data after resource dump loading
  int patchAndLoadData(IGenLoad & crd, int flags, int res_sz);

  void loadSkins(IGenLoad & crd, int flags, const DataBlock *desc = nullptr);

  // explicit destructor
  void clearData();

  uint32_t getResSize() const { return (interlocked_relaxed_load(packedFields) >> RES_SIZE_SHIFT) & RES_SIZE_MASK; }

  void setResSizeNonTS(uint32_t res_size)
  {
    packedFields = (packedFields & (~(RES_SIZE_MASK << RES_SIZE_SHIFT))) | res_size << RES_SIZE_SHIFT;
  }

  void setBoundPackUsed(bool used)
  {
    if (used)
      interlocked_or(packedFields, BOUND_PACK_USED_MASK << BOUND_PACK_USED_SHIFT);
    else
      interlocked_and(packedFields, ~(BOUND_PACK_USED_MASK << BOUND_PACK_USED_SHIFT));
  }

end_dclass_decl();


// Instance of dynamic renderable scene that contains key node transforms
// for this instance.
// Switches LODs by distance from the viewer in beforeRender().
class DynamicRenderableSceneInstance
{
public:
  DAG_DECLARE_NEW(midmem)

  using NodeCollapserBits = eastl::array<uint32_t, 4 * 2>;

  static float lodDistanceScale;

  DynamicRenderableSceneInstance(DynamicRenderableSceneLodsResource *res, bool activate_instance = true);

  ~DynamicRenderableSceneInstance();

  void activateInstance(bool a = true);
  inline void deactivateInstance() { activateInstance(false); }

  DynamicRenderableSceneResource *getCurSceneResource() { return sceneResource; }
  const DynamicRenderableSceneResource *getCurSceneResource() const { return sceneResource; }

  DynamicRenderableSceneLodsResource *getLodsResource() { return lods; }
  DynamicRenderableSceneLodsResource *cloneLodsResource() const { return lods ? lods->clone() : nullptr; }
  void changeLodsResource(DynamicRenderableSceneLodsResource *new_lods);

  const DynamicRenderableSceneLodsResource *getConstLodsResource() const { return lods; }
  const DynamicRenderableSceneLodsResource *getLodsResource() const { return lods; }
  int getLodsCount() const { return lods->lods.size(); }

  int getNodeId(const char *name) const;

  inline uint32_t getNodeCount() const { return count; }

  void setNodeOpacity(uint32_t n_id, real value)
  {
    if (n_id < getNodeCount())
      opacity()[n_id] = value;
  }
  void showNode(uint32_t n_id, bool show)
  {
    if (n_id < getNodeCount())
      hidden()[n_id] = !show;
  }

  void showSkinnedNodesConnectedToBone(int bone_id, bool need_show);

  void clearNodeCollapser() { nodeCollapserBits.fill(0); }

  void markNodeCollapserNode(uint32_t node_index)
  {
    int boneId = getBoneForNode(node_index);
    G_ASSERT_RETURN(boneId < 256, );
    if (boneId >= 0)
      nodeCollapserBits[boneId / 32] |= 1 << (boneId % 32);
  }

  const NodeCollapserBits &getNodeCollapserBits() const { return nodeCollapserBits; }

  void setNodeWtm(uint32_t n_id, const TMatrix &wtm)
  {
    if (n_id < getNodeCount())
    {
      nodeWtm()[n_id] = wtm;
      wtmToOriginVec()[n_id] = wtm.getcol(3) - origin;
    }
  }

  inline float getDistSq() const { return distSq; }

  void savePrevNodeWtm()
  {
    int tmp = curWtm;
    curWtm = prevWtm;
    prevWtm = tmp;

    tmp = curOrig;
    curOrig = prevOrig;
    prevOrig = tmp;

    originPrev = origin;
  }

  const TMatrix &getPrevNodeWtm(int n_id) const { return nodePrevWtm()[n_id]; }

  void setPrevNodeWtm(int n_id, const TMatrix &wtm)
  {
    if (n_id < getNodeCount())
    {
      TMatrix &m = nodePrevWtm_ptr()[n_id];              //-V758
      Point3 &toOrigin = wtmToOriginVecPrev_ptr()[n_id]; //-V758
      m = wtm;
      toOrigin = m.getcol(3) - originPrev;
    }
  }

  void setNodeWtm(uint32_t n_id, const mat44f &wtm)
  {
    if (n_id < getNodeCount())
    {
      TMatrix &m = nodeWtm()[n_id];              //-V758
      Point3 &toOrigin = wtmToOriginVec()[n_id]; //-V758
      v_mat_43ca_from_mat44(m.m[0], wtm);
      toOrigin = m.getcol(3) - origin;
    }
  }


  // Relative to origin.

  void setOrigin(const Point3 &in_origin) { origin = in_origin; }
  const Point3 &getOrigin() const { return origin; }
  void setOriginPrev(const Point3 &in_origin_prev) { originPrev = in_origin_prev; }
  const Point3 &getOriginPrev() const { return originPrev; }

  void setNodeWtmRelToOrigin(uint32_t n_id, const TMatrix &wtm_rel_to_origin)
  {
    if (n_id < getNodeCount())
    {
      nodeWtm()[n_id] = wtm_rel_to_origin;
      wtmToOriginVec()[n_id] = wtm_rel_to_origin.getcol(3);
      nodeWtm()[n_id].setcol(3, wtm_rel_to_origin.getcol(3) + origin);
    }
  }

  void setNodeWtmRelToOrigin(uint32_t n_id, const mat44f &wtm_rel_to_origin)
  {
    if (n_id < getNodeCount())
    {
      TMatrix &m = nodeWtm()[n_id];              //-V758
      Point3 &toOrigin = wtmToOriginVec()[n_id]; //-V758
      v_mat_43ca_from_mat44(m.m[0], wtm_rel_to_origin);
      toOrigin = m.getcol(3);
      m.setcol(3, m.getcol(3) + origin);
    }
  }

  TMatrix getNodeWtmRelToOrigin(int n_id) const
  {
    TMatrix wtmRelToOrigin = nodeWtm()[n_id];
    wtmRelToOrigin.setcol(3, wtmToOriginVec()[n_id]);
    return wtmRelToOrigin;
  }


  TMatrix getNodePrevWtmRelToOrigin(int n_id) const
  {
    TMatrix wtmRelToOrigin = nodePrevWtm()[n_id];
    wtmRelToOrigin.setcol(3, wtmToOriginVecPrev()[n_id]);
    return wtmRelToOrigin;
  }


  void setBoundingBox(const BBox3 &in_bbox)
  {
    bbox = in_bbox;
    autoCalcBBox = false;
  }

  // Choose LOD by squared distance from camera pos, returns index of choosed LOD
  int chooseLodByDistSq(float dist_sq, float dist_mul = 1.0f);

  // Choose LOD by distance from camera pos, returns index of choosed LOD
  int chooseLod(const Point3 &camera_pos, float dist_mul = 1.0f);

  void setLod(int lod);
  inline void setCurrentLod(int lod);
  void setBestLodLimit(int lod);

  // sets disableAutoChooseLod, which prevents curLodNo changes via chooseLod()
  void setDisableAutoChooseLod(bool dis) { disableAutoChooseLod = dis; }

  inline int getCurrentLodNo() const { return curLodNo; }
  inline bool validateLod(uint32_t lod) const;

  // Switches LODs
  void beforeRender(const Point3 &view_pos);

  void render(real opacity = 1.f);
  void renderTrans(real opacity = 1.f);
  void renderDistortion();

  inline const TMatrix &getNodeWtm(uint32_t n_id) const { return nodeWtm()[n_id]; }
  inline real getNodeOpacity(uint32_t n_id) const { return opacity()[n_id]; }
  inline bool getNodeHidden(uint32_t n_id) const { return (hidden()[n_id]); }
  inline bool isNodeHidden(uint32_t n_id) const
  {
    if (hidden()[n_id])
      return true;
    unsigned *c = (unsigned *)nodeWtm()[n_id].m[0];
    uint32_t cOred = (c[0] | c[1] | c[2]);
    return cOred == 0 || cOred == 0x80000000;
  }
  inline BBox3 getLocalBoundingBox() const { return lods->getLocalBoundingBox(); }
  inline BBox3 getBoundingBox() const
  {
    if (autoCalcBBox)
    {
      // todo: replace with vectorized v_bbox3_init
      BBox3 ret, lbb = lods->getLocalBoundingBox();
      TMatrix tm = nodeWtm()[0];

      ret += Point3(lbb.lim[0].x, lbb.lim[0].y, lbb.lim[0].z) * tm;
      ret += Point3(lbb.lim[0].x, lbb.lim[0].y, lbb.lim[1].z) * tm;
      ret += Point3(lbb.lim[0].x, lbb.lim[1].y, lbb.lim[0].z) * tm;
      ret += Point3(lbb.lim[1].x, lbb.lim[1].y, lbb.lim[1].z) * tm;
      ret += Point3(lbb.lim[1].x, lbb.lim[0].y, lbb.lim[0].z) * tm;
      ret += Point3(lbb.lim[1].x, lbb.lim[1].y, lbb.lim[1].z) * tm;
      ret += Point3(lbb.lim[1].x, lbb.lim[1].y, lbb.lim[0].z) * tm;

      return ret;
    }

    return bbox;
  }
  BSphere3 getBoundingSphere() const;

  inline void autoCalcBoundingBox(bool calc) { autoCalcBBox = calc; }

  bool matchBetterBoneIdxAndItmForPoint(const Point3 &pos, int &bone_idx, TMatrix &bone_itm);
  int getBoneForNode(int node_idx) const;
  int getNodeForBone(uint32_t bone_idx) const;

  __forceinline const float *opacity_ptr() const { return (const float *)(allNodesData.get() + count * OPACITY); }
  __forceinline const bool *hidden_ptr() const { return (const bool *)(allNodesData.get() + count * HIDDEN); }

  void clipNodes(const Frustum &frustum, dag::Vector<int, framemem_allocator> &node_list);

  void findRigidNodesWithMaterials(dag::ConstSpan<const char *> material_names,
    const eastl::fixed_function<2 * sizeof(void *), void(int)> &cb) const;

  uint32_t getUniqueId() const { return uniqueId; }

protected:
  Ptr<DynamicRenderableSceneResource> sceneResource;
  Ptr<DynamicRenderableSceneLodsResource> lods;

  eastl::unique_ptr<uint8_t[]> allNodesData;
  uint32_t count = 0;
  int8_t curLodNo = 0;
  int8_t forceLod = -1; // -1 for not forcing
  int8_t bestLodLimit = -1;
  uint8_t autoCalcBBox : 1, disableAutoChooseLod : 1, instanceActive : 1, _resv : 5;

  float distSq = 0.0f;

  Point3 origin;
  Point3 originPrev;
  BBox3 bbox;

  uint32_t uniqueId;

  NodeCollapserBits nodeCollapserBits;

  enum Offsets
  {
    CUR_WTM = 0,
    PREV_WTM = CUR_WTM + sizeof(TMatrix),
    ORIGIN_VEC = PREV_WTM + sizeof(TMatrix),
    PREV_ORIGIN_VEC = ORIGIN_VEC + sizeof(Point3),
    OPACITY = PREV_ORIGIN_VEC + sizeof(Point3),
    HIDDEN = OPACITY + sizeof(float),
    ALL_DATA_SIZE = HIDDEN + sizeof(bool)
  };
  int curWtm = CUR_WTM;
  int prevWtm = PREV_WTM;
  int curOrig = ORIGIN_VEC;
  int prevOrig = PREV_ORIGIN_VEC;

  __forceinline const TMatrix *nodeWtm_ptr() const { return (const TMatrix *)(allNodesData.get() + count * curWtm); }
  __forceinline const TMatrix *nodePrevWtm_ptr() const { return (const TMatrix *)(allNodesData.get() + count * prevWtm); }
  __forceinline const Point3 *wtmToOriginVec_ptr() const { return (const Point3 *)(allNodesData.get() + count * curOrig); }
  __forceinline const Point3 *wtmToOriginVecPrev_ptr() const { return (const Point3 *)(allNodesData.get() + count * prevOrig); }

  TMatrix *nodeWtm_ptr() { return (TMatrix *)(allNodesData.get() + count * curWtm); }
  TMatrix *nodePrevWtm_ptr() { return (TMatrix *)(allNodesData.get() + count * prevWtm); }
  Point3 *wtmToOriginVec_ptr() { return (Point3 *)(allNodesData.get() + count * curOrig); }
  Point3 *wtmToOriginVecPrev_ptr() { return (Point3 *)(allNodesData.get() + count * prevOrig); }
  float *opacity_ptr() { return (float *)(allNodesData.get() + count * OPACITY); }
  bool *hidden_ptr() { return (bool *)(allNodesData.get() + count * HIDDEN); }

  dag::ConstSpan<TMatrix> nodeWtm() const { return dag::ConstSpan<TMatrix>(nodeWtm_ptr(), count); }
  dag::ConstSpan<TMatrix> nodePrevWtm() const { return dag::ConstSpan<TMatrix>(nodePrevWtm_ptr(), count); }
  dag::ConstSpan<Point3> wtmToOriginVec() const { return dag::ConstSpan<Point3>(wtmToOriginVec_ptr(), count); }
  dag::ConstSpan<Point3> wtmToOriginVecPrev() const { return dag::ConstSpan<Point3>(wtmToOriginVecPrev_ptr(), count); }
  dag::ConstSpan<float> opacity() const { return dag::ConstSpan<float>(opacity_ptr(), count); }
  dag::ConstSpan<bool> hidden() const { return dag::ConstSpan<bool>(hidden_ptr(), count); }

  dag::Span<TMatrix> nodeWtm() { return dag::Span<TMatrix>(nodeWtm_ptr(), count); }
  dag::Span<TMatrix> nodePrevWtm() { return dag::Span<TMatrix>(nodePrevWtm_ptr(), count); }
  dag::Span<Point3> wtmToOriginVec() { return dag::Span<Point3>(wtmToOriginVec_ptr(), count); }
  dag::Span<Point3> wtmToOriginVecPrev() { return dag::Span<Point3>(wtmToOriginVecPrev_ptr(), count); }
  dag::Span<float> opacity() { return dag::Span<float>(opacity_ptr(), count); }
  dag::Span<bool> hidden() { return dag::Span<bool>(hidden_ptr(), count); }
};

inline void DynamicRenderableSceneInstance::setCurrentLod(int lod)
{
  if (validateLod(uint32_t(lod)))
  {
    lods->updateReqLod(lod);
    if (lod < lods->getQlBestLod())
      lod = lods->getQlBestLod();
    sceneResource = lods->lods[curLodNo = lod].scene;
  }
}

inline bool DynamicRenderableSceneInstance::validateLod(uint32_t lod) const
{
  // Due to C++ standard conversions, a negative value passed to this function will be above 0x80000000 unsigned
  // https://docs.microsoft.com/en-us/cpp/cpp/standard-conversions?view=vs-2019#integral-conversions
  return lod < lods->lods.size();
}

#define DynamicRenderableSceneLodsResourceClassName "dynobj"

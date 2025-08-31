//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shadersRes.h>
#include <shaders/dag_instShaderMeshRes.h>
#include <math/dag_bounds3.h>
#include <3d/dag_textureIDHolder.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/array.h>

class IRenderWrapperControl;
class TMatrix;
class DataBlock;


void init_rendinst_impostor_shader();
void rendinst_impostor_set_parallax_mode(bool allowed);
void rendinst_impostor_set_view_mode(bool allowed);
bool rendinst_impostor_is_parallax_allowed();
bool rendinst_impostor_is_tri_view_allowed();
void rendinst_impostor_disable_anisotropy();

decl_dclass_and_id(RenderableInstanceResource, DObject, 0xBD23B4A7u)
public:
  DAG_DECLARE_NEW(midmem)

  static RenderableInstanceResource *loadResourceInternal(IGenLoad & crd, int srl_flags, ShaderMatVdata &smvd);

  void render(const TMatrix &tm, IRenderWrapperControl &rwc, real trans = 1);
  void renderTrans(const TMatrix &tm, IRenderWrapperControl &rwc, real trans = 1);

  void gatherUsedTex(TextureIdSet & tex_id_list) const;
  void gatherUsedMat(Tab<ShaderMaterial *> & mat_list) const;

  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new);

  void duplicateMaterial(TEXTUREID tex_id, Tab<ShaderMaterial *> & old_mat, Tab<ShaderMaterial *> & new_mat)
  {
    if (getMesh())
      if (ShaderMesh *mesh = getMesh()->getMesh()->getMesh())
        mesh->duplicateMaterial(tex_id, old_mat, new_mat);
  }
  void duplicateMat(ShaderMaterial * prev_m, Tab<ShaderMaterial *> & old_mat, Tab<ShaderMaterial *> & new_mat)
  {
    if (getMesh())
      if (ShaderMesh *mesh = getMesh()->getMesh()->getMesh())
        mesh->duplicateMat(prev_m, old_mat, new_mat);
  }

  InstShaderMeshResource *getMesh() { return rigid.mesh; }
  const InstShaderMeshResource *getMesh() const { return rigid.mesh; }

  RenderableInstanceResource *clone() const;
  void updateShaderElems()
  {
    if (rigid.mesh)
      rigid.mesh->getMesh()->getMesh()->updateShaderElems();
  }

  static inline int dumpStartOfs() { return offsetof(RenderableInstanceResource, rigid); }
  void *dumpStartPtr() { return &rigid; }

protected:
  struct RigidObject
  {
    PATCHABLE_DATA64(Ptr<InstShaderMeshResource>, mesh);
  };

  RigidObject rigid;

  RenderableInstanceResource() {}
end_dclass_decl();


// Container for LODs, each LOD is RenderableInstanceResource.
decl_dclass_and_id(RenderableInstanceLodsResource, DObject, 0x0F076634u)
public:
  DAG_DECLARE_NEW(midmem)

  struct Lod
  {
    PatchablePtr<RenderableInstanceResource> scene;
    real range;
    float texScale;
    int getAllElems(Tab<dag::ConstSpan<ShaderMesh::RElem>> &out_elems) const;
  };
  struct ImpostorData
  {
    PatchablePtr<Point3> convexPointsPtr;
    PatchablePtr<BBox3> shadowPointsPtr;
    uint16_t convexPointsCount, shadowPointsCount;
    float minY, maxY, cylRad;
    float maxFacingLeavesDelta, bboxAspectRatio;

    dag::ConstSpan<Point3> convexPoints() const { return make_span_const(convexPointsPtr.get(), convexPointsCount); }
    dag::ConstSpan<BBox3> shadowPoints() const { return make_span_const(shadowPointsPtr.get(), shadowPointsCount); }
  };

  struct ImpostorTextures
  {
    TEXTUREID albedo_alpha = BAD_TEXTUREID;
    TEXTUREID normal_translucency = BAD_TEXTUREID;
    TEXTUREID ao_smoothness = BAD_TEXTUREID;
    TEXTUREID shadowAtlas = BAD_TEXTUREID;
    Texture *shadowAtlasTex = nullptr;
    unsigned int width = 0;
    unsigned int height = 0;
    void close()
    {
      ShaderGlobal::reset_from_vars_and_release_managed_tex(albedo_alpha);
      ShaderGlobal::reset_from_vars_and_release_managed_tex(normal_translucency);
      ShaderGlobal::reset_from_vars_and_release_managed_tex(ao_smoothness);
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(shadowAtlas, shadowAtlasTex);
    }
    bool isInitialized()
    {
      return albedo_alpha != BAD_TEXTUREID && normal_translucency != BAD_TEXTUREID && ao_smoothness != BAD_TEXTUREID;
    }
    ImpostorTextures() = default;
    ~ImpostorTextures() { close(); }
    ImpostorTextures(const ImpostorTextures &) = delete;
    ImpostorTextures &operator=(const ImpostorTextures &) = delete;
  };

  struct ImpostorSliceData
  {
    Point4 sliceTm = Point4(0, 0, 0, 0);
    Point4 clippingLines = Point4(0, 0, 0, 0);
  };

  struct ImpostorParams
  {
    eastl::array<ImpostorSliceData, 9> perSliceParams = {};
    Point4 scale = Point4(0, 0, 0, 0);
    Point4 boundingSphere = Point4(0, 0, 0, 0);
    eastl::array<Point4, 4> vertexOffsets = {};
    uint32_t horizontalSamples = 0;
    uint32_t verticalSamples = 0;
    Point4 crownCenter1 = Point4(0, 0, 0, 0), invCrownRad1 = Point4(0, 0, 0, 0);
    Point4 crownCenter2 = Point4(0, 0, 0, 0), invCrownRad2 = Point4(0, 0, 0, 0);
    bool preshadowEnabled = true;
    float bottomGradient = 0.0;
    bool hasBakedTexture() const { return horizontalSamples != 0; }
  };

  friend class RenderableInstanceLodsResSrc;

public:
  static int (*get_skip_first_lods_count)(const char *name, bool has_impostors, int total_lods);

public:
  static RenderableInstanceLodsResource *loadResource(IGenLoad & crd, int srl_flags, const char *name,
    const DataBlock *desc = nullptr);
  static RenderableInstanceLodsResource *makeStubRes(const DataBlock *b = NULL);

  using ImpostorTextureCallback = void (*)(const RenderableInstanceLodsResource *);
  static void setImpostorTextureCallback(ImpostorTextureCallback callback);

  void gatherUsedTex(TextureIdSet & tex_id_list) const;
  void gatherUsedMat(Tab<ShaderMaterial *> & mat_list) const;

  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new);

  void duplicateMaterial(TEXTUREID tex_id, Tab<ShaderMaterial *> & old_mat, Tab<ShaderMaterial *> & new_mat)
  {
    for (int i = 0; i < lods.size(); ++i)
      if (lods[i].scene)
        lods[i].scene->duplicateMaterial(tex_id, old_mat, new_mat);
  }
  void duplicateMat(ShaderMaterial * prev_m, Tab<ShaderMaterial *> & old_mat, Tab<ShaderMaterial *> & new_mat)
  {
    for (int i = 0; i < lods.size(); ++i)
      if (lods[i].scene)
        lods[i].scene->duplicateMat(prev_m, old_mat, new_mat);
  }

  inline float getMaxDist() const { return lods.size() ? lods.back().range : 0; }

  bool isBakedImpostor() const;
  bool setImpostorVars(ShaderMaterial * mat) const;
  bool hasImpostorVars(ShaderMaterial * mat) const;
  bool setImpostorTransitionRange(ShaderMaterial * mat, float transition_lod_start, float transition_range) const;
  void prepareTextures(const char *name, uint32_t shadow_atlas_size, int shadow_atlas_mip_offset, int texture_format_flags);

  RenderableInstanceLodsResource *clone() const;
  void updateShaderElems()
  {
    for (int i = 0; i < lods.size(); ++i)
      if (lods[i].scene)
        lods[i].scene->updateShaderElems();
  }

  float getTexScale(int lod) const { return lods[lod].texScale; }

  static inline int dumpStartOfs() { return offsetof(RenderableInstanceLodsResource, lods); }
  void *dumpStartPtr() { return &lods; }
  const uint8_t *dumpStart() const { return (uint8_t *)(void *)&lods; }

  static const char *getStaticClassName() { return "rendInst"; }

  dag::ConstSpan<Ptr<ShaderMatVdata>> getSmvd() const { return make_span_const(&smvd, 1); }
  void setResLoadingFlag(bool is_loading)
  {
    if (is_loading)
      interlocked_or(packedFields, RES_LD_FLG_MASK << RES_LD_FLG_SHIFT);
    else
      interlocked_and(packedFields, ~(RES_LD_FLG_MASK << RES_LD_FLG_SHIFT));
  }
  bool getResLoadingFlag() const { return (interlocked_relaxed_load(packedFields) & (RES_LD_FLG_MASK << RES_LD_FLG_SHIFT)) != 0; }
  bool areLodsSplit() const { return smvd && smvd->areLodsSplit(); }
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
  RenderableInstanceLodsResource *getFirstOriginal() { return this; }
  RenderableInstanceLodsResource *getNextClone() { return nullptr; }
  const RenderableInstanceLodsResource *getFirstOriginal() const { return this; }
  const RenderableInstanceLodsResource *getNextClone() const { return nullptr; }
  static void lockClonesList() {}
  static void unlockClonesList() {}
  static void (*on_higher_lod_required)(RenderableInstanceLodsResource *res, unsigned req_lod, unsigned cur_lod);

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
    if (lod < qlMinAllowedLod)
      lod = qlMinAllowedLod;
    const int reqLFU = getQlReqLFU();
    const uint16_t reqLod = getQlReqLod();
    if (reqLFU == ::dagor_frame_no() && lod < reqLod)
    {
      setQlReqLod(lod);
      if (on_higher_lod_required)
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
      if (on_higher_lod_required)
      {
        uint32_t qlBestLod = getQlBestLod();
        if (lod < qlBestLod || lod < newPrevLod)
          on_higher_lod_required(this, lod, qlBestLod);
      }
    }
  }
  int getQlBestLod() const { return (interlocked_relaxed_load(packedFields) >> QL_BEST_LOD_SHIFT) & QL_BEST_LOD_MASK; }
  void applyQlBestLod(int lod0)
  {
    const uint32_t m = ~(QL_BEST_LOD_MASK << QL_BEST_LOD_SHIFT), v = (lod0 & QL_BEST_LOD_MASK) << QL_BEST_LOD_SHIFT;
    uint32_t storedValue = interlocked_relaxed_load(packedFields);
    while (interlocked_compare_exchange(packedFields, (storedValue & m) | v, storedValue) != storedValue)
      storedValue = interlocked_acquire_load(packedFields);
  }

  unsigned getQlMinAllowedLod() const { return qlMinAllowedLod; }

  unsigned incQlReloadCnt() { return interlocked_increment(qlReloadCnt); }
  unsigned getQlReloadCnt() const { return interlocked_acquire_load(qlReloadCnt); }
  unsigned incQlDiscardCnt() { return interlocked_increment(qlDiscardCnt); }
  unsigned getQlDiscardCnt() const { return interlocked_acquire_load(qlDiscardCnt); }

protected:
  void setQlReqLod(uint16_t new_lod) { interlocked_release_store(qlReqLod, new_lod); }
  void setQlReqLFU(int frame_no) { interlocked_release_store(qlReqLFU, frame_no); }

  enum FieldsMasks : uint32_t
  {
    RES_SIZE_SHIFT = 0x0,
    RES_SIZE_SIZE = 24,
    RES_SIZE_MASK = (1 << RES_SIZE_SIZE) - 1,
    HAS_IMPOSTOR_MAT_SHIFT = RES_SIZE_SHIFT + RES_SIZE_SIZE,
    HAS_IMPOSTOR_MAT_SIZE = 0x1,
    HAS_IMPOSTOR_MAT_MASK = (1 << HAS_IMPOSTOR_MAT_SIZE) - 1,
    HAS_OCCLUDER_BOX_SHIFT = HAS_IMPOSTOR_MAT_SHIFT + HAS_IMPOSTOR_MAT_SIZE,
    HAS_OCCLUDER_BOX_SIZE = 0x1,
    HAS_OCCLUDER_BOX_MASK = (1 << HAS_OCCLUDER_BOX_SIZE) - 1,
    HAS_OCCLUDER_QUAD_SHIFT = HAS_OCCLUDER_BOX_SHIFT + HAS_OCCLUDER_BOX_SIZE,
    HAS_OCCLUDER_QUAD_SIZE = 0x1,
    HAS_OCCLUDER_QUAD_MASK = (1 << HAS_OCCLUDER_QUAD_SIZE) - 1,
    HAS_IMPOSTOR_DATA_SHIFT = HAS_OCCLUDER_QUAD_SHIFT + HAS_OCCLUDER_QUAD_SIZE,
    HAS_IMPOSTOR_DATA_SIZE = 0x1,
    HAS_IMPOSTOR_DATA_MASK = (1 << HAS_IMPOSTOR_DATA_SIZE) - 1,
    QL_BEST_LOD_SHIFT = HAS_IMPOSTOR_DATA_SHIFT + HAS_IMPOSTOR_DATA_SIZE,
    QL_BEST_LOD_SIZE = 0x3,
    QL_BEST_LOD_MASK = (1 << QL_BEST_LOD_SIZE) - 1,
    RES_LD_FLG_SHIFT = QL_BEST_LOD_SIZE + QL_BEST_LOD_SHIFT,
    RES_LD_FLG_SIZE = 0x1,
    RES_LD_FLG_MASK = (1 << RES_LD_FLG_SIZE) - 1
  };
  static_assert(RES_LD_FLG_SHIFT + RES_LD_FLG_SIZE == 32,
    "Since we use one dword for the bitfield, we shouldn't have unhandled bits there.");
  uint32_t packedFields = 0;
  unsigned char rotationPaletteSize = 1;
  unsigned char qlMinAllowedLod = 0;
  volatile unsigned short qlReqLod = qlReqLodInitialValue, qlReqLodPrev = qlReqLodInitialValue;
  volatile unsigned short qlReloadCnt = 0, qlDiscardCnt = 0;
  int qlReqLFU = 0;
  mutable uint32_t bvhId : 31 = 0;
  mutable bool hasTreeOrFlagMaterial : 1 = false;
  Ptr<ShaderMatVdata> smvd;
  PATCHABLE_32BIT_PAD32(_resv[3]);

public:
  PatchableTab<Lod> lods;
  BBox3 bbox;        //< in local coords: bounding box
  Point3 bsphCenter; //< in local coords: bounding sphere center
  real bsphRad;      //< in local coords: bounding sphere radius
  real bound0rad;    //< radius of bounding sphere with center at (0,0,0)
protected:
  uint16_t impostorDataOfs; //< offset (in bytes) from &lods to ImpostorData struct (or 0 when no impostor data)
  enum ExtraFlags : uint16_t
  {
    HAS_FLOATING_BOX_FLG = 1 << 0,
  };
  uint16_t extraFlags;

  float occl[4 * 3]; //< present ONLY if hasOccluderBox==1 or hasOccluderQuad=1 (in local coords) (BBox3 or Point3[4])
  BBox3 floatingBox; //< present ONLY if hasFloatingBox() == 1

  struct ImpostorRtData
  {
    ImpostorParams params = {};
    ImpostorTextures tex = {}; // (7..8) * 32-bits
    Point3 tiltLimit = Point3(5, 5, 5) * DEG_TO_RAD;

    static ImpostorParams null_params;
    static ImpostorTextures null_tex;
  };

public:
  uint32_t getBvhId() const { return bvhId; }
  void setBvhId(uint32_t id) const { bvhId = id; }

  bool hasTreeOrFlag() const { return hasTreeOrFlagMaterial; }

#if _TARGET_64BIT && !defined(DEBUG_DOBJECTS)
  void setRiExtraId(int id) { DObject::_resv = id; }
  int getRiExtraId() const { return DObject::_resv; }
#else
  constexpr void setRiExtraId(int) {}
  constexpr int getRiExtraId() const { return -1; }
#endif

  void loadImpostorData(const char *name);
  bool hasImpostor() const
  {
    return (interlocked_relaxed_load(packedFields) & (HAS_IMPOSTOR_MAT_MASK << HAS_IMPOSTOR_MAT_SHIFT)) != 0;
  }
  const BBox3 *getOccluderBox() const { return hasOccluderBox() ? (const BBox3 *)occl : NULL; }
  const Point3 *getOccluderQuadPts() const { return hasOccluderQuad() ? (const Point3 *)occl : NULL; }
  const BBox3 *getFloatingBox() const { return hasFloatingBox() ? &floatingBox : nullptr; }
  const ImpostorData *getImpostorData() const
  {
    return impostorDataOfs ? (const ImpostorData *)(dumpStart() + impostorDataOfs) : nullptr;
  }
  uint32_t getRotationPaletteSize() const { return rotationPaletteSize; }

  const ImpostorParams &getImpostorParams() const { return hasImpostor() ? impRtdPtr()->params : ImpostorRtData::null_params; }
  ImpostorParams &getImpostorParamsE()
  {
    G_ASSERT(hasImpostor()); // otherwise non-const reference is returned to the null_params value
    return const_cast<ImpostorParams &>(getImpostorParams());
  }
  const ImpostorTextures &getImpostorTextures() const { return hasImpostor() ? impRtdPtr()->tex : ImpostorRtData::null_tex; }
  ImpostorTextures &getImpostorTexturesE() { return const_cast<ImpostorTextures &>(getImpostorTextures()); }
  Point3 getTiltLimit() const { return hasImpostor() ? impRtdPtr()->tiltLimit : Point3(0, 0, 0); }
  void setTiltLimit(const Point3 &lim)
  {
    if (hasImpostor())
      impRtdPtr()->tiltLimit = lim;
  }

protected:
  RenderableInstanceLodsResource() { setRiExtraId(-1); }
  RenderableInstanceLodsResource(const RenderableInstanceLodsResource &);
  ~RenderableInstanceLodsResource() { clearData(); }

  void patchAndLoadData(int res_sz, IGenLoad &crd, int flags, const char *name);

  ImpostorRtData *impRtdPtr() const
  {
    return (ImpostorRtData *)(getResSize() + (char *)const_cast<RenderableInstanceLodsResource *>(this)->dumpStartPtr());
  }

  // explicit destructor
  void clearData();

  uint32_t getResSize() const { return (interlocked_relaxed_load(packedFields) >> RES_SIZE_SHIFT) & RES_SIZE_MASK; }

  void setHasImpostorMat() { interlocked_or(packedFields, HAS_IMPOSTOR_MAT_MASK << HAS_IMPOSTOR_MAT_SHIFT); }

  void setResSizeNonTS(uint32_t res_size)
  {
    packedFields = (packedFields & (~(RES_SIZE_MASK << RES_SIZE_SHIFT))) | res_size << RES_SIZE_SHIFT;
  }

  bool hasOccluderBox() const
  {
    return (interlocked_relaxed_load(packedFields) & (HAS_OCCLUDER_BOX_MASK << HAS_OCCLUDER_BOX_SHIFT)) != 0;
  }

  bool hasOccluderQuad() const
  {
    return (interlocked_relaxed_load(packedFields) & (HAS_OCCLUDER_QUAD_MASK << HAS_OCCLUDER_QUAD_SHIFT)) != 0;
  }

  bool hasFloatingBox() const { return (interlocked_relaxed_load(extraFlags) & HAS_FLOATING_BOX_FLG) != 0; }

  bool hasImpostorData() const
  {
    return (interlocked_relaxed_load(packedFields) & (HAS_IMPOSTOR_DATA_MASK << HAS_IMPOSTOR_DATA_SHIFT)) != 0;
  }

end_dclass_decl();


#define RenderableInstanceLodsResourceClassName "instobj"

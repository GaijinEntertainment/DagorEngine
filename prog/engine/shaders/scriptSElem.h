// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  scripted shader element class
************************************************************************/
#ifndef __SCRIPTSELEM_H
#define __SCRIPTSELEM_H

#include "shadersBinaryData.h"
#include <shaders/dag_renderStateId.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderState.h>
#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_atomic_types.h>
#include "shStateBlock.h"


/************************************************************************
  forwards
************************************************************************/
class ScriptedShaderMaterial;
class Interval;

/*********************************
 *
 * class ScriptedShaderElement
 *
 *********************************/
class ScriptedShaderElement final : public ShaderElement
{
private:
  ScriptedShaderElement(const shaderbindump::ShaderCode &matcode, ScriptedShaderMaterial &m, const char *info);
  ~ScriptedShaderElement();

public:
  typedef shaders_internal::Tex Tex;
  typedef shaders_internal::Buf Buf;

  struct VarMap
  {
    int32_t varOfs;
    uint16_t intervalId : 14, type : 2;
    uint16_t totalMul;
  };
  struct PackedPassId
  {
    static constexpr ShaderStateBlockId DELETED_STATE_BLOCK_ID = static_cast<ShaderStateBlockId>(-1);

    mutable struct Id
    {
      // NOTE: might also contain the special value of DELETED_STATE_BLOCK_ID, be careful
      dag::AtomicPod<ShaderStateBlockId> v;
      int pr;
    } id; // we cache it inside stateBlocksSpinLock
    PackedPassId() = default;
    PackedPassId(PackedPassId &&p)
    {
      memcpy(this, &p, sizeof(*this));
      memset(&p, 0, sizeof(p));
    }
  };

  const shaderbindump::ShaderCode &code;
  const shaderbindump::ShaderClass &shClass;
  mutable OSSpinlock stateBlocksSpinLock;
  mutable VDECL usedVdecl; // cached
  uint8_t stageDest;
  enum
  {
    NO_VARMAP_TABLE,
    NO_DYNVARIANT,
    COMPLEX_VARMAP
  };
  uint8_t dynVariantOption = NO_DYNVARIANT;

  // for each dynamic variant create it's own passes
  SmallTab<PackedPassId, MidmemAlloc> passes;

  SmallTab<int, MidmemAlloc> texVarOfs;
  SmallTab<VarMap, MidmemAlloc> varMapTable;
  uint32_t dynVariantCollectionId = 0;

  mutable int tex_level = 15;

  mutable bool texturesLoaded = false;

public:
  ScriptedShaderElement(ScriptedShaderElement &&) = default;

  const uint8_t *getVars() const { return ((const uint8_t *)this) + sizeof(ScriptedShaderElement); }
  uint8_t *getVars() { return ((uint8_t *)this) + sizeof(ScriptedShaderElement); }

  static ScriptedShaderElement *create(const shaderbindump::ShaderCode &matcode, ScriptedShaderMaterial &m, const char *info);
  void acquireTexRefs();
  void releaseTexRefs();

  void recreateElem(const shaderbindump::ShaderCode &matcode, ScriptedShaderMaterial &m);

  // select current passes by dynamic variants
  int chooseDynamicVariant(dag::ConstSpan<uint8_t> norm_values, unsigned int &out_variant_code) const;
  int chooseDynamicVariant(unsigned int &out_variant_code) const;
  int chooseCachedDynamicVariant(unsigned int variant_code) const;

  void setStatesForVariant(int curVariant, uint32_t program, ShaderStateBlockId state_index) const;
  void getDynamicVariantStates(int variant_code, int cur_variant, uint32_t &program, ShaderStateBlockId &state_index,
    shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state, shaders::TexStateIdx &tex_state) const;

  void update_stvar(ScriptedShaderMaterial &m, int stvarid);

  GCC_HOT void execute_chosen_stcode(uint16_t stcodeId, uint16_t extStcodeId, const shaderbindump::ShaderCode::Pass *cPass,
    const uint8_t *vars) const;

  SNC_LIKELY_TARGET bool setStates() const override;
  SNC_LIKELY_TARGET void render(int minv, int numv, int sind, int numf, int base_vertex, int prim = PRIM_TRILIST) const override;

  bool setStatesDispatch() const override;
  bool dispatchCompute(int tgx, int tgy, int tgz, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS, bool set_states = true) const;
  eastl::array<uint16_t, 3> getThreadGroupSizes() const;
  uint32_t getWaveSize() const;
  bool dispatchComputeThreads(int threads_x, int threads_y, int threads_z, GpuPipeline gpu_pipeline, bool set_states) const;
  bool dispatchComputeIndirect(Sbuffer *args, int ofs, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS, bool set_states = true) const;

  bool dispatchMesh(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z, bool set_states = true) const;
  bool dispatchMeshThreads(uint32_t threads_x, uint32_t threads_y, uint32_t threads_z, bool set_states = true) const;
  bool dispatchMeshIndirect(Sbuffer *args, uint32_t count, uint32_t stride, uint32_t offset, bool set_states = true) const;
  bool dispatchMeshIndirectCount(Sbuffer *args, uint32_t args_offset, uint32_t args_stride, Sbuffer *count, uint32_t count_offset,
    uint32_t max_count, bool set_states = true) const;

  SNC_LIKELY_TARGET int getTextureCount() const override;
  SNC_LIKELY_TARGET TEXTUREID getTexture(int index) const override;
  SNC_LIKELY_TARGET void gatherUsedTex(TextureIdSet &tex_id_list) const override;
  SNC_LIKELY_TARGET bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new) override;
  SNC_LIKELY_TARGET bool hasTexture(TEXTUREID tex_id) const override;
  SNC_LIKELY_TARGET bool checkAndPrefetchMissingTextures() const override;

  // Return vertex size on shader input.
  SNC_LIKELY_TARGET unsigned int getVertexStride() const override { return code.vertexStride; }

  SNC_LIKELY_TARGET dag::ConstSpan<ShaderChannelId> getChannels() const { return code.channel; }

  SNC_LIKELY_TARGET void replaceVdecl(VDECL vDecl) override;
  SNC_LIKELY_TARGET VDECL getEffectiveVDecl() const override;

  SNC_LIKELY_TARGET int getSupportedBlock(int variant, int layer) const override;

  // call specified function
  void callFunction(int id, int out_reg, dag::ConstSpan<int> in_regs, char *regs);

  void resetStateBlocks();
  void preCreateStateBlocks();
  void resetShaderPrograms(bool delete_programs = true);
  void preCreateShaderPrograms();
  void detachElem();

  const char *getShaderClassName() const override;
  void setProgram(uint32_t variant);
  PROGRAM getComputeProgram(const shaderbindump::ShaderCode::ShRef *p) const;
  inline bool setReqTexLevel(int req_tex_level = 15) const override
  {
    bool increased = req_tex_level > tex_level;
    tex_level = req_tex_level;
    return increased;
  }

private:
  static const unsigned int invalid_variant = 0xFFFFFFFF;
  // use upper 16 bit of seFlags for internal needs
  __forceinline void prepareShaderProgram(PackedPassId::Id &pass_id, int variant, unsigned int variant_code) const;
  __forceinline void preparePassId(PackedPassId::Id &pass_id, int variant, unsigned int variant_code) const;
  void preparePassIdOOL(PackedPassId::Id &pass_id, int variant, unsigned int variant_code) const;

  const shaderbindump::ShaderCode::ShRef *getPassCode() const;

  ShaderStateBlockId recordStateBlock(const shaderbindump::ShaderCode::ShRef &p, const shaderbindump::ShaderCode &sc) const;
  VDECL initVdecl() const;

  ScriptedShaderElement(const ScriptedShaderElement &);
  ScriptedShaderElement &operator=(const ScriptedShaderElement &);
};

#endif //__SCRIPTSELEM_H

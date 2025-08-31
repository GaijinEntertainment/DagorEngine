// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shaderSave.h"
#include "variablesMerger.h"
#include <generic/dag_tab.h>
#include "nameMap.h"
#include "hlslRegisters.h"
#include "globalConfig.h"
#include <util/dag_bindump_ext.h>
#include <util/dag_string.h>
#include <EASTL/vector_set.h>
#include <EASTL/array.h>

struct CryptoHash;
namespace ShaderTerminal
{
struct named_const;
}
namespace shc
{
class TargetContext;
}

class ShaderStateBlock;
class StcodeRoutine;

static bool operator==(const Tab<bindump::Address<ShaderStateBlock>> &a, const Tab<bindump::Address<ShaderStateBlock>> &b)
{
  if (a.size() != b.size())
    return false;

  return eastl::equal(a.begin(), a.end(), b.begin());
}

static bool operator!=(const Tab<bindump::Address<ShaderStateBlock>> &a, const Tab<bindump::Address<ShaderStateBlock>> &b)
{
  return !(a == b);
}

struct NamedConstBlock
{
  struct NamedConst
  {
    HlslRegisterSpace rspace = HLSL_RSPACE_INVALID;
    int regIndex = -1;
    short size = 0;
    bool isDynamic = false;
    String hlslDecl, hlslPostfix;
  };

  struct RegisterProperties
  {
    SCFastNameMap sn;
    Tab<NamedConst> sc;
  };

  struct SlotTextureSubAllocators
  {
    HlslRegAllocator vsTex, psTex, vsSamplers, psSamplers;
  };

  struct CstHandle
  {
    ShaderStage stage;
    int id;
    bool isBufConst;

    friend bool operator==(CstHandle l, CstHandle r) = default;
    friend bool operator!=(CstHandle l, CstHandle r) = default;

    static CstHandle makeInvalidHandle(ShaderStage stage, bool is_cbuf_const) { return {stage, -1, is_cbuf_const}; }
  };
  friend bool is_valid(CstHandle hnd) { return hnd.id >= 0; }

  using MergedVariablesData = ShaderParser::VariablesMerger;

  BINDUMP_BEGIN_NON_SERIALIZABLE();
  RegisterProperties pixelProps, vertexProps;
  ShaderStateBlock *globConstBlk = nullptr;
  bool multidrawCbuf = false;

  eastl::array<HlslRegAllocator, HLSL_RSPACE_COUNT> vertexRegAllocators =
    make_default_hlsl_reg_allocators(shc::config().hlslMaximumVsfAllowed);
  eastl::array<HlslRegAllocator, HLSL_RSPACE_COUNT> pixelOrComputeRegAllocators =
    make_default_hlsl_reg_allocators(shc::config().hlslMaximumPsfAllowed);

  // Either for global const block (for LEV_GLOBAL_CONST), or static cbuf (for LEV_SHADER)
  HlslRegAllocator bufferedConstsRegAllocator = make_default_cbuf_reg_allocator();
  RegisterProperties bufferedConstProps{};

  SlotTextureSubAllocators slotTextureSuballocators{};

  // @TODO: better (for mt?)
  mutable String cachedPixelOrComputeHlsl, cachedVertexHlsl;

  BINDUMP_END_NON_SERIALIZABLE();

  SerializableTab<bindump::Address<ShaderStateBlock>> suppBlk;

  NamedConstBlock() : suppBlk(midmem) {}

  CstHandle addConst(int stage, const char *name, HlslRegisterSpace reg_space, int sz, int hardcoded_reg, bool is_dynamic,
    bool is_global_const_block);

  // @TODO: error message
  bool initSlotTextureSuballocators(int vs_tex_count, int ps_tex_count);

  NamedConst &getSlot(CstHandle hnd)
  {
    G_ASSERT(is_valid(hnd));
    return (hnd.isBufConst ? bufferedConstProps : (hnd.stage == STAGE_VS ? vertexProps : pixelProps)).sc[hnd.id];
  }
  const NamedConst &getSlot(CstHandle hnd) const { return const_cast<NamedConstBlock *>(this)->getSlot(hnd); }

  void addHlslDecl(CstHandle hnd, String &&hlsl_decl, String &&hlsl_postfix);

  void patchHlsl(String &src, ShaderStage stage, const MergedVariablesData &merged_vars, int &max_const_no_used,
    eastl::string_view hw_defines, bool uses_dual_source_blending);

  CryptoHash getDigest(ShaderStage stage, const MergedVariablesData &merged_vars) const;

  void buildDrawcallIdHlslDecl(String &out_text) const;
  void buildStaticConstBufHlslDecl(String &out_text, const MergedVariablesData &merged_vars) const;
  void buildGlobalConstBufHlslDecl(String &out_text) const;
  void buildHlslDeclText(String &out_text, ShaderStage stage) const
  {
    eastl::vector_set<const NamedConstBlock *> builtBlocks{};
    doBuildHlslDeclText(out_text, stage, builtBlocks);
  }

  void buildAllHlsl(String *out_text, const MergedVariablesData &merged_vars, ShaderStage stage) const
  {
    String &cache = stage == STAGE_VS ? cachedVertexHlsl : cachedPixelOrComputeHlsl;
    if (cache.empty())
    {
      buildDrawcallIdHlslDecl(cache);
      buildStaticConstBufHlslDecl(cache, merged_vars);
      buildHlslDeclText(cache, stage);
    }

    if (out_text)
      out_text->append(cache.data(), cache.length());
  }
  void dropHlslCaches() const
  {
    clear_and_shrink(cachedVertexHlsl);
    clear_and_shrink(cachedPixelOrComputeHlsl);
  }

  void doBuildHlslDeclText(String &out_text, ShaderStage stage, eastl::vector_set<const NamedConstBlock *> &built_blocks) const;

  eastl::pair<int, int> getStaticTexRange(ShaderStage stage) const;

  struct AllocatedRegInfo
  {
    eastl::string name;
    int size;
  };

  auto makeInfoProvider(const RegisterProperties NamedConstBlock::*props, HlslRegisterSpace rspace) const;
};

enum class ShaderBlockLevel
{
  FRAME,
  SCENE,
  OBJECT,
  SHADER,
  GLOBAL_CONST,

  UNDEFINED = -1
};

inline constexpr size_t SHADER_BLOCK_HIER_LEVELS = size_t(ShaderBlockLevel::SHADER);

class ShaderStateBlock
{
public:
  ShaderStateBlock() : nameId(-1), stcodeId(-1), cppStcodeId(-1), layerLevel(ShaderBlockLevel::UNDEFINED), regSize(0) {}
  ShaderStateBlock(const char *name, ShaderBlockLevel lev, NamedConstBlock &&ncb, dag::Span<int> stcode,
    DynamicStcodeRoutine *cpp_stcode, int maxregsize, shc::TargetContext &a_ctx);

  bool canBeSupportedBy(ShaderBlockLevel lev) const { return layerLevel < lev; }
  int getVsNameId(const char *name) const;
  int getPsNameId(const char *name) const;

  struct BuildTimeData
  {
    unsigned uidMask = 0, uidVal = 0;
    int suppListOfs = -1;
    unsigned suppMask = 0;
  };

  NamedConstBlock shConst;
  bindump::string name;
  int nameId;
  int stcodeId, regSize;
  int cppStcodeId;
  ShaderBlockLevel layerLevel;

  BINDUMP_NON_SERIALIZABLE(
    //! filled and used in make_scripted_shaders_dump()
    BuildTimeData btd;);
};

class ShaderBlockTable
{
  Tab<ShaderStateBlock *> blocks{};
  FastNameMap blockNames;

  // This field is needed for the following reason:
  // the main table is structured as follows: each block is created on the heap, and an owning pointer is stored in blocks array field.
  // However, if the block is empty (i. e. supports none) a pointer to a valid empty block has to be registered. On serialization, this
  // empty block has to be serialized too so that the array of pointers is serialized correctly. Since this table is used for building
  // the data which is then serialized, it has to have its own empty block so that it can then be serialized and pointers to it are
  // valid.
  bindump::Ptr<ShaderStateBlock> emptyBlk;

public:
  ShaderBlockTable() { emptyBlk.create() = ShaderStateBlock{}; }
  ~ShaderBlockTable();

  bool registerBlock(ShaderStateBlock *blk, bool allow_identical_redecl = false);
  ShaderStateBlock *findBlock(const char *name) const;
  int countBlock(ShaderBlockLevel level = ShaderBlockLevel::UNDEFINED) const;

  // We need to be able to get a mutable pointer to empty block even though we don't mutate it, as it can be put in blocks array.
  ShaderStateBlock *emptyBlock() { return emptyBlk.get(); }
  const ShaderStateBlock *emptyBlock() const { return emptyBlk.get(); }

  Tab<ShaderStateBlock *> release();
  bindump::Ptr<ShaderStateBlock> releaseEmptyBlock() { return eastl::move(emptyBlk); }

  void link(Tab<ShaderStateBlock *> &loaded_blocks, dag::ConstSpan<int> stcode_remap, dag::ConstSpan<int> external_stcode_remap);
};

inline auto NamedConstBlock::makeInfoProvider(const RegisterProperties NamedConstBlock::*props, HlslRegisterSpace rspace) const
{
  return [this, props, rspace](int reg) {
    auto getInfoForReg = [&](const RegisterProperties &props) {
      for (const auto &[ind, cst] : enumerate(props.sc))
      {
        if (cst.rspace == rspace && cst.regIndex == reg)
          return eastl::make_tuple(props.sn.getName(ind), int(cst.size));
      }
      return eastl::make_tuple((const char *)nullptr, 1);
    };

    for (const auto &blk : suppBlk)
    {
      if (auto [name, size] = getInfoForReg(blk->shConst.*props); name)
      {
        return AllocatedRegInfo{eastl::string{name} + ", <block " + blk->name.c_str() + ">", size};
      }
    }
    if (auto [name, size] = getInfoForReg(this->*props); name)
      return AllocatedRegInfo{eastl::string{name} + ", <shader>", size};

    return AllocatedRegInfo{"<unnamed>", 1};
  };
}
// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shaderSave.h"
#include "shlexterm.h"
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
class CompiledPreshader;

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
    const Terminal *declTerm = nullptr;
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
    static CstHandle makeErrorHandle(ShaderStage stage, bool is_cbuf_const) { return {stage, -2, is_cbuf_const}; }
  };
  friend bool is_valid(CstHandle hnd) { return hnd.id >= 0; }
  friend bool is_error(CstHandle hnd) { return hnd.id == -2; }

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

  CstHandle addConst(int stage, const char *name, Terminal *decl_term, Lexer &lexer, HlslRegisterSpace reg_space, int sz,
    int hardcoded_reg, bool is_dynamic, bool is_global_const_block);

  // @TODO: error message
  bool initSlotTextureSuballocators(int vs_tex_count, int vs_smp_count, int ps_tex_count, int ps_smp_count, Lexer &lexer);

  NamedConst &getSlot(CstHandle hnd)
  {
    G_ASSERT(is_valid(hnd));
    return (hnd.isBufConst ? bufferedConstProps : (hnd.stage == STAGE_VS ? vertexProps : pixelProps)).sc[hnd.id];
  }
  const NamedConst &getSlot(CstHandle hnd) const { return const_cast<NamedConstBlock *>(this)->getSlot(hnd); }

  void addHlslDecl(CstHandle hnd, String &&hlsl_decl, String &&hlsl_postfix);

  void patchHlsl(String &src, ShaderStage stage, const CompiledPreshader &preshader, Lexer &lexer, int &max_const_no_used,
    eastl::string_view hw_defines, bool uses_dual_source_blending);

  CryptoHash getDigest(ShaderStage stage, const CompiledPreshader &preshader) const;

  void buildDrawcallIdHlslDecl(String &out_text) const;
  void buildStaticConstBufHlslDecl(String &out_text, const CompiledPreshader &preshader) const;
  void buildGlobalConstBufHlslDecl(String &out_text) const;
  void buildHlslDeclText(String &out_text, ShaderStage stage) const;

  void buildAllHlsl(String *out_text, const CompiledPreshader &preshader, ShaderStage stage) const
  {
    String &cache = stage == STAGE_VS ? cachedVertexHlsl : cachedPixelOrComputeHlsl;
    if (cache.empty())
    {
      buildDrawcallIdHlslDecl(cache);
      buildStaticConstBufHlslDecl(cache, preshader);
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

  void preprocessHlslDecls(String &out_text, ShaderStage stage, eastl::vector_set<const NamedConstBlock *> &built_blocks,
    eastl::vector<const NamedConst *> &out_gathered_consts) const;

  struct AllocatedRegInfo
  {
    eastl::string desc;
    int size;
  };

  auto makeInfoProvider(const RegisterProperties NamedConstBlock::*props_field, HlslRegisterSpace rspace, Lexer &lexer) const;
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

inline auto NamedConstBlock::makeInfoProvider(const RegisterProperties NamedConstBlock::*props_field, HlslRegisterSpace rspace,
  Lexer &lexer) const
{
  return [this, props_field, rspace, &lexer](int reg) {
    auto getInfoForReg = [&](const RegisterProperties &props) {
      for (const auto &[ind, cst] : enumerate(props.sc))
      {
        if (cst.rspace == rspace && cst.regIndex == reg)
        {
          eastl::optional<eastl::string> sourceRef{};
          if (const auto *t = cst.declTerm; t && t->num == SHADER_TOKENS::SHTOK_ident)
            if (const char *fn = lexer.get_filename(t->file_start))
            {
              sourceRef.emplace(eastl::string::CtorSprintf{}, "%s(%i,%i)", fn, t->line_start, t->col_start);
              if (shc::config().dumpRegsMacroCallstacks && !t->macro_call_stack.empty())
              {
                sourceRef->append("\n      Call stack:\n    ");
                for (auto it = t->macro_call_stack.rbegin(); it != t->macro_call_stack.rend(); ++it)
                  sourceRef->append_sprintf("  %s()\n          %s(%i)\n    ", it->name, lexer.get_filename(it->file), it->line);
              }
            }
          return eastl::make_tuple(props.sn.getName(ind), sourceRef, int(cst.size));
        }
      }
      return eastl::make_tuple((const char *)nullptr, eastl::optional<eastl::string>{}, 1);
    };

    auto getInfoForBlockReg = [&](const ShaderStateBlock *blk) -> eastl::optional<AllocatedRegInfo> {
      if (!blk)
        return eastl::nullopt;
      if (auto [name, ref, size] = getInfoForReg(blk->shConst.*props_field); name)
      {
        return AllocatedRegInfo{
          eastl::string{name} + ", <block " + blk->name.c_str() + ", " + (ref ? ref->c_str() : "unknown source location") + ">\n",
          size};
      }
      return eastl::nullopt;
    };

    auto getInfoForBlockRecursive = [&](const ShaderStateBlock *blk, auto &&self) -> eastl::optional<AllocatedRegInfo> {
      if (!blk)
        return eastl::nullopt;
      if (auto info = self(blk->shConst.globConstBlk, self))
        return eastl::move(*info);
      for (const auto &sblk : blk->shConst.suppBlk)
        if (auto info = self(sblk, self))
          return eastl::move(*info);
      return getInfoForBlockReg(blk);
    };

    for (const auto &blk : suppBlk)
      if (auto info = getInfoForBlockRecursive(blk, getInfoForBlockRecursive))
        return eastl::move(*info);
    if (auto [name, ref, size] = getInfoForReg(this->*props_field); name)
      return AllocatedRegInfo{eastl::string{name} + ", <shader, " + (ref ? ref->c_str() : "unknown source location") + ">\n", size};

    return AllocatedRegInfo{"<unnamed>\n", 1};
  };
}

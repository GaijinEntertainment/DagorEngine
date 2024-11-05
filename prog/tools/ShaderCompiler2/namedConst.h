// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsyntok.h"
#include "shaderSave.h"
#include "variablesMerger.h"
#include <generic/dag_tab.h>
#include "nameMap.h"
#include <util/dag_bindump_ext.h>
#include <util/dag_string.h>
#include <EASTL/vector_set.h>
#include <EASTL/array.h>

struct CryptoHash;
namespace ShaderTerminal
{
struct named_const;
}

class ShaderStateBlock;
class StcodeRoutine;

enum class StaticCbuf
{
  NONE,
  SINGLE,
  ARRAY
};

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

enum class NamedConstSpace
{
  unknown,
  vsf,
  psf,
  csf,
  vsmp,
  smp,
  sampler,
  uav,
  vs_uav,
  vs_buf,
  ps_buf,
  cs_buf,
  vs_cbuf,
  ps_cbuf,
  cs_cbuf,
  count,
};

struct NamedConstBlock
{
  struct NamedConst
  {
    NamedConstSpace nameSpace = NamedConstSpace::unknown;
    int regIndex = -1;
    short size = 0;
    bool isDynamic = false;
    String hlsl;
  };

  struct RegisterProperties
  {
    SCFastNameMap sn;
    Tab<NamedConst> sc;
    eastl::array<eastl::vector_set<int>, (int)NamedConstSpace::count> hardcoded_regs;
    Tab<eastl::string> postfixHlsl;
  };

  using MergedVariablesData = ShaderParser::VariablesMerger;

  BINDUMP_BEGIN_NON_SERIALIZABLE();
  RegisterProperties pixelProps, vertexProps;
  ShaderStateBlock *globConstBlk = nullptr;
  StaticCbuf staticCbufType = StaticCbuf::NONE;
  BINDUMP_END_NON_SERIALIZABLE();

  SerializableTab<bindump::Address<ShaderStateBlock>> suppBlk;

  NamedConstBlock() : suppBlk(midmem) {}

  int addConst(int stage, const char *name, NamedConstSpace name_space, int sz, int hardcoded_reg, String &&hlsl_decl);
  void addHlslPostCode(const char *postfix_hlsl);
  void markStatic(int id, NamedConstSpace name_space);

  int arrangeRegIndices(int base_reg, NamedConstSpace name_space, int *base_cblock_reg = nullptr);
  int getRegForNamedConst(const char *name_buf, NamedConstSpace ns, bool pixel_shader);
  int getRegForNamedConstEx(const char *name_buf, const char *blk_name, NamedConstSpace ns, bool pixel_shader);

  void patchHlsl(String &src, bool pixel_shader, bool compute_shader, const MergedVariablesData &merged_vars, int &max_const_no_used);
  void patchStcodeIndices(dag::Span<int> stcode, StcodeRoutine &cpp_stcode, bool static_blk);

  CryptoHash getDigest(bool ps_const, bool cs_const, const MergedVariablesData &merged_vars) const;

  void buildDrawcallIdHlslDecl(String &out_text) const;
  void buildStaticConstBufHlslDecl(String &out_text, const MergedVariablesData &merged_vars) const;
  void buildGlobalConstBufHlslDecl(String &out_text, bool pixel_shader, bool compute_shader, SCFastNameMap &added_names) const;
  void buildHlslDeclText(String &out_text, bool pixel_shader, bool compute_shader, SCFastNameMap &added_names,
    bool has_static_cbuf) const;
  static const char *nameSpaceToStr(NamedConstSpace name_space);
};

class ShaderStateBlock
{
public:
  ShaderStateBlock() : nameId(-1), stcodeId(-1), layerLevel(-1), regSize(0) {}
  ShaderStateBlock(const char *name, int lev, const NamedConstBlock &ncb, dag::Span<int> stcode, StcodeRoutine *cpp_stcode,
    int maxregsize, StaticCbuf supp_static_cbuf);

  bool canBeSupportedBy(int lev) { return layerLevel < lev; }
  int getVsNameId(const char *name);
  int getPsNameId(const char *name);

public:
  static bool registerBlock(ShaderStateBlock *blk, bool allow_identical_redecl = false);
  static void deleteAllBlocks();
  static ShaderStateBlock *findBlock(const char *name);
  static Tab<ShaderStateBlock *> &getBlocks();
  static int countBlock(int level = -1);
  static ShaderStateBlock *emptyBlock();
  static bindump::Ptr<ShaderStateBlock> &getEmptyBlock();

  static void link(Tab<ShaderStateBlock *> &loaded_blocks, dag::ConstSpan<int> stcode_remap);

public:
  enum
  {
    LEV_FRAME,
    LEV_SCENE,
    LEV_OBJECT,
    LEV_SHADER,
    LEV_GLOBAL_CONST
  };

  struct UsageIdx
  {
    int vsf, vsmp, psf, smp, sampler, csf, uav, vs_uav, ps_buf, cs_buf, vs_buf, ps_cbuf, cs_cbuf, vs_cbuf;
  };
  struct BuildTimeData
  {
    unsigned uidMask, uidVal;
    int suppListOfs = -1;
    unsigned suppMask = 0;
  };

  NamedConstBlock shConst;
  bindump::string name;
  int nameId;
  int stcodeId, regSize;
  int layerLevel;
  StaticCbuf supportsStaticCbuf = StaticCbuf::NONE;

  UsageIdx start, final;

  BINDUMP_NON_SERIALIZABLE(
    //! filled and used in make_scripted_shaders_dump()
    BuildTimeData btd;);
};

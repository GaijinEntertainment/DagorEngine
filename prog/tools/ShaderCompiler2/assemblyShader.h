/************************************************************************
  shader assembler
/************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __ASSEMBLYSHADER_H
#define __ASSEMBLYSHADER_H

#include "shsem.h"
#include "shsyn.h"
#include "shcode.h"
#include "shSemCode.h"
#include "shExprParser.h"
#include "namedConst.h"
#include "nameMap.h"
#include <EASTL/vector_map.h>

/************************************************************************/
/* forwards
/************************************************************************/
struct ShHardwareOptions;
class CodeSourceBlocks;

constexpr bool useScarlettWave32 = false;

namespace ShaderParser
{
/*********************************
 *
 * class AssembleShaderEvalCB
 *
 *********************************/
class AssembleShaderEvalCB : public ShaderEvalCB, public ShaderBoolEvalCB
{
public:
  struct HlslCompile
  {
    String profile, entry;
    hlsl_compile_class *compile;

    HlslCompile() : compile(NULL) {}
    void reset() { compile = NULL; }
  };

  ShaderSyntaxParser &parser;
  ShaderSemCode &code;
  ShaderClass &sclass;
  ShaderSemCode::Pass *curpass;
  ShaderSemCode::PassTab *curvariant;
  const ShaderVariant::TypeTable &allRefStaticVars;
  Terminal *shname_token;
  bool dont_render = false;
  Terminal *no_dynstcode = nullptr;
  Tab<Terminal *> stcode_vars;
  const ShaderVariant::VariantInfo &variant;
  const ShHardwareOptions &opt;
  int maxregsize;

  int last_reg_ind;

  HlslCompile hlslPs, hlslVs, hlslHs, hlslDs, hlslGs, hlslCs, hlslMs, hlslAs;
  NamedConstBlock shConst;
  int shBlockLev;
  StaticCbuf supportsStaticCbuf = StaticCbuf::NONE;
  String evalExprErrStr;

  // shadervar id, opcode -> register
  eastl::vector_map<eastl::pair<int, int>, Register> stVarToReg;
  eastl::array<int, 256> usedRegs{};

  eastl::array<eastl::vector<state_block_stat>, (int)ShaderStage::STAGE_MAX> externalStateBlocks;
  eastl::map<eastl::string, eastl::vector<eastl::string>> combinedNameToOrigNames;
  bool needToMerge = true;

  /************************************************************************/
  /* AssemblyShader.cpp
  /************************************************************************/
  static bool compareShader(Terminal *shname_token, bool_value &e);
  static bool compareHWToken(int hw_token, const ShHardwareOptions &opt);
  static bool compareHW(bool_value &e, const ShHardwareOptions &opt) { return compareHWToken(e.hw->var->num, opt); }

  static void buildHwDefines(const ShHardwareOptions &opt);
  static const String &getBuiltHwDefines();

  AssembleShaderEvalCB(ShaderSemCode &sc, ShaderSyntaxParser &p, ShaderClass &cls, Terminal *shname,
    const ShaderVariant::VariantInfo &variant, const ShHardwareOptions &_opt, const ShaderVariant::TypeTable &all_ref_static_vars,
    bool need_to_merge);

  inline class Register _add_reg_word32(int sz, bool aligned);
  inline Register add_vec_reg(int num = 1);
  inline Register add_reg();
  inline Register add_resource_reg();

  Register add_reg(int type);

  void error(const char *msg, const Terminal *const t);
  inline void warning(const char *msg, const Terminal *const t);

  void eval_static(static_var_decl &s);
  void eval_interval_decl(interval &interv) {}
  void eval_bool_decl(bool_decl &) override;
  void decl_bool_alias(const char *name, bool_expr &expr) override;
  void eval_init_stat(SHTOK_ident *var, shader_init_value &v);
  void eval_channel_decl(channel_decl &s, int stream_id = 0);

  int get_state_var(const Terminal &s, int &vt, bool &is_global, bool allow_not_found = false);
  int get_blend_k(const Terminal &s);
  int get_stencil_cmpf_k(const Terminal &s);
  int get_stensil_op_k(const Terminal &s);
  int get_bool_const(const Terminal &s);
  void get_depth_cmpf_k(const Terminal &s, int &cmpf);

  void eval_state(state_stat &s);
  void eval_zbias_state(zbias_state_stat &s);
  void eval_external_block(external_state_block &);
  void eval_external_block_stat(state_block_stat &, ShaderStage stage);
  void handle_external_block_stat(state_block_stat &, ShaderStage stage);
  void eval(immediate_const_block &) override {}
  void eval_error_stat(error_stat &) override;
  void eval_render_stage(render_stage_stat &s);
  void eval_assume_stat(assume_stat &s) override {}
  void eval_command(shader_directive &s);
  void eval_supports(supports_stat &);

  inline int eval_if(bool_expr &e);
  void eval_else(bool_expr &) {}
  void eval_endif(bool_expr &) {}

  virtual ShVarBool eval_expr(bool_expr &e);
  virtual ShVarBool eval_bool_value(bool_value &e);
  virtual int eval_interval_value(const char *ival_name);

  void merge_external_blocks();
  void end_pass(Terminal *terminal);

  void end_eval(shader_decl &sh);

  virtual void eval_shader_locdecl(local_var_decl &s);

  virtual void eval_hlsl_compile(hlsl_compile_class &hlsl_compile);
  virtual void eval_hlsl_decl(hlsl_local_decl_class &hlsl_compile) {}

  void hlsl_compile(HlslCompile &hlsl);
}; // class AssembleShaderEvalCB

class Register
{
  int startReg = -1;
  int count = 0;
  int currentReg = -1;
  AssembleShaderEvalCB *owner = nullptr;
  friend class AssembleShaderEvalCB;

  void acquireRegs(int r, int num)
  {
    startReg = r;
    count = num;
    currentReg = r;
    for (int i = startReg; i < startReg + count; i++)
      owner->usedRegs[i]++;
  }
  void releaseRegs()
  {
    for (int i = startReg; i < startReg + count; i++)
    {
      G_ASSERT(owner->usedRegs[i] > 0);
      owner->usedRegs[i]--;
    }
    eastl::move(*this).release();
  }

  Register(int reg, AssembleShaderEvalCB &owner) : startReg(reg), count(4), currentReg(reg), owner(&owner) {}

  Register(int num, bool aligned, AssembleShaderEvalCB &owner) : count(num), owner(&owner)
  {
    for (int i = 0; i <= owner.usedRegs.size() - num; i += (aligned ? 4 : 1))
    {
      if (eastl::all_of(&owner.usedRegs[i], &owner.usedRegs[i + num], [](int used) { return used == 0; }))
      {
        acquireRegs(i, num);
        return;
      }
    }
    owner.error("Unable to allocate registers", nullptr);
  }

  void swap(Register &r)
  {
    eastl::swap(startReg, r.startReg);
    eastl::swap(currentReg, r.currentReg);
    eastl::swap(count, r.count);
    eastl::swap(owner, r.owner);
  }

public:
  Register() = default;
  Register(const Register &r) : owner(r.owner)
  {
    acquireRegs(r.startReg, r.count);
    currentReg = r.currentReg;
  }
  Register &operator=(const Register &r)
  {
    Register re(r);
    re.swap(*this);
    return *this;
  }
  Register(Register &&r) noexcept { r.swap(*this); }
  Register &operator=(Register &&r) noexcept
  {
    Register re(eastl::move(r));
    re.swap(*this);
    return *this;
  }
  ~Register() { releaseRegs(); }

  int release() &&
  {
    int r = currentReg;
    startReg = -1;
    currentReg = -1;
    count = 0;
    return r;
  }

  void reset(int r)
  {
    releaseRegs();
    G_ASSERT(owner->usedRegs[r] > 0);
    acquireRegs(r, 1);
  }

  explicit operator int() const
  {
    G_ASSERT(startReg != -1 && count > 0 && currentReg != -1);
    return currentReg;
  }
};

inline Register AssembleShaderEvalCB::_add_reg_word32(int num, bool aligned)
{
  Register reg(num, aligned, *this);
  if (maxregsize < int(reg) + num)
    maxregsize = int(reg) + num;
  return reg;
}

inline Register AssembleShaderEvalCB::add_vec_reg(int num) { return _add_reg_word32(4 * num, true); }
inline Register AssembleShaderEvalCB::add_reg() { return _add_reg_word32(1, false); }
inline Register AssembleShaderEvalCB::add_resource_reg() { return _add_reg_word32(2, false); }

inline void AssembleShaderEvalCB::warning(const char *msg, const Terminal *const t)
{
  G_ASSERT(t);
  parser.get_lex_parser().set_warning(t->file_start, t->line_start, t->col_start, msg);
}

inline int AssembleShaderEvalCB::eval_if(bool_expr &e) { return eval_expr(e).value ? IF_TRUE : IF_FALSE; }


void resolve_pending_shaders_from_cache();

// clear caches
void clear_per_file_caches();

extern CodeSourceBlocks *curVsCode, *curHsCode, *curDsCode, *curGsCode, *curPsCode, *curCsCode, *curMsCode, *curAsCode;

static inline CodeSourceBlocks *getSourceBlocks(const char *profile)
{
  switch (profile[0])
  {
    case 'v': return ShaderParser::curVsCode;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
    case 'h': return ShaderParser::curHsCode;
    case 'd': return ShaderParser::curDsCode;
    case 'g': return ShaderParser::curGsCode;
    case 'p': return ShaderParser::curPsCode;
    case 'c': return ShaderParser::curCsCode;
    case 'm': return ShaderParser::curMsCode;
    case 'a': return ShaderParser::curAsCode;
  }
  return NULL;
}

extern SCFastNameMap renderStageToIdxMap;
} // namespace ShaderParser

#endif //__ASSEMBLYSHADER_H

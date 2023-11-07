// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include "assemblyShader.h"
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shUtils.h>

#include "varMap.h"
#include "globVar.h"
#include "boolVar.h"
#include "sh_stat.h"
#include "semUtils.h"
#include "shaderTab.h"
#include "shCompiler.h"
#include "compileResult.h"
#include "hash.h"
#include "shHardwareOpt.h"
#include "transcodeShader.h"
#include "codeBlocks.h"
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <debug/dag_debug.h>
#include <math/random/dag_random.h>
#include "fast_isalnum.h"
#include <memory/dag_regionMemAlloc.h>

#include "DebugLevel.h"
#include <EASTL/vector_map.h>
#include <EASTL/string_view.h>
#include <EASTL/set.h>

using namespace ShaderParser;

// return maximum permitted FSH version
extern d3d::shadermodel::Version getMaxFSHVersion();

extern DebugLevel hlslDebugLevel;
extern bool hlslDumpCodeAlways, hlslDumpCodeOnError;
extern bool hlslSkipValidation;
extern bool hlslNoDisassembly;
extern bool hlslShowWarnings;
extern bool hlslWarningsAsErrors;
extern bool isDebugModeEnabled;
extern bool enableBindless;
#if _CROSS_TARGET_SPIRV
extern bool compilerHlslCc;
extern bool compilerDXC;
#endif

static bool is_hlsl_debug() { return hlslDebugLevel != DebugLevel::NONE; }

#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
extern dx12::dxil::Platform targetPlatform;
#endif

#if !_TARGET_PC_MACOSX
#include <windows.h>
#else
#include <unistd.h>
#endif

#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_METAL
#include "hlsl2metal/asmShaderHLSL2Metal.h"
#include "hlsl2metal/asmShaderHLSL2MetalGlslang.h"
#elif _CROSS_TARGET_SPIRV
#include "hlsl2spirv/asmShaderSpirV.h"
#elif _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
#elif _CROSS_TARGET_DX11 //_TARGET_PC is also defined
#include "hlsl11transcode/asmShaders11.h"
#include <D3Dcompiler.h>
#endif

#ifdef _CROSS_TARGET_METAL
extern bool use_ios_token;
extern bool use_binary_msl;
extern bool use_metal_glslang;
#endif

struct CachedShader
{
  enum
  {
    TYPE_VS = 1,
    TYPE_HS,
    TYPE_DS,
    TYPE_GS,
    TYPE_PS,
    TYPE_CS,
    TYPE_MS,
    TYPE_AS
  };

  int codeType;
  Tab<unsigned> relCode;
  ComputeShaderInfo computeShaderInfo;
  CachedShader() : relCode(midmem_ptr()), codeType(0) {}

  dag::ConstSpan<unsigned> getShaderOutCode(int type) const { return codeType == type ? relCode : dag::ConstSpan<unsigned>(); }

  const ComputeShaderInfo &getComputeShaderInfo() const
  {
    G_ASSERT(codeType == TYPE_CS || codeType == TYPE_MS || codeType == TYPE_AS);
    return computeShaderInfo;
  }

  static int typeFromProfile(const char *profile)
  {
    switch (profile[0])
    {
      case 'v': return TYPE_VS;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
      case 'h': return TYPE_HS;
      case 'd': return TYPE_DS;
      case 'g': return TYPE_GS;
      case 'p': return TYPE_PS;
      case 'c': return TYPE_CS;
      case 'm': return TYPE_MS;
      case 'a': return TYPE_AS;
    }
    G_ASSERTF(0, "profile=%s", profile);
    return 0;
  }
};

//! returns cached shader idx and cache entry (c1,c2,c3)
static int find_cached_shader(const CryptoHash &code_digest, const CryptoHash &const_digest, const char *entry, const char *profile,
  int &out_c1, int &out_c2, int &out_c3);

//! writes shader to cache entry (c1,c2,c3); returns true when new unique shader added
static bool post_shader_to_cache(int c1, int c2, int c3, const CompileResult &result, int type);

//! sets cached shader index =-2 for entry
static void mark_cached_shader_entry_as_pending(int c1, int c2, int c3);

//! returns cached shader for entry
static CachedShader &resolve_cached_shader(int c1, int c2, int c3);


/*********************************
 *
 * class AssembleShaderEvalCB
 *
 *********************************/
AssembleShaderEvalCB::AssembleShaderEvalCB(ShaderSemCode &sc, ShaderSyntaxParser &p, ShaderClass &cls, Terminal *shname,
  const ShaderVariant::VariantInfo &_variant, const ShHardwareOptions &_opt, const ShaderVariant::TypeTable &all_ref_static_vars,
  bool need_to_merge) :
  code(sc),
  sclass(cls),
  curpass(NULL),
  parser(p),
  dont_render(false),
  maxregsize(0),
  shname_token(shname),
  variant(_variant),
  opt(_opt),
  allRefStaticVars(all_ref_static_vars),
  needToMerge(need_to_merge)
{
  curvariant = sc.getCurPasses();
  G_ASSERT(curvariant);

  curvariant->pass.emplace();
  curpass = &*curvariant->pass;
  shBlockLev = ShaderStateBlock::LEV_SHADER;
}

void AssembleShaderEvalCB::error(const char *msg, const Terminal *const t)
{
  if (t)
  {
    eastl::string str = msg;
    if (!t->macro_call_stack.empty())
      str.append("\nCall stack:\n");
    for (auto it = t->macro_call_stack.rbegin(); it != t->macro_call_stack.rend(); ++it)
      str.append_sprintf("  %s()\n    %s(%i)\n", it->name, parser.get_lex_parser().get_filename(it->file), it->line);
    parser.get_lex_parser().set_error(t->file_start, t->line_start, t->col_start, str.c_str());
  }
  else
    parser.get_lex_parser().set_error(msg);
}

Register AssembleShaderEvalCB::add_reg(int type)
{
  switch (type)
  {
    case SHVT_BUFFER:
    case SHVT_TEXTURE: return add_resource_reg();
    case SHVT_INT4:
    case SHVT_COLOR4: return add_vec_reg();
    case SHVT_INT: return add_reg();
    case SHVT_FLOAT4X4: return add_vec_reg(4);
    default: G_ASSERT(0);
  }
  return {};
}

ShaderVarType shtok_to_shvt(int shtok)
{
  switch (shtok)
  {
    case SHADER_TOKENS::SHTOK_int: return SHVT_INT;
    case SHADER_TOKENS::SHTOK_int4: return SHVT_INT4;
    case SHADER_TOKENS::SHTOK_float: return SHVT_REAL;
    case SHADER_TOKENS::SHTOK_float4x4: return SHVT_FLOAT4X4;
    case SHADER_TOKENS::SHTOK_float4: return SHVT_COLOR4;
    case SHADER_TOKENS::SHTOK_texture: return SHVT_TEXTURE;
    default: G_ASSERT(0); return SHVT_INT;
  }
}

void AssembleShaderEvalCB::eval_static(static_var_decl &s)
{
  ShaderVarType t = shtok_to_shvt(s.type->type->num);

  int varNameId = VarMap::addVarId(s.name->text);

  int v = code.find_var(varNameId);
  if (v >= 0)
  {
    eastl::string message(eastl::string::CtorSprintf{}, "static variable '%s' already declared in ", s.name->text);
    message += parser.get_lex_parser().get_symbol_location(varNameId, SymbolType::STATIC_VARIABLE);
    error(message.c_str(), s.name);
    return;
  }
  parser.get_lex_parser().register_symbol(varNameId, SymbolType::STATIC_VARIABLE, s.name);
  v = append_items(code.vars, 1);
  code.vars[v].type = t;
  code.vars[v].nameId = varNameId;
  code.vars[v].terminal = s.name;
  code.vars[v].dynamic = s.mode && s.mode->mode->num == SHADER_TOKENS::SHTOK_dynamic;
  if (s.no_warnings)
    code.vars[v].noWarnings = true;

  bool inited = (!s.mode && s.init) || code.vars[v].dynamic || (t != SHVT_TEXTURE) || s.init;
  if (!inited)
    error(String(32, "Variable '%s' must be inited", s.name->text), s.name);

  bool varReferenced = true;

  if (!s.mode)
  {
    int intervalNameId = IntervalValue::getIntervalNameId(s.name->text);
    ShaderVariant::ExtType intervalIndex = allRefStaticVars.getIntervals()->getIntervalIndex(intervalNameId);
    const Interval *interv = allRefStaticVars.getIntervals()->getInterval(intervalIndex);
    varReferenced = interv ? allRefStaticVars.findType(interv->getVarType(), intervalIndex) != -1 : false;
    code.vars[v].used = varReferenced;
  }

  int sv = sclass.find_static_var(varNameId);
  if (sv < 0 && varReferenced)
  {
    sv = append_items(sclass.stvar, 1);
    sclass.stvar[sv].type = t;
    sclass.stvar[sv].nameId = varNameId;
    Color4 val(0, 0, 0, 1);
    if (s.init && s.init->expr)
    {
      ExpressionParser::getStatic().setOwner(this);
      if (!ExpressionParser::getStatic().parseConstExpression(*s.init->expr, val))
      {
        error("Wrong expression", s.name);
        return;
      }
    }
    switch (t)
    {
      case SHVT_COLOR4: sclass.stvar[sv].defval.c4 = val; break;
      case SHVT_REAL: sclass.stvar[sv].defval.r = val[0]; break;
      case SHVT_INT: sclass.stvar[sv].defval.i = real2int(val[0]); break;
      case SHVT_INT4:
        sclass.stvar[sv].defval.i4 = IPoint4(real2int(val[0]), real2int(val[1]), real2int(val[2]), real2int(val[3]));
        break;
      case SHVT_FLOAT4X4:
        // default value is not supported
        break;
      case SHVT_TEXTURE:
        if (real2int(val[0]) != 0)
        {
          error("texture may be inited only with 0", s.name);
          return;
        }
        sclass.stvar[sv].defval.texId = BAD_TEXTUREID;
        break;
      default: G_ASSERT(0);
    }
  }
  else if (sv >= 0)
  {
    if (sclass.stvar[sv].type != t)
    {
      error(String("static var '") + s.name->text + "' defined with different type", s.name);
      return;
    }
  }

  if (varReferenced)
  {
    int i = append_items(code.stvarmap, 1);
    code.stvarmap[i].v = v;
    code.stvarmap[i].sv = sv;
  }

  if (s.init && !s.init->expr)
    eval_init_stat(s.name, *s.init);
}

static String mangle(const char *name, const ShaderVariant::VariantInfo &variant)
{
  return String(0, "%s_stat_%p_dyn_%p", name, &variant.stat, variant.dyn);
}

// clang-format off
// clang-format linearizes this function
void AssembleShaderEvalCB::eval_bool_decl(bool_decl &decl)
{
  BoolVar::add(mangle(shname_token->text, variant), decl, parser);
}
// clang-format on

void AssembleShaderEvalCB::decl_bool_alias(const char *name, bool_expr &expr)
{
  bool_decl decl;
  SHTOK_ident ident;
  ident.file_start = 0;
  ident.line_start = 0;
  ident.col_start = 0;
  decl.name = &ident;
  decl.name->text = name;
  decl.expr = &expr;
  eval_bool_decl(decl);
}

void AssembleShaderEvalCB::eval_init_stat(SHTOK_ident *var, shader_init_value &v)
{
  int varNameId = VarMap::getVarId(var->text);

  int vi = code.find_var(varNameId);
  if (vi < 0)
  {
    error(String("unknown variable '") + var->text + "'", var);
    return;
  }

  if (v.color)
  {
    if (code.vars[vi].type != SHVT_COLOR4)
    {
      error(String("can't assign color to ") + ShUtils::shader_var_type_name(code.vars[vi].type), v.color->color);
      return;
    }
    int c;
    switch (v.color->color->num)
    {
      case SHADER_TOKENS::SHTOK_diffuse: c = SHCOD_DIFFUSE; break;
      case SHADER_TOKENS::SHTOK_emissive: c = SHCOD_EMISSIVE; break;
      case SHADER_TOKENS::SHTOK_specular: c = SHCOD_SPECULAR; break;
      case SHADER_TOKENS::SHTOK_ambient: c = SHCOD_AMBIENT; break;
      default: G_ASSERT(0);
    }
    int ident_id = IntervalValue::getIntervalNameId(var->text);
    int intervalIndex = variant.intervals.getIntervalIndex(ident_id);
    if (intervalIndex != INTERVAL_NOT_INIT && !code.vars[vi].dynamic)
    {
      int stVarId = sclass.find_static_var(varNameId);
      if (stVarId < 0)
      {
        error(String(256, "variable <%s> is not static var", var->text), var);
        return;
      }

      sclass.shInitCode.push_back(stVarId);
      sclass.shInitCode.push_back(shaderopcode::makeOp0(c));
      return;
    }

    code.initcode.push_back(vi);
    code.initcode.push_back(shaderopcode::makeOp0(c));
  }
  else if (v.tex)
  {
    if (code.vars[vi].type != SHVT_TEXTURE)
    {
      error(String("can't assign texture to ") + ShUtils::shader_var_type_name(code.vars[vi].type), v.tex->tex);
      return;
    }
    int ind;
    if (v.tex->tex_num)
      ind = semutils::int_number(v.tex->tex_num->text);
    else if (v.tex->tex_name && v.tex->tex_name->num == SHADER_TOKENS::SHTOK_diffuse)
      ind = 0;
    else
      G_ASSERT(0);

    code.vars[vi].slot = ind;

    int e_texture_ident_id = IntervalValue::getIntervalNameId(var->text);
    int intervalIndex = variant.intervals.getIntervalIndex(e_texture_ident_id);
    int stVarId = intervalIndex != INTERVAL_NOT_INIT ? sclass.find_static_var(varNameId) : -1;
    if (stVarId >= 0 && !code.vars[vi].dynamic)
    {
      int opcode = shaderopcode::makeOp2(SHCOD_TEXTURE, ind, 0);
      for (int i = 0; i < sclass.shInitCode.size(); i += 2)
        if (sclass.shInitCode[i] == stVarId)
        {
          if (sclass.shInitCode[i + 1] != opcode)
            error(String(256, "ambiguous init for static texture <%s> used in branching", var->text), v.tex->tex);
          return;
        }

      sclass.shInitCode.push_back(stVarId);
      sclass.shInitCode.push_back(opcode);
      return;
    }

    code.initcode.push_back(vi);
    code.initcode.push_back(shaderopcode::makeOp2(SHCOD_TEXTURE, ind, 0));
  }
  else
    G_ASSERT(0);
}

static inline int channel_type(int token)
{
  switch (token)
  {
    case SHADER_TOKENS::SHTOK_float1: return SCTYPE_FLOAT1;
    case SHADER_TOKENS::SHTOK_float2: return SCTYPE_FLOAT2;
    case SHADER_TOKENS::SHTOK_float3: return SCTYPE_FLOAT3;
    case SHADER_TOKENS::SHTOK_float4: return SCTYPE_FLOAT4;
    case SHADER_TOKENS::SHTOK_short2: return SCTYPE_SHORT2;
    case SHADER_TOKENS::SHTOK_short4: return SCTYPE_SHORT4;
    case SHADER_TOKENS::SHTOK_ubyte4: return SCTYPE_UBYTE4;
    case SHADER_TOKENS::SHTOK_color8: return SCTYPE_E3DCOLOR;
    case SHADER_TOKENS::SHTOK_half2: return SCTYPE_HALF2;
    case SHADER_TOKENS::SHTOK_half4: return SCTYPE_HALF4;
    case SHADER_TOKENS::SHTOK_short2n: return SCTYPE_SHORT2N;
    case SHADER_TOKENS::SHTOK_short4n: return SCTYPE_SHORT4N;
    case SHADER_TOKENS::SHTOK_ushort2n: return SCTYPE_USHORT2N;
    case SHADER_TOKENS::SHTOK_ushort4n: return SCTYPE_USHORT4N;
    case SHADER_TOKENS::SHTOK_udec3: return SCTYPE_UDEC3;
    case SHADER_TOKENS::SHTOK_dec3n: return SCTYPE_DEC3N;
    default: return -1;
  }
}

static inline int channel_usage(int token)
{
  switch (token)
  {
    case SHADER_TOKENS::SHTOK_pos: return SCUSAGE_POS;
    case SHADER_TOKENS::SHTOK_norm: return SCUSAGE_NORM;
    case SHADER_TOKENS::SHTOK_vcol: return SCUSAGE_VCOL;
    case SHADER_TOKENS::SHTOK_tc: return SCUSAGE_TC;
    case SHADER_TOKENS::SHTOK_extra: return SCUSAGE_EXTRA;
    default: return -1;
  }
}

void AssembleShaderEvalCB::eval_channel_decl(channel_decl &s, int str_idx)
{
  ShaderChannelId ch;

  int tp = ::channel_type(s.type->type->num);
  if (tp < 0)
  {
    error(String(128, "unsupported channel type: %s", s.type->type->text), s.type->type);
    return;
  }
  G_ASSERT(tp >= 0);
  ch.t = tp;
  int vbus = ::channel_usage(s.usg->usage->num);
  G_ASSERT(vbus >= 0);
  if (vbus == SCUSAGE_EXTRA || vbus == SCUSAGE_LIGHTMAP)
  {
    error("invalid channel usage", s.usg->usage);
    return;
  }
  ch.vbu = vbus;
  ch.vbui = 0;
  if (s.usgi)
  {
    G_ASSERT(s.usgi->index);
    ch.vbui = semutils::int_number(s.usgi->index->text);
  }

  int us = ::channel_usage(s.src_usg->usage->num);
  G_ASSERT(us >= 0);
  ch.u = us;
  ch.ui = 0;
  if (s.src_usgi)
  {
    G_ASSERT(s.src_usgi->index);
    ch.ui = semutils::int_number(s.src_usgi->index->text);
  }

  if (s.modifier)
  {
    switch (s.modifier->num)
    {
      case SHADER_TOKENS::SHTOK_signed_pack: ch.mod = CMOD_SIGNED_PACK; break;
      case SHADER_TOKENS::SHTOK_unsigned_pack: ch.mod = CMOD_UNSIGNED_PACK; break;
      case SHADER_TOKENS::SHTOK_bounding_pack: ch.mod = CMOD_BOUNDING_PACK; break;

      case SHADER_TOKENS::SHTOK_mul_1k: ch.mod = CMOD_MUL_1K; break;
      case SHADER_TOKENS::SHTOK_mul_2k: ch.mod = CMOD_MUL_2K; break;
      case SHADER_TOKENS::SHTOK_mul_4k: ch.mod = CMOD_MUL_4K; break;
      case SHADER_TOKENS::SHTOK_mul_8k: ch.mod = CMOD_MUL_8K; break;
      case SHADER_TOKENS::SHTOK_mul_16k: ch.mod = CMOD_MUL_16K; break;
      case SHADER_TOKENS::SHTOK_mul_32767: ch.mod = CMOD_SIGNED_SHORT_PACK; break;

      default: G_ASSERT(0);
    }

    // debug("modifier detected: %d", ch.mod);
  }
  ch.streamId = str_idx;
  code.channel.push_back(ch);
}


int AssembleShaderEvalCB::get_state_var(const Terminal &s, int &vt, bool &is_global, bool allow_not_found)
{
  is_global = false;
  int varNameId = VarMap::getVarId(s.text);

  int vi = code.find_var(varNameId);
  if (vi < 0)
  {
    // else try to find global variable
    vi = ShaderGlobal::get_var_internal_index(varNameId);
    if (vi <= -1)
    {
      if (!allow_not_found)
        error(String("unknown variable '") + s.text + "'", &s);
      return -2;
    }

    vt = ShaderGlobal::get_var(vi).type;
    is_global = true;

    return vi;
  }
  code.vars[vi].used = true;
  vt = code.vars[vi].type;
  return vi;
}


int AssembleShaderEvalCB::get_blend_k(const Terminal &s)
{
  const char *nm = s.text;
  if (strcmp(nm, "zero") == 0 || strcmp(nm, "0") == 0)
    return DRV3DC::BLEND_ZERO;
  else if (strcmp(nm, "one") == 0 || strcmp(nm, "1") == 0)
    return DRV3DC::BLEND_ONE;
  else if (strcmp(nm, "sc") == 0)
    return DRV3DC::BLEND_SRCCOLOR;
  else if (strcmp(nm, "isc") == 0)
    return DRV3DC::BLEND_INVSRCCOLOR;
  else if (strcmp(nm, "sa") == 0)
    return DRV3DC::BLEND_SRCALPHA;
  else if (strcmp(nm, "isa") == 0)
    return DRV3DC::BLEND_INVSRCALPHA;
  else if (strcmp(nm, "da") == 0)
    return DRV3DC::BLEND_DESTALPHA;
  else if (strcmp(nm, "ida") == 0)
    return DRV3DC::BLEND_INVDESTALPHA;
  else if (strcmp(nm, "dc") == 0)
    return DRV3DC::BLEND_DESTCOLOR;
  else if (strcmp(nm, "idc") == 0)
    return DRV3DC::BLEND_INVDESTCOLOR;
  else if (strcmp(nm, "sasat") == 0)
    return DRV3DC::BLEND_SRCALPHASAT;
  else if (strcmp(nm, "bf") == 0)
    return DRV3DC::BLEND_BLENDFACTOR;
  else if (strcmp(nm, "ibf") == 0)
    return DRV3DC::BLEND_INVBLENDFACTOR;
  else
  {
    error("unknown blend factor", &s);
    return -1;
  }
}

int AssembleShaderEvalCB::get_stensil_op_k(const Terminal &s)
{
  const char *nm = s.text;
  if (strcmp(nm, "keep") == 0)
    return DRV3DC::STNCLOP_KEEP;
  else if (strcmp(nm, "zero") == 0)
    return DRV3DC::STNCLOP_ZERO;
  else if (strcmp(nm, "replace") == 0)
    return DRV3DC::STNCLOP_REPLACE;
  else if (strcmp(nm, "incrsat") == 0)
    return DRV3DC::STNCLOP_INCRSAT;
  else if (strcmp(nm, "decrsat") == 0)
    return DRV3DC::STNCLOP_DECRSAT;
  else if (strcmp(nm, "incr") == 0)
    return DRV3DC::STNCLOP_INCR;
  else if (strcmp(nm, "dect") == 0)
    return DRV3DC::STNCLOP_DECR;
  else
  {
    error("unknown stencil op", &s);
    return -1;
  }
}

int AssembleShaderEvalCB::get_stencil_cmpf_k(const Terminal &s)
{
  const char *nm = s.text;
  if (strcmp(nm, "never") == 0)
    return DRV3DC::CMPF_NEVER;
  else if (strcmp(nm, "less") == 0)
    return DRV3DC::CMPF_LESS;
  else if (strcmp(nm, "equal") == 0)
    return DRV3DC::CMPF_EQUAL;
  else if (strcmp(nm, "lessequal") == 0)
    return DRV3DC::CMPF_LESSEQUAL;
  else if (strcmp(nm, "greater") == 0)
    return DRV3DC::CMPF_GREATER;
  else if (strcmp(nm, "notequal") == 0)
    return DRV3DC::CMPF_NOTEQUAL;
  else if (strcmp(nm, "greaterequal") == 0)
    return DRV3DC::CMPF_GREATEREQUAL;
  else if (strcmp(nm, "always") == 0)
    return DRV3DC::CMPF_ALWAYS;
  else
  {
    error("unknown stencil cmp func", &s);
    return -1;
  }
}

void AssembleShaderEvalCB::get_depth_cmpf_k(const Terminal &s, int &cmpf)
{
  const char *nm = s.text;
  if (strcmp(nm, "equal") == 0)
    cmpf = DRV3DC::CMPF_EQUAL;
  else if (strcmp(nm, "notequal") == 0)
    cmpf = DRV3DC::CMPF_NOTEQUAL;
  else if (strcmp(nm, "always") == 0)
    cmpf = DRV3DC::CMPF_ALWAYS;
  else
  {
    error("unknown depth cmp func", &s);
    cmpf = -1;
  }
}

int AssembleShaderEvalCB::get_bool_const(const Terminal &s)
{
  if (s.num == SHADER_TOKENS::SHTOK__true)
    return 1;
  else if (s.num == SHADER_TOKENS::SHTOK__false)
    return 0;
  else
    error("expected true or false", &s);
  return -1;
}

void AssembleShaderEvalCB::eval_blend_value(const Terminal &blend_func_tok, const SHTOK_intnum *const index_tok,
  ShaderSemCode::Pass::BlendFactors &blend_factors)
{
  auto blend_factor = get_blend_k(blend_func_tok);
  if (index_tok)
  {
    // independent blending is not supported on all platforms yet
    // exclude this error on platforms, where driver support introduced
#if !(_CROSS_TARGET_DX12 || _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL)
    error("independent blending is not supported on this platform", &blend_func_tok);
#endif

    auto index = semutils::int_number(index_tok->text);
    if (index < 0 || index >= shaders::RenderState::NumIndependentBlendParameters)
    {
      error(String(128,
              "blend factor index must be less than %d. Can be increased if needed. See: "
              "shaders::RenderState::NumIndependentBlendParameters",
              shaders::RenderState::NumIndependentBlendParameters),
        &blend_func_tok);
    }

    blend_factors[index] = blend_factor;
    curpass->independent_blending = true;
  }
  else
  {
    eastl::fill(blend_factors.begin(), blend_factors.end(), blend_factor);
  }
}

void AssembleShaderEvalCB::eval_state(state_stat &s)
{
  if (s.var)
  {

    if (!s.value)
    {
      error("expected value", s.var->var);
      return;
    }

    bool supports_indexing =
      (s.var->var->num == SHADER_TOKENS::SHTOK_blend_src) || (s.var->var->num == SHADER_TOKENS::SHTOK_blend_dst) ||
      (s.var->var->num == SHADER_TOKENS::SHTOK_blend_asrc) || (s.var->var->num == SHADER_TOKENS::SHTOK_blend_adst) ||
      (s.var->var->num == SHADER_TOKENS::SHTOK_color_write);

    if (s.index && !supports_indexing)
    {
      error(String(0, "%s[%d] is not allowed (index is not supported for this attribute", s.var->var->text,
              semutils::int_number(s.index->text)),
        s.var->var);
      return;
    }

    unsigned color_write_mask = 0;
    switch (s.var->var->num)
    {
      case SHADER_TOKENS::SHTOK_blend_src: eval_blend_value(*s.value->value, s.index, curpass->blend_src); break;
      case SHADER_TOKENS::SHTOK_blend_dst: eval_blend_value(*s.value->value, s.index, curpass->blend_dst); break;
      case SHADER_TOKENS::SHTOK_blend_asrc: eval_blend_value(*s.value->value, s.index, curpass->blend_asrc); break;
      case SHADER_TOKENS::SHTOK_blend_adst: eval_blend_value(*s.value->value, s.index, curpass->blend_adst); break;
      case SHADER_TOKENS::SHTOK_alpha_to_coverage: curpass->alpha_to_coverage = get_bool_const(*s.value->value); break;

      // Zero based. 0 means one instance which is the usual non-instanced case.
      case SHADER_TOKENS::SHTOK_view_instances: curpass->view_instances = semutils::int_number(s.value->value->text) - 1; break;

      case SHADER_TOKENS::SHTOK_cull_mode:
      {
        const char *v = s.value->value->text;
        if (strcmp(v, "ccw") == 0)
          curpass->cull_mode = CULL_CCW;
        else if (strcmp(v, "cw") == 0)
          curpass->cull_mode = CULL_CW;
        else if (strcmp(v, "none") == 0)
          curpass->cull_mode = CULL_NONE;
        else
          error(String(128, "unknown cull mode <%s>", s.value->value->text), s.value->value);
      }
      break;

      case SHADER_TOKENS::SHTOK_stencil: curpass->stencil = get_bool_const(*s.value->value); break;

      case SHADER_TOKENS::SHTOK_stencil_func: curpass->stencil_func = get_stencil_cmpf_k(*s.value->value); break;

      case SHADER_TOKENS::SHTOK_stencil_ref:
      {
        int val = semutils::int_number(s.value->value->text);
        if (val < 0)
          val = 0;
        else if (val > 255)
          val = 255;
        curpass->stencil_ref = val;
      }
      break;

      case SHADER_TOKENS::SHTOK_stencil_pass: curpass->stencil_pass = get_stensil_op_k(*s.value->value); break;

      case SHADER_TOKENS::SHTOK_stencil_fail: curpass->stencil_fail = get_stensil_op_k(*s.value->value); break;

      case SHADER_TOKENS::SHTOK_stencil_zfail: curpass->stencil_zfail = get_stensil_op_k(*s.value->value); break;

      case SHADER_TOKENS::SHTOK_color_write:
        color_write_mask = 0;
        if (!s.value->value)
          error("color_write should be a value of int, rgba swizzle, true or false", s.var->var);
        else if (s.value->value->num == SHADER_TOKENS::SHTOK__true || s.value->value->num == SHADER_TOKENS::SHTOK__false)
          color_write_mask = get_bool_const(*s.value->value) ? 0xF : 0;
        else if (s.value->value->num == SHADER_TOKENS::SHTOK_intnum)
        {
          unsigned val = semutils::int_number(s.value->value->text);
          if (val & (~15))
            error("color_write should be value of [0 15]", s.var->var);
          color_write_mask = val & 15;
        }
        else if (s.value->value->num == SHADER_TOKENS::SHTOK_ident && s.static_var)
        {
          if (s.index)
          {
            error(String(0, "color_write from static var \"%s\" cannot be used with index", s.value->value->text), s.var->var);
            return;
          }

          bool is_global = false;
          int vt, vi = get_state_var(*s.value->value, vt, is_global);
          if (vi < -1)
          {
            error(String(0, "missing var \"%s\" for color_write statement", s.value->value->text), s.var->var);
            return;
          }
          if (is_global || code.vars[vi].dynamic || vt != SHVT_INT)
          {
            error(String(0, "static int var \"%s\" required for color_write statement", s.value->value->text), s.var->var);
            return;
          }

          int varNameId = VarMap::getVarId(s.value->value->text);
          int sv = sclass.find_static_var(varNameId);
          curpass->color_write = sclass.stvar[sv].defval.i;
          break;
        }
        else if (s.value->value->num == SHADER_TOKENS::SHTOK_ident)
        {
          unsigned val = 0, val2 = 0;
          const char *valStr = s.value->value->text;
          for (; valStr && valStr[0];)
          {
            unsigned pval = val;
            char swizzle[] = "rgba";
            for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1)
              if (!(val & bit) && valStr[0] == swizzle[i])
              {
                val |= bit;
                valStr++;
              }
            if (pval == val)
              break;
          }

          if (valStr[0] != 0)
            error("color_write should be swizzle of 'rgba' ", s.var->var);

          color_write_mask = val;
        }
        else if (!s.value->value)
        {
          error("color_write should be a value of int, rgba swizzle, true or false", s.var->var);
          break;
        }

        if (!s.index)
        {
          curpass->color_write = color_write_mask;
          for (int i = 1; i < 8; i++)
            curpass->color_write |= color_write_mask << (i * 4);
        }
        else
        {
          int idx = semutils::int_number(s.index->text);
          if (idx >= 0 && idx < 8)
          {
            curpass->color_write &= ~(0xF << (idx * 4));
            curpass->color_write |= color_write_mask << (idx * 4);
          }
          else
            error(String(0, "bad index for %s[%d]", s.var->var->text, idx), s.var->var);
        }
        break;

      case SHADER_TOKENS::SHTOK_z_test: curpass->z_test = get_bool_const(*s.value->value); break;

      case SHADER_TOKENS::SHTOK_z_func:
      {
        get_depth_cmpf_k(*s.value->value, curpass->z_func);
        break;
      }

      case SHADER_TOKENS::SHTOK_z_write:
        if (curpass->z_write >= 0)
          error("z_write already specified for this render pass", s.var->var);
        else if (s.value->value->num == SHADER_TOKENS::SHTOK__true || s.value->value->num == SHADER_TOKENS::SHTOK__false)
        {
          curpass->z_write = get_bool_const(*s.value->value);
        }
        else if (s.value->value->num == SHADER_TOKENS::SHTOK_intnum)
        {
          int val = semutils::int_number(s.value->value->text);
          curpass->z_write = val ? 1 : 0;
        }
        else
          error("expected boolean, integer number or int variable", s.value->value);
        break;

      case SHADER_TOKENS::SHTOK_slope_z_bias:
      case SHADER_TOKENS::SHTOK_z_bias:
      {
        bool slope = (s.var->var->num == SHADER_TOKENS::SHTOK_slope_z_bias);
        if ((!slope && curpass->z_bias) || (slope && curpass->slope_z_bias))
          error(String(slope ? "slope_" : "") + "z_bias already specified for this render pass", s.var->var);
        else if (s.value->value->num == SHADER_TOKENS::SHTOK_float || s.value->value->num == SHADER_TOKENS::SHTOK_intnum)
        {
          real val = semutils::real_number(s.value->value->text);
          if (slope)
          {
            curpass->slope_z_bias = true;
            curpass->slope_z_bias_val = val;
          }
          else
          {
            curpass->z_bias = true;
            curpass->z_bias_val = val;
          }
        }
        else
          error("expected real number or real variable", s.value->value);
      }
      break;

      default: G_ASSERT(0);
    }
  }
  else
    G_ASSERT(0);
}

void AssembleShaderEvalCB::eval_zbias_state(zbias_state_stat &s)
{
  switch (s.var->num)
  {
    case SHADER_TOKENS::SHTOK_slope_z_bias:
    case SHADER_TOKENS::SHTOK_z_bias:
    {
      bool slope = (s.var->num == SHADER_TOKENS::SHTOK_slope_z_bias);
      if ((!slope && curpass->z_bias) || (slope && curpass->slope_z_bias))
        error(String(slope ? "slope_" : "") + "z_bias already specified for this render pass", s.var);
      else if (s.const_value)
      {
        real val = semutils::real_number(s.const_value);
        if (slope)
        {
          curpass->slope_z_bias = true;
          curpass->slope_z_bias_val = val;
        }
        else
        {
          curpass->z_bias = true;
          curpass->z_bias_val = val;
        }
      }
      else
        error("expected real number or real variable", s.value);
    }
    break;
    default: G_ASSERT(0);
  }
}

static const char *profiles[] = {"cs", "ps", "vs", nullptr};

void AssembleShaderEvalCB::eval_external_block(external_state_block &state_block)
{
  ShaderStage stage = STAGE_MAX;
  for (int i = 0; profiles[i]; ++i)
    if (strcmp(profiles[i], state_block.scope->text) == 0)
    {
      stage = (ShaderStage)i;
      break;
    }
  if (stage >= STAGE_MAX)
    error(String(32, "external block <%s> is not one of <vs, ps, cs>", state_block.scope->text), state_block.scope);

  auto eval_if_stat = [this, stage](state_block_if_stat &s, auto &&eval_if_stat) -> void {
    G_ASSERT(s.expr);

    auto eval_stats = [&](auto &stats) {
      for (auto &stat : stats)
      {
        if (stat->state_block_if_stat)
          eval_if_stat(*stat->state_block_if_stat, eval_if_stat);
        else
          eval_external_block_stat(*stat, stage);
      }
    };

    int res = eval_if(*s.expr);
    G_ASSERT(res != ShaderEvalCB::IF_BOTH);
    if (res == ShaderEvalCB::IF_TRUE)
    {
      eval_stats(s.true_stat);
    }
    else
    {
      eval_stats(s.false_stat);
      if (s.else_if)
        eval_if_stat(*s.else_if, eval_if_stat);
    }
  };

  for (auto stat : state_block.state_block_stat)
  {
    if (stat->state_block_if_stat)
      eval_if_stat(*stat->state_block_if_stat, eval_if_stat);
    else
      eval_external_block_stat(*stat, stage);
  }
}

#define TYPE_LIST_FLOAT() TYPE(f1) TYPE(f2) TYPE(f3) TYPE(f4) TYPE(f44)
#define TYPE_LIST_INT()   TYPE(i1) TYPE(i2) TYPE(i3) TYPE(i4)
#define TYPE_LIST_BUF()   TYPE(buf) TYPE(cbuf)
#define TYPE_LIST_TEX()   TYPE(tex) TYPE(tex2d) TYPE(tex3d) TYPE(texArray) TYPE(texCube) TYPE(texCubeArray)
#define TYPE_LIST_SMP()   TYPE(smp) TYPE(smp2d) TYPE(smp3d) TYPE(smpArray) TYPE(smpCube) TYPE(smpCubeArray)
#define TYPE_LIST_SHD() \
  TYPE(shd) TYPE(shdArray) TYPE(staticCube) TYPE(staticTexArray) TYPE(staticTex3D) TYPE(staticCubeArray) TYPE(uav)

#define TYPE_LIST TYPE_LIST_FLOAT() TYPE_LIST_INT() TYPE_LIST_BUF() TYPE_LIST_TEX() TYPE_LIST_SMP() TYPE_LIST_SHD()

enum class VariableType
{
  Unknown,
#define TYPE(type) type,
  TYPE_LIST
#undef TYPE
    staticSampler
};

static eastl::vector_map<eastl::string_view, VariableType> g_var_types = {
#define TYPE(type) {"@" #type, VariableType::type},
  TYPE_LIST
#undef TYPE
  {"@static", VariableType::staticSampler}};

static VariableType name_space_to_type(const char *name_space)
{
  auto found = g_var_types.find(name_space);
  if (found == g_var_types.end())
    return VariableType::Unknown;

  return found->second;
}

void AssembleShaderEvalCB::eval_external_block_stat(state_block_stat &state_block, ShaderStage stage)
{
  auto &block = externalStateBlocks[(int)stage];

  G_ASSERT(state_block.var || state_block.arr || state_block.reg);
  if (!state_block.var)
    return handle_external_block_stat(state_block, stage);

  VariableType type = name_space_to_type(state_block.var->var->nameSpace->text);
  // clang-format off
  switch (type)
  {
    case VariableType::f1:
    case VariableType::f2:
    case VariableType::f3:
    case VariableType::i1:
    case VariableType::i2:
    case VariableType::i3:
      break;
    default:
      return handle_external_block_stat(state_block, stage);
  }
  // clang-format on
  if (state_block.var->val->builtin_var || !needToMerge)
    return handle_external_block_stat(state_block, stage);

  if (state_block.var->val->expr)
  {
    auto is_expr_dynamic = [&]() {
      ExpressionParser::getStatic().setOwner(this);
      ComplexExpression colorExpr(shexpr::VT_COLOR4);

      if (!ExpressionParser::getStatic().parseExpression(*state_block.var->val->expr, &colorExpr))
        return false;
      if (!colorExpr.collapseNumbers())
        return false;

      Tab<int> cod(tmpmem);
      Register dr = add_vec_reg();

      Color4 v;
      if (colorExpr.evaluate(v))
        return false;

      // this is an expression - assembly it for future calculation
      if (!colorExpr.assembly(*this, cod, dr, false))
        return false;

      stVarToReg.clear();
      return colorExpr.isDynamic();
    };

    if (!is_expr_dynamic())
      return handle_external_block_stat(state_block, stage);
  }

  block.emplace_back(state_block);
}

void AssembleShaderEvalCB::handle_external_block_stat(state_block_stat &state_block, ShaderStage stage)
{
  static const int float4_to_cod[STAGE_MAX] = {SHCOD_CS_CONST, SHCOD_FSH_CONST, SHCOD_VPR_CONST};
  static const NamedConstSpace float4_to_namespace[STAGE_MAX] = {NamedConstSpace::csf, NamedConstSpace::psf, NamedConstSpace::vsf};
  static const NamedConstSpace buffer_to_namespace[STAGE_MAX] = {
    NamedConstSpace::cs_buf, NamedConstSpace::ps_buf, NamedConstSpace::vs_buf};
  static const NamedConstSpace cbuffer_to_namespace[STAGE_MAX] = {
    NamedConstSpace::cs_cbuf, NamedConstSpace::ps_cbuf, NamedConstSpace::vs_cbuf};

  auto isBindlessType = [](VariableType type) -> bool {
    return type == VariableType::staticSampler || type == VariableType::staticTex3D || type == VariableType::staticCube ||
           type == VariableType::staticCubeArray || type == VariableType::staticTexArray;
  };

  String def_hlsl;

  {
    Terminal *var = nullptr, *nameSpace = nullptr;
    Terminal *shader_var = nullptr;
    Terminal *builtin_var = nullptr;
    SHTOK_hlsl_text *hlsl = nullptr;
    G_ASSERT(state_block.var || state_block.arr || state_block.reg);
    if (state_block.var)
    {
      var = state_block.var->var->name;
      nameSpace = state_block.var->var->nameSpace;
      hlsl = state_block.var->hlsl_var_text;
      if (state_block.var->val->expr)
        shader_var = state_block.var->val->expr->lhs->lhs->var_name;
      builtin_var = state_block.var->val->builtin_var;
    }
    else if (state_block.arr)
    {
      var = state_block.arr->var->name;
      nameSpace = state_block.arr->var->nameSpace;
      hlsl = state_block.arr->hlsl_var_text;
    }
    else
    {
      var = state_block.reg->var->name;
      nameSpace = state_block.reg->var->nameSpace;
      hlsl = state_block.reg->hlsl_var_text;
      shader_var = state_block.reg->shader_var;
    }
    G_ASSERT(var);
    int conflict_blk_num = 0;
    for (int i = 0; i < shConst.suppBlk.size(); i++)
    {
      if (stage == STAGE_VS)
      {
        if (shConst.suppBlk[i]->getVsNameId(var->text) != -1)
          conflict_blk_num++;
      }
      else
      {
        if (shConst.suppBlk[i]->getPsNameId(var->text) != -1)
          conflict_blk_num++;
      }
    }
    if (conflict_blk_num && conflict_blk_num == shConst.suppBlk.size())
    {
      error(String(32, "named const <%s%s> conflicts with one in <%s> block", var->text, nameSpace->text,
              shConst.suppBlk[0]->name.c_str()),
        var);
      return;
    }
    else if (conflict_blk_num)
      warning(String(32, "named const <%s%s> hides one in supported non-compatible blocks", var->text, nameSpace->text), var);

    if (shConst.globConstBlk)
    {
      bool conflict_blk = false;
      if (stage == STAGE_VS)
      {
        conflict_blk = shConst.globConstBlk->getVsNameId(var->text) != -1;
      }
      else
      {
        conflict_blk = shConst.globConstBlk->getPsNameId(var->text) != -1;
      }
      if (conflict_blk)
        error(String(32, "named const <%s%s> conflicts with global const block <%s>", var->text, nameSpace->text,
                shConst.globConstBlk->name.c_str()),
          var);
    }

    const VariableType type = name_space_to_type(nameSpace->text);
    if (type == VariableType::Unknown)
    {
      error(String(32, "named const <%s> has unknown type <%s> block", var->text, nameSpace->text), nameSpace);
      return;
    }

    // validate hlsl existence
    if ((type == VariableType::buf || type == VariableType::cbuf || type == VariableType::tex || type == VariableType::smp) && !hlsl)
      error(String(32, "named const <%s> has type of <%s> and so requires hlsl", var->text, nameSpace->text), nameSpace);

    const bool isBindless = isBindlessType(type) && enableBindless;
    String varName(var->text);
    // This is a hacky way to support bindless textures in vertex shaders.
    // Anyway we store all bindless indices and constants for both stages in the same const buffer.
    // TODO: Make a single propertyCollection for static parameters for vertex/fragment shaders pair. It should simplify the code.
    ShaderStage orig_stage = stage;
    if (isBindless)
    {
      G_ASSERT(stage == STAGE_PS || stage == STAGE_VS);
      varName = String(0, "%s_%s_bindless_id", var->text, stage == STAGE_PS ? "ps" : "vs");
      stage = STAGE_PS;
    }
    if (isBindlessType(type))
    {
      int vt;
      bool is_global;
      int var_id = get_state_var(*shader_var, vt, is_global);
      if (is_global)
        error(String(32, "@static texture %s can't be global", var->text), nameSpace);
      else if (code.vars[var_id].dynamic)
        error(String(32, "@static texture %s can't be dynamic", var->text), nameSpace);
      if (!is_global && !code.vars[var_id].dynamic)
      {
        switch (type)
        {
          case VariableType::staticSampler: code.vars[var_id].texType = ShaderVarTextureType::SHVT_TEX_2D; break;
          case VariableType::staticTex3D: code.vars[var_id].texType = ShaderVarTextureType::SHVT_TEX_3D; break;
          case VariableType::staticCube: code.vars[var_id].texType = ShaderVarTextureType::SHVT_TEX_CUBE; break;
          case VariableType::staticTexArray: code.vars[var_id].texType = ShaderVarTextureType::SHVT_TEX_2D_ARRAY; break;
          case VariableType::staticCubeArray: code.vars[var_id].texType = ShaderVarTextureType::SHVT_TEX_CUBE_ARRAY; break;
          default: break;
        }
      }
    }

    int shcod = -1;
    int shtype = -1;
    NamedConstSpace name_space = NamedConstSpace::unknown;
    switch (type)
    {
      case VariableType::f1:
      case VariableType::f2:
      case VariableType::f3:
      case VariableType::f4:
      case VariableType::f44:
        shcod = float4_to_cod[stage];
        shtype = SHVT_COLOR4;
        name_space = float4_to_namespace[stage];
        break;
      case VariableType::i1:
      case VariableType::i2:
      case VariableType::i3:
      case VariableType::i4:
        shcod = float4_to_cod[stage];
        shtype = SHVT_INT4;
        name_space = float4_to_namespace[stage];
        break;
      case VariableType::tex: // this one is supposed to place texture in same numeration as buffer, i.e. t16+ and doesn't have sampler
      case VariableType::tex2d:
      case VariableType::tex3d:
      case VariableType::texArray:
      case VariableType::texCube:
      case VariableType::texCubeArray:
        shcod = stage == STAGE_VS ? SHCOD_TEXTURE_VS : SHCOD_TEXTURE;
        shtype = SHVT_TEXTURE;
        name_space = buffer_to_namespace[stage];
        break;

      case VariableType::smp:
      case VariableType::smp2d:
      case VariableType::smp3d:
      case VariableType::smpArray:
      case VariableType::smpCube:
      case VariableType::smpCubeArray:
      case VariableType::shd:
      case VariableType::shdArray:
        shcod = stage == STAGE_VS ? SHCOD_TEXTURE_VS : SHCOD_TEXTURE;
        shtype = SHVT_TEXTURE;
        name_space = stage == STAGE_VS ? NamedConstSpace::vsmp : NamedConstSpace::smp;
        break;
      case VariableType::staticSampler:
      case VariableType::staticCube:
      case VariableType::staticCubeArray:
      case VariableType::staticTexArray:
      case VariableType::staticTex3D:
        shcod = isBindless ? SHCOD_REG_BINDLESS : (stage == STAGE_VS ? SHCOD_TEXTURE_VS : SHCOD_TEXTURE);
        shtype = SHVT_TEXTURE;
        name_space = isBindless ? (stage == STAGE_VS ? NamedConstSpace::vsf : NamedConstSpace::psf)
                                : (stage == STAGE_VS ? NamedConstSpace::vsmp : NamedConstSpace::smp);
        break;
      case VariableType::cbuf:
        shcod = SHCOD_CONST_BUFFER;
        shtype = SHVT_BUFFER;
        name_space = cbuffer_to_namespace[stage];
        break;
      case VariableType::buf:
        shcod = SHCOD_BUFFER;
        shtype = SHVT_BUFFER;
        name_space = buffer_to_namespace[stage];
        break;
      case VariableType::uav:
      {
        G_ASSERT(shader_var);
        if (!state_block.reg)
        {
          bool is_global;
          int vi = get_state_var(*shader_var, shtype, is_global);
          G_ASSERT(vi > -1);
          if (shtype != SHVT_TEXTURE && shtype != SHVT_BUFFER)
            error("@uav should be declared with texture or buffer only", var);
        }
        shcod = shtype == SHVT_TEXTURE ? SHCOD_RWTEX : SHCOD_RWBUF;
        name_space = NamedConstSpace::uav;
        break;
      }
      default: G_ASSERTF(0, "unhandled type <%s>", nameSpace->text);
    }
    //== don't use eastl::vector here since it allocates memory with RegionMemAlloc, leading to 15Gb mem consumption
    Tab<external_var_value_single *> initializer;
    RegionMemAlloc rm_alloc(4 << 20, 4 << 20);
    if (state_block.arr)
    {
      initializer.push_back(state_block.arr->arr0);
      append_items(initializer, state_block.arr->arrN.size(), state_block.arr->arrN.data());
    }
    else if (state_block.var && state_block.var->par)
    {
      if (!shader_var)
        error("Wrong array syntax", var);
      int vt;
      bool is_global;
      int vi = get_state_var(*shader_var, vt, is_global);
      const ShaderGlobal::Var &v = ShaderGlobal::get_var(vi);
      for (int i = 0; i < v.array_size; i++)
      {
        eastl::string name = v.getName();
        if (i > 0)
        {
          name = eastl::string(eastl::string::CtorSprintf{}, "%s[%i]", name.c_str(), i);
        }
        external_var_value_single *value = new (&rm_alloc) external_var_value_single;
        value->builtin_var = nullptr;
        value->expr = new (&rm_alloc) arithmetic_expr;
        value->expr->lhs = new (&rm_alloc) arithmetic_expr_md;
        value->expr->lhs->lhs = new (&rm_alloc) arithmetic_operand;
        value->expr->lhs->lhs->unary_op = nullptr;
        value->expr->lhs->lhs->real_value = nullptr;
        value->expr->lhs->lhs->color_value = nullptr;
        value->expr->lhs->lhs->func = nullptr;
        value->expr->lhs->lhs->expr = nullptr;
        value->expr->lhs->lhs->cmask = nullptr;
        value->expr->lhs->lhs->var_name = new (&rm_alloc) SHTOK_ident;
        value->expr->lhs->lhs->var_name->text = str_dup(name.data(), &rm_alloc);
        initializer.emplace_back(value);
      }
    }
    else if (state_block.var)
    {
      initializer.push_back(state_block.var->val);
    }

    int registers_count = initializer.size();
    if (state_block.arr && !state_block.arr->par &&
        ((type == VariableType::f44 && initializer.size() > 4) || (type != VariableType::f44 && initializer.size() > 1)))
      error(String(64, "variable <%s> is of type <%s> requires [] for initialization with %d vectors", var->text, nameSpace->text,
              initializer.size()),
        var);

    bool is_matrix_rvalue = false;
    if (type == VariableType::f44)
    {
      if (!builtin_var && initializer.size() % 4)
      {
        if (initializer.size() == 1 && initializer[0]->expr && initializer[0]->expr->lhs->lhs->var_name)
        {
          int var_type;
          bool is_global;
          get_state_var(*initializer[0]->expr->lhs->lhs->var_name, var_type, is_global);
          is_matrix_rvalue = var_type == SHVT_FLOAT4X4;
          if (is_matrix_rvalue)
            shtype = SHVT_FLOAT4X4;
        }
        if (!is_matrix_rvalue)
          error(String(64,
                  "variable <%s> is of type float4x4 and is supposed to be assigned from globtm or"
                  " array of quadruple vectors (assigned array has %d vectors now)",
                  var->text, initializer.size()),
            var);
      }
      registers_count = initializer.size() > 4 ? initializer.size() : 4;
    }
    if (type != VariableType::f4 && type != VariableType::i4 && type != VariableType::f44 &&
        (initializer.size() > 1 || (state_block.arr && state_block.arr->par)))
    {
      error(String(64, "variable <%s> is of type <%s> and not @f4, @f44, @i4 and so can't be array ", var->text, nameSpace->text),
        var);
    }

    int hardcoded_reg = -1;
    if (state_block.reg)
    {
      registers_count = 1;

      bool is_global;
      int reg_type;
      int vi = get_state_var(*shader_var, reg_type, is_global);
      G_ASSERT(vi >= 0);
      if (reg_type != SHVT_INT)
        error(String(0, "Hardcoded register %s must be of type `int`, but found %s", shader_var->text,
                ShUtils::shader_var_type_name(reg_type)),
          shader_var);
      if (is_global)
      {
        hardcoded_reg = ShaderGlobal::get_var(vi).value.i;
        ShaderGlobal::get_var(vi).isAlwaysReferenced = true;
      }
      else
        error("Only global variables for hardcoded registers are supported", shader_var);
    }

    const char *var_type_str = nullptr;
    switch (type)
    {
      case VariableType::f1: var_type_str = "float"; break;
      case VariableType::f2: var_type_str = "float2"; break;
      case VariableType::f3: var_type_str = "float3"; break;
      case VariableType::f4: var_type_str = "float4"; break;
      case VariableType::i1: var_type_str = "int"; break;
      case VariableType::i2: var_type_str = "int2"; break;
      case VariableType::i3: var_type_str = "int3"; break;
      case VariableType::i4: var_type_str = "int4"; break;
      case VariableType::f44: var_type_str = "float4x4"; break;
      case VariableType::tex2d: var_type_str = "Texture2D"; break;
      case VariableType::staticTexArray:
      case VariableType::texArray: var_type_str = "Texture2DArray"; break;
      case VariableType::staticTex3D:
      case VariableType::tex3d: var_type_str = "Texture3D"; break;
      case VariableType::staticCube:
      case VariableType::texCube: var_type_str = "TextureCube"; break;
      case VariableType::staticCubeArray:
      case VariableType::texCubeArray: var_type_str = "TextureCubeArray"; break;
      case VariableType::staticSampler:
      case VariableType::smp2d: var_type_str = "Texture2D"; break;
      case VariableType::smp3d: var_type_str = "Texture3D"; break;
      case VariableType::smpCube: var_type_str = "TextureCube"; break;
#if (_CROSS_TARGET_C1 | _CROSS_TARGET_C2)



#else
      case VariableType::shdArray:
      case VariableType::smpArray: var_type_str = "Texture2DArray"; break;
      case VariableType::smpCubeArray: var_type_str = "TextureCubeArray"; break;
#endif
      case VariableType::shd: var_type_str = "Texture2D"; break;
    }
    def_hlsl.clear();
    if (hlsl)
      def_hlsl = hlsl->text;

    const char *found_namespace = def_hlsl.find(nameSpace->text);

    while (char *p = strstr(def_hlsl, "@buf"))
      def_hlsl.insert(p - def_hlsl.data() + 1, String(0, "%s_", profiles[stage]));
    while (char *p = strstr(def_hlsl, "@cbuf"))
      def_hlsl.insert(p - def_hlsl.data() + 1, String(0, "%s_", profiles[stage]));
    while (char *p = strstr(def_hlsl, "@tex"))
      def_hlsl.insert(p - def_hlsl.data() + 1, String(0, "%s_", profiles[stage]));

    if (var_type_str)
    {
      if (!def_hlsl.empty())
        def_hlsl += '\n';
      char tempbuf[512];
      tempbuf[0] = 0;
      if ((type == VariableType::i4 || type == VariableType::f4) && initializer.size() > 1)
        SNPRINTF(tempbuf, sizeof(tempbuf), "%s %s[%d]: register($%s%s);", var_type_str, var->text, (int)initializer.size(), var->text,
          NamedConstBlock::nameSpaceToStr(name_space));
      else if (type == VariableType::f44 && state_block.arr && initializer.size() > 4)
        SNPRINTF(tempbuf, sizeof(tempbuf), "%s %s[%d]: register($%s%s);", var_type_str, var->text, (int)initializer.size() / 4,
          var->text, NamedConstBlock::nameSpaceToStr(name_space));
      else if (type == VariableType::f44 && state_block.arr && initializer.size() == 4)
        SNPRINTF(tempbuf, sizeof(tempbuf), "%s %s: register($%s%s);", var_type_str, var->text, var->text,
          NamedConstBlock::nameSpaceToStr(name_space));
      else if ((type == VariableType::staticSampler || type == VariableType::staticCube || type == VariableType::staticTexArray) &&
               isBindless)
        SNPRINTF(tempbuf, sizeof(tempbuf), "uint2 %s: register($%s%s);", varName.c_str(), var->text,
          NamedConstBlock::nameSpaceToStr(name_space));
      else
      {
        if (type == VariableType::tex2d || type == VariableType::texArray || type == VariableType::tex3d ||
            type == VariableType::texCube || type == VariableType::texCubeArray)
          SNPRINTF(tempbuf, sizeof(tempbuf), "%s %s%s;", var_type_str, var->text, String(0, "@%s_tex", profiles[stage]).c_str());
        else
          SNPRINTF(tempbuf, sizeof(tempbuf), "%s %s%s;", var_type_str, var->text, NamedConstBlock::nameSpaceToStr(name_space));
        def_hlsl += tempbuf;
        tempbuf[0] = 0;

#if (_CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL)
        if (type == VariableType::shd || type == VariableType::shdArray)
          def_hlsl.aprintf(0, "SamplerComparisonState %s_cmpSampler:register($%s@shd);", var->text, var->text);
#endif
        if (type == VariableType::staticSampler)
          def_hlsl.aprintf(0,
            "TextureSampler get_%s()\n"
            "{\n"
            "  TextureSampler texSamp;\n"
            "  texSamp.tex = %s;\n"
            "  texSamp.smp = %s_samplerstate;\n"
            "  return texSamp;\n"
            "}\n",
            var->text, var->text, var->text, var->text, var->text);
        if (type == VariableType::staticCube)
          def_hlsl.aprintf(0,
            "TextureSamplerCube get_%s()\n"
            "{\n"
            "  TextureSamplerCube texSamp;\n"
            "  texSamp.tex = %s;\n"
            "  texSamp.smp = %s_samplerstate;\n"
            "  return texSamp;\n"
            "}\n",
            var->text, var->text, var->text);
        if (type == VariableType::staticCubeArray)
          def_hlsl.aprintf(0,
            "TextureCubeArraySampler get_%s()\n"
            "{\n"
            "  TextureCubeArraySampler texSamp;\n"
            "  texSamp.tex = %s;\n"
            "  texSamp.smp = %s_samplerstate;\n"
            "  return texSamp;\n"
            "}\n",
            var->text, var->text, var->text);
        if (type == VariableType::staticTexArray)
          def_hlsl.aprintf(0,
            "TextureArraySampler get_%s()\n"
            "{\n"
            "  TextureArraySampler texSamp;\n"
            "  texSamp.tex = %s;\n"
            "  texSamp.smp = %s_samplerstate;\n"
            "  return texSamp;\n"
            "}\n",
            var->text, var->text, var->text);
        if (type == VariableType::staticTex3D)
          def_hlsl.aprintf(0,
            "Texture3DSampler get_%s()\n"
            "{\n"
            "  Texture3DSampler texSamp;\n"
            "  texSamp.tex = %s;\n"
            "  texSamp.smp = %s_samplerstate;\n"
            "  return texSamp;\n"
            "}\n",
            var->text, var->text, var->text);
      }
      if (tempbuf[0])
        def_hlsl += tempbuf;
    }
    else if (!found_namespace)
    {
      error(String(32, "named const <%s> must contain the namespace `%s`", var->text, nameSpace->text), var);
    }

    if (type == VariableType::smp2d && shader_var)
    {
      int vt;
      bool is_global;
      int var_id = get_state_var(*shader_var, vt, is_global);
      if (!is_global && !code.vars[var_id].dynamic)
        error(String(0, "Texture %s in material should use @static.", shader_var->text), var);
    }

    if (!def_hlsl.empty())
    {
      // validate names in HLSL block
      for (char *p = strchr(def_hlsl, '@'); p; p = strchr(p + 1, '@'))
      {
        int clen = 0;
        while (p - clen >= def_hlsl.data() && fast_isalnum_or_(*(p - clen - 1)))
          clen++;
        if (strncmp(p - clen, var->text, clen) != 0)
          error(String(32, "named const <%s> is referenced improperly in HLSL as <%.*s>", var->text, clen, p - clen), var);
      }

      char tempbufLine[512];
      tempbufLine[0] = 0;
      SNPRINTF(tempbufLine, sizeof(tempbufLine), "#line %d \"%s\"\n", var->line_start,
        parser.get_lex_parser().get_input_stream()->get_filename(var->file_start));
      def_hlsl.insert(0, tempbufLine, (uint32_t)strlen(tempbufLine));
      def_hlsl.insert(def_hlsl.length(), "\n", 1);
      // def_hlsl = String(0, "#line %d \"%s\"\n%s\n",
      //   var->line_start, parser.get_lex_parser().get_input_stream()->get_filename(var->file_start), def_hlsl);
      if (strstr(def_hlsl, "#include "))
      {
        CodeSourceBlocks cb;
        struct NullEval : public ShaderBoolEvalCB
        {
          bool anyCond = false;
          virtual ShVarBool eval_expr(bool_expr &)
          {
            anyCond = true;
            return ShVarBool(false, true);
          }
          virtual ShVarBool eval_bool_value(bool_value &)
          {
            anyCond = true;
            return ShVarBool(false, true);
          }
          virtual int eval_interval_value(const char *ival_name)
          {
            anyCond = true;
            return -1;
          }
        } null_eval;

        if (!cb.parseSourceCode("", def_hlsl, null_eval))
        {
          error(String(32, "bad HLSL decl for named const <%s>:\n%s", var->text, def_hlsl), var);
          return;
        }

        dag::ConstSpan<char> main_src = cb.buildSourceCode(cb.getPreprocessedCode(null_eval));
        if (null_eval.anyCond)
        {
          error(String(32, "HLSL decl for named const <%s> shall not contain pre-processor branches:\n%s", var->text, def_hlsl), var);
          return;
        }

        def_hlsl.printf(0, "%.*s", main_src.size(), main_src.data());
      }
      // debug("add const <%s> hlsl:\n%s", var->text, def_hlsl);
    }

    if (isBindless)
    {
      const char *typeName = (type == VariableType::staticCube)        ? "TextureSamplerCube"
                             : (type == VariableType::staticCubeArray) ? "TextureCubeArraySampler"
                             : (type == VariableType::staticTex3D)     ? "Texture3DSampler"
                             : (type == VariableType::staticTexArray)  ? "TextureArraySampler"
                                                                       : "TextureSampler";
#if _CROSS_TARGET_C1 || _CROSS_TARGET_C2

#else
      const char *sampleSuffix = (type == VariableType::staticCube)        ? "_cube"
                                 : (type == VariableType::staticCubeArray) ? "_cube_array"
                                 : (type == VariableType::staticTex3D)     ? "3d"
                                 : (type == VariableType::staticTexArray)  ? "_array"
                                                                           : "";
#endif
      String usageHlsl;
      usageHlsl.aprintf(0,
        "#ifndef BINDLESS_GETTER_%s\n"
        "#define BINDLESS_GETTER_%s\n"
        "%s get_%s()\n"
        "{\n"
        "  %s texSamp;\n"
        "  texSamp.tex = static_textures%s[get_%s().x];\n"
        "  texSamp.smp = static_samplers[get_%s().y];\n"
        "  return texSamp;\n"
        "}\n"
        "#endif\n",
        var->text, var->text, typeName, var->text, typeName, sampleSuffix, varName, varName);
      shConst.addHlslPostCode(usageHlsl);
    }
    else if ((shtype == SHVT_COLOR4 || shtype == SHVT_INT4) &&
             (type == VariableType::f44 && initializer.size() == 4 || initializer.size() == 1))
    {
      def_hlsl[def_hlsl.size() - 2] = ' ';
      def_hlsl.aprintf(0, "%s get_%s() { return %s; }\n", var_type_str, var->text, var->text);
    }

    if (shtype == SHVT_COLOR4 || shtype == SHVT_INT4)
    {
      auto found = combinedNameToOrigNames.find(var->text);
      if (found != combinedNameToOrigNames.end())
      {
        eastl::set<eastl::string> printed_names;
        def_hlsl.aprintf(0, "\n");
        for (auto &orig_names : found->second)
        {
          if (printed_names.emplace(orig_names).second)
            def_hlsl.aprintf(0, "%s\n", orig_names.c_str());
        }
      }
    }

    def_hlsl.aprintf(0, "#define get_name_%s %i\n", var->text, sclass.uniqueStrings.addNameId(var->text) + sclass.messages.size());

    const int id = shConst.addConst(stage, varName, name_space, registers_count, hardcoded_reg, eastl::move(def_hlsl));
    if (id < 0)
      error(String(32, "Redeclaration for variable <%s> in external block is not allowed", varName.c_str()), var);

    // now generate stcode
    size_t stcode_size = curpass->get_alt_curstcode(true).size();
    for (int initI = 0; initI < initializer.size(); ++initI)
    {
      stVarToReg.clear();
      auto value = initializer[initI];
      if (value->builtin_var)
      {
        if (type == VariableType::f44 && initializer.size() == 1 && !is_matrix_rvalue)
        {
          if (stage != STAGE_VS)
            error("Built-in matrices can only be declared in the vertex shader", var);
          G_ASSERTF(unsigned(id) < 0x3FF, "id=%d base=%d", id, initI);
          int type = 0;
          switch (value->builtin_var->num)
          {
            case SHADER_TOKENS::SHTOK_globtm: type = P1_SHCOD_G_TM_GLOBTM; break;
            case SHADER_TOKENS::SHTOK_projtm: type = P1_SHCOD_G_TM_PROJTM; break;
            case SHADER_TOKENS::SHTOK_viewprojtm: type = P1_SHCOD_G_TM_VIEWPROJTM; break;
            default: G_ASSERTF(0, "s.value->value->num=%d %s", value->builtin_var->num, value->builtin_var->text);
          }
          curpass->push_stcode(shaderopcode::makeOp2(SHCOD_G_TM, (type << 10) | id, initI));
          continue;
        }
        int builtin_v = -1, builtin_cp = 0;
        switch (value->builtin_var->num)
        {
          case SHADER_TOKENS::SHTOK_local_view_x:
            builtin_v = SHCOD_LVIEW;
            builtin_cp = 0;
            break;
          case SHADER_TOKENS::SHTOK_local_view_y:
            builtin_v = SHCOD_LVIEW;
            builtin_cp = 1;
            break;
          case SHADER_TOKENS::SHTOK_local_view_z:
            builtin_v = SHCOD_LVIEW;
            builtin_cp = 2;
            break;
          case SHADER_TOKENS::SHTOK_local_view_pos:
            builtin_v = SHCOD_LVIEW;
            builtin_cp = 3;
            break;
          case SHADER_TOKENS::SHTOK_world_local_x:
            builtin_v = SHCOD_TMWORLD;
            builtin_cp = 0;
            break;
          case SHADER_TOKENS::SHTOK_world_local_y:
            builtin_v = SHCOD_TMWORLD;
            builtin_cp = 1;
            break;
          case SHADER_TOKENS::SHTOK_world_local_z:
            builtin_v = SHCOD_TMWORLD;
            builtin_cp = 2;
            break;
          case SHADER_TOKENS::SHTOK_world_local_pos:
            builtin_v = SHCOD_TMWORLD;
            builtin_cp = 3;
            break;
          default: G_ASSERT(false);
        }
        if (type > VariableType::f4 && !(type == VariableType::f44 && state_block.arr))
        {
          error("Lvalue must be f1-4 variable", var);
        }
        Register cr = add_vec_reg();
        curpass->push_stcode(shaderopcode::makeOp2(builtin_v, int(cr), builtin_cp));
        curpass->push_stcode(shaderopcode::makeOp3(shcod, id, initI, int(cr)));
        continue;
      }
      G_ASSERT(value->expr);
      if (shtype == SHVT_COLOR4 || shtype == SHVT_INT4)
      {
        ExpressionParser::getStatic().setOwner(this);

        ComplexExpression colorExpr(shexpr::VT_COLOR4);

        if (!ExpressionParser::getStatic().parseExpression(*value->expr, &colorExpr))
          continue;
        if (!colorExpr.collapseNumbers())
          continue;

        Tab<int> cod(tmpmem);
        Register dr = add_vec_reg();

        Color4 v;
        bool isDynamic = false;

        if (colorExpr.evaluate(v))
        {
          // this is a constant - place it immediately
          Expression::assemblyConstant(cod, v, int(dr));
        }
        else
        {
          // this is an expression - assembly it for future calculation
          if (!colorExpr.assembly(*this, cod, dr, shtype == SHVT_INT4))
          {
            error("error while assembling expression", colorExpr.getTerminal());
            continue;
          }

          isDynamic = colorExpr.isDynamic();
        }

        cod.push_back(shaderopcode::makeOp3(shcod, id, initI, int(dr)));

        curpass->append_alt_stcode(isDynamic, cod);
        if (!isDynamic)
          shConst.markStatic(id, name_space);
      }
      else
      {
        G_ASSERT(initI == 0);
        G_ASSERT(value->expr->lhs->lhs->var_name);
        int var_type;
        bool is_global;
        int var_id = get_state_var(*value->expr->lhs->lhs->var_name, var_type, is_global);
        if (var_id < -1)
          continue;
        if (shtype != var_type)
        {
          error(String("can't assign ") + ShUtils::shader_var_type_name(var_type) + " to " + ShUtils::shader_var_type_name(shtype),
            value->expr->lhs->lhs->var_name);
          continue;
        }

        if (is_global)
        {
          int gc;
          Register reg;
          switch (shtype)
          {
            case SHVT_TEXTURE:
              gc = SHCOD_GET_GTEX;
              reg = add_reg(shtype);
              break;
            case SHVT_BUFFER:
              gc = SHCOD_GET_GBUF;
              reg = add_reg(shtype);
              break;
            case SHVT_FLOAT4X4:
              gc = SHCOD_GET_GMAT44;
              reg = add_vec_reg(4);
              break;
            default: G_ASSERT(0);
          }

          curpass->push_stcode(shaderopcode::makeOp2(gc, int(reg), var_id));
          if (shcod == SHCOD_BUFFER || shcod == SHCOD_CONST_BUFFER)
          {
            curpass->push_stcode(shaderopcode::makeOpStageSlot(shcod, stage, id, int(reg)));
          }
          else
          {
            curpass->push_stcode(shaderopcode::makeOp3(shcod, id, 0, int(reg)));
            if (shtype == SHVT_FLOAT4X4)
            {
              curpass->push_stcode(shaderopcode::makeOp3(shcod, id, 1, int(reg) + 1 * 4));
              curpass->push_stcode(shaderopcode::makeOp3(shcod, id, 2, int(reg) + 2 * 4));
              curpass->push_stcode(shaderopcode::makeOp3(shcod, id, 3, int(reg) + 3 * 4));
            }
          }
        }
        else
        {
          bool dyn = (var_id >= 0 && code.vars[var_id].dynamic);
          int buffer[2];

          G_ASSERT(shtype == SHVT_TEXTURE);

          if (!dyn)
            shConst.markStatic(id, name_space);

          Register reg = add_reg(shtype);
          buffer[0] = shaderopcode::makeOp2(SHCOD_GET_TEX, int(reg), var_id);
          buffer[1] = shaderopcode::makeOp3(shcod, id, 0, int(reg));

          curpass->append_alt_stcode(dyn, make_span_const(buffer, 2));
        }
      }
    }
    if (curpass->get_alt_curstcode(true).size() > stcode_size)
      stcode_vars.push_back(var);
  }
}

void AssembleShaderEvalCB::eval_supports(supports_stat &s)
{
  for (int i = 0; i < s.name.size(); i++)
  {
    if (s.name[i]->num != SHADER_TOKENS::SHTOK_none)
    {
      using namespace std::string_view_literals;
      if (s.name[i]->text == "__static_cbuf"sv)
      {
        if (supportsStaticCbuf != StaticCbuf::ARRAY)
          supportsStaticCbuf = StaticCbuf::SINGLE;
        continue;
      }
      if (s.name[i]->text == "__static_multidraw_cbuf"sv)
      {
        // Currently we support multidraw constbuffers only for bindless material version.
        supportsStaticCbuf = enableBindless ? StaticCbuf::ARRAY : StaticCbuf::SINGLE;
        continue;
      }
      if (s.name[i]->text == "__draw_id"sv)
      {
        continue;
      }
    }
    ShaderStateBlock *b =
      (s.name[i]->num == SHADER_TOKENS::SHTOK_none) ? ShaderStateBlock::emptyBlock() : ShaderStateBlock::findBlock(s.name[i]->text);
    if (!b)
    {
      error(String(32, "can't support undefined block <%s>", s.name[i]->text), s.name[i]);
      continue;
    }

    bool found = (b == shConst.globConstBlk);
    for (int j = 0; j < shConst.suppBlk.size(); j++)
      if (shConst.suppBlk[j] == b)
      {
        found = true;
        break;
      }
    if (found)
    {
      warning(String(32, "double support for block <%s>", s.name[i]->text), s.name[i]);
      continue;
    }

    if (b->layerLevel < ShaderStateBlock::LEV_SHADER && !b->canBeSupportedBy(shBlockLev))
    {
      error(String(32, "block <%s>, layer %d, cannot be supported here", s.name[i]->text, b->layerLevel), s.name[i]);
      continue;
    }

    for (int j = 0, e = shConst.vertexProps.sn.nameCount(); j < e; j++)
      if (b->getVsNameId(shConst.vertexProps.sn.getName(j)) != -1)
      {
        error(String(32, "can't support block <%s>: VS named const <%s> overlaps", s.name[i]->text, shConst.vertexProps.sn.getName(j)),
          s.name[i]);
        continue;
      }

    for (int j = 0, e = shConst.pixelProps.sn.nameCount(); j < e; j++)
      if (b->getPsNameId(shConst.pixelProps.sn.getName(j)) != -1)
      {
        error(String(32, "can't support block <%s>: PS named const <%s> overlaps", s.name[i]->text, shConst.pixelProps.sn.getName(j)),
          s.name[i]);
        continue;
      }
    if (b->layerLevel == ShaderStateBlock::LEV_GLOBAL_CONST)
      shConst.globConstBlk = b;
    else
      shConst.suppBlk.push_back(b);
  }
}

void AssembleShaderEvalCB::eval_render_stage(render_stage_stat &s)
{
  String stage_nm;
  if (s.name_s)
    stage_nm.printf(0, "%.*s", strlen(s.name_s->text) - 2, s.name_s->text + 1);

  const char *nm = s.name ? s.name->text : stage_nm;
  int idx = renderStageToIdxMap.getNameId(nm);
  if (idx < 0)
    error(String(32, "bad renderStage <%s>", nm), s.name);
  else if (code.renderStageIdx < 0)
  {
    code.renderStageIdx = idx;
    code.flags = (idx & SC_STAGE_IDX_MASK) | (code.flags & ~SC_STAGE_IDX_MASK);
    if (code.renderStageIdx)
      code.flags |= SC_NEW_STAGE_FMT;
  }
  else if (code.renderStageIdx != idx)
    error(String(32, "renderStageIdx tries to change from %d (%s) to %d (%s)", code.renderStageIdx,
            renderStageToIdxMap.getName(code.renderStageIdx), idx, renderStageToIdxMap.getName(idx)),
      s.name);
}
void AssembleShaderEvalCB::eval_command(shader_directive &s)
{
  switch (s.command->num)
  {
    case SHADER_TOKENS::SHTOK_no_ablend: curpass->force_noablend = true; return;
    case SHADER_TOKENS::SHTOK_dont_render:
      dont_render = true;
      if (curpass)
        curpass->finish_pass(false);
      throw GsclStopProcessingException();
      return;
    case SHADER_TOKENS::SHTOK_no_dynstcode: no_dynstcode = s.command; return;
    case SHADER_TOKENS::SHTOK_render_trans:
    {
      int idx = renderStageToIdxMap.getNameId("trans");
      if (idx < 0)
        error(String(32, "bad renderStage <%s>", "trans"), s.command);
      else if (code.renderStageIdx < 0)
      {
        code.renderStageIdx = idx;
        code.flags = (idx & SC_STAGE_IDX_MASK) | (code.flags & ~SC_STAGE_IDX_MASK);
        code.flags |= SC_NEW_STAGE_FMT;
      }
      else if (code.renderStageIdx != idx)
        error(String(32, "renderStageIdx tries to change from %d (%s) to %d (%s)", code.renderStageIdx,
                renderStageToIdxMap.getName(code.renderStageIdx), idx, renderStageToIdxMap.getName(idx)),
          s.command);
    }
      return;
    default: G_ASSERT(0);
  }
}


bool AssembleShaderEvalCB::compareShader(Terminal *shname_token, bool_value &e)
{
  G_ASSERT(e.cmpop);
  switch (e.cmpop->op->num)
  {
    case SHADER_TOKENS::SHTOK_eq:
    case SHADER_TOKENS::SHTOK_assign: return strcmp(shname_token->text, e.shader->text) == 0;
    case SHADER_TOKENS::SHTOK_noteq: return strcmp(shname_token->text, e.shader->text) != 0;
    default: G_ASSERT(0);
  }
  return false;
}


bool AssembleShaderEvalCB::compareHWToken(int hw_token, const ShHardwareOptions &opt)
{
  switch (hw_token)
  {
    // case SHADER_TOKENS::SHTOK_separate_ablend: return opt.sepABlend;
    case SHADER_TOKENS::SHTOK_fsh_4_0: return opt.fshVersion >= 4.0_sm;
    case SHADER_TOKENS::SHTOK_fsh_4_1: return opt.fshVersion >= 4.1_sm;
    case SHADER_TOKENS::SHTOK_fsh_5_0: return opt.fshVersion >= 5.0_sm;

    case SHADER_TOKENS::SHTOK_pc: // backward comp
#if _CROSS_TARGET_DX11
      return true;
#else
      return false;
#endif
      break;

    case SHADER_TOKENS::SHTOK_dx11:
#if _CROSS_TARGET_DX11 | _CROSS_TARGET_EMPTY
      return true;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_dx12:
#if _CROSS_TARGET_DX12
      return true;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_metaliOS:
#if _CROSS_TARGET_METAL
      return use_ios_token;
#else
      return false;
#endif
    case SHADER_TOKENS::SHTOK_metal:
#if _CROSS_TARGET_METAL
      return true;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_vulkan:
#if _CROSS_TARGET_SPIRV
      return true;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_ps4:
#if _CROSS_TARGET_C1

#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_ps5:
#if _CROSS_TARGET_C2

#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_xbox:
#if _CROSS_TARGET_DX12
      // on dx12 we have different profiles with the same compiler
      return dx12::dxil::is_xbox_platform(targetPlatform);
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_scarlett:
#if _CROSS_TARGET_DX12
      // on dx12 we have different profiles with the same compiler
      return dx12::dxil::is_xbox_platform(targetPlatform);
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_mesh:
#if _CROSS_TARGET_DX12
      // Not all DX12 targets support mesh shaders, this depends on the target platform
      return dx12::dxil::platform_has_mesh_support(targetPlatform);
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_bindless: return enableBindless;

    default: G_ASSERT(0);
  }

  return false;
}

static String hwDefinesStr;
void AssembleShaderEvalCB::buildHwDefines(const ShHardwareOptions &opt)
{
  hwDefinesStr = "";
#define ADD_HW_MACRO(TOKEN) \
  hwDefinesStr.aprintf(0, "#define _HARDWARE_%s %d\n", String(#TOKEN).toUpper(), compareHWToken(SHADER_TOKENS::SHTOK_##TOKEN, opt))
  ADD_HW_MACRO(pc);
  ADD_HW_MACRO(dx11);
  ADD_HW_MACRO(dx12);
  ADD_HW_MACRO(metaliOS);
  ADD_HW_MACRO(metal);
  ADD_HW_MACRO(vulkan);
  ADD_HW_MACRO(ps4);
  ADD_HW_MACRO(ps5);
  ADD_HW_MACRO(xbox);
  ADD_HW_MACRO(scarlett);
  ADD_HW_MACRO(fsh_4_0);
  ADD_HW_MACRO(fsh_4_1);
  ADD_HW_MACRO(fsh_5_0);
#undef ADD_HW_MACRO
}
const String &AssembleShaderEvalCB::getBuiltHwDefines() { return hwDefinesStr; }

ShVarBool AssembleShaderEvalCB::eval_expr(bool_expr &e)
{
  evalExprErrStr = "";
  ShVarBool v = ShaderParser::eval_shader_bool(e, *this);
  if (!evalExprErrStr.empty() && (!v.isConst || v.value))
  {
    String cond;
    ShaderParser::build_bool_expr_string(e, cond);
    sh_debug(SHLOG_ERROR, "expr \"%s\" is not const.false and gives errors:\n%s", cond, evalExprErrStr);
  }
  evalExprErrStr = "";
  return v;
}
ShVarBool AssembleShaderEvalCB::eval_bool_value(bool_value &e)
{
  if (e.two_sided)
  {
    return ShVarBool(variant.getValue(ShaderVariant::VARTYPE_MODE, ShaderVariant::TWO_SIDED), true);
  }
  else if (e.real_two_sided)
  {
    return ShVarBool(variant.getValue(ShaderVariant::VARTYPE_MODE, ShaderVariant::REAL_TWO_SIDED), true);
  }
  else if (e.shader)
  {
    return ShVarBool(compareShader(shname_token, e), true);
  }
  else if (e.hw)
  {
    return ShVarBool(compareHW(e, opt), true);
  }
  else if (e.interval_ident)
  {
    int varType;
    bool isGlobal;
    int varIndex = get_state_var(*e.interval_ident, varType, isGlobal, true);
    bool has_corresponded_var = varIndex >= 0;

    G_ASSERT(e.cmpop);
    G_ASSERT(e.interval_value);

    Interval::BooleanExpr expr = Interval::EXPR_NOTINIT;
    switch (e.cmpop->op->num)
    {
      case SHADER_TOKENS::SHTOK_eq: expr = Interval::EXPR_EQ; break;
      case SHADER_TOKENS::SHTOK_assign: expr = Interval::EXPR_EQ; break;
      case SHADER_TOKENS::SHTOK_greater: expr = Interval::EXPR_GREATER; break;
      case SHADER_TOKENS::SHTOK_greatereq: expr = Interval::EXPR_GREATER_EQ; break;
      case SHADER_TOKENS::SHTOK_smaller: expr = Interval::EXPR_SMALLER; break;
      case SHADER_TOKENS::SHTOK_smallereq: expr = Interval::EXPR_SMALLER_EQ; break;
      case SHADER_TOKENS::SHTOK_noteq: expr = Interval::EXPR_NOT_EQ; break;
      default: G_ASSERT(0);
    }

    if (expr == Interval::EXPR_NOTINIT)
      return ShVarBool(false, false);

    const IntervalList *intervalList = &variant.intervals;

    // search interval by name
    int e_interval_ident_id = IntervalValue::getIntervalNameId(e.interval_ident->text);
    int intervalIndex = variant.intervals.getIntervalIndex(e_interval_ident_id);
    if (intervalIndex == INTERVAL_NOT_INIT)
    {
      intervalIndex = ShaderGlobal::getIntervalList().getIntervalIndex(e_interval_ident_id);

      if (intervalIndex == INTERVAL_NOT_INIT)
      {
        error(String(256, "[ERROR] interval %s not found!", e.interval_ident->text), e.interval_ident);
        return ShVarBool(false, false);
      }

      intervalList = &ShaderGlobal::getIntervalList();

      if (!has_corresponded_var)
        isGlobal = true;
    }

    const Interval *interv = intervalList->getInterval(intervalIndex);
    G_ASSERT(interv);

    ShaderVariant::ValueType varValue = variant.getValue(interv->getVarType(), intervalIndex);
    if (varValue == -1 && shc::getAssumedVarsBlock())
    {
      float value = 0;

      if (shc::getAssumedValue(e.interval_ident->text, shname_token->text, isGlobal, value))
        varValue = interv->normalizeValue(value);
    }
    const Interval *interval = intervalList->getInterval(intervalIndex);
    if (!ShaderVariant::ValueRange(0, interval->getValueCount() - 1).isInRange(varValue))
    {
      evalExprErrStr.aprintf(0, "  illegal normalized value (%d) for this interval (%s)\n", varValue, interval->getNameStr());
      return ShVarBool(false, false);
    }

    String error_msg;
    bool bool_result = interval->checkExpression(varValue, expr, e.interval_value->text, error_msg);
    if (!error_msg.empty())
      error(error_msg, e.interval_ident);

    return ShVarBool(bool_result, true);
  }
  else if (e.texture_name)
  {
    int varType;
    bool isGlobal;
    int varIndex = get_state_var(*e.texture_name, varType, isGlobal);
    if (varIndex < 0)
    {
      evalExprErrStr.aprintf(0, "  variable <%s> not defined!\n", e.texture_name);
      return ShVarBool(false, false);
    }

    const IntervalList *intervalList = &variant.intervals;

    // search interval by name
    int e_texture_ident_id = IntervalValue::getIntervalNameId(e.texture_name->text);
    int intervalIndex = variant.intervals.getIntervalIndex(e_texture_ident_id);
    if (intervalIndex == INTERVAL_NOT_INIT)
    {
      intervalIndex = ShaderGlobal::getIntervalList().getIntervalIndex(e_texture_ident_id);

      if (intervalIndex == INTERVAL_NOT_INIT)
      {
        evalExprErrStr.aprintf(0, "  interval %s not found!\n", e.texture_name->text);
        return ShVarBool(false, false);
      }

      intervalList = &ShaderGlobal::getIntervalList();
    }

    const Interval *interv = intervalList->getInterval(intervalIndex);
    G_ASSERT(interv);

    ShaderVariant::ValueType varValue = variant.getValue(interv->getVarType(), intervalIndex);
    if (varValue == -1 && shc::getAssumedVarsBlock())
    {
      float val = 0;
      if (shc::getAssumedValue(e.texture_name->text, shname_token->text, isGlobal, val))
        varValue = interv->normalizeValue(val);
    }

    bool result = false;
    G_ASSERT(e.cmpop);
    switch (e.cmpop->op->num)
    {
      case SHADER_TOKENS::SHTOK_eq:
      case SHADER_TOKENS::SHTOK_assign:
      {
        result = varValue != 1;
        break;
      }
      case SHADER_TOKENS::SHTOK_noteq:
      {
        result = varValue == 1;
        break;
      }
      case SHADER_TOKENS::SHTOK_greater:
      case SHADER_TOKENS::SHTOK_greatereq:
      case SHADER_TOKENS::SHTOK_smaller:
      case SHADER_TOKENS::SHTOK_smallereq:
        error("[ERROR] operators '>', '>=', '<' and '<=' are not supported here!", e.cmpop->op);
        break;
      default: G_ASSERT(0);
    }

    return ShVarBool(result, true);
  }
  else if (e.bool_var)
  {
    auto expr = BoolVar::get_expr(mangle(shname_token->text, variant), *e.bool_var, parser);
    if (expr)
      return eval_expr(*expr);
  }
  else if (e.maybe)
  {
    auto expr = BoolVar::maybe(mangle(shname_token->text, variant), *e.maybe_bool_var);
    if (expr)
      return eval_expr(*expr);
    else
      return ShVarBool(false, true);
  }
  else if (e.true_value)
  {
    return ShVarBool(true, true);
  }
  else if (e.false_value)
  {
    return ShVarBool(false, true);
  }
  else
  {
    G_ASSERT(0);
  }
  return ShVarBool(false, false);
}

// clang-format off
// clang-format linearizes this function
void AssembleShaderEvalCB::eval_error_stat(error_stat &s)
{
  error(s.message->text, s.message);
}
// clang-format on

int AssembleShaderEvalCB::eval_interval_value(const char *ival_name)
{
  int varNameId = VarMap::getVarId(ival_name);
  int vi = code.find_var(varNameId);
  if (vi >= 0)
    code.vars[vi].used = true;

  bool isGlobal = false;
  const IntervalList *intervalList = &variant.intervals;

  // search interval by name
  int e_interval_ident_id = IntervalValue::getIntervalNameId(ival_name);
  int intervalIndex = variant.intervals.getIntervalIndex(e_interval_ident_id);
  if (intervalIndex == INTERVAL_NOT_INIT)
  {
    intervalIndex = ShaderGlobal::getIntervalList().getIntervalIndex(e_interval_ident_id);

    if (intervalIndex == INTERVAL_NOT_INIT)
    {
      sh_debug(SHLOG_ERROR, "interval %s not found!", ival_name);
      return 0;
    }

    intervalList = &ShaderGlobal::getIntervalList();
    isGlobal = true;
  }

  const Interval *interv = intervalList->getInterval(intervalIndex);
  G_ASSERT(interv);

  ShaderVariant::ValueType varValue = variant.getValue(interv->getVarType(), intervalIndex);
  if (varValue == -1 && shc::getAssumedVarsBlock())
  {
    float value = 0;

    if (shc::getAssumedValue(ival_name, shname_token->text, isGlobal, value))
      varValue = interv->normalizeValue(value);
  }
  return varValue;
}

void AssembleShaderEvalCB::merge_external_blocks()
{
  RegionMemAlloc rm_alloc(4 << 20, 4 << 20);
  ExpressionParser::getStatic().setOwner(this);

  for (int stage = STAGE_CS; stage < STAGE_MAX; stage++)
  {
    auto &stats = externalStateBlocks[stage];
    if (stats.empty())
      continue;

    using VarsByDimension = eastl::array<eastl::vector<state_block_stat *>, 4 + 1>;
    VarsByDimension float_vars;
    VarsByDimension int_vars;

    for (auto &stat : stats)
    {
      bool is_float_var = false;
      int components = 0;
      VariableType type = name_space_to_type(stat.var->var->nameSpace->text);
      // clang-format off
      switch (type)
      {
        case VariableType::f1: components = 1; is_float_var = true; break;
        case VariableType::f2: components = 2; is_float_var = true; break;
        case VariableType::f3: components = 3; is_float_var = true; break;
        case VariableType::i1: components = 1; is_float_var = false; break;
        case VariableType::i2: components = 2; is_float_var = false; break;
        case VariableType::i3: components = 3; is_float_var = false; break;
        default: G_ASSERT(false);
      }
      // clang-format on

      auto &vars = is_float_var ? float_vars : int_vars;
      vars[components].emplace_back(&stat);
    }

    auto merge = [&](VarsByDimension &vars, int dim1, int dim2, auto &merge) {
      if (dim1 != dim2)
      {
        if (vars[dim1].empty() || vars[dim2].empty())
          return;
      }
      else if (vars[dim1].size() < 2)
        return;

      external_variable *var_a = vars[dim1].back()->var;
      vars[dim1].pop_back();
      external_variable *var_b = vars[dim2].back()->var;
      vars[dim2].pop_back();

      String merged_name(32, "%s__%s", var_a->var->name->text, var_b->var->name->text);
      String merged_name_space(8, "@%c%i", var_a->var->nameSpace->text[1], dim1 + dim2);

      const char *comps[] = {"", "x", "xy", "xyz", "xyzw"};

      String local_a(32, "local %s __%s%i = 0;", merged_name_space[1] == 'f' ? "float4" : "int4", var_a->var->name->text, stage);
      auto *local_a_stat = parse_shader_stat(local_a, local_a.length(), &rm_alloc);
      local_a_stat->local_decl->expr = var_a->val->expr;

      String local_b(32, "local %s __%s%i = 0;", merged_name_space[1] == 'f' ? "float4" : "int4", var_b->var->name->text, stage);
      auto *local_b_stat = parse_shader_stat(local_b, local_b.length(), &rm_alloc);
      local_b_stat->local_decl->expr = var_b->val->expr;

      int reg1 = ExpressionParser::getStatic().parseLocalVarDecl(*local_a_stat->local_decl)->reg;
      int reg2 = ExpressionParser::getStatic().parseLocalVarDecl(*local_b_stat->local_decl)->reg;

      // clang-format off
      String merged_var(32, "(%s) { %s%s = (%s.%s, %s.%s); }",
                        profiles[stage], merged_name, merged_name_space,
                        local_a_stat->local_decl->name->text, comps[dim1],
                        local_b_stat->local_decl->name->text, comps[dim2]);
      auto *merged_var_stat = parse_shader_stat(merged_var, merged_var.length(), &rm_alloc);

      auto &combined_name_to_orig = combinedNameToOrigNames[merged_name.c_str()];
      combined_name_to_orig.emplace_back(eastl::string::CtorSprintf{},
                                         "/*merged_consts*/ static %s%s %s = (%s).%s;",
                                          merged_name_space[1] == 'f' ? "float" : "int",
                                          dim1 > 1 ? eastl::to_string(dim1).c_str() : "",
                                          var_a->var->name->text,
                                          merged_name.c_str(),
                                          comps[dim1]);
      combined_name_to_orig.emplace_back(eastl::string::CtorSprintf{},
                                         "/*merged_consts*/ static %s%s %s = (%s).%s;",
                                          merged_name_space[1] == 'f' ? "float" : "int",
                                          dim2 > 1 ? eastl::to_string(dim2).c_str() : "",
                                          var_b->var->name->text,
                                          merged_name.c_str(),
                                          &comps[dim1 + dim2][dim1]);
      // clang-format on
      auto found = combinedNameToOrigNames.find(var_a->var->name->text);
      if (found != combinedNameToOrigNames.end())
        combined_name_to_orig.insert(combined_name_to_orig.end(), found->second.begin(), found->second.end());
      found = combinedNameToOrigNames.find(var_b->var->name->text);
      if (found != combinedNameToOrigNames.end())
        combined_name_to_orig.insert(combined_name_to_orig.end(), found->second.begin(), found->second.end());

      if (dim1 + dim2 == 4)
      {
        handle_external_block_stat(*merged_var_stat->external_block->state_block_stat[0], (ShaderStage)stage);
        ShaderParser::Register(reg1, *this);
        ShaderParser::Register(reg2, *this);
      }
      else
      {
        vars[dim1 + dim2].emplace_back(merged_var_stat->external_block->state_block_stat[0]);
      }

      merge(vars, dim1, dim2, merge);
    };

    for (auto *vars : {&float_vars, &int_vars})
    {
      merge(*vars, 3, 1, merge);
      merge(*vars, 2, 2, merge);
      merge(*vars, 2, 1, merge);
      merge(*vars, 3, 1, merge);
      merge(*vars, 1, 1, merge);
      merge(*vars, 2, 2, merge);
      merge(*vars, 2, 1, merge);

      for (int dim = 1; dim <= 4; dim++)
        for (auto *stat : (*vars)[dim])
          handle_external_block_stat(*stat, (ShaderStage)stage);
    }
  }
}

void AssembleShaderEvalCB::end_pass(Terminal *terminal)
{
  merge_external_blocks();

  ShaderSemCode::Pass &p = *curpass;

  for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
  {
    if (p.blend_src[i] * p.blend_dst[i] < 0)
    {
      error(String(32, "no blend src/dst specified for attachement: %d", i), terminal);
    }

    if (!p.force_noablend)
    {
      if (p.blend_asrc[i] * p.blend_adst[i] < 0)
      {
        error(String(32, "no blend asrc/adst specified for attachement: %d", i), terminal);
      }
    }
  }

  if (p.cull_mode < 0)
    p.cull_mode = CULL_CCW;

  if (p.alpha_to_coverage < 0)
    p.alpha_to_coverage = 0;

  if (p.view_instances < 0)
    p.view_instances = 0;
  else if (p.view_instances > 0)
  {
#if !_CROSS_TARGET_DX12 && !_CROSS_TARGET_C2
    error("View instances are only supported on DX12 and PS5 for now!", terminal);
#endif

#if _CROSS_TARGET_DX12
    if (p.view_instances > 3)
      error("There can be a maximum of 4 view instances with DX12!", terminal);
#elif _CROSS_TARGET_C2


#endif
  }

  if (p.z_write < 0)
    p.z_write = 1;

  if (p.z_test < 0)
    p.z_test = 1;

  p.z_bias = true;

  if (!p.slope_z_bias)
  {
    p.slope_z_bias = true;
    p.push_stblkcode(shaderopcode::makeOp2_8_16(SHCOD_IMM_REAL1, int(add_reg()), 0));
  }

  bool accept = !dont_render;
  if (accept)
  {
    curvariant->suppBlk.resize(shConst.suppBlk.size());
    for (size_t i = 0; i < shConst.suppBlk.size(); i++)
      curvariant->suppBlk[i] = shConst.suppBlk[i];

    StaticCbuf staticCbufType = supportsStaticCbuf != StaticCbuf::ARRAY ? StaticCbuf::SINGLE : StaticCbuf::ARRAY;
    ShaderStateBlock ssb("", ShaderStateBlock::LEV_SHADER, eastl::move(shConst), {}, 0, staticCbufType);
    shConst.pixelProps.sc = eastl::move(ssb.shConst.pixelProps.sc);
    shConst.vertexProps.sc = eastl::move(ssb.shConst.vertexProps.sc);
    shConst.staticCbufType = ssb.shConst.staticCbufType;

    if (hlslCs.compile)
    {
      if (hlslVs.compile || hlslHs.compile || hlslDs.compile || hlslGs.compile || hlslPs.compile || hlslMs.compile || hlslAs.compile)
      {
        error(String(128, "%s: compute shader is not combineable with VS/HS/DS/GS/MS/AS/PS", shname_token->text), terminal);
        accept = false;
        goto end;
      }
    }
    if (hlslMs.compile)
    {
      if (hlslVs.compile || hlslHs.compile || hlslDs.compile || hlslGs.compile)
      {
        error(String(128, "%s: mesh shader is not combineable with VS/HS/DS/GS", shname_token->text), terminal);
        accept = false;
        goto end;
      }
    }
    if (hlslAs.compile)
    {
      if (!hlslMs.compile)
      {
        error(String(128, "%s: amplification shader needs a paired mesh shader", shname_token->text), terminal);
        accept = false;
        goto end;
      }
    }
    if (hlslVs.compile)
      hlsl_compile(hlslVs);
    if (hlslHs.compile || hlslDs.compile)
    {
      if (hlslHs.compile && hlslDs.compile)
      {
        hlsl_compile(hlslHs);
        hlsl_compile(hlslDs);
      }
      else
      {
        error(String(128, "%s: incomplete hull-tessellation/domain shader stage", shname_token->text), terminal);
        accept = false;
        goto end;
      }
    }
    if (hlslGs.compile)
      hlsl_compile(hlslGs);
    if (hlslPs.compile)
      hlsl_compile(hlslPs);
    if (hlslCs.compile)
      hlsl_compile(hlslCs);

    if (hlslMs.compile)
      hlsl_compile(hlslMs);
    if (hlslAs.compile)
      hlsl_compile(hlslAs);

    if (ErrorCounter::curShader().err > 0)
    {
      sh_process_errors();
      accept = false;
      goto end;
    }

    if (!hlslCs.profile)
    {
      if (!curpass->fsh.data() && hlslPs.profile && strcmp(hlslPs.profile, "ps_null"))
      {
        error(String(128, "%s: missing pixel shader", shname_token->text), terminal);
        accept = false;
      }
      if (!curpass->vpr.data())
      {
        error(String(128, "%s: missing vertex shader", shname_token->text), terminal);
        accept = false;
      }
      if (accept && curpass->ps30 != curpass->vs30 && strcmp(hlslPs.profile, "ps_null"))
      {
        error(String(128, "%s: PS/VS 3.0 mismatch: vs30=%s and ps30=%s", shname_token->text, curpass->vs30 ? "yes" : "no",
                curpass->ps30 ? "yes" : "no"),
          terminal);
        accept = false;
      }
    }
  }

  if (accept)
  {
    shConst.patchStcodeIndices(make_span(p.get_alt_curstcode(true)), false);
    shConst.patchStcodeIndices(make_span(p.get_alt_curstcode(false)), true);
    if (shConst.staticCbufType == StaticCbuf::ARRAY)
      p.push_stblkcode(shaderopcode::makeOp0(SHCOD_PACK_MATERIALS));
  }

end:
  curpass->finish_pass(accept);
  if (accept && no_dynstcode && curpass->stcode.size())
  {
    error(String(128, "%s: has dynstcode, while it was required to have none with no_dynstcode:", shname_token->text), no_dynstcode);
    for (Terminal *var : stcode_vars)
      error(String(32, "'%s'", var->text), var);
    debug("\n******* State code shader --s--");
    ShUtils::shcod_dump(curpass->stcode, nullptr); // todo: we need variable names!
  }
  curpass = nullptr;
}

void AssembleShaderEvalCB::end_eval(shader_decl &sh)
{
  end_pass(shname_token);

  code.regsize = maxregsize;
}


void AssembleShaderEvalCB::eval_shader_locdecl(local_var_decl &s)
{
  ExpressionParser::getStatic().setOwner(this);
  ExpressionParser::getStatic().parseLocalVarDecl(s);
}


void AssembleShaderEvalCB::eval_hlsl_compile(hlsl_compile_class &hlsl_compile)
{
  // Get source and settings.

  String profile(hlsl_compile.profile->text + 1);
  String entry(hlsl_compile.entry->text + 1);
  erase_items(profile, profile.length() - 1, 1);
  erase_items(entry, entry.length() - 1, 1);

  if (dd_stricmp(profile, "target_vs_for_gs") == 0)
  {
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#else
    profile = "target_vs";
#endif
  }
  if (dd_stricmp(profile, "target_vs_for_tess") == 0)
  {
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#else
    profile = "target_vs";
#endif
  }

  if (dd_stricmp(profile, "target_vs_half") == 0)
  {
#if _CROSS_TARGET_METAL || _CROSS_TARGET_SPIRV
    profile = opt.enableHalfProfile ? "vs_5_0_half" : "target_vs";
#else
    profile = "target_vs";
#endif
  }

  if (dd_stricmp(profile, "target_ps_half") == 0)
  {
#if _CROSS_TARGET_METAL || _CROSS_TARGET_SPIRV
    profile = opt.enableHalfProfile ? "ps_5_0_half" : "target_ps";
#else
    profile = "target_ps";
#endif
  }

  if (dd_stricmp(profile, "target_ps") == 0)
  {
    if (!opt
           .fshVersion //
           .map(6.6_sm, "ps_6_6")
           .map(6.0_sm, "ps_6_0")
           .map(5.0_sm, "ps_5_0")
           .map(4.1_sm, "ps_4_1")
           .map(4.0_sm, "ps_4_0")
           .get([&profile](auto str) { profile = str; }))
    {
      sh_debug(SHLOG_ERROR, "can't find out target for target_ps/%d", opt.fshVersion);
    }
  }
  else if (dd_stricmp(profile, "target_vs") == 0)
  {
    if (!opt
           .fshVersion //
           .map(6.6_sm, "vs_6_6")
           .map(6.0_sm, "vs_6_0")
           .map(5.0_sm, "vs_5_0")
           .map(4.1_sm, "vs_4_1")
           .map(4.0_sm, "vs_4_0")
           .get([&profile](auto str) { profile = str; }))
    {
      sh_debug(SHLOG_ERROR, "can't find out target for target_vs/%d", opt.fshVersion);
    }
  }
  else if (dd_stricmp(profile, "target_hs") == 0 || dd_stricmp(profile, "target_ds") == 0)
  {
    if (!opt
           .fshVersion //
           .map(6.6_sm, "%cs_6_6")
           .map(6.0_sm, "%cs_6_0")
           .map(5.0_sm, "%cs_5_0")
           .get([&profile](auto fmt) { profile.printf(0, fmt, profile[7]); }))
    {
      sh_debug(SHLOG_ERROR, "can't find out target for %s/%d", profile, opt.fshVersion);
    }
  }
  else if (dd_stricmp(profile, "target_gs") == 0)
  {
    if (!opt
           .fshVersion //
           .map(6.6_sm, "gs_6_6")
           .map(6.0_sm, "gs_6_0")
           .map(5.0_sm, "gs_5_0")
           .map(4.1_sm, "gs_4_1")
           .map(4.0_sm, "gs_4_0")
           .get([&profile](auto str) { profile = str; }))
    {
      sh_debug(SHLOG_ERROR, "can't find out target for %s/%d", profile, opt.fshVersion);
    }
  }
  else if (dd_stricmp(profile, "target_cs") == 0)
  {
    if (!opt
           .fshVersion //
           .map(6.6_sm, "cs_6_6")
           .map(6.0_sm, "cs_6_0")
           .map(5.0_sm, "cs_5_0")
           .get([&profile](auto str) { profile = str; }))
    {
      sh_debug(SHLOG_ERROR, "can't find out target for target_cs/%d", opt.fshVersion);
    }
  }
  else if ((dd_stricmp(profile, "target_ms") == 0) || (dd_stricmp(profile, "target_as") == 0))
  {
    if (!opt
           .fshVersion //
           .map(6.6_sm, "%cs_6_6")
           // Requires min profile of 6.5, as mesh shaders are a optional feature, they are available on all
           // targets starting from 5.0.
           .map(6.0_sm, "%cs_6_5")
           .map(5.0_sm, "%cs_6_5")
           .get([&profile](auto fmt) { profile.printf(0, fmt, profile[7]); }))
    {
      sh_debug(SHLOG_ERROR, "can't find out target for %s/%d", profile, opt.fshVersion);
    }
  }

  profile.toLower();
  HlslCompile *hlslXS = NULL;
  const char *shader_type = "";
  switch (profile[0])
  {
    case 'v':
      hlslXS = &hlslVs;
      shader_type = "vertex";
      break;
    case 'h':
      hlslXS = &hlslHs;
      shader_type = "hull";
      break;
    case 'd':
      hlslXS = &hlslDs;
      shader_type = "domain";
      break;
    case 'g':
      hlslXS = &hlslGs;
      shader_type = "geometry";
      break;
    case 'p':
      hlslXS = &hlslPs;
      shader_type = "pixel";
      break;
    case 'c':
      hlslXS = &hlslCs;
      shader_type = "compute";
      break;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2








#endif
    case 'm':
#if _CROSS_TARGET_DX12
      // Here is the only place where we check that compile for MS is actually a valid thing to do
      if (dx12::dxil::platform_has_mesh_support(targetPlatform))
#endif
      {
        hlslXS = &hlslMs;
        shader_type = "mesh";
      }
      break;
    case 'a':
#if _CROSS_TARGET_DX12
      // Here is the only place where we check that compile for AS is actually a valid thing to do
      if (dx12::dxil::platform_has_mesh_support(targetPlatform))
#endif
      {
        hlslXS = &hlslAs;
        shader_type = "amplification";
      }
      break;
  }

  if (!hlslXS)
  {
    error(String(0, "unsupported profile %s", profile), hlsl_compile.profile);
    return;
  }

  if (hlslXS->compile)
  {
    error(String(0, "duplicate %s shader", shader_type), hlsl_compile.profile);
    return;
  }
  hlslXS->profile = profile;
  hlslXS->entry = entry;
  if (strcmp(profile, "ps_null") != 0)
    hlslXS->compile = &hlsl_compile;

#if !_CROSS_TARGET_C1 && !_CROSS_TARGET_C2 && !_CROSS_TARGET_DX11
  if (strcmp(profile, "ps_3_0") == 0)
    curpass->ps30 = true;
  else if (strcmp(profile, "vs_3_0") == 0)
    curpass->vs30 = true;
#endif
}

class CompileShaderJob : public cpujobs::IJob
{
public:
  CompileShaderJob(AssembleShaderEvalCB *ascb, AssembleShaderEvalCB::HlslCompile &hlsl,
    dag::ConstSpan<CodeSourceBlocks::Unconditional *> code_blocks, uint64_t variant_hash)
  {
    static const char *def_cg_args[] = {"--assembly", "--mcgb", "--fenable-bx2", "--nobcolor", "--fastmath", NULL};

    entry = hlsl.entry;
    profile = hlsl.profile;
    hlsl_compile_token = hlsl.compile->profile;
    curpass = ascb->curpass;
    parser = &ascb->parser.get_lex_parser();
    shaderName = ascb->shname_token->text;
    cgArgs = hlslNoDisassembly ? def_cg_args + 1 : def_cg_args;

    bool compile_ps = (profile[0] == 'p' || profile[0] == 'c');

    CodeSourceBlocks &code = *getSourceBlocks(profile);
    source.printf(0, "#define _SHADER_STAGE_%cS 1\n", toupper(profile[0]));
    source += AssembleShaderEvalCB::getBuiltHwDefines();
    dag::ConstSpan<char> main_src = code.buildSourceCode(code_blocks);
    source.append(main_src.data(), main_src.size());
    ascb->shConst.patchHlsl(source, compile_ps, profile[0] == 'c', max_constants_no, bones_const_used);
    int base = append_items(source, 16);
    memset(&source[base], 0, 16);

    if (hlslDumpCodeAlways)
      compileCtx = sh_get_compile_context();

    shader_variant_hash = variant_hash;
  }

  virtual void doJob();
  virtual void releaseJob()
  {
    if (!compile_result.bytecode.empty())
    {
      addResults();
    }
    delete this;
  }

  void addResults();

protected:
  ShaderSemCode::Pass *curpass;
  SimpleString entry, profile;
  String source;
  String compileCtx;
  const char **cgArgs;
  CompileResult compile_result;
  int max_constants_no, bones_const_used;

  const char *shaderName;
  SHTOK_string *hlsl_compile_token;
  ShaderLexParser *parser;

  uint64_t shader_variant_hash;
};

static void apply_from_cache(char profile, int c1, int c2, int c3, ShaderSemCode::Pass &p)
{
  auto &cache = resolve_cached_shader(c1, c2, c3);
  switch (profile)
  {
    case 'v': p.vpr = cache.getShaderOutCode(CachedShader::TYPE_VS); break;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
    case 'h': p.hs = cache.getShaderOutCode(CachedShader::TYPE_HS); break;
    case 'd': p.ds = cache.getShaderOutCode(CachedShader::TYPE_DS); break;
    case 'g': p.gs = cache.getShaderOutCode(CachedShader::TYPE_GS); break;
    case 'p': p.fsh = cache.getShaderOutCode(CachedShader::TYPE_PS); break;
    case 'c':
      p.cs = cache.getShaderOutCode(CachedShader::TYPE_CS);
      p.threadGroupSizes[0] = cache.getComputeShaderInfo().threadGroupSizeX;
      p.threadGroupSizes[1] = cache.getComputeShaderInfo().threadGroupSizeY;
      p.threadGroupSizes[2] = cache.getComputeShaderInfo().threadGroupSizeZ;
      break;
    case 'm':
      p.vpr = cache.getShaderOutCode(CachedShader::TYPE_MS);
      p.threadGroupSizes[0] = cache.getComputeShaderInfo().threadGroupSizeX;
      p.threadGroupSizes[1] = cache.getComputeShaderInfo().threadGroupSizeY;
      p.threadGroupSizes[2] = cache.getComputeShaderInfo().threadGroupSizeZ;
      break;
    case 'a':
      p.gs = cache.getShaderOutCode(CachedShader::TYPE_AS);
      p.threadGroupSizes[0] = cache.getComputeShaderInfo().threadGroupSizeX;
      p.threadGroupSizes[1] = cache.getComputeShaderInfo().threadGroupSizeY;
      p.threadGroupSizes[2] = cache.getComputeShaderInfo().threadGroupSizeZ;
      break;
    default: G_ASSERTF(false, "Wrong profile %c", profile);
  }
}

static Tab<ShaderSemCode::Pass *> pending_passes(midmem);
void ShaderParser::resolve_pending_shaders_from_cache()
{
  for (int i = 0; i < pending_passes.size(); i++)
  {
    ShaderSemCode::Pass &p = *pending_passes[i];

    char profiles[] =
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#endif
      "vhdgpcma";

    int c1, c2, c3;
    for (char profile : profiles)
      if (profile && p.getCidx(profile, c1, c2, c3))
        apply_from_cache(profile, c1, c2, c3, p);
  }
  pending_passes.clear();
}

void AssembleShaderEvalCB::hlsl_compile(AssembleShaderEvalCB::HlslCompile &hlsl)
{
  bool compile_ps = (hlsl.profile[0] == 'p' || hlsl.profile[0] == 'c');

  CodeSourceBlocks *codeP = getSourceBlocks(hlsl.profile);
  G_ASSERT(codeP && "ShaderParser::curXsCode must be non null here by design");
  CodeSourceBlocks &code = *codeP;

  dag::ConstSpan<CodeSourceBlocks::Unconditional *> code_blocks = code.getPreprocessedCode(*this);
  CryptoHash code_digest = code.getCodeDigest(code_blocks);
  CryptoHash const_digest = shConst.getDigest(compile_ps, hlsl.profile[0] == 'c');

  ShaderCompilerStat::hlslCompileCount++;
  int c1, c2, c3;
  int cs_idx = find_cached_shader(code_digest, const_digest, hlsl.entry, hlsl.profile, c1, c2, c3);

  curpass->setCidx(hlsl.profile[0], c1, c2, c3);
  if (cs_idx != -1)
  {
    // Reuse cached shader.
    ShaderCompilerStat::hlslCacheHitCount++;

    if (cs_idx < 0)
    {
      // not ready yet, add to pending list to update later
      if (!pending_passes.size() || pending_passes.back() != curpass)
        pending_passes.push_back(curpass);
    }
    else
    {
      apply_from_cache(hlsl.profile[0], c1, c2, c3, *curpass);
    }
  }
  else
  {
    mark_cached_shader_entry_as_pending(c1, c2, c3);

    auto hash_variant_src = [](const ShaderVariant::VariantSrc &src, uint64_t &result) {
      const ShaderVariant::TypeTable &types = src.getTypes();
      for (int i = 0; i < types.getCount(); i++)
      {
        String type_name = types.getTypeName(i);
        eastl::string_view type_name_view(type_name.c_str(), type_name.size());
        result = fnv1a_step<64>(static_cast<uint64_t>(eastl::hash<eastl::string_view>()(type_name_view)), result);
        result = fnv1a_step<64>(static_cast<uint64_t>(src.getNormalizedValue(i)), result);
      }
    };

    uint64_t shader_variant_hash = 0u;
    hash_variant_src(variant.stat, shader_variant_hash);
    if (variant.dyn)
      hash_variant_src(*variant.dyn, shader_variant_hash);

    CompileShaderJob *job = new CompileShaderJob(this, hlsl, code_blocks, shader_variant_hash);
    if (shc::compileJobsCount > 1)
    {
      static int cpu = grnd();
      cpu = (cpu + 1) % shc::compileJobsCount;

      cpujobs::add_job(shc::compileJobsMgrBase + cpu, job);
    }
    else
    {
      job->doJob();
      job->releaseJob();
    }
  }

  // Release.
  hlsl.reset();
}

extern int hlslOptimizationLevel;
extern bool hlsl2021;
extern bool enableFp16;

#include "hashed_cache.h"
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include "sha1_cache_version.h"
bool useSha1Cache = true, writeSha1Cache = true;
extern char *sha1_cache_dir;

static inline void calc_sha1_stripped(HASH_CONTEXT &sha1, const char *src, size_t total_len)
{
  const char *c_start = src, *next_line = src;
  while (c_start && (next_line = strstr(c_start, "\n#line ")))
  {
    HASH_UPDATE(&sha1, (const unsigned char *)c_start, (uint32_t)(next_line - c_start));
    c_start = strstr(next_line + 1, "\n");
  };
  size_t at = (c_start - src);
  if (c_start != nullptr && at < total_len)
  {
    HASH_UPDATE(&sha1, (const unsigned char *)c_start, (uint32_t)(size_t(total_len) - at));
  }
}

void CompileShaderJob::doJob()
{
  char sha1SrcPath[420];
  sha1SrcPath[0] = 0;
  unsigned char srcSha1[HASH_SIZE];
  useSha1Cache &= !is_hlsl_debug() && hlslNoDisassembly;

#if _CROSS_TARGET_DX12
  if (strstr(source, "SV_ViewID"))
  {
    if (profile[3] < '6')
    {
      profile[3] = '6';
      profile[5] = '1';
    }
    else if (profile[3] == '6' && profile[5] == '0')
    {
      profile[5] = '1';
    }
  }
#endif

  const unsigned sourceLen = i_strlen(source);
  if (useSha1Cache)
  {
    HASH_CONTEXT sha1;
    HASH_INIT(&sha1);
    HASH_UPDATE(&sha1, (const unsigned char *)&sha1_cache_version, sizeof(sha1_cache_version));
    // HASH_UPDATE( &sha1, (const unsigned char*)source.c_str(), (uint32_t)sourceLen );
    calc_sha1_stripped(sha1, source.c_str(), (uint32_t)sourceLen);
    HASH_UPDATE(&sha1, (const unsigned char *)profile.c_str(), (uint32_t)strlen(profile));
    HASH_UPDATE(&sha1, (const unsigned char *)entry.c_str(), (uint32_t)strlen(entry));
    HASH_UPDATE(&sha1, (const unsigned char *)&hlslOptimizationLevel, (uint32_t)sizeof(hlslOptimizationLevel)); // optimization level
                                                                                                                // is part of output
                                                                                                                // dir, but still
    HASH_UPDATE(&sha1, (const unsigned char *)&hlsl2021, (uint32_t)sizeof(hlsl2021));
    HASH_UPDATE(&sha1, (const unsigned char *)&enableFp16, (uint32_t)sizeof(enableFp16));
    HASH_UPDATE(&sha1, (const unsigned char *)&enableBindless, (uint32_t)sizeof(enableBindless));
#if _CROSS_TARGET_SPIRV
    auto mode = compilerDXC ? CompilerMode::DXC : compilerHlslCc ? CompilerMode::HLSLCC : CompilerMode::DEFAULT;
    HASH_UPDATE(&sha1, (const unsigned char *)&mode, (uint32_t)sizeof(mode));
#elif _CROSS_TARGET_DX12
    HASH_UPDATE(&sha1, (const unsigned char *)&targetPlatform, (uint32_t)sizeof(targetPlatform));
    HASH_UPDATE(&sha1, (const unsigned char *)&useScarlettWave32, (uint32_t)sizeof(useScarlettWave32));
#endif
    if (isDebugModeEnabled)
      HASH_UPDATE(&sha1, (const unsigned char *)&isDebugModeEnabled, (uint32_t)sizeof(isDebugModeEnabled));

    HASH_FINISH(&sha1, srcSha1);

    SNPRINTF(sha1SrcPath, sizeof(sha1SrcPath), "%s/src/%s/" HASH_LIST_STRING, sha1_cache_dir,
      profile.c_str(), // sources and binaries lives in different subfolders. that is to reduce risk of collision, while probably not
                       // needed
      HASH_LIST(srcSha1));
    file_ptr_t sha1LinkFile = df_open(sha1SrcPath, DF_READ);
    if (sha1LinkFile)
    {
      char sha1BinPath[420];
      sha1BinPath[0] = 0;
      char buf[132];
      if (char *binSha1Read = df_gets((char *)buf, sizeof(buf), sha1LinkFile))
      {
        // sources and binaries lives in different subfolders. that is to reduce risk of collision, while probably not needed
        SNPRINTF(sha1BinPath, sizeof(sha1BinPath), "%s/bin/%s/%s", sha1_cache_dir, profile.c_str(), binSha1Read);
        df_close(sha1LinkFile);
        file_ptr_t sha1BinFile = df_open(sha1BinPath, DF_READ);
        if (!sha1BinFile)
        {
          sh_debug(SHLOG_NORMAL, "Sha1 link %s in %s is invalid.", sha1BinPath, sha1SrcPath);
        }
        else
        {
          int sha1FileLen;
          const void *content = df_mmap(sha1BinFile, &sha1FileLen);
          if (!sha1FileLen)
          {
            sh_debug(SHLOG_NORMAL, "link is broken?");
            df_close(sha1BinFile);
          }
          else
          {
            const uint8_t *end_bytecode = (const uint8_t *)content + sha1FileLen - sizeof(ComputeShaderInfo);
            compile_result.bytecode.assign((const uint8_t *)content, end_bytecode);
            memcpy(&compile_result.computeShaderInfo, end_bytecode, sizeof(ComputeShaderInfo));
            df_unmap(content, sha1FileLen);
            df_close(sha1BinFile);
            if (hlslDumpCodeAlways)
              debug("=== compiling code:\n%s\nreused built from %s", compileCtx, sha1SrcPath);
            return;
          }
        }
      }
      else
      {
        sh_debug(SHLOG_NORMAL, "Sha1 link in %s is missing.", sha1SrcPath);
        df_close(sha1LinkFile);
      }
    }
  }

  bool full_debug = false;
  const char *full_debug_str = strstr(source, "#pragma debugfull");
  if (full_debug_str == nullptr)
    full_debug_str = strstr(source, "#pragma dfull");
  if (full_debug_str != nullptr)
  {
    full_debug = true;

    // replace #pragma with comment
    memcpy((void *)full_debug_str, "//     ", 7);
  }

  bool forceDisableWarnings = false;
  const char *forceDisableWarningsStr = strstr(source, "#pragma force_disable_warnings");
  if (forceDisableWarningsStr != nullptr)
  {
    forceDisableWarnings = true;

    // replace #pragma with comment
    memcpy((void *)forceDisableWarningsStr, "//     ", 7);
  }

  // todo: add flags
  //  Compile.
#if _CROSS_TARGET_C1






#elif _CROSS_TARGET_C2





#elif _CROSS_TARGET_METAL
  if (use_metal_glslang)
    compile_result =
      compileShaderMetalGlslang(source, profile, entry, !hlslNoDisassembly, hlslSkipValidation, hlslOptimizationLevel ? true : false,
        max_constants_no, bones_const_used, shaderName, use_ios_token, use_binary_msl, shader_variant_hash);
  else
    compile_result =
      compileShaderMetal(source, profile, entry, !hlslNoDisassembly, hlslSkipValidation, hlslOptimizationLevel ? true : false,
        max_constants_no, bones_const_used, shaderName, use_ios_token, use_binary_msl, shader_variant_hash);
#elif _CROSS_TARGET_SPIRV
  compile_result = compileShaderSpirV(source, profile, entry, !hlslNoDisassembly, hlslSkipValidation,
    hlslOptimizationLevel ? true : false, max_constants_no, bones_const_used, shaderName,
    compilerDXC      ? CompilerMode::DXC
    : compilerHlslCc ? CompilerMode::HLSLCC
                     : CompilerMode::DEFAULT,
    shader_variant_hash);
#elif _CROSS_TARGET_EMPTY
  compile_result.bytecode.resize(sizeof(uint64_t));
#elif _CROSS_TARGET_DX12
  extern wchar_t *dx12_pdb_cache_dir;
  // NOTE: when we support this kind of switch somehow this can be replaced with actual information
  // of use or not
  compile_result = dx12::dxil::compileShader(make_span_const(source).first(sourceLen), // source length includes trailing zeros
    profile, entry, !hlslNoDisassembly, hlsl2021, enableFp16, hlslSkipValidation, hlslOptimizationLevel ? true : false,
    is_hlsl_debug(), dx12_pdb_cache_dir, max_constants_no, bones_const_used, shaderName, targetPlatform, useScarlettWave32,
    !forceDisableWarnings && hlslWarningsAsErrors, full_debug ? DebugLevel::FULL_DEBUG_INFO : hlslDebugLevel);
#else //_CROSS_TARGET_DX11
  unsigned int flags = is_hlsl_debug() ? D3DCOMPILE_DEBUG : 0;
  flags |= hlslSkipValidation ? D3DCOMPILE_SKIP_VALIDATION : 0;
  flags |= is_hlsl_debug()
             ? D3DCOMPILE_SKIP_OPTIMIZATION
             : (hlslOptimizationLevel >= 3
                   ? D3DCOMPILE_OPTIMIZATION_LEVEL3
                   : (hlslOptimizationLevel >= 2
                         ? D3DCOMPILE_OPTIMIZATION_LEVEL2
                         : (hlslOptimizationLevel >= 1 ? D3DCOMPILE_OPTIMIZATION_LEVEL0 : D3DCOMPILE_SKIP_OPTIMIZATION)));
  flags |= hlslWarningsAsErrors ? D3DCOMPILE_WARNINGS_ARE_ERRORS : 0;
  compile_result = compileShaderDX11(shaderName, source, NULL, profile, entry, !hlslNoDisassembly,
    full_debug ? DebugLevel::FULL_DEBUG_INFO : hlslDebugLevel, hlslSkipValidation, flags, max_constants_no, bones_const_used);
#endif

  if (hlslDumpCodeAlways)
    debug("=== compiling code:\n%s\n%s==== code end", compileCtx, source);

  if (compile_result.bytecode.empty())
  {
    sh_enter_atomic_debug();
    // Display error message.
    parser->set_error(hlsl_compile_token->file_start, hlsl_compile_token->line_start, hlsl_compile_token->col_start,
      String(256, "compile(%s, %s) failed:", profile.str(), entry.str()));

    if (!compile_result.errors.empty())
    {
      // debug("errors compiling HLSL code:\n%s", source.str());
      const char *es = (const char *)compile_result.errors.c_str(), *ep = strchr(es, '\n');
      while (es)
        if (ep)
        {
          sh_debug(SHLOG_NORMAL, "  %.*s", ep - es, es);
          size_t link_error = eastl::string_view(es, ep - es).find("External function used in non-library profile");
          if (link_error != eastl::string::npos && eastl::string_view(es, ep - es).find("_assert", link_error) != eastl::string::npos)
            sh_debug(SHLOG_NORMAL,
              "\nNote: the shader uses assert, but it was not enabled.\n"
              "Tip: add a macro `ENABLE_ASSERT(%.*s)` to the shader",
              2, profile.c_str());
          es = ep + 1;
          ep = strchr(es, '\n');
        }
        else
          break;
      sh_debug(SHLOG_NORMAL, "  %s\n", es);
    }
    else
      sh_debug(SHLOG_NORMAL, "  D3DXCompileShader error");

    if (!hlslDumpCodeAlways && hlslDumpCodeOnError)
      debug("=== compiling code:\n%s==== code end", source.str());

    sh_leave_atomic_debug();
#if _TARGET_PC_MACOSX
    usleep(150 * 1000);
#else
    Sleep(150);
#endif
    return;
  }

  if (!compile_result.errors.empty() && hlslShowWarnings)
  {
    parser->set_warning(hlsl_compile_token->file_start, hlsl_compile_token->line_start, hlsl_compile_token->col_start,
      String(256, "compile(%s, %s) finished with warnings:", profile.str(), entry.str()));

    const char *es = (const char *)compile_result.errors.c_str(), *ep = strchr(es, '\n');
    while (es)
      if (ep)
      {
        sh_debug(SHLOG_NORMAL, "  %.*s", ep - es, es);
        es = ep + 1;
        ep = strchr(es, '\n');
      }
      else
      {
        sh_debug(SHLOG_NORMAL, "  %s", es);
        break;
      }
  }
  if (writeSha1Cache && useSha1Cache && !compile_result.bytecode.empty() && dd_mkpath(sha1SrcPath))
  {
    unsigned char binSha1[HASH_SIZE];
    HASH_CONTEXT sha1;
    HASH_INIT(&sha1);
    HASH_UPDATE(&sha1, (const unsigned char *)compile_result.bytecode.data(), compile_result.bytecode.size());
    HASH_UPDATE(&sha1, (const unsigned char *)&compile_result.computeShaderInfo, sizeof(compile_result.computeShaderInfo));
    HASH_FINISH(&sha1, binSha1);
    char sha1BinPath[420];
    sha1BinPath[0] = 0;
    SNPRINTF(sha1BinPath, sizeof(sha1BinPath), "%s/bin/%s/" HASH_LIST_STRING, sha1_cache_dir,
      profile.c_str(), // sources and binaries lives in different subfolders. that is to reduce risk of collision, while probably not
                       // needed
      HASH_LIST(binSha1));
    DagorStat binbuf;
    if (df_stat(sha1BinPath, &binbuf) != -1 && binbuf.size == compile_result.bytecode.size() + sizeof(ComputeShaderInfo))
    {
      // blob is already saved
    }
    else
    {
      // save blob
      dd_mkpath(sha1BinPath);
      char tmpFileName[DAGOR_MAX_PATH];
      SNPRINTF(tmpFileName, sizeof(tmpFileName),
        "%s/"
        "%s_bin_" HASH_TEMP_STRING "XXXXXX",
        sha1_cache_dir, profile.c_str(), HASH_LIST(binSha1));
      file_ptr_t tmpF = df_mkstemp(tmpFileName);
      int written = df_write(tmpF, compile_result.bytecode.data(), compile_result.bytecode.size());
      written += df_write(tmpF, &compile_result.computeShaderInfo, sizeof(ComputeShaderInfo));
      if (written == compile_result.bytecode.size() + sizeof(ComputeShaderInfo))
      {
        df_close(tmpF);
        if (!dd_rename(tmpFileName, sha1BinPath))
          dd_erase(tmpFileName);
      }
      else
      {
        df_close(tmpF);
        dd_erase(tmpFileName);
        dd_erase(sha1SrcPath);
        return;
      }
    }

    char tmpFileName[DAGOR_MAX_PATH];
    SNPRINTF(tmpFileName, sizeof(tmpFileName),
      "%s/"
      "%s_src_" HASH_TEMP_STRING "XXXXXX",
      sha1_cache_dir, profile.c_str(), HASH_LIST(srcSha1));
    file_ptr_t tmpF = df_mkstemp(tmpFileName);
    if (tmpF)
    {
      char buf[132];
      SNPRINTF(buf, sizeof(buf), HASH_LIST_STRING, HASH_LIST(binSha1));

      const int bufLen = (int)strlen(buf);
      if (df_write(tmpF, buf, bufLen) == bufLen)
      {
        df_close(tmpF);
        if (!dd_rename(tmpFileName, sha1SrcPath))
          dd_erase(tmpFileName);
      }
      else
      {
        df_close(tmpF);
        dd_erase(tmpFileName);
      }
    }
  }
}

void CompileShaderJob::addResults()
{
  int c1, c2, c3;
  G_ASSERT(curpass->getCidx(profile[0], c1, c2, c3, true));

  // Fill cache entry
  bool added_new = post_shader_to_cache(c1, c2, c3, compile_result, CachedShader::typeFromProfile(profile));

  if (!added_new)
    ShaderCompilerStat::hlslEqResultCount++;

  apply_from_cache(profile[0], c1, c2, c3, *curpass);

  // Disassembly for debug.
  if (!hlslNoDisassembly && !compile_result.bytecode.empty())
  {
    debug("HLSL: profile='%s', entry='%s'", profile, entry);
    if (!compile_result.disassembly.empty())
    {
      int totalInstructionsNum = 0;
      int textureInstructionsNum = 0;
      int arithmeticInstructionsNum = 0;
      int flowInstructionsNum = 0;

      debug("disassembly:\n%s", compile_result.disassembly.c_str());

      // Add statistics.

      const char *str1 = strstr(compile_result.disassembly.c_str(), "// approximately ");
      if (!str1)
        str1 = strstr(compile_result.disassembly.c_str(), "// Approximately ");
      if (str1)
      {
        totalInstructionsNum = atoi(str1 + strlen("// approximately "));
        const char *str2 = strstr(str1, "instruction slots used (");
        if (str2)
        {
          textureInstructionsNum = atoi(str2 + strlen("instruction slots used ("));
          const char *str3 = strstr(str2, "texture, ");
          if (str3)
            arithmeticInstructionsNum = atoi(str3 + strlen("texture, "));
          const char *str4 = strstr(str2, "ALU, ");
          if (str4)
            flowInstructionsNum = atoi(str4 + strlen("ALU, "));
        }
      }
      ShaderCompilerStat::ShaderStatistics *stat = NULL;
      for (unsigned int i = 0; i < ShaderCompilerStat::shaderStatisticsList.size(); i++)
      {
        if (dd_stricmp(ShaderCompilerStat::shaderStatisticsList[i].name, shaderName) == 0 &&
            dd_stricmp(ShaderCompilerStat::shaderStatisticsList[i].entry, entry) == 0)
        {
          stat = &ShaderCompilerStat::shaderStatisticsList[i];
          break;
        }
      }

      if (!stat)
      {
        append_items(ShaderCompilerStat::shaderStatisticsList, 1);
        stat = &ShaderCompilerStat::shaderStatisticsList.back();
        stat->minInstructions = totalInstructionsNum;
        stat->minTextureInstructions = textureInstructionsNum;
        stat->minArithmeticInstructions = arithmeticInstructionsNum;
        stat->minFlowInstructions = flowInstructionsNum;
      }

      stat->name = shaderName;
      stat->entry = entry;
      stat->hlslVariants++;

      if (totalInstructionsNum < stat->minInstructions)
        stat->minInstructions = totalInstructionsNum;
      if (totalInstructionsNum > stat->maxInstructions)
        stat->maxInstructions = totalInstructionsNum;
      stat->totalInstructions += totalInstructionsNum;

      if (textureInstructionsNum < stat->minTextureInstructions)
        stat->minTextureInstructions = textureInstructionsNum;
      if (textureInstructionsNum > stat->maxTextureInstructions)
        stat->maxTextureInstructions = textureInstructionsNum;
      stat->totalTextureInstructions += textureInstructionsNum;

      if (arithmeticInstructionsNum < stat->minArithmeticInstructions)
        stat->minArithmeticInstructions = arithmeticInstructionsNum;
      if (arithmeticInstructionsNum > stat->maxArithmeticInstructions)
        stat->maxArithmeticInstructions = arithmeticInstructionsNum;
      stat->totalArithmeticInstructions += arithmeticInstructionsNum;

      stat->minFlowInstructions = min(flowInstructionsNum, stat->minFlowInstructions);
      stat->maxFlowInstructions = max(flowInstructionsNum, stat->maxFlowInstructions);
      stat->totalFlowInstructions += flowInstructionsNum;
    }
  }
}


struct ShaderCacheEntry
{
  struct Lv2
  {
    struct Lv3
    {
      SimpleString entry, profile;
      int codeIdx;
    };

    CryptoHash constDigest;
    Tab<Lv3> lv3;

    Lv2() : lv3(midmem) {}

    int addEntry(const char *entry, const char *profile)
    {
      Lv3 &l3 = lv3.push_back();
      l3.codeIdx = -1;
      l3.entry = entry;
      l3.profile = profile;
      return lv3.size() - 1;
    }
  };

  CryptoHash codeDigest;
  Tab<Lv2> lv2;

  ShaderCacheEntry() : lv2(midmem) {}

  int addEntry(const CryptoHash &const_digest, const char *entry, const char *profile)
  {
    Lv2 &l2 = lv2.push_back();
    l2.constDigest = const_digest;
    l2.addEntry(entry, profile);
    return lv2.size() - 1;
  }
};

//
// Shader compilation cache data and routines
//
static Tab<ShaderCacheEntry> cache_entries(midmem_ptr());
static Tab<CachedShader> cached_shaders_list(midmem_ptr());
static int psCodeCount = 0, vsCodeCount = 0;
static int hsCodeCount = 0, dsCodeCount = 0, gsCodeCount = 0;
static int csCodeCount = 0;

static int find_cached_shader(const CryptoHash &code_digest, const CryptoHash &const_digest, const char *entry, const char *profile,
  int &out_c1, int &out_c2, int &out_c3)
{
  for (int i = cache_entries.size() - 1; i >= 0; i--)
    if (cache_entries[i].codeDigest == code_digest)
    {
      ShaderCacheEntry &e = cache_entries[i];
      for (int j = e.lv2.size() - 1; j >= 0; j--)
        if (e.lv2[j].constDigest == const_digest)
        {
          ShaderCacheEntry::Lv2 &l2 = e.lv2[j];
          for (int k = l2.lv3.size() - 1; k >= 0; k--)
            if (l2.lv3[k].entry == entry && l2.lv3[k].profile == profile)
            {
              out_c1 = i;
              out_c2 = j;
              out_c3 = k;
              return l2.lv3[k].codeIdx;
            }

          out_c1 = i;
          out_c2 = j;
          out_c3 = l2.addEntry(entry, profile);
          return -1;
        }
      out_c1 = i;
      out_c2 = e.addEntry(const_digest, entry, profile);
      out_c3 = 0;
      return -1;
    }

  ShaderCacheEntry &e = cache_entries.push_back();
  e.codeDigest = code_digest;
  out_c1 = cache_entries.size() - 1;
  out_c2 = e.addEntry(const_digest, entry, profile);
  out_c3 = 0;
  return -1;
}

static bool post_shader_to_cache(int c1, int c2, int c3, const CompileResult &result, int codeType)
{
  size_t size_in_unsigned = (result.bytecode.size() + 3) / sizeof(unsigned);
  bool added_new = false;
  int codeIdx = -1;
  for (int i = cached_shaders_list.size() - 1; i >= 0; i--)
    if (cached_shaders_list[i].codeType == codeType && cached_shaders_list[i].relCode.size() == size_in_unsigned + 1 &&
        eastl::equal(result.bytecode.begin(), result.bytecode.end(), (const uint8_t *)&cached_shaders_list[i].relCode[1]))
    {
      codeIdx = i;
      break;
    }

  // Store new shader
  if (codeIdx == -1)
  {
    codeIdx = append_items(cached_shaders_list, 1);
    CachedShader &cachedShader = cached_shaders_list[codeIdx];

    cachedShader.relCode.resize(size_in_unsigned + 1);
    cachedShader.codeType = codeType;
    switch (codeType)
    {
      case CachedShader::TYPE_VS:
        cachedShader.relCode[0] = _MAKE4C('DX9v');
        vsCodeCount++;
        break;
      case CachedShader::TYPE_HS:
        cachedShader.relCode[0] = _MAKE4C('D11h');
        hsCodeCount++;
        break;
      case CachedShader::TYPE_DS:
        cachedShader.relCode[0] = _MAKE4C('D11d');
        dsCodeCount++;
        break;
      case CachedShader::TYPE_GS:
        cachedShader.relCode[0] = _MAKE4C('D11g');
        gsCodeCount++;
        break;
      case CachedShader::TYPE_PS:
        cachedShader.relCode[0] = _MAKE4C('DX9p');
        psCodeCount++;
        break;
      case CachedShader::TYPE_CS:
        cachedShader.relCode[0] = _MAKE4C('D11c');
        cachedShader.computeShaderInfo = result.computeShaderInfo;
        csCodeCount++;
        break;
      case CachedShader::TYPE_AS:
        cachedShader.relCode[0] = _MAKE4C('D11g');
        cachedShader.computeShaderInfo = result.computeShaderInfo;
        gsCodeCount++;
        break;
      case CachedShader::TYPE_MS:
        cachedShader.relCode[0] = _MAKE4C('DX9v');
        cachedShader.computeShaderInfo = result.computeShaderInfo;
        vsCodeCount++;
        break;
    }
    eastl::copy(result.bytecode.begin(), result.bytecode.end(), (uint8_t *)&cachedShader.relCode[1]);
    added_new = true;
  }

  cache_entries[c1].lv2[c2].lv3[c3].codeIdx = codeIdx;
  return added_new;
}

static CachedShader &resolve_cached_shader(int c1, int c2, int c3)
{
  static CachedShader empty;
  int idx = cache_entries[c1].lv2[c2].lv3[c3].codeIdx;
  return idx < 0 ? empty : cached_shaders_list[idx];
}
static void mark_cached_shader_entry_as_pending(int c1, int c2, int c3) { cache_entries[c1].lv2[c2].lv3[c3].codeIdx = -2; }

void ShaderParser::clear_per_file_caches()
{
  debug("[stat] cached_shaders_list.count=%d", cached_shaders_list.size());

  // erase cached data
  cached_shaders_list.clear();
  cache_entries.clear();
  psCodeCount = 0;
  vsCodeCount = 0;
  hsCodeCount = 0;
  dsCodeCount = 0;
  gsCodeCount = 0;
  csCodeCount = 0;

  CodeSourceBlocks::resetCompilation();
}

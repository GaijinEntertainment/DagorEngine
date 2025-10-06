// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "assemblyShader.h"
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shUtils.h>
#include <shaders/shFunc.h>

#include "hwSemantic.h"
#include "shaderSemantic.h"
#include "transcodeCommon.h"
#include "hwAssembly.h"
#include "variantAssembly.h"
#include "shTargetContext.h"

#include "varMap.h"
#include "shCompContext.h"
#include "globVar.h"
#include "boolVar.h"
#include "samplers.h"
#include "sh_stat.h"
#include "semUtils.h"
#include "shaderTab.h"
#include "shCompiler.h"
#include "compileResult.h"
#include "globalConfig.h"
#include "hash.h"
#include "shCompilationInfo.h"
#include "transcodeShader.h"
#include "codeBlocks.h"
#include "defer.h"
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <debug/dag_debug.h>
#include <math/random/dag_random.h>
#include "fast_isalnum.h"
#include <memory/dag_regionMemAlloc.h>
#include <util/dag_bitwise_cast.h>
#include <drv/shadersMetaData/dxil/utility.h>

#include "DebugLevel.h"
#include <EASTL/vector_map.h>
#include <EASTL/string_view.h>
#include <EASTL/set.h>
#include <EASTL/optional.h>
#include <util/dag_strUtil.h>

#include "debugSpitfile.h"

using namespace ShaderParser;

static bool is_hlsl_debug() { return shc::config().hlslDebugLevel != DebugLevel::NONE; }

#if _TARGET_PC_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_METAL
#include "hlsl2metal/asmShaderHLSL2Metal.h"
#elif _CROSS_TARGET_SPIRV
#include "hlsl2spirv/asmShaderSpirV.h"
#elif _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
#elif _CROSS_TARGET_DX11 //_TARGET_PC is also defined
#include "hlsl11transcode/asmShaders11.h"
#include <D3Dcompiler.h>
#endif

/*********************************
 *
 * class AssembleShaderEvalCB
 *
 *********************************/
AssembleShaderEvalCB::AssembleShaderEvalCB(shc::VariantContext &ctx) :
  semantic::VariantBoolExprEvalCB{ctx},
  ctx{ctx},
  sclass{ctx.shCtx().compiledShader()},
  code{ctx.parsedSemCode()},
  curvariant{&ctx.parsedPass()},
  curpass{ctx.hasParsedPass() ? &ctx.parsedPass().pass.value() : nullptr},
  stBytecodeAccum{ctx.stBytecode()},
  stCppcodeAccum{ctx.cppStcode()},
  shConst{ctx.namedConstTable()},
  parser{ctx.tgtCtx().sourceParseState().parser},
  exprParser{ctx},
  allRefStaticVars{ctx.shCtx().typeTables().referencedTypes},
  dont_render(false),
  variant(ctx.variant()),
  varMerger{ctx.tgtCtx()}
{}

eastl::optional<ShaderVarType> shtok_to_shvt(int shtok)
{
  switch (shtok)
  {
    case SHADER_TOKENS::SHTOK_int: return SHVT_INT;
    case SHADER_TOKENS::SHTOK_int4: return SHVT_INT4;
    case SHADER_TOKENS::SHTOK_float: return SHVT_REAL;
    case SHADER_TOKENS::SHTOK_float4x4: return SHVT_FLOAT4X4;
    case SHADER_TOKENS::SHTOK_float4: return SHVT_COLOR4;
    case SHADER_TOKENS::SHTOK_texture: return SHVT_TEXTURE;
    default: return eastl::nullopt;
  }
}

void AssembleShaderEvalCB::eval_static(static_var_decl &s)
{
  auto shvt = shtok_to_shvt(s.type->type->num);
  if (!shvt)
  {
    report_error(parser, s.type->type, "Unsupported shadervar type %s to declare as static/dynamic variable", s.type->type->text);
    return;
  }
  ShaderVarType t = *shvt;

  int varNameId = ctx.tgtCtx().varNameMap().addVarId(s.name->text);

  int v = code.find_var(varNameId);
  if (v >= 0)
  {
    eastl::string message(eastl::string::CtorSprintf{}, "static variable '%s' already declared in ", s.name->text);
    message += parser.get_lexer().get_symbol_location(varNameId, SymbolType::STATIC_VARIABLE);
    report_error(parser, s.name, message.c_str());
    return;
  }
  parser.get_lexer().register_symbol(varNameId, SymbolType::STATIC_VARIABLE, s.name);
  v = append_items(code.vars, 1);
  code.vars[v].type = t;
  code.vars[v].nameId = varNameId;
  code.vars[v].terminal = s.name;
  code.vars[v].dynamic = s.mode && s.mode->mode->num == SHADER_TOKENS::SHTOK_dynamic;
  if (s.no_warnings)
    code.vars[v].noWarnings = true;

  code.staticStcodeVars.add(s.name->text, v);

  bool inited = (!s.mode && s.init) || code.vars[v].dynamic || (t != SHVT_TEXTURE) || s.init;
  if (!inited)
    report_error(parser, s.name, "Variable '%s' must be inited", s.name->text);

  bool varReferenced = true;

  if (!s.mode)
  {
    int intervalNameId = ctx.tgtCtx().intervalNameMap().getNameId(s.name->text);
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

    if (sclass.stvarsAreDynamic.size() <= sv)
      sclass.stvarsAreDynamic.resize(sv + 1);
    sclass.stvarsAreDynamic[sv] = code.vars[v].dynamic;

    const bool expectingInt = t == SHVT_INT || t == SHVT_INT4;
    Color4 val = expectingInt ? Color4{bitwise_cast<float>(0), bitwise_cast<float>(0), bitwise_cast<float>(0), bitwise_cast<float>(1)}
                              : Color4{0, 0, 0, 1};
    if (s.init && s.init->expr)
    {
      shexpr::ValueType expectedValType = shexpr::VT_UNDEFINED;
      if (t == SHVT_REAL || t == SHVT_INT || t == SHVT_TEXTURE)
        expectedValType = shexpr::VT_REAL;
      else if (t == SHVT_COLOR4 || t == SHVT_INT4)
        expectedValType = shexpr::VT_COLOR4;
      else if (t == SHVT_FLOAT4X4)
      {
        report_error(parser, s.name, "float4x4 default value is not supported");
        return;
      }

      if (!exprParser.parseConstExpression(*s.init->expr, val, ExpressionParser::Context{expectedValType, expectingInt, s.name}))
      {
        report_error(parser, s.name, "Wrong expression");
        return;
      }
    }

    switch (t)
    {
      case SHVT_COLOR4: sclass.stvar[sv].defval.c4.set(val); break;
      case SHVT_REAL: sclass.stvar[sv].defval.r = val[0]; break;
      case SHVT_INT: sclass.stvar[sv].defval.i = bitwise_cast<int>(val[0]); break;
      case SHVT_INT4:
        sclass.stvar[sv].defval.i4.set(bitwise_cast<int>(val[0]), bitwise_cast<int>(val[1]), bitwise_cast<int>(val[2]),
          bitwise_cast<int>(val[3]));
        break;
      case SHVT_FLOAT4X4:
        // default value is not supported
        break;
      case SHVT_TEXTURE:
        if (real2int(val[0]) != 0)
        {
          report_error(parser, s.name, "texture may be inited only with 0");
          return;
        }
        sclass.stvar[sv].defval.texId = unsigned(BAD_TEXTUREID);
        break;
      default: G_ASSERT(0);
    }
  }
  else if (sv >= 0)
  {
    if (sclass.stvar[sv].type != t)
    {
      report_error(parser, s.name, "static var '%s' defined with different type", s.name->text);
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

void AssembleShaderEvalCB::eval_bool_decl(bool_decl &decl) { ctx.localBoolVars().add(decl, parser); }

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
  int varNameId = ctx.tgtCtx().varNameMap().getVarId(var->text);

  int vi = code.find_var(varNameId);
  if (vi < 0)
  {
    report_error(parser, var, "unknown variable '%s'", var->text);
    return;
  }

  if (v.color)
  {
    if (code.vars[vi].type != SHVT_COLOR4)
    {
      report_error(parser, v.color->color, "can't assign color to %s", ShUtils::shader_var_type_name(code.vars[vi].type));
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
    int ident_id = ctx.tgtCtx().intervalNameMap().getNameId(var->text);
    int intervalIndex = variant.intervals.getIntervalIndex(ident_id);
    if (intervalIndex != INTERVAL_NOT_INIT && !code.vars[vi].dynamic)
    {
      int stVarId = sclass.find_static_var(varNameId);
      if (stVarId < 0)
      {
        report_error(parser, var, "variable <%s> is not static var", var->text);
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
      report_error(parser, v.tex->tex, "can't assign texture to %s", ShUtils::shader_var_type_name(code.vars[vi].type));
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

    int e_texture_ident_id = ctx.tgtCtx().intervalNameMap().getNameId(var->text);
    int intervalIndex = variant.intervals.getIntervalIndex(e_texture_ident_id);
    int stVarId = intervalIndex != INTERVAL_NOT_INIT ? sclass.find_static_var(varNameId) : -1;
    if (stVarId >= 0 && !code.vars[vi].dynamic)
    {
      int opcode = shaderopcode::makeOp2(SHCOD_TEXTURE, ind, 0);
      for (int i = 0; i < sclass.shInitCode.size(); i += 2)
        if (sclass.shInitCode[i] == stVarId)
        {
          if (sclass.shInitCode[i + 1] != opcode)
            report_error(parser, v.tex->tex, "ambiguous init for static texture <%s> used in branching", var->text);
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
    report_error(parser, s.type->type, "unsupported channel type: %s", s.type->type->text);
    return;
  }
  G_ASSERT(tp >= 0);
  ch.t = tp;
  int vbus = ::channel_usage(s.usg->usage->num);
  G_ASSERT(vbus >= 0);
  if (vbus == SCUSAGE_EXTRA || vbus == SCUSAGE_LIGHTMAP)
  {
    report_error(parser, s.usg->usage, "invalid channel usage");
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
  else if (strcmp(nm, "sc1") == 0)
    return DRV3DC::EXT_BLEND_SRC1COLOR;
  else if (strcmp(nm, "isc1") == 0)
    return DRV3DC::EXT_BLEND_INVSRC1COLOR;
  else if (strcmp(nm, "sa1") == 0)
    return DRV3DC::EXT_BLEND_SRC1ALPHA;
  else if (strcmp(nm, "isa1") == 0)
    return DRV3DC::EXT_BLEND_INVSRC1ALPHA;
  else
  {
    report_error(parser, &s, "unknown blend factor");
    return -1;
  }
}

int AssembleShaderEvalCB::get_blend_op_k(const Terminal &s)
{
  const char *nm = s.text;
  if (strcmp(nm, "add") == 0)
    return DRV3DC::BLENDOP_ADD;
  else if (strcmp(nm, "min") == 0)
    return DRV3DC::BLENDOP_MIN;
  else if (strcmp(nm, "max") == 0)
    return DRV3DC::BLENDOP_MAX;
  else
  {
    report_error(parser, &s, "unknown blend op");
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
    report_error(parser, &s, "unknown stencil op");
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
    report_error(parser, &s, "unknown stencil cmp func");
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
    report_error(parser, &s, "unknown depth cmp func");
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
    report_error(parser, &s, "expected true or false");
  return -1;
}

void AssembleShaderEvalCB::eval_blend_value(const Terminal &blend_func_tok, const SHTOK_intnum *const index_tok,
  SemanticShaderPass::BlendValues &blend_values, const BlendValueType type)
{
  auto blendValue = type == BlendValueType::BlendFunc ? get_blend_op_k(blend_func_tok) : get_blend_k(blend_func_tok);
  curpass->dual_source_blending |=
    type == BlendValueType::Factor && (blendValue == DRV3DC::EXT_BLEND_SRC1COLOR || blendValue == DRV3DC::EXT_BLEND_INVSRC1COLOR ||
                                        blendValue == DRV3DC::EXT_BLEND_SRC1ALPHA || blendValue == DRV3DC::EXT_BLEND_INVSRC1ALPHA);

  if (index_tok)
  {
    auto index = semutils::int_number(index_tok->text);
    if (index < 0 || index >= shaders::RenderState::NumIndependentBlendParameters)
    {
      report_error(parser, &blend_func_tok,
        "blend value index must be less than %d. Can be increased if needed. See: "
        "shaders::RenderState::NumIndependentBlendParameters",
        shaders::RenderState::NumIndependentBlendParameters);
    }

    blend_values[index] = blendValue;
    curpass->independent_blending = true;
  }
  else
  {
    eastl::fill(blend_values.begin(), blend_values.end(), blendValue);
  }
}

void AssembleShaderEvalCB::eval_state(state_stat &s)
{
  if (s.var)
  {

    if (!s.value)
    {
      report_error(parser, s.var->var, "expected value");
      return;
    }

    const bool isPlainVal = s.value->plain_value != nullptr;
    const bool takesPlainVal = s.var->var->num != SHADER_TOKENS::SHTOK_blend_factor;

    if (isPlainVal && !takesPlainVal)
    {
      report_error(parser, s.var->var, "blend_factor takes a color literal");
      return;
    }
    else if (!isPlainVal && takesPlainVal)
    {
      report_error(parser, s.var->var, "%s does not take a color value as argument", s.var->var->text);
      return;
    }

    const bool supports_indexing =
      (s.var->var->num == SHADER_TOKENS::SHTOK_blend_src) || (s.var->var->num == SHADER_TOKENS::SHTOK_blend_dst) ||
      (s.var->var->num == SHADER_TOKENS::SHTOK_blend_asrc) || (s.var->var->num == SHADER_TOKENS::SHTOK_blend_adst) ||
      (s.var->var->num == SHADER_TOKENS::SHTOK_blend_op) || (s.var->var->num == SHADER_TOKENS::SHTOK_blend_aop) ||
      (s.var->var->num == SHADER_TOKENS::SHTOK_color_write);

    if (s.index && !supports_indexing)
    {
      report_error(parser, s.var->var, "%s[%d] is not allowed (index is not supported for this attribute", s.var->var->text,
        semutils::int_number(s.index->text));
      return;
    }

    unsigned color_write_mask = 0;

    if (takesPlainVal)
    {
      Terminal *val = s.value->plain_value->value;

      switch (s.var->var->num)
      {
        case SHADER_TOKENS::SHTOK_blend_src: eval_blend_value(*val, s.index, curpass->blend_src, BlendValueType::Factor); break;
        case SHADER_TOKENS::SHTOK_blend_dst: eval_blend_value(*val, s.index, curpass->blend_dst, BlendValueType::Factor); break;
        case SHADER_TOKENS::SHTOK_blend_asrc: eval_blend_value(*val, s.index, curpass->blend_asrc, BlendValueType::Factor); break;
        case SHADER_TOKENS::SHTOK_blend_adst: eval_blend_value(*val, s.index, curpass->blend_adst, BlendValueType::Factor); break;
        case SHADER_TOKENS::SHTOK_blend_op: eval_blend_value(*val, s.index, curpass->blend_op, BlendValueType::BlendFunc); break;
        case SHADER_TOKENS::SHTOK_blend_aop: eval_blend_value(*val, s.index, curpass->blend_aop, BlendValueType::BlendFunc); break;
        case SHADER_TOKENS::SHTOK_alpha_to_coverage: curpass->alpha_to_coverage = get_bool_const(*val); break;

        // Zero based. 0 means one instance which is the usual non-instanced case.
        case SHADER_TOKENS::SHTOK_view_instances: curpass->view_instances = semutils::int_number(val->text) - 1; break;

        case SHADER_TOKENS::SHTOK_cull_mode:
        {
          const char *v = val->text;
          if (strcmp(v, "ccw") == 0)
            curpass->cull_mode = CULL_CCW;
          else if (strcmp(v, "cw") == 0)
            curpass->cull_mode = CULL_CW;
          else if (strcmp(v, "none") == 0)
            curpass->cull_mode = CULL_NONE;
          else
            report_error(parser, val, "unknown cull mode <%s>", val->text);
        }
        break;

        case SHADER_TOKENS::SHTOK_stencil: curpass->stencil = get_bool_const(*val); break;

        case SHADER_TOKENS::SHTOK_stencil_func: curpass->stencil_func = get_stencil_cmpf_k(*val); break;

        case SHADER_TOKENS::SHTOK_stencil_ref:
        {
          int v = semutils::int_number(val->text);
          if (v < 0)
            v = 0;
          else if (v > 255)
            v = 255;
          curpass->stencil_ref = v;
        }
        break;

        case SHADER_TOKENS::SHTOK_stencil_pass: curpass->stencil_pass = get_stensil_op_k(*val); break;

        case SHADER_TOKENS::SHTOK_stencil_fail: curpass->stencil_fail = get_stensil_op_k(*val); break;

        case SHADER_TOKENS::SHTOK_stencil_zfail: curpass->stencil_zfail = get_stensil_op_k(*val); break;

        case SHADER_TOKENS::SHTOK_color_write:
          color_write_mask = 0;
          if (!val)
            report_error(parser, s.var->var, "color_write should be a value of int, rgba swizzle, true or false");
          else if (val->num == SHADER_TOKENS::SHTOK__true || val->num == SHADER_TOKENS::SHTOK__false)
            color_write_mask = get_bool_const(*val) ? 0xF : 0;
          else if (val->num == SHADER_TOKENS::SHTOK_intnum)
          {
            unsigned v = semutils::int_number(val->text);
            if (v & (~15))
              report_error(parser, s.var->var, "color_write should be value of [0 15]");
            color_write_mask = v & 15;
          }
          else if (val->num == SHADER_TOKENS::SHTOK_ident && s.static_var)
          {
            if (s.index)
            {
              report_error(parser, s.var->var, "color_write from static var \"%s\" cannot be used with index", val->text);
              return;
            }

            auto [vi, vt, is_global] = semantic::lookup_state_var(*val, ctx);
            if (vi < -1)
            {
              report_error(parser, s.var->var, "missing var \"%s\" for color_write statement", val->text);
              return;
            }
            if (is_global || code.vars[vi].dynamic || vt != SHVT_INT)
            {
              report_error(parser, s.var->var, "static int var \"%s\" required for color_write statement", val->text);
              return;
            }

            int varNameId = ctx.tgtCtx().varNameMap().getVarId(val->text);
            int sv = sclass.find_static_var(varNameId);
            curpass->color_write = sclass.stvar[sv].defval.i;
            break;
          }
          else if (val->num == SHADER_TOKENS::SHTOK_ident)
          {
            unsigned v = 0, v2 = 0;
            const char *valStr = val->text;
            for (; valStr && valStr[0];)
            {
              unsigned pval = v;
              char swizzle[] = "rgba";
              for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1)
                if (!(v & bit) && valStr[0] == swizzle[i])
                {
                  v |= bit;
                  valStr++;
                }
              if (pval == v)
                break;
            }

            if (valStr[0] != 0)
              report_error(parser, s.var->var, "color_write should be swizzle of 'rgba'");

            color_write_mask = v;
          }
          else if (!val)
          {
            report_error(parser, s.var->var, "color_write should be a value of int, rgba swizzle, true or false");
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
              report_error(parser, s.var->var, "bad index for %s[%d]", s.var->var->text, idx);
          }
          break;

        case SHADER_TOKENS::SHTOK_z_test: curpass->z_test = get_bool_const(*val); break;

        case SHADER_TOKENS::SHTOK_z_func:
        {
          get_depth_cmpf_k(*val, curpass->z_func);
          break;
        }

        case SHADER_TOKENS::SHTOK_z_write:
          if (curpass->z_write >= 0)
            report_error(parser, s.var->var, "z_write already specified for this render pass");
          else if (val->num == SHADER_TOKENS::SHTOK__true || val->num == SHADER_TOKENS::SHTOK__false)
          {
            curpass->z_write = get_bool_const(*val);
          }
          else if (val->num == SHADER_TOKENS::SHTOK_intnum)
          {
            int v = semutils::int_number(val->text);
            curpass->z_write = v ? 1 : 0;
          }
          else
            report_error(parser, val, "expected boolean, integer number or int variable");
          break;

        case SHADER_TOKENS::SHTOK_slope_z_bias:
        case SHADER_TOKENS::SHTOK_z_bias:
        {
          bool slope = (s.var->var->num == SHADER_TOKENS::SHTOK_slope_z_bias);
          if ((!slope && curpass->z_bias) || (slope && curpass->slope_z_bias))
            report_error(parser, s.var->var, "%sz_bias already specified for this render pass", slope ? "slope_" : "");
          else if (val->num == SHADER_TOKENS::SHTOK_float || val->num == SHADER_TOKENS::SHTOK_intnum)
          {
            real v = semutils::real_number(val->text);
            if (slope)
            {
              curpass->slope_z_bias = true;
              curpass->slope_z_bias_val = v;
            }
            else
            {
              curpass->z_bias = true;
              curpass->z_bias_val = v;
            }
          }
          else
            report_error(parser, val, "expected real number or real variable");
        }
        break;

        default: G_ASSERT(0);
      }
    }
    else
    {
      G_ASSERT(s.var->var->num == SHADER_TOKENS::SHTOK_blend_factor);

      // Construct a fake expression holding a color value
      // @TODO: make a utility for constructing fake ast nodes
      ShaderTerminal::arithmetic_operand o{};
      o.unary_op = nullptr;
      o.var_name = nullptr;
      o.real_value = nullptr;
      o.func = nullptr;
      o.expr = nullptr;
      o.cmask = nullptr;
      o.color_value = s.value->color_value;
      ShaderTerminal::arithmetic_expr_md em{};
      em.lhs = &o;
      ShaderTerminal::arithmetic_expr e{};
      e.lhs = &em;

      Color4 val{};

      if (!exprParser.parseConstExpression(e, val, ExpressionParser::Context{shexpr::VT_COLOR4, false, s.var->var}))
      {
        report_error(parser, s.var->var, "Wrong expression for blend_factor color");
        return;
      }

      if (val.r < 0.f || val.g < 0.f || val.b < 0.f || val.a < 0.f || val.r > 1.f || val.g > 1.f || val.b > 1.f || val.a > 1.f)
      {
        report_error(parser, s.var->var, "Wrong expression for blend_factor color: all components must be in the [0, 1] range");
        return;
      }

      curpass->blend_factor = e3dcolor(val);
      curpass->blend_factor_specified = true;
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
        report_error(parser, s.var, "%sz_bias already specified for this render pass", slope ? "slope_" : "");
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
        report_error(parser, s.value, "expected real number or real variable");
    }
    break;
    default: G_ASSERT(0);
  }
}

void AssembleShaderEvalCB::eval_external_block(external_state_block &state_block)
{
  auto stageMaybe = semantic::parse_state_block_stage(state_block.scope->text);
  if (!stageMaybe)
    report_error(parser, state_block.scope, "external block <%s> is not one of <vs, ps, cs>", state_block.scope->text);
  else
    addBlockType(state_block.scope->text, state_block.scope);

  ShaderStage stage = *stageMaybe;
  ctx.cppStcode().cppStcode.reportStageUsage(stage);

  auto eval_if_stat = [this, stage](state_block_if_stat &s, auto &&eval_if_stat) -> void {
    G_ASSERT(s.expr);

    auto eval_stats = [&, this](auto &stats) {
      for (auto &stat : stats)
      {
        if (stat->stblock_if_stat)
          eval_if_stat(*stat->stblock_if_stat, eval_if_stat);
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

  for (auto stat : state_block.stblock_stat)
  {
    if (stat->stblock_if_stat)
      eval_if_stat(*stat->stblock_if_stat, eval_if_stat);
    else
      eval_external_block_stat(*stat, stage);
  }
}

void AssembleShaderEvalCB::eval_external_block_stat(state_block_stat &s, ShaderStage stage)
{
  semantic::VariableType vt = semantic::parse_named_const_type(s);
  if (vt == semantic::VariableType::Unknown)
  {
    report_error(parser, &s, "Invalid preshader var type");
    return;
  }

  PreshaderStat stat{&s, stage, vt};

  if (s.reg || s.reg_arr)
    preshaderHardcodedStats.emplace_back(stat);
  else if (semantic::vt_is_numeric(vt))
    preshaderScalarStats.emplace_back(stat);
  else if (semantic::vt_is_static_texture(vt))
    preshaderStaticTextureStats.emplace_back(stat);
  else
    preshaderDynamicResourceStats.emplace_back(stat);
}

void AssembleShaderEvalCB::process_external_block_stat(const PreshaderStat &stat)
{
  using semantic::VariableType;

  auto &[st, stage, vt] = stat;

  G_ASSERT(st->var || st->arr || st->reg || st->reg_arr);

  if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
    usedPreshaderStatements.push_back(uintptr_t(st));

  G_ASSERT(st->var || st->arr || st->reg || st->reg_arr);
  if (!st->var || st->var->val->builtin_var)
    return compile_external_block_stat(stat);

  // clang-format off
  switch (vt)
  {
    case VariableType::f1:
    case VariableType::f2:
    case VariableType::f3:
    case VariableType::i1:
    case VariableType::i2:
    case VariableType::i3:
    case VariableType::u1:
    case VariableType::u2:
    case VariableType::u3:
      break;
    default:
      return compile_external_block_stat(stat);
  }
  // clang-format on

  auto exprIsDynamic = [&, this]() {
    if (!st->var->val->expr)
      return true;

    ComplexExpression colorExpr(st->var->val->expr, shexpr::VT_COLOR4);

    if (!exprParser.parseExpression(*st->var->val->expr, &colorExpr,
          ExpressionParser::Context{shexpr::VT_COLOR4, vt_is_integer(vt), st->var->var->name}))
    {
      return false;
    }

    if (Symbol *s = colorExpr.hasDynamicAndMaterialTermsAt())
    {
      hasDynStcodeRelyingOnMaterialParams = true;
      exprWithDynamicAndMaterialTerms = s;
    }

    if (!colorExpr.collapseNumbers(parser))
      return false;

    Color4 v;
    if (colorExpr.evaluate(v, parser))
      return false;

    return colorExpr.isDynamic();
  };

  if (ctx.shCtx().blockLevel() != ShaderBlockLevel::GLOBAL_CONST && exprIsDynamic())
    varMerger.addConstStat(*st, stage);
  else
    varMerger.addBufferedStat(*st, stage);
}

void AssembleShaderEvalCB::compile_external_block_stat(const PreshaderStat &stat)
{
  RegionMemAlloc rm_alloc(4 << 20, 4 << 20);

  const auto parsedDefMaybe = semantic::parse_named_const_definition(*stat.stat, stat.stage, stat.vt, ctx, &rm_alloc);
  if (!parsedDefMaybe)
    return;

  const semantic::NamedConstDefInfo &def = *parsedDefMaybe;

  if (def.isBindless)
    code.vars[def.bindlessVarId].texType = def.shvarTexType;

  hasDynStcodeRelyingOnMaterialParams |= def.hasDynStcodeRelyingOnMaterialParams;
  if (def.exprWithDynamicAndMaterialTerms)
    exprWithDynamicAndMaterialTerms = def.exprWithDynamicAndMaterialTerms;

  // Validate that the same shadervar has not been used both as a uav and an srv resource in the same pass
  if (def.shvarType == SHVT_TEXTURE || def.shvarType == SHVT_BUFFER)
  {
    const bool isRw = def.type == semantic::VariableType::uav;
    const auto setForVar = [this](bool isRw, bool isGlobal) -> decltype(uavGlobalShadervarRefs) & {
      return isGlobal ? (isRw ? uavGlobalShadervarRefs : srvGlobalShadervarRefs)
                      : (isRw ? uavLocalShadervarRefs : srvLocalShadervarRefs);
    };

    for (const auto &elem : def.initializer)
    {
      G_ASSERT(elem.isGlobalVar() || elem.isMaterialVar());
      bool isGlobal = elem.isGlobalVar();
      const char *varName = elem.varName();

      auto &set = setForVar(isRw, isGlobal);
      const auto &conflictSet = setForVar(!isRw, isGlobal);

      if (auto it = conflictSet.find(varName); it != conflictSet.end())
      {
        const auto strForUsage = [](bool isRw) { return isRw ? "UAV" : "SRV"; };
        report_error(parser, stat.stat, "The same %s shadervar is used as a %s, while being previously used as a %s at %s(%d, %d)",
          isGlobal ? "global" : "material", strForUsage(isRw), strForUsage(!isRw),
          parser.get_lexer().get_filename(it->second->file_start), it->second->line_start, it->second->col_start);
      }

      set.emplace(varName, stat.stat);
    }
  }

  {
    Terminal *const var = def.varTerm;
    const int hardcoded_reg = def.hardcodedRegister;
    const int registers_count = def.registerSize;
    const ShaderStage stage = def.stage;
    const String &varName = def.mangledName;
    const HlslRegisterSpace reg_space = def.regSpace;

    const auto hnd = shConst.addConst(stage, varName, reg_space, registers_count, hardcoded_reg, def.isDynamic,
      ctx.shCtx().blockLevel() == ShaderBlockLevel::GLOBAL_CONST);
    if (!is_valid(hnd))
    {
      // @TODO: implement correct expression comparison to not allow silent redecls
      if (!hnd.isBufConst)
      {
        report_error(parser, var, "Redeclaration for variable <%s> in external block is not allowed", varName.c_str());
      }
      else
      {
        report_debug_message(parser, *var,
          "Redeclaration for variable <%s> for %s is skipped. If it's declaration differs from previous, this is UB.", varName.c_str(),
          def.isDynamic ? "global const block" : "static cbuf");
      }
      return;
    }

    const int reg = shConst.getSlot(hnd).regIndex;

    auto builtHlslMaybe = assembly::build_hlsl_decl_for_named_const(def, ctx, reg, varMerger);
    if (!builtHlslMaybe)
      return;

    assembly::NamedConstDeclarationHlsl &builtHlsl = *builtHlslMaybe;

    ctx.namedConstTable().addHlslDecl(hnd, eastl::move(builtHlsl.definition), eastl::move(builtHlsl.postfix));

    if (!assembly::build_stcode_for_named_const<assembly::StcodeBuildFlagsBits::ALL>(def, reg, ctx, &rm_alloc))
      return;

    stcode_vars.push_back(var);
  }

  if (def.pairSamplerTmpDecl)
  {
    G_ASSERT(ctx.shCtx().blockLevel() != ShaderBlockLevel::GLOBAL_CONST);

    const String samplerConstName{0, "%s%s", def.mangledName.c_str(), def.pairSamplerBindSuffix};
    const auto hnd = shConst.addConst(def.stage, samplerConstName, HLSL_RSPACE_S, 1, def.hardcodedRegister, def.isDynamic, false);
    if (!is_valid(hnd))
    {
      report_error(parser, def.varTerm, "Redeclaration for variable <%s> with implicit pair sampler is not allowed",
        def.mangledName.c_str());
      return;
    }

    const int reg = shConst.getSlot(hnd).regIndex;
    String hlsl = assembly::build_hlsl_for_pair_sampler(def.mangledName.c_str(), def.pairSamplerIsShadow, reg, ctx);
    shConst.addHlslDecl(hnd, eastl::move(hlsl), {});

    if (def.isDynamic && def.hardcodedRegister == -1)
    {
      if (def.pairSamplerIsGlobal)
        ctx.tgtCtx().samplers().add(*def.pairSamplerTmpDecl, parser);
      else
        add_dynamic_sampler_for_stcode(code, sclass, *def.pairSamplerTmpDecl, parser, ctx.tgtCtx().varNameMap());

      auto [samplerVarId, varType, isGlobal] = semantic::lookup_state_var(*def.pairSamplerTmpDecl->name, ctx);
      G_ASSERT(isGlobal == def.pairSamplerIsGlobal);
      G_ASSERT(varType == SHVT_SAMPLER);

      assembly::build_stcode_for_pair_sampler<assembly::StcodeBuildFlagsBits::ALL>(samplerConstName.c_str(),
        def.pairSamplerName.c_str(), reg, def.stage, samplerVarId, def.pairSamplerIsGlobal, ctx);
    }
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
        reserveSpecialCbufferAt(HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF, MATERIAL_PARAMS_CONST_BUF_REGISTER);
        continue;
      }
      if (s.name[i]->text == "__static_multidraw_cbuf"sv)
      {
        // Currently we support multidraw constbuffers only for bindless material version.
        reserveSpecialCbufferAt(HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF, MATERIAL_PARAMS_CONST_BUF_REGISTER);
        shConst.multidrawCbuf = true;
        continue;
      }
      if (s.name[i]->text == "__draw_id"sv)
      {
        continue;
      }
    }
    ShaderStateBlock *b = (s.name[i]->num == SHADER_TOKENS::SHTOK_none) ? ctx.tgtCtx().blocks().emptyBlock()
                                                                        : ctx.tgtCtx().blocks().findBlock(s.name[i]->text);
    if (!b)
    {
      report_error(parser, s.name[i], "can't support undefined block <%s>", s.name[i]->text);
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
      report_error(parser, s.name[i], "double support for block <%s>", s.name[i]->text);
      continue;
    }

    if (b->layerLevel < ShaderBlockLevel::SHADER && !b->canBeSupportedBy(ctx.shCtx().blockLevel()))
    {
      report_error(parser, s.name[i], "block <%s>, layer %d, cannot be supported here", s.name[i]->text, int(b->layerLevel));
      continue;
    }

    if (b->shConst.multidrawCbuf)
      shConst.multidrawCbuf = true;

    if (b->layerLevel == ShaderBlockLevel::GLOBAL_CONST)
      shConst.globConstBlk = b;
    else
    {
      for_each_hlsl_reg_space([this, b](HlslRegisterSpace space) {
        auto vr = shConst.vertexRegAllocators[space].reserveAllFrom(b->shConst.vertexRegAllocators[space]);
        auto pcr = shConst.pixelOrComputeRegAllocators[space].reserveAllFrom(b->shConst.pixelOrComputeRegAllocators[space]);

        auto reportFailure = [this, b, space](ShaderStage stage) {
          auto propsField = stage == STAGE_VS ? &NamedConstBlock::vertexProps : &NamedConstBlock::pixelProps;
          auto regAllocsField =
            stage == STAGE_VS ? &NamedConstBlock::vertexRegAllocators : &NamedConstBlock::pixelOrComputeRegAllocators;
          auto errorMsg = string_f("Supported blk registers overlap at space %c for %s shader\n", HLSL_RSPACE_ALL_SYMBOLS[space],
            SHADER_STAGE_NAMES[stage]);
          errorMsg.append_sprintf("Trying to support block %s. ", b->name.c_str());
          errorMsg += get_reg_alloc_dump((b->shConst.*regAllocsField)[stage], space, b->shConst.makeInfoProvider(propsField, space));

          errorMsg.append("\nConflicting blocks:\n");
          eastl::vector_set<const ShaderStateBlock *> conflictingBlocks{};
          auto collectBlocks = [&](const NamedConstBlock &consts, auto &&self) -> void {
            auto processOneBlock = [&](const auto *blk) {
              auto regalloc = (blk->shConst.*regAllocsField)[space]; // Copy out to simulate collision
              if (auto allocRes = regalloc.reserveAllFrom((b->shConst.*regAllocsField)[space]); !allocRes)
              {
                conflictingBlocks.insert(blk);
                errorMsg.append_sprintf("%s, ", blk->name.c_str());
                errorMsg +=
                  get_reg_alloc_dump((blk->shConst.*regAllocsField)[stage], space, blk->shConst.makeInfoProvider(propsField, space));
              }
            };
            for (const auto &blk : consts.suppBlk)
            {
              if (conflictingBlocks.find(blk.get()) != conflictingBlocks.end())
                continue;
              self(blk->shConst, self);
              processOneBlock(blk.get());
            }
            if (consts.globConstBlk && conflictingBlocks.find(consts.globConstBlk) == conflictingBlocks.end())
            {
              self(consts.globConstBlk->shConst, self);
              processOneBlock(consts.globConstBlk);
            }
          };

          collectBlocks(shConst, collectBlocks);
        };

        if (!vr)
          reportFailure(STAGE_VS);
        if (!pcr)
          reportFailure(isCompute() ? STAGE_CS : STAGE_PS);
      });
      shConst.suppBlk.push_back(b);
    }
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
    report_error(parser, s.name, "bad renderStage <%s>", nm);
  else if (code.renderStageIdx < 0)
  {
    code.renderStageIdx = idx;
    code.flags = (idx & SC_STAGE_IDX_MASK) | (code.flags & ~SC_STAGE_IDX_MASK);
    if (code.renderStageIdx)
      code.flags |= SC_NEW_STAGE_FMT;
  }
  else if (code.renderStageIdx != idx)
  {
    report_error(parser, s.name, "renderStageIdx tries to change from %d (%s) to %d (%s)", code.renderStageIdx,
      renderStageToIdxMap.getName(code.renderStageIdx), idx, renderStageToIdxMap.getName(idx));
  }
}
void AssembleShaderEvalCB::eval_command(shader_directive &s)
{
  switch (s.command->num)
  {
    case SHADER_TOKENS::SHTOK_no_ablend: curpass->force_noablend = true; return;
    case SHADER_TOKENS::SHTOK_dont_render:
      dont_render = true;
      throw GsclStopProcessingException();
      return;
    case SHADER_TOKENS::SHTOK_no_dynstcode: no_dynstcode = s.command; return;
    case SHADER_TOKENS::SHTOK_render_trans:
    {
      int idx = renderStageToIdxMap.getNameId("trans");
      if (idx < 0)
        report_error(parser, s.command, "bad renderStage <%s>", "trans");
      else if (code.renderStageIdx < 0)
      {
        code.renderStageIdx = idx;
        code.flags = (idx & SC_STAGE_IDX_MASK) | (code.flags & ~SC_STAGE_IDX_MASK);
        code.flags |= SC_NEW_STAGE_FMT;
      }
      else if (code.renderStageIdx != idx)
      {
        report_error(parser, s.command, "renderStageIdx tries to change from %d (%s) to %d (%s)", code.renderStageIdx,
          renderStageToIdxMap.getName(code.renderStageIdx), idx, renderStageToIdxMap.getName(idx));
      }
    }
      return;
    default: G_ASSERT(0);
  }
}

// clang-format off
// clang-format linearizes this function
void AssembleShaderEvalCB::eval_error_stat(error_stat &s)
{
  report_error(parser, s.message, s.message->text);
}
// clang-format on

void AssembleShaderEvalCB::compilePreshader()
{
  G_ASSERT(stBytecodeAccum.stcode.empty());
  G_ASSERT(stBytecodeAccum.stblkcode.empty());
  G_ASSERT(!stCppcodeAccum.cppStcode.hasCode());
  G_ASSERT(!stCppcodeAccum.cppStblkcode.hasCode());

  // Push header for static stcode straight away to avoid push-front copies
  // Format:
  // 1: SHCOD_STATIC_MULTIDRAW_BLOCK/SHCOD_STATIC_BLOCK, #consts
  // 2: if not bindless: vsTexBase [8], vsSamplerBase [4], vsTexCount [4], psTexBase [8], psSamplerBase [4], psTexCount [4]
  //    else: 0, 0, 0, 0
  const bool needsStblkcodeHeader = ctx.shCtx().blockLevel() == ShaderBlockLevel::SHADER && !isCompute();
  if (needsStblkcodeHeader)
  {
    stBytecodeAccum.push_stblkcode(shaderopcode::makeOp1(0, 0));
    stBytecodeAccum.push_stblkcode(shaderopcode::makeData4(0, 0, 0, 0));
  }

  // Then, process hardcoded preshader stats (to be able to allocate around them)
  for (auto &s : preshaderHardcodedStats)
    compile_external_block_stat(s);

  // Then, process all numeric stats. By doing them first, we can infer whether special constbuffers are actually required, thus
  // obtaining all the info needed to allocate other cbuffer registers.
  for (auto &s : preshaderScalarStats)
  {
    eastl::visit(
      [this](auto &&s) {
        if constexpr (eastl::is_same_v<eastl::remove_reference_t<decltype(s)>, PreshaderStat>)
          process_external_block_stat(s);
        else
          process_shader_locdecl(*s);
      },
      s);
  }

  varMerger.mergeAllVars(this);

  // Then, if bindless is used, we just compile the stats for static textures, as they are going to emit uint2 elements to the material
  // cbuf. @TODO: introduce packing with other material consts.
  if (needsStblkcodeHeader)
  {
    // @TODO: validate that compute stuff does NOT need an stblkcode header
    if (shc::config().enableBindless)
    {
      for (auto &s : preshaderStaticTextureStats)
        process_external_block_stat(s);
    }
    // Otherwise, we need to procure a contiguous range in the t space for slot textures and a range in the s space for samplers
    else
    {
      static_assert(STAGE_PS < STAGE_VS && STAGE_CS < STAGE_VS);

      fast_sort(preshaderStaticTextureStats, [](const PreshaderStat &s1, const PreshaderStat &s2) { return s1.stage < s2.stage; });
      auto pivot = eastl::find_if(preshaderStaticTextureStats.begin(), preshaderStaticTextureStats.end(),
        [](const PreshaderStat &s) { return s.stage == STAGE_VS; });
      dag::Span<PreshaderStat> psTexStats{preshaderStaticTextureStats.begin(), pivot - preshaderStaticTextureStats.begin()},
        vsTexStats{pivot, preshaderStaticTextureStats.end() - pivot};

      const uint32_t vsRangeExtent = uint32_t{vsTexStats.size()};
      const uint32_t psRangeExtent = uint32_t{psTexStats.size()};
      G_ASSERT(vsRangeExtent < (1 << 4));
      G_ASSERT(psRangeExtent < (1 << 4));

      if (!shConst.initSlotTextureSuballocators(vsRangeExtent, psRangeExtent))
      {
        sh_debug(SHLOG_ERROR, "Failed to allocate %d vs static texture range and %d ps static texture range", vsRangeExtent,
          psRangeExtent);
        return;
      }

      for (auto &s : preshaderStaticTextureStats)
        compile_external_block_stat(s);

      auto [vsTexRangeBase, _1] = shConst.slotTextureSuballocators.vsTex.getRange();
      auto [psTexRangeBase, _2] = shConst.slotTextureSuballocators.psTex.getRange();
      auto [vsSamplerRangeBase, _3] = shConst.slotTextureSuballocators.vsSamplers.getRange();
      auto [psSamplerRangeBase, _4] = shConst.slotTextureSuballocators.psSamplers.getRange();

      uint8_t vsSamplerBaseAndExtentPacked = (uint8_t(vsSamplerRangeBase << 4)) | (uint8_t(vsRangeExtent & 0xF));
      uint8_t psSamplerBaseAndExtentPacked = (uint8_t(psSamplerRangeBase << 4)) | (uint8_t(psRangeExtent & 0xF));
      stBytecodeAccum.stblkcode[1] =
        shaderopcode::makeData4(vsTexRangeBase, vsSamplerBaseAndExtentPacked, psTexRangeBase, psSamplerBaseAndExtentPacked);

      if (curpass)
      {
        curpass->vsTexSmpRange = SlotTexturesRangeInfo{uint8_t(vsTexRangeBase), uint8_t(vsSamplerRangeBase), uint8_t(vsRangeExtent)};
        curpass->psTexSmpRange = SlotTexturesRangeInfo{uint8_t(psTexRangeBase), uint8_t(psSamplerRangeBase), uint8_t(psRangeExtent)};
      }
    }

    auto [constBase, constCap] = shConst.bufferedConstsRegAllocator.getRange();
    if (constBase > 0)
    {
      sh_debug(SHLOG_ERROR, "Invalid allocation of material cbuf registers: range is [%d, %d), but it must begin at 0", constBase,
        constCap);
      return;
    }
    const auto signOpcod = shConst.multidrawCbuf ? SHCOD_STATIC_MULTIDRAW_BLOCK : SHCOD_STATIC_BLOCK;
    stBytecodeAccum.stblkcode[0] = shaderopcode::makeOp1(signOpcod, constCap);

    if (shConst.multidrawCbuf)
      ctx.cppStcode().cppStblkcode.reportMutlidrawSupport();
  }

  // Now, based on gathered consts we reserve slots for cbuf-s that will hold them

  // Concervatively reserve 0 for implicit cbuf (as it may be used via hlsl-hardcoded regs)
  // @TODO: try and collect hardcoded regs from parsed code blocks to factor them into the allocators
  if (ctx.shCtx().blockLevel() != ShaderBlockLevel::GLOBAL_CONST)
    reserveSpecialCbufferAt(HlslSlotSemantic::RESERVED_FOR_IMPLICIT_CONST_CBUF, 0);

  // Then, reserve material const buf if we have static consts (or remove reservation if we don't)
  if (ctx.shCtx().blockLevel() == ShaderBlockLevel::SHADER) // static cbuf is only used by shaders, not blocks
  {
    if (shConst.bufferedConstsRegAllocator.hasRegs())
    {
      G_ASSERT(!isCompute());
      reserveSpecialCbufferAt(HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF, MATERIAL_PARAMS_CONST_BUF_REGISTER);
    }
    else
    {
      // If there are no regs to fill, but support blocks declared supp __static_cbuf, we unreserve
      G_VERIFY(shConst.vertexRegAllocators[HLSL_RSPACE_B].unreserveIfUsed(HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF,
        MATERIAL_PARAMS_CONST_BUF_REGISTER));
      G_VERIFY(shConst.pixelOrComputeRegAllocators[HLSL_RSPACE_B].unreserveIfUsed(HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF,
        MATERIAL_PARAMS_CONST_BUF_REGISTER));
    }
  }

  // Reserve special global consts block slot if such a block was declared
  // @TODO: find out the reason why just checking supp blks is not enough (this was how it was done before too)
  if (ctx.tgtCtx().blocks().countBlock(ShaderBlockLevel::GLOBAL_CONST) > 0)
    reserveSpecialCbufferAt(HlslSlotSemantic::RESERVED_FOR_GLOBAL_CONST_CBUF, GLOBAL_CONST_BUF_REGISTER);

  // Finally, reservations are completed, and we process automatic preshaders decls for dynamic resources
  // @TODO: use the fact that we know these to be dynamic and don't recalculate
  for (auto &s : preshaderDynamicResourceStats)
    process_external_block_stat(s);

  // After assebmling everything, do validations on the code that has been assembled
  validateDynamicConstsForMultidraw();

  if (curpass)
  {
    curpass->psOrCsConstRange = shConst.pixelOrComputeRegAllocators[HLSL_RSPACE_C].getRange(HlslSlotSemantic::ALLOCATED);
    curpass->vsConstRange = shConst.vertexRegAllocators[HLSL_RSPACE_C].getRange(HlslSlotSemantic::ALLOCATED);
    curpass->bufferedConstRange = shConst.bufferedConstsRegAllocator.getRange(HlslSlotSemantic::ALLOCATED);
  }
}

void AssembleShaderEvalCB::end_pass()
{
  Terminal *terminal = ctx.shCtx().declTerm();

  SemanticShaderPass &p = *curpass;

  bool usesBlendFactor = false;
  for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
  {
    if (p.blend_src[i] * p.blend_dst[i] < 0)
      report_error(parser, terminal, "no blend src/dst specified for attachement: %d", i);

    if (p.blend_src[i] == DRV3DC::BLEND_BLENDFACTOR || p.blend_src[i] == DRV3DC::BLEND_INVBLENDFACTOR ||
        p.blend_dst[i] == DRV3DC::BLEND_BLENDFACTOR || p.blend_dst[i] == DRV3DC::BLEND_INVBLENDFACTOR)
    {
      usesBlendFactor = true;
    }

    if (!p.force_noablend)
    {
      if (p.blend_asrc[i] * p.blend_adst[i] < 0)
        report_error(parser, terminal, "no blend asrc/adst specified for attachement: %d", i);

      if (p.blend_asrc[i] == DRV3DC::BLEND_BLENDFACTOR || p.blend_asrc[i] == DRV3DC::BLEND_INVBLENDFACTOR ||
          p.blend_adst[i] == DRV3DC::BLEND_BLENDFACTOR || p.blend_adst[i] == DRV3DC::BLEND_INVBLENDFACTOR)
      {
        usesBlendFactor = true;
      }
    }

    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage
    // In 'Dual-Source Color Blending' section: Valid blend operations include: add, subtract and revsubtract
    // (and we only have add/min/max).
    if (p.dual_source_blending && p.blend_op[i] != DRV3DC::BLENDOP_ADD)
      report_error(parser, terminal, "dual source blending (sc1/isc1/sa1/isa1) only supports 'add' blend operation");
  }

  if (p.independent_blending && p.dual_source_blending)
  {
    report_error(parser, terminal,
      "dual source blending (sc1/isc1/sa1/isa1) is incompatible with independent because only one final render target is supported");
  }

#if _CROSS_TARGET_SPIRV
  if (p.dual_source_blending)
  {
    report_debug_message(parser, *terminal,
      "dual source blending (sc1/isc1/sa1/isa1) for vulkan is not supported by all devices. Be sure to check driver caps before "
      "invoking this shader!");
  }
#endif

  if (usesBlendFactor && !p.blend_factor_specified)
  {
    report_debug_message(parser, *terminal,
      "WARNING: blend factor blending is used for src/dst/asrc/adst blending, but the factor is not specified. "
      "Make sure to prepare the shader invocation with a call to d3d::set_blend_factor.");
  }
  else if (!usesBlendFactor && p.blend_factor_specified)
  {
    report_error(parser, terminal,
      "blend_factor was specified, but the blend state does not use it. Use bf/ibf to use blend factor for blending.");
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
    report_error(parser, terminal, "View instances are only supported on DX12 and PS5 for now!");
#endif

#if _CROSS_TARGET_DX12
    if (p.view_instances > 3)
      report_error(parser, terminal, "There can be a maximum of 4 view instances with DX12!");
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
    stBytecodeAccum.push_stblkcode(shaderopcode::makeOp2_8_16(SHCOD_IMM_REAL1, int(stBytecodeAccum.regAllocator->add_reg()), 0));
  }

  bool accept = false;
  if (!dont_render)
  {
    hlsls.forEach([&accept](HlslCompile &comp) { accept |= comp.hasCompilation(); });

    if (!accept) // if the shader has no compile directives, treat it as implicit dont_render w/ a breadcrumb to debug log
    {
      sh_debug(SHLOG_INFO, "WARNING: a pass doesn't contain compile directives, but is not explicitly marked as dont_render.");
      dont_render = true;
    }
  }
  const char *shname = ctx.shCtx().name();
  if (accept)
  {
    curvariant->suppBlk.resize(shConst.suppBlk.size());
    for (size_t i = 0; i < shConst.suppBlk.size(); i++)
      curvariant->suppBlk[i] = shConst.suppBlk[i];


    if (isCompute())
    {
      if (isGraphics() || isMesh())
      {
        report_error(parser, terminal, "%s: compute shader is not combineable with VS/HS/DS/GS/MS/AS/PS", shname);
        accept = false;
        goto end;
      }
    }
    if (isMesh())
    {
      if (hlsls.fields.vs.hasCompilation() || hlsls.fields.hs.hasCompilation() || hlsls.fields.ds.hasCompilation() ||
          hlsls.fields.gs.hasCompilation())
      {
        report_error(parser, terminal, "%s: mesh shader is not combineable with VS/HS/DS/GS", shname);
        accept = false;
        goto end;
      }
    }
    if (hlsls.fields.as.hasCompilation())
    {
      if (!hlsls.fields.ms.hasCompilation())
      {
        report_error(parser, terminal, "%s: amplification shader needs a paired mesh shader", shname);
        accept = false;
        goto end;
      }
    }
    if ((hlsls.fields.hs.hasCompilation() || hlsls.fields.ds.hasCompilation()) &&
        !(hlsls.fields.hs.hasCompilation() && hlsls.fields.ds.hasCompilation()))
    {
      report_error(parser, terminal, "%s: incomplete hull-tessellation/domain shader stage", shname);
      accept = false;
      goto end;
    }

    for_each_hlsl_stage([this](HlslCompilationStage stage) { hlsl_compile(stage); });

    if (ErrorCounter::curShader().err > 0)
    {
      sh_process_errors();
      accept = false;
      goto end;
    }

    if (!isCompute())
    {
      if (!curpass->fsh.data() && hlsls.fields.ps.hasCompilation())
      {
        report_error(parser, terminal, "%s: missing pixel shader", shname);
        accept = false;
      }
      if (!curpass->vpr.data())
      {
        report_error(parser, terminal, "%s: missing vertex shader", shname);
        accept = false;
      }
      if (accept && curpass->ps30 != curpass->vs30 && hlsls.fields.ps.hasCompilation())
      {
        report_error(parser, terminal, "%s: PS/VS 3.0 mismatch: vs30=%s and ps30=%s", shname, curpass->vs30 ? "yes" : "no",
          curpass->ps30 ? "yes" : "no");
        accept = false;
      }
    }
  }

end:
  if (accept)
  {
    assembly::build_cpp_declarations_for_used_local_vars(ctx);
    auto &cache = ctx.tgtCtx().stcodeCache();
    auto cacheRefs = cache.findOrPost(eastl::move(ctx.stBytecode()));
    curpass->stcode = cacheRefs.stcode;
    curpass->stblkcode = cacheRefs.stblkcode;
    curpass->cppstcode = eastl::move(ctx.cppStcode());
    if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
    {
      curpass->constPackedVarsMaps = eastl::move(varMerger.constVarsMaps);
      curpass->bufferedPackedVarsMap = eastl::move(varMerger.bufferedVarsMap);
      curpass->usedConstStatAstNodes = eastl::move(usedPreshaderStatements);
      curpass->boolAstNodesEvaluationResults = eastl::move(boolElementsEvaluationResults);
    }
  }
  if (accept && no_dynstcode && curpass->stcode.size())
  {
    report_error(parser, no_dynstcode, "%s: has dynstcode, while it was required to have none with no_dynstcode:", shname);
    for (Terminal *var : stcode_vars)
      report_error(parser, var, "'%s'", var->text);
    debug("\n******* State code shader --s--");
    ShUtils::shcod_dump(curpass->stcode, nullptr); // todo: we need variable names!
  }
  curpass = nullptr;
}

bool AssembleShaderEvalCB::validateDynamicConstsForMultidraw()
{
  if (!shConst.multidrawCbuf)
    return true;

  if (hasDynStcodeRelyingOnMaterialParams)
  {
    report_error(parser, exprWithDynamicAndMaterialTerms,
      "Encountered both static and dynamic operands in an stcode expression!\n\n"
      "  With bindless enabled this can break multidraw, as dynstcode is run once per combined draw call.\n"
      "  Consider splitting the expression into separate parts.");
    return false;
  }

  return true;
}

void AssembleShaderEvalCB::end_eval(shader_decl &sh)
{
  compilePreshader();
  code.regsize = ctx.stBytecode().regAllocator->requiredRegCount();

  hlsls.forEach([this](HlslCompile &comp) { evalHlslCompileClass(&comp); });
  end_pass();
}


void AssembleShaderEvalCB::process_shader_locdecl(local_var_decl &s)
{
  auto parseResMaybe = semantic::parse_local_var_decl(s, ctx);
  if (!parseResMaybe)
    return;

  if (!parseResMaybe->var->isConst)
    assembly::assemble_local_var<assembly::StcodeBuildFlagsBits::ALL>(parseResMaybe->var, parseResMaybe->expr.get(), s.name, ctx);
}


void AssembleShaderEvalCB::eval_hlsl_compile(hlsl_compile_class &hlsl_compile)
{
  semantic::HlslCompileClass compile = semantic::parse_hlsl_compilation_info(hlsl_compile, parser, ctx.tgtCtx().compCtx());

  HlslCompile &hlslXS = hlsls.all[compile.stage];

  if (auto *def = compile.tryGetDefaultTarget())
  {
    hlslXS.defaultTarget = eastl::move(def->target);
    return;
  }

  if (hlslXS.hasCompilation())
  {
    report_error(parser, hlsl_compile.profile, "duplicate %s shader", HLSL_ALL_NAMES[compile.stage]);
    return;
  }

  hlslXS.symbol = &hlsl_compile;

  if (auto *compDirective = compile.tryGetCompile())
  {
    if (compDirective->useDefaultProfileIfExists && !hlslXS.defaultTarget.empty())
      compDirective->profile = hlslXS.defaultTarget;

    switch (compile.stage)
    {
      case HLSL_PS:
      case HLSL_CS: curpass->enableFp16.psOrCs = compDirective->useHalfs; break;
      case HLSL_VS:
      case HLSL_MS: curpass->enableFp16.vsOrMs = compDirective->useHalfs; break;
      case HLSL_HS: curpass->enableFp16.hs = compDirective->useHalfs; break;
      case HLSL_DS: curpass->enableFp16.ds = compDirective->useHalfs; break;
      case HLSL_GS:
      case HLSL_AS: curpass->enableFp16.gsOrAs = compDirective->useHalfs; break;
    }

#if !_CROSS_TARGET_C1 && !_CROSS_TARGET_C2 && !_CROSS_TARGET_DX11 && !_CROSS_TARGET_EMPTY
    if (strcmp(compDirective->profile, "ps_3_0") == 0)
      curpass->ps30 = true;
    else if (strcmp(compDirective->profile, "vs_3_0") == 0)
      curpass->vs30 = true;
#endif

    hlslXS.compile.emplace(eastl::move(*compDirective));
  }
}


void AssembleShaderEvalCB::eval_hlsl_decl(hlsl_local_decl_class &sh)
{
  if (sh.ident)
    addBlockType(sh.ident->text, sh.ident);
}

void AssembleShaderEvalCB::addBlockType(const char *name, const Terminal *t)
{
  if (!name)
    return;

  BlockPipelineType type = BLOCK_MAX;
  // From earlier evalution we know that name is in {cs, ps, vs, hs, ds, gs, ms, as}
  if (strstr("cs", name) != 0)
    type = BLOCK_COMPUTE;
  else if (strstr("ps", name) != 0)
    type = BLOCK_GRAPHICS_PS;
  else if (strstr("ms/as", name) != 0)
    type = BLOCK_GRAPHICS_MESH;
  else if (strstr("vs/hs/ds/gs", name) != 0)
    type = BLOCK_GRAPHICS_VERTEX;
  else
  {
    report_error(parser, t, "Unknown block type (%s)", name);
    return;
  }

  declaredBlockTypes[type] = true;

  // Check for conflicting block types
  if (declaredBlockTypes[BLOCK_COMPUTE] && hasDeclaredGraphicsBlocks())
    report_error(parser, t, "It is illegal to declare both (cs) and (ps/vs/hs/ds/gs/ms/as) blocks in one shader");
  if (declaredBlockTypes[BLOCK_GRAPHICS_VERTEX] && declaredBlockTypes[BLOCK_GRAPHICS_MESH])
    report_error(parser, t, "It is illegal to declare both (vs/hs/ds/gs) and (ms/as) blocks in one shader");
}

bool AssembleShaderEvalCB::hasDeclaredGraphicsBlocks()
{
  return declaredBlockTypes[BLOCK_GRAPHICS_PS] || declaredBlockTypes[BLOCK_GRAPHICS_VERTEX] || declaredBlockTypes[BLOCK_GRAPHICS_MESH];
}

bool AssembleShaderEvalCB::hasDeclaredMeshPipelineBlocks() { return declaredBlockTypes[BLOCK_GRAPHICS_MESH]; }


void AssembleShaderEvalCB::evalHlslCompileClass(HlslCompile *comp)
{
  if (!comp->hasCompilation())
    return;

  bool isCompute = comp == &hlsls.fields.cs;
  if (isCompute && hasDeclaredGraphicsBlocks())
  {
    report_error(parser, comp->symbol, "Can not compile %s shader with (ps/vs/hs/ds/gs/ms/as) blocks declared", comp->stageName);
    return;
  }
  else if (!isCompute)
  {
    if (declaredBlockTypes[BLOCK_COMPUTE])
    {
      report_error(parser, comp->symbol, "Can not compile %s shader with (cs) blocks declared", comp->stageName);
      return;
    }

    bool isMesh = comp == &hlsls.fields.ms || comp == &hlsls.fields.as;
    bool isVertex = !isMesh && comp != &hlsls.fields.ps;
    if (isMesh && declaredBlockTypes[BLOCK_GRAPHICS_VERTEX])
    {
      report_error(parser, comp->symbol, "Can not compile %s shader with (vs/hs/ds/gs) blocks declared", comp->stageName);
      return;
    }
    else if (isVertex && declaredBlockTypes[BLOCK_GRAPHICS_MESH])
    {
      report_error(parser, comp->symbol, "Can not compile %s shader with (ms/as) blocks declared", comp->stageName);
      return;
    }
  }
}

void AssembleShaderEvalCB::reserveSpecialCbufferAt(HlslSlotSemantic cbuffer_sem, int reg)
{
  G_ASSERT(cbuffer_sem >= HlslSlotSemantic::RESERVED_FOR_IMPLICIT_CONST_CBUF);
  auto vr = shConst.vertexRegAllocators[HLSL_RSPACE_B].reserve(cbuffer_sem, reg);
  auto pcr = shConst.pixelOrComputeRegAllocators[HLSL_RSPACE_B].reserve(cbuffer_sem, reg);
  if (!vr)
  {
    G_ASSERT(!vr.error().outOfRange);
    report_reg_reserve_failed(nullptr, reg, 1, HLSL_RSPACE_B, cbuffer_sem, vr.error(), shConst.vertexRegAllocators[HLSL_RSPACE_B],
      shConst.makeInfoProvider(&NamedConstBlock::vertexProps, HLSL_RSPACE_B));
  }
  if (!pcr)
  {
    G_ASSERT(!pcr.error().outOfRange);
    report_reg_reserve_failed(nullptr, reg, 1, HLSL_RSPACE_B, cbuffer_sem, pcr.error(),
      shConst.pixelOrComputeRegAllocators[HLSL_RSPACE_B], shConst.makeInfoProvider(&NamedConstBlock::pixelProps, HLSL_RSPACE_B));
  }
}

class CompileShaderJob : public shc::Job
{
public:
  CompileShaderJob(AssembleShaderEvalCB *ascb, AssembleShaderEvalCB::HlslCompile &hlsl, HlslCompilationStage stage,
    const hlsl_compile_class *compile_symbol, dag::ConstSpan<CodeSourceBlocks::Unconditional *> code_blocks, uint64_t variant_hash) :
    ctx{ascb->ctx.shCtx()}, dynVariant{ascb->ctx.variant().dyn}, stage{stage}
  {
    G_ASSERT(is_valid(stage));

    static const char *def_cg_args[] = {"--assembly", "--mcgb", "--fenable-bx2", "--nobcolor", "--fastmath", NULL};

    entry = hlsl.compile->entry;
    profile = hlsl.compile->profile;
    hlsl_compile_token = compile_symbol->profile;
    enableFp16 = hlsl.compile->useHalfs;
    curpass = ascb->curpass;
    lexer = &ascb->parser.get_lexer();
    shaderName = ctx.name();
    cgArgs = shc::config().hlslNoDisassembly ? def_cg_args + 1 : def_cg_args;

    CodeSourceBlocks &code = *getSourceBlocks(profile);
    dag::ConstSpan<char> main_src = code.buildSourceCode(code_blocks);
    source.setStr(main_src.data(), main_src.size());

    eastl::string src_predefines = assembly::build_hardware_defines_hlsl(profile.c_str(), enableFp16, ascb->ctx.tgtCtx().compCtx());
#if _CROSS_TARGET_C2








#endif

    ascb->shConst.patchHlsl(source, HLSL_STAGE_TO_SHADER_STAGE[stage], ascb->varMerger, max_constants_no,
      eastl::string_view{src_predefines}, curpass->dual_source_blending);
    int base = append_items(source, 16);
    memset(&source[base], 0, 16);

    if (shc::config().hlslDumpCodeAlways || shc::config().validateIdenticalBytecode || debug_output_dir_shader_name)
      compileCtx = sh_get_compile_context();

    shader_variant_hash = variant_hash;
  }

protected:
  void doJobBody() override;
  void releaseJobBody() override;
  eastl::optional<dxil::StreamOutputComponentInfo> parsePragmaStreamOutput(const char *pragma);
  bool skipEmptyPragmaSpaces(const char *&pragma);

  void addResults();

  // @TODO: this is not a good solution (as is keeping other mutable refs in this job's fields) -- we have to make sure this is only
  // used in the constructor & addResults. Also, currently jobs outlive VariantContext, which also should be fixed, for now we keep
  // a pointer to VariantSrc.
  shc::ShaderContext &ctx;
  const ShaderVariant::VariantSrc *dynVariant;

  HlslCompilationStage stage = HLSL_INVALID;

  SemanticShaderPass *curpass;
  SimpleString entry, profile;
  String source;
  String compileCtx;
  const char **cgArgs;
  CompileResult compile_result;
  int max_constants_no;
  bool enableFp16;

  const char *shaderName;
  const SHTOK_string *hlsl_compile_token;
  Lexer *lexer;

  uint64_t shader_variant_hash;
};

void AssembleShaderEvalCB::hlsl_compile(HlslCompilationStage stage)
{
  auto &hlsl = hlsls.all[stage];
  if (!hlsl.compile)
    return;

  G_ASSERT(hlsl.hasCompilation());
  semantic::HlslCompileDirective &compile = hlsl.compile.value();

  CodeSourceBlocks *codeP = getSourceBlocks(compile.profile);
  G_ASSERT(codeP && "ShaderParser::curXsCode must be non null here by design");
  CodeSourceBlocks &code = *codeP;

  ShaderBytecodeCache &cache = ctx.tgtCtx().bytecodeCache();

  dag::ConstSpan<CodeSourceBlocks::Unconditional *> code_blocks = code.getPreprocessedCode(*this);
  CryptoHash code_digest = code.getCodeDigest(code_blocks);
  CryptoHash const_digest = shConst.getDigest(HLSL_STAGE_TO_SHADER_STAGE[stage], varMerger);

  ShaderCompilerStat::hlslCompileCount++;
  auto [cacheIdx, entryIds] = cache.find(code_digest, const_digest, compile.entry, compile.profile);

  curpass->setCidx(stage, entryIds);
  if (cacheIdx != ShaderCacheIndex::EMPTY)
  {
    // Reuse cached shader.
    ShaderCompilerStat::hlslCacheHitCount++;

    if (cacheIdx == ShaderCacheIndex::PENDING) // not ready yet, add to pending list to update later
      cache.registerPendingPass(*curpass);
    else
      apply_shader_from_cache(*curpass, stage, entryIds, cache);
  }
  else
  {
    cache.markEntryAsPending(entryIds);

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

    CompileShaderJob *job = new CompileShaderJob{this, hlsl, stage, hlsl.symbol, code_blocks, shader_variant_hash};
    shc::add_job(job, shc::JobMgrChoiceStrategy::ROUND_ROBIN);
  }
}

#include "hashed_cache.h"
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_logSys.h>
#include "sha1_cache_version.h"

// @TODO: refactor hlsl stage handling in compilation jobs/caching code, and move it to a separate module.

static const char *find_line_comment(const char *s1, const char *s1_end)
{
  const char *p = (const char *)memchr(s1, '/', s1_end - s1);
  while (p && p + 1 < s1_end)
  {
    if (p[1] == '/')
      return p;
    p = (const char *)memchr(p + 2, '/', s1_end - p - 2);
  }
  return nullptr;
}
static const char *get_next_line_trail_to_strip(const char *c_start, bool hold_file_line)
{
  const char *next_line = strstr(c_start, "\n#line ");
  const char *next_line2 = next_line ? find_line_comment(c_start, next_line) : strstr(c_start, "//");
  if (next_line2 && (!next_line || next_line2 < next_line))
  {
    while (next_line2 > c_start && (*(next_line2 - 1) == ' ' || *(next_line2 - 1) == '\t'))
      next_line2--;
    if (next_line2 > c_start && *(next_line2 - 1) == '\n')
      next_line2--;
    return next_line2;
  }
  if (hold_file_line && next_line && strncmp(next_line, "\n#line 1 \"precompiled\"", 22) != 0)
    next_line = strchr(next_line + 7, ' ');
  return next_line;
}
static inline void calc_sha1_stripped(HASH_CONTEXT &sha1, const char *src, size_t total_len, bool hold_file_line,
  String *out_stripped = nullptr)
{
  const char *c_start = src, *next_line = src;
  if (out_stripped)
    out_stripped->clear();
  while (c_start && (next_line = get_next_line_trail_to_strip(c_start, hold_file_line)))
  {
    HASH_UPDATE(&sha1, (const unsigned char *)c_start, (uint32_t)(next_line - c_start));
    if (out_stripped)
      out_stripped->append(c_start, (uint32_t)(next_line - c_start));
    c_start = strstr(next_line + 1, "\n");
  };
  size_t at = (c_start - src);
  if (c_start != nullptr && at < total_len)
  {
    HASH_UPDATE(&sha1, (const unsigned char *)c_start, (uint32_t)(size_t(total_len) - at));
    if (out_stripped)
      out_stripped->append(c_start, (uint32_t)(size_t(total_len) - at));
  }
}
static String make_dump_filepath(const char *shaderName, uint64_t shader_variant_hash, const char *shader_sha1)
{
  String filepath;
  if (const char *log_fn = get_log_filename())
  {
    if (const char *ext = dd_get_fname_ext(log_fn))
      filepath.printf(0, "%.*s/", ext - log_fn, log_fn);
    else if (const char *p = dd_get_fname(log_fn))
      filepath.printf(0, "%.*s./dumps/", p - log_fn, log_fn);
  }
  filepath.aprintf(0, "%s_%08x_%s", shaderName, shader_variant_hash, shader_sha1);
  return filepath;
}
static void dump_hlsl_src(const char *compileCtx, const char *source, const eastl::vector<uint8_t> &bytecode, //
  const char *sha1SrcPath, bool reused, const char *shaderName, uint64_t shader_variant_hash)
{
  HASH_CONTEXT sha1;
  unsigned char shader_sha1[HASH_SIZE];
  String shader_sha1_s, status;
  HASH_INIT(&sha1);
  HASH_UPDATE(&sha1, (const unsigned char *)bytecode.data(), bytecode.size());
  HASH_FINISH(&sha1, shader_sha1);
  data_to_str_hex(shader_sha1_s, shader_sha1, sizeof(shader_sha1));
  if (bytecode.empty())
    status = "failed to compile";
  else if (reused)
    status.printf(0, "reused built %s from (%s)", shader_sha1_s, sha1SrcPath);
  else if (sha1SrcPath && *sha1SrcPath)
    status.printf(0, "built %s and stored as (%s)", shader_sha1_s, sha1SrcPath);
  else
    status.printf(0, "built %s", shader_sha1_s);

  if (shc::config().hlslDumpCodeSeparate)
  {
    String fn = make_dump_filepath(shaderName, shader_variant_hash, shader_sha1_s);
    dd_mkpath(fn);
    FullFileSaveCB cwr(fn);
    if (cwr.fileHandle)
    {
      const char *prolog_str = "=== compiling code:\n";
      const char *epilog_str = "\n==> ";
      cwr.write(prolog_str, strlen(prolog_str));
      cwr.write(compileCtx, strlen(compileCtx));
      cwr.write("\n", 1);
      cwr.write(source, strlen(source));
      cwr.write(epilog_str, strlen(epilog_str));
      cwr.write(status, strlen(status));
      cwr.write("\n", 1);
      // String src_stripped;
      // calc_sha1_stripped(sha1, source, strlen(source), false, &src_stripped);
      // cwr.write(src_stripped, strlen(src_stripped));
      debug("=== written dump to:%s", fn);
    }
    else
      logwarn("=== cannot write dump to:%s", fn);
    if (!bytecode.empty()) // when successfl build we don't need to duplicate dump to log
      return;
  }

  debug("=== compiling code:\n%s\n%s\n%s", compileCtx, source, status);
}

void CompileShaderJob::doJobBody()
{
  // @TODO: it should be const in fields!
  const shc::ShaderContext &sctx = ctx;

  char sha1SrcPath[420];
  sha1SrcPath[0] = 0;
  unsigned char srcSha1[HASH_SIZE];

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

  const unsigned sourceLen = (unsigned)strlen(source);
  if (shc::config().useSha1Cache)
  {
    const bool enableBindlessVar = shc::config().enableBindless;
    const bool isDebugModeEnabledVar = sctx.isDebugModeEnabled();
    const int hlslOptimizationLevelVar = shc::config().hlslOptimizationLevel;
    const bool hlsl2021Var = shc::config().hlsl2021;

    HASH_CONTEXT sha1;
    HASH_INIT(&sha1);
    HASH_UPDATE(&sha1, (const unsigned char *)&sha1_cache_version, sizeof(sha1_cache_version));
    // HASH_UPDATE( &sha1, (const unsigned char*)source.c_str(), (uint32_t)sourceLen );
    calc_sha1_stripped(sha1, source.c_str(), (uint32_t)sourceLen, isDebugModeEnabledVar);
    HASH_UPDATE(&sha1, (const unsigned char *)profile.c_str(), (uint32_t)strlen(profile));
    HASH_UPDATE(&sha1, (const unsigned char *)entry.c_str(), (uint32_t)strlen(entry));
    // optimization level is a part of output dir, but still
    HASH_UPDATE(&sha1, (const unsigned char *)&hlslOptimizationLevelVar, (uint32_t)sizeof(hlslOptimizationLevelVar));
    HASH_UPDATE(&sha1, (const unsigned char *)&hlsl2021Var, (uint32_t)sizeof(hlsl2021Var));
    HASH_UPDATE(&sha1, (const unsigned char *)&enableFp16, (uint32_t)sizeof(enableFp16));
    HASH_UPDATE(&sha1, (const unsigned char *)&enableBindlessVar, (uint32_t)sizeof(enableBindlessVar));
#if _CROSS_TARGET_SPIRV
    auto mode =
      shc::config().compilerDXC ? CompilerMode::DXC : (shc::config().compilerHlslCc ? CompilerMode::HLSLCC : CompilerMode::DEFAULT);
    HASH_UPDATE(&sha1, (const unsigned char *)&mode, (uint32_t)sizeof(mode));
#elif _CROSS_TARGET_DX12
    const auto platform = shc::config().targetPlatform;
    HASH_UPDATE(&sha1, (const unsigned char *)&platform, (uint32_t)sizeof(platform));
    HASH_UPDATE(&sha1, (const unsigned char *)&useScarlettWave32, (uint32_t)sizeof(useScarlettWave32));
#endif
#if _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
    const char *dxcVer = spirv::getDXCVerString(shc::config().dxcContext);
    HASH_UPDATE(&sha1, (const unsigned char *)dxcVer, (uint32_t)strlen(dxcVer));
    for (const String &i : shc::config().dxcParams)
      HASH_UPDATE(&sha1, (const unsigned char *)i.c_str(), (uint32_t)i.size());
#endif

    if (isDebugModeEnabledVar)
      HASH_UPDATE(&sha1, (const unsigned char *)&isDebugModeEnabledVar, (uint32_t)sizeof(isDebugModeEnabledVar));

    HASH_FINISH(&sha1, srcSha1);

    SNPRINTF(sha1SrcPath, sizeof(sha1SrcPath), "%s/src/%s/" HASH_LIST_STRING, shc::config().sha1CacheDir,
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
        SNPRINTF(sha1BinPath, sizeof(sha1BinPath), "%s/bin/%s/%s", shc::config().sha1CacheDir, profile.c_str(), binSha1Read);
        df_close(sha1LinkFile);
        file_ptr_t sha1BinFile = df_open(sha1BinPath, DF_READ);
        if (!sha1BinFile)
        {
          debug("Sha1 link %s in %s is invalid.", sha1BinPath, sha1SrcPath);
        }
        else
        {
          int sha1FileLen;
          const void *content = df_mmap(sha1BinFile, &sha1FileLen);
          if (!sha1FileLen)
          {
            debug("link is broken?");
            df_close(sha1BinFile);
          }
          else
          {
            const uint8_t *end_bytecode = (const uint8_t *)content + sha1FileLen - sizeof(ComputeShaderInfo);
            compile_result.bytecode.assign((const uint8_t *)content, end_bytecode);
            memcpy(&compile_result.computeShaderInfo, end_bytecode, sizeof(ComputeShaderInfo));
            df_unmap(content, sha1FileLen);
            df_close(sha1BinFile);
            if (shc::config().hlslDumpCodeAlways)
              dump_hlsl_src(compileCtx, source, compile_result.bytecode, sha1SrcPath, true, shaderName, shader_variant_hash);
            ShaderCompilerStat::hlslExternalCacheHitCount.fetch_add(1);
            return;
          }
        }
      }
      else
      {
        debug("Sha1 link in %s is missing.", sha1SrcPath);
        df_close(sha1LinkFile);
      }
    }
  }

  auto localHlslOptimizationLevel =
    (shc::config().hlslDebugLevel == DebugLevel::FULL_DEBUG_INFO) ? 0 : shc::config().hlslOptimizationLevel;
  bool optimizationLevelHasBeenOverriden = false;
  auto lastOptimizationLevel = localHlslOptimizationLevel;
  bool full_debug = false, forceDisableWarnings = false;
  bool embed_source = shc::config().hlslEmbedSource;
  bool useWave32 = useScarlettWave32;
  int waveSpecification = 0;
  bool useHlsl2021 = shc::config().hlsl2021;
  bool usePsInvariantPos = false;
#if _CROSS_TARGET_DX12
  dag::Vector<dxil::StreamOutputComponentInfo> streamOutputComponents;
#endif
  for (const char *pragma = strstr(source, "#pragma "); pragma; pragma = strstr(pragma, "#pragma "))
  {
    void *original = (void *)pragma;
    pragma += strlen("#pragma ");
#define PRAGMA(b) (strncmp(pragma, b, strlen(b)) == 0)
    if (PRAGMA("debugfull") || PRAGMA("dfull"))
    {
      full_debug = true;
      localHlslOptimizationLevel = 0;
    }
    else if (PRAGMA("embed_source"))
    {
      embed_source = true;
    }
    else if (PRAGMA("force_disable_warnings"))
    {
      forceDisableWarnings = true;
    }
    else if (PRAGMA("hlsl2021"))
    {
      useHlsl2021 = true;
    }
    else if (PRAGMA("force_min_opt_level "))
    {
      pragma += strlen("force_min_opt_level ");
      localHlslOptimizationLevel = max(atoi(pragma), shc::config().hlslOptimizationLevel);
      if (eastl::exchange(optimizationLevelHasBeenOverriden, true) && localHlslOptimizationLevel != lastOptimizationLevel)
      {
        compile_result.errors.append_sprintf("#pragma force_min_opt_level redefined with value %d (previous=%d)",
          lastOptimizationLevel, localHlslOptimizationLevel);
      }
      lastOptimizationLevel = localHlslOptimizationLevel;
    }
#if _CROSS_TARGET_C1 || _CROSS_TARGET_C2




#endif
    else if (PRAGMA("wave"))
    {
      pragma += strlen("wave");
      if (strncmp(pragma, "32", 2) == 0)
      {
        if (waveSpecification == 64)
        {
          compile_result.errors = "wave64 was already specified, now seeing wave32";
          debug("Hlsl compilation failed, exiting job...");
          return;
        }
        useWave32 = true;
        waveSpecification = 32;
      }
      else if (strncmp(pragma, "64", 2) == 0)
      {
        if (waveSpecification == 32)
        {
          compile_result.errors = "wave32 was already specified, now seeing wave64";
          debug("Hlsl compilation failed, exiting job...");
          return;
        }
        useWave32 = false;
        waveSpecification = 64;
      }
      else if (strncmp(pragma, "def", 3) == 0)
      {
        int waves = useScarlettWave32 ? 32 : 64;
        if (waveSpecification == 32 && waves != 32)
        {
          compile_result.errors = "wave32 was already specified, now seeing wave64 as def";
          debug("Hlsl compilation failed, exiting job...");
          return;
        }
        if (waveSpecification == 64 && waves != 64)
        {
          compile_result.errors = "wave64 was already specified, now seeing wave32 as def";
          debug("Hlsl compilation failed, exiting job...");
          return;
        }
        useWave32 = useScarlettWave32;
        waveSpecification = waves;
      }
    }
    else if (PRAGMA("stream_output") && isspace(*(pragma + LITSTR_LEN("stream_output"))))
    {
      // Stream output is implemented only for DX12 now. This pragma is ignored for other targets.
#if _CROSS_TARGET_DX12
      const auto streamOutputComponent = parsePragmaStreamOutput(pragma);
      if (!streamOutputComponent)
        return; // Compilation failed due to invalid stream output declaration
      streamOutputComponents.push_back(*streamOutputComponent);
#endif
    }
    else
      continue;
    // replace #pragma with comment
    memcpy(original, "//     ", LITSTR_LEN("#pragma"));
  }

#if _CROSS_TARGET_METAL || _CROSS_TARGET_SPIRV
  if (debug_output_dir_shader_name)
    spitfile(shaderName, entry, "intervals", shader_variant_hash, compileCtx.data(), data_size(compileCtx),
      debug_output_dir_shader_name);
#endif

    //  Compile.
#if _CROSS_TARGET_EMPTY
  compile_result.bytecode.resize(sizeof(uint64_t));
#elif _CROSS_TARGET_C1



#elif _CROSS_TARGET_C2



#elif _CROSS_TARGET_METAL
  if (shc::config().dxcContext)
  {
    compile_result = compileShaderMetal(shc::config().dxcContext, source, profile, entry, !shc::config().hlslNoDisassembly,
      useHlsl2021, enableFp16, shc::config().hlslSkipValidation, localHlslOptimizationLevel ? true : false, max_constants_no,
      shaderName, shc::config().useIosToken, shc::config().useBinaryMsl, shader_variant_hash, shc::config().enableBindless);
  }
  else
  {
    compile_result.bytecode.resize(sizeof(uint64_t));
  }
#elif _CROSS_TARGET_SPIRV
  if (shc::config().dxcContext)
  {
    compile_result =
      compileShaderSpirV(shc::config().dxcContext, source, profile, entry, !shc::config().hlslNoDisassembly, useHlsl2021, enableFp16,
        shc::config().hlslSkipValidation, localHlslOptimizationLevel ? true : false, max_constants_no, shaderName,
        shc::config().compilerDXC      ? CompilerMode::DXC
        : shc::config().compilerHlslCc ? CompilerMode::HLSLCC
                                       : CompilerMode::DEFAULT,
        shader_variant_hash, shc::config().enableBindless, full_debug || shc::config().hlslDebugLevel != DebugLevel::NONE,
        shc::config().dumpSpirvOnly);
  }
  else
  {
    compile_result.bytecode.resize(sizeof(uint64_t));
  }
#elif _CROSS_TARGET_DX12
  // NOTE: when we support this kind of switch somehow this can be replaced with actual information
  // of use or not
  compile_result = dx12::dxil::compileShader(make_span_const(source).first(sourceLen), // source length includes trailing zeros
    profile, entry, !shc::config().hlslNoDisassembly, useHlsl2021, enableFp16, shc::config().hlslSkipValidation,
    localHlslOptimizationLevel ? true : false, is_hlsl_debug(), shc::config().dx12PdbCacheDir, max_constants_no, shaderName,
    shc::config().targetPlatform, useWave32, !forceDisableWarnings && shc::config().hlslWarningsAsErrors,
    full_debug ? DebugLevel::FULL_DEBUG_INFO : shc::config().hlslDebugLevel, embed_source, streamOutputComponents);
#else //_CROSS_TARGET_DX11
  unsigned int flags = is_hlsl_debug() ? D3DCOMPILE_DEBUG : 0;
  flags |= shc::config().hlslSkipValidation ? D3DCOMPILE_SKIP_VALIDATION : 0;
  flags |= is_hlsl_debug()
             ? D3DCOMPILE_SKIP_OPTIMIZATION
             : (localHlslOptimizationLevel >= 3
                   ? D3DCOMPILE_OPTIMIZATION_LEVEL3
                   : (localHlslOptimizationLevel >= 2
                         ? D3DCOMPILE_OPTIMIZATION_LEVEL2
                         : (localHlslOptimizationLevel >= 1 ? D3DCOMPILE_OPTIMIZATION_LEVEL0 : D3DCOMPILE_SKIP_OPTIMIZATION)));
  flags |= shc::config().hlslWarningsAsErrors ? D3DCOMPILE_WARNINGS_ARE_ERRORS : 0;
  compile_result = compileShaderDX11(shaderName, source, NULL, profile, entry, !shc::config().hlslNoDisassembly,
    full_debug ? DebugLevel::FULL_DEBUG_INFO : shc::config().hlslDebugLevel, shc::config().hlslSkipValidation, embed_source, flags,
    max_constants_no);
#endif

  if (shc::config().hlslDumpCodeAlways)
    dump_hlsl_src(compileCtx, source, compile_result.bytecode, sha1SrcPath, false, shaderName, shader_variant_hash);

  if (compile_result.bytecode.empty())
  {
    debug("Hlsl compilation failed, exiting job...");
    return;
  }

  if (shc::config().writeSha1Cache && shc::config().useSha1Cache && !compile_result.bytecode.empty() && dd_mkpath(sha1SrcPath))
  {
    unsigned char binSha1[HASH_SIZE];
    HASH_CONTEXT sha1;
    HASH_INIT(&sha1);
    HASH_UPDATE(&sha1, (const unsigned char *)compile_result.bytecode.data(), compile_result.bytecode.size());
    HASH_UPDATE(&sha1, (const unsigned char *)&compile_result.computeShaderInfo, sizeof(compile_result.computeShaderInfo));
    HASH_FINISH(&sha1, binSha1);
    char sha1BinPath[420];
    sha1BinPath[0] = 0;
    SNPRINTF(sha1BinPath, sizeof(sha1BinPath), "%s/bin/%s/" HASH_LIST_STRING, shc::config().sha1CacheDir,
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
        shc::config().sha1CacheDir, profile.c_str(), HASH_LIST(binSha1));
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
      shc::config().sha1CacheDir, profile.c_str(), HASH_LIST(srcSha1));
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

void CompileShaderJob::releaseJobBody()
{
  DEFER([this] { delete this; });

  sh_set_current_dyn_variant(dynVariant);
  DEFER([] { sh_set_current_dyn_variant(nullptr); });

  if (!compile_result.logs.empty())
    sh_debug(SHLOG_INFO, "Compilation log:\n%s", compile_result.logs.c_str());

  const bool failed = compile_result.bytecode.empty() || (shc::config().hlslWarningsAsErrors && !compile_result.errors.empty());

  if (failed)
  {
    // Display error message.
    lexer->set_error(hlsl_compile_token->file_start, hlsl_compile_token->line_start, hlsl_compile_token->col_start,
      String(256, "compile(%s, %s) failed:", profile.str(), entry.str()));

    if (!compile_result.errors.empty())
    {
      // debug("errors compiling HLSL code:\n%s", source.str());
      const char *es = (const char *)compile_result.errors.c_str(), *ep = strchr(es, '\n');
      while (es)
        if (ep)
        {
          auto profile_to_assert_stage = [](const SimpleString &profile) {
            if (strstr(profile, "vs"))
              return "ps";
            return profile.c_str();
          };
          sh_debug(SHLOG_NORMAL, "  %.*s", ep - es, es);
          size_t link_error = eastl::string_view(es, ep - es).find("External function used in non-library profile");
          if (link_error != eastl::string::npos && eastl::string_view(es, ep - es).find("_assert", link_error) != eastl::string::npos)
          {
            sh_debug(SHLOG_NORMAL,
              "\nNote: the shader uses assert, but it was not enabled.\n"
              "Tip: add a macro `ENABLE_ASSERT(%.*s)` to the shader",
              2, profile_to_assert_stage(profile));
            if (strstr(profile, "vs"))
              sh_debug(SHLOG_NORMAL,
                "Tip 2: ENABLE_ASSERT(vs) is not allowed. Rather, ENABLE_ASSERT(ps) enables asserts for both ps and vs");
          }
          es = ep + 1;
          ep = strchr(es, '\n');
        }
        else
          break;
      sh_debug(SHLOG_NORMAL, "  %s\n", es);
    }
    else
      sh_debug(SHLOG_NORMAL, "  D3DXCompileShader error");

    if (!shc::config().hlslDumpCodeAlways && shc::config().hlslDumpCodeOnError)
      debug("=== compiling code:\n%s==== code end", source.str());

    return;
  }

  if (!compile_result.errors.empty() && shc::config().hlslShowWarnings)
  {
    lexer->set_warning(hlsl_compile_token->file_start, hlsl_compile_token->line_start, hlsl_compile_token->col_start,
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

  addResults();
}

eastl::optional<dxil::StreamOutputComponentInfo> CompileShaderJob::parsePragmaStreamOutput(const char *pragma)
{
  dxil::StreamOutputComponentInfo result = {};
  pragma += LITSTR_LEN("stream_output");

  if (!skipEmptyPragmaSpaces(pragma))
  {
    compile_result.errors.sprintf(
      "Pragma stream_output parsing failed (space after `stream_output` is not found), symbol: %c (0x%02X)", *pragma, *pragma);
    return eastl::nullopt;
  }

  if (isdigit(*pragma))
  {
    result.slot = *pragma - '0';
    pragma++;
  }
  else
  {
    compile_result.errors.sprintf("Pragma stream_output parsing failed (slot is not found), symbol: %c (0x%02X)", *pragma, *pragma);
    return eastl::nullopt;
  }

  if (!skipEmptyPragmaSpaces(pragma))
  {
    compile_result.errors.sprintf("Pragma stream_output parsing failed (space after slot is not found), symbol: %c (0x%02X)", *pragma,
      *pragma);
    return eastl::nullopt;
  }

  uint32_t idx = 0;
  while ((isalpha(*pragma) || *pragma == '_') && idx < dxil::MAX_SEMANTIC_NAME_SIZE - 1)
  {
    result.semanticName[idx++] = *pragma;
    pragma++;
  }
  if (idx == 0)
  {
    compile_result.errors.sprintf("Pragma stream_output parsing failed (semantic name is empty), symbol: %c (0x%02X)", *pragma,
      *pragma);
    return eastl::nullopt;
  }
  result.semanticName[idx] = '\0';
  if (isdigit(*pragma))
  {
    result.semanticIndex = atoi(pragma);
    while (isdigit(*pragma))
      pragma++;
  }

  if (!skipEmptyPragmaSpaces(pragma))
  {
    compile_result.errors.sprintf("Pragma stream_output parsing failed (space after semantic is not found), symbol: %c (0x%02X)",
      *pragma, *pragma);
    return eastl::nullopt;
  }

  result.mask = 0;
  if (*pragma == 'X')
  {
    result.mask |= 0x1;
    pragma++;
  }
  if (*pragma == 'Y')
  {
    result.mask |= 0x2;
    pragma++;
  }
  if (*pragma == 'Z')
  {
    result.mask |= 0x4;
    pragma++;
  }
  if (*pragma == 'W')
  {
    result.mask |= 0x8;
    pragma++;
  }
  if (result.mask == 0)
  {
    compile_result.errors.sprintf("Pragma stream_output parsing failed (mask is not found), symbol: %c (0x%02X)", *pragma, *pragma);
    return eastl::nullopt;
  }

  while (isspace(*pragma) && *pragma != '\n')
    pragma++;
  if (*pragma != '\n')
  {
    compile_result.errors.sprintf("Pragma stream_output parsing failed (end of line is not found), symbol: %c (0x%02X)", *pragma,
      *pragma);
    return eastl::nullopt;
  }

  return result;
}

bool CompileShaderJob::skipEmptyPragmaSpaces(const char *&pragma)
{
  if (!isspace(*pragma))
    return false;
  while (isspace(*pragma) && *pragma != '\n')
    pragma++;
  return true;
}

void CompileShaderJob::addResults()
{
  auto entryIdsMaybe = curpass->getCidx(stage, true);
  G_VERIFY(entryIdsMaybe.has_value());
  ShaderCacheLevelIds entryIds = entryIdsMaybe.value();

  ShaderBytecodeCache &cache = ctx.tgtCtx().bytecodeCache();

  // Fill cache entry
  bool added_new = cache.post(entryIds, compile_result, stage, compileCtx);

  if (!added_new)
  {
    ShaderCompilerStat::hlslEqResultCount++;
    if (shc::config().validateIdenticalBytecode)
    {
      const auto &cachedShader = cache.resolveEntry(entryIds);
      sh_debug(SHLOG_INFO, "Equal shaders for profile %s were found:\n\nThe first one:\n%s\nThe second one:\n%s\n", profile,
        cachedShader.compileCtx, compileCtx);
    }
  }

  apply_shader_from_cache(*curpass, stage, entryIds, cache);

  // Disassembly for debug.
  if (!shc::config().hlslNoDisassembly && !compile_result.bytecode.empty())
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

void ShaderParser::clear_per_file_caches() { CodeSourceBlocks::resetCompilation(); }

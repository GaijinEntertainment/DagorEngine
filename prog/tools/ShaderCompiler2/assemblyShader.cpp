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
  parser{ctx.tgtCtx().sourceParseState().parser},
  exprParser{ctx},
  allRefStaticVars{ctx.shCtx().typeTables().referencedTypes},
  dont_render(false),
  variant(ctx.variant())
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

  uint32_t flags = 0;
  uint32_t stubCol = 0;

  for (auto *attrib : s.attrib)
  {
    if (streq(attrib->name->text, "stub"))
    {
      if (code.vars[v].dynamic)
      {
        report_error(parser, attrib->name, "'%s' attribute is not supported for dynamic vars", attrib->name->text);
        return;
      }
      if (code.vars[v].type != SHVT_TEXTURE)
      {
        report_error(parser, attrib->name, "'%s' attribute is only supported for texture vars", attrib->name->text);
        return;
      }
      if (attrib->value)
      {
        report_error(parser, attrib->name, "'%s' attribute takes a color expression", attrib->name->text);
        return;
      }

      Color4 val{};
      if (!exprParser.parseConstExpression(*attrib->expr, val, ExpressionParser::Context{shexpr::VT_COLOR4, false, attrib->name}))
      {
        report_error(parser, attrib->name, "Wrong expression for '%s' color", attrib->name->text);
        return;
      }

      val = clamp(val, Color4(0.f, 0.f, 0.f, 0.f), Color4(1.f, 1.f, 1.f, 1.f));
      flags |= ShaderClass::VF_HAS_STUB_COLOR;
      stubCol =
        (uint32_t(255.f * val.r)) | (uint32_t(255.f * val.g) << 8) | (uint32_t(255.f * val.b) << 16) | (uint32_t(255.f * val.a) << 24);
    }
    else
    {
      report_warning(parser, *attrib->name, "Unknown static attribute '%s'", attrib->name->text);
    }
  }

  const bool hasStubColor = flags & ShaderClass::VF_HAS_STUB_COLOR;

  bool varReferenced = true;

  if (!s.mode && !hasStubColor)
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
    sclass.stvar[sv].additionalFlags = flags;
    sclass.stvar[sv].stubColor = stubCol;

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
    if (sclass.stvar[sv].additionalFlags != flags)
    {
      report_error(parser, s.name, "static var '%s' defined with different attributes", s.name->text);
      return;
    }
    if ((flags & ShaderClass::VF_HAS_STUB_COLOR) && (sclass.stvar[sv].stubColor != stubCol))
    {
      report_error(parser, s.name, "static var '%s' defined with different stub colors", s.name->text);
      return;
    }
  }

  if (varReferenced)
  {
    int i = append_items(code.stvarmap, 1);
    code.stvarmap[i].v = v;
    code.stvarmap[i].sv = sv;
  }

  preshaderSource.staticVarDecls.push_back(&s);

  if (s.init && !s.init->expr)
    eval_init_stat(s.name, *s.init, varReferenced);

  if (hasStubColor)
  {
    int opcode = shaderopcode::makeOp2(SHCOD_TEXTURE_STUBCOL, 0, sv);
    for (int i = 0; i < sclass.shInitCode.size(); i += 2)
      if (sclass.shInitCode[i + 1] == opcode)
      {
        if (sclass.shInitCode[i] != stubCol)
          report_error(parser, s.name, "ambiguous stub color for static texture <%s> used in branching", s.name->text);
        return;
      }

    sclass.shInitCode.push_back(stubCol);
    sclass.shInitCode.push_back(opcode);
  }
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

void AssembleShaderEvalCB::eval_init_stat(SHTOK_ident *var, shader_init_value &v, bool is_referenced)
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
    if (is_referenced && !code.vars[vi].dynamic)
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

    int stVarId = is_referenced ? sclass.find_static_var(varNameId) : -1;
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
    preshaderSource.hardcodedStats.emplace_back(stat);
  else if (semantic::vt_is_numeric(vt))
    preshaderSource.scalarStats.emplace_back(stat);
  else if (semantic::vt_is_static_texture(vt))
    preshaderSource.staticTextureStats.emplace_back(stat);
  else
    preshaderSource.dynamicResourceStats.emplace_back(stat);
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

bool AssembleShaderEvalCB::end_pass()
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
  p.slope_z_bias = true;

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
    curvariant->suppBlk.resize(curpass->preshader->namedConstTable.suppBlk.size());
    for (const auto &[i, blk] : enumerate(curpass->preshader->namedConstTable.suppBlk))
      curvariant->suppBlk[i] = blk;

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
      // curpass->getCidx would have id if we're compiling in thread
      if (curpass->fsh.empty() && !curpass->getCidx(HLSL_PS).has_value() && hlsls.fields.ps.hasCompilation())
      {
        report_error(parser, terminal, "%s: missing pixel shader", shname);
        accept = false;
      }
      if (curpass->vpr.empty() && !curpass->getCidx(HLSL_VS).has_value() && !curpass->getCidx(HLSL_MS).has_value())
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
  return accept;
}

void AssembleShaderEvalCB::end_eval(shader_decl &sh)
{
  static_assert(STAGE_PS < STAGE_VS && STAGE_CS < STAGE_VS);

  preshaderSource.isCompute = isCompute();

  // First goes all PS stats, then all VS stats
  // Within the stage, all textures with samplers go first, then without samplers
  fast_sort(preshaderSource.staticTextureStats, [](const PreshaderStat &s1, const PreshaderStat &s2) {
    if (s1.stage != s2.stage)
      return s1.stage < s2.stage;
    const bool s1NeedSampler = semantic::vt_is_static_sampled_texture(s1.vt);
    const bool s2NeedSampler = semantic::vt_is_static_sampled_texture(s2.vt);
    return s1NeedSampler > s2NeedSampler;
  });

  if (ctx.shCtx().isDebugModeEnabled())
  {
    Tab<uint32_t> stages = {STAGE_VS, STAGE_PS};
    if (isCompute())
      stages = {STAGE_CS};
    // else if (hlsls.fields.vs.symbol == nullptr) // mesh shader
    //   stages = {STAGE_VS+1, STAGE_VS+2, STAGE_PS};
    for (auto stage : stages)
      eval_external_block(*ctx.shCtx().debugBlock(stage));
  }

  auto &preshaderCache = ctx.shCtx().preshaderCache();
  CompiledPreshader *compiledPreshaderRef = preshaderCache.query(preshaderSource);
  if (!compiledPreshaderRef)
  {
    if (auto preshaderCompilationOutputMaybe = compile_variant_preshader(preshaderSource, ctx))
      compiledPreshaderRef = preshaderCache.post(eastl::move(preshaderSource), eastl::move(preshaderCompilationOutputMaybe->code));
    else
      return;
  }
  else
  {
    for (auto const &name : compiledPreshaderRef->dynamicSamplerImplicitVars)
    {
      sampler_decl smpDecl = {};
      SHTOK_ident smpNameId = {};
      smpNameId.text = name.c_str();
      smpDecl.name = &smpNameId;
      smpDecl.is_always_referenced = nullptr;
      add_dynamic_sampler_for_stcode(code, sclass, smpDecl, parser, ctx.tgtCtx().varNameMap());
    }
    for (int vid : compiledPreshaderRef->usedMaterialVarIds)
      code.vars[vid].used = true;
  }

  for (auto [i, shvt] : enumerate(compiledPreshaderRef->shadervarTexTypes))
  {
    if (shvt != SHVT_TEX_UNKNOWN)
      ctx.parsedSemCode().vars[i].texType = shvt;
  }

  if (curpass)
  {
    curpass->preshader = compiledPreshaderRef;

    auto const staticVsTexRange = compiledPreshaderRef->namedConstTable.slotTextureSuballocators.vsTex.getRange();
    auto const staticPsTexRange = compiledPreshaderRef->namedConstTable.slotTextureSuballocators.psTex.getRange();
    auto const staticVsSamplersRange = compiledPreshaderRef->namedConstTable.slotTextureSuballocators.vsSamplers.getRange();
    auto const staticPsSamplersRange = compiledPreshaderRef->namedConstTable.slotTextureSuballocators.psSamplers.getRange();
    curpass->vsTexSmpRange = SlotTexturesRangeInfo{uint8_t(staticVsTexRange.min), uint8_t(staticVsSamplersRange.min),
      uint8_t(staticVsTexRange.cap - staticVsTexRange.min), uint8_t(staticVsSamplersRange.cap - staticVsSamplersRange.min)};
    curpass->psTexSmpRange = SlotTexturesRangeInfo{uint8_t(staticPsTexRange.min), uint8_t(staticPsSamplersRange.min),
      uint8_t(staticPsTexRange.cap - staticPsTexRange.min), uint8_t(staticPsSamplersRange.cap - staticPsSamplersRange.min)};

    curpass->psOrCsConstRange =
      compiledPreshaderRef->namedConstTable.pixelOrComputeRegAllocators[HLSL_RSPACE_C].getRange(HlslSlotSemantic::ALLOCATED);
    curpass->vsConstRange =
      compiledPreshaderRef->namedConstTable.vertexRegAllocators[HLSL_RSPACE_C].getRange(HlslSlotSemantic::ALLOCATED);
    curpass->bufferedConstRange =
      compiledPreshaderRef->namedConstTable.bufferedConstsRegAllocator.getRange(HlslSlotSemantic::ALLOCATED);
  }

  code.regsize = compiledPreshaderRef->requiredRegCount;
  hlsls.forEach([this](HlslCompile &comp) { evalHlslCompileClass(&comp); });

  if (end_pass())
  {
    if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
      curpass->boolAstNodesEvaluationResults = eastl::move(boolElementsEvaluationResults);
    if (no_dynstcode && (compiledPreshaderRef->dynStBytecode.size() || compiledPreshaderRef->cppStcode.cppStcode.hasCode()))
    {
      report_error(parser, no_dynstcode,
        "%s: has dynstcode, while it was required to have none with no_dynstcode:", ctx.shCtx().name());
      for (Terminal *var : compiledPreshaderRef->stcodeVars)
        report_error(parser, var, "'%s'", var->text);
      debug("\n******* State code shader --s--");
      ShUtils::shcod_dump(compiledPreshaderRef->dynStBytecode, nullptr);
    }
  }

  curpass = nullptr;
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

#if !_CROSS_TARGET_C1 && !_CROSS_TARGET_C2 && !_CROSS_TARGET_DX12 && !_CROSS_TARGET_DX11 && !_CROSS_TARGET_EMPTY
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

class CompileShaderJob : public shc::Job
{
public:
  CompileShaderJob(AssembleShaderEvalCB *ascb, AssembleShaderEvalCB::HlslCompile &hlsl, HlslCompilationStage stage,
    const hlsl_compile_class *compile_symbol, CodeSourceBlocks &code_blocks,
    dag::ConstSpan<CodeSourceBlocks::Unconditional *> preprocessed_blocks, uint64_t variant_hash) :
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

    dag::ConstSpan<char> main_src = code_blocks.buildSourceCode(preprocessed_blocks);
    source.setStr(main_src.data(), main_src.size());

    eastl::string src_predefines = assembly::build_hardware_defines_hlsl(profile.c_str(), enableFp16, ascb->ctx.tgtCtx().compCtx());
#if _CROSS_TARGET_C2








#endif

    ascb->curpass->preshader->namedConstTable.patchHlsl(source, HLSL_STAGE_TO_SHADER_STAGE[stage], *ascb->curpass->preshader, *lexer,
      max_constants_no, eastl::string_view{src_predefines}, curpass->dual_source_blending);
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

  CodeSourceBlocks *codeP = ctx.shCtx().hlslCodeBlocks().validProfileSwitch(compile.profile);
  G_ASSERTF(codeP, "ShaderParser::curXsCode must be non null here by design");
  CodeSourceBlocks &code = *codeP;

  ShaderBytecodeCache &cache = ctx.tgtCtx().bytecodeCache();

  dag::ConstSpan<CodeSourceBlocks::Unconditional *> code_blocks = code.getPreprocessedCode(*this);
  CryptoHash code_digest = code.getCodeDigest(code_blocks);
  CryptoHash const_digest = curpass->preshader->namedConstTable.getDigest(HLSL_STAGE_TO_SHADER_STAGE[stage], *curpass->preshader);

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

    shc::add_job(new CompileShaderJob{this, hlsl, stage, hlsl.symbol, code, code_blocks, shader_variant_hash});
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

static void upgrade_sm(const shc::ShaderContext &ctx, char *sm, const char *new_sm, const char *reason)
{
  G_ASSERT(strlen(new_sm) == 3 && isdigit(new_sm[0]) && new_sm[1] == '.' && isdigit(new_sm[2]));

  const uint32_t numberOffset = (0 == strncmp(sm, "lib", 3)) ? 4 : 3;

  auto &major = sm[numberOffset + 0];
  auto &minor = sm[numberOffset + 2];

  const uint32_t smV = (uint32_t(major) << 8) | minor;
  const uint32_t newV = (uint32_t(new_sm[0]) << 8) | new_sm[2];

  if (smV < newV)
  {
    sh_debug(SHLOG_INFO, "Upgrading shader model from %s to %s for %s due to %s", sm, new_sm, ctx.name(), reason);
    major = new_sm[0];
    minor = new_sm[2];
  }
}

void CompileShaderJob::doJobBody()
{
  // @TODO: it should be const in fields!
  const shc::ShaderContext &sctx = ctx;

  char sha1SrcPath[420];
  sha1SrcPath[0] = 0;
  unsigned char srcSha1[HASH_SIZE];

#if _CROSS_TARGET_DX12 || _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL || (!_CROSS_TARGET_DX11 && !_CROSS_TARGET_EMPTY)
#if _CROSS_TARGET_DX12
  if (dx12::dxil::Platform::PC != shc::config().targetPlatform)
  {
    // All non PC (eg XB One and XB Series) support 6.6 and up, so upgrade to it, to use dynamic resources for bindless
    upgrade_sm(sctx, profile, "6.6", "GDK Baseline profile 6.6");
  }
  else
#endif
  {
    if (strstr(source, "SV_ViewID"))
      upgrade_sm(sctx, profile, "6.1", "presence of SV_ViewID");

    if (strstr(source, "SV_ShadingRate"))
      upgrade_sm(sctx, profile, "6.4", "presence of SV_ShadingRate");
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
#if _CROSS_TARGET_SPIRV
    HASH_UPDATE(&sha1, (const unsigned char *)shaderName, (uint32_t)strlen(shaderName));
#endif
    HASH_UPDATE(&sha1, (const unsigned char *)entry.c_str(), (uint32_t)strlen(entry));
    // optimization level is a part of output dir, but still
    HASH_UPDATE(&sha1, (const unsigned char *)&hlslOptimizationLevelVar, (uint32_t)sizeof(hlslOptimizationLevelVar));
    HASH_UPDATE(&sha1, (const unsigned char *)&hlsl2021Var, (uint32_t)sizeof(hlsl2021Var));
    HASH_UPDATE(&sha1, (const unsigned char *)&enableFp16, (uint32_t)sizeof(enableFp16));
    HASH_UPDATE(&sha1, (const unsigned char *)&enableBindlessVar, (uint32_t)sizeof(enableBindlessVar));
#if _CROSS_TARGET_DX12
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
          int sha1FileLen = 0;
          const uint8_t *content = (const uint8_t *)df_mmap(sha1BinFile, &sha1FileLen);
          if (!sha1FileLen)
          {
            debug("link is broken?");
            df_close(sha1BinFile);
          }
          else
          {
            uint32_t metadata_size = 0;
            memcpy(&metadata_size, content, sizeof(metadata_size));

            uint32_t bytecode_size = 0;
            memcpy(&bytecode_size, content + sizeof(metadata_size), sizeof(bytecode_size));

            const uint8_t *start_metadata = content + sizeof(metadata_size) + sizeof(bytecode_size);
            const uint8_t *end_metadata = start_metadata + metadata_size;
            compile_result.metadata.assign(start_metadata, end_metadata);

            const uint8_t *start_bytecode = end_metadata;
            const uint8_t *end_bytecode = start_bytecode + bytecode_size;
            compile_result.bytecode.assign(start_bytecode, end_bytecode);
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
    compile_result = compileShaderSpirV(shc::config().dxcContext, source, profile, entry, !shc::config().hlslNoDisassembly,
      useHlsl2021, enableFp16, shc::config().hlslSkipValidation, localHlslOptimizationLevel ? true : false, max_constants_no,
      shaderName, shader_variant_hash, shc::config().enableBindless, full_debug || shc::config().hlslDebugLevel != DebugLevel::NONE,
      shc::config().dumpSpirvOnly, shc::config().sortGlobalConstsByOffset);
  }
  else
  {
    compile_result.bytecode.resize(sizeof(uint64_t));
  }
#elif _CROSS_TARGET_DX12
  // NOTE: when we support this kind of switch somehow this can be replaced with actual information
  // of use or not
  compile_result = dx12::dxil::compileShader({.name = shaderName,
    .profile = profile,
    .entry = entry,
    .source = make_span_const(source).first(sourceLen),
    .needDisasm = !shc::config().hlslNoDisassembly,
    .maxConstantsNo = max_constants_no,
    .platform = shc::config().targetPlatform,
    .warningsAsErrors = !forceDisableWarnings && shc::config().hlslWarningsAsErrors,
    .embedSource = embed_source,
    .debugLevel = full_debug ? DebugLevel::FULL_DEBUG_INFO : shc::config().hlslDebugLevel,
    .compilationOptions =
      {
        .optimize = localHlslOptimizationLevel ? true : false,
        .skipValidation = shc::config().hlslSkipValidation,
        .debugInfo = is_hlsl_debug(),
        .scarlettW32 = useWave32,
        .hlsl2021 = useHlsl2021,
        .enableFp16 = enableFp16,
      },
    .PDBDir = shc::config().dx12PdbCacheDir,
    .streamOutputComponents = streamOutputComponents});
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
    uint32_t metadata_size = compile_result.metadata.size();
    uint32_t bytecode_size = compile_result.bytecode.size();
    uint32_t total_size = sizeof(metadata_size) + sizeof(bytecode_size) + metadata_size + bytecode_size + sizeof(ComputeShaderInfo);

    unsigned char binSha1[HASH_SIZE];
    HASH_CONTEXT sha1;
    HASH_INIT(&sha1);
    HASH_UPDATE(&sha1, (const unsigned char *)&metadata_size, sizeof(metadata_size));
    HASH_UPDATE(&sha1, (const unsigned char *)&bytecode_size, sizeof(bytecode_size));
    HASH_UPDATE(&sha1, (const unsigned char *)compile_result.metadata.data(), compile_result.metadata.size());
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
    if (df_stat(sha1BinPath, &binbuf) != -1 && binbuf.size == total_size)
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
      int written = df_write(tmpF, &metadata_size, sizeof(metadata_size));
      written += df_write(tmpF, &bytecode_size, sizeof(bytecode_size));
      written += df_write(tmpF, compile_result.metadata.data(), compile_result.metadata.size());
      written += df_write(tmpF, compile_result.bytecode.data(), compile_result.bytecode.size());
      written += df_write(tmpF, &compile_result.computeShaderInfo, sizeof(ComputeShaderInfo));
      if (written == total_size)
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
  auto entryIdsMaybe = curpass->getCidx(stage);
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

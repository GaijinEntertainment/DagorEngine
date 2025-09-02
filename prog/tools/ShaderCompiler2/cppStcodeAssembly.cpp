// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cppStcodeAssembly.h"
#include "cppStcode.h"
#include "cppStcodeUtils.h"
#include "cppStcodeBuilder.h"
#include "variantSemantic.h"
#include "shSemCode.h"
#include "globalConfig.h"
#include "commonUtils.h"
#include <debug/dag_assert.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>
#include <util/dag_string.h>
#include <util/dag_stlqsort.h>
#include <generic/dag_enumerate.h>

using shader_layout::StcodeConstValidationMask;

static const char *bool_to_str(bool val) { return val ? "true" : "false"; }

static constexpr int FLOAT4_REG_SIZE = 4;

static void resolve_placeholder_at(eastl::string &templ, size_t at, const char *substitute)
{
  G_ASSERT(at < templ.length());
  const char *p = templ.begin() + at;

  size_t subsLen = strlen(substitute);
  eastl::string dst;
  dst.resize(templ.size() - 1 + subsLen);
  eastl::copy(templ.cbegin(), p, dst.begin());
  eastl::copy_n(substitute, subsLen, dst.begin() + (p - templ.begin()));
  eastl::copy(p + 1, templ.cend(), dst.begin() + (p + subsLen - templ.begin()));

  templ = eastl::move(dst);
}

void resolve_first_placeholder(eastl::string &templ, char placeholder, const char *substitute)
{
  const char *p = strchr(templ.begin(), placeholder);
  G_ASSERTF(p, "The template \"%s\" does not contain the placeholder '%c'");

  resolve_placeholder_at(templ, p - templ.begin(), substitute);
}

void resolve_last_placeholder(eastl::string &templ, char placeholder, const char *substitute)
{
  const char *p = strrchr(templ.begin(), placeholder);
  G_ASSERTF(p, "The template \"%s\" does not contain the placeholder '%c'");

  resolve_placeholder_at(templ, p - templ.begin(), substitute);
}

/// StcodeExpression ///
void StcodeExpression::specifyNextExprElement(ElementType elem_type, const void *arg1, const void *arg2, const void *arg3,
  const void *arg4)
{
  RET_IF_SHOULD_NOT_COMPILE();

  G_ASSERT(strchr(content.c_str(), EXPR_ELEMENT_PLACEHOLDER));

  eastl::string element{};
  switch (elem_type)
  {
    case ElementType::COMPLEX_SINGLE_OP:
      G_ASSERT(!arg1 && !arg2 && !arg3 && !arg4);
      element = string_f("%c%c", EXPR_UNARY_OP_PLACEHOLDER, EXPR_ELEMENT_PLACEHOLDER);
      break;
    case ElementType::COLORVAL:
      G_ASSERT(!arg2 && !arg3 && !arg4);
      element = string_f("%s4(%c, %c, %c, %c)", arg1 ? (const char *)arg1 : "float", EXPR_ELEMENT_PLACEHOLDER,
        EXPR_ELEMENT_PLACEHOLDER, EXPR_ELEMENT_PLACEHOLDER, EXPR_ELEMENT_PLACEHOLDER);
      break;
    case ElementType::GLOBVAR:
    {
      G_ASSERT(arg1 && !arg4);
      eastl::string varDereference;
      const bool needPointerDeref = bool(arg3);
      if (strchr((const char *)arg1, '[')) // This means, that variable name is and index into an array (arr[i])
        varDereference = eastl::string((const char *)arg1);
      else if (needPointerDeref)
        varDereference = string_f("(*%s)", (const char *)arg1);
      else
        varDereference = string_f("%s", (const char *)arg1);
      eastl::string typeCast = arg2 ? string_f("(%s)", (const char *)arg2) : "";
      element = string_f("(%c%s%s)", EXPR_UNARY_OP_PLACEHOLDER, typeCast.c_str(), varDereference.c_str());
    }
    break;
    case ElementType::LOCVAR:
    {
      G_ASSERT(arg1 && !arg3 && !arg4);
      eastl::string typeCast = arg2 ? string_f("(%s)", (const char *)arg2) : "";
      element = string_f("(%c%s%s)", EXPR_UNARY_OP_PLACEHOLDER, typeCast.c_str(), (const char *)arg1);
    }
    break;
    case ElementType::REAL_CONST:
      G_ASSERT(arg1 && !arg2 && !arg3 && !arg4);
      element = string_f("%.8ff", *(const float *)arg1);
      break;
    case ElementType::INT_CONST:
      G_ASSERT(arg1 && !arg2 && !arg3 && !arg4);
      element = string_f("%d", *(const int *)arg1);
      break;
    case ElementType::SINGLE_CHANNEL_MASK:
    {
      G_ASSERT(arg1 && !arg2 && !arg3 && !arg4);
      char channelSymbol = '\0';
      switch (*(const shexpr::ColorChannel *)arg1)
      {
        case shexpr::CC_R: channelSymbol = 'r'; break;
        case shexpr::CC_G: channelSymbol = 'g'; break;
        case shexpr::CC_B: channelSymbol = 'b'; break;
        case shexpr::CC_A: channelSymbol = 'a'; break;

        default: G_ASSERT(0);
      }
      element = string_f("%c.%c", EXPR_ELEMENT_PLACEHOLDER, channelSymbol);
    }
    break;
    case ElementType::MULTIPLE_CHANNEL_MASK:
      G_ASSERT(arg1 && !arg2 && !arg3 && !arg4);
      element = string_f("%c.swizzle(\"%s\")", EXPR_ELEMENT_PLACEHOLDER, arg1);
      break;
    case ElementType::COMPLEX_MULTIPLE_OPS:
    {
      G_ASSERT(arg1 && arg2 && !arg3 && !arg4);
      const shexpr::BinaryOperator *opsData = (const shexpr::BinaryOperator *)arg1;
      int opCount = *(const int *)arg2;
      dag::ConstSpan<shexpr::BinaryOperator> ops = make_span_const(opsData, opCount);
      StcodeBuilder exprPlaceholder; // placeholder for first operand
      exprPlaceholder.emplaceBackFmt("%c", EXPR_ELEMENT_PLACEHOLDER);
      for (auto op : ops)
      {
        char opSymbol = '\0';
        switch (op)
        {
          case shexpr::OP_ADD: opSymbol = '+'; break;
          case shexpr::OP_SUB: opSymbol = '-'; break;
          case shexpr::OP_MUL: opSymbol = '*'; break;
          case shexpr::OP_DIV: opSymbol = '/'; break;
          default: G_ASSERT(0);
        }
        exprPlaceholder.emplaceBackFmt(" %c %c", opSymbol, EXPR_ELEMENT_PLACEHOLDER);
      }

      element = string_f("%c(%s)", EXPR_UNARY_OP_PLACEHOLDER, exprPlaceholder.release().c_str());
    }
    break;
    case ElementType::SHVAR:
    {
      G_ASSERT(arg1 && arg2);
      eastl::string typeCast = arg3 ? string_f("(%s)", (const char *)arg3) : "";
      const bool isResource = bool(arg4);
      element = string_f("(%c%s%s(%s, %s))", EXPR_UNARY_OP_PLACEHOLDER, typeCast.c_str(), isResource ? "VARLOC" : "VAR",
        (const char *)arg1, (const char *)arg2);
    }
    break;
      break;
    case ElementType::FUNC:
    {
      G_ASSERT(arg1 && arg2 && !arg3 && !arg4);
      const char *name = (const char *)arg1;
      size_t argCount = *(const size_t *)arg2;
      functional::FunctionId funcId;
      G_ASSERT(functional::getFuncId(name, funcId));

      element = getFunctionCallTemplate(funcId, name, argCount);
    }
    break;
    case ElementType::COLOR_CONST:
      G_ASSERT(arg1 && arg2 && arg3 && arg4);
      element = string_f("float4(%.8ff, %.8ff, %.8ff, %.8ff)", *(const float *)arg1, *(const float *)arg2, *(const float *)arg3,
        *(const float *)arg4);
      break;

    default: G_ASSERT(0);
  }

  resolve_first_placeholder(content, EXPR_ELEMENT_PLACEHOLDER, element.c_str());
}

void StcodeExpression::specifyNextExprUnaryOp(shexpr::UnaryOperator op)
{
  RET_IF_SHOULD_NOT_COMPILE();

  G_ASSERT(strchr(content.c_str(), EXPR_UNARY_OP_PLACEHOLDER));
  // @NOTE: because we specify unary ops while unwinding, the placeholders have to be resolved in reverse order
  resolve_last_placeholder(content, EXPR_UNARY_OP_PLACEHOLDER, op == shexpr::UOP_NEGATIVE ? "-" : "");
}

eastl::string StcodeExpression::releaseAssembledCode()
{
  RET_IF_SHOULD_NOT_COMPILE({});

  G_ASSERT(!strchr(content.c_str(), EXPR_ELEMENT_PLACEHOLDER) && !strchr(content.c_str(), EXPR_UNARY_OP_PLACEHOLDER));
  return eastl::move(content);
}

eastl::string StcodeExpression::getFunctionCallTemplate(functional::FunctionId func_id, const char *name, int arg_cnt)
{
  // @NOTE: fade_val actually takes 2 args, the first being a call to get_shader_global_time_phase with 2 args itself.
  //        Here it is more appropriate to handle this as two calls.
  if (func_id == functional::BF_FADE_VAL)
    --arg_cnt;

  StcodeBuilder argsPlaceholder;
  for (int i = 0; i < arg_cnt; i++)
    argsPlaceholder.emplaceBackFmt("%s%c", i == 0 ? "" : ", ", EXPR_ELEMENT_PLACEHOLDER);

  switch (func_id)
  {
    case functional::BF_POW:
    case functional::BF_SQRT:
    case functional::BF_MIN:
    case functional::BF_MAX:
    case functional::BF_FSEL:
    case functional::BF_SIN:
    case functional::BF_COS: return string_f("v%s(%s)", name, argsPlaceholder.release().c_str());
    case functional::BF_VECPOW: return string_f("vpow(%s)", argsPlaceholder.release().c_str());
    case functional::BF_SRGBREAD: return string_f("srgb_read(%s)", argsPlaceholder.release().c_str());
    case functional::BF_ANIM_FRAME: return string_f("anim_frame(%s)", argsPlaceholder.release().c_str());
    case functional::BF_TIME_PHASE: return string_f("get_shader_global_time_phase(%s)", argsPlaceholder.release().c_str());
    case functional::BF_WIND_COEFF: return string_f("wind_coeff(get_shader_global_time_phase(%s))", argsPlaceholder.release().c_str());
    case functional::BF_FADE_VAL:
      return string_f("wind_coeff(get_shader_global_time_phase(%s), %s)", argsPlaceholder.release().c_str(), ", ?");
    case functional::BF_GET_DIMENSIONS: return string_f("get_tex_dim(%s)", argsPlaceholder.release().c_str());
    case functional::BF_GET_SIZE: return string_f("get_buf_size(%s)", argsPlaceholder.release().c_str());
    case functional::BF_GET_VIEWPORT: return eastl::string("get_viewport()");
    case functional::BF_EXISTS_TEX: return string_f("exists_tex(%s)", argsPlaceholder.release().c_str());
    case functional::BF_EXISTS_BUF: return string_f("exists_buf(%s)", argsPlaceholder.release().c_str());
    case functional::BF_REQUEST_SAMPLER: return string_f("request_sampler(%s)", argsPlaceholder.release().c_str());
    default: G_ASSERT_RETURN(0, eastl::string());
  }
}


/// StcodeRoutine ///

StcodeRoutine::StcodeRoutine()
{
  if (shc::config().generateCppStcodeValidationData)
    constMask = eastl::make_unique<StcodeConstValidationMask>();
}

void StcodeRoutine::addSetConstStmt(const char *type, size_t type_size, const char *location, const char *expr, const char *dest_array,
  int array_index)
{
  const auto arrOffset = array_index == 0 ? "" : string_f(" + (%d * %d)", type_size, array_index);
  const auto loc = string_f("(LOC(%s)%s)", location, arrOffset.c_str());
  addFormattedStatement("(%s &)(%s[%s]) = %s;\n", type, dest_array, loc.c_str(), expr);
}

// @TODO: proper indentation for decls
void StcodeRoutine::addBoolVarDecl(const char *name)
{
  RET_IF_SHOULD_NOT_COMPILE();
  decls.emplaceBackFmt("  bool %s = false;\n", name);
}

void StcodeRoutine::addLocalVarDecl(ShaderVarType type, const char *name)
{
  RET_IF_SHOULD_NOT_COMPILE();
  decls.emplaceBackFmt("  %s %s;\n", stcode::shadervar_type_to_stcode_type(type), name);
}

void StcodeRoutine::setVarValue(const char *name, const char *val)
{
  RET_IF_SHOULD_NOT_COMPILE();
  ++nonContributingStatementsCnt;
  addFormattedStatement("%s = %s;\n", name, val);
}

void StcodeRoutine::addStmt(const char *val) { addFormattedStatement("%s;\n", val); }

CryptoHash StcodeRoutine::calcHash(const CryptoHash *base)
{
  auto hasher = base ? CryptoHasher{*base} : CryptoHasher{};
  if (hasCode())
  {
    code.iterateFragments([&hasher](const eastl::string &s) { hasher.update(s.c_str(), s.length()); });
    decls.iterateFragments([&hasher](const eastl::string &s) { hasher.update(s.c_str(), s.length()); });
    hasher.update(0u);
    for (const auto &r : vertexRegs.content)
      hasher.update(r.reg);
    hasher.update(0u);
    for (const auto &r : pixelOrComputeRegs.content)
      hasher.update(r.reg);
  }
  return hasher.hash();
}

/// DynamicStcodeRoutine ///

static const char *stage_to_name_prefix(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_CS: return "cs";
    case STAGE_PS: return "ps";
    case STAGE_VS: return "vs";
    default: G_ASSERT_RETURN(0, "");
  }
}
static const char *stage_to_name_suffix(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_CS: return "_cs";
    case STAGE_PS: return "_ps";
    case STAGE_VS: return "_vs";
    default: G_ASSERT_RETURN(0, "");
  }
}
static const char *stage_to_static_tex_name_suffix(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_PS: return "_pt";
    case STAGE_VS: return "_vt";
    default: G_ASSERT_RETURN(0, "");
  }
}
static const char *stage_to_runtime_const_str(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_CS: return "STAGE_CS";
    case STAGE_PS: return "STAGE_PS";
    case STAGE_VS: return "STAGE_VS";
    default: G_ASSERT_RETURN(0, "");
  }
}
static const char *stage_to_const_array_name(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_CS: return "csconst__";
    case STAGE_PS: return "psconst__";
    case STAGE_VS: return "vsconst__";
    default: G_ASSERT_RETURN(0, "");
  }
}
static const char *stage_to_tex_array_name(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_PS: return "pstex__";
    case STAGE_VS: return "vstex__";
    default: G_ASSERT_RETURN(0, "");
  }
}
static auto stage_to_dyn_const_range_names(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_CS: return eastl::make_pair("DYNOFS_CS", "DYNRNG_CS");
    case STAGE_PS: return eastl::make_pair("DYNOFS_PS", "DYNRNG_PS");
    case STAGE_VS: return eastl::make_pair("DYNOFS_VS", "DYNRNG_VS");
    default: G_ASSERT_RETURN(0, eastl::make_pair("", ""));
  }
}

static constexpr char STATIC_CONSTS_NAME_SUFFIX[] = "_scbuf";
static constexpr char STATIC_CONSTS_ARRAY_NAME[] = "stconst__";

StcodeAccumulator DynamicStcodeRoutine::releaseAssembledCode(size_t routine_idx, size_t static_var_idx,
  HlslRegRange ps_or_cs_const_range, HlslRegRange vs_const_range)
{
  RET_IF_SHOULD_NOT_COMPILE({});

  StcodeAccumulator result;
  result.headerStrings.emplaceBackFmt("ROUTINE_VISIBILITY void dynroutine%u(const void *vars, uint32_t dyn_table_offset);\n",
    routine_idx);

  if (static_var_idx != STATIC_VAR_ABSCENT_ID)
  {
    result.cppStrings.emplaceBackFmt("\n#ifdef CUR_STVAR_ID\n#undef CUR_STVAR_ID\n#endif\n\n#define CUR_STVAR_ID %u\n",
      static_var_idx);
  }

  if (!hasCode())
    result.cppStrings.emplaceBackFmt("ROUTINE_VISIBILITY void dynroutine%u(const void *, uint32_t) {}\n\n", routine_idx);
  else
  {
    G_ASSERT(blockDepth == 1);

    constexpr char cppRoutineTemplate[] =
#include "_stcodeTemplates/dynamicRoutine.cpp.inl.fmt"
      ;

    const char *locMacro = usesDynamicBranching ? (isCompute ? "LOCDYN_COMP" : "LOCDYN_GRAPH") : "LOCSTAT";
    result.cppStrings.emplaceBack("\n#ifdef LOC\n#undef LOC\n#endif\n\n");
    result.cppStrings.emplaceBackFmt("#define LOC(name_) %s(name_)\n", locMacro);

    // @TODO: builders
    eastl::string locations = "";
    eastl::string consts = "";
    eastl::string splats = "";
    auto appendRegLocations = [&](ShaderStage stage) {
      const HlslRegRange constRange = (stage == STAGE_VS ? vs_const_range : ps_or_cs_const_range);

      // @TODO: identation with a uniform API (to allow to centrally change format
      for (const auto &loc : regsForStage(stage).content)
      {
        if (usesDynamicBranching) // Locations are already filled with indirect indices and named properly
          locations.append_sprintf("    %s = %u,\n", loc.name.c_str(), loc.reg);
        else // Otherwise, names have to be postfixed and registers calculated
        {
          const int reg = loc.isNumeric ? (loc.reg - constRange.min) * FLOAT4_REG_SIZE : loc.reg;
          locations.append_sprintf("    %s%s = %u,\n", loc.name.c_str(), stage_to_name_suffix(stage), reg);
        }
      }

      if (const int range = constRange.cap - constRange.min; range > 0)
      {
        const char *arrName = stage_to_const_array_name(stage);
        consts.append_sprintf("  alignas(16) float %s[%d];\n", arrName, range * FLOAT4_REG_SIZE);

        if (usesDynamicBranching)
        {
          auto [dynOfsName, dynRangeName] = stage_to_dyn_const_range_names(stage);
          splats.append_sprintf("  if (%s > 0) set_const(%s, %s, (float4 *)%s, %s);\n", dynRangeName,
            stage_to_runtime_const_str(stage), dynOfsName, arrName, dynRangeName);
        }
        else
        {
          splats.append_sprintf("  set_const(%s, %d, (float4 *)%s, %d);\n", stage_to_runtime_const_str(stage), constRange.min, arrName,
            range);
        }
      }
    };

    if (isCompute)
      appendRegLocations(STAGE_CS);
    else
    {
      appendRegLocations(STAGE_VS);
      appendRegLocations(STAGE_PS);
    }

    result.cppStrings.emplaceBackFmt(cppRoutineTemplate, routine_idx, locations.c_str(), consts.c_str(), decls.release().c_str(),
      code.release().c_str(), splats.c_str());
  }

  return result;
}

const char *glob_matrix_cb_name(StcodeRoutine::GlobMatrixType type)
{
  switch (type)
  {
    case StcodeRoutine::GlobMatrixType::GLOB: return "get_globtm";
    case StcodeRoutine::GlobMatrixType::PROJ: return "get_projtm";
    case StcodeRoutine::GlobMatrixType::VIEWPROJ: return "get_viewprojtm";
    default: G_ASSERT_RETURN(0, nullptr);
  }
}

void DynamicStcodeRoutine::addShaderGlobMatrix(ShaderStage stage, GlobMatrixType mtype, const char *name, int id)
{
  RET_IF_SHOULD_NOT_COMPILE();

  const eastl::string location(eastl::string::CtorSprintf{}, "%s%s", name, stage_to_name_suffix(stage));
  const eastl::string tempName(eastl::string::CtorSprintf{}, "tm_%s%s", name, stage_to_name_suffix(stage));

  addFormattedStatement("float4x4 %s;\n", tempName.c_str());
  addFormattedStatement("%s(&%s);\n", glob_matrix_cb_name(mtype), tempName.c_str());
  addSetConstStmt("float4x4", 4, location.c_str(), tempName.c_str(), stage_to_const_array_name(stage), 0);

  regsForStage(stage).add(name, id, true, 4);
}

void DynamicStcodeRoutine::addShaderGlobVec(ShaderStage stage, GlobVecType vtype, const char *name, int id, int compId)
{
  RET_IF_SHOULD_NOT_COMPILE();

  auto globVecCbName = [](GlobVecType vtype) {
    switch (vtype)
    {
      case GlobVecType::VIEW: return "get_lview";
      case GlobVecType::WORLD: return "get_lworld";
      default: G_ASSERT(0); return "";
    }
  };

  const eastl::string location(eastl::string::CtorSprintf{}, "%s%s", name, stage_to_name_suffix(stage));
  const eastl::string tempName(eastl::string::CtorSprintf{}, "lv_%s%s", name, stage_to_name_suffix(stage));

  addFormattedStatement("float4 %s;\n", tempName.c_str());
  addFormattedStatement("%s(%d, &%s);\n", globVecCbName(vtype), compId, tempName.c_str());
  addSetConstStmt("float4", 1, location.c_str(), tempName.c_str(), stage_to_const_array_name(stage), 0);

  regsForStage(stage).add(name, id, true, 1);
}

static const char *resource_cb_name(StcodeRoutine::ResourceType type)
{
  switch (type)
  {
    case StcodeRoutine::ResourceType::TEXTURE: return "set_tex";
    case StcodeRoutine::ResourceType::BUFFER: return "set_buf";
    case StcodeRoutine::ResourceType::CONST_BUFFER: return "set_const_buf";
    case StcodeRoutine::ResourceType::SAMPLER: return "set_sampler";
    case StcodeRoutine::ResourceType::RWBUF: return "set_rwbuf";
    case StcodeRoutine::ResourceType::RWTEX: return "set_rwtex";
    case StcodeRoutine::ResourceType::TLAS: return "set_tlas";
    default: G_ASSERT(0); return nullptr;
  }
}

void DynamicStcodeRoutine::addGlobalShaderResource(ShaderStage stage, ResourceType rtype, const char *name, const char *var_name,
  int id)
{
  RET_IF_SHOULD_NOT_COMPILE();

  const eastl::string location(eastl::string::CtorSprintf{}, "%s%s", name, stage_to_name_suffix(stage));

  addFormattedStatement("%s(%s, LOC(%s), %s);\n", resource_cb_name(rtype), stage_to_runtime_const_str(stage), location.c_str(),
    var_name);
  regsForStage(stage).add(name, id);
}

void DynamicStcodeRoutine::addDynamicShaderResource(ShaderStage stage, ResourceType rtype, const char *name, const char *var_name,
  int id)
{
  RET_IF_SHOULD_NOT_COMPILE();

  G_ASSERTF(rtype == ResourceType::TEXTURE || rtype == ResourceType::SAMPLER,
    "Invalid (or not yet implemented) type for shader resource: %d", (int)rtype);

  const char *varTypeName = "void";
  const char *varMacroName = "VARLOC";

  const eastl::string location(eastl::string::CtorSprintf{}, "%s_%s", name, stage_to_name_prefix(stage));

  if (rtype == ResourceType::TEXTURE)
    addFormattedStatement("acquire_tex(%s(%s, %s));", varMacroName, var_name, varTypeName);

  addFormattedStatement("%s(%s, LOC(%s), %s(%s, %s));\n", resource_cb_name(rtype), stage_to_runtime_const_str(stage), location.c_str(),
    varMacroName, var_name, varTypeName);
  regsForStage(stage).add(name, id);
}

static const char *get_shader_const_type_name(ShaderVarType type, semantic::VariableType vt)
{
  using semantic::VariableType;
  switch (vt)
  {
    case VariableType::f1: return "float1";
    case VariableType::f2: return "float2";
    case VariableType::f3: return "float3";
    case VariableType::i1:
    case VariableType::u1: return "int1";
    case VariableType::i2:
    case VariableType::u2: return "int2";
    case VariableType::i3:
    case VariableType::u3: return "int3";

    default: return stcode::shadervar_type_to_stcode_type(type);
  }
}

static bool shader_const_is_numeric(ShaderVarType shvt)
{
  return item_is_in(shvt, {SHVT_INT, SHVT_REAL, SHVT_INT4, SHVT_COLOR4, SHVT_FLOAT4X4});
}

void DynamicStcodeRoutine::addShaderConst(ShaderStage stage, ShaderVarType shvt, semantic::VariableType vt, const char *name, int id,
  const char *val, int array_index)
{
  RET_IF_SHOULD_NOT_COMPILE();

  const char *typeName = get_shader_const_type_name(shvt, vt);
  const eastl::string location = string_f("%s_%s", name, stage_to_name_prefix(stage));

  const int regSize = shvt == SHVT_FLOAT4X4 ? 4 : 1;
  const int size = shader_const_is_numeric(shvt) ? regSize * FLOAT4_REG_SIZE : regSize;

  addSetConstStmt(typeName, size, location.c_str(), val, stage_to_const_array_name(stage), array_index);

  if (array_index == 0)
    regsForStage(stage).add(name, id, true, regSize);
  else
  {
    for (auto &loc : regsForStage(stage).content)
    {
      if (name == loc.name)
      {
        G_ASSERT(loc.isNumeric);
        loc.regSize = max(loc.regSize, regSize * (array_index + 1));
      }
    }
  }
}

/// StaticStcodeRoutine ///

StcodeAccumulator StaticStcodeRoutine::releaseAssembledCode(size_t routine_idx, size_t static_var_idx, HlslRegRange const_range,
  SlotTexturesRangeInfo ps_tex_smp_range, SlotTexturesRangeInfo vs_tex_smp_range)
{
  RET_IF_SHOULD_NOT_COMPILE({});

  StcodeAccumulator result;
  result.headerStrings.emplaceBackFmt("ROUTINE_VISIBILITY void stroutine%u(void *context);\n", routine_idx);

  if (static_var_idx != STATIC_VAR_ABSCENT_ID)
  {
    result.cppStrings.emplaceBackFmt("\n#ifdef CUR_STVAR_ID\n#undef CUR_STVAR_ID\n#endif\n\n#define CUR_STVAR_ID %u\n",
      static_var_idx);
  }

  const char *multidrawCbuf = bool_to_str(supportsMultidraw);

  if (!hasCode())
  {
    result.cppStrings.emplaceBackFmt("ROUTINE_VISIBILITY void stroutine%u(void *context)\n{\n", routine_idx);
    result.cppStrings.emplaceBackFmt("  create_state(nullptr, 0, nullptr, 0, nullptr, 0, %s, context);\n", multidrawCbuf);
    result.cppStrings.emplaceBack("}\n\n");
  }
  else
  {
    G_ASSERT(blockDepth == 1);

    constexpr char cppRoutineTemplate[] =
#include "_stcodeTemplates/staticRoutine.cpp.inl.fmt"
      ;

    constexpr char LOC_MACRO[] = "LOCSTAT";
    result.cppStrings.emplaceBack("\n#ifdef LOC\n#undef LOC\n#endif\n\n");
    result.cppStrings.emplaceBackFmt("#define LOC(name_) %s(name_)\n", LOC_MACRO);

    // @TODO: builders
    eastl::string locations = "";
    eastl::string consts = "";
    eastl::string createStateCall = "";

    for (const auto &loc : constRegs.content)
    {
      locations.append_sprintf("    %s%s = %u,\n", loc.name.c_str(), STATIC_CONSTS_NAME_SUFFIX,
        (loc.reg - const_range.min) * FLOAT4_REG_SIZE);
    }

    // NOTE: we HAVE to zero out unused bytes for bindless state block cache to behave properly w.r.t. static constants
    if (const int range = const_range.cap - const_range.min; range > 0)
      consts.append_sprintf("  alignas(16) float %s[%d] = {};\n", STATIC_CONSTS_ARRAY_NAME, range * FLOAT4_REG_SIZE);

    auto appendTexLocations = [&](ShaderStage stage) {
      const SlotTexturesRangeInfo texSmpRange = stage == STAGE_VS ? vs_tex_smp_range : ps_tex_smp_range;

      for (const auto &loc : regsForStage(stage).content)
      {
        locations.append_sprintf("    %s%s = %u,\n", loc.name.c_str(), stage_to_static_tex_name_suffix(stage),
          loc.reg - texSmpRange.texBase);
      }

      if (texSmpRange.count > 0)
      {
        const char *arrName = stage_to_tex_array_name(stage);
        consts.append_sprintf("  uint %s[%d];\n", arrName, texSmpRange.count);
      }
    };

    appendTexLocations(STAGE_VS);
    appendTexLocations(STAGE_PS);

    const char *vsTexPtr = vs_tex_smp_range.count > 0 ? stage_to_tex_array_name(STAGE_VS) : "nullptr";
    const char *psTexPtr = ps_tex_smp_range.count > 0 ? stage_to_tex_array_name(STAGE_PS) : "nullptr";
    const char *constsPtr = const_range.cap > const_range.min ? STATIC_CONSTS_ARRAY_NAME : "nullptr";

    eastl::string vsTexRange = string_f("%u", vs_tex_smp_range.raw);
    eastl::string psTexRange = string_f("%u", ps_tex_smp_range.raw);
    eastl::string constsRange = string_f("%u", const_range.cap - const_range.min);

    createStateCall = string_f("  create_state(%s, %s, %s, %s, (float4 *)%s, %s, %s, context);\n", vsTexPtr, vsTexRange.c_str(),
      psTexPtr, psTexRange.c_str(), constsPtr, constsRange.c_str(), multidrawCbuf);

    result.cppStrings.emplaceBackFmt(cppRoutineTemplate, routine_idx, locations.c_str(), consts.c_str(), decls.release().c_str(),
      code.release().c_str(), createStateCall.c_str());
  }

  return result;
}

void StaticStcodeRoutine::addStaticShaderTex(ShaderStage stage, const char *name, const char *var_name, int id)
{
  RET_IF_SHOULD_NOT_COMPILE();

  G_ASSERT(stage != STAGE_CS);

  const eastl::string location(eastl::string::CtorSprintf{}, "%s%s", name, stage_to_static_tex_name_suffix(stage));

  addFormattedStatement("%s[LOC(%s)] = acquire_tex(VARLOC(%s, void));\n", stage_to_tex_array_name(stage), location.c_str(), var_name);
  regsForStage(stage).add(name, id);
}

void StaticStcodeRoutine::addBindlessShaderTexture(const char *name, const char *var_name, int id)
{
  RET_IF_SHOULD_NOT_COMPILE();

  addFormattedStatement("reg_bindless(VARLOC(%s, void), LOC(%s%s) / %d, bindless_ctx);\n", var_name, name, STATIC_CONSTS_NAME_SUFFIX,
    FLOAT4_REG_SIZE);
  addShaderConst(SHVT_INT4, semantic::VariableType::i2, name, id, "int4(-1, -1, 0, 0)");
}

void StaticStcodeRoutine::addShaderConst(ShaderVarType shvt, semantic::VariableType vt, const char *name, int id, const char *val,
  int array_index)
{
  RET_IF_SHOULD_NOT_COMPILE();

  const char *typeName = get_shader_const_type_name(shvt, vt);
  const eastl::string location = string_f("%s%s", name, STATIC_CONSTS_NAME_SUFFIX);

  const int regSize = shvt == SHVT_FLOAT4X4 ? 4 : 1;
  const int size = shader_const_is_numeric(shvt) ? regSize * FLOAT4_REG_SIZE : regSize;

  addSetConstStmt(typeName, size, location.c_str(), val, STATIC_CONSTS_ARRAY_NAME, array_index);

  if (array_index == 0)
    constRegs.add(name, id, true, regSize);
  else
  {
    for (auto &loc : constRegs.content)
    {
      if (name == loc.name)
      {
        G_ASSERT(loc.isNumeric);
        loc.regSize = max(loc.regSize, regSize * (array_index + 1));
      }
    }
  }
}

CryptoHash StaticStcodeRoutine::calcHash(const CryptoHash *base)
{
  auto hasher = CryptoHasher{StcodeRoutine::calcHash(base)};
  if (!constRegs.content.empty())
  {
    hasher.update(0u);
    for (const auto &r : constRegs.content)
      hasher.update(r.reg);
  }
  hasher.update(supportsMultidraw);
  return hasher.hash();
}

/// StcodeGlobalVars ///
void StcodeGlobalVars::setVar(ShaderVarType shv_type, const char *name, int id_in_final_dump)
{
  RET_IF_SHOULD_NOT_COMPILE();

  G_ASSERTF(type != Type::MAIN_COLLECTION || id_in_final_dump != INVALID_IN_DUMP_VAR_ID,
    "When updating final collection and assembling fetcher code, pass in a globvar id");
  G_ASSERTF(type != Type::REFERENCED_BY_SHADER || id_in_final_dump == INVALID_IN_DUMP_VAR_ID,
    "When using StcodeGlobVars for intermediate storage, don't pass the id, as it is not known");

  const char *type_name = stcode::shadervar_type_to_stcode_type(shv_type);

  eastl::string varDecl(eastl::string::CtorSprintf{}, "%s *%s", type_name, name);
  cppCode.emplaceBackFmt("%s = nullptr;\n", varDecl.c_str());

  switch (type)
  {
    case Type::MAIN_COLLECTION: fetchersOrFwdCpp.emplaceBackFmt("{(void **)&%s, %d},\n", name, id_in_final_dump); break;
    case Type::REFERENCED_BY_SHADER: fetchersOrFwdCpp.emplaceBackFmt("extern %s;\n", varDecl.c_str()); break;
  }
}

StcodeGlobalVars::Strings StcodeGlobalVars::releaseAssembledCode()
{
  RET_IF_SHOULD_NOT_COMPILE({});

  return Strings{cppCode.release(), fetchersOrFwdCpp.release()};
}


/// StcodeShader ///
void StcodeShader::addStaticVarLocations(StcodeStaticVars &&locations)
{
  RET_IF_SHOULD_NOT_COMPILE();

  CryptoHasher stvarHasher{};
  stvarHasher.update(uint32_t(locations.content.size()));
  for (const auto &loc : locations.content)
  {
    stvarHasher.update(loc.name.c_str());
    stvarHasher.update(uint32_t(loc.getId()));
  }
  const CryptoHash stvarHash = stvarHasher.hash();

  if (auto it = eastl::find(stvarHashes.begin(), stvarHashes.end(), stvarHash); it != stvarHashes.end())
  {
    curStvarId = eastl::distance(stvarHashes.begin(), it);
    return;
  }

  curStvarId = stvarHashes.size();
  stvarHashes.push_back(stvarHash);

  StcodeBuilder offsetsCppStrings;
  offsetsCppStrings.emplaceBackFmt("enum class OffsetStvar%u : size_t\n{\n", curStvarId);
  for (const auto &loc : locations.content)
    offsetsCppStrings.emplaceBackFmt("  %s_ofs = %d,\n", loc.name.c_str(), loc.getId());
  offsetsCppStrings.emplaceBack("};\n");

  code.cppStrings.pushFront(offsetsCppStrings.release());
}

static bool is_prefix(dag::ConstSpan<int> table, dag::ConstSpan<int> enclosing_table)
{
  return table.size() <= enclosing_table.size() &&
         memcmp(table.cbegin(), enclosing_table.cbegin(), table.size() * elem_size(table)) == 0;
}

int StcodeShader::addRegisterTableWithIndex(Tab<int> &&table)
{
  RET_IF_SHOULD_NOT_COMPILE(-1);

  G_ASSERT(regTable.offsets.empty());

  for (int i = 0; i < regTable.combinedTable.size(); ++i)
  {
    if (is_prefix(table, regTable.combinedTable[i]))
      return i;
  }
  regTable.combinedTable.push_back(eastl::move(table));
  return int(regTable.combinedTable.size() - 1);
}

int StcodeShader::addRegisterTableWithOffset(Tab<int> &&table)
{
  RET_IF_SHOULD_NOT_COMPILE(-1);

  G_ASSERT(regTable.combinedTable.size() == regTable.offsets.size());

  for (int i = 0; i < regTable.combinedTable.size(); ++i)
  {
    if (is_prefix(table, regTable.combinedTable[i]))
      return regTable.offsets[i];
  }
  regTable.offsets.push_back(regTable.offsets.empty() ? 0 : (regTable.offsets.back() + (int)regTable.combinedTable.back().size()));
  regTable.combinedTable.push_back(eastl::move(table));
  return regTable.offsets.back();
}

int StcodeShader::addCode(DynamicStcodeRoutine &&routine, HlslRegRange ps_or_cs_const_range, HlslRegRange vs_const_range)
{
  RET_IF_SHOULD_NOT_COMPILE(-1);

  const CryptoHash hash = curStvarId >= 0 ? routine.calcHash(&stvarHashes[curStvarId]) : routine.calcHash();
  for (int i = 0; i < dynamicRoutineHashes.size(); ++i)
  {
    if (dynamicRoutineHashes[i] == hash)
      return i;
  }

  dynamicRoutineHashes.push_back(hash);

  hasCode = true;
  StcodeAccumulator res =
    routine.releaseAssembledCode(dynamicRoutineHashes.size() - 1, curStvarId, ps_or_cs_const_range, vs_const_range);
  code.cppStrings.merge(eastl::move(res.cppStrings));
  code.headerStrings.merge(eastl::move(res.headerStrings));

  return int(dynamicRoutineHashes.size() - 1);
}

int StcodeShader::addCode(StaticStcodeRoutine &&routine, HlslRegRange const_range, SlotTexturesRangeInfo ps_tex_smp_range,
  SlotTexturesRangeInfo vs_tex_smp_range)
{
  RET_IF_SHOULD_NOT_COMPILE(-1);

  const CryptoHash hash = curStvarId >= 0 ? routine.calcHash(&stvarHashes[curStvarId]) : routine.calcHash();
  for (int i = 0; i < staticRoutineHashes.size(); ++i)
  {
    if (staticRoutineHashes[i] == hash)
      return i;
  }

  staticRoutineHashes.push_back(hash);

  hasCode = true;
  StcodeAccumulator res =
    routine.releaseAssembledCode(staticRoutineHashes.size() - 1, curStvarId, const_range, ps_tex_smp_range, vs_tex_smp_range);
  code.cppStrings.merge(eastl::move(res.cppStrings));
  code.headerStrings.merge(eastl::move(res.headerStrings));

  return int(staticRoutineHashes.size() - 1);
}

StcodeShader::RoutineRemappingTables StcodeShader::linkRoutinesFromFile(dag::ConstSpan<CryptoHash> dynamic_routine_hashes,
  dag::ConstSpan<CryptoHash> static_routine_hashes, const char *fn)
{
  RET_IF_SHOULD_NOT_COMPILE({});

  auto linkRoutines = [&](dag::ConstSpan<CryptoHash> new_routine_hashes, auto &hash_collection, auto &name_collection,
                        const char *name_prefix) {
    G_ASSERT(hash_collection.size() == name_collection.size());

    Tab<int> routineRemapping{};
    routineRemapping.resize(new_routine_hashes.size(), -1);

    for (auto [id, hash] : enumerate(new_routine_hashes))
    {
      int i;
      for (i = 0; i < hash_collection.size(); ++i)
      {
        if (hash_collection[i] == hash)
          break;
      }

      if (i == name_collection.size())
      {
        const eastl::string filename = stcode::extract_shader_name_from_path(fn);
        name_collection.emplace_back(string_f("_%s::%sroutine%u", filename.c_str(), name_prefix, id));
        hash_collection.emplace_back(hash);
      }

      routineRemapping[id] = i;
    }

    return routineRemapping;
  };

  return {linkRoutines(dynamic_routine_hashes, dynamicRoutineHashes, dynamicRoutineNames, "dyn"),
    linkRoutines(static_routine_hashes, staticRoutineHashes, staticRoutineNames, "st")};
}

StcodeShader::InterfaceStrings StcodeShader::releaseAssembledInterfaceCode()
{
  RET_IF_SHOULD_NOT_COMPILE({});

  StcodeBuilder regTableStrings;
  if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
  {
    regTableStrings.emplaceBackFmt("static const int16_t reg_table_data[] = {");
    int count = 0;
    for (const auto &subtable : regTable.combinedTable)
    {
      count += subtable.size();
      for (int x : subtable)
      {
        G_ASSERT(x >= INT16_MIN && x <= INT16_MAX);
        regTableStrings.emplaceBackFmt(" %hd,", int16_t(x));
      }
    }
    regTableStrings.emplaceBack(" };\n");
    regTableStrings.emplaceBack("extern const int16_t *const reg_table = &reg_table_data[0];\n");
  }

  StcodeBuilder dynamicPointerTableEntries, staticPointerTableEntries;
  for (const auto &name : dynamicRoutineNames)
    dynamicPointerTableEntries.emplaceBackFmt("&%s,\n", name.c_str());
  for (const auto &name : staticRoutineNames)
    staticPointerTableEntries.emplaceBackFmt("&%s,\n", name.c_str());
  clear_and_shrink(dynamicRoutineNames);
  clear_and_shrink(staticRoutineNames);

  return {regTableStrings.release(), dynamicPointerTableEntries.release(), staticPointerTableEntries.release()};
}

static int get_table_header_size(bool is_compute)
{
  // Table is headed by 2 or 4 ints -- csRangeMin/csRangeSize or psRangeMin/psRangeSize and vsRangeMin/vsRangeSize
  return is_compute ? 2 : 4;
}

StcodePassRegInfoTable build_pass_stcode_reg_table(const ShaderSemCode &semcode)
{
  StcodePassRegInfoTable passInfoTable{};

  for (auto &pt : semcode.passes)
  {
    if (!pt || !pt->pass)
      continue;

    SemanticShaderPass &p = *pt->pass;
    if (p.usedConstStatAstNodes.empty())
    {
      passInfoTable[&p] = StcodePassRegInfo{};
      continue;
    }

    StcodePassRegInfo info{};

    auto processRegTable = [&](const auto &merged_vars_lookup_table, const StcodeRegisters &regs, HlslRegRange const_range,
                             auto &&name_mangler) {
      for (const auto &loc : regs.content)
      {
        if (loc.isNumeric)
        {
          const int reg = loc.reg - const_range.min;
          if (auto it = merged_vars_lookup_table.find(loc.name); it != merged_vars_lookup_table.end())
          {
            for (const auto &var : it->second)
              info.regsByName.emplace_back(name_mangler(var.name), reg * FLOAT4_REG_SIZE + var.offset);
          }
          else
            info.regsByName.emplace_back(name_mangler(loc.name), reg * FLOAT4_REG_SIZE);
        }
        else
          info.regsByName.emplace_back(name_mangler(loc.name), loc.reg);
      }
    };

    info.isCompute = p.cppstcode.cppStcode.isCompute;
    info.psOrCsCstRange = p.psOrCsConstRange;

    if (info.isCompute)
    {
      processRegTable(p.constPackedVarsMaps[STAGE_CS], p.cppstcode.cppStcode.pixelOrComputeRegs, p.psOrCsConstRange,
        [](const auto &str) { return str + stage_to_name_suffix(STAGE_CS); });
    }
    else
    {
      info.vsCstRange = p.vsConstRange;
      processRegTable(p.constPackedVarsMaps[STAGE_PS], p.cppstcode.cppStcode.pixelOrComputeRegs, p.psOrCsConstRange,
        [](const auto &str) { return str + stage_to_name_suffix(STAGE_PS); });
      processRegTable(p.constPackedVarsMaps[STAGE_VS], p.cppstcode.cppStcode.vertexRegs, p.vsConstRange,
        [](const auto &str) { return str + stage_to_name_suffix(STAGE_VS); });
    }

    passInfoTable[&p] = eastl::move(info);
  }

  return passInfoTable;
}

static auto fetch_unique_reg_names(const StcodePassRegInfoTable &table)
{
  eastl::vector_set<eastl::string> uniqueNames{};
  for (const auto &[pass, info] : table)
  {
    for (const auto &[name, _] : info.regsByName)
      uniqueNames.insert(name);
  }

  return uniqueNames;
}

StcodeBranchedRoutineDataTable build_pass_reg_tables_for_branched_dynstcode(const StcodePassRegInfoTable &passRegsTable,
  DynamicStcodeRoutine &branched_routine)
{
  RET_IF_SHOULD_NOT_COMPILE({});

  StcodeBranchedRoutineDataTable resTable{};

  auto uniqueNames = fetch_unique_reg_names(passRegsTable);

  for (const auto &[pass, info] : passRegsTable)
  {
    auto [it, ok] = resTable.passRegisterTables.emplace(pass, Tab<int>{});
    G_ASSERT(ok);
    auto &t = it->second;

    // Table is headed by 2 or 4 ints -- csRangeMin/csRangeSize or psRangeMin/psRangeSize and vsRangeMin/vsRangeSize
    const int headerSize = get_table_header_size(info.isCompute);
    t.resize(uniqueNames.size() + headerSize, -1);

    if (info.psOrCsCstRange.min < info.psOrCsCstRange.cap)
    {
      t[0] = info.psOrCsCstRange.min;
      t[1] = info.psOrCsCstRange.cap - info.psOrCsCstRange.min;
    }
    if (!info.isCompute && info.vsCstRange.min < info.vsCstRange.cap)
    {
      t[2] = info.vsCstRange.min;
      t[3] = info.vsCstRange.cap - info.vsCstRange.min;
    }
    dag::Span<int> passRegTable = make_span(t).subspan(headerSize);

    for (const auto &[name, reg] : info.regsByName)
      passRegTable[uniqueNames.find(name) - uniqueNames.begin()] = reg;

    resTable.commonVsConstRange = update_range(resTable.commonVsConstRange, info.vsCstRange);
    resTable.commonPsOrCsConstRange = update_range(resTable.commonPsOrCsConstRange, info.psOrCsCstRange);
  }

  branched_routine.pixelOrComputeRegs.clear();
  branched_routine.vertexRegs.clear();

  for (size_t i = 0; i < uniqueNames.size(); ++i)
  {
    const auto &nm = uniqueNames[i];
    auto it = nm.rbegin();
    if (*it == 's' && *(--it) == 'v')
      branched_routine.vertexRegs.add(nm.c_str(), i);
    else
      branched_routine.pixelOrComputeRegs.add(nm.c_str(), i);
  }

  return resTable;
}

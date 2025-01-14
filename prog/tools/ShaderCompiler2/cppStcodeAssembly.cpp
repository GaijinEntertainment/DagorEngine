// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cppStcodeAssembly.h"
#include "cppStcode.h"
#include "cppStcodeUtils.h"
#include "cppStcodeBuilder.h"
#include <debug/dag_assert.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>
#include <util/dag_string.h>
#include <util/dag_stlqsort.h>


template <class... TArgs>
static eastl::string string_f(const char *fmt, TArgs &&...args)
{
  return eastl::string(eastl::string::CtorSprintf{}, fmt, eastl::forward<TArgs>(args)...);
}

static void resolve_placeholder_at(eastl::string &templ, size_t at, const char *substitute)
{
  G_ASSERT(at < templ.length());
  const char *p = templ.begin() + at;

  size_t subsLen = strlen(substitute);
  eastl::string dst;
  dst.resize(templ.size() - 1 + subsLen);
  eastl::copy(templ.cbegin(), p, dst.begin());
  eastl::copy_n(substitute, subsLen, &dst.at(p - templ.begin()));
  eastl::copy(p + 1, templ.cend(), &dst.at((p - templ.begin()) + subsLen));

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
      G_ASSERT(!arg1 && !arg2 && !arg3 && !arg4);
      element = string_f("float4(%c, %c, %c, %c)", EXPR_ELEMENT_PLACEHOLDER, EXPR_ELEMENT_PLACEHOLDER, EXPR_ELEMENT_PLACEHOLDER,
        EXPR_ELEMENT_PLACEHOLDER);
      break;
    case ElementType::GLOBVAR:
    {
      G_ASSERT(arg1 && !arg3 && !arg4);
      eastl::string varDereference;
      if (strchr((const char *)arg1, '[')) // This means, that variable name is and index into an array (arr[i])
        varDereference = eastl::string((const char *)arg1);
      else
        varDereference = string_f("(*%s)", (const char *)arg1);
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
      G_ASSERT(arg1 && arg2 && !arg4);
      eastl::string typeCast = arg3 ? string_f("(%s)", (const char *)arg3) : "";
      element = string_f("(%c%sVAR(%s, %s))", EXPR_UNARY_OP_PLACEHOLDER, typeCast.c_str(), (const char *)arg1, (const char *)arg2);
    }
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
StcodeAccumulator StcodeRoutine::releaseAssembledCode(size_t routine_idx, size_t static_var_idx)
{
  RET_IF_SHOULD_NOT_COMPILE({});

  StcodeAccumulator result;
  result.headerStrings.emplaceBackFmt("ROUTINE_VISIBILITY void routine%u(const void *vars, bool is_compute);\n", routine_idx);

  if (static_var_idx != STATIC_VAR_ABSCENT_ID)
  {
    result.cppStrings.emplaceBackFmt("\n#ifdef CUR_STVAR_ID\n#undef CUR_STVAR_ID\n#endif\n\n#define CUR_STVAR_ID %u\n",
      static_var_idx);
  }

  if (code.empty())
    result.cppStrings.emplaceBackFmt("ROUTINE_VISIBILITY void routine%u(const void *, bool) {}\n\n", routine_idx);
  else
  {
    constexpr char cppRoutineTemplate[] =
#include "_stcodeTemplates/routine.cpp.inl.fmt"
      ;

    eastl::string locations = "";
    auto append_reg_locations = [](eastl::string &locations, const StcodeRegisters &stage_regs) {
      for (const auto &loc : stage_regs.content)
        locations.append_sprintf("    %s = %u,\n", loc.name.c_str(), loc.getRegNum());
    };

    append_reg_locations(locations, vertexRegs);
    append_reg_locations(locations, pixelRegs);

    result.cppStrings.emplaceBackFmt(cppRoutineTemplate, routine_idx, locations.c_str(), code.release().c_str());
  }

  return result;
}

static const char *stage_to_name_prefix(ShaderStage stage) { return stage == STAGE_VS ? "vs" : "pcs"; }
static const char *stage_to_runtime_const_str(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_CS:
    case STAGE_PS: return "pcs_stage";
    case STAGE_VS: return "STAGE_VS";
    default: G_ASSERT_RETURN(0, "");
  }
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

void StcodeRoutine::addShaderGlobMatrix(ShaderStage stage, GlobMatrixType type, const char *name, int id)
{
  RET_IF_SHOULD_NOT_COMPILE();

  const eastl::string location(eastl::string::CtorSprintf{}, "%s_%s", name, stage_to_name_prefix(stage));
  const eastl::string tempName(eastl::string::CtorSprintf{}, "tm_%s_%s", name, stage_to_name_prefix(stage));

  code.emplaceBackFmt("  float4x4 %s;\n", tempName.c_str());
  code.emplaceBackFmt("  %s(&%s);\n", glob_matrix_cb_name(type), tempName.c_str());
  code.pushBack(genSetConstStmt(stage, "float4x4", 4, location.c_str(), tempName.c_str(), 0));

  regsForStage(stage).add(location.c_str(), id);
}

void StcodeRoutine::addShaderGlobVec(ShaderStage stage, GlobVecType type, const char *name, int id, int compId)
{
  RET_IF_SHOULD_NOT_COMPILE();

  auto globVecCbName = [](GlobVecType type) {
    switch (type)
    {
      case GlobVecType::VIEW: return "get_lview";
      case GlobVecType::WORLD: return "get_lworld";
      default: G_ASSERT(0); return "";
    }
  };

  const eastl::string location(eastl::string::CtorSprintf{}, "%s_%s", name, stage_to_name_prefix(stage));
  const eastl::string tempName(eastl::string::CtorSprintf{}, "lv_%s_%s", name, stage_to_name_prefix(stage));

  code.emplaceBackFmt("  float4 %s;\n", tempName.c_str());
  code.emplaceBackFmt("  %s(%d, &%s);\n", globVecCbName(type), compId, tempName.c_str());
  code.pushBack(genSetConstStmt(stage, "float4", 1, location.c_str(), tempName.c_str(), 0));

  regsForStage(stage).add(location.c_str(), id);
}

eastl::string StcodeRoutine::genSetConstStmt(ShaderStage stage, const char *type, size_t type_reg_size, const char *location,
  const char *expr, int array_index)
{
  const eastl::string valStmt(eastl::string::CtorSprintf{}, "  %s %s__val%d = %s;\n", type, location, array_index, expr);

  const eastl::string loc(eastl::string::CtorSprintf{}, "LOC(%s)%s", location,
    array_index == 0 ? "" : string_f(" + %d", array_index).c_str());
  const eastl::string valPtr(eastl::string::CtorSprintf{}, "%s&%s__val%d", strcmp(type, "float4") == 0 ? "" : "(float4 *)", location,
    array_index);

  const eastl::string setCallStmt(eastl::string::CtorSprintf{}, "  set_const(%s, %s, %s, %llu);\n", stage_to_runtime_const_str(stage),
    loc.c_str(), valPtr.c_str(), type_reg_size);

  return valStmt + setCallStmt;
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

void StcodeRoutine::addGlobalShaderResource(ShaderStage stage, ResourceType type, const char *name, const char *var_name,
  bool var_is_global, int id)
{
  RET_IF_SHOULD_NOT_COMPILE();

  const eastl::string location(eastl::string::CtorSprintf{}, "%s_%s", name, stage_to_name_prefix(stage));

  // Directly stored resources (samplers and tlases) are passed to callbacks as void * to sampler handle
  const char *varDeref = (var_is_global && !resourceIsStoredDirectly(type)) ? "*" : "";
  const eastl::string varVal(eastl::string::CtorSprintf{}, "%s%s", varDeref, var_name);

  code.emplaceBackFmt("  %s(%s, LOC(%s), %s);\n", resource_cb_name(type), stage_to_runtime_const_str(stage), location.c_str(),
    varVal.c_str());
  regsForStage(stage).add(location.c_str(), id);
}

void StcodeRoutine::addShaderResource(ShaderStage stage, ResourceType type, const char *name, const char *var_name, int id)
{
  RET_IF_SHOULD_NOT_COMPILE();

  const char *varTypeName = nullptr;
  switch (type)
  {
    case ResourceType::TEXTURE: varTypeName = "uint"; break;
    case ResourceType::SAMPLER: varTypeName = "void *"; break;
    default: G_ASSERTF(0, "Invalid (or not yet implemented) type for shader resource: %d", (int)type);
  }

  const eastl::string location(eastl::string::CtorSprintf{}, "%s_%s", name, stage_to_name_prefix(stage));

  code.emplaceBackFmt("  %s(%s, LOC(%s), VAR(%s, %s));\n", resource_cb_name(type), stage_to_runtime_const_str(stage), location.c_str(),
    var_name, varTypeName);
  regsForStage(stage).add(location.c_str(), id);
}

void StcodeRoutine::addLocalVar(ShaderVarType type, const char *name, const char *val)
{
  RET_IF_SHOULD_NOT_COMPILE();
  code.emplaceBackFmt("  %s %s = %s;\n", stcode::shadervar_type_to_stcode_type(type), name, val);
}

void StcodeRoutine::addShaderConst(ShaderStage stage, ShaderVarType type, const char *name, int id, const char *val, int array_index)
{
  RET_IF_SHOULD_NOT_COMPILE();

  const char *typeName = stcode::shadervar_type_to_stcode_type(type);
  const eastl::string location(eastl::string::CtorSprintf{}, "%s_%s", name, stage_to_name_prefix(stage));

  code.pushBack(genSetConstStmt(stage, typeName, strstr(typeName, "4x4") ? 4 : 1, location.c_str(), val, array_index));

  if (array_index == 0)
    regsForStage(stage).add(location.c_str(), id);
}

void StcodeRoutine::addStmt(const char *val) { code.pushBack(string_f("%s;\n", val)); }

void StcodeRoutine::patchConstLocation(ShaderStage stage, int old_id, int new_id)
{
  RET_IF_SHOULD_NOT_COMPILE();
  regsForStage(stage).patch(old_id, new_id);
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
    case Type::MAIN_COLLECTION:
      fetchersOrFwdCpp.emplaceBackFmt("%s = (%s *)get_shadervar_ptr_from_dump(%d);\n", name, type_name, id_in_final_dump);
      break;

    case Type::REFERENCED_BY_SHADER: fetchersOrFwdCpp.emplaceBackFmt("extern %s;\n", varDecl.c_str()); break;
  }
}

StcodeGlobalVars::Strings StcodeGlobalVars::releaseAssembledCode()
{
  RET_IF_SHOULD_NOT_COMPILE({});

  return Strings{cppCode.release(), fetchersOrFwdCpp.release()};
}


/// StcodeShader ///
void StcodeShader::addStaticVarLocations(StcodeRegisters &&locations)
{
  RET_IF_SHOULD_NOT_COMPILE();

  if (locations.content.empty())
    return;

  StcodeBuilder offsetsCppStrings;
  offsetsCppStrings.emplaceBackFmt("enum class OffsetStvar%u : size_t\n{\n", curStvarId);
  for (const auto &loc : locations.content)
    offsetsCppStrings.emplaceBackFmt("  %s_ofs = %d,\n", loc.name.c_str(), loc.getRegNum());
  offsetsCppStrings.emplaceBack("};\n");

  code.cppStrings.pushFront(offsetsCppStrings.release());

  ++curStvarId;
}

void StcodeShader::addCode(StcodeRoutine &&routine, size_t routine_idx)
{
  RET_IF_SHOULD_NOT_COMPILE();

  hasCode = true;
  StcodeAccumulator res = routine.releaseAssembledCode(routine_idx, curStvarId);
  code.cppStrings.merge(eastl::move(res.cppStrings));
  code.headerStrings.merge(eastl::move(res.headerStrings));
}


/// StcodeInterface ///
static constexpr char routine_id_placeholder = '?';

void StcodeInterface::addRoutine(int global_routine_idx, int local_routine_idx, const char *fn)
{
  RET_IF_SHOULD_NOT_COMPILE();

  eastl::string filename = stcode::extract_shader_name_from_path(fn);

  if (routines.size() <= global_routine_idx)
    routines.resize(global_routine_idx + 1);

  routines[global_routine_idx] = {string_f("_%s::routine%u", filename.c_str(), local_routine_idx), global_routine_idx};
}

void StcodeInterface::patchRoutineGlobalId(int prev_id, int new_id)
{
  RET_IF_SHOULD_NOT_COMPILE();

  RoutineData &data = routines[prev_id];
  G_ASSERT(data.id == prev_id);
  data.id = new_id;
}

eastl::string StcodeInterface::releaseAssembledCode()
{
  RET_IF_SHOULD_NOT_COMPILE({});

  stlsort::sort(routines.begin(), routines.end(), [](const RoutineData &d1, const RoutineData &d2) {
    return d1.id != ROUTINE_NOT_PRESENT && (d2.id == ROUTINE_NOT_PRESENT || d1.id < d2.id);
  });

  StcodeBuilder pointerTableEntries;
  for (const RoutineData &routine : routines)
  {
    if (routine.id == ROUTINE_NOT_PRESENT)
      pointerTableEntries.pushBack("nullptr,\n");
    else
      pointerTableEntries.emplaceBackFmt("&%s,\n", routine.namespacedName.c_str());
  }
  routines.clear();

  return pointerTableEntries.release();
}

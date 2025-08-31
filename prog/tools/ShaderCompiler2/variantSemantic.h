// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shVariantContext.h"

namespace semantic
{

struct VarLookupRes
{
  int id;
  int valType;
  bool isGlobal;
};

VarLookupRes lookup_state_var(const char *name, shc::VariantContext &ctx, bool allow_not_found = false, const Terminal *t = nullptr);

inline VarLookupRes lookup_state_var(const Terminal &name_term, shc::VariantContext &ctx, bool allow_not_found = false)
{
  return lookup_state_var(name_term.text, ctx, allow_not_found, &name_term);
}

class VariantBoolExprEvalCB : public ShaderParser::ShaderBoolEvalCB
{
  String *curEvalExprErrStr = nullptr;
  shc::VariantContext &ctx;

public:
  explicit VariantBoolExprEvalCB(shc::VariantContext &a_ctx) : ctx{a_ctx} {}

  ShVarBool eval_expr(bool_expr &) override;
  ShVarBool eval_bool_value(bool_value &val) override;
  int eval_interval_value(const char *ival_name) override;
  void report_bool_eval_error(const char *error) override { curEvalExprErrStr->append(error); }
};

eastl::optional<ShaderStage> parse_state_block_stage(const char *stage_str);

#define TYPE_LIST_FLOAT() TYPE(f1) TYPE(f2) TYPE(f3) TYPE(f4) TYPE(f44)
#define TYPE_LIST_INT()   TYPE(i1) TYPE(i2) TYPE(i3) TYPE(i4)
#define TYPE_LIST_UINT()  TYPE(u1) TYPE(u2) TYPE(u3) TYPE(u4)
#define TYPE_LIST_BUF()   TYPE(buf) TYPE(cbuf)
#define TYPE_LIST_TEX()   TYPE(tex) TYPE(tex2d) TYPE(tex3d) TYPE(texArray) TYPE(texCube) TYPE(texCubeArray)
#define TYPE_LIST_SMP()   TYPE(smp) TYPE(smp2d) TYPE(smp3d) TYPE(smpArray) TYPE(smpCube) TYPE(smpCubeArray) TYPE(sampler)
#define TYPE_LIST_OTHER() \
  TYPE(shd) TYPE(shdArray) TYPE(staticCube) TYPE(staticTexArray) TYPE(staticTex3D) TYPE(staticCubeArray) TYPE(uav) TYPE(tlas)

#define TYPE_LIST TYPE_LIST_FLOAT() TYPE_LIST_INT() TYPE_LIST_UINT() TYPE_LIST_BUF() TYPE_LIST_TEX() TYPE_LIST_SMP() TYPE_LIST_OTHER()

enum class VariableType
{
  Unknown,
#define TYPE(type) type,
  TYPE_LIST
#undef TYPE
    staticSampler
};

inline constexpr bool vt_is_numeric(VariableType vt)
{
  switch (vt)
  {
#define TYPE(type) case VariableType::type:
    TYPE_LIST_INT()
    TYPE_LIST_UINT()
    TYPE_LIST_FLOAT()
    return true;

    TYPE_LIST_BUF()
    TYPE_LIST_TEX()
    TYPE_LIST_SMP()
    TYPE_LIST_OTHER()
    return false;
#undef TYPE
  }

  return false;
}

inline constexpr bool vt_is_integer(VariableType vt)
{
  switch (vt)
  {
#define TYPE(type) case VariableType::type:
    TYPE_LIST_INT()
    TYPE_LIST_UINT()
    return true;

    TYPE_LIST_FLOAT()
    TYPE_LIST_BUF()
    TYPE_LIST_TEX()
    TYPE_LIST_SMP()
    TYPE_LIST_OTHER()
    return false;
#undef TYPE
  }

  return false;
}

inline constexpr bool vt_is_sampled_texture(VariableType vt)
{
  switch (vt)
  {
#define TYPE(type) case VariableType::type:
    TYPE_LIST_SMP()
    return true;
#undef TYPE

    default: return false;
  }

  return false;
}

inline constexpr bool vt_is_static_texture(VariableType vt)
{
  return vt == VariableType::staticSampler || vt == VariableType::staticTex3D || vt == VariableType::staticCube ||
         vt == VariableType::staticCubeArray || vt == VariableType::staticTexArray;
}

inline constexpr int vt_float_size(VariableType vt)
{
  switch (vt)
  {
    case VariableType::f1:
    case VariableType::i1:
    case VariableType::u1: return 1;
    case VariableType::f2:
    case VariableType::i2:
    case VariableType::u2: return 2;
    case VariableType::f3:
    case VariableType::i3:
    case VariableType::u3: return 3;
    case VariableType::f4:
    case VariableType::i4:
    case VariableType::u4: return 4;
    case VariableType::f44: return 16;

    default: G_ASSERT_RETURN(0, 0);
  }
}

inline VariableType name_space_to_type(const char *name_space)
{
  static const eastl::vector_map<eastl::string_view, VariableType> var_types = {
#define TYPE(type) {"@" #type, VariableType::type},
    TYPE_LIST
#undef TYPE
    {"@static", VariableType::staticSampler}};

  auto found = var_types.find(name_space);
  if (found == var_types.end())
    return VariableType::Unknown;

  return found->second;
}

#undef TYPE_LIST_FLOAT
#undef TYPE_LIST_INT
#undef TYPE_LIST_UINT
#undef TYPE_LIST_BUF
#undef TYPE_LIST_TEX
#undef TYPE_LIST_SMP
#undef TYPE_LIST_OTHER
#undef TYPE_LIST

struct NamedConstInitializerElement
{
  using ArithmeticExpr = ShaderParser::ComplexExpression;
  using ArithmeticConst = Color4;
  struct BuiltinVar
  {
    int tokNum;
  };
  struct VarBase
  {
    int id;
    ShaderVarType type;
    const char *name;
  };
  struct MaterialVar : VarBase
  {};
  struct GlobalVar : VarBase
  {};

  eastl::variant<ArithmeticExpr, ArithmeticConst, GlobalVar, MaterialVar, BuiltinVar> value;
  Symbol *symbol;

  template <class T>
  NamedConstInitializerElement(T &&val, Symbol *symbol) : value{eastl::forward<T>(val)}, symbol{symbol}
  {}

  MOVE_ONLY_TYPE(NamedConstInitializerElement);

  bool isArithmExpr() const { return eastl::holds_alternative<ArithmeticExpr>(value); }
  bool isArithmConst() const { return eastl::holds_alternative<ArithmeticConst>(value); }
  bool isGlobalVar() const { return eastl::holds_alternative<GlobalVar>(value); }
  bool isMaterialVar() const { return eastl::holds_alternative<MaterialVar>(value); }
  bool isBuiltinVar() const { return eastl::holds_alternative<BuiltinVar>(value); }

  bool isShVar() const { return isGlobalVar() || isMaterialVar(); }

  const ArithmeticExpr &arithmExpr() const
  {
    G_ASSERT(isArithmExpr());
    return eastl::get<ArithmeticExpr>(value);
  }
  const ArithmeticConst &arithmConst() const
  {
    G_ASSERT(isArithmConst());
    return eastl::get<ArithmeticConst>(value);
  }
  int globVarId() const
  {
    G_ASSERT(isGlobalVar());
    return eastl::get<GlobalVar>(value).id;
  }
  int materialVarId() const
  {
    G_ASSERT(isMaterialVar());
    return eastl::get<MaterialVar>(value).id;
  }
  const char *varName() const
  {
    G_ASSERT(isGlobalVar() || isMaterialVar());
    return isGlobalVar() ? eastl::get<GlobalVar>(value).name : eastl::get<MaterialVar>(value).name;
  }
  ShaderVarType varType() const
  {
    G_ASSERT(isGlobalVar() || isMaterialVar());
    return isGlobalVar() ? eastl::get<GlobalVar>(value).type : eastl::get<MaterialVar>(value).type;
  }
  int builtinVarNum() const
  {
    G_ASSERT(isBuiltinVar());
    return eastl::get<BuiltinVar>(value).tokNum;
  }
};

// @TODO: maybe reorganize fields, since this thing is >cache line
struct NamedConstDefInfo
{
  const char *baseName = nullptr;
  String mangledName{};

  Tab<NamedConstInitializerElement> initializer{};
  int registerSize = 0;
  int arrayElemCount = 0;
  int hardcodedRegister = -1;

  VariableType type;
  ShaderVarType shvarType = SHVT_UNKNOWN;
  ShaderVarTextureType shvarTexType = SHVT_TEX_UNKNOWN;
  HlslRegisterSpace regSpace = HLSL_RSPACE_INVALID;
  ShaderStage stage;

  Terminal *varTerm = nullptr;
  Terminal *nameSpaceTerm = nullptr;
  Terminal *shaderVarTerm = nullptr;
  Terminal *builtinVarTerm = nullptr;
  Terminal *sizeVarTerm = nullptr;
  SHTOK_hlsl_text *hlsl = nullptr;

  int bindlessVarId = -1;

  sampler_decl *pairSamplerTmpDecl = nullptr;
  String pairSamplerName{};
  const char *pairSamplerBindSuffix = nullptr;

  Symbol *exprWithDynamicAndMaterialTerms = nullptr;

  bool isDynamic : 1 = false;
  bool isArray : 1 = false;
  bool isBindless : 1 = false;
  bool pairSamplerIsGlobal : 1 = false;
  bool pairSamplerIsShadow : 1 = false;
  bool hasDynStcodeRelyingOnMaterialParams : 1 = false;

  bool isHardcodedArray() const { return hardcodedRegister >= 0 && isArray; }
  bool isRegularArray() const { return isArray && !isHardcodedArray(); }
};

inline VariableType parse_named_const_type(const state_block_stat &state_block)
{
  SHTOK_type_ident *ns = nullptr;
  if (state_block.var)
    ns = state_block.var->var->nameSpace;
  else if (state_block.arr)
    ns = state_block.arr->var->nameSpace;
  else if (state_block.reg)
    ns = state_block.reg->var->nameSpace;
  else if (state_block.reg_arr)
    ns = state_block.reg_arr->var->nameSpace;
  return name_space_to_type(ns->text);
}

eastl::optional<NamedConstDefInfo> parse_named_const_definition(const state_block_stat &state_block, ShaderStage stage,
  VariableType vt, shc::VariantContext &ctx, IMemAlloc *tmp_memory = nullptr);

struct LocalVarDefInfo
{
  LocalVar *var;
  eastl::unique_ptr<ShaderParser::ComplexExpression> expr;
};

eastl::optional<LocalVarDefInfo> parse_local_var_decl(local_var_decl &decl, shc::VariantContext &ctx,
  bool ignore_color_dimension_mismatch = false, bool allow_override = false);

} // namespace semantic

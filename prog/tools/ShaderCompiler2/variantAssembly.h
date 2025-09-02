// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shVariantContext.h"
#include "variantSemantic.h"
#include "shExpr.h"


namespace assembly
{

// @TODO: these should be a functions that build generally, however for now constexpr flags will be enough

using StcodeBuildFlags = uint32_t;
struct StcodeBuildFlagsBits
{
  static constexpr StcodeBuildFlags BYTECODE = 1;
  static constexpr StcodeBuildFlags CPP = 1 << 1;

  static constexpr StcodeBuildFlags ALL = BYTECODE | CPP;
};

template <StcodeBuildFlags FLAGS>
void assemble_local_var(LocalVar *var, ShaderParser::ComplexExpression *rootExpr, ShaderTerminal::SHTOK_ident *decl_name,
  shc::VariantContext &ctx);

struct NamedConstDeclarationHlsl
{
  String definition;
  String postfix;
};

eastl::optional<NamedConstDeclarationHlsl> build_hlsl_decl_for_named_const(const semantic::NamedConstDefInfo &def,
  shc::VariantContext &ctx, int dest_register, const ShaderParser::VariablesMerger &var_merger);

template <StcodeBuildFlags FLAGS>
bool build_stcode_for_named_const(const semantic::NamedConstDefInfo &def, int dest_register, shc::VariantContext &ctx,
  IMemAlloc *tmp_memory = nullptr, bool add_sampler_vars = true);

String build_hlsl_for_pair_sampler(const char *const_name, bool is_shadow, int dest_register, shc::VariantContext &ctx);

template <StcodeBuildFlags FLAGS>
void build_stcode_for_pair_sampler(const char *const_name, const char *var_name, int dest_reg, ShaderStage stage, int var_id,
  bool is_global, shc::VariantContext &ctx);

void build_cpp_declarations_for_used_local_vars(shc::VariantContext &ctx);
void build_cpp_declarations_for_used_bool_vars(shc::VariantContext &ctx);

} // namespace assembly

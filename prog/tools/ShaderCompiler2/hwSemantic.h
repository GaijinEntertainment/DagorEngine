// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shCompContext.h"
#include "shlexterm.h"
#include "hlslStage.h"
#include "hlslRegisters.h"
#include "shsyn.h"
#include "EASTL/variant.h"


namespace semantic
{

bool compare_hw_token(int tok, const shc::CompilationContext &ctx);

inline bool compare_hw_token(ShaderTerminal::bool_value &hw_bool, const shc::CompilationContext &ctx)
{
  G_ASSERT(hw_bool.hw);
  return compare_hw_token(hw_bool.hw->var->num, ctx);
}

struct HlslCompileDirective
{
  String profile, entry;
  bool useHalfs = false;
  bool useDefaultProfileIfExists = false;
};

struct HlslSetDefaultTargetDirective
{
  String target;
};

struct HlslNullCompileDirective
{};

struct HlslCompileClass
{
  eastl::variant<HlslCompileDirective, HlslSetDefaultTargetDirective, HlslNullCompileDirective> directive;
  HlslCompilationStage stage;

  HlslCompileClass() : directive{HlslNullCompileDirective{}}, stage{HLSL_PS} {}
  HlslCompileClass(auto &&directive, HlslCompilationStage stage) : directive{eastl::move(directive)}, stage{stage} {}

  HlslCompileDirective *tryGetCompile() { return eastl::get_if<HlslCompileDirective>(&directive); }
  HlslSetDefaultTargetDirective *tryGetDefaultTarget() { return eastl::get_if<HlslSetDefaultTargetDirective>(&directive); }
  bool isNullCompile() const { return eastl::get_if<HlslNullCompileDirective>(&directive) != nullptr; }
};

HlslCompileClass parse_hlsl_compilation_info(ShaderTerminal::hlsl_compile_class &hlsl_compile, Parser &parser,
  const shc::CompilationContext &ctx);

bool validate_hardcoded_regs_in_hlsl_block(const ShaderTerminal::SHTOK_hlsl_text *hlsl);

} // namespace semantic
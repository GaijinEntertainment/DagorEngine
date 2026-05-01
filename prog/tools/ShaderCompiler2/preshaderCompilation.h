// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsyn.h"
#include "const3d.h"
#include "shaderTab.h"
#include "shErrorReporting.h"
#include "namedConst.h"
#include "cppStcodeAssembly.h"
#include "variablesMerger.h"
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/variant.h>
#include <ska_hash_map/flat_hash_map2.hpp>

namespace semantic
{
enum class VariableType; // Forward-declared so as not to create a dependecy on variant semantic
}

namespace shc
{
class VariantContext;
}

struct PreshaderStat
{
  ShaderTerminal::state_block_stat *stat;
  ShaderStage stage;
  semantic::VariableType vt;
};

inline bool operator==(const PreshaderStat &s1, const PreshaderStat &s2)
{
  return s1.stat == s2.stat && s1.stage == s2.stage && s1.vt == s2.vt;
}

struct PreshaderCompilationInput
{
  Tab<ShaderTerminal::supports_stat *> supportStats{};
  Tab<eastl::variant<PreshaderStat, ShaderTerminal::local_var_decl *>> scalarStats{};
  Tab<PreshaderStat> staticTextureStats{};
  Tab<PreshaderStat> dynamicResourceStats{};
  Tab<PreshaderStat> hardcodedStats{};
  bool isCompute = false;

  // @NOTE: this is not used for codegen. However, this is required for unique identification, as different var maps lead to different
  // bytecodes, and they only consolidate at eval end (and we want caching while it is ongoing)
  Tab<ShaderTerminal::static_var_decl *> staticVarDecls{};
};

inline bool operator==(const PreshaderCompilationInput &i1, const PreshaderCompilationInput &i2)
{
  if (i1.supportStats.size() != i2.supportStats.size() || i1.scalarStats.size() != i2.scalarStats.size() ||
      i1.staticTextureStats.size() != i2.staticTextureStats.size() ||
      i1.dynamicResourceStats.size() != i2.dynamicResourceStats.size() || i1.hardcodedStats.size() != i2.hardcodedStats.size() ||
      i1.staticVarDecls.size() != i2.staticVarDecls.size() || i1.isCompute != i2.isCompute)
  {
    return false;
  }

  return i1.supportStats == i2.supportStats && i1.scalarStats == i2.scalarStats && i1.staticTextureStats == i2.staticTextureStats &&
         i1.dynamicResourceStats == i2.dynamicResourceStats && i1.hardcodedStats == i2.hardcodedStats &&
         i1.staticVarDecls == i2.staticVarDecls;
}

namespace eastl
{

template <>
struct hash<PreshaderStat>
{
  size_t operator()(const PreshaderStat &stat) const noexcept
  {
    size_t h = 0;
    hash_into(h, uintptr_t(stat.stat));
    hash_into(h, int(stat.stage));
    hash_into(h, stat.vt);
    return h;
  }
};

template <>
struct hash<PreshaderCompilationInput>
{
  size_t operator()(const PreshaderCompilationInput &inp) const noexcept
  {
    constexpr size_t MAGIC = 0xFADEDEED;
    size_t h = 0;
    hash_into(h, inp.isCompute);
    hash_into(h, MAGIC);
    for (const auto *s : inp.supportStats)
      hash_into(h, uintptr_t(s));
    hash_into(h, MAGIC);
    for (const auto &s : inp.hardcodedStats)
      hash_into(h, s);
    hash_into(h, MAGIC);
    for (const auto &v : inp.scalarStats)
    {
      hash_into(h, v.index());
      eastl::visit(
        [&h](const auto &v) {
          if constexpr (eastl::is_same_v<eastl::decay_t<decltype(v)>, PreshaderStat>)
            hash_into(h, v);
          else
            hash_into(h, uintptr_t(v));
        },
        v);
    }
    hash_into(h, MAGIC);
    for (const auto &s : inp.staticTextureStats)
      hash_into(h, s);
    hash_into(h, MAGIC);
    for (const auto &s : inp.dynamicResourceStats)
      hash_into(h, s);
    hash_into(h, MAGIC);
    for (const auto *s : inp.staticVarDecls)
      hash_into(h, uintptr_t(s));
    return h;
  }
};

} // namespace eastl

struct CompiledPreshader
{
  NamedConstBlock namedConstTable;
  StcodePass cppStcode{};
  TabStcode dynStBytecode, stBlkBytecode;
  ShaderParser::VariablesMerger::MergedVarsMapsPerStage constPackedVarsMaps;
  ShaderParser::VariablesMerger::MergedVarsMap bufferedPackedVarsMap;

  Tab<uintptr_t> usedPreshaderStatements{};
  Tab<uintptr_t> preshaderVarsNeedingSamplerAsm{};

  Tab<Terminal *> stcodeVars;

  bool isCompute;
  int requiredRegCount;

  Tab<String> dynamicSamplerImplicitVars;
  Tab<int> usedMaterialVarIds;

  Tab<ShaderVarTextureType> shadervarTexTypes;

  int stblkcodeNo = -1;
  int stcodeNo = -1;
  int cppStblkcodeId = -1;
  int cppStcodeId = -1;
  // true means cppStcode, dynStBytecode and stBlkBytecode have been flushed to target caches,
  // and instead can be referred to by the ids above
  bool postedToTarget = false;
};

struct PreshaderCompilationOutput
{
  CompiledPreshader code;
};

eastl::optional<PreshaderCompilationOutput> compile_variant_preshader(const PreshaderCompilationInput &input,
  shc::VariantContext &vctx);

class PreshaderCompilationCache
{
  ska::flat_hash_map<PreshaderCompilationInput, eastl::unique_ptr<CompiledPreshader>> cache;

public:
  CompiledPreshader *query(const PreshaderCompilationInput &input) const
  {
    if (auto it = cache.find(input); it != cache.end())
      return it->second.get();
    return nullptr;
  }

  CompiledPreshader *post(PreshaderCompilationInput &&input, CompiledPreshader &&code)
  {
    auto [it, ok] = cache.emplace(eastl::move(input), eastl::make_unique<CompiledPreshader>(eastl::move(code)));
    G_ASSERT(ok);
    return it->second.get();
  }
};

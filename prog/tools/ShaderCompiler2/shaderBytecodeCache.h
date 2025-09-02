// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hash.h"
#include "hlslStage.h"
#include "compileResult.h"
#include "commonUtils.h"
#include <generic/dag_tab.h>
#include <memory/dag_memBase.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>

class SemanticShaderPass;

enum class ShaderCacheIndex : int32_t
{
  EMPTY = -1,
  PENDING = -2,
};

struct ShaderCacheLevelIds
{
  int l1, l2, l3;
};

inline bool is_valid(ShaderCacheIndex id)
{
  G_ASSERT(eastl::to_underlying(id) >= eastl::to_underlying(ShaderCacheIndex::PENDING));
  return id != ShaderCacheIndex::EMPTY && id != ShaderCacheIndex::PENDING;
}

struct ShaderCacheEntry
{
  struct Level2
  {
    struct Level3
    {
      SimpleString entry, profile;
      ShaderCacheIndex codeIdx;
    };

    CryptoHash constDigest;
    Tab<Level3> lv3;

    Level2() : lv3(midmem) {}

    int addEntry(const char *entry, const char *profile)
    {
      Level3 &l3 = lv3.push_back();
      l3.codeIdx = ShaderCacheIndex::EMPTY;
      l3.entry = entry;
      l3.profile = profile;
      return lv3.size() - 1;
    }
  };

  CryptoHash codeDigest;
  Tab<Level2> lv2;

  ShaderCacheEntry() : lv2(midmem) {}

  int addEntry(const CryptoHash &const_digest, const char *entry, const char *profile)
  {
    Level2 &l2 = lv2.push_back();
    l2.constDigest = const_digest;
    l2.addEntry(entry, profile);
    return lv2.size() - 1;
  }
};

struct CachedShader
{
  HlslCompilationStage codeType;
  Tab<unsigned> relCode;
  ComputeShaderInfo computeShaderInfo;
  String compileCtx;
  CachedShader() : relCode(midmem_ptr()), codeType(HLSL_INVALID) {}

  dag::ConstSpan<unsigned> getShaderOutCode(HlslCompilationStage type) const
  {
    return codeType == type ? relCode : dag::ConstSpan<unsigned>();
  }

  const ComputeShaderInfo &getComputeShaderInfo() const
  {
    G_ASSERT(codeType == HLSL_CS || codeType == HLSL_MS || codeType == HLSL_AS);
    return computeShaderInfo;
  }
};

class ShaderBytecodeCache
{
  Tab<ShaderCacheEntry> cacheEntries{midmem_ptr()};
  Tab<CachedShader> cachedShadersList{midmem_ptr()};
  PerHlslStage<int> codeCounts{};

  Tab<SemanticShaderPass *> pendingShaderPasses{midmem_ptr()};

  static inline const CachedShader EMPTY_CACHED_SHADER = {};

public:
  ShaderBytecodeCache() = default;
  ~ShaderBytecodeCache() { debug("[stat] cached_shaders_list.count=%d", cachedShadersList.size()); }
  PINNED_TYPE(ShaderBytecodeCache);

  struct LookupResult
  {
    ShaderCacheIndex id;
    ShaderCacheLevelIds c;
  };

  bool post(ShaderCacheLevelIds c, const CompileResult &result, HlslCompilationStage stage, const String &compile_ctx);
  LookupResult find(const CryptoHash &code_digest, const CryptoHash &const_digest, const char *entry, const char *profile);

  const CachedShader &resolveEntry(ShaderCacheLevelIds c) const
  {
    ShaderCacheIndex idx = cacheEntries[c.l1].lv2[c.l2].lv3[c.l3].codeIdx;
    return is_valid(idx) ? cachedShadersList[eastl::to_underlying(idx)] : EMPTY_CACHED_SHADER;
  }

  void markEntryAsPending(ShaderCacheLevelIds c) { cacheEntries[c.l1].lv2[c.l2].lv3[c.l3].codeIdx = ShaderCacheIndex::PENDING; }

  void resolvePendingPasses();
  void registerPendingPass(SemanticShaderPass &p);
};

void apply_shader_from_cache(SemanticShaderPass &pass, HlslCompilationStage stage, ShaderCacheLevelIds c,
  const ShaderBytecodeCache &cache);
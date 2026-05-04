// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shaderBytecodeCache.h"
#include "globalConfig.h"
#include "shSemCode.h"

// @TODO: this probably is used in other places, may be better to pull out
static constexpr int HLSL_CACHE_STAMPS[HLSL_COUNT] = {
  _MAKE4C('DX9p'), // HLSL_PS
  _MAKE4C('DX9v'), // HLSL_VS
  _MAKE4C('D11c'), // HLSL_CS
  _MAKE4C('D11d'), // HLSL_DS
  _MAKE4C('D11h'), // HLSL_HS
  _MAKE4C('D11g'), // HLSL_GS
  _MAKE4C('D11v'), // HLSL_MS - aliased with vs
  _MAKE4C('D11g'), // HLSL_AS - aliased with gs
};

static bool is_in_cache(const CachedShader &c, const CompileResult &r)
{
  if (r.metadata.size() + 4 != c.relMetadata.size() || !eastl::equal(r.metadata.begin(), r.metadata.end(), c.relMetadata.data() + 4))
    return false;

  size_t size_in_unsigned = (r.bytecode.size() + 3) / sizeof(unsigned);
  if (size_in_unsigned != c.relCode.size() || !eastl::equal(r.bytecode.begin(), r.bytecode.end(), (const uint8_t *)c.relCode.data()))
    return false;

  return true;
}

bool ShaderBytecodeCache::post(ShaderCacheLevelIds c, const CompileResult &result, HlslCompilationStage stage,
  const String &compile_ctx)
{
  size_t size_in_unsigned = (result.bytecode.size() + 3) / sizeof(unsigned);
  bool added_new = false;
  ShaderCacheIndex codeIdx = ShaderCacheIndex::EMPTY;
  for (int i = cachedShadersList.size() - 1; i >= 0; i--)
    if (cachedShadersList[i].codeType == stage && is_in_cache(cachedShadersList[i], result))
    {
      codeIdx = ShaderCacheIndex{i};
      break;
    }

  // Store new shader
  if (codeIdx == ShaderCacheIndex::EMPTY)
  {
    codeIdx = ShaderCacheIndex(append_items(cachedShadersList, 1));
    CachedShader &cachedShader = cachedShadersList[eastl::to_underlying(codeIdx)];

    cachedShader.relMetadata.resize(result.metadata.size() + 4);
    memcpy(cachedShader.relMetadata.data(), &HLSL_CACHE_STAMPS[stage], sizeof(HLSL_CACHE_STAMPS[stage]));
    eastl::copy(result.metadata.begin(), result.metadata.end(), (uint8_t *)&cachedShader.relMetadata[4]);

    cachedShader.relCode.resize(size_in_unsigned);
    cachedShader.codeType = stage;
    if (shc::config().validateIdenticalBytecode)
      cachedShader.compileCtx = compile_ctx;
    ++codeCounts.all[stage];
    if (item_is_in(stage, {HLSL_CS, HLSL_AS, HLSL_MS}))
      cachedShader.computeShaderInfo = result.computeShaderInfo;
    eastl::copy(result.bytecode.begin(), result.bytecode.end(), (uint8_t *)&cachedShader.relCode[0]);
    for (size_t i = result.bytecode.size(); i < size_in_unsigned * sizeof(unsigned); ++i)
      ((uint8_t *)cachedShader.relCode.data())[i] = 0;
    added_new = true;
  }

  cacheEntries[c.l1].lv2[c.l2].lv3[c.l3].codeIdx = codeIdx;
  return added_new;
}

ShaderBytecodeCache::LookupResult ShaderBytecodeCache::find(const CryptoHash &code_digest, const CryptoHash &const_digest,
  const char *entry, const char *profile)
{
  ShaderBytecodeCache::LookupResult res{};
  for (int i = int(cacheEntries.size() - 1); i >= 0; i--)
    if (cacheEntries[i].codeDigest == code_digest)
    {
      ShaderCacheEntry &e = cacheEntries[i];
      for (int j = e.lv2.size() - 1; j >= 0; j--)
        if (e.lv2[j].constDigest == const_digest)
        {
          ShaderCacheEntry::Level2 &l2 = e.lv2[j];
          for (int k = l2.lv3.size() - 1; k >= 0; k--)
            if (l2.lv3[k].entry == entry && l2.lv3[k].profile == profile)
              return {l2.lv3[k].codeIdx, {i, j, k}};

          return {ShaderCacheIndex::EMPTY, {i, j, l2.addEntry(entry, profile)}};
        }
      return {ShaderCacheIndex::EMPTY, {i, e.addEntry(const_digest, entry, profile), 0}};
    }

  ShaderCacheEntry &e = cacheEntries.push_back();
  e.codeDigest = code_digest;
  return {ShaderCacheIndex::EMPTY, {int(cacheEntries.size() - 1), e.addEntry(const_digest, entry, profile), 0}};
}

void ShaderBytecodeCache::resolvePendingPasses()
{
  for (SemanticShaderPass *p : pendingShaderPasses)
  {
    eastl::optional<ShaderCacheLevelIds> entryIdsMaybe;
    for (HlslCompilationStage stage : HLSL_ALL_LIST)
    {
      if ((entryIdsMaybe = p->getCidx(stage)).has_value())
        apply_shader_from_cache(*p, stage, entryIdsMaybe.value(), *this);
    }
  }
  pendingShaderPasses.clear();
}

void ShaderBytecodeCache::registerPendingPass(SemanticShaderPass &p)
{
  if (!pendingShaderPasses.size() || pendingShaderPasses.back() != &p)
    pendingShaderPasses.push_back(&p);
}

void apply_shader_from_cache(SemanticShaderPass &pass, HlslCompilationStage stage, ShaderCacheLevelIds c,
  const ShaderBytecodeCache &cache)
{
  auto &cachedShader = cache.resolveEntry(c);
  ShaderStageData *dst[HLSL_COUNT] = {&pass.fsh, &pass.vpr, &pass.cs, &pass.ds, &pass.hs, &pass.gs, &pass.vpr, &pass.gs};
  dst[stage]->bytecode = cachedShader.getShaderOutCode(stage);
  dst[stage]->metadata = cachedShader.getShaderOutMetadata(stage);
  if (item_is_in(stage, {HLSL_CS, HLSL_AS, HLSL_MS}))
  {
    pass.threadGroupSizes[0] = cachedShader.getComputeShaderInfo().threadGroupSizeX;
    pass.threadGroupSizes[1] = cachedShader.getComputeShaderInfo().threadGroupSizeY;
    pass.threadGroupSizes[2] = cachedShader.getComputeShaderInfo().threadGroupSizeZ;
#if _CROSS_TARGET_DX12
    pass.scarlettWave32 = cachedShader.getComputeShaderInfo().scarlettWave32;
#endif
  }
}

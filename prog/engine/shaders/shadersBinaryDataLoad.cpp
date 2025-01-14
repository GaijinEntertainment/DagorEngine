// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shadersBinaryData.h"
#include "mapBinarySearch.h"
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_dynVariantsCache.h>
#include <ioSys/dag_zstdIo.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_dataBlock.h>
#include <string.h>
#include <shaders/shader_ver.h>
#include <shaders/dag_linkedListOfShadervars.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_commands.h>

namespace shaderbindump
{
static uint32_t generation = 0;
uint32_t get_generation() { return generation; }
static dag::Vector<shaderbindump::VariantTable::IntervalBind> intervalBinds;
struct IntervalBindRange
{
  uint16_t start;
  uint16_t count;
};
static dag::Vector<IntervalBindRange> intervalBindRanges;
static OSSpinlock mutex;

uint32_t get_dynvariant_collection_id(const shaderbindump::ShaderCode &code)
{
  OSSpinlockScopedLock lock(mutex);
  if (intervalBindRanges.empty())
    intervalBindRanges.push_back(IntervalBindRange{0, 0});
  const auto &codePieces = code.dynVariants.codePieces;
  uint32_t collectionSize = codePieces.size();
  if (collectionSize == 0)
    return 0;
  int intervalRangeStart = -1;
  if (collectionSize <= intervalBinds.size())
  {
    for (uint32_t i = 0, ie = intervalBinds.size() - collectionSize; i < ie && intervalRangeStart == -1; ++i)
    {
      bool matched = true;
      for (uint32_t j = 0, je = collectionSize; j < je && matched; ++j)
        matched =
          (codePieces[j].intervalId == intervalBinds[i + j].intervalId && codePieces[j].totalMul == intervalBinds[i + j].totalMul);
      if (matched)
        intervalRangeStart = i;
    }
  }
  if (intervalRangeStart == -1)
  {
    intervalRangeStart = intervalBinds.size();
    intervalBinds.insert(intervalBinds.end(), codePieces.begin(), codePieces.end());
  }
  int intervalRangeId = -1;
  for (uint32_t i = 0, ie = intervalBindRanges.size(); i < ie && intervalRangeId == -1; ++i)
  {
    if (intervalBindRanges[i].start == intervalRangeStart && intervalBindRanges[i].count == collectionSize)
      intervalRangeId = i;
  }
  if (intervalRangeId == -1)
  {
    intervalRangeId = intervalBindRanges.size();
    intervalBindRanges.emplace_back(IntervalBindRange{(uint16_t)intervalRangeStart, (uint16_t)collectionSize});
  }
  return intervalRangeId;
}

template <typename T>
inline void build_dynvariant_collection_cache_impl(T &cache)
{
  const auto &sh = shBinDumpOwner();
  OSSpinlockScopedLock lock(mutex);
  cache.resize_noinit(intervalBindRanges.size());
  int *p = cache.data();
  for (const auto &r : intervalBindRanges)
  {
    int v = 0;
    for (uint32_t j = r.start, je = j + r.count; j < je; ++j)
      v += sh.globIntervalNormValues[intervalBinds[j].intervalId] * intervalBinds[j].totalMul;
    *p++ = v;
  }
}

void build_dynvariant_collection_cache(dag::Vector<int, framemem_allocator> &cache) { build_dynvariant_collection_cache_impl(cache); }

void build_dynvariant_collection_cache(dag::Vector<int> &cache) { build_dynvariant_collection_cache_impl(cache); }
} // namespace shaderbindump

template <typename A>
DynVariantsCache<A>::DynVariantsCache()
{
  shaderbindump::build_dynvariant_collection_cache(cache);
}
template class DynVariantsCache<framemem_allocator>;
template class DynVariantsCache<EASTLAllocatorType>;

template <typename F>
static bool read_shdump_file(IGenLoad &crd, int size, bool full_file_load, F cb)
{
#if _TARGET_C1 | _TARGET_C2


#else
  if (full_file_load) // mmaped code path
#endif
  {
    G_ASSERT(crd.tell() == 0);
    int flen = -1;
    auto fdata = (const uint8_t *)df_mmap(static_cast<FullFileLoadCB &>(crd).fileHandle, &flen);
    if (!fdata || flen != size || !cb(fdata, size))
    {
      df_unmap(fdata, flen);
      return false;
    }
    df_unmap(fdata, flen);
  }
  else // generic path with memcopy
  {
    Tab<uint8_t> dump(size);
    if (crd.tryRead(dump.data(), size) != size || !cb(dump.data(), size))
      return false;
  }
  return true;
}

bool ScriptedShadersBinDumpOwner::load(IGenLoad &crd, int size, bool full_file_load)
{
  clear();

  if (!read_shdump_file(crd, size, full_file_load, [&](const uint8_t *data, int size_) { return loadData(data, size_); }))
    return false;

  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByName("graphics");
  const size_t maxBindumpShadervars =
    graphicsBlk ? graphicsBlk->getInt("max_bindump_shadervars", DEFAULT_MAX_SHVARS) : DEFAULT_MAX_SHVARS;

  G_ASSERTF(mShaderDump->varMap.size() <= maxBindumpShadervars,
    "Total number of shadervars (%d) exceeds the max allowed for mapping (%d). "
    "Remove redundant shadervars, or increase graphics/max_bindump_shadervars in the config",
    mShaderDump->varMap.size(), maxBindumpShadervars);

  initVarIndexMaps(maxBindumpShadervars);
  shadervars::resolve_shadervars();
  ShaderVariableInfo::resolveAll();
  if (mShaderDump == &shBinDump())
    d3d::driver_command(Drv3dCommand::REGISTER_SHADER_DUMP, this, (void *)crd.getTargetName());

  mInitialDataHash = mem_hash_fnv1<64>((char *)mSelfData.data(), mSelfData.size());
  return true;
}

bool ScriptedShadersBinDumpOwner::loadData(const uint8_t *dump, int size)
{
  clear();

  auto header = bindump::map<shader_layout::ScriptedShadersBinDumpCompressed>(dump);
  if (!header)
    return false;

  if (header->signPart1 != _MAKE4C('VSPS') || header->signPart2 != _MAKE4C('dump') || header->version != SHADER_BINDUMP_VER)
    return false;

  mShaderDump = header->scriptedShadersBindumpCompressed.decompress(mSelfData, bindump::BehaviorOnUncompressed::Copy);
  if (!mShaderDump)
    return false;

  if (mShaderDump == &shBinDump())
  {
    initAfterLoad();
    mDictionary = eastl::unique_ptr<ZSTD_DDict_s, ZstdDictionaryDeleter>(
      zstd_create_ddict(dag::ConstSpan<char>(mShaderDump->dictionary.data(), mShaderDump->dictionary.size()), true));
  }

  mShaderDumpV2 = bindump::map<shader_layout::ScriptedShadersBinDumpV2>(mSelfData.data());
  mShaderDumpV3 = bindump::map<shader_layout::ScriptedShadersBinDumpV3>(mSelfData.data());

  uint64_t cache_size_in_mb = 0;
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  cache_size_in_mb = graphicsBlk->getInt("decompressed_shaders_cache_size_in_mb", 0);

  uint64_t decompressed_size = mShaderDump->shGroups.empty() ? 0 : mShaderDump->shGroups[0].getDecompressedSize();
  uint64_t cache_capacity =
    cache_size_in_mb && decompressed_size ? eastl::max((uint64_t)1, (cache_size_in_mb << 20) / decompressed_size) : 1;

  mDecompressedGropusLru = eastl::make_unique<decompressed_groups_cache_t>(cache_capacity);

  return true;
}

void ScriptedShadersBinDumpOwner::copyDecompressedShader(const DecompressedGroup &group, uint16_t index_in_group,
  ShaderBytecode &tmpbuf)
{
  auto &shader = group.sh_group->shaders[index_in_group];
  clear_and_resize(tmpbuf, (shader.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t));
  eastl::copy(shader.begin(), shader.end(), (uint8_t *)tmpbuf.data());
}

void ScriptedShadersBinDumpOwner::loadDecompressedShader(uint16_t group_id, uint16_t index_in_group, ShaderBytecode &tmpbuf)
{
  OSSpinlockScopedLock lock(mDecompressedGroupsLruMutex);
  // Note: .get() is actually inserting new empty data for missing key
  const DecompressedGroup &group = mDecompressedGropusLru->get(group_id);
  if (!group.decompressed_data.empty())
    copyDecompressedShader(group, index_in_group, tmpbuf);
}

void ScriptedShadersBinDumpOwner::storeDecompressedGroup(uint16_t group_id, DecompressedGroup &&decompressed_group)
{
  OSSpinlockScopedLock lock(mDecompressedGroupsLruMutex);
  DecompressedGroup &group = mDecompressedGropusLru->get(group_id);
  group = eastl::move(decompressed_group);
}

dag::ConstSpan<uint32_t> ScriptedShadersBinDumpOwner::getCode(uint32_t id, ShaderCodeType type, ShaderBytecode &tmpbuf)
{
  TIME_PROFILE(decompress_shader);

  id += type == ShaderCodeType::VERTEX ? 0 : vprId.size();
  auto &sh_index = mShaderDump->shGroupsMapping[id];

  tmpbuf.clear();
  loadDecompressedShader(sh_index.groupId, sh_index.indexInGroup, tmpbuf);
  if (tmpbuf.empty())
  {
    DecompressedGroup group;
    group.sh_group = mShaderDump->shGroups[sh_index.groupId].decompress(group.decompressed_data, bindump::BehaviorOnUncompressed::Copy,
      mDictionary.get());
    copyDecompressedShader(group, sh_index.indexInGroup, tmpbuf);
    storeDecompressedGroup(sh_index.groupId, eastl::move(group));
  }
  return make_span_const(tmpbuf);
}

void ScriptedShadersBinDumpOwner::initAfterLoad()
{
  shaderbindump::generation++;
  // init block state
  shaderbindump::blockStateWord = 0;
  static const char *null_name[shaderbindump::MAX_BLOCK_LAYERS] = {"!frame", "!scene", "!obj"};
  for (int i = 0; i < shaderbindump::MAX_BLOCK_LAYERS; i++)
  {
    int id = mapbinarysearch::bfindStrId(mShaderDump->blockNameMap, null_name[i]);
    if (id < 0)
      shaderbindump::nullBlock[i] = nullptr;
    else
    {
      shaderbindump::nullBlock[i] = &mShaderDump->blocks[id];
      shaderbindump::blockStateWord |= shaderbindump::nullBlock[i]->uidVal;
    }
  }

  G_ASSERT(mShaderDump->globVars.v.size() < 0x2000);
  G_ASSERT(mShaderDump->varMap.size() < 0x2000);

  clear_and_resize(globVarIntervalIdx, mShaderDump->globVars.v.size());
  mem_set_ff(globVarIntervalIdx);
  clear_and_resize(globIntervalNormValues, mShaderDump->intervals.size());
  mem_set_0(globIntervalNormValues);
  for (int i = 0; i < mShaderDump->intervals.size(); i++)
  {
    if (mShaderDump->intervals[i].type == mShaderDump->intervals[i].TYPE_GLOBAL_INTERVAL)
    {
      int gvid = bfind_packed_uint16_x2(mShaderDump->gvMap, mShaderDump->intervals[i].nameId);
      float v = 0.0f;
      switch (mShaderDump->globVars.v[gvid].type)
      {
        case SHVT_INT: v = mShaderDump->globVars.get<int>(gvid); break;
        case SHVT_REAL: v = mShaderDump->globVars.get<real>(gvid); break;
        case SHVT_TEXTURE:
        case SHVT_BUFFER:
        case SHVT_TLAS: break;
        default: continue;
      }
      globVarIntervalIdx[gvid] = i;
      globIntervalNormValues[i] = mShaderDump->intervals[i].getNormalizedValue(v);
    }
    else if (mShaderDump->intervals[i].type == mShaderDump->intervals[i].TYPE_ASSUMED_INTERVAL)
    {
      int gvid = bfind_packed_uint16_x2(mShaderDump->gvMap, mShaderDump->intervals[i].nameId);
      if (gvid >= 0 && mShaderDump->globVars.v[gvid].type == SHVT_INT)
        globVarIntervalIdx[gvid] = i;
    }
  }

  shaderbindump::intervalBinds.clear();
  shaderbindump::intervalBindRanges.clear();

  clear_and_resize(vprId, mShaderDump->vprCount);
  eastl::fill(vprId.begin(), vprId.end(), BAD_VPROG);
  clear_and_resize(fshId, mShaderDump->fshCount);
  eastl::fill(fshId.begin(), fshId.end(), BAD_FSHADER);
}

void ScriptedShadersBinDumpOwner::clear()
{
  if (mShaderDump == &shBinDump())
  {
    // tell the driver that we are going to unload this bindump
    d3d::driver_command(Drv3dCommand::REGISTER_SHADER_DUMP);
  }
  mDecompressedGropusLru.reset();
  mDictionary.reset();
  mSelfData = {};
  mShaderDump = nullptr;
  mDictionary = nullptr;
  vprId.clear();
  fshId.clear();
}

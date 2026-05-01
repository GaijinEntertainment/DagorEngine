// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shadersBinaryData.h"
#include "shBindumpsPrivate.h"
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
#include <shaders/dag_bindumpReloadListener.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_commands.h>
#include <util/dag_watchdog.h>
#include <generic/dag_align.h>
#include <3d/dag_lockTexture.h>
#include <drv/3d/dag_info.h>

namespace shaderbindump
{
static dag::Vector<shaderbindump::VariantTable::IntervalBind> intervalBinds;
struct IntervalBindRange
{
  uint16_t start;
  uint16_t count;
};
static dag::Vector<IntervalBindRange> intervalBindRanges;
static OSSpinlock mutex;

ShaderStubTexturesRepository g_stub_texture_repo;

void reset_interval_binds()
{
  intervalBinds.clear();
  intervalBindRanges.clear();
}

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
  const auto &globalData = shGlobalData();
  OSSpinlockScopedLock lock(mutex);
  cache.resize_noinit(intervalBindRanges.size());
  int *p = cache.data();
  for (const auto &r : intervalBindRanges)
  {
    int v = 0;
    for (uint32_t j = r.start, je = j + r.count; j < je; ++j)
      v += globalData.globIntervalNormValues[intervalBinds[j].intervalId] * intervalBinds[j].totalMul;
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
static bool read_shdump_file(IGenLoad &crd, int size, bool mmaped_load, F cb)
{
  if (mmaped_load) // mmaped code path
  {
    G_ASSERT(crd.tell() == 0);
    int flen = -1;
    auto fdata = (const uint8_t *)df_mmap(static_cast<FullFileLoadCB &>(crd).fileHandle, &flen);
    if (!fdata)
    {
      debug("[SH] Failed to map bindump file");
      return false;
    }
    if (flen != size)
    {
      debug("[SH] Bindump file mapped size doesn't match required size");
      df_unmap(fdata, flen);
      return false;
    }
    if (!cb(fdata, size))
    {
      df_unmap(fdata, flen);
      return false;
    }
    df_unmap(fdata, flen);
  }
  else // generic path with memcopy
  {
    Tab<uint8_t> dump(size);
    if (crd.tryRead(dump.data(), size) != size)
    {
      debug("[SH] Failed to fread bindump file contents");
      return false;
    }
    if (!cb(dump.data(), size))
    {
      return false;
    }
  }
  return true;
}

bool ScriptedShadersBinDumpOwner::loadFromFile(IGenLoad &crd, int size, bool mmaped_load)
{
  // Clear before loading the file to avoid holding more data in memory than necessary
  clear();

  // on systems with slow disk drives this may lead to freeze detection kicking in
  watchdog_kick();

  return read_shdump_file(crd, size, mmaped_load,
    [&](const uint8_t *data, int size_) { return loadFromData(data, size_, crd.getTargetName()); });
}

bool ScriptedShadersBinDumpOwner::loadFromData(uint8_t const *dump, int size, char const *dump_name)
{
  clear();

  debug("[SH] Dump file loaded at %p, size is %d bytes", dump, size);

  if (dump_name)
    name.setStr(dump_name);

  auto mappedDump = bindump::map<shader_layout::ScriptedShadersBinDumpCompressed>(dump);
  if (!mappedDump)
  {
    debug("[SH] Failed to map bindump compressed structure");
    return false;
  }

  const auto &header = mappedDump->header;

  if (header.magicPart1 != _MAKE4C('VSPS') || header.magicPart2 != _MAKE4C('dump') || header.version != SHADER_BINDUMP_VER)
  {
    // @TODO: should this be a fatal too? seems like sign maybe should cause a fatal, but version -- allow to look for other dumps.
    debug("[SH] Invalid bindump magic=%x|%x version=%d", header.magicPart1, header.magicPart2, header.version);
    return false;
  }

  static_assert(offsetof(eastl::remove_pointer_t<decltype(mappedDump)>, scriptedShadersBindumpCompressed) == sizeof(header));
  auto data = dag::ConstSpan<uint8_t>{dump, size}.subspan(sizeof(header));

  debug("[SH] Calculating checksum for dump data without header at %p, size is %lu bytes", data.data(), data.size());

  Blake3Csum csum = blake3_csum(data.data(), data.size());
  static_assert(sizeof(csum.data) == sizeof(header.checksumHash));

  if (memcmp(header.checksumHash, csum.data, sizeof(csum.data)) != 0)
  {
    DAG_FATAL("Corrupt file: shaders bindump checksum validation failed\n"
              "  checksum from header : %s\n"
              "  checksum computed from content : %s",
      make_hash_log_string(header.checksumHash).c_str(), make_hash_log_string(csum.data).c_str());
    return false;
  }

  debug("[SH] Shaders bindump is intact with checksum : %s", make_hash_log_string(header.checksumHash).c_str());

  // @TODO: this should never be true if checksum matched, remove if this does not occur for some time
  if (DAGOR_UNLIKELY(size <= mappedDump->scriptedShadersBindumpCompressed.getCompressedSize()))
  {
    G_ASSERT_LOG(size > mappedDump->scriptedShadersBindumpCompressed.getCompressedSize(), // Repeating for better assert message
      "Invalid shader dump file: total file size is %d bytes, while header specifies a mappedDump size of %u bytes", size,
      mappedDump->scriptedShadersBindumpCompressed.getCompressedSize());
    return false;
  }

  mShaderDump = mappedDump->scriptedShadersBindumpCompressed.decompress(mSelfData, bindump::BehaviorOnUncompressed::Copy);
  if (!mShaderDump)
  {
    debug("[SH] Failed to map bindump main structure");
    return false;
  }

  mShaderDumpV2 = bindump::map<shader_layout::ScriptedShadersBinDumpV2>(mSelfData.data());
  mShaderDumpV3 = bindump::map<shader_layout::ScriptedShadersBinDumpV3>(mSelfData.data());
  mShaderDumpV4 = bindump::map<shader_layout::ScriptedShadersBinDumpV4>(mSelfData.data());
  mShaderDumpV5 = bindump::map<shader_layout::ScriptedShadersBinDumpV5>(mSelfData.data());

  mDictionary = eastl::unique_ptr<ZSTD_DDict_s, ZstdDictionaryDeleter>(
    zstd_create_ddict(dag::ConstSpan<char>(mShaderDump->dictionary.data(), mShaderDump->dictionary.size()), true));
  mInitialDataHash = mem_hash_fnv1<64>((char *)mSelfData.data(), mSelfData.size());

  return true;
}

ShaderSource ScriptedShadersBinDumpOwner::getCode(uint32_t id, ShaderCodeType type) const
{
  id += type == ShaderCodeType::VERTEX ? 0 : vprId.size();
  return getCodeById(id);
}

ShaderSource ScriptedShadersBinDumpOwner::getCodeById(uint32_t id) const
{
  TIME_PROFILE(decompress_shader);

  const auto &compressed_data = mShaderDump->shaders[id];
  const auto &metadata = mShaderDump->shaders_metadata[id];
  uint32_t size = dag::align_up(mShaderDump->uncompressed_shader_sizes[id], sizeof(uint32_t));
  return {.compressedData = make_span_const(compressed_data.data(), compressed_data.size()),
    .metadata = make_span_const(metadata.data(), metadata.size()),
    .uncompressedSize = size,
    .dictionary = mDictionary.get()};
}

void ScriptedShadersBinDumpOwner::initAfterLoad(bool is_main)
{
  G_ASSERT(mShaderDump->globVars.v.size() < 0x2000);
  G_ASSERT(mShaderDump->varMap.size() < 0x2000);

  clear_and_resize(vprId, mShaderDump->vprCount);
  eastl::fill(vprId.begin(), vprId.end(), BAD_VPROG);
  clear_and_resize(fshId, mShaderDump->fshCount);
  eastl::fill(fshId.begin(), fshId.end(), BAD_FSHADER);
  // TODO: fsh and csh should have different sizes, but this is on shader compiler people to do
  clear_and_resize(cshId, mShaderDump->fshCount);
  eastl::fill(cshId.begin(), cshId.end(), BAD_PROGRAM);

  if (mShaderDumpV5 && d3d::is_inited())
  {
    for (auto key : mShaderDumpV5->usedStubTextureKeys)
      shaderbindump::g_stub_texture_repo.add(key);
  }

  ++generation;
}

void ScriptedShadersBinDumpOwner::clear()
{
  mDictionary.reset();
  mSelfData = {};
  mShaderDump = nullptr;
  mDictionary = nullptr;
  vprId.clear();
  fshId.clear();
  cshId.clear();
  name = {};
  shaderMats = {};
  shaderMatElems = {};
  assertionCtx.close();
}

void ScriptedShadersGlobalData::initAfterLoad(ScriptedShadersBinDumpOwner const *backing_dump, bool is_main)
{
  isInitialized = true;

  backing = backing_dump;
  G_ASSERT_RETURN(backing, );
  auto const *dump = backing->getDump();
  G_ASSERT_RETURN(dump, );

  clear_and_resize(globVarIntervalIdx, dump->globVars.v.size());
  mem_set_ff(globVarIntervalIdx);
  clear_and_resize(globIntervalNormValues, dump->intervals.size());
  mem_set_0(globIntervalNormValues);

  for (int i = 0; i < dump->intervals.size(); i++)
  {
    if (dump->intervals[i].type == dump->intervals[i].TYPE_GLOBAL_INTERVAL)
    {
      int gvid = bfind_packed_uint16_x2(dump->gvMap, dump->intervals[i].nameId);
      float v = 0.0f;
      switch (dump->globVars.v[gvid].type)
      {
        case SHVT_INT: v = dump->globVars.get<int>(gvid); break;
        case SHVT_REAL: v = dump->globVars.get<real>(gvid); break;
        case SHVT_TEXTURE:
        case SHVT_BUFFER:
        case SHVT_TLAS: break;
        default: continue;
      }
      globVarIntervalIdx[gvid] = i;
      globIntervalNormValues[i] = dump->intervals[i].getNormalizedValue(v);
    }
    else if (dump->intervals[i].type == dump->intervals[i].TYPE_ASSUMED_INTERVAL)
    {
      int gvid = bfind_packed_uint16_x2(dump->gvMap, dump->intervals[i].nameId);
      if (gvid >= 0 && dump->globVars.v[gvid].type == SHVT_INT)
        globVarIntervalIdx[gvid] = i;
    }
  }

  globVarsState.load(*dump);
  preRequestStaticSamplers(*backing);

  violatedAssumedIntervals.clear();

  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByName("graphics");
  const size_t maxBindumpShadervars =
    graphicsBlk ? graphicsBlk->getInt("max_bindump_shadervars", DEFAULT_MAX_SHVARS) : DEFAULT_MAX_SHVARS;

  G_ASSERTF(dump->varMap.size() <= maxBindumpShadervars,
    "Total number of shadervars (%d) exceeds the max allowed for mapping (%d). "
    "Remove redundant shadervars, or increase graphics/max_bindump_shadervars in the config",
    dump->varMap.size(), maxBindumpShadervars);

  initVarIndexMaps(maxBindumpShadervars);
  initBlockIndexMap();
}

void ScriptedShadersGlobalData::clear() { globVarsState.clear(); }

const UniqueTex &ShaderStubTexturesRepository::query(uint32_t col, ShaderVarTextureType shvtt) const
{
  shvtt = shvtt == SHVT_TEX_UNKNOWN ? SHVT_TEX_2D : shvtt;
  auto it = stubTexturesMap.find(shader_layout::StubTextureKey{col, shvtt});
  if (DAGOR_UNLIKELY(it == stubTexturesMap.end()))
  {
    logwarn("Stub texture for col=%x type=%d was not found, probably due to export shaders being used before d3d init.", col,
      int(shvtt));
    return stubTextureStore[0];
  }
  return stubTextureStore[it->second];
}

void ShaderStubTexturesRepository::add(shader_layout::StubTextureKey key)
{
  auto [it, isNew] = stubTexturesMap.emplace(key, stubTextureStore.size());
  if (!isNew)
    return;

  auto &tex = stubTextureStore.emplace_back();

  char namebuf[64];
  SNPRINTF(namebuf, sizeof(namebuf), "shader_stubtex_%d_%x", key.textype, key.col);
  int flags = TEXFMT_R8G8B8A8 | TEXCF_WRITEONLY;

  switch (key.textype)
  {
    case SHVT_TEX_UNKNOWN:
    case SHVT_TEX_2D: tex = dag::create_tex(nullptr, 1, 1, flags, 1, namebuf); break;
    case SHVT_TEX_3D: tex = dag::create_voltex(1, 1, 1, flags, 1, namebuf); break;
    case SHVT_TEX_CUBE: tex = dag::create_cubetex(1, flags, 1, namebuf); break;
    case SHVT_TEX_2D_ARRAY: tex = dag::create_array_tex(1, 1, 1, flags, 1, namebuf); break;
    case SHVT_TEX_CUBE_ARRAY: tex = dag::create_cube_array_tex(1, 1, flags, 1, namebuf); break;
    default: G_ASSERT(0);
  }

  const bool isCube = (key.textype == SHVT_TEX_CUBE || key.textype == SHVT_TEX_CUBE_ARRAY);
  bool ok = true;

  for (int layer = 0, lc = isCube ? 6 : 1; layer < lc; ++layer)
  {
    eastl::optional<int> ll{};
    if (isCube)
      ll.emplace(layer);
    if (auto lock = lock_texture<uint32_t>(stubTextureStore.back().getBaseTex(), 0, TEXLOCK_WRITE, ll))
    {
      lock.at(0, 0) = key.col;
    }
    else
    {
      ok = false;
      break;
    }
  }

  if (!ok)
  {
    logerr("Failed to create stubtex for col=%x type=%d", key.col, key.textype);
    stubTexturesMap.erase(it);
    stubTextureStore.pop_back();
  }
}

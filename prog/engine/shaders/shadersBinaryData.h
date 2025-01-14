// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderStates.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_patchTab.h>
#include <generic/dag_smallTab.h>
#include <util/dag_stdint.h>
#include <shaders/shInternalTypes.h>
#include <shaders/shader_layout.h>

#include <util/dag_globDef.h>
#include <shaders/dag_renderStateId.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_TMatrix4.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_spinlock.h>
#include <EASTL/span.h>
#include <EASTL/bonus/lru_cache.h>
#include <EASTL/array.h>

struct Color4;
struct ShaderChannelId;
class BaseTexture;
class IGenLoad;

namespace shaderbindump
{
extern uint32_t get_generation();

using VarList = bindump::Mapper<shader_layout::VarList>;
using Interval = bindump::Mapper<shader_layout::Interval>;
using VariantTable = bindump::Mapper<shader_layout::VariantTable>;
using ShaderCode = bindump::Mapper<shader_layout::ShaderCode>;
using ShaderClass = bindump::Mapper<shader_layout::ShaderClass>;
using ShaderBlock = bindump::Mapper<shader_layout::ShaderBlock>;

const ShaderClass &null_shader_class(bool with_code = true);
const ShaderCode &null_shader_code();
} // namespace shaderbindump

using ScriptedShadersBinDump = bindump::Mapper<shader_layout::ScriptedShadersBinDump>;
using ScriptedShadersBinDumpV2 = bindump::Mapper<shader_layout::ScriptedShadersBinDumpV2>;
using ScriptedShadersBinDumpV3 = bindump::Mapper<shader_layout::ScriptedShadersBinDumpV3>;
using StrHolder = bindump::Mapper<bindump::StrHolder>;

static constexpr uint32_t SHADERVAR_IDX_ABSENT = 0xFFFE;
static constexpr uint32_t SHADERVAR_IDX_INVALID = 0xFFFF;

enum class ShaderCodeType
{
  VERTEX,
  PIXEL,
  COMPUTE = PIXEL,
};

using ShaderBytecode = Tab<uint32_t>;

struct ScriptedShadersBinDumpOwner
{
  bool load(IGenLoad &crd, int size, bool full_file_load = false);
  bool loadData(const uint8_t *dump, int size);
  // normally called from load, but can be called explicitly to restore bindump
  void initAfterLoad();

  void clear();

  size_t getDumpSize() const { return mSelfData.size(); }
  HashVal<64> getDumpHash() const { return mInitialDataHash; }

  dag::ConstSpan<uint32_t> getCode(uint32_t id, ShaderCodeType type, ShaderBytecode &tmpbuf);

  ScriptedShadersBinDump *operator->() { return mShaderDump; }
  ScriptedShadersBinDump *getDump() { return mShaderDump; }
  ScriptedShadersBinDumpV2 *getDumpV2() { return mShaderDumpV2; }
  ScriptedShadersBinDumpV3 *getDumpV3() { return mShaderDumpV3; }

  Tab<int16_t> globVarIntervalIdx;
  Tab<uint8_t> globIntervalNormValues;

  dag::Vector<int> vprId;
  dag::Vector<int> fshId;

  // Runtime map name id -> internal dump id
  // @NOTE: have to set size on construction, cause das AOT accesses these arrays in funciton simulation without intializing the
  // bindump, which is quite weird
  //
  // @TODO: fix aot sim instead
  Tab<uint16_t> varIndexMap = Tab<uint16_t>(DEFAULT_MAX_SHVARS);
  Tab<uint16_t> globvarIndexMap = Tab<uint16_t>(DEFAULT_MAX_SHVARS);

  auto getDecompressionDict() { return mDictionary.get(); }
  size_t maxShadervarCnt() const { return varIndexMap.size(); }

private:
  static constexpr size_t DEFAULT_MAX_SHVARS = 4096;

  struct DecompressedGroup
  {
    Tab<uint8_t> decompressed_data;
    bindump::Mapper<shader_layout::ShGroup> *sh_group = nullptr;
  };
  using decompressed_groups_cache_t = eastl::lru_cache<uint16_t, DecompressedGroup>;

  ScriptedShadersBinDump *mShaderDump = nullptr;
  ScriptedShadersBinDumpV2 *mShaderDumpV2 = nullptr;
  ScriptedShadersBinDumpV3 *mShaderDumpV3 = nullptr;
  Tab<uint8_t> mSelfData;
  HashVal<64> mInitialDataHash;

  eastl::unique_ptr<decompressed_groups_cache_t> mDecompressedGropusLru;
  OSSpinlock mDecompressedGroupsLruMutex;

  void copyDecompressedShader(const DecompressedGroup &decompressed_group, uint16_t index_in_group, ShaderBytecode &tmpbuf);
  void loadDecompressedShader(uint16_t group_id, uint16_t index_in_group, ShaderBytecode &tmpbuf);
  void storeDecompressedGroup(uint16_t group_id, DecompressedGroup &&decompressed_group);

  void initVarIndexMaps(size_t max_shadervar_cnt);

  struct ZstdDictionaryDeleter
  {
    void operator()(struct ZSTD_DDict_s *dict) const { zstd_destroy_ddict(dict); }
  };
  eastl::unique_ptr<struct ZSTD_DDict_s, ZstdDictionaryDeleter> mDictionary;
};

namespace shaderbindump
{
static constexpr int MAX_BLOCK_LAYERS = 3;

extern unsigned blockStateWord;
extern shaderbindump::ShaderBlock *nullBlock[MAX_BLOCK_LAYERS];

extern bool autoBlockStateWordChange;

#if DAGOR_DBGLEVEL > 0
extern const ShaderClass *shClassUnderDebug;

void dumpShaderInfo(const ShaderClass &cls, bool dump_variants = true);
void dumpVar(const shaderbindump::VarList &vars, int var);
void dumpVars(const shaderbindump::VarList &vars);
void dumpUnusedVariants(const shaderbindump::ShaderClass &cls);

void add_exec_stcode_time(const shaderbindump::ShaderClass &cls, const __int64 &time);

bool markInvalidVariant(int shader_nid, unsigned short stat_varcode, unsigned short dyn_varcode);
bool hasShaderInvalidVariants(int shader_nid);
void resetInvalidVariantMarks();

const char *decodeVariantStr(dag::ConstSpan<shaderbindump::VariantTable::IntervalBind> p, unsigned c, String &tmp);
dag::ConstSpan<unsigned> getVariantCodesForIdx(const shaderbindump::VariantTable &vt, int code_idx);
const char *decodeStaticVariants(const shaderbindump::ShaderClass &shClass, int code_idx);
#endif

struct ShaderInterval
{
  bindump::string name;
  int value = -1;
  int valueCount = 0;
};

struct ShaderVariant
{
  bindump::vector<ShaderInterval> intervals;
  uint32_t size = 0;
  int vprId = -1;
  int fshId = -1;
};

struct ShaderStaticVariant
{
  bindump::vector<ShaderInterval> staticIntervals;
  bindump::vector<ShaderVariant> dynamicVariants;
};

struct ShaderStatistics
{
  bindump::string shaderName;
  bindump::vector<ShaderStaticVariant> staticVariants;
};

uint32_t get_dynvariant_collection_id(const shaderbindump::ShaderCode &code);
void build_dynvariant_collection_cache(dag::Vector<int, framemem_allocator> &cache);
void build_dynvariant_collection_cache(dag::Vector<int> &cache);

extern ScriptedShadersBinDumpOwner shadersBinDump;
extern ScriptedShadersBinDump emptyDump;
inline ScriptedShadersBinDump &get_dump_safe(ScriptedShadersBinDump *d) { return d ? *d : shaderbindump::emptyDump; }
} // namespace shaderbindump

inline ScriptedShadersBinDumpOwner &shBinDumpOwner() { return shaderbindump::shadersBinDump; }
inline ScriptedShadersBinDump &shBinDumpRW() { return shaderbindump::get_dump_safe(shBinDumpOwner().getDump()); }
inline const ScriptedShadersBinDump &shBinDump() { return shBinDumpRW(); }

ScriptedShadersBinDumpOwner &shBinDumpExOwner(bool main);
inline ScriptedShadersBinDump &shBinDumpExRW(bool main) { return shaderbindump::get_dump_safe(shBinDumpExOwner(main).getDump()); }
inline const ScriptedShadersBinDump &shBinDumpEx(bool main) { return shBinDumpExRW(main); }

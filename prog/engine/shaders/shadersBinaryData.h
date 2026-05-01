// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shBindumps.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_shaderModelVersion.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_shader.h>
#include <generic/dag_patchTab.h>
#include <generic/dag_smallTab.h>
#include <dag/dag_vectorSet.h>
#include <util/dag_stdint.h>
#include <shaders/shInternalTypes.h>
#include <shaders/shader_layout.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>

#include <util/dag_globDef.h>
#include <shaders/dag_renderStateId.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_TMatrix4.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_spinlock.h>
#include <EASTL/span.h>
#include <EASTL/array.h>
#include <dag/dag_vectorMap.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include "shaderVarsState.h"
#include "shAssertPrivate.h"

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
using ScriptedShadersBinDumpV4 = bindump::Mapper<shader_layout::ScriptedShadersBinDumpV4>;
using ScriptedShadersBinDumpV5 = bindump::Mapper<shader_layout::ScriptedShadersBinDumpV5>;
using StrHolder = bindump::Mapper<bindump::StrHolder>;

static constexpr uint32_t SHADERVAR_IDX_ABSENT = 0xFFFE;
static constexpr uint32_t SHADERVAR_IDX_INVALID = 0xFFFF;

#ifdef SET_DEFAULT_MAX_SHVARS
static constexpr size_t DEFAULT_MAX_SHVARS = SET_DEFAULT_MAX_SHVARS;
#else
static constexpr size_t DEFAULT_MAX_SHVARS = 4096;
#endif

enum class ShaderCodeType
{
  VERTEX,
  PIXEL,
  COMPUTE = PIXEL,
};

using ShaderBytecode = Tab<uint8_t>;

struct ScriptedShadersGlobalData;

struct ScriptedShadersBinDumpOwner
{
  ShaderBindumpHandle selfHandle = {};
  uint32_t generation = 0;

  dag::Vector<int> vprId;
  dag::Vector<int> fshId;
  dag::Vector<int> cshId;

  String name{};

  ScriptedShadersBinDump *mShaderDump = nullptr;
  ScriptedShadersBinDumpV2 *mShaderDumpV2 = nullptr;
  ScriptedShadersBinDumpV3 *mShaderDumpV3 = nullptr;
  ScriptedShadersBinDumpV4 *mShaderDumpV4 = nullptr;
  ScriptedShadersBinDumpV5 *mShaderDumpV5 = nullptr;
  Tab<uint8_t> mSelfData;
  HashVal<64> mInitialDataHash;

  struct ZstdDictionaryDeleter
  {
    void operator()(struct ZSTD_DDict_s *dict) const { zstd_destroy_ddict(dict); }
  };
  eastl::unique_ptr<struct ZSTD_DDict_s, ZstdDictionaryDeleter> mDictionary;

  ska::flat_hash_set<ScriptedShaderMaterial *> shaderMats;
  ska::flat_hash_set<ScriptedShaderElement *> shaderMatElems;

  eastl::vector_map<uint32_t, uint32_t> gvarInverseLinkMap{};
  eastl::vector_map<uint32_t, uint32_t> intervalInverseLinkMap{};
  eastl::vector_map<shader_layout::blk_word_t, shader_layout::blk_word_t> blockInverseLinkMap{};

  stcode::Context stcodeCtx;

  shader_assert::AssertionContext assertionCtx;

  bool loadFromFile(IGenLoad &crd, int size, bool mmaped_load = false);
  bool loadFromData(uint8_t const *dump, int size, char const *dump_name = nullptr);

  // Must be called after loadFromXXX
  void initAfterLoad(bool is_main);

  void clear();

  size_t getDumpSize() const { return mSelfData.size(); }
  HashVal<64> getDumpHash() const { return mInitialDataHash; }

  ShaderSource getCode(uint32_t id, ShaderCodeType type) const;
  ShaderSource getCodeById(uint32_t id) const;

  ScriptedShadersBinDump *operator->() { return mShaderDump; }
  ScriptedShadersBinDump *getDump() { return mShaderDump; }
  ScriptedShadersBinDumpV2 *getDumpV2() { return mShaderDumpV2; }
  ScriptedShadersBinDumpV3 *getDumpV3() { return mShaderDumpV3; }

  const ScriptedShadersBinDump *getDump() const { return mShaderDump; }
  const ScriptedShadersBinDumpV2 *getDumpV2() const { return mShaderDumpV2; }
  const ScriptedShadersBinDumpV3 *getDumpV3() const { return mShaderDumpV3; }
  const ScriptedShadersBinDumpV4 *getDumpV4() const { return mShaderDumpV4; }
  const ScriptedShadersBinDumpV5 *getDumpV5() const { return mShaderDumpV5; }

  auto getDecompressionDict() { return mDictionary.get(); }

  bool linkAgaistGlobalData(ScriptedShadersGlobalData const &global_link_data, bool dump_changed);
};

struct ScriptedShadersGlobalData
{
  ScriptedShadersBinDumpOwner const *backing = nullptr;

  Tab<int16_t> globVarIntervalIdx{};
  Tab<uint8_t> globIntervalNormValues{};

#if DAGOR_DBGLEVEL > 0
  ska::flat_hash_map<D3dResource *, int> globResourceVarPointerUsageMap{};
#endif

  ShaderVarsState globVarsState{};

  dag::VectorSet<int> violatedAssumedIntervals{};

  // Runtime map name id -> internal dump id
  // @NOTE: have to set size on construction, cause das AOT accesses these arrays in funciton simulation without intializing the
  // bindump, which is quite weird
  //
  // @TODO: fix aot sim instead
  Tab<uint16_t> varIndexMap = Tab<uint16_t>(DEFAULT_MAX_SHVARS);
  Tab<uint16_t> globvarIndexMap = Tab<uint16_t>(DEFAULT_MAX_SHVARS);

  Tab<int16_t> shaderBlockIndexMap{};

  bool isInitialized = false;


  void initAfterLoad(ScriptedShadersBinDumpOwner const *backing_dump, bool is_main);
  void clear();

  size_t maxShadervarCnt() const { return varIndexMap.size(); }

#if DAGOR_DBGLEVEL > 0
  bool validateShadervarWrite(int shadervar_id, int shadervar_name_id) const;
#else
  bool validateShadervarWrite(int, int) const { return true; }
#endif

  void initVarIndexMaps(size_t max_shadervar_cnt);
  void initBlockIndexMap();
  void preRequestStaticSamplers(ScriptedShadersBinDumpOwner const &from_dump);

  bool initialized() const { return backing != nullptr; }
};

struct ShadersBinDumpAssetData
{
  String fullName{};
  int dumpFileSz = 0;
  bool mmapedLoad = true; // To consider: make something like FullMMappedFileLoadCB
  eastl::optional<FullFileLoadCB> fileCrd;
  eastl::optional<InPlaceMemLoadCB> memCrd;
#if _TARGET_ANDROID
  Tab<char> assetData;
#endif

  IGenLoad &getCrd() { return memCrd ? (IGenLoad &)memCrd.value() : (IGenLoad &)fileCrd.value(); }

  bool valid() const
  {
    G_ASSERT(!(memCrd && fileCrd));
    return memCrd || fileCrd;
  }
};

bool load_shaders_bindump_asset(ShadersBinDumpAssetData &dump_asset, const char *src_filename,
  d3d::shadermodel::Version shader_model_version);

class ShaderStubTexturesRepository
{
  dag::VectorMap<uint64_t, size_t> stubTexturesMap;
  Tab<UniqueTex> stubTextureStore;

  friend struct ScriptedShadersBinDumpOwner;
  friend class ShadersRestartProc;

public:
  ShaderStubTexturesRepository() { stubTextureStore.emplace_back(); } // default for fallback
  ShaderStubTexturesRepository(ShaderStubTexturesRepository &&) = default;
  ShaderStubTexturesRepository &operator=(ShaderStubTexturesRepository &&) = default;
  ShaderStubTexturesRepository(const ShaderStubTexturesRepository &) = delete;
  ShaderStubTexturesRepository &operator=(const ShaderStubTexturesRepository &) = delete;
  ~ShaderStubTexturesRepository() { shutdown(); }

  const UniqueTex &query(uint32_t col, ShaderVarTextureType shvtt) const;
  bool filled() const { return !stubTexturesMap.empty(); }

private:
  void add(shader_layout::StubTextureKey key);
  void shutdown()
  {
    stubTexturesMap.clear();
    stubTextureStore.resize(1);
  }
};

namespace shaderbindump
{
static constexpr int MAX_BLOCK_LAYERS = 3;

extern unsigned blockStateWord;
extern shaderbindump::ShaderBlock *nullBlock[MAX_BLOCK_LAYERS];

extern bool autoBlockStateWordChange;

extern ShaderStubTexturesRepository g_stub_texture_repo;

#if DAGOR_DBGLEVEL > 0
extern const ShaderClass *shClassUnderDebug;

struct ShaderDumpDetails
{
  bool headerOnly = false;
  bool dumpVariants = false;

  bool localVars = false;
  bool staticInit = false;
  bool varSummary = false;
  bool staticVariantTable = false;

  bool codeBlocks = false;
  bool stvarmap = false;
  bool vertexChannels = false;
  bool dynamicVariantTable = false;

public:
  void validate()
  {
    if (headerOnly)
    {
      *this = ShaderDumpDetails();
      headerOnly = true;
    }

    staticVariantTable |= dumpVariants;
    dynamicVariantTable |= dumpVariants;

    codeBlocks |= stvarmap || vertexChannels || dynamicVariantTable;
  }
  bool operator==(const ShaderDumpDetails &other) const = default;
};

struct DumpDetails
{
  bool globalVars = false;
  bool namedStateBlocks = false;
  bool maxregCount = false;

  bool shaders = false;
  ShaderDumpDetails shaderDetails = {};

  bool assembly = false;
  bool stcode = false;

public:
  enum class Preset
  {
    MINIMAL,
    BRIEF,
    TABLES,
    DETAIL,
    DEFAULT = DETAIL,
    FULL
  };

  // clang-format off
  /*
  --- DUMP OUTPUT PRESETS ---
  *---------**-------*-----------*---------*---------*-----*---------**---------*-----------*-------*---------*---------*-------------*-------*---------*-----------*-------------*
  |   \ OPT || GLOB  | NAMED     | MAXREG  |         |     |         ||         | VARIANTS  | LOCAL | STATIC  | VARIANT | ST VARIANT  |       | ST VAR  | VERTEX    | DYN VARIANT |
  | PRE \   || VARS  | STBLOCKS  | CNT     | SHADERS | ASM | STCODE  || HEADER  |           | VARS  | INIT    | SUMMARY | TABLE       | CODES | MAP     | CHANNELS  | TABLE       |
  *---------**-------*-----------*---------*---------*-----*---------**---------*-----------*-------*---------*---------*-------------*-------*---------*-----------*-------------*
  | MINIMAL ||       |           |         |    +    |     |         ||    +    |           |       |         |         |             |       |         |           |             |
  *---------**-------*-----------*---------*---------*-----*---------**---------*-----------*-------*---------*---------*-------------*-------*---------*-----------*-------------*
  | BRIEF   ||       |           |    +    |    +    |     |         ||         |           |   +   |    +    |    +    |             |       |         |           |             |
  *---------**-------*-----------*---------*---------*-----*---------**---------*-----------*-------*---------*---------*-------------*-------*---------*-----------*-------------*
  | TABLES  ||       |     +     |    +    |    +    |     |         ||         |           |   +   |    +    |    +    |      +      |   +   |         |           |      +      |
  *---------**-------*-----------*---------*---------*-----*---------**---------*-----------*-------*---------*---------*-------------*-------*---------*-----------*-------------*
  | DETAIL  ||   +   |     +     |    +    |    +    |     |         ||         |           |   +   |    +    |    +    |      +      |   +   |    +    |     +     |      +      |
  *---------**-------*-----------*---------*---------*-----*---------**---------*-----------*-------*---------*---------*-------------*-------*---------*-----------*-------------*
  | FULL    ||   +   |     +     |    +    |    +    |  +  |    +    ||         |     +     |   +   |    +    |    +    |      +      |   +   |    +    |     +     |      +      |
  *---------**-------*-----------*---------*---------*-----*---------**---------*-----------*-------*---------*---------*-------------*-------*---------*-----------*-------------*
  */
  // clang-format on

  DumpDetails() = default;
  DumpDetails(Preset pre)
  {
    switch (pre)
    {
      case Preset::MINIMAL:
        shaders = true;
        shaderDetails.headerOnly = true;
        break;

      case Preset::FULL:
        assembly = true;
        stcode = true;
        shaderDetails.dumpVariants = true;
        [[fallthrough]];
      case Preset::DETAIL:
        globalVars = true;
        shaderDetails.stvarmap = true;
        shaderDetails.vertexChannels = true;
        [[fallthrough]];
      case Preset::TABLES:
        namedStateBlocks = true;
        shaderDetails.staticVariantTable = true;
        shaderDetails.codeBlocks = true;
        shaderDetails.dynamicVariantTable = true;
        [[fallthrough]];
      case Preset::BRIEF:
        maxregCount = true;
        shaders = true;
        shaderDetails.localVars = true;
        shaderDetails.staticInit = true;
        shaderDetails.varSummary = true;
    }
  }
  void validate()
  {
    shaderDetails.validate();
    shaders |= shaderDetails != ShaderDumpDetails();
  }
  bool operator==(const DumpDetails &other) const = default;
};

enum class SortType
{
  NAME,
  DEFAULT = NAME,
  PASS
};

void getTotalPasses(const ShaderClass &cls, uint32_t &total_passes, uint32_t &total_unique_shaders);
void dumpShaderInfo(ScriptedShadersBinDump const &dump, const ShaderClass &cls, bool dump_variants = true);
void dumpShaderInfo(ScriptedShadersBinDump const &dump, const ShaderClass &cls, shaderbindump::ShaderDumpDetails details);
void dumpVar(ScriptedShadersBinDump const &dump, const shaderbindump::VarList &vars, const ShaderVarsState *state, int var);
void dumpVars(ScriptedShadersBinDump const &dump, const shaderbindump::VarList &vars, const ShaderVarsState *state);
void dumpUnusedVariants(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderClass &cls);

void add_exec_stcode_time(const shaderbindump::ShaderClass &cls, const __int64 &time);

bool markInvalidVariant(ShaderBindumpHandle dump_hnd, int shader_nid, unsigned short stat_varcode, unsigned short dyn_varcode);
bool hasShaderInvalidVariants(ShaderBindumpHandle dump_hnd, int shader_nid);
void resetInvalidVariantMarks(ShaderBindumpHandle dump_hnd);
void resetAllInvalidVariantMarks();

const char *decodeVariantStr(ScriptedShadersBinDump const &dump, dag::ConstSpan<shaderbindump::VariantTable::IntervalBind> p,
  unsigned c, String &tmp);
dag::ConstSpan<unsigned> getVariantCodesForIdx(ScriptedShadersBinDump const &dump, const shaderbindump::VariantTable &vt,
  int code_idx);
const char *decodeStaticVariants(ScriptedShadersBinDump const &dump, const shaderbindump::ShaderClass &shClass, int code_idx);
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
  bindump::string dumpName;
  bindump::vector<ShaderStaticVariant> staticVariants;
};

void reset_interval_binds();
uint32_t get_dynvariant_collection_id(const shaderbindump::ShaderCode &code);
void build_dynvariant_collection_cache(dag::Vector<int, framemem_allocator> &cache);
void build_dynvariant_collection_cache(dag::Vector<int> &cache);

extern ScriptedShadersBinDumpOwner shadersBinDump;
extern ScriptedShadersGlobalData shadersGlobalData;
extern ScriptedShadersBinDump emptyDump;
inline ScriptedShadersBinDump &get_dump_safe(ScriptedShadersBinDump *d) { return d ? *d : shaderbindump::emptyDump; }
} // namespace shaderbindump

inline ScriptedShadersBinDumpOwner &shBinDumpOwner() { return shaderbindump::shadersBinDump; }
inline ScriptedShadersGlobalData &shGlobalData() { return shaderbindump::shadersGlobalData; }
inline const ScriptedShadersBinDump &shBinDump() { return shaderbindump::get_dump_safe(shBinDumpOwner().getDump()); }

ScriptedShadersBinDumpOwner &shBinDumpExOwner(bool main);
ScriptedShadersGlobalData &shGlobalDataEx(bool main);
inline ScriptedShadersBinDump &shBinDumpExRW(bool main) { return shaderbindump::get_dump_safe(shBinDumpExOwner(main).getDump()); }
inline const ScriptedShadersBinDump &shBinDumpEx(bool main) { return shBinDumpExRW(main); }

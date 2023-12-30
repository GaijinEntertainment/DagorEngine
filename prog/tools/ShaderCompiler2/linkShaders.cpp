#include "linkShaders.h"
#include "loadShaders.h"

#include "globVar.h"
#include "varMap.h"
#include "shcode.h"
#include "shaderSave.h"
#include "shLog.h"
#include "shCacheVer.h"
#include "shadervarGenerator.h"
#include "binDumpUtils.h"
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_zstdIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <math/random/dag_random.h>
#include <EASTL/vector.h>
#include <EASTL/algorithm.h>
#include "assemblyShader.h"
#include "shCompiler.h"
#include "sha1_cache_version.h"
#include "hashed_cache.h"

#include <debug/dag_debug.h>
#include <3d/dag_renderStates.h>

#if _CROSS_TARGET_SPIRV
#include <spirv/compiled_meta_data.h>
#endif

#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
extern dx12::dxil::Platform targetPlatform;

#include "DebugLevel.h"
#endif

static Tab<ShaderClass *> shader_class(tmpmem_ptr());
static Tab<dag::ConstSpan<unsigned>> shaders_fsh(tmpmem_ptr());
static Tab<dag::ConstSpan<unsigned>> shaders_vpr(tmpmem_ptr());
static Tab<TabStcode> shaders_stcode(tmpmem_ptr());
static ShadervarGenerator shadervar_generator;

static Tab<TabFsh> ld_sh_fsh(tmpmem_ptr());
static Tab<TabVpr> ld_sh_vpr(tmpmem_ptr());
static Tab<SmallTab<unsigned, TmpmemAlloc> *> shaders_comp_prog(tmpmem_ptr());
static Tab<shaders::RenderState> render_states;

void init_shader_class() { VarMap::init(); }

void close_shader_class()
{
  for (int i = 0; i < shader_class.size(); ++i)
    if (shader_class[i])
      delete shader_class[i];

  clear_and_shrink(shader_class);
  ShaderGlobal::clear();
  VarMap::clear();
  IntervalValue::resetIntervalNames();

  clear_and_shrink(shaders_fsh);
  clear_and_shrink(shaders_vpr);
  clear_and_shrink(shaders_stcode);
  clear_and_shrink(render_states);

  clear_and_shrink(ld_sh_fsh);
  clear_and_shrink(ld_sh_vpr);

  clear_shared_stcode_cache();
  ShaderParser::clear_per_file_caches();
  ShaderStateBlock::deleteAllBlocks();
  clear_all_ptr_items(shaders_comp_prog);
}

void add_shader_class(ShaderClass *sc)
{
  if (!sc)
    return;
  shader_class.push_back(sc);
}

int add_fshader(dag::ConstSpan<unsigned> code)
{
  if (!code.size())
    return -1;

  for (int i = shaders_fsh.size() - 1; i >= 0; i--)
    if (shaders_fsh[i].size() == code.size() && (shaders_fsh[i].data() == code.data() || mem_eq(code, shaders_fsh[i].data())))
      return i;

  shaders_fsh.push_back(code);
  return shaders_fsh.size() - 1;
}

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>

extern bool useSha1Cache, writeSha1Cache;
extern char *sha1_cache_dir;

int add_vprog(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs, dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs)
{
  if (!vs.size())
    return -1;
  dag::ConstSpan<unsigned> code = vs;
  SmallTab<unsigned, TmpmemAlloc> *comp_prog = NULL;
#if !_CROSS_TARGET_EMPTY && !_CROSS_TARGET_METAL
  if (hs.size() || ds.size() || gs.size())
  {
#if _CROSS_TARGET_SPIRV
    constexpr size_t cacheEntryPreamble = sizeof(uint32_t) * 1;
    // just concatenates each blob and adds some chunk headers for type and location data
    size_t comboSize =
      sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(spirv::CombinedChunk) + data_size(vs) - cacheEntryPreamble;
    uint32_t chunkCount = 1;
    if (hs.size())
    {
      comboSize += sizeof(spirv::CombinedChunk) + data_size(hs) - cacheEntryPreamble;
      ++chunkCount;
    }
    if (ds.size())
    {
      comboSize += sizeof(spirv::CombinedChunk) + data_size(ds) - cacheEntryPreamble;
      ++chunkCount;
    }
    if (gs.size())
    {
      comboSize += sizeof(spirv::CombinedChunk) + data_size(gs) - cacheEntryPreamble;
      ++chunkCount;
    }
    comp_prog = new SmallTab<unsigned, TmpmemAlloc>;
    clear_and_resize(*comp_prog, (comboSize + sizeof(unsigned) - 1) / sizeof(unsigned));
    auto pre = reinterpret_cast<uint32_t *>(comp_prog->data());
    auto magic = reinterpret_cast<uint32_t *>(pre + 1);
    auto count = reinterpret_cast<uint32_t *>(magic + 1);
    auto comboChunks = reinterpret_cast<spirv::CombinedChunk *>(count + 1);
    auto comboData = reinterpret_cast<uint8_t *>(comboChunks + chunkCount);
    auto comboDataStart = comboData;

    // add fake pre id to pass as a normal shader
    // TODO: this results in wrong shader used bytes statistics at the end
    *pre = _MAKE4C('DX9v');
    *magic = spirv::SPIR_V_COMBINED_BLOB_IDENT;
    *count = chunkCount;
    comboChunks->stage = VK_SHADER_STAGE_VERTEX_BIT;
    comboChunks->offset = comboData - comboDataStart;
    comboChunks->size = static_cast<uint32_t>(data_size(vs) - cacheEntryPreamble);
    // always skip over first uint32 as it stores the shader type magic id
    memcpy(comboData, reinterpret_cast<const uint8_t *>(vs.data()) + cacheEntryPreamble, data_size(vs) - cacheEntryPreamble);
    comboData += data_size(vs) - cacheEntryPreamble;
    ++comboChunks;

    if (hs.size())
    {
      comboChunks->stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
      comboChunks->offset = comboData - comboDataStart;
      comboChunks->size = static_cast<uint32_t>(data_size(hs) - cacheEntryPreamble);
      // always skip over first uint32 as it stores the shader type magic id
      memcpy(comboData, reinterpret_cast<const uint8_t *>(hs.data()) + cacheEntryPreamble, data_size(hs) - cacheEntryPreamble);
      comboData += data_size(hs) - cacheEntryPreamble;
      ++comboChunks;
    }
    if (ds.size())
    {
      comboChunks->stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
      comboChunks->offset = comboData - comboDataStart;
      comboChunks->size = static_cast<uint32_t>(data_size(ds) - cacheEntryPreamble);
      // always skip over first uint32 as it stores the shader type magic id
      memcpy(comboData, reinterpret_cast<const uint8_t *>(ds.data()) + cacheEntryPreamble, data_size(ds) - cacheEntryPreamble);
      comboData += data_size(ds) - cacheEntryPreamble;
      ++comboChunks;
    }
    if (gs.size())
    {
      comboChunks->stage = VK_SHADER_STAGE_GEOMETRY_BIT;
      comboChunks->offset = comboData - comboDataStart;
      comboChunks->size = static_cast<uint32_t>(data_size(gs) - cacheEntryPreamble);
      // always skip over first uint32 as it stores the shader type magic id
      memcpy(comboData, reinterpret_cast<const uint8_t *>(gs.data()) + cacheEntryPreamble, data_size(gs) - cacheEntryPreamble);
      comboData += data_size(gs) - cacheEntryPreamble;
      ++comboChunks;
    }
    code = make_span(*comp_prog);
#elif _CROSS_TARGET_DX12
    comp_prog = new SmallTab<unsigned, TmpmemAlloc>;
    // skip over first uint32_t with cache tag
    dx12::dxil::combineShaders(*comp_prog, make_span(vs).subspan(1), make_span(hs).subspan(hs.size() ? 1 : 0),
      make_span(ds).subspan(ds.size() ? 1 : 0), make_span(gs).subspan(gs.size() ? 1 : 0), _MAKE4C('DX9v'));
    code = make_span(*comp_prog);
#else
    int vs_len = vs.size() - 4, hs_len = hs.size() - 4, ds_len = ds.size() - 4, gs_len = gs.size() - 4;

    if (hs_len < 0)
      hs_len = 0;
    else
      G_ASSERT(hs[0] == _MAKE4C('D11h'));
    if (ds_len < 0)
      ds_len = 0;
    else
      G_ASSERT(ds[0] == _MAKE4C('D11d'));
    if (gs_len < 0)
      gs_len = 0;
    else
      G_ASSERT(gs[0] == _MAKE4C('D11g'));
    G_ASSERT(vs[0] == _MAKE4C('DX9v'));

    Tab<unsigned> comp_unp;
    comp_unp.resize(9 + vs_len + hs_len + ds_len + gs_len);
    mem_set_0(comp_unp);
    comp_unp[0] = _MAKE4C('DX11');
    comp_unp[1] = data_size(comp_unp) - elem_size(comp_unp) * 4;
    comp_unp[2] = vs[2];
    comp_unp[3] = vs[3];
    comp_unp[4] = _MAKE4C('DX11');
    comp_unp[5] = vs_len;
    comp_unp[6] = hs_len | (hs.size() ? hs[2] & 0xFF000000 : 0);
    comp_unp[7] = ds_len;
    comp_unp[8] = gs_len;
    memcpy(&comp_unp[9], &vs[4], elem_size(vs) * vs_len);
    if (hs_len)
      memcpy(&comp_unp[9 + vs_len], &hs[4], elem_size(hs) * hs_len);
    if (ds_len)
      memcpy(&comp_unp[9 + vs_len + hs_len], &ds[4], elem_size(ds) * ds_len);
    if (gs_len)
      memcpy(&comp_unp[9 + vs_len + hs_len + ds_len], &gs[4], elem_size(gs) * gs_len);
    code = comp_unp;

    comp_prog = new SmallTab<unsigned, TmpmemAlloc>;
    clear_and_resize(*comp_prog, code.size());
    mem_copy_from(*comp_prog, code.data());
    code = make_span(*comp_prog);
#endif
  }
#endif
  // NOTE: if this changes finalize_xbox_one_shaders has to be updated too!
  for (int i = shaders_vpr.size() - 1; i >= 0; i--)
    if (shaders_vpr[i].size() == code.size() && (shaders_vpr[i].data() == code.data() || mem_eq(code, shaders_vpr[i].data())))
    {
      if (comp_prog)
        delete comp_prog;
      return i;
    }
  if (comp_prog)
    shaders_comp_prog.push_back(comp_prog);
  shaders_vpr.push_back(code);
  return shaders_vpr.size() - 1;
}

#if _CROSS_TARGET_DX12
extern int hlslOptimizationLevel;
extern bool hlslSkipValidation;
extern bool hlsl2021;
extern bool enableFp16;
extern DebugLevel hlslDebugLevel;
static bool is_hlsl_debug() { return hlslDebugLevel != DebugLevel::NONE; }
extern char *sha1_cache_dir;

#if REPORT_RECOMPILE_PROGRESS
#define REPORT debug
#else
#define REPORT(...)
#endif
VertexProgramAndPixelShaderIdents add_phase_one_progs(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs,
  dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, dag::ConstSpan<unsigned> ps)
{
  VertexProgramAndPixelShaderIdents result{-1, -1};
  auto vShaderData = make_span(vs).subspan(vs.size() ? 1 : 0);
  auto hShaderData = make_span(hs).subspan(hs.size() ? 1 : 0);
  auto dShaderData = make_span(ds).subspan(ds.size() ? 1 : 0);
  auto gShaderData = make_span(gs).subspan(gs.size() ? 1 : 0);
  auto pShaderData = make_span(ps).subspan(ps.size() ? 1 : 0);

  auto rootSignatureDefinition =
    dx12::dxil::generateRootSignatureDefinition(vShaderData, hShaderData, dShaderData, gShaderData, pShaderData);

  for (int i = shaders_vpr.size() - 1; i >= 0; i--)
  {
    if (dx12::dxil::comparePhaseOneVertexProgram(shaders_vpr[i], vShaderData, hShaderData, dShaderData, gShaderData,
          rootSignatureDefinition))
    {
      result.vprog = i;
      break;
    }
  }

  for (int i = shaders_fsh.size() - 1; i >= 0; i--)
  {
    if (dx12::dxil::comparePhaseOnePixelShader(shaders_fsh[i], pShaderData, rootSignatureDefinition))
    {
      result.fsh = i;
      break;
    }
  }

  dx12::dxil::CompilationOptions compileOptions;
  compileOptions.optimize = hlslOptimizationLevel ? true : false;
  compileOptions.skipValidation = hlslSkipValidation;
  compileOptions.debugInfo = is_hlsl_debug();
  compileOptions.scarlettW32 = useScarlettWave32;
  compileOptions.hlsl2021 = hlsl2021;
  compileOptions.enableFp16 = enableFp16;
  if (-1 == result.vprog)
  {
    auto packed = dx12::dxil::combinePhaseOneVertexProgram(vShaderData, hShaderData, dShaderData, gShaderData, rootSignatureDefinition,
      _MAKE4C('DX9v'), compileOptions);
    result.vprog = shaders_vpr.size();
    shaders_vpr.push_back(*packed);
    shaders_comp_prog.push_back(packed.release());
  }

  if (-1 == result.fsh)
  {
    auto packed = dx12::dxil::combinePhaseOnePixelShader(pShaderData, rootSignatureDefinition, _MAKE4C('DX9p'), !gShaderData.empty(),
      !hShaderData.empty() && !dShaderData.empty(), compileOptions);
    result.fsh = shaders_fsh.size();
    shaders_fsh.push_back(*packed);
    shaders_comp_prog.push_back(packed.release());
  }

  return result;
}

struct RecompileJobBase : public cpujobs::IJob
{
  struct MappingFileLayout
  {
    uint32_t ilSize;
    uint32_t byteCodeVersion;
    uint32_t byteCodeSize;
    uint8_t byteCodeHash[HASH_SIZE];
  };
  size_t compileTarget;
  uint8_t ilHash[HASH_SIZE];
  char ilPath[420];
  RecompileJobBase(size_t ct) : compileTarget{ct} {}

  eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>> load_cache(dag::ConstSpan<unsigned> uncompiled, const char *type_name)
  {
    if (!useSha1Cache)
      return {};

    const char *profile_name = dx12::dxil::Platform::XBOX_ONE == targetPlatform ? "x" : "xs";
    HASH_CONTEXT ctx;
    HASH_INIT(&ctx);
    HASH_UPDATE(&ctx, reinterpret_cast<const unsigned char *>(uncompiled.data()), data_size(uncompiled));
    HASH_FINISH(&ctx, ilHash);
    SNPRINTF(ilPath, sizeof(ilPath), "%s/il/%s/%s/" HASH_LIST_STRING, sha1_cache_dir, type_name, profile_name, HASH_LIST(ilHash));
    auto file = df_open(ilPath, DF_READ);
    if (!file)
    {
      return {};
    }

    MappingFileLayout mappingFile{};
    auto readSize = df_read(file, &mappingFile, sizeof(mappingFile));
    df_close(file);
    if (readSize < sizeof(mappingFile) || mappingFile.ilSize != data_size(uncompiled) ||
        mappingFile.byteCodeVersion != sha1_cache_version)
    {
      return {};
    }

    char byteCodePath[420];
    SNPRINTF(byteCodePath, sizeof(byteCodePath), "%s/bc/%s/%s/" HASH_LIST_STRING, sha1_cache_dir, type_name, profile_name,
      HASH_LIST(mappingFile.byteCodeHash));
    auto byteCodeFile = df_open(byteCodePath, DF_READ);
    if (!byteCodeFile)
    {
      return {};
    }
    auto fileSize = df_length(byteCodeFile);
    if (fileSize != mappingFile.byteCodeSize + sizeof(uint32_t))
    {
      df_close(byteCodeFile);
      return {};
    }

    // need to store the version or we might use outdated bins, if the final format did change.
    uint32_t version = 0;
    df_read(byteCodeFile, &version, sizeof(version));
    if (version != sha1_cache_version)
    {
      df_close(byteCodeFile);
      return {};
    }

    auto result = eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>();
    clear_and_resize(*result, mappingFile.byteCodeSize / sizeof(unsigned));
    readSize = df_read(byteCodeFile, result->data(), data_size(*result));
    df_close(byteCodeFile);
    if (readSize != data_size(*result))
    {
      return {};
    }
    return result;
  }

  void store_cache(dag::ConstSpan<unsigned> uncompiled, dag::ConstSpan<unsigned> compiled, const char *type_name)
  {
    if (!writeSha1Cache)
      return;

    const char *profile_name = dx12::dxil::Platform::XBOX_ONE == targetPlatform ? "x" : "xs";
    MappingFileLayout mappingFile{};
    mappingFile.ilSize = data_size(uncompiled);
    mappingFile.byteCodeSize = data_size(compiled);
    mappingFile.byteCodeVersion = sha1_cache_version;
    HASH_CONTEXT ctx;
    HASH_INIT(&ctx);
    HASH_UPDATE(&ctx, reinterpret_cast<const unsigned char *>(uncompiled.data()), data_size(uncompiled));
    HASH_FINISH(&ctx, mappingFile.byteCodeHash);
    char bcPath[420];
    SNPRINTF(bcPath, sizeof(bcPath), "%s/bc/%s/%s/" HASH_LIST_STRING, sha1_cache_dir, type_name, profile_name,
      HASH_LIST(mappingFile.byteCodeHash));

    DagorStat bcInfo;
    char tempPath[DAGOR_MAX_PATH];
    if (df_stat(bcPath, &bcInfo) != -1)
    {
      // if (bcInfo.size != mappingFile.byteCodeSize) {}
    }
    else
    {
      SNPRINTF(tempPath, sizeof(tempPath), "%s/%s_bc_%s_" HASH_TEMP_STRING "XXXXXX", sha1_cache_dir, type_name, profile_name,
        HASH_LIST(mappingFile.byteCodeHash));
      auto file = df_mkstemp(tempPath);
      if (!file)
      {
        return;
      }
      // store the version so that we can identify if a cache blob is outdated.
      uint32_t version = sha1_cache_version;
      df_write(file, &version, sizeof(version));
      auto written = df_write(file, compiled.data(), data_size(compiled));
      df_close(file);
      if (written != data_size(compiled))
      {
        dd_erase(tempPath);
        return;
      }

      dd_mkpath(bcPath);
      if (!dd_rename(tempPath, bcPath))
      {
        dd_erase(tempPath);
        return;
      }
    }
    SNPRINTF(tempPath, sizeof(tempPath), "%s/%s_il_%s_" HASH_TEMP_STRING "XXXXXX", sha1_cache_dir, type_name, profile_name,
      HASH_LIST(ilHash));
    auto file = df_mkstemp(tempPath);
    if (!file)
    {
      return;
    }
    auto written = df_write(file, &mappingFile, sizeof(mappingFile));
    df_close(file);
    if (written != sizeof(mappingFile))
    {
      dd_erase(tempPath);
      return;
    }

    dd_mkpath(ilPath);
    if (!dd_rename(tempPath, ilPath))
    {
      dd_erase(ilPath);
    }
  }
};
extern wchar_t *dx12_pdb_cache_dir;

struct RecompileVPRogJob : RecompileJobBase
{
  using RecompileJobBase::RecompileJobBase;

  void doJob() override
  {
    static const char cache_name[] = "vp";
    bool writeCache = false;
    auto recompiledVProgBlob = load_cache(shaders_vpr[compileTarget], cache_name);
    if (!recompiledVProgBlob)
    {
      writeCache = true;
      auto recompiledVProg =
        dx12::dxil::recompileVertexProgram(shaders_vpr[compileTarget], targetPlatform, dx12_pdb_cache_dir, hlslDebugLevel);
      if (!recompiledVProg)
      {
        DAG_FATAL("Recompilation of vprog failed");
      }
      recompiledVProgBlob = eastl::move(*recompiledVProg);
    }
    if (recompiledVProgBlob)
    {
      auto ref = eastl::find_if(eastl::begin(shaders_comp_prog), eastl::end(shaders_comp_prog),
        [cmp = shaders_vpr[compileTarget].data()](auto tab) { return cmp == tab->data(); });

      REPORT("vprog size %u -> %u", data_size(shaders_vpr[compileTarget]), data_size(*recompiledVProgBlob));

      if (writeCache)
      {
        store_cache(shaders_vpr[compileTarget], *recompiledVProgBlob, cache_name);
      }
      shaders_vpr[compileTarget] = *recompiledVProgBlob;

      G_ASSERT(ref != eastl::end(shaders_comp_prog));
      if (ref != eastl::end(shaders_comp_prog))
      {
        delete *ref;
        *ref = recompiledVProgBlob.release();
      }
    }
  }
  void releaseJob() override {}
};

struct RecompilePShJob : RecompileJobBase
{
  using RecompileJobBase::RecompileJobBase;

  void doJob() override
  {
    static const char cache_name[] = "fs";
    bool writeCache = false;
    auto recompiledFShBlob = load_cache(shaders_fsh[compileTarget], cache_name);
    if (!recompiledFShBlob)
    {
      writeCache = true;
      auto recompiledFSh =
        dx12::dxil::recompilePixelSader(shaders_fsh[compileTarget], targetPlatform, dx12_pdb_cache_dir, hlslDebugLevel);
      if (!recompiledFSh)
      {
        DAG_FATAL("Recompilation of fsh failed");
      }
      recompiledFShBlob = eastl::move(*recompiledFSh);
    }
    if (recompiledFShBlob)
    {
      auto ref = eastl::find_if(eastl::begin(shaders_comp_prog), eastl::end(shaders_comp_prog),
        [cmp = shaders_fsh[compileTarget].data()](auto tab) { return cmp == tab->data(); });

      REPORT("fsh size %u -> %u", data_size(shaders_fsh[compileTarget]), data_size(*recompiledFShBlob));

      if (writeCache)
      {
        store_cache(shaders_fsh[compileTarget], *recompiledFShBlob, cache_name);
      }
      shaders_fsh[compileTarget] = *recompiledFShBlob;

      G_ASSERT(ref != eastl::end(shaders_comp_prog));
      if (ref != eastl::end(shaders_comp_prog))
      {
        delete *ref;
        *ref = recompiledFShBlob.release();
      }
    }
  }
  void releaseJob() override {}
};

struct IndexReplacmentInfo
{
  size_t target;
  size_t old;
};

eastl::vector<IndexReplacmentInfo> dedup_table(Tab<dag::ConstSpan<unsigned>> &table, const char *name)
{
  eastl::vector<IndexReplacmentInfo> result;
  if (table.empty())
    return result;

  REPORT("Looking for recompiled %s duplicates...", name);

  for (size_t o = 0; o < table.size() - 1; ++o)
  {
    auto &base = table[o];
    if (base.empty())
      continue;

    for (size_t d = o + 1; d < table.size(); ++d)
    {
      auto &cmp = table[d];
      if (cmp.size() != base.size())
        continue;
      if (!mem_eq(cmp, base.data()))
        continue;

      cmp.reset();

      IndexReplacmentInfo r;
      r.target = o;
      r.old = d;
      result.push_back(r);
      REPORT("Found %s id %u as duplicate of %u", name, d, o);
    }
  }

  // if nothing was remapped we don't need to fill holes
  if (result.empty())
    return result;

  REPORT("Found %u duplicates...", result.size());

  while (table.back().empty())
    table.pop_back();
  return result;
}

eastl::vector<IndexReplacmentInfo> empty_fill_table(Tab<dag::ConstSpan<unsigned>> &table, const char *name)
{
  eastl::vector<IndexReplacmentInfo> result;
  if (table.empty())
    return result;

  REPORT("Filling empty slots for %s...", name);

  for (size_t i = 0; i < table.size(); ++i)
  {
    auto &base = table[i];
    if (!base.empty())
      continue;

    IndexReplacmentInfo r;
    r.target = i;
    r.old = table.size() - 1;
    result.push_back(r);
    REPORT("Found empty %s id %u mapping %u to it", name, r.target, r.old);

    base = table.back();
    table.pop_back();
  }

  return result;
}

int16_t patch_index(int16_t old, const eastl::vector<IndexReplacmentInfo> &table, const char *name)
{
  auto ref = eastl::find_if(begin(table), end(table), [old](auto &info) { return info.old == old; });
  if (ref == end(table))
    return old;
  REPORT("patching %s id from %u to %u", name, old, ref->target);
  return static_cast<int16_t>(ref->target);
}

void patch_passes(const eastl::vector<IndexReplacmentInfo> &vprg, const eastl::vector<IndexReplacmentInfo> &fsh)
{
  for (ShaderClass *sh : shader_class)
  {
    if (!sh)
      continue;

    for (ShaderCode *cd : sh->code)
    {
      if (!cd)
        continue;

      for (ShaderCode::Pass &p : cd->allPasses)
      {
        p.fsh = patch_index(p.fsh, fsh, "fsh");
        p.vprog = patch_index(p.vprog, vprg, "vprog");
      }
    }
  }
}

void dedup_shaders()
{
  auto vproCount = shaders_vpr.size();
  auto fshCount = shaders_fsh.size();

  auto vprogDups = dedup_table(shaders_vpr, "vprog");
  auto fshDups = dedup_table(shaders_fsh, "fsh");

  if (vprogDups.empty() && fshDups.empty())
  {
    REPORT("No duplicates found...");
    return;
  }

  patch_passes(vprogDups, fshDups);

  vprogDups = empty_fill_table(shaders_vpr, "vprog");
  fshDups = empty_fill_table(shaders_fsh, "fsh");

  patch_passes(vprogDups, fshDups);

  REPORT("Removed %u (%u - %u) vprogs...", vproCount - shaders_vpr.size(), vproCount, shaders_vpr.size());
  REPORT("Removed %u (%u - %u) fshs...", fshCount - shaders_fsh.size(), fshCount, shaders_fsh.size());
}

void recompile_shaders()
{
  if (shc::compileJobsCount > 1)
  {
    eastl::vector<RecompileVPRogJob> vprogRecompileJobList;
    eastl::vector<RecompilePShJob> fshRecompileJobList;
    eastl::vector<RecompileJobBase *> allJobs;
    vprogRecompileJobList.reserve(shaders_vpr.size());
    fshRecompileJobList.reserve(shaders_fsh.size());

    // generate all jobs first
    for (size_t i = 0; i < shaders_vpr.size(); ++i)
    {
      vprogRecompileJobList.emplace_back(i);
    }
    for (size_t i = 0; i < shaders_fsh.size(); ++i)
    {
      fshRecompileJobList.emplace_back(i);
    }

    allJobs.reserve(vprogRecompileJobList.size() + fshRecompileJobList.size());

    // interleave pix and vertex prog recompile to more evenly spread the work load
    size_t vpAdded = 0;
    size_t fhAdded = 0;
    while ((vpAdded < vprogRecompileJobList.size()) || (fhAdded < fshRecompileJobList.size()))
    {
      if (vpAdded < vprogRecompileJobList.size())
      {
        allJobs.push_back(&vprogRecompileJobList[vpAdded++]);
      }
      if (fhAdded < fshRecompileJobList.size())
      {
        allJobs.push_back(&fshRecompileJobList[fhAdded++]);
      }
    }

    // we submit work in batches on N per thread, we start with 6
    int jobload_per_worker = 6;
    size_t issueCount = 0;
    while (issueCount < allJobs.size())
    {
      // as less work is left to be submitted, we reduce the number of items we submit per batch
      auto workLeft = allJobs.size();
      if (workLeft < jobload_per_worker * 4)
        jobload_per_worker = 2;
      if (workLeft < jobload_per_worker * 2)
        jobload_per_worker = 1;

      // check all workers and if anyone is idle, give it the next batch of work
      for (int worker = 0; worker < shc::compileJobsCount; ++worker)
      {
        if (!cpujobs::is_job_manager_busy(shc::compileJobsMgrBase + worker))
        {
          for (int job = 0; job < jobload_per_worker && issueCount < allJobs.size(); ++job)
          {
            cpujobs::add_job(shc::compileJobsMgrBase + worker, allJobs[issueCount++]);
          }
        }
      }
      // do one job if jobs are available, we submitted more than one job per worker or its the last job
      if (issueCount < allJobs.size() && ((jobload_per_worker > 1) || ((allJobs.size() - issueCount) == 1)))
      {
        allJobs[issueCount++]->doJob();
      }
    }
    // wit for everyone to finish
    for (int i = 0; i < shc::compileJobsCount; i++)
    {
      while (cpujobs::is_job_manager_busy(shc::compileJobsMgrBase + i))
      {
        sleep_msec(10);
      }
    }
    cpujobs::release_done_jobs();
  }
  else
  {
    for (size_t i = 0; i < shaders_vpr.size(); ++i)
    {
      RecompileVPRogJob{i}.doJob();
    }
    for (size_t i = 0; i < shaders_fsh.size(); ++i)
    {
      RecompilePShJob{i}.doJob();
    }
  }

  dedup_shaders();
}
#endif

int add_stcode(dag::ConstSpan<int> code)
{
  if (!code.size())
    return -1;

  for (int i = shaders_stcode.size() - 1; i >= 0; i--)
    if (shaders_stcode[i].size() == code.size() && mem_eq(code, shaders_stcode[i].data()))
      return i;

  int i = append_items(shaders_stcode, 1);
  shaders_stcode[i].Tab<int>::operator=(code);
  shaders_stcode[i].shrink_to_fit();
  return i;
}

static int add_render_state(const shaders::RenderState &rs)
{
  for (int i = 0; i < render_states.size(); ++i)
    if (rs == render_states[i])
      return i;
  render_states.push_back(rs);
  return static_cast<int>(render_states.size()) - 1;
}

int add_render_state(const ShaderSemCode::Pass &state)
{
  shaders::RenderState rs;
  rs.ztest = state.z_test != 0 ? 1 : 0;
  rs.zwrite = state.z_write != 0 ? 1 : 0;
  if (state.z_test)
  {
    rs.zFunc = state.z_func;
  }
  if (state.stencil)
  {
    rs.stencilRef = state.stencil_ref;
    rs.stencil.func = state.stencil_func;
    rs.stencil.fail = state.stencil_fail;
    rs.stencil.zFail = state.stencil_zfail;
    rs.stencil.pass = state.stencil_pass;
    rs.stencil.readMask = rs.stencil.writeMask = state.stencil_mask;
  }
  if (!state.force_noablend)
  {
    rs.independentBlendEnabled = state.independent_blending;
    for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
    {
      rs.blendParams[i].ablendFactors.src = state.blend_src[i];
      rs.blendParams[i].ablendFactors.dst = state.blend_dst[i];
      rs.blendParams[i].sepablendFactors.src = state.blend_asrc[i];
      rs.blendParams[i].sepablendFactors.dst = state.blend_adst[i];
      rs.blendParams[i].ablend = state.blend_src[i] >= 0 && state.blend_dst[i] >= 0 ? 1 : 0;
      rs.blendParams[i].sepablend = state.blend_asrc[i] >= 0 && state.blend_adst[i] >= 0 ? 1 : 0;
    }
  }
  else
  {
    rs.independentBlendEnabled = 0;
    for (auto &blendParam : rs.blendParams)
    {
      blendParam.ablend = 0;
      blendParam.sepablend = 0;
    }
  }
  rs.cull = state.cull_mode;
  rs.alphaToCoverage = state.alpha_to_coverage;
  rs.viewInstanceCount = state.view_instances;
  rs.colorWr = state.color_write;
  rs.zBias = state.z_bias ? -state.z_bias_val / 1000.f : 0;
  rs.slopeZBias = state.slope_z_bias ? state.slope_z_bias_val : 0;

  return add_render_state(rs);
}

static int ld_add_fshader(TabFsh &code, int upper)
{
  if (!code.size())
    return -1;

  for (int i = 0; i < upper; ++i)
    if (shaders_fsh[i].size() == code.size() && mem_eq(code, shaders_fsh[i].data()))
      return i;

  int i = append_items(ld_sh_fsh, 1);
  ld_sh_fsh[i] = eastl::move(code);
  ld_sh_fsh[i].shrink_to_fit();
  shaders_fsh.push_back(ld_sh_fsh[i]);
  return shaders_fsh.size() - 1;
}

static int ld_add_vprog(TabVpr &code, int upper)
{
  if (!code.size())
    return -1;

  for (int i = 0; i < upper; ++i)
    if (shaders_vpr[i].size() == code.size() && mem_eq(code, shaders_vpr[i].data()))
      return i;

  int i = append_items(ld_sh_vpr, 1);
  ld_sh_vpr[i] = eastl::move(code);
  ld_sh_vpr[i].shrink_to_fit();
  shaders_vpr.push_back(ld_sh_vpr[i]);
  return shaders_vpr.size() - 1;
}

void count_shader_stats(unsigned &uniqueFshBytesInFile, unsigned &uniqueFshCountInFile, unsigned &uniqueVprBytesInFile,
  unsigned &uniqueVprCountInFile, unsigned &stcodeBytes)
{
  int i;

  uniqueFshCountInFile = shaders_fsh.size();
  uniqueVprCountInFile = shaders_vpr.size();
  uniqueFshBytesInFile = 0;
  uniqueVprBytesInFile = 0;
  stcodeBytes = 0;

  for (i = 0; i < shaders_fsh.size(); i++)
    uniqueFshBytesInFile += data_size(shaders_fsh[i]);
  for (i = 0; i < shaders_vpr.size(); i++)
    uniqueVprBytesInFile += data_size(shaders_vpr[i]);

  for (i = 0; i < shaders_stcode.size(); i++)
    stcodeBytes += data_size(shaders_stcode[i]);
}

static void link_shaders_fsh_and_vpr(ShadersBindump &bindump, Tab<int> &fsh_lnktbl, Tab<int> &vpr_lnktbl)
{
  // load FSH
  int fshNum = bindump.shaders_fsh.size();
  fsh_lnktbl.resize(fshNum);
  int upper = shaders_fsh.size();
  for (int i = 0; i < fshNum; i++)
    fsh_lnktbl[i] = ld_add_fshader(bindump.shaders_fsh[i], upper);

  // load VPR
  int vprNum = bindump.shaders_vpr.size();
  vpr_lnktbl.resize(vprNum);
  upper = shaders_vpr.size();
  for (int i = 0; i < vprNum; i++)
    vpr_lnktbl[i] = ld_add_vprog(bindump.shaders_vpr[i], upper);
}

bool load_shaders_bindump(ShadersBindump &shaders, bindump::IReader &full_file_reader)
{
  CompressedShadersBindump compressed;
  bindump::streamRead(compressed, full_file_reader);

  if (compressed.eof != SHADER_CACHE_EOF)
    return false;

  eastl::vector<uint8_t> decompressed_data;
  if (compressed.decompressed_size != 0)
  {
    decompressed_data.resize(compressed.decompressed_size);
    size_t decompressed_size = zstd_decompress(decompressed_data.data(), decompressed_data.size(),
      compressed.compressed_shaders.data(), compressed.compressed_shaders.size());
    G_ASSERT(decompressed_size == compressed.decompressed_size);
  }
  else
  {
    decompressed_data = eastl::move(compressed.compressed_shaders);
  }

  bindump::MemoryReader reader(decompressed_data.data(), decompressed_data.size());
  bindump::streamRead(shaders, reader);
  return true;
}

bool link_scripted_shaders(const uint8_t *mapped_data, int data_size, const char *filename)
{
  ShadersBindump shaders;
  bindump::MemoryReader compressed_reader(mapped_data, data_size);
  if (!load_shaders_bindump(shaders, compressed_reader))
    DAG_FATAL("corrupted OBJ file: %s", filename);

  shadervar_generator.addShadervarsAndIntervals(make_span(shaders.variable_list), shaders.intervals);

  // Load render states
  int renderStatesCount = shaders.render_states.size();
  Tab<int> renderStateLinkTable(tmpmem);
  renderStateLinkTable.resize(renderStatesCount);
  for (int i = 0; i < renderStatesCount; ++i)
    renderStateLinkTable[i] = add_render_state(shaders.render_states[i]);

  Tab<int> fsh_lnktbl(tmpmem);
  Tab<int> vpr_lnktbl(tmpmem);
  link_shaders_fsh_and_vpr(shaders, fsh_lnktbl, vpr_lnktbl);

  // Load stcode.
  Tab<TabStcode> &shaderStcodeFromFile = shaders.shaders_stcode;

  Tab<int> global_var_link_table(tmpmem);
  Tab<ShaderVariant::ExtType> interval_link_table(tmpmem);
  ShaderGlobal::link(shaders.variable_list, shaders.intervals, global_var_link_table, interval_link_table);

  Tab<int> smp_link_table;
  Sampler::link(shaders.static_samplers, smp_link_table);

  // Link stcode, stblkcode.
  Tab<int> stcode_lnktbl(tmpmem);
  stcode_lnktbl.resize(shaderStcodeFromFile.size());
  for (int i = 0; i < shaderStcodeFromFile.size(); i++)
  {
    bindumphlp::patchStCode(make_span(shaderStcodeFromFile[i]), global_var_link_table, smp_link_table);
    stcode_lnktbl[i] = add_stcode(shaderStcodeFromFile[i]);
  }

  // Link blocks
  ShaderStateBlock::link(shaders.state_blocks, stcode_lnktbl);

  // load shaders
  Tab<ShaderClass *> &shaderClassesFromFile = shaders.shader_classes;
  shader_class.reserve(shaderClassesFromFile.size());
  for (unsigned int shaderNo = 0; shaderNo < shaderClassesFromFile.size(); shaderNo++)
  {
    for (unsigned int existingShaderNo = 0; existingShaderNo < shader_class.size(); existingShaderNo++)
    {
      if (shader_class[existingShaderNo]->name == shaderClassesFromFile[shaderNo]->name)
      {
        sh_debug(SHLOG_ERROR, "Duplicated shader '%s'", shaderClassesFromFile[shaderNo]->name.c_str());
      }
    }

    int total_passes = 0, reduced_passes = 0;
    shaderClassesFromFile[shaderNo]->staticVariants.linkIntervalList();

    for (unsigned int codeNo = 0; codeNo < shaderClassesFromFile[shaderNo]->code.size(); codeNo++)
    {
      ShaderCode &code = *shaderClassesFromFile[shaderNo]->code[codeNo];
      int id;

      code.link();
      code.dynVariants.link(interval_link_table);

      for (unsigned int passNo = 0; passNo < code.allPasses.size(); passNo++)
      {
        ShaderCode::Pass &pass = code.allPasses[passNo];

        if (pass.fsh >= 0)
        {
          G_ASSERT(pass.fsh < fsh_lnktbl.size());
          id = fsh_lnktbl[pass.fsh];
          pass.fsh = id;
          G_ASSERTF(pass.fsh == id, "pass.fsh=%d  id=%d", pass.fsh, id);
        }

        if (pass.vprog >= 0)
        {
          G_ASSERT(pass.vprog < vpr_lnktbl.size());
          id = vpr_lnktbl[pass.vprog];
          pass.vprog = id;
          G_ASSERTF(pass.vprog == id, "pass.vprog=%d  id=%d", pass.vprog, id);
        }

        if (pass.stcodeNo >= 0)
        {
          G_ASSERT(pass.stcodeNo < stcode_lnktbl.size());
          pass.stcodeNo = stcode_lnktbl[pass.stcodeNo];
        }

        if (pass.stblkcodeNo >= 0)
        {
          G_ASSERT(pass.stblkcodeNo < stcode_lnktbl.size());
          pass.stblkcodeNo = stcode_lnktbl[pass.stblkcodeNo];
        }

        if (pass.renderStateNo >= 0)
        {
          G_ASSERT(pass.renderStateNo < renderStateLinkTable.size());
          pass.renderStateNo = renderStateLinkTable[pass.renderStateNo];
        }
      }

      // reduce passTabs
      SmallTab<int, TmpmemAlloc> idxmap;
      Tab<ShaderCode::PassTab *> newpass(midmem);

      clear_and_resize(idxmap, code.passes.size());
      mem_set_ff(idxmap);

      for (unsigned int passNo = 0; passNo < idxmap.size(); passNo++)
      {
        ShaderCode::PassTab *p = code.passes[passNo];

        for (int i = 0; i < newpass.size(); i++)
          if (p == newpass[i])
          {
            idxmap[passNo] = i;
            break;
          }
          else if (p && newpass[i] && p->rpass.get() == newpass[i]->rpass.get() && p->suppBlk == newpass[i]->suppBlk)
          {
            idxmap[passNo] = i;
            delete p;
            code.passes[passNo] = NULL;
            break;
          }

        if (idxmap[passNo] == -1)
        {
          idxmap[passNo] = newpass.size();
          newpass.push_back(p);
        }
      }

      total_passes += idxmap.size();
      reduced_passes += newpass.size();

      if (newpass.size() < idxmap.size())
      {
        // when really reduced, update code.dynVariants
        for (int i = code.dynVariants.getVarCount() - 1; i >= 0; i--)
        {
          G_ASSERT(code.dynVariants.getVariant(i)->codeId < (int)idxmap.size());
          if (code.dynVariants.getVariant(i)->codeId >= 0)
            code.dynVariants.getVariant(i)->codeId = idxmap[code.dynVariants.getVariant(i)->codeId];
        }
        code.passes = eastl::move(newpass);
      }
    }
    shader_class.push_back(shaderClassesFromFile[shaderNo]);
  }
  return true;
}

void save_scripted_shaders(const char *filename, dag::ConstSpan<SimpleString> files)
{
  if (files.empty())
    shadervar_generator.generateShadervars();

  CompressedShadersBindump compressed;
  ShadersBindump shaders;

  compressed.cache_sign = SHADER_CACHE_SIGN;
  compressed.cache_version = SHADER_CACHE_VER;
  compressed.eof = SHADER_CACHE_EOF;

  // save dependens
  for (int i = 0; i < files.size(); ++i)
    compressed.dependency_files.emplace_back(files[i].c_str());

  shaders.variable_list = eastl::move(ShaderGlobal::getMutableVariableList());
  shaders.static_samplers = eastl::move(g_samplers);
  shaders.intervals = eastl::move(ShaderGlobal::getMutableIntervalList());
  shaders.empty_block = eastl::move(ShaderStateBlock::getEmptyBlock());
  shaders.state_blocks = eastl::move(ShaderStateBlock::getBlocks());
  shaders.shader_classes = eastl::move(shader_class);
  shaders.render_states = eastl::move(render_states);
  shaders.shaders_stcode = eastl::move(shaders_stcode);

  // save FSH
  for (int i = 0; i < shaders_fsh.size(); i++)
  {
    Tab<uint32_t> code(shaders_fsh[i], tmpmem_ptr());
    shaders.shaders_fsh.push_back(eastl::move(code));
  }

  // save VPR
  for (int i = 0; i < shaders_vpr.size(); i++)
  {
    Tab<uint32_t> code(shaders_vpr[i], tmpmem_ptr());
    shaders.shaders_vpr.push_back(eastl::move(code));
  }

  bindump::VectorWriter mem_writer;
  bindump::streamWrite(shaders, mem_writer);

  if (mem_writer.mData.size() > (1 << 30))
  {
    compressed.compressed_shaders.resize(zstd_compress_bound(mem_writer.mData.size()));
    size_t compressed_size = zstd_compress(compressed.compressed_shaders.data(), compressed.compressed_shaders.size(),
      mem_writer.mData.data(), mem_writer.mData.size(), 9);
    G_ASSERTF(compressed_size < 0x20000000, "zstd_compress() returns %d (0x%x), srcDataSz=%u destBufSz=%u", (ptrdiff_t)compressed_size,
      compressed_size, mem_writer.mData.size(), compressed.compressed_shaders.size());

    compressed.compressed_shaders.resize(compressed_size);
    compressed.decompressed_size = mem_writer.mData.size();
  }
  else
  {
    compressed.compressed_shaders.resize(mem_writer.mData.size());
    eastl::copy(mem_writer.mData.begin(), mem_writer.mData.end(), compressed.compressed_shaders.begin());
    compressed.decompressed_size = 0;
  }

  bindump::writeToFileFast(compressed, filename);
}
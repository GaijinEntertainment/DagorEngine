// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "linkShaders.h"
#include "gitRunner.h"
#include "loadShaders.h"
#include "globalConfig.h"
#include "deSerializationContext.h"

#include "globVar.h"
#include "varMap.h"
#include "shcode.h"
#include "shaderSave.h"
#include "shLog.h"
#include "shCacheVer.h"
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
#include "cppStcode.h"

#include <debug/dag_debug.h>
#include <drv/3d/dag_renderStates.h>

#if _CROSS_TARGET_SPIRV
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#endif

void close_shader_class() { ShaderParser::clear_per_file_caches(); }

void add_shader_class(ShaderClass *sc, shc::TargetContext &ctx)
{
  if (!sc)
    return;
  ctx.storage().shaderClass.push_back(sc);
}

int add_fshader(dag::ConstSpan<unsigned> code, shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  if (!code.size())
    return -1;

  for (int i = stor.shadersFsh.size() - 1; i >= 0; i--)
  {
    if (stor.shadersFsh[i].size() == code.size() &&
        (stor.shadersFsh[i].data() == code.data() || mem_eq(code, stor.shadersFsh[i].data())))
    {
      return i;
    }
  }

  stor.shadersFsh.push_back(code);
  return stor.shadersFsh.size() - 1;
}

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>

int add_vprog(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs, dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs,
  shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  if (!vs.size())
    return -1;
  dag::ConstSpan<unsigned> code = vs;
  SmallTab<unsigned, TmpmemAlloc> *comp_prog = NULL;
#if _CROSS_TARGET_METAL
  G_ASSERT(hs.size() == 0);
  G_ASSERT(ds.size() == 0);
  G_ASSERT(vs.size());
  // if its mesh shader and company
  if (vs[0] == _MAKE4C('D11v'))
  {
    uint32_t header = _MAKE4C('DX9v'), new_header = _MAKE4C('MTLM'), ms_size = data_size(vs), as_size = data_size(gs);
    size_t combo_size = sizeof(header) + sizeof(new_header) + sizeof(ms_size) + ms_size + sizeof(as_size) + as_size;

    comp_prog = new SmallTab<unsigned, TmpmemAlloc>;
    clear_and_resize(*comp_prog, (combo_size + sizeof(unsigned) - 1) / sizeof(unsigned));

    uint32_t offset = 0;
    uint8_t *data = (uint8_t *)comp_prog->data();
    memcpy(data + offset, &header, sizeof(header));
    offset += sizeof(header);

    memcpy(data + offset, &new_header, sizeof(new_header));
    offset += sizeof(new_header);

    memcpy(data + offset, &ms_size, sizeof(ms_size));
    offset += sizeof(ms_size);
    memcpy(data + offset, vs.data(), ms_size);
    offset += ms_size;

    memcpy(data + offset, &as_size, sizeof(as_size));
    offset += sizeof(as_size);
    if (as_size)
    {
      memcpy(data + offset, gs.data(), as_size);
      offset += as_size;
    }

    code = make_span(*comp_prog);
  }
#elif !_CROSS_TARGET_EMPTY
  if (hs.size() || ds.size() || gs.size() || vs[0] == _MAKE4C('D11v'))
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
  for (int i = stor.shadersVpr.size() - 1; i >= 0; i--)
    if (stor.shadersVpr[i].size() == code.size() &&
        (stor.shadersVpr[i].data() == code.data() || mem_eq(code, stor.shadersVpr[i].data())))
    {
      if (comp_prog)
        delete comp_prog;
      return i;
    }
  if (comp_prog)
    stor.shadersCompProg.push_back(comp_prog);
  stor.shadersVpr.push_back(code);
  return stor.shadersVpr.size() - 1;
}

#if _CROSS_TARGET_DX12
static bool is_hlsl_debug() { return shc::config().hlslDebugLevel != DebugLevel::NONE; }

#if REPORT_RECOMPILE_PROGRESS
#define REPORT debug
#else
#define REPORT(...)
#endif

VertexProgramAndPixelShaderIdents add_phase_one_progs(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs,
  dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, dag::ConstSpan<unsigned> ps,
  SemanticShaderPass::EnableFp16State enableFp16, shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  VertexProgramAndPixelShaderIdents result{-1, -1};
  auto vShaderData = make_span(vs).subspan(vs.size() ? 1 : 0);
  auto hShaderData = make_span(hs).subspan(hs.size() ? 1 : 0);
  auto dShaderData = make_span(ds).subspan(ds.size() ? 1 : 0);
  auto gShaderData = make_span(gs).subspan(gs.size() ? 1 : 0);
  auto pShaderData = make_span(ps).subspan(ps.size() ? 1 : 0);

  // @TODO: this might not be an actual hard requirement, but only of our API, need to check
  if (enableFp16.vsOrMs != enableFp16.hs || enableFp16.hs != enableFp16.ds || enableFp16.ds != enableFp16.gsOrAs)
  {
    auto enabledStr = [](bool enabled) { return enabled ? "enabled" : "disabled"; };
    sh_debug(SHLOG_ERROR,
      "All vertex program shaders have to either have all halfs enabled or disabled for two-phase compilation.\n"
      "Got: vs: %s, hs: %s, ds: %s, gs: %s",
      enabledStr(enableFp16.vsOrMs), enabledStr(enableFp16.hs), enabledStr(enableFp16.ds), enabledStr(enableFp16.gsOrAs));
  }

  auto rootSignatureDefinition =
    dx12::dxil::generateRootSignatureDefinition(vShaderData, hShaderData, dShaderData, gShaderData, pShaderData);

  for (int i = stor.shadersVpr.size() - 1; i >= 0; i--)
  {
    if (dx12::dxil::comparePhaseOneVertexProgram(stor.shadersVpr[i], vShaderData, hShaderData, dShaderData, gShaderData,
          rootSignatureDefinition))
    {
      result.vprog = i;
      break;
    }
  }

  for (int i = stor.shadersFsh.size() - 1; i >= 0; i--)
  {
    if (dx12::dxil::comparePhaseOnePixelShader(stor.shadersFsh[i], pShaderData, rootSignatureDefinition))
    {
      result.fsh = i;
      break;
    }
  }

  dx12::dxil::CompilationOptions compileOptions;
  compileOptions.optimize = shc::config().hlslOptimizationLevel ? true : false;
  compileOptions.skipValidation = shc::config().hlslSkipValidation;
  compileOptions.debugInfo = is_hlsl_debug();
  compileOptions.scarlettW32 = useScarlettWave32;
  compileOptions.hlsl2021 = shc::config().hlsl2021;
  compileOptions.enableFp16 = enableFp16.vsOrMs;
  if (-1 == result.vprog)
  {
    auto packed = dx12::dxil::combinePhaseOneVertexProgram(vShaderData, hShaderData, dShaderData, gShaderData, rootSignatureDefinition,
      _MAKE4C('DX9v'), compileOptions);
    result.vprog = stor.shadersVpr.size();
    stor.shadersVpr.push_back(*packed);
    stor.shadersCompProg.push_back(packed.release());
  }

  compileOptions.enableFp16 = enableFp16.psOrCs;
  if (-1 == result.fsh)
  {
    auto packed = dx12::dxil::combinePhaseOnePixelShader(pShaderData, rootSignatureDefinition, _MAKE4C('DX9p'), !gShaderData.empty(),
      !hShaderData.empty() && !dShaderData.empty(), compileOptions);
    result.fsh = stor.shadersFsh.size();
    stor.shadersFsh.push_back(*packed);
    stor.shadersCompProg.push_back(packed.release());
  }

  return result;
}

namespace
{
struct RecompileJobBase : shc::Job
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
  shc::TargetContext &ctx;
  RecompileJobBase(size_t ct, shc::TargetContext &cref) : compileTarget{ct}, ctx{cref} {}

  eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>> load_cache(dag::ConstSpan<unsigned> uncompiled, const char *type_name)
  {
    if (!shc::config().useSha1Cache)
      return {};

    const char *profile_name = dx12::dxil::Platform::XBOX_ONE == shc::config().targetPlatform ? "x" : "xs";
    HASH_CONTEXT ctx;
    HASH_INIT(&ctx);
    HASH_UPDATE(&ctx, reinterpret_cast<const unsigned char *>(uncompiled.data()), data_size(uncompiled));
    HASH_FINISH(&ctx, ilHash);
    SNPRINTF(ilPath, sizeof(ilPath), "%s/il/%s/%s/" HASH_LIST_STRING, shc::config().sha1CacheDir, type_name, profile_name,
      HASH_LIST(ilHash));
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
    SNPRINTF(byteCodePath, sizeof(byteCodePath), "%s/bc/%s/%s/" HASH_LIST_STRING, shc::config().sha1CacheDir, type_name, profile_name,
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
    if (!shc::config().writeSha1Cache)
      return;

    const char *profile_name = dx12::dxil::Platform::XBOX_ONE == shc::config().targetPlatform ? "x" : "xs";
    MappingFileLayout mappingFile{};
    mappingFile.ilSize = data_size(uncompiled);
    mappingFile.byteCodeSize = data_size(compiled);
    mappingFile.byteCodeVersion = sha1_cache_version;
    HASH_CONTEXT ctx;
    HASH_INIT(&ctx);
    HASH_UPDATE(&ctx, reinterpret_cast<const unsigned char *>(uncompiled.data()), data_size(uncompiled));
    HASH_FINISH(&ctx, mappingFile.byteCodeHash);
    char bcPath[420];
    SNPRINTF(bcPath, sizeof(bcPath), "%s/bc/%s/%s/" HASH_LIST_STRING, shc::config().sha1CacheDir, type_name, profile_name,
      HASH_LIST(mappingFile.byteCodeHash));

    DagorStat bcInfo;
    char tempPath[DAGOR_MAX_PATH];
    if (df_stat(bcPath, &bcInfo) != -1)
    {
      // if (bcInfo.size != mappingFile.byteCodeSize) {}
    }
    else
    {
      SNPRINTF(tempPath, sizeof(tempPath), "%s/%s_bc_%s_" HASH_TEMP_STRING "XXXXXX", shc::config().sha1CacheDir, type_name,
        profile_name, HASH_LIST(mappingFile.byteCodeHash));
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
    SNPRINTF(tempPath, sizeof(tempPath), "%s/%s_il_%s_" HASH_TEMP_STRING "XXXXXX", shc::config().sha1CacheDir, type_name, profile_name,
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

struct RecompileVPRogJob : RecompileJobBase
{
  using RecompileJobBase::RecompileJobBase;

  void doJobBody() override
  {
    ShaderTargetStorage &stor = ctx.storage();

    static const char cache_name[] = "vp";
    bool writeCache = false;
    auto recompiledVProgBlob = load_cache(stor.shadersVpr[compileTarget], cache_name);
    if (!recompiledVProgBlob)
    {
      writeCache = true;
      auto recompiledVProg = dx12::dxil::recompileVertexProgram(stor.shadersVpr[compileTarget], shc::config().targetPlatform,
        shc::config().dx12PdbCacheDir, shc::config().hlslDebugLevel, shc::config().hlslEmbedSource);
      if (!recompiledVProg)
      {
        sh_debug(SHLOG_FATAL, "Recompilation of vprog failed");
      }
      recompiledVProgBlob = eastl::move(*recompiledVProg);
    }
    if (recompiledVProgBlob)
    {
      auto ref = eastl::find_if(eastl::begin(stor.shadersCompProg), eastl::end(stor.shadersCompProg),
        [cmp = stor.shadersVpr[compileTarget].data()](auto tab) { return cmp == tab->data(); });

      REPORT("vprog size %u -> %u", data_size(stor.shadersVpr[compileTarget]), data_size(*recompiledVProgBlob));

      if (writeCache)
      {
        store_cache(stor.shadersVpr[compileTarget], *recompiledVProgBlob, cache_name);
      }
      stor.shadersVpr[compileTarget] = *recompiledVProgBlob;

      G_ASSERT(ref != eastl::end(stor.shadersCompProg));
      if (ref != eastl::end(stor.shadersCompProg))
      {
        delete *ref;
        *ref = recompiledVProgBlob.release();
      }
    }
  }
  void releaseJobBody() {}
};

struct RecompilePShJob : RecompileJobBase
{
  using RecompileJobBase::RecompileJobBase;

  void doJobBody() override
  {
    ShaderTargetStorage &stor = ctx.storage();

    static const char cache_name[] = "fs";
    bool writeCache = false;
    auto recompiledFShBlob = load_cache(stor.shadersFsh[compileTarget], cache_name);
    if (!recompiledFShBlob)
    {
      writeCache = true;
      auto recompiledFSh = dx12::dxil::recompilePixelSader(stor.shadersFsh[compileTarget], shc::config().targetPlatform,
        shc::config().dx12PdbCacheDir, shc::config().hlslDebugLevel, shc::config().hlslEmbedSource);
      if (!recompiledFSh)
      {
        sh_debug(SHLOG_FATAL, "Recompilation of fsh failed");
      }
      recompiledFShBlob = eastl::move(*recompiledFSh);
    }
    if (recompiledFShBlob)
    {
      auto ref = eastl::find_if(eastl::begin(stor.shadersCompProg), eastl::end(stor.shadersCompProg),
        [cmp = stor.shadersFsh[compileTarget].data()](auto tab) { return cmp == tab->data(); });

      REPORT("fsh size %u -> %u", data_size(stor.shadersFsh[compileTarget]), data_size(*recompiledFShBlob));

      if (writeCache)
      {
        store_cache(stor.shadersFsh[compileTarget], *recompiledFShBlob, cache_name);
      }
      stor.shadersFsh[compileTarget] = *recompiledFShBlob;

      G_ASSERT(ref != eastl::end(stor.shadersCompProg));
      if (ref != eastl::end(stor.shadersCompProg))
      {
        delete *ref;
        *ref = recompiledFShBlob.release();
      }
    }
  }
  void releaseJobBody() override {}
};
} // namespace

struct IndexReplacmentInfo
{
  size_t target;
  size_t old;
};

static eastl::vector<IndexReplacmentInfo> dedup_table(Tab<dag::ConstSpan<unsigned>> &table, const char *name)
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

static eastl::vector<IndexReplacmentInfo> empty_fill_table(Tab<dag::ConstSpan<unsigned>> &table, const char *name)
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

static int16_t patch_index(int16_t old, const eastl::vector<IndexReplacmentInfo> &table, const char *name)
{
  auto ref = eastl::find_if(begin(table), end(table), [old](auto &info) { return info.old == old; });
  if (ref == end(table))
    return old;
  REPORT("patching %s id from %u to %u", name, old, ref->target);
  return static_cast<int16_t>(ref->target);
}

static void patch_passes(const eastl::vector<IndexReplacmentInfo> &vprg, const eastl::vector<IndexReplacmentInfo> &fsh,
  shc::TargetContext &ctx)
{
  for (ShaderClass *sh : ctx.storage().shaderClass)
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

static void dedup_shaders(shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  auto vproCount = stor.shadersVpr.size();
  auto fshCount = stor.shadersFsh.size();

  auto vprogDups = dedup_table(stor.shadersVpr, "vprog");
  auto fshDups = dedup_table(stor.shadersFsh, "fsh");

  if (vprogDups.empty() && fshDups.empty())
  {
    REPORT("No duplicates found...");
    return;
  }

  patch_passes(vprogDups, fshDups, ctx);

  vprogDups = empty_fill_table(stor.shadersVpr, "vprog");
  fshDups = empty_fill_table(stor.shadersFsh, "fsh");

  patch_passes(vprogDups, fshDups, ctx);

  REPORT("Removed %u (%u - %u) vprogs...", vproCount - stor.shadersVpr.size(), vproCount, stor.shadersVpr.size());
  REPORT("Removed %u (%u - %u) fshs...", fshCount - stor.shadersFsh.size(), fshCount, stor.shadersFsh.size());
}

void recompile_shaders(shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  if (shc::is_multithreaded())
  {
    eastl::vector<RecompileVPRogJob> vprogRecompileJobList;
    eastl::vector<RecompilePShJob> fshRecompileJobList;
    eastl::vector<RecompileJobBase *> allJobs;
    vprogRecompileJobList.reserve(stor.shadersVpr.size());
    fshRecompileJobList.reserve(stor.shadersFsh.size());

    // generate all jobs first
    for (size_t i = 0; i < stor.shadersVpr.size(); ++i)
    {
      vprogRecompileJobList.emplace_back(i, ctx);
    }
    for (size_t i = 0; i < stor.shadersFsh.size(); ++i)
    {
      fshRecompileJobList.emplace_back(i, ctx);
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

    size_t issueCount = 0;
    while (issueCount < allJobs.size())
      shc::add_job(allJobs[issueCount++], shc::JobMgrChoiceStrategy::LEAST_BUSY_COOPERATIVE);

    shc::await_all_jobs();
  }
  else
  {
    for (size_t i = 0; i < stor.shadersVpr.size(); ++i)
    {
      RecompileVPRogJob job{i, ctx};
      job.doJob();
      job.releaseJob();
    }
    for (size_t i = 0; i < stor.shadersFsh.size(); ++i)
    {
      RecompilePShJob job{i, ctx};
      job.doJob();
      job.releaseJob();
    }
  }

  dedup_shaders(ctx);
}
#endif

int add_stcode(dag::ConstSpan<int> code, shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  if (!code.size())
    return -1;

  for (int i = stor.shadersStcode.size() - 1; i >= 0; i--)
    if (stor.shadersStcode[i].size() == code.size() && mem_eq(code, stor.shadersStcode[i].data()))
      return i;

  int i = append_items(stor.shadersStcode, 1);
  stor.shadersStcode[i].Tab<int>::operator=(code);
  stor.shadersStcode[i].shrink_to_fit();
  return i;
}

void add_stcode_validation_mask(int stcode_id, shader_layout::StcodeConstValidationMask *mask, shc::TargetContext &ctx)
{
  if (stcode_id < 0)
    return;

  G_ASSERT(mask);
  auto &masks = ctx.storage().stcodeConstValidationMasks;
  if (masks.size() <= stcode_id)
    masks.resize(stcode_id + 1, nullptr);
  auto &dst = masks[stcode_id];
  if (dst)
    dst->merge(*mask);
  else
    dst = mask;
}

static int add_render_state(const shaders::RenderState &rs, shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  for (int i = 0; i < stor.renderStates.size(); ++i)
    if (rs == stor.renderStates[i])
      return i;
  stor.renderStates.push_back(rs);
  return static_cast<int>(stor.renderStates.size()) - 1;
}

int add_render_state(const SemanticShaderPass &state, shc::TargetContext &ctx)
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
    rs.dualSourceBlendEnabled = state.dual_source_blending;
    rs.blendFactor = state.blend_factor;
    rs.blendFactorUsed = state.blend_factor_specified;
    if (rs.dualSourceBlendEnabled)
    {
      rs.dualSourceBlend.params.ablendFactors.src = state.blend_src[0];
      rs.dualSourceBlend.params.ablendFactors.dst = state.blend_dst[0];
      rs.dualSourceBlend.params.sepablendFactors.src = state.blend_asrc[0];
      rs.dualSourceBlend.params.sepablendFactors.dst = state.blend_adst[0];
      rs.dualSourceBlend.params.ablend = state.blend_src[0] >= 0 && state.blend_dst[0] >= 0 ? 1 : 0;
      rs.dualSourceBlend.params.sepablend = state.blend_asrc[0] >= 0 && state.blend_adst[0] >= 0 ? 1 : 0;
      rs.dualSourceBlend.params.blendOp = state.blend_op[0];
      rs.dualSourceBlend.params.sepablendOp = state.blend_aop[0];
    }
    else
    {
      for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
      {
        rs.blendParams[i].ablendFactors.src = state.blend_src[i];
        rs.blendParams[i].ablendFactors.dst = state.blend_dst[i];
        rs.blendParams[i].sepablendFactors.src = state.blend_asrc[i];
        rs.blendParams[i].sepablendFactors.dst = state.blend_adst[i];
        rs.blendParams[i].ablend = state.blend_src[i] >= 0 && state.blend_dst[i] >= 0 ? 1 : 0;
        rs.blendParams[i].sepablend = state.blend_asrc[i] >= 0 && state.blend_adst[i] >= 0 ? 1 : 0;
        rs.blendParams[i].blendOp = state.blend_op[i];
        rs.blendParams[i].sepablendOp = state.blend_aop[i];
      }
    }
  }
  else
  {
    rs.independentBlendEnabled = 0;
    rs.dualSourceBlendEnabled = 0;
    rs.blendFactorUsed = 0;
    for (auto &blendParam : rs.blendParams)
    {
      blendParam.ablend = 0;
      blendParam.sepablend = 0;
    }
    rs.blendFactor = E3DCOLOR{0u};
  }
  rs.cull = state.cull_mode;
  rs.alphaToCoverage = state.alpha_to_coverage;
  rs.viewInstanceCount = state.view_instances;
  rs.colorWr = state.color_write;
  rs.zBias = state.z_bias ? -state.z_bias_val / 1000.f : 0;
  rs.slopeZBias = state.slope_z_bias ? state.slope_z_bias_val : 0;

  return add_render_state(rs, ctx);
}

static int ld_add_fshader(TabFsh &code, int upper, shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  if (!code.size())
    return -1;

  for (int i = 0; i < upper; ++i)
    if (stor.shadersFsh[i].size() == code.size() && mem_eq(code, stor.shadersFsh[i].data()))
      return i;

  int i = append_items(stor.ldShFsh, 1);
  stor.ldShFsh[i] = eastl::move(code);
  stor.ldShFsh[i].shrink_to_fit();
  stor.shadersFsh.push_back(stor.ldShFsh[i]);
  return stor.shadersFsh.size() - 1;
}

static int ld_add_vprog(TabVpr &code, int upper, shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  if (!code.size())
    return -1;

  for (int i = 0; i < upper; ++i)
    if (stor.shadersVpr[i].size() == code.size() && mem_eq(code, stor.shadersVpr[i].data()))
      return i;

  int i = append_items(stor.ldShVpr, 1);
  stor.ldShVpr[i] = eastl::move(code);
  stor.ldShVpr[i].shrink_to_fit();
  stor.shadersVpr.push_back(stor.ldShVpr[i]);
  return stor.shadersVpr.size() - 1;
}

void count_shader_stats(unsigned &uniqueFshBytesInFile, unsigned &uniqueFshCountInFile, unsigned &uniqueVprBytesInFile,
  unsigned &uniqueVprCountInFile, unsigned &stcodeBytes, const shc::TargetContext &ctx)
{
  const ShaderTargetStorage &stor = ctx.storage();

  int i;

  uniqueFshCountInFile = stor.shadersFsh.size();
  uniqueVprCountInFile = stor.shadersVpr.size();
  uniqueFshBytesInFile = 0;
  uniqueVprBytesInFile = 0;
  stcodeBytes = 0;

  for (i = 0; i < stor.shadersFsh.size(); i++)
    uniqueFshBytesInFile += data_size(stor.shadersFsh[i]);
  for (i = 0; i < stor.shadersVpr.size(); i++)
    uniqueVprBytesInFile += data_size(stor.shadersVpr[i]);

  for (i = 0; i < stor.shadersStcode.size(); i++)
    stcodeBytes += data_size(stor.shadersStcode[i]);
}

static void link_shaders_fsh_and_vpr(ShadersBindump &bindump, Tab<int> &fsh_lnktbl, Tab<int> &vpr_lnktbl, shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  // load FSH
  int fshNum = bindump.shadersFsh.size();
  fsh_lnktbl.resize(fshNum);
  int upper = stor.shadersFsh.size();
  for (int i = 0; i < fshNum; i++)
    fsh_lnktbl[i] = ld_add_fshader(bindump.shadersFsh[i], upper, ctx);

  // load VPR
  int vprNum = bindump.shadersVpr.size();
  vpr_lnktbl.resize(vprNum);
  upper = stor.shadersVpr.size();
  for (int i = 0; i < vprNum; i++)
    vpr_lnktbl[i] = ld_add_vprog(bindump.shadersVpr[i], upper, ctx);
}

bool load_shaders_bindump(ShadersBindump &shaders, bindump::IReader &full_file_reader, shc::TargetContext &ctx)
{
  ShadersDeSerializationScope deSerScope{ctx};

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

bool link_scripted_shaders(const uint8_t *mapped_data, int data_size, const char *filename, const char *source_name,
  shc::TargetContext &ctx)
{
  ShadersBindump shaders;
  bindump::MemoryReader compressed_reader(mapped_data, data_size);
  if (!load_shaders_bindump(shaders, compressed_reader, ctx))
    sh_debug(SHLOG_FATAL, "corrupted OBJ file: %s", filename);

  // Load render states
  int renderStatesCount = shaders.renderStates.size();
  Tab<int> renderStateLinkTable(tmpmem);
  renderStateLinkTable.resize(renderStatesCount);
  for (int i = 0; i < renderStatesCount; ++i)
    renderStateLinkTable[i] = add_render_state(shaders.renderStates[i], ctx);

  Tab<int> fsh_lnktbl(tmpmem);
  Tab<int> vpr_lnktbl(tmpmem);
  link_shaders_fsh_and_vpr(shaders, fsh_lnktbl, vpr_lnktbl, ctx);

  // Load stcode.
  Tab<TabStcode> &shaderStcodeFromFile = shaders.shadersStcode;

  Tab<int> global_var_link_table(tmpmem);
  Tab<ShaderVariant::ExtType> interval_link_table(tmpmem);
  ctx.globVars().link(shaders.variable_list, shaders.intervals, global_var_link_table, interval_link_table);

  Tab<int> smp_link_table;
  ctx.samplers().link(shaders.static_samplers, smp_link_table);

  // Link stcode, stblkcode.
  Tab<int> stcode_lnktbl(tmpmem);
  stcode_lnktbl.resize(shaderStcodeFromFile.size());
  for (int i = 0; i < shaderStcodeFromFile.size(); i++)
  {
    bindumphlp::patchStCode(make_span(shaderStcodeFromFile[i]), global_var_link_table, smp_link_table);
    stcode_lnktbl[i] = add_stcode(shaderStcodeFromFile[i], ctx);
    if (shc::config().generateCppStcodeValidationData)
      add_stcode_validation_mask(stcode_lnktbl[i], eastl::exchange(shaders.stcodeConstMasks[i], nullptr), ctx);
  }

  for (auto &ptr : shaders.stcodeConstMasks)
  {
    if (ptr)
      delete eastl::exchange(ptr, nullptr);
  }

  // Link cpp stcode, and save routine remapping table to fill pass indices
  auto [dynamicCppStcodeRemappingTable, staticCppStcodeRemappingTable] =
    ctx.cppStcode().linkRoutinesFromFile(shaders.dynamicCppcodeHashes, shaders.staticCppcodeHashes, source_name);

  // Link blocks
  ctx.blocks().link(shaders.state_blocks, stcode_lnktbl, dynamicCppStcodeRemappingTable);

  // load shaders
  ctx.storage().shaderClass.reserve(shaders.shaderClasses.size());

  // For branched cpp stcode, filled along the way
  eastl::vector_map<int, int> dynamicOffsetRemappingTable{};

  for (ShaderClass *classFromFile : shaders.shaderClasses)
  {
    for (ShaderClass *existingClass : ctx.storage().shaderClass)
    {
      if (existingClass->name == classFromFile->name)
        sh_debug(SHLOG_ERROR, "Duplicated shader '%s'", classFromFile->name.c_str());
    }

    int total_passes = 0, reduced_passes = 0;

    classFromFile->staticVariants.setContextRef(ctx);
    classFromFile->staticVariants.linkIntervalList();

    for (ShaderCode *code : classFromFile->code)
    {
      int id;

      code->link(ctx);

      code->dynVariants.setContextRef(ctx);
      code->dynVariants.link(interval_link_table);

      if (code->branchedCppStcodeId >= 0)
      {
        G_ASSERT(code->branchedCppStcodeId < dynamicCppStcodeRemappingTable.size());
        code->branchedCppStcodeId = dynamicCppStcodeRemappingTable[code->branchedCppStcodeId];
      }

      if (code->branchedCppStblkcodeId >= 0)
      {
        G_ASSERT(code->branchedCppStblkcodeId < staticCppStcodeRemappingTable.size());
        code->branchedCppStblkcodeId = staticCppStcodeRemappingTable[code->branchedCppStblkcodeId];
      }

      for (ShaderCode::Pass &pass : code->allPasses)
      {
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

        if (pass.branchlessCppStcodeId >= 0)
        {
          G_ASSERT(pass.branchlessCppStcodeId < dynamicCppStcodeRemappingTable.size());
          pass.branchlessCppStcodeId = dynamicCppStcodeRemappingTable[pass.branchlessCppStcodeId];
        }

        if (pass.branchlessCppStblkcodeId >= 0)
        {
          G_ASSERT(pass.branchlessCppStblkcodeId < staticCppStcodeRemappingTable.size());
          pass.branchlessCppStblkcodeId = staticCppStcodeRemappingTable[pass.branchlessCppStblkcodeId];
        }

        if (pass.branchedCppStcodeTableOffset >= 0)
        {
          auto [it, inserted] = dynamicOffsetRemappingTable.emplace(pass.branchedCppStcodeTableOffset, -1);
          if (inserted)
          {
            it->second = ctx.cppStcode().addRegisterTableWithOffset(
              eastl::move(shaders.cppcodeRegisterTables[pass.branchedCppStcodeTableOffset]));
          }

          pass.branchedCppStcodeTableOffset = it->second;
        }

        if (pass.branchedCppStblkcodeTableOffset >= 0)
        {
          auto [it, inserted] = dynamicOffsetRemappingTable.emplace(pass.branchedCppStblkcodeTableOffset, -1);
          if (inserted)
          {
            it->second = ctx.cppStcode().addRegisterTableWithOffset(
              eastl::move(shaders.cppcodeRegisterTables[pass.branchedCppStblkcodeTableOffset]));
          }

          pass.branchedCppStblkcodeTableOffset = it->second;
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

      clear_and_resize(idxmap, code->passes.size());
      mem_set_ff(idxmap);

      for (unsigned int passNo = 0; passNo < idxmap.size(); passNo++)
      {
        ShaderCode::PassTab *p = code->passes[passNo];

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
            code->passes[passNo] = NULL;
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
        for (int i = code->dynVariants.getVarCount() - 1; i >= 0; i--)
        {
          G_ASSERT(code->dynVariants.getVariant(i)->codeId < (int)idxmap.size());
          if (code->dynVariants.getVariant(i)->codeId >= 0)
            code->dynVariants.getVariant(i)->codeId = idxmap[code->dynVariants.getVariant(i)->codeId];
        }
        code->passes = eastl::move(newpass);
      }
    }
    ctx.storage().shaderClass.push_back(classFromFile);
  }
  return true;
}

void save_scripted_shaders(const char *filename, dag::ConstSpan<SimpleString> files, shc::TargetContext &ctx,
  bool need_cppstcode_file_write)
{
  ShadersDeSerializationScope deSerScope{ctx};

  CompressedShadersBindump compressed;
  ShadersBindump shaders;

  compressed.cache_sign = SHADER_CACHE_SIGN;
  compressed.cache_version = SHADER_CACHE_VER;
  compressed.last_blk_hash = ctx.compCtx().compInfo().targetBlkHash();
  compressed.eof = SHADER_CACHE_EOF;

  // save dependens
  for (int i = 0; i < files.size(); ++i)
    compressed.dependency_files.emplace_back(files[i].c_str());

  shaders.variable_list = eastl::move(ctx.globVars().getMutableVariableList());
  shaders.static_samplers = ctx.samplers().releaseSamplers();
  shaders.intervals = eastl::move(ctx.globVars().getMutableIntervalList());
  shaders.empty_block = ctx.blocks().releaseEmptyBlock();
  shaders.state_blocks = ctx.blocks().release();
  shaders.shaderClasses = eastl::move(ctx.storage().shaderClass);
  shaders.renderStates = eastl::move(ctx.storage().renderStates);

  if (shc::config().generateCppStcodeValidationData)
    G_ASSERT(shaders.stcodeConstMasks.size() == shaders.shadersStcode.size());
  else
    G_ASSERT(shaders.stcodeConstMasks.empty());

  shaders.shadersStcode = eastl::move(ctx.storage().shadersStcode);
  shaders.stcodeConstMasks = eastl::move(ctx.storage().stcodeConstValidationMasks);

  shaders.dynamicCppcodeHashes = eastl::move(ctx.cppStcode().dynamicRoutineHashes);
  shaders.staticCppcodeHashes = eastl::move(ctx.cppStcode().staticRoutineHashes);
  shaders.dynamicCppcodeRoutineNames = eastl::move(ctx.cppStcode().dynamicRoutineNames);
  shaders.staticCppcodeRoutineNames = eastl::move(ctx.cppStcode().staticRoutineNames);

  shaders.cppcodeRegisterTables.reserve(ctx.cppStcode().regTable.combinedTable.size());
  for (auto &&t : ctx.cppStcode().regTable.combinedTable)
    shaders.cppcodeRegisterTables.push_back(eastl::move(t));
  shaders.cppcodeRegisterTableOffsets = eastl::move(ctx.cppStcode().regTable.offsets);

  // save FSH
  for (const auto &fsh : ctx.storage().shadersFsh)
  {
    Tab<uint32_t> code(fsh, tmpmem_ptr());
    shaders.shadersFsh.push_back(eastl::move(code));
  }

  // save VPR
  for (const auto &vpr : ctx.storage().shadersVpr)
  {
    Tab<uint32_t> code(vpr, tmpmem_ptr());
    shaders.shadersVpr.push_back(eastl::move(code));
  }

  // 4gb, cause eastl asserts this size, dag::Vector with default settings uses 32bit indices, and a lot of the bindump library code
  // also assumes 32 bits is enough
  constexpr size_t MAX_UNCOMPRESSED_SIZE = (1ull << 32) - 1;

  bindump::SizeMetter sizeChecker;
  bindump::streamWrite(shaders, sizeChecker);

  if (sizeChecker.mSize > MAX_UNCOMPRESSED_SIZE)
  {
    sh_debug(SHLOG_FATAL,
      "The shaders bindump is too large (%llu bytes uncompressed, doesn't fit into 32-bit indexing), "
      "consider disabling global debug info options, or reducing the shader set",
      sizeChecker.mSize);
    return;
  }

  bindump::VectorWriter mem_writer;
  bindump::streamWrite(shaders, mem_writer);

  if (mem_writer.mData.size() > (1u << 30))
  {
    compressed.compressed_shaders.resize(zstd_compress_bound(mem_writer.mData.size()));
    size_t compressed_size = zstd_compress(compressed.compressed_shaders.data(), compressed.compressed_shaders.size(),
      mem_writer.mData.data(), mem_writer.mData.size(), 9);

    const size_t maxCompressedSize = 0x20000000;
    if (shc::config().constrainCompressedBindumpSize && compressed_size > maxCompressedSize)
    {
      sh_debug(SHLOG_FATAL,
        "Compressed bindump size is too large, zstd_compress() returns %lld (0x%llx), srcDataSz=%llu destBufSz=%llu, "
        "while cap is %llu, consider disabling global debug info options, or reducing the shader set",
        (ptrdiff_t)compressed_size, compressed_size, mem_writer.mData.size(), compressed.compressed_shaders.size(), maxCompressedSize);
      return;
    }

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

  if (need_cppstcode_file_write)
    save_compiled_cpp_stcode(eastl::move(ctx.cppStcode()), ctx.compCtx().compInfo());
}

void update_shaders_timestamps(dag::ConstSpan<SimpleString> dependencies, shc::TargetContext &ctx)
{
  // Shader class timestamp feature is only used for DX12
#if _CROSS_TARGET_DX12
  int64_t mostRecentTimestamp = 0;

  if (shc::config().useGitTimestamps)
    mostRecentTimestamp = get_git_files_last_commit_timestamp(dependencies);
  else
    sh_debug(SHLOG_INFO, "Timestamps are disabled by the flag useGitTimestamps, using 0 as timestamp");
#else
  G_UNUSED(dependencies);
  int64_t mostRecentTimestamp = 0;
#endif
  for (ShaderClass *cls : ctx.storage().shaderClass)
    cls->timestamp = mostRecentTimestamp;
}

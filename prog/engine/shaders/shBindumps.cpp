// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shBindumpsPrivate.h"
#include "shStateBlk.h"
#include "mapBinarySearch.h"
#include <shaders/shLimits.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shUtils.h>
#include <drv/3d/dag_lock.h>
#include <util/dag_generationReferencedData.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_finally.h>
#include <math/dag_mathBase.h>

bool ScriptedShadersBinDumpOwner::linkAgaistGlobalData(ScriptedShadersGlobalData const &global_link_data, bool dump_changed)
{
  G_ASSERTF_RETURN(global_link_data.backing->getDumpV4(), false, "Linking additional bindumps requires V4 dump header");
  G_ASSERTF_RETURN(mShaderDumpV4, false, "Linking additional bindumps requires V4 dump header");

  auto const &mainDumpOwner = *global_link_data.backing;
  auto const &mainDump = *mainDumpOwner.getDump();
  auto const &mainDumpV4 = *mainDumpOwner.getDumpV4();
  auto const &gvState = global_link_data.globVarsState;

  Tab<int32_t> gvarLinkMap(mShaderDump->gvMap.size(), -1);
  Tab<int32_t> intervalLinkMap(mShaderDump->intervals.size(), -1);

  eastl::vector_map<uint32_t, uint32_t> newGvarInverseLinkMap{};
  eastl::vector_map<uint32_t, uint32_t> newIntervalInverseLinkMap{};
  eastl::vector_map<shader_layout::blk_word_t, shader_layout::blk_word_t> newBlockInverseLinkMap{};

  // global vars
  for (uint32_t gvMapping : mShaderDump->gvMap)
  {
    uint32_t dumpNameId = gvMapping & 0xFFFF;
    uint32_t dumpGvarId = gvMapping >> 16;

    auto const &gvarName = mShaderDump->varMap[dumpNameId];
    auto metadata = mShaderDumpV4->globVarsMetadata[dumpGvarId];

    if (!metadata.isRef && !metadata.isLiteral)
    {
      logerr("Trying to load primary bindump as an additional dump, contains mutable non-ref global var '%s'.", gvarName.c_str());
      return false;
    }

    int nid = VariableMap::getVariableId(gvarName.c_str());
    int mainDumpGvarId = global_link_data.globvarIndexMap[nid];
    if (mainDumpGvarId == SHADERVAR_IDX_ABSENT || mainDumpGvarId == SHADERVAR_IDX_INVALID)
    {
      logerr("Failed to link additional dump, referenced global variable '%s' not found in primary dump.", gvarName.c_str());
      return false;
    }

    auto const &rec = mShaderDump->globVars.v[dumpGvarId];
    auto const &mainRec = mainDump.globVars.v[mainDumpGvarId];
    auto mainMetadata = mainDumpV4.globVarsMetadata[mainDumpGvarId];
    if (rec.type != mainRec.type)
    {
      logerr("Failed to link additional dump, inconsistent type of gvar '%s'.", gvarName.c_str());
      return false;
    }

    if (metadata.isLiteral)
    {
      if (metadata.isLiteral != mainMetadata.isLiteral)
      {
        logerr("Failed to link additional dump, inconsistent literal state of gvar '%s'.", gvarName.c_str());
        return false;
      }

      bool literalsAreEqual = false;
      switch (rec.type)
      {
        case SHVT_INT: literalsAreEqual = mShaderDump->globVars.get<int>(dumpGvarId) == gvState.get<int>(mainDumpGvarId); break;
        case SHVT_INT4:
          literalsAreEqual = mShaderDump->globVars.get<IPoint4>(dumpGvarId) == gvState.get<IPoint4>(mainDumpGvarId);
          break;
        case SHVT_REAL: literalsAreEqual = mShaderDump->globVars.get<float>(dumpGvarId) == gvState.get<float>(mainDumpGvarId); break;
        case SHVT_COLOR4:
          literalsAreEqual = mShaderDump->globVars.get<Color4>(dumpGvarId) == gvState.get<Color4>(mainDumpGvarId);
          break;

        // @TODO: here we also would want the AST hashes for expression-based fields, but they are not in the dump yet
        case SHVT_SAMPLER:
          literalsAreEqual = mShaderDump->globVars.get<d3d::SamplerInfo>(dumpGvarId) == gvState.get<d3d::SamplerInfo>(mainDumpGvarId);
          break;

        default:
          logerr("Failed to link additional dump, invalid type %d for literal gvar '%s'.", rec.type, gvarName.c_str());
          return false;
      }

      if (!literalsAreEqual)
      {
        logerr("Failed to link additional dump, non-matching literal values of gvar '%s'.", gvarName.c_str());
        return false;
      }
    }
    else
    {
      G_ASSERT_CONTINUE(metadata.isRef);
    }

    gvarLinkMap[dumpGvarId] = mainDumpGvarId;
    newGvarInverseLinkMap.emplace(mainDumpGvarId, dumpGvarId);
  }

  // intervals
  for (int i = 0; auto const &interval : mShaderDump->intervals) // enumerate doesn't work well with bindump types
  {
    if (interval.type == interval.TYPE_GLOBAL_INTERVAL || interval.type == interval.TYPE_ASSUMED_INTERVAL)
    {
      int dumpGvarId = bfind_packed_uint16_x2(mShaderDump->gvMap, interval.nameId);
      if (dumpGvarId >= 0)
      {
        int mainDumpGvarId = gvarLinkMap[dumpGvarId];
        int mainDumpIntervalId = global_link_data.globVarIntervalIdx[mainDumpGvarId];

        if (mainDumpIntervalId < 0)
        {
          logerr("Failed to link additional dump, referenced interval '%s' is not found in the main dump.", name.c_str());
          return false;
        }

        char const *intervalName = mShaderDump->varMap[interval.nameId].c_str();

        auto const &mainInterval = mainDump.intervals[mainDumpIntervalId];
        if (interval.type != mainInterval.type)
        {
          logerr("Failed to link additional dump, inconsistent type of interval '%s'.", intervalName);
          return false;
        }
        if (interval.maxVal.size() != mainInterval.maxVal.size())
        {
          if (interval.type == interval.TYPE_ASSUMED_INTERVAL)
            logerr("Failed to link additional dump, inconsistent assumed value of interval '%s'.", intervalName);
          else
            logerr("Failed to link additional dump, inconsistent value count of interval '%s'.", intervalName);
          return false;
        }
        for (size_t j = 0; j < interval.maxVal.size(); ++j)
        {
          if (!is_relative_equal_float(interval.maxVal[j], mainInterval.maxVal[j]))
          {
            logerr("Failed to link additional dump, inconsistent values of interval '%s'.", intervalName);
            return false;
          }
        }

        // @TODO: validate interval infos from V2 header for extra paranoia

        intervalLinkMap[i] = mainDumpIntervalId;
        newIntervalInverseLinkMap.emplace(mainDumpIntervalId, i);
      }
    }

    ++i;
  }

  // Now, we patch the stcode with linked ids
  for (size_t stcodeId = 0; stcodeId < mShaderDump->stcode.size(); ++stcodeId)
  {
    auto &stcode = mShaderDump->stcode[stcodeId];

    if (stcode.empty() || stcode[0] == SHCOD_STATIC_BLOCK || stcode[0] == SHCOD_STATIC_MULTIDRAW_BLOCK)
      continue;

    for (int i = 0; i < stcode.size(); i++)
    {
      int &opc = stcode[i];
      int op = shaderopcode::getOp(opc);
      switch (op)
      {
        case SHCOD_GET_GVEC:
        case SHCOD_GET_GMAT44:
        case SHCOD_GET_GREAL:
        case SHCOD_GET_GTEX:
        case SHCOD_GET_GBUF:
        case SHCOD_GET_GTLAS:
        case SHCOD_GET_GINT:
        case SHCOD_GET_GINT_TOREAL:
        case SHCOD_GET_GIVEC_TOREAL:
        {
          uint32_t const ro = shaderopcode::getOp2p1(opc);
          uint32_t const index = shaderopcode::getOp2p2(opc);
          uint32_t const mappedIndex = gvarLinkMap[dump_changed ? index : gvarInverseLinkMap[index]];
          opc = shaderopcode::makeOp2(op, ro, mappedIndex);
          break;
        }

        case SHCOD_GLOB_SAMPLER:
        {
          uint32_t const stage = shaderopcode::getOpStageSlot_Stage(opc);
          uint32_t const slot = shaderopcode::getOpStageSlot_Slot(opc);
          uint32_t const id = shaderopcode::getOpStageSlot_Reg(opc);
          uint32_t const mappedIndex = gvarLinkMap[dump_changed ? id : gvarInverseLinkMap[id]];
          opc = shaderopcode::makeOpStageSlot(op, stage, slot, mappedIndex);
          break;
        }

        case SHCOD_IMM_REAL:
        case SHCOD_MAKE_VEC: i++; break;
        case SHCOD_IMM_VEC: i += 4; break;
        case SHCOD_CALL_FUNCTION: i += shaderopcode::getOpFunctionCall_ArgCount(opc); break;

        default: break;
      }
    }
  }

  // And the interval references with linked ids
  for (shader_layout::VariantTable<>::IntervalBind &intervalBind : mShaderDump->vtPcsStorage)
  {
    int mapIndex = -1;
    if (dump_changed)
    {
      mapIndex = intervalBind.intervalId;
    }
    else
    {
      if (auto it = intervalInverseLinkMap.find(intervalBind.intervalId); it != intervalInverseLinkMap.end())
        mapIndex = it->second;
    }
    if (mapIndex >= 0 && mapIndex < intervalLinkMap.size() && intervalLinkMap[mapIndex] >= 0)
      intervalBind.intervalId = intervalLinkMap[mapIndex];
  }

  for (auto &mapping : mShaderDumpV3->immutableSamplersMap)
  {
    if (mapping.globalVarId >= 0)
      mapping.globalVarId = gvarLinkMap[dump_changed ? mapping.globalVarId : gvarInverseLinkMap[mapping.globalVarId]];
  }

  // Finally, link blocks (can be done only after stcode patching for correct comparison)
  eastl::vector_map<shader_layout::blk_word_t, shader_layout::blk_word_t> blockUidToMainUidMap{};

  for (int i = 0; auto &block : mShaderDump->blocks)
  {
    char const *blockName = mShaderDump->blockNameMap[block.nameId].c_str();

    int blockNid = ShaderGlobal::getBlockId(blockName);
    if (blockNid < 0)
    {
      logerr("Failed to link additional dump, referenced block '%s' is not found in the main dump.", blockName);
      return false;
    }

    int mainDumpBlockId = global_link_data.shaderBlockIndexMap[blockNid];
    auto const &mainBlock = mainDump.blocks[mainDumpBlockId];

    if (block.stcodeId == -1 || mainBlock.stcodeId == -1)
    {
      if (block.stcodeId >= 0 || mainBlock.stcodeId >= 0)
      {
        logerr("Failed to link additional dump, inconsistent stcode routines for block '%s'.", blockName);
        return false;
      }
    }
    else
    {
      auto const &stc = mShaderDump->stcode[block.stcodeId];
      auto const &mstc = mainDump.stcode[mainBlock.stcodeId];
      if (stc.size() != mstc.size() || memcmp(stc.data(), mstc.data(), stc.size() * sizeof(stc[0])) != 0)
      {
        logerr("Failed to link additional dump, inconsistent stcode routines for block '%s'.", blockName);
#if DAGOR_DBGLEVEL > 0
        debug("Loaded:");
        ShUtils::shcod_dump(stc, &shBinDump().globVars, &shGlobalData().globVarsState, nullptr, &shBinDump());
        debug("Main:");
        ShUtils::shcod_dump(mstc, &shBinDump().globVars, &shGlobalData().globVarsState, nullptr, &shBinDump());
#endif
        return false;
      }
    }

    blockUidToMainUidMap.emplace(block.uidVal, mainDump.blocks[mainDumpBlockId].uidVal);
    newBlockInverseLinkMap.emplace(mainDump.blocks[mainDumpBlockId].uidVal, block.uidVal);

    ++i;
  }

  // And patch block uids
  for (auto &w : mShaderDump->blkPartSign)
    w = blockUidToMainUidMap[dump_changed ? w : blockInverseLinkMap[w]];
  for (auto &w : mShaderDump->globalSuppBlkSign)
    w = blockUidToMainUidMap[dump_changed ? w : blockInverseLinkMap[w]];

  // Update inverse link maps so that we can relink an already patched bindump on main dump reload
  gvarInverseLinkMap = eastl::move(newGvarInverseLinkMap);
  intervalInverseLinkMap = eastl::move(newIntervalInverseLinkMap);
  blockInverseLinkMap = eastl::move(newBlockInverseLinkMap);

  return true;
}

struct Context
{
  GenerationReferencedData<ShaderBindumpHandle, eastl::unique_ptr<ScriptedShadersBinDumpOwner>> dumps;
};

static Context g_ctx{};

ScriptedShadersBinDump const &get_shaders_dump(ShaderBindumpHandle hnd)
{
  auto const &owner = get_shaders_dump_owner(hnd);
  G_ASSERT_RETURN(owner.getDump(), shBinDump());
  return *owner.getDump();
}

ScriptedShadersBinDumpOwner const &get_shaders_dump_owner(ShaderBindumpHandle hnd)
{
  switch (uint32_t(hnd))
  {
    case uint32_t(MAIN_BINDUMP_HANDLE): return shBinDumpOwner();
    case uint32_t(SEC_EXP_BINDUMP_HANDLE): return shBinDumpExOwner(false);
    default:
    {
      auto const *owner = g_ctx.dumps.cget(hnd);
      G_ASSERT_RETURN(owner, shBinDumpOwner());
      return *owner->get();
    }
  }
}

void iterate_all_shader_dumps(dag::FunctionRef<void(ScriptedShadersBinDumpOwner &)> cb, bool do_secondary_exp)
{
  cb(shBinDumpOwner());
  if (do_secondary_exp)
    cb(shBinDumpExOwner(false));

  iterate_all_additional_shader_dumps(cb);
}

void iterate_all_additional_shader_dumps(dag::FunctionRef<void(ScriptedShadersBinDumpOwner &)> cb)
{
  for (unsigned i = 0, sz = g_ctx.dumps.totalSize(); i < sz; ++i)
  {
    ShaderBindumpHandle hnd = g_ctx.dumps.getRefByIdx(i);
    if (hnd == INVALID_BINDUMP_HANDLE)
      continue;
    cb(**g_ctx.dumps.get(hnd));
  }
}

#define GET_DUMP(hnd_, retonfail_)                                                                  \
  if (hnd == INVALID_BINDUMP_HANDLE)                                                                \
    return retonfail_;                                                                              \
  auto *dump_ = g_ctx.dumps.get(hnd_);                                                              \
  G_ASSERTF_RETURN(dump_ && *dump_, retonfail_, "Invalid shaderbindump handle %x", uint32_t(hnd_)); \
  auto &owner = **dump_;                                                                            \
  auto *dump = owner.getDump();

struct LoadedBindumpData
{
  dag::ConstSpan<uint8_t> dumpData{};
  ShadersBinDumpAssetData asset{};
  Tab<uint8_t> loadedData{};
  bool isMapped = false;

  LoadedBindumpData() = default;
  LoadedBindumpData(LoadedBindumpData const &) = delete;
  LoadedBindumpData &operator=(LoadedBindumpData const &) = delete;
  LoadedBindumpData(LoadedBindumpData &&) = delete;
  LoadedBindumpData &operator=(LoadedBindumpData &&) = delete;

  ~LoadedBindumpData()
  {
    if (isMapped && !dumpData.empty())
      df_unmap(dumpData.data(), dumpData.size());
  }
};

static bool load_bindump_data_from_file(LoadedBindumpData &ld, char const *src_filename,
  d3d::shadermodel::Version shader_model_version)
{
  if (!load_shaders_bindump_asset(ld.asset, src_filename, shader_model_version))
  {
    debug("[SH] Additional shader dump file '%s' not found", ld.asset.fullName.str());
    return {};
  }

  debug("[SH] Loading additional shaders from '%s'...", ld.asset.fullName.str());

  if (ld.asset.memCrd)
  {
    ld.dumpData = {ld.asset.memCrd->data(), ld.asset.memCrd->size()};
    ld.isMapped = false;
  }
  else if (ld.asset.mmapedLoad)
  {
    G_ASSERT(ld.asset.fileCrd->tell() == 0);
    int flen = -1;
    auto const *fdata = (uint8_t const *)df_mmap(ld.asset.fileCrd->fileHandle, &flen);
    if (!fdata || flen != ld.asset.dumpFileSz)
    {
      df_unmap(fdata, flen);
      logerr("[SH] Additional shader dump file '%s' could not be mapped", ld.asset.fullName.str());
      return false;
    }
    ld.dumpData = {fdata, ld.asset.dumpFileSz};
    ld.isMapped = true;
  }
  else
  {
    ld.loadedData.resize(ld.asset.dumpFileSz);
    if (ld.asset.fileCrd->tryRead(ld.loadedData.data(), ld.asset.dumpFileSz) != ld.asset.dumpFileSz)
    {
      logerr("[SH] Additional shader dump file '%s' could not be read in", ld.asset.fullName.str());
      return false;
    }
    ld.dumpData = ld.loadedData;
    ld.isMapped = false;
  }

  return true;
}

static ShaderBindumpHandle load_additional_shaders_bindump(dag::ConstSpan<uint8_t> dump_data,
  ScriptedShadersGlobalData const &global_link_data, char const *dump_name)
{
  if (!global_link_data.initialized())
  {
    logerr("[SH] Trying to load additional shader dump '%s' before the main one.", dump_name);
    return INVALID_BINDUMP_HANDLE;
  }

  ShaderBindumpHandle hnd = g_ctx.dumps.emplaceOne(eastl::make_unique<ScriptedShadersBinDumpOwner>());
  G_ASSERT(hnd);

  Finally onFail{[&] { g_ctx.dumps.destroyReference(hnd); }};

  ScriptedShadersBinDumpOwner &dest = **g_ctx.dumps.get(hnd);
  dest.selfHandle = hnd;

  stcode::disable(dest.stcodeCtx);

  if (!dest.loadFromData(dump_data.data(), dump_data.size(), dump_name))
  {
    logerr("[SH] Unable to load additional shader dump '%s'", dump_name);
    return INVALID_BINDUMP_HANDLE;
  }

  if (!dest.linkAgaistGlobalData(global_link_data, true))
  {
    logerr("[SH] Unable to link additional shader dump '%s'", dump_name);
    return INVALID_BINDUMP_HANDLE;
  }

  dest.initAfterLoad(false);
  dest.assertionCtx.init(dest);
  debug("[SH] Additional shaders from '%s' loaded OK regs = %d", dump_name, dest->maxRegSize);
  if (dest->maxRegSize > MAX_TEMP_REGS)
  {
    logerr("Too many shader temp registers %d, current limit is %d", dest->maxRegSize, MAX_TEMP_REGS);
    return INVALID_BINDUMP_HANDLE;
  }

  onFail.release();
  return hnd;
}

ShaderBindumpHandle load_additional_shaders_bindump(dag::ConstSpan<uint8_t> dump_data, char const *dump_name)
{
  return load_additional_shaders_bindump(dump_data, shGlobalData(), dump_name);
}

ShaderBindumpHandle load_additional_shaders_bindump(char const *src_filename, d3d::shadermodel::Version shader_model_version)
{
  LoadedBindumpData ld{};
  if (!load_bindump_data_from_file(ld, src_filename, shader_model_version))
    return INVALID_BINDUMP_HANDLE;

  return load_additional_shaders_bindump(ld.dumpData, ld.asset.fullName.str());
}

ShaderBindumpHandle load_additional_shaders_bindump(char const *src_filename, char const *name_override,
  d3d::shadermodel::Version shader_model_version)
{
  LoadedBindumpData ld{};
  if (!load_bindump_data_from_file(ld, src_filename, shader_model_version))
    return INVALID_BINDUMP_HANDLE;

  return load_additional_shaders_bindump(ld.dumpData, name_override);
}

bool reinit_shaders_bindump(ScriptedShadersBinDumpOwner &dest, ScriptedShadersGlobalData const &global_link_data)
{
  G_ASSERTF(dest.selfHandle != MAIN_BINDUMP_HANDLE && dest.selfHandle != SEC_EXP_BINDUMP_HANDLE, "Can not reinit %s dump inplace",
    dest.selfHandle == MAIN_BINDUMP_HANDLE ? "main" : "exp");
  G_ASSERT(global_link_data.initialized());

  dest.assertionCtx.close();

  FINALLY([&] {
    dest.assertionCtx.init(dest);
    ++dest.generation;
  });

  if (!dest.linkAgaistGlobalData(global_link_data, false))
  {
    logerr("[SH] Unable to link additional shader dump '%s'", dest.name.c_str());
    return false;
  }

  debug("[SH] Additional shaders from '%s' reinited OK regs = %d", dest.name.c_str(), dest->maxRegSize);

  return true;
}

static bool reload_shaders_bindump(ShaderBindumpHandle hnd, ScriptedShadersBinDumpOwner &dest,
  ScriptedShadersGlobalData const &global_link_data, dag::ConstSpan<uint8_t> dump_data, char const *dump_name)
{
  G_ASSERTF(hnd != MAIN_BINDUMP_HANDLE && hnd != SEC_EXP_BINDUMP_HANDLE, "Can not reload %s dump from data",
    hnd == MAIN_BINDUMP_HANDLE ? "main" : "exp");
  G_ASSERT(global_link_data.initialized()); // Supposedly unreachable due to logerr in load_additional_shaders_bindump

  d3d::GpuAutoLock gpuLock;

  shaders_internal::cleanup_shaders_on_reload(dest);

  d3d::driver_command(Drv3dCommand::UNLOAD_SHADER_MEMORY);
  d3d::driver_command(Drv3dCommand::D3D_FLUSH);

  dest.assertionCtx.close();

  ScriptedShadersBinDumpOwner prevOwner = eastl::move(dest);
  dest.clear();

  Finally onFail{[&] {
    dest.clear();
    dest = eastl::move(prevOwner);
    dest.initAfterLoad(false);
    dest.assertionCtx.init(dest);
  }};

  if (!dest.loadFromData(dump_data.data(), dump_data.size(), dump_name))
  {
    logerr("[SH] Unable to load additional shader dump '%s'", dump_name);
    return false;
  }

  if (!dest.linkAgaistGlobalData(global_link_data, true))
  {
    logerr("[SH] Unable to link additional shader dump '%s'", dump_name);
    return false;
  }

  dest.initAfterLoad(false);
  dest.assertionCtx.init(dest);
  debug("[SH] Additional shaders from '%s' loaded OK regs = %d", dump_name, dest->maxRegSize);
  if (dest->maxRegSize > MAX_TEMP_REGS)
  {
    logerr("Too many shader temp registers %d, current limit is %d", dest->maxRegSize, MAX_TEMP_REGS);
    return false;
  }

  if (!shaders_internal::reload_shaders_materials(dest, nullptr, prevOwner, nullptr))
    return false;

  prevOwner.clear();
  onFail.release();
  return true;
}

bool reload_shaders_bindump(ShaderBindumpHandle hnd, dag::ConstSpan<uint8_t> dump_data, char const *dump_name)
{
  GET_DUMP(hnd, false);
  return reload_shaders_bindump(hnd, owner, shGlobalData(), dump_data, dump_name);
}

bool reload_shaders_bindump(ShaderBindumpHandle hnd, char const *src_filename, d3d::shadermodel::Version shader_model_version)
{
  if (hnd == MAIN_BINDUMP_HANDLE)
    return load_shaders_bindump_with_fence(src_filename, shader_model_version);
  else if (hnd == SEC_EXP_BINDUMP_HANDLE)
    return load_shaders_bindump(src_filename, shader_model_version, true);

  GET_DUMP(hnd, false);

  LoadedBindumpData ld{};
  if (!load_bindump_data_from_file(ld, src_filename, shader_model_version))
    return false;

  return reload_shaders_bindump(hnd, owner, shGlobalData(), ld.dumpData, ld.asset.fullName.str());
}

bool reload_shaders_bindump(ShaderBindumpHandle hnd, char const *src_filename, char const *name_override,
  d3d::shadermodel::Version shader_model_version)
{
  G_ASSERT_RETURN(hnd != MAIN_BINDUMP_HANDLE && hnd != SEC_EXP_BINDUMP_HANDLE, false);
  GET_DUMP(hnd, false);

  LoadedBindumpData ld{};
  if (!load_bindump_data_from_file(ld, src_filename, shader_model_version))
    return false;

  return reload_shaders_bindump(hnd, owner, shGlobalData(), ld.dumpData, name_override);
}

void unload_shaders_bindump(ShaderBindumpHandle hnd)
{
  if (hnd == MAIN_BINDUMP_HANDLE || hnd == SEC_EXP_BINDUMP_HANDLE)
  {
    unload_shaders_bindump(hnd == SEC_EXP_BINDUMP_HANDLE);
    return;
  }

  GET_DUMP(hnd, );

  {
    d3d::GpuAutoLock gpuLock;

    // have to delete shaders before ruining memory with move because if loading will fail
    // we do dest.initAfterLoad which clears shader arrays without destroying them
    shaders_internal::cleanup_shaders_on_reload(owner);

    // signal backend that the memory backing shader bytecode is about to be invalidated
    d3d::driver_command(Drv3dCommand::UNLOAD_SHADER_MEMORY);

    d3d::driver_command(Drv3dCommand::D3D_FLUSH);
  }

  owner.clear();
  g_ctx.dumps.destroyReference(hnd);
}

uint32_t get_shaders_bindump_generation(ShaderBindumpHandle hnd) { return get_shaders_dump_owner(hnd).generation; }

#undef GET_DUMP

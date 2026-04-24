// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include "platformUtils.h"
#include <EASTL/vector_set.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <memory/dag_framemem.h>
#include <generic/dag_enumerate.h>
#include <util/dag_finally.h>
#include <util/dag_hash.h>
#include <render/blkToConstBuffer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>

static ShaderVariableInfo global_time_phaseVarId("global_time_phase", true);

static ShaderVariableInfo fog_shader_typeVarId("fog_shader_type", true);
static ShaderVariableInfo envi_cover_shader_typeVarId("envi_cover_shader_type", true);
static ShaderVariableInfo nbs_qualityVarId("nbs_quality", true);
static eastl::array<ShaderVariableInfo, 2> nbs_perm_id_gVarIds = {
  ShaderVariableInfo("nbsPermIdG0", true), ShaderVariableInfo("nbsPermIdG1", true)};
static eastl::array<ShaderGlobal::Interval, 2> nbs_perm_id_gIntervalHashes = {
  ShaderGlobal::interval<"nbsPermIdG0"_h>, ShaderGlobal::interval<"nbsPermIdG1"_h>};

struct ShaderBindumpHandleWithRc : ShaderBindumpHandle
{
  int rc = 0;

  ShaderBindumpHandleWithRc(ShaderBindumpHandle hnd) : ShaderBindumpHandle{hnd}, rc{1} {}
  ShaderBindumpHandleWithRc(const ShaderBindumpHandleWithRc &) = default;
  ShaderBindumpHandleWithRc &operator=(const ShaderBindumpHandleWithRc &) = default;
};

static eastl::vector_set<NodeBasedShaderManager *> node_based_shaders_with_cached_resources;
static ska::flat_hash_map<eastl::string, ShaderBindumpHandleWithRc> node_based_shader_bindumps;

NodeBasedShaderManager::~NodeBasedShaderManager()
{
  shutdown();
  node_based_shaders_with_cached_resources.erase(this);
}

void NodeBasedShaderManager::shutdown()
{
  if (scriptedShadersDumpHandle != INVALID_BINDUMP_HANDLE)
  {
    auto it = node_based_shader_bindumps.find(scriptedShadersDumpName);
    G_ASSERT_RETURN(it != node_based_shader_bindumps.end(), );
    G_ASSERT_RETURN(ShaderBindumpHandle(it->second) == scriptedShadersDumpHandle, );
    --it->second.rc;
    if (it->second.rc == 0)
    {
      unload_shaders_bindump(scriptedShadersDumpHandle);
      node_based_shader_bindumps.erase(it);
    }
    scriptedShadersDumpHandle = INVALID_BINDUMP_HANDLE;
    scriptedShadersDumpName.clear();
    cachedDumpGeneration = 0;
  }
}

String NodeBasedShaderManager::buildScriptedShaderName(char const *asset)
{
  char const *lastSep = strrchr(asset, '/');
  asset = lastSep ? lastSep + 1 : asset;
  char const *first = asset, *last = asset;
  while (*last && *last != '.')
    ++last;
  String shader{first, int(last - first)};
  shader += "_";
  shader += PLATFORM_LABELS[get_nbsm_platform()];
  return shader;
}

char const *NodeBasedShaderManager::getShaderBlockName() const { return get_shader_type_block_name(shader); }

void NodeBasedShaderManager::setShadervars(int variant_id)
{
  ShaderGlobal::set_float(global_time_phaseVarId, get_shader_global_time_phase(0, 0));

  switch (shader)
  {
    case NodeBasedShaderType::Fog: fog_shader_typeVarId.set_int(variant_id); break;
    case NodeBasedShaderType::EnviCover: envi_cover_shader_typeVarId.set_int(variant_id); break;
  }

  if (permGroupActiveIdx.empty())
  {
    for (auto &varid : nbs_perm_id_gVarIds)
      varid.set_int(0);
  }
  else
  {
    for (auto [i, p] : enumerate(permGroupActiveIdx))
    {
      nbs_perm_id_gVarIds[i].set_int(p);
    }
  }

  nbs_qualityVarId.set_int(int(nbsQuality));
}

bool NodeBasedShaderManager::update(const String &shader_name, const DataBlock &shader_blk, String &out_errors)
{
  lastCompileIsSuccess = compileScriptedShaders(shader_name, shader_blk, out_errors);
  return lastCompileIsSuccess;
}

static eastl::string normalize_shader_res_name(const char *shader_name)
{
  const char *lastSlashPtr = strrchr(shader_name, '/');
  const char *nameBegin = lastSlashPtr ? lastSlashPtr + 1 : shader_name;
  const char *jsonPos = strstr(nameBegin, ".json");
  return eastl::string(nameBegin, jsonPos ? jsonPos : (nameBegin + strlen(nameBegin)));
}

bool NodeBasedShaderManager::loadOptionalGraphsInfoFromStream(InPlaceMemLoadCB &load_stream)
{
  DataBlock optionalGraphsBlock;
  optionalGraphsBlock.loadFromStream(load_stream);
  optionalGraphMap.clear();
  permGroupActiveIdx.clear();
  permGroupSizes.clear();

  permCount = optionalGraphsBlock.getInt("permutationCnt", 1);
  uint32_t permGroupCount = optionalGraphsBlock.getInt("permutationGroupCnt", 0);
  permGroupActiveIdx.resize(permGroupCount);
  permGroupSizes.resize(permGroupCount);

  const uint32_t permutationNameId = optionalGraphsBlock.getNameId("permutation");
  const uint32_t permGroupCntNameId = optionalGraphsBlock.getNameId("permutationGroupSizes");

  for (uint32_t i = 0; i < optionalGraphsBlock.blockCount(); ++i)
  {
    if (const DataBlock *currentBlock = optionalGraphsBlock.getBlock(i); currentBlock->getBlockNameId() == permutationNameId)
    {
      const uint32_t groupIdx = currentBlock->getInt("groupIdx");
      const uint32_t elemIdx = currentBlock->getInt("elemIdx");
      optionalGraphMap[currentBlock->getStr("graph")] = {groupIdx, elemIdx};
    }
    else if (currentBlock->getBlockNameId() == permGroupCntNameId)
    {
      for (uint32_t groupIdx = 0; groupIdx < currentBlock->paramCount(); groupIdx++)
      {
        permGroupActiveIdx[groupIdx] = 0;
        permGroupSizes[groupIdx] = currentBlock->getInt(groupIdx);
      }
    }
    else
    {
      logerr("Encountered a group called: %s in NBS permutation discription, this indicates a mismatch of loc_shaders and game "
             "version, please do a CVS pull! Disabling permutation graphs...",
        currentBlock->getBlockName());
      return false;
    }
  }

  return true;
}

/*
Binary format for scripted loc shader:
Header datablock
{
  N String parameters for optional graph names.
}
bindump data
*/

bool NodeBasedShaderManager::loadScriptedShadersFromResources(dag::Span<uint8_t> data, const String &shader_name,
  bool keep_permutation)
{
  G_ASSERT(scriptedShadersDumpHandle == INVALID_BINDUMP_HANDLE);

  InPlaceMemLoadCB loadStream(data.data(), data_size(data));

  auto disablePermutations = [&, this] {
    permCount = 1;
    permGroupActiveIdx.clear();
    permGroupSizes.clear();
  };

  if (!loadOptionalGraphsInfoFromStream(loadStream))
    disablePermutations();

  if (permGroupActiveIdx.size() > c_countof(nbs_perm_id_gVarIds))
  {
    logerr("Too many permutation groups (%d > %d), this indicates a mismatch of loc_shaders and game "
           "version, please do a CVS pull! Disabling permutation graphs...",
      permGroupActiveIdx.size(), c_countof(nbs_perm_id_gVarIds));
    disablePermutations();
  }

  for (auto [i, size] : enumerate(permGroupSizes))
  {
    size_t valueCount = ShaderGlobal::get_subinterval_count(nbs_perm_id_gIntervalHashes[i]);
    if (size > valueCount)
    {
      logerr("Too many permutations (%d > %d) in group %d, this indicates a mismatch of loc_shaders and game "
             "version, please do a CVS pull! Disabling permutation graphs...",
        size, valueCount, i);
      disablePermutations();
      break;
    }
  }

  if (!keep_permutation)
  {
    for (unsigned &idx : permGroupActiveIdx)
      idx = 0;
  }

  scriptedShadersDumpName = shader_name.c_str();
  if (auto it = node_based_shader_bindumps.find(scriptedShadersDumpName); it != node_based_shader_bindumps.end())
  {
    ++it->second.rc;
    scriptedShadersDumpHandle = it->second; // spliced
  }
  else
  {
    scriptedShadersDumpHandle = load_additional_shaders_bindump(
      dag::ConstSpan<uint8_t>{loadStream.data(), loadStream.size()}.subspan(loadStream.tell()), shader_name.str());
    if (scriptedShadersDumpHandle == INVALID_BINDUMP_HANDLE)
    {
      logerr("Invalid loc shaders bindump in resource '%s', please run dabuild!", shader_name.str());
      return false;
    }
    node_based_shader_bindumps.emplace(scriptedShadersDumpName, ShaderBindumpHandleWithRc{scriptedShadersDumpHandle});
  }
  cachedDumpGeneration = get_shaders_bindump_generation(scriptedShadersDumpHandle);

  lastCompileIsSuccess = true;
  return true;
}

bool NodeBasedShaderManager::loadFromResources(const String &shader_path, bool keep_permutation)
{
  eastl::string resName = node_based_shader_get_resource_name(shader_path);
  auto datap = (dag::Span<uint8_t> *)get_one_game_resource_ex(resName.c_str(), LShaderGameResClassId);
  if (!datap)
  {
    logerr("shader does not exist '%s'", resName.c_str());
    return false;
  }

  FINALLY([&] { release_game_resource_ex(datap, LShaderGameResClassId); });

  auto data = *datap;
  G_ASSERTF_RETURN(*(int32_t *)data.data() == _MAKE4C('DShl'), false, "Outdated NBS asset type, please run dabuild!");
  data = data.subspan(sizeof(int32_t));
  return loadScriptedShadersFromResources(data, shader_path, keep_permutation);
}

void NodeBasedShaderManager::enableOptionalGraph(const String &graph_name, bool enable)
{
  if (graph_name.empty())
    return;

  if (auto it = optionalGraphMap.find(graph_name.c_str()); it != optionalGraphMap.end())
  {
    const PermutationDesc &permDesc = it->second;
    if (enable)
    {
      if (permGroupActiveIdx[permDesc.groupIdx] != 0)
        logerr("Activated permutation %s from subgroup %d, while it was already active with permutation %d, the previous entity is "
               "now ignored!",
          graph_name, permDesc.groupIdx, permGroupActiveIdx[permDesc.groupIdx]);
      permGroupActiveIdx[permDesc.groupIdx] = permDesc.elemIdx;
    }
    else if (permGroupActiveIdx[permDesc.groupIdx] == permDesc.elemIdx)
      permGroupActiveIdx[permDesc.groupIdx] = 0;
    else
    {
      logerr("Tried to deactivate permutation %s, but another permutation with group:%d id:%d has overwritten the group!", graph_name,
        permDesc.elemIdx, permGroupActiveIdx[permDesc.groupIdx]);
    }
  }
  else
  {
    logerr("Tried to activate permutation graph %s, but it does not exist", graph_name.c_str());
  }
}

const char *node_based_shader_current_platform_suffix() { return PLATFORM_LABELS[get_nbsm_platform()]; }

eastl::string node_based_shader_get_resource_name(const char *shader_path)
{
  return eastl::string(eastl::string::CtorSprintf{}, "%s_%s", normalize_shader_res_name(shader_path).c_str(),
    node_based_shader_current_platform_suffix());
}

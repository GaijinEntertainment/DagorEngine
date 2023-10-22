#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <3d/dag_drv3d.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include "platformLabels.h"
#include <EASTL/vector_set.h>
#include <memory/dag_framemem.h>
#if !NBSM_COMPILE_ONLY
#include <render/blkToConstBuffer.h>
#include <shaders/dag_shaders.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#endif

static eastl::vector_set<NodeBasedShaderManager *> node_based_shaders_with_cached_resources;

NodeBasedShaderManager::~NodeBasedShaderManager()
{
  resetCachedResources();
  node_based_shaders_with_cached_resources.erase(this);
}

void NodeBasedShaderManager::updateBlkDataConstBuffer(const DataBlock &shader_blk)
{
  currentIntParametersBlk.reset(new DataBlock(*shader_blk.getBlockByNameEx("inputs_int")));
  currentFloatParametersBlk.reset(new DataBlock(*shader_blk.getBlockByNameEx("inputs_float")));
  currentFloat4ParametersBlk.reset(new DataBlock(*shader_blk.getBlockByNameEx("inputs_float4")));
}

void NodeBasedShaderManager::updateBlkDataTextures(const DataBlock &shader_blk)
{
  currentTextures2dBlk.reset(new DataBlock(*shader_blk.getBlockByNameEx("inputs_texture2D")));
  currentTextures3dBlk.reset(new DataBlock(*shader_blk.getBlockByNameEx("inputs_texture3D")));
  currentTextures2dNoSamplerBlk.reset(new DataBlock(*shader_blk.getBlockByNameEx("inputs_texture2D_nosampler")));
  currentTextures3dNoSamplerBlk.reset(new DataBlock(*shader_blk.getBlockByNameEx("inputs_texture3D_nosampler")));
}

void NodeBasedShaderManager::updateBlkDataBuffers(const DataBlock &shader_blk)
{
  currentBuffersBlk.reset(new DataBlock(*shader_blk.getBlockByNameEx("inputs_Buffer")));
}

void NodeBasedShaderManager::updateBlkDataMetadata(const DataBlock &shader_blk)
{
  metadataBlk = eastl::make_unique<DataBlock>(*shader_blk.getBlockByNameEx("metadata"));
}

bool NodeBasedShaderManager::update(const DataBlock &shader_blk, String &out_errors, PLATFORM platform_id)
{
  if (compileShaderProgram(shader_blk, out_errors, platform_id))
  {
    updateBlkDataMetadata(shader_blk);
    updateBlkDataResources(shader_blk);
  }
  return lastCompileIsSuccess;
}

void NodeBasedShaderManager::getPlatformList(Tab<String> &out_platforms, PLATFORM platform_id)
{
  for (int i = 0; i < PLATFORM::LOAD_COUNT; i++)
  {
    if (platform_id >= 0 && platform_id != i)
      continue;

    out_platforms.push_back(String(PLATFORM_LABELS[i]));
  }
}

void NodeBasedShaderManager::updateBlkDataResources(const DataBlock &shader_blk)
{
  updateBlkDataConstBuffer(shader_blk);
  updateBlkDataTextures(shader_blk);
  updateBlkDataBuffers(shader_blk);
  cachedVariableGeneration = ~0u;
  isDirty = true;
}

#if !NBSM_COMPILE_ONLY
static NodeBasedShaderManager::PLATFORM get_nbsm_platform()
{
  return d3d::get_driver_code()
    .map(d3d::xboxOne, NodeBasedShaderManager::PLATFORM::DX12_XBOX_ONE)
    .map(d3d::scarlett, NodeBasedShaderManager::PLATFORM::DX12_XBOX_SCARLETT)
    .map(d3d::ps4, NodeBasedShaderManager::PLATFORM::PS4)
    .map(d3d::ps5, NodeBasedShaderManager::PLATFORM::PS5)
    .map(d3d::dx11, NodeBasedShaderManager::PLATFORM::DX11)
    .map(d3d::dx12, NodeBasedShaderManager::PLATFORM::DX12)
    .map(d3d::vulkan, d3d::get_driver_desc().caps.hasBindless ? NodeBasedShaderManager::PLATFORM::VULKAN_BINDLESS
                                                              : NodeBasedShaderManager::PLATFORM::VULKAN)
    .map(d3d::metal, NodeBasedShaderManager::PLATFORM::MTL)
    .map(d3d::stub, NodeBasedShaderManager::PLATFORM::VULKAN)
    .map(d3d::any, NodeBasedShaderManager::PLATFORM::LOAD_COUNT);
}

static eastl::string normalize_shader_res_name(const char *shader_name)
{
  const char *lastSlashPtr = strrchr(shader_name, '/');
  const char *nameBegin = lastSlashPtr ? lastSlashPtr + 1 : shader_name;
  const char *jsonPos = strstr(nameBegin, ".json");
  return eastl::string(nameBegin, jsonPos ? jsonPos : (nameBegin + strlen(nameBegin)));
}

/*
Binary format for loc shader:
Header datablock
{
  N String parameters for optional graph names.
}
2^N times:
  Pemutation parameters datablock
  binary data for permutation
*/

void NodeBasedShaderManager::loadShaderFromStream(uint32_t idx, uint32_t platform_id, InPlaceMemLoadCB &load_stream,
  uint32_t variant_id)
{
  G_ASSERTF(platform_id < PLATFORM::LOAD_COUNT && int(platform_id) >= 0, "Unknown platform id: %d", platform_id);
  G_ASSERTF(variant_id < get_shader_variant_count(shader) && int(variant_id) >= 0, "Unknown variant id for shader #%d: %d",
    (int)shader, variant_id);
  resetCachedResources();
  ShaderBin &permutation = shaderBin[platform_id][variant_id][idx];
  permParameters[idx].loadFromStream(load_stream);
  uint32_t shaderBinSize;
  load_stream.read((&shaderBinSize), sizeof(shaderBinSize));
  permutation.resize((shaderBinSize + sizeof(uint32_t) - 1) / sizeof(uint32_t));
  load_stream.read(permutation.data(), shaderBinSize);
}

bool NodeBasedShaderManager::loadFromResources(const String &shader_path, bool keep_permutation)
{
  PLATFORM platform_id = get_nbsm_platform();
  G_ASSERTF(platform_id < PLATFORM::LOAD_COUNT && int(platform_id) >= 0, "Unknown platform id: %d", platform_id);
  eastl::string resName = node_based_shader_get_resource_name(shader_path);
  auto data = (dag::Span<uint8_t> *)get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(resName.c_str()), LShaderGameResClassId);
  if (!data)
  {
    logerr("shader does not exist '%s'", resName.c_str());
    return false;
  }
  InPlaceMemLoadCB loadStream(data->data(), data_size(*data));
  DataBlock optionalGraphNamesBlock;
  optionalGraphNamesBlock.loadFromStream(loadStream);
  optionalGraphNames.clear();
  for (uint32_t i = 0; i < optionalGraphNamesBlock.paramCount(); ++i)
    optionalGraphNames.emplace_back(optionalGraphNamesBlock.getStr(i));

  G_ASSERTF(optionalGraphNames.size() < 16, "Optional graphs count should be less than 16");
  optionalGraphNames.resize(min<unsigned>(optionalGraphNames.size(), 15u));

  // Workaround for old for format
  if (strcmp(optionalGraphNamesBlock.getStr("subgraph", ""), "") == 0)
  {
    loadStream.close();
    loadStream = InPlaceMemLoadCB(data->data(), data_size(*data));
  }

  const uint32_t permCount = (1 << optionalGraphNames.size());
  G_ASSERTF(!keep_permutation || permParameters.size() == permCount, "We can't keep permutation if permutations count changed.");
  permParameters.resize(permCount);
  if (!keep_permutation)
    permutationId = 0; // graph names have changed, go back to default permutation

  int variantsCnt = get_shader_variant_count(shader);
  for (ShaderBinVariants &shVariant : shaderBin)
  {
    shVariant.resize(variantsCnt);
    for (int variantId = 0; variantId < variantsCnt; ++variantId)
      shVariant[variantId].resize(permCount);
  }
  for (uint32_t i = 0; i < permCount; ++i)
    for (uint32_t variantId = 0; variantId < variantsCnt; ++variantId)
      loadShaderFromStream(i, platform_id, loadStream, variantId);

  updateBlkDataResources(permParameters[permutationId]);

  lastCompileIsSuccess = true;
  release_game_resource((GameResource *)data);
  return true;
}

const NodeBasedShaderManager::ShaderBinPermutations &NodeBasedShaderManager::getPermutationShadersBin(uint32_t variant_id) const
{
  G_ASSERTF(variant_id < get_shader_variant_count(shader) && int(variant_id) >= 0, "Unknown variant id for shader #%d: %d",
    (int)shader, variant_id);
  PLATFORM platform_id = get_nbsm_platform();
  G_ASSERTF(platform_id < PLATFORM::LOAD_COUNT && int(platform_id) >= 0, "Unknown platform id: %d", platform_id);
  return shaderBin[platform_id][variant_id];
}

const NodeBasedShaderManager::ArrayValue *NodeBasedShaderManager::getArrayValue(const char *name, size_t name_length) const
{
  for (const ArrayValue &arrayValue : arrayValues)
    if (strncmp(arrayValue.first.str(), name, name_length) == 0)
      return &arrayValue;

  return nullptr;
}

void NodeBasedShaderManager::setArrayValue(const char *name, const Tab<Point4> &values)
{
  for (ArrayValue &arrayValue : arrayValues)
    if (strcmp(arrayValue.first.str(), name) == 0)
    {
      arrayValue.second = &values;
      return;
    }

  arrayValues.emplace_back(String(name), &values);
}

const DataBlock &NodeBasedShaderManager::getMetadata() const
{
  if (metadataBlk)
    return *metadataBlk;
  else
    return emptyBlk;
}

void NodeBasedShaderManager::setConstantBuffer()
{
  ShaderGlobal::set_real(::get_shader_variable_id("global_time_phase", true), get_shader_global_time_phase(0, 0));

  dag::Vector<float, framemem_allocator> parametersBuffer;
  const uint32_t REGISTERS_ALIGNMENT = 4;
  parametersBuffer.reserve(
    (currentIntVariables.size() + currentFloatVariables.size() + REGISTERS_ALIGNMENT - 1) / REGISTERS_ALIGNMENT * REGISTERS_ALIGNMENT +
    currentFloat4Variables.size() * REGISTERS_ALIGNMENT);
  for (auto &shaderVar : currentIntVariables)
  {
    const int varType = shaderVar.varType;
    const int varId = shaderVar.varId;
    float valueToAdd;
    if (varType == SHVT_INT) // or may be should be error?
      valueToAdd = bitwise_cast<float, int>(ShaderGlobal::get_int_fast(varId));
    else
      valueToAdd = bitwise_cast<float, int>(shaderVar.val);
    parametersBuffer.push_back(valueToAdd);
  }

  for (auto &shaderVar : currentFloatVariables)
  {
    const int varType = shaderVar.varType;
    const int varId = shaderVar.varId;
    float valueToAdd;
    if (varType == SHVT_REAL)
      valueToAdd = ShaderGlobal::get_real_fast(varId);
    else if (varType == SHVT_INT) // or may be should be error?
      valueToAdd = ShaderGlobal::get_int_fast(varId);
    else
      valueToAdd = shaderVar.val;
    parametersBuffer.push_back(valueToAdd);
  }
  parametersBuffer.resize((parametersBuffer.size() + REGISTERS_ALIGNMENT - 1) / REGISTERS_ALIGNMENT * REGISTERS_ALIGNMENT, 0.f);

  for (auto &shaderVar : currentFloat4Variables)
  {
    Point4 valueToAdd;
    if (shaderVar.varType == SHVT_COLOR4)
    {
      Color4 paramValue = ShaderGlobal::get_color4_fast(shaderVar.varId);
      valueToAdd = Point4(reinterpret_cast<const real *>(&paramValue), Point4::CTOR_FROM_PTR);
    }
    else
      valueToAdd = shaderVar.val;
    for (uint32_t i = 0; i < 4; ++i)
      parametersBuffer.push_back(valueToAdd[i]);
  }
  G_ASSERT(parametersBuffer.size() % 4 == 0);

  if (parametersBuffer.size()) // call d3d::release_cb0_data(STAGE_CS) after draw
    d3d::set_cb0_data(STAGE_CS, reinterpret_cast<float *>(parametersBuffer.data()), parametersBuffer.size() / REGISTERS_ALIGNMENT);
}

void NodeBasedShaderManager::setTextures(dag::Vector<NodeBasedTexture> &node_based_textures, int &offset, bool has_sampler) const
{
  for (int i = 0, ie = node_based_textures.size(); i < ie; ++i, ++offset)
  {
    const NodeBasedTexture &nbTex = node_based_textures[i];
    if (nbTex.dynamicTextureVarId >= 0)
    {
      TEXTUREID id = ShaderGlobal::get_tex_fast(nbTex.dynamicTextureVarId);
      G_FAST_ASSERT(get_managed_texture_refcount(id) > 0);
      mark_managed_tex_lfu(id);
      d3d::set_tex(STAGE_CS, offset, D3dResManagerData::getBaseTex(id), has_sampler);
    }
    else
    {
      d3d::set_tex(STAGE_CS, offset, nbTex.usedTextureResId, has_sampler);
    }
  }
}

void NodeBasedShaderManager::setBuffers(int &offset) const
{
  for (int i = 0, ie = nodeBasedBuffers.size(); i < ie; ++i, ++offset)
  {
    const NodeBasedBuffer &nbBuf = nodeBasedBuffers[i];
    if (nbBuf.dynamicBufferVarId >= 0)
    {
      D3DRESID id = ShaderGlobal::get_buf_fast(nbBuf.dynamicBufferVarId);
      Sbuffer *buffer = (Sbuffer *)acquire_managed_res(id);
      d3d::set_buffer(STAGE_CS, offset, buffer);
      G_FAST_ASSERT(get_managed_texture_refcount(id) > 1);
      release_managed_res(id);
    }
    else
    {
      d3d::set_buffer(STAGE_CS, offset, nbBuf.usedBufferResId);
    }
  }
}

void NodeBasedShaderManager::setConstants()
{
  if (isDirty)
  {
    isDirty = !invalidateCachedResources();
  }
  if (cachedVariableGeneration != VariableMap::generation())
    precacheVariables();
  setConstantBuffer();
  int offset = 0;
  setTextures(nodeBasedTextures, offset, true);
  setTextures(nodeBasedTexturesNoSampler, offset, false);
  setBuffers(offset);
}

void NodeBasedShaderManager::enableOptionalGraph(const String &graph_name, bool enable)
{
  for (uint32_t i = 0; i < optionalGraphNames.size(); ++i)
  {
    if (optionalGraphNames[i] == graph_name)
    {
      if (enable)
        permutationId |= 1 << i;
      else
        permutationId &= ~(1 << i);
    }
  }
  updateBlkDataResources(permParameters[permutationId]);
}

void NodeBasedShaderManager::precacheVariables()
{
  currentFloatVariables.clear();
  for (int i = 0; i < currentFloatParametersBlk->paramCount(); ++i)
  {
    const char *paramName = currentFloatParametersBlk->getStr(i);
    const int shaderVarId = ::get_shader_variable_id(paramName, true);
    const int varType = ShaderGlobal::get_var_type(shaderVarId);
    if (varType == SHVT_REAL || varType == SHVT_INT)
      currentFloatVariables.emplace_back(CachedFloat{paramName, varType, shaderVarId});
    else
    {
      currentFloatVariables.emplace_back(CachedFloat{paramName, 0.f});
      logerr("paramName = %s has type %d not int/real", paramName, varType);
    }
  }
  currentIntVariables.clear();
  for (int i = 0; i < currentIntParametersBlk->paramCount(); ++i)
  {
    const char *paramName = currentIntParametersBlk->getStr(i);
    const int shaderVarId = ::get_shader_variable_id(paramName, true);
    const int varType = ShaderGlobal::get_var_type(shaderVarId);
    if (varType == SHVT_INT)
      currentIntVariables.emplace_back(CachedInt{paramName, varType, shaderVarId});
    else
    {
      currentIntVariables.emplace_back(CachedInt{paramName, 0});
      logerr("paramName = %s has type %d not int", paramName, varType);
    }
  }
  currentFloat4Variables.clear();
  for (int i = 0; i < currentFloat4ParametersBlk->paramCount(); ++i)
  {
    const char *paramName = currentFloat4ParametersBlk->getStr(i);
    const char *arrayOpen = strstr(paramName, "[");
    const bool isArray = arrayOpen != nullptr;
    if (isArray)
    {
      int fillIx = 0;
      int arraySize = strtol(arrayOpen + 1, nullptr, 10);
      if (const ArrayValue *arrayValue = getArrayValue(paramName, arrayOpen - paramName))
      {
        G_ASSERT(arrayValue->second->size() <= arraySize);
        for (const Point4 &value : *arrayValue->second)
        {
          currentFloat4Variables.emplace_back(CachedFloat4{paramName, value});
          fillIx++;
        }
      }

      for (; fillIx < arraySize; ++fillIx)
        currentFloat4Variables.emplace_back(CachedFloat4{paramName, Point4(0, 0, 0, 0)});
    }
    else
    {
      const int shaderVarId = ::get_shader_variable_id(paramName, true);
      const int varType = ShaderGlobal::get_var_type(shaderVarId);
      if (varType == SHVT_COLOR4)
        currentFloat4Variables.emplace_back(CachedFloat4{paramName, varType, shaderVarId});
      else
        currentFloat4Variables.emplace_back(CachedFloat4{paramName, Point4(0, 0, 0, 0)});
    }
  }
  cachedVariableGeneration = VariableMap::generation();
}


void NodeBasedShaderManager::fillTextureCache(dag::Vector<NodeBasedTexture> &node_based_textures, const DataBlock &src_block,
  int offset, bool &has_loaded)
{
  for (int i = 0, ei = src_block.paramCount(); i < ei; ++i)
  {
    const char *texName = src_block.getStr(i);
    NodeBasedTexture &nbTex = node_based_textures[i + offset];
    TEXTUREID resId = get_managed_texture_id(texName);
    bool isDynamic = resId == BAD_TEXTUREID;
    nbTex.dynamicTextureVarId = isDynamic ? ::get_shader_variable_id(texName, true) : -1;
    nbTex.usedTextureResId = isDynamic ? nullptr : acquire_managed_tex(resId);
    if (!isDynamic)
    {
      resIdsToRelease.push_back(resId);
      if (!nbTex.usedTextureResId)
        has_loaded = false;
    }
    else if (ShaderGlobal::get_tex_fast(nbTex.dynamicTextureVarId) == BAD_TEXTUREID)
      nbTex.dynamicTextureVarId = -1;
  }
}
void NodeBasedShaderManager::fillBufferCache(const DataBlock &src_block, int offset, bool &has_loaded)
{
  for (int i = 0, ei = src_block.paramCount(); i < ei; ++i)
  {
    const char *bufferName = src_block.getStr(i);
    NodeBasedBuffer &nbBuf = nodeBasedBuffers[i + offset];
    D3DRESID resId = get_managed_res_id(bufferName);
    bool isDynamic = resId == BAD_D3DRESID;
    nbBuf.dynamicBufferVarId = isDynamic ? ::get_shader_variable_id(bufferName, true) : -1;
    nbBuf.usedBufferResId = isDynamic ? nullptr : (Sbuffer *)acquire_managed_res(resId);
    if (!isDynamic)
    {
      resIdsToRelease.push_back(resId);
      if (!nbBuf.usedBufferResId)
        has_loaded = false;
    }
    else if (ShaderGlobal::get_buf_fast(nbBuf.dynamicBufferVarId) == BAD_TEXTUREID)
      nbBuf.dynamicBufferVarId = -1;
  }
}

bool NodeBasedShaderManager::invalidateCachedResources()
{
  node_based_shaders_with_cached_resources.insert(this);
  bool hasLoaded = true;
  for (D3DRESID id : resIdsToRelease)
    release_managed_res(id);
  resIdsToRelease.resize(0);

  nodeBasedTextures.resize(currentTextures2dBlk->paramCount() + currentTextures3dBlk->paramCount());
  nodeBasedTexturesNoSampler.resize(currentTextures2dNoSamplerBlk->paramCount() + currentTextures3dNoSamplerBlk->paramCount());
  nodeBasedBuffers.resize(currentBuffersBlk->paramCount());

  resIdsToRelease.reserve(nodeBasedTextures.size() + nodeBasedTexturesNoSampler.size() + nodeBasedBuffers.size());

  fillTextureCache(nodeBasedTextures, *currentTextures2dBlk, 0, hasLoaded);
  fillTextureCache(nodeBasedTextures, *currentTextures3dBlk, currentTextures2dBlk->paramCount(), hasLoaded);
  fillTextureCache(nodeBasedTexturesNoSampler, *currentTextures2dNoSamplerBlk, 0, hasLoaded);
  fillTextureCache(nodeBasedTexturesNoSampler, *currentTextures3dNoSamplerBlk, currentTextures2dNoSamplerBlk->paramCount(), hasLoaded);
  fillBufferCache(*currentBuffersBlk, 0, hasLoaded);

  return hasLoaded;
}

void NodeBasedShaderManager::clearAllCachedResources()
{
  for (NodeBasedShaderManager *mgr : node_based_shaders_with_cached_resources)
    mgr->resetCachedResources();
  node_based_shaders_with_cached_resources.clear();
}

const char *node_based_shader_current_platform_suffix() { return PLATFORM_LABELS[get_nbsm_platform()]; }

eastl::string node_based_shader_get_resource_name(const char *shader_path)
{
  return eastl::string(eastl::string::CtorSprintf{}, "%s_%s", normalize_shader_res_name(shader_path).c_str(),
    node_based_shader_current_platform_suffix());
}
#endif

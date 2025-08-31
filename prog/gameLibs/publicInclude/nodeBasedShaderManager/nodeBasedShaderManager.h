//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_TMatrix4.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <webui/nodeBasedShaderType.h>
#include <3d/dag_resMgr.h>
#include <drv/3d/dag_samplerHandle.h>

class InPlaceMemLoadCB;
class BaseTexture;
class Sbuffer;

class NodeBasedShaderManager
{
public:
  enum PLATFORM
  {
    DX11 = 0,
    XBOXONE,
    PS4,
    VULKAN,
    DX12,
    DX12_XBOX_ONE,
    DX12_XBOX_SCARLETT,
    PS5,
    MTL,
    VULKAN_BINDLESS,
    MTL_BINDLESS,
    COUNT,
    ALL = -1,
  };

  using ShaderBin = Tab<uint32_t>;
  using ShaderBinPermutations = eastl::vector<ShaderBin>;
  using ShaderBinVariants = eastl::vector<ShaderBinPermutations>;

private:
  using ArrayValue = eastl::pair<String, const Tab<Point4> *>;

  DataBlock emptyBlk;
  uint32_t lastShaderSourceHash = 0;
  bool lastCompileIsSuccess = false;
  String lastErrors;
  eastl::unique_ptr<DataBlock> currentIntParametersBlk, currentInt4ParametersBlk, currentFloatParametersBlk,
    currentFloat4ParametersBlk, currentFloat4x4ParametersBlk, currentTextures2dBlk, currentTextures3dBlk, currentTextures2dArrayBlk,
    currentTextures2dShdArrayBlk, currentTextures2dNoSamplerBlk, currentTextures3dNoSamplerBlk, currentBuffersBlk, currentCBuffersBlk,
    metadataBlk;

  template <class T>
  struct CachedVariable
  {
    const char *paramName; // it points to datablock and so persists
    T val;
    int varType = -1, varId = -1;
    CachedVariable(const char *n, const T &v) : paramName(n), val(v) {}
    CachedVariable(const char *n, int vt, int vid) : paramName(n), varType(vt), varId(vid) {}
  };

  typedef CachedVariable<int> CachedInt;
  typedef CachedVariable<IPoint4> CachedInt4;
  typedef CachedVariable<float> CachedFloat;
  typedef CachedVariable<Point4> CachedFloat4;
  typedef CachedVariable<Matrix44> CachedFloat4x4;

  uint32_t cachedVariableGeneration = ~0u;
  eastl::vector<CachedInt> currentIntVariables;
  eastl::vector<CachedInt4> currentInt4Variables;
  eastl::vector<CachedFloat> currentFloatVariables;
  eastl::vector<CachedFloat4> currentFloat4Variables;
  eastl::vector<CachedFloat4x4> currentFloat4x4Variables;
  String shaderName, shaderFileSuffix;
  eastl::vector<String> optionalGraphNames;
  eastl::vector<ArrayValue> arrayValues;
  uint32_t permutationId;
  NodeBasedShaderType shader;


  struct NodeBasedTexture
  {
    BaseTexture *usedTextureResId = nullptr;
    int dynamicTextureVarId = -1;
    d3d::SamplerHandle sampler = d3d::INVALID_SAMPLER_HANDLE;
    int dynamicSamplerVarId = -1;
  };

  struct NodeBasedBuffer
  {
    Sbuffer *usedBufferResId;
    int dynamicBufferVarId;
  };

  dag::Vector<NodeBasedTexture> nodeBasedTextures;
  dag::Vector<NodeBasedBuffer> nodeBasedBuffers;
  dag::Vector<NodeBasedBuffer> nodeBasedCBuffers;
  dag::Vector<D3DRESID> resIdsToRelease;
  bool isDirty = true;

  bool compileShaderProgram(const DataBlock &shader_blk, String &out_error, PLATFORM platform_id);
  void updateBlkDataConstBuffer(const DataBlock &shader_blk);
  void updateBlkDataTextures(const DataBlock &shader_blk);
  void updateBlkDataBuffers(const DataBlock &shader_blk);
  void updateBlkDataResources(const DataBlock &shader_blk);
  void precacheVariables();

  void setBaseConstantBuffer();
  void setSubConstantBuffers() const;
  void setTextures(int &offset) const;
  void setBuffers(int &offset) const;

  void loadShaderFromStream(uint32_t idx, uint32_t platform_id, InPlaceMemLoadCB &load_stream, uint32_t variant_id);

  const ArrayValue *getArrayValue(const char *name, size_t name_length) const;

  bool invalidateCachedResources();

  void fillTextureCache(const DataBlock &src_block, int offset, bool &has_loaded, bool has_sampler);
  void fillBufferCache(const DataBlock &src_block, dag::Vector<NodeBasedBuffer> &bufferCache, int offset, bool &has_loaded);

  eastl::array<ShaderBinVariants, PLATFORM::COUNT> shaderBin;
  eastl::vector<DataBlock> permParameters;

public:
  NodeBasedShaderManager(NodeBasedShaderType shader, const String &shader_name, const String &shader_file_suffix) :
    shaderName(shader_name), shaderFileSuffix(shader_file_suffix), shader(shader)
  {
    permutationId = 0;
    optionalGraphNames.resize(1);
    uint32_t variantsCnt = get_shader_variant_count(shader);
    for (ShaderBinVariants &shVariant : shaderBin)
    {
      shVariant.resize(variantsCnt);
      for (ShaderBinPermutations &shBin : shVariant)
        shBin.resize(1);
    }
    permParameters.resize(1);
  }
  ~NodeBasedShaderManager();

  bool loadFromResources(const String &shader_path, bool keep_permutation = false);
  void saveToFile(const String &shader_path, PLATFORM platform_id) const;
  void getShadersBinariesFileNames(const String &shader_path, Tab<String> &file_names, PLATFORM platform_id) const;

  void setArrayValue(const char *name, const Tab<Point4> &values);

  bool update(const DataBlock &shader_blk, String &out_errors, PLATFORM platform_id);

  const ShaderBinPermutations &getPermutationShadersBin(uint32_t variant_id) const;
  uint32_t getCurrentPermutationIdx() const { return permutationId; }
  void setConstants();
  void resetSubCbuffers();

  static void getPlatformList(Tab<String> &out_platforms, PLATFORM platform_id);

  void enableOptionalGraph(const String &graph_name, bool enable);

  void resetCachedResources()
  {
    for (D3DRESID id : resIdsToRelease)
      release_managed_res(id);
    nodeBasedTextures.clear();
    nodeBasedBuffers.clear();
    resIdsToRelease.clear();
    cachedVariableGeneration = ~0u;
    isDirty = true;
  }

  static void clearAllCachedResources();
  static void initCompilation();
};

const char *node_based_shader_current_platform_suffix();
eastl::string node_based_shader_get_resource_name(const char *shader_path);

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
#include <EASTL/string_map.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_TMatrix4.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <webui/nodeBasedShaderType.h>
#include <3d/dag_resMgr.h>
#include <drv/3d/dag_samplerHandle.h>
#include <shaders/dag_shBindumps.h>

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

private:
  struct PermutationDesc
  {
    uint32_t groupIdx, elemIdx;
  };

  // Common fields
  uint32_t lastShaderSourceHash = 0;
  bool lastCompileIsSuccess = false;
  String lastErrors;

  eastl::string_map<PermutationDesc> optionalGraphMap;

  ShaderBindumpHandle scriptedShadersDumpHandle = INVALID_BINDUMP_HANDLE;
  eastl::string scriptedShadersDumpName{};
  uint32_t cachedDumpGeneration = 0;

  String shaderName, shaderFileSuffix;
  eastl::vector<uint32_t> permGroupSizes;
  eastl::vector<uint32_t> permGroupActiveIdx;
  NodeBasedShaderQuality nbsQuality;
  uint32_t permCount;
  NodeBasedShaderType shader;

  bool compileScriptedShaders(const String &shader_name, const DataBlock &shader_blk, String &out_error);

  bool loadOptionalGraphsInfoFromStream(InPlaceMemLoadCB &load_stream);
  bool loadScriptedShadersFromResources(dag::Span<uint8_t> data, const String &shader_name, bool keep_permutation = false);

  // For compilation
  static String toolsPath;
  static String rootPath;

public:
  NodeBasedShaderManager(NodeBasedShaderType shader, const String &shader_name, const String &shader_file_suffix) :
    shaderName(shader_name),
    shaderFileSuffix(shader_file_suffix),
    shader(shader),
    permCount(0),
    nbsQuality(NodeBasedShaderQuality::Low)
  {}
  ~NodeBasedShaderManager();

  void shutdown();

  bool loadFromResources(const String &shader_path, bool keep_permutation = false);
  void getShadersBinariesFileNames(const String &shader_path, Tab<String> &file_names, PLATFORM platform_id) const;

  void setQuality(NodeBasedShaderQuality nbs_quality) { nbsQuality = nbs_quality; }

  bool update(const String &shader_name, const DataBlock &shader_blk, String &out_errors);

  void enableOptionalGraph(const String &graph_name, bool enable);

  static void initCompilation();

  static String buildScriptedShaderName(char const *asset);

  ShaderBindumpHandle bindumpHandle() const { return scriptedShadersDumpHandle; }
  char const *getShaderBlockName() const;
  void setShadervars(int variant_id);
};

const char *node_based_shader_current_platform_suffix();
eastl::string node_based_shader_get_resource_name(const char *shader_path);

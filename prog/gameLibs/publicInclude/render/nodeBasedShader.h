//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_driver.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_string.h>
#include <webui/nodeBasedShaderType.h>
#include <shaders/dag_computeShaders.h>

class NodeBasedShaderManager;
class DataBlock;

class NodeBasedShader
{
  // Dshl backend data
  ComputeShaderElement *computeShader = nullptr;
  int shaderBlockId = -1;

  eastl::unique_ptr<NodeBasedShaderManager> shaderManager;
  String loadedShaderName;
  uint32_t variantId = 0;

  void createShaders();

  // called when backing shader memory is to be invalidated
  void shutdown();

public:
  NodeBasedShader(NodeBasedShaderType shader, const String &shader_name, const String &shader_file_suffix, uint32_t variant_id);
  ~NodeBasedShader();

  void init(const String &blk, bool keep_permutation = false);
  void reset();
  void closeShader();

  bool update(const String &shader_name, const DataBlock &shader_blk, String &out_errors);
  void dispatch(int xdim, int ydim, int zdim) const;
  bool isValid() const;
  void enableOptionalGraph(const String &graph_name, bool enable);
  void setQuality(NodeBasedShaderQuality nbs_quality);
};

namespace nodebasedshaderutils
{
eastl::vector<String> getAvailableVolumeChannels();
eastl::vector<String> getAvailableInt();
eastl::vector<String> getAvailableFloat();
eastl::vector<String> getAvailableFloat4();
eastl::vector<String> getAvailableTextures();
eastl::vector<String> getAvailableBuffers();
} // namespace nodebasedshaderutils

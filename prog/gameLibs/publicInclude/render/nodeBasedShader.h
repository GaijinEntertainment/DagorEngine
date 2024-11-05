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

class NodeBasedShaderManager;
class DataBlock;

class NodeBasedShader
{
  struct ShaderProgramDeleter
  {
    void operator()(PROGRAM *ptr)
    {
      if (ptr)
      {
        d3d::delete_program(*ptr);
        delete ptr;
        ptr = nullptr;
      }
    }
  };

  using PerFrameFogShaderType = eastl::unique_ptr<PROGRAM, ShaderProgramDeleter>;

  eastl::vector<PerFrameFogShaderType> shadersCache;
  eastl::unique_ptr<NodeBasedShaderManager> shaderManager;
  String loadedShaderName;
  uint32_t variantId = 0;
  uint32_t currentShaderIdx = 0;

  void createShaders();

public:
  NodeBasedShader(NodeBasedShaderType shader, const String &shader_name, const String &shader_file_suffix, uint32_t variant_id);
  ~NodeBasedShader();

  void init(const DataBlock &blk);
  void init(const String &blk, bool keep_permutation = false);
  void reset();
  void closeShader();

  bool update(const String &shader_name, const DataBlock &shader_blk, String &out_errors);
  void dispatch(int xdim, int ydim, int zdim) const;
  bool isValid() const;
  void enableOptionalGraph(const String &graph_name, bool enable);

  void setArrayValue(const char *name, const Tab<Point4> &values);

  const DataBlock &getMetadata() const;

  PROGRAM *getProgram();
};

namespace nodebasedshaderutils
{
eastl::vector<String> getAvailableInt();
eastl::vector<String> getAvailableFloat();
eastl::vector<String> getAvailableFloat4();
eastl::vector<String> getAvailableTextures();
eastl::vector<String> getAvailableBuffers();
} // namespace nodebasedshaderutils

//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_shaders.h>

class DynamicShaderHelper
{
public:
  DynamicShaderHelper() = default;
  DynamicShaderHelper(const char *shader_name) { init(shader_name, nullptr, 0, shader_name); }
  Ptr<ShaderMaterial> material;
  Ptr<ShaderElement> shader;

  bool init(const char *shader_name, CompiledShaderChannelId *channels, int num_channels, const char *module_info,
    bool optional = false);

  void close();
};

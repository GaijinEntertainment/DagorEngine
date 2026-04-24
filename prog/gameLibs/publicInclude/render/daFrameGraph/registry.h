//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/baseRegistry.h>


namespace dafg
{

class Registry : public BaseRegistry
{
  friend class NameSpace;

  Registry(NodeNameId node, InternalRegistry *reg) : BaseRegistry(node, reg) {}

public:
  auto bindTexPs(const char *tex_name, const char *shader_var_name)
  {
    return BaseRegistry::read(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
  }
  auto bindTexVs(const char *tex_name, const char *shader_var_name)
  {
    return BaseRegistry::read(tex_name).texture().atStage(dafg::Stage::VS).bindToShaderVar(shader_var_name);
  }
  auto bindTexCs(const char *tex_name, const char *shader_var_name)
  {
    return BaseRegistry::read(tex_name).texture().atStage(dafg::Stage::CS).bindToShaderVar(shader_var_name);
  }

  template <typename T>
  auto bindBlob(const char *blob_name, const char *shader_var_name)
  {
    return BaseRegistry::read(blob_name).blob<T>().bindToShaderVar(shader_var_name);
  }

  auto readTexHndlPs(const char *tex_name)
  {
    return BaseRegistry::read(tex_name).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
  }

  auto readTexHndlVs(const char *tex_name)
  {
    return BaseRegistry::read(tex_name).texture().atStage(dafg::Stage::VS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
  }

  auto readTexHndlCs(const char *tex_name)
  {
    return BaseRegistry::read(tex_name).texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
  }

  void postFx(const char *shader_name)
  {
    BaseRegistry::draw(shader_name, DrawPrimitive::TRIANGLE_LIST).primitiveCount(1).instanceCount(1);
  }

  auto createTexture2d(const char *name, Texture2dCreateInfo info) { return create(name).texture(info); }

  template <class T>
  auto createBlob(const char *name)
  {
    return create(name).blob<T>();
  }
};

} // namespace dafg

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>

#include <render/nodeBasedShader.h>
#include <drv/3d/dag_dispatch.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_lock.h>

void NodeBasedShader::dispatch(int xdim, int ydim, int zdim) const
{
  if (DAGOR_UNLIKELY(!computeShader || shaderBlockId < 0))
  {
    logerr("NodeBasedShader::dispatch failed : shader was not loaded");
    return;
  }

  shaderManager->setShadervars(variantId);

  {
    SCOPE_RESET_SHADER_BLOCKS;
    ShaderGlobal::setBlock(shaderBlockId, ShaderGlobal::LAYER_SCENE);
    computeShader->dispatch(xdim, ydim, zdim);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
  }

  return;
}

bool NodeBasedShader::update(const String &shader_name, const DataBlock &shader_blk, String &out_errors)
{
  if (shaderManager->update(shader_name, shader_blk, out_errors))
  {
    loadedShaderName = shader_name;
    createShaders();
    return true;
  }
  return false;
}

NodeBasedShader::NodeBasedShader(NodeBasedShaderType shader, const String &shader_name, const String &shader_file_suffix,
  uint32_t variant_id) :
  variantId(variant_id), shaderManager(eastl::make_unique<NodeBasedShaderManager>(shader, shader_name, shader_file_suffix))
{
  G_ASSERTF(variant_id < get_shader_variant_count(shader) && int(variant_id) >= 0, "Unknown variant id for shader #%d: %d",
    (int)shader, variant_id);
}
NodeBasedShader::~NodeBasedShader() { shutdown(); }

void NodeBasedShader::closeShader()
{
  delete eastl::exchange(computeShader, nullptr);
  shaderBlockId = -1;
}

// this should be called afterReset and not sure if its needed at all if we don't destroy engine shaders
void NodeBasedShader::reset()
{
  shutdown();
  init(loadedShaderName, true);
}

void NodeBasedShader::shutdown()
{
  delete eastl::exchange(computeShader, nullptr);
  shaderBlockId = -1;

  shaderManager->shutdown();
}

void NodeBasedShader::createShaders()
{
  auto dumpHnd = shaderManager->bindumpHandle();
  auto shaderName = NodeBasedShaderManager::buildScriptedShaderName(loadedShaderName);
  if (!computeShader)
  {
    computeShader = new_compute_shader(dumpHnd, shaderName);
    G_ASSERTF(computeShader, "Can't create compute shader");
  }
  if (shaderBlockId == -1)
  {
    shaderBlockId = ShaderGlobal::getBlockId(shaderManager->getShaderBlockName(), ShaderGlobal::LAYER_SCENE);
    G_ASSERTF(shaderBlockId >= 0, "Can't fetch nbs block");
  }
  return;
}

void NodeBasedShader::init(const String &shader_name, bool keep_permutation)
{
  if (shader_name.empty())
    return;
  loadedShaderName = shader_name;
  if (shaderManager->loadFromResources(shader_name, keep_permutation))
    createShaders();
  else
    closeShader();
}

bool NodeBasedShader::isValid() const { return computeShader != nullptr; }

template <int VarType>
eastl::vector<String> getAvailableShaderVars()
{
  eastl::vector<String> availableShaderVars;
  for (int i = 0; i < VariableMap::getVariablesCount(); ++i)
  {
    String name(VariableMap::getVariableName(i));
    const int varId = VariableMap::getVariableId(name.c_str());
    if (!name)
      continue;
    if (ShaderGlobal::get_var_type(varId) == VarType && (VarType != SHVT_TEXTURE || get_managed_texture_id(name) != BAD_TEXTUREID))
      availableShaderVars.push_back(name);
  }
  return availableShaderVars;
}

void NodeBasedShader::enableOptionalGraph(const String &graph_name, bool enable)
{
  shaderManager->enableOptionalGraph(graph_name, enable);
}

void NodeBasedShader::setQuality(NodeBasedShaderQuality nbs_quality) { shaderManager->setQuality(nbs_quality); }


namespace nodebasedshaderutils
{
eastl::vector<String> getAvailableInt() { return getAvailableShaderVars<SHVT_INT>(); }
eastl::vector<String> getAvailableFloat() { return getAvailableShaderVars<SHVT_REAL>(); }
eastl::vector<String> getAvailableFloat4() { return getAvailableShaderVars<SHVT_COLOR4>(); }
eastl::vector<String> getAvailableTextures() { return getAvailableShaderVars<SHVT_TEXTURE>(); }
eastl::vector<String> getAvailableBuffers() { return getAvailableShaderVars<SHVT_BUFFER>(); }
} // namespace nodebasedshaderutils

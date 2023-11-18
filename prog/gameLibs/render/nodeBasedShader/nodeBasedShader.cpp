#include <ioSys/dag_dataBlock.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <shaders/dag_shaders.h>

#include <render/nodeBasedShader.h>

void NodeBasedShader::dispatch(int xdim, int ydim, int zdim) const
{
  if (shadersCache.empty() || !shadersCache[currentShaderIdx])
    return;

  shaderManager->setConstants();
  d3d::set_program(*shadersCache[currentShaderIdx]);
  d3d::dispatch(xdim, ydim, zdim);
  d3d::release_cb0_data(STAGE_CS);
  ShaderElement::invalidate_cached_state_block();
}

void NodeBasedShader::setArrayValue(const char *name, const Tab<Point4> &values)
{
  if (shadersCache.empty() || !shadersCache[currentShaderIdx])
    return;

  shaderManager->setArrayValue(name, values);
}

bool NodeBasedShader::update(const String &shader_name, const DataBlock &shader_blk, String &out_errors)
{
  if (shaderManager->update(shader_blk, out_errors, NodeBasedShaderManager::PLATFORM::ALL))
  {
    loadedShaderName = nullptr;
    shaderManager->saveToFile(shader_name, NodeBasedShaderManager::PLATFORM::ALL);
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
NodeBasedShader::~NodeBasedShader() = default;

void NodeBasedShader::init(const DataBlock &blk) { init(String(blk.getStr("rootFogGraph", nullptr))); }

void NodeBasedShader::closeShader()
{
  shadersCache.clear();
  shaderManager->resetCachedResources();
}

void NodeBasedShader::reset()
{
  shadersCache.clear();
  init(loadedShaderName, true);
}

void NodeBasedShader::createShaders()
{
  const NodeBasedShaderManager::ShaderBinPermutations &shaderBins = shaderManager->getPermutationShadersBin(variantId);
  shadersCache.clear();
  shadersCache.reserve(shaderBins.size());
  for (const auto &shaderBin : shaderBins)
  {
    PROGRAM updatedProgram = d3d::create_program_cs(shaderBin.data(), CSPreloaded::No);
    G_ASSERTF(updatedProgram != BAD_PROGRAM, "Can't create compute shader");
    shadersCache.emplace_back(new PROGRAM{updatedProgram}); // -V1023
  }
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

bool NodeBasedShader::isValid() const
{
  return !shadersCache.empty() && shadersCache[currentShaderIdx] && *shadersCache[currentShaderIdx] != BAD_PROGRAM;
}

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
  currentShaderIdx = shaderManager->getCurrentPermutationIdx();
}

PROGRAM *NodeBasedShader::getProgram() { return shadersCache[currentShaderIdx].get(); }

const DataBlock &NodeBasedShader::getMetadata() const { return shaderManager->getMetadata(); }

namespace nodebasedshaderutils
{
eastl::vector<String> getAvailableInt() { return getAvailableShaderVars<SHVT_INT>(); }
eastl::vector<String> getAvailableFloat() { return getAvailableShaderVars<SHVT_REAL>(); }
eastl::vector<String> getAvailableFloat4() { return getAvailableShaderVars<SHVT_COLOR4>(); }
eastl::vector<String> getAvailableTextures() { return getAvailableShaderVars<SHVT_TEXTURE>(); }
eastl::vector<String> getAvailableBuffers() { return getAvailableShaderVars<SHVT_BUFFER>(); }
} // namespace nodebasedshaderutils

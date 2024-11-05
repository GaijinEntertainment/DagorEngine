// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <backend/intermediateRepresentation.h>
#include <dag/dag_vectorSet.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <memory/dag_framemem.h>

#include <frontend/internalRegistry.h>
#include <id/idRange.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <render/daBfg/bfg.h>

#include <shaders/dag_stcode.h>


// NOTE: this is defined inside scriptSElem.cpp
extern void (*scripted_shader_element_on_before_resource_used)(const ShaderElement *selem, const D3dResource *, const char *);

// NOTE: this is defined inside shaderBlock.cpp
extern void (*shader_block_on_before_resource_used)(const D3dResource *, const char *);

namespace dabfg
{

static dag::VectorSet<const D3dResource *> managed_resources;
static dag::VectorSet<const D3dResource *> validated_external_resources;
static dag::VectorSet<const D3dResource *> current_node_resources;
static eastl::string current_node_name;

static dag::VectorSet<eastl::pair<eastl::string, eastl::string>> found_errors;

struct ResKeyHash
{
  inline unsigned operator()(const D3DRESID &k) const { return eastl::hash<unsigned>{}(unsigned(k)); }
};

static eastl::string get_res_owning_shadervars(const D3dResource *res)
{
  eastl::string message;
  for (int i = 0, n = VariableMap::getGlobalVariablesCount(); i < n; ++i)
  {
    const char *name = VariableMap::getGlobalVariableName(i);
    if (!name)
      continue;
    int varId = VariableMap::getVariableId(name);
    int varType = ShaderGlobal::get_var_type(varId);
    if ((varType == SHVT_TEXTURE && D3dResManagerData::getD3dRes(ShaderGlobal::get_tex(varId)) == res) ||
        (varType == SHVT_BUFFER && D3dResManagerData::getD3dRes(ShaderGlobal::get_buf(varId)) == res))
      message.append_sprintf("'%s', ", VariableMap::getVariableName(varId));
  }
  if (!message.empty())
    message.resize(message.size() - 2);
  else
    message.append("none");
  return message;
}

static eastl::string get_res_owning_dynamic_vars(const ShaderElement *selem, const D3dResource *res)
{
  // No dynamic vars for buffers
  if (res->restype() != RES3D_TEX)
    return "none";

  eastl::string message;
  const int texCount = selem->getTextureCount();
  for (int i = 0; i < texCount; ++i)
  {
    const D3DRESID id = selem->getTexture(i);
    if (id != BAD_D3DRESID && res == D3dResManagerData::getD3dRes(id))
      message.append_sprintf("%d, ", i); // TODO: resolve the name of the shader var
  }
  if (!message.empty())
    message.resize(message.size() - 2);
  else
    message.append("none");

  return message;
}

static bool should_complain(const D3dResource *res)
{
  // If the resource is not FG-managed and is not an external one that user
  // explicitly asked us to validate, don't complain about it.
  if (managed_resources.find(res) == managed_resources.end() &&
      validated_external_resources.find(res) == validated_external_resources.end())
    return false;

  // If the node properly requested the resource -- everything is OK
  if (current_node_resources.find(res) != current_node_resources.end())
    return false;

  // Complain only once per resource per node (or lack of a node).
  if (found_errors.find({res->getResName(), current_node_name}) != found_errors.end())
    return false;

  found_errors.emplace(res->getResName(), current_node_name);
  return true;
}

// NOTE: defines used for simple string literal concatenation
#define OUTSIDE_OF_NODE_ERROR_MESSAGE                                  \
  "daBfg: discovered a frame graph resource '%s' which is bound into " \
  "a d3d register outside of a daBfg node, this is UB!"

#define INSIDE_NODE_ERROR_MSSAGE                                       \
  "daBfg: discovered a frame graph resource '%s' which is bound into " \
  "a d3d register in node '%s' without being requested by it, this is UB!"

#define IN_SHADER_EXPLANATION                                                         \
  "Reason: the resource was leaked outside of FG through a shader variable "          \
  "(local or global) and accessed through it while applying STCODE for shader '%s'. " \
  "Please, unbind the resource from the following shader variables: %s. "             \
  "Also make sure it is not bound into a dynamic variable through the following variables: %s."

#define IN_BLOCK_EXPLANATION                                                        \
  "Reason: the resource was leaked outside of FG through a global shader variable " \
  "and accessed through it while applying STCODE for shader block '%s'. "           \
  "Please, unbind the resource from the following shader variables: %s"

#define IN_CPPSTCODE_EXPLANATION                                                    \
  "Reason: the resource was leaked outside of FG through a global shader variable " \
  "and accessed through it while apply cppstcode routine '%s'. "                    \
  "Please, unbind the resource from the following shader variables: %s"

static void validate_shader(const ShaderElement *selem, const D3dResource *res, const char *shader_name)
{
  if (!should_complain(res))
    return;

  if (current_node_name.empty())
    logerr(OUTSIDE_OF_NODE_ERROR_MESSAGE " " IN_SHADER_EXPLANATION, res->getResName(), shader_name, get_res_owning_shadervars(res),
      get_res_owning_dynamic_vars(selem, res));
  else
    logerr(INSIDE_NODE_ERROR_MSSAGE " " IN_SHADER_EXPLANATION, res->getResName(), current_node_name, shader_name,
      get_res_owning_shadervars(res), get_res_owning_dynamic_vars(selem, res));
}

static void validate_block(const D3dResource *res, const char *block_name)
{
  if (!should_complain(res))
    return;

  if (current_node_name.empty())
    logerr(OUTSIDE_OF_NODE_ERROR_MESSAGE " " IN_BLOCK_EXPLANATION, res->getResName(), block_name, get_res_owning_shadervars(res));
  else
    logerr(INSIDE_NODE_ERROR_MSSAGE " " IN_BLOCK_EXPLANATION, res->getResName(), block_name, get_res_owning_shadervars(res));
}

static void validate_cppstcode(const D3dResource *res, const char *routine_name)
{
  if (!should_complain(res))
    return;

  if (current_node_name.empty())
    logerr(OUTSIDE_OF_NODE_ERROR_MESSAGE " " IN_CPPSTCODE_EXPLANATION, res->getResName(), routine_name,
      get_res_owning_shadervars(res));
  else
    logerr(INSIDE_NODE_ERROR_MSSAGE " " IN_CPPSTCODE_EXPLANATION, res->getResName(), routine_name, get_res_owning_shadervars(res));
}

#undef OUTSIDE_OF_NODE_ERROR_MESSAGE
#undef INSIDE_NODE_ERROR_MSSAGE
#undef IN_SHADER_EXPLANATION
#undef IN_BLOCK_EXPLANATION

void validation_restart()
{
  scripted_shader_element_on_before_resource_used = &validate_shader;
  shader_block_on_before_resource_used = &validate_block;
  stcode::set_on_before_resource_used_cb(&validate_cppstcode);
  managed_resources.clear();
}

void validation_set_current_node(const InternalRegistry &registry, NodeNameId nodeId)
{
  current_node_resources.clear();
  current_node_name.clear();

  if (nodeId == NodeNameId::Invalid)
    return;

  current_node_name = registry.knownNames.getName(nodeId);

  current_node_resources.reserve(
    registry.resourceProviderReference.providedResources.size() + registry.resourceProviderReference.providedHistoryResources.size());

  auto addResource = [](const auto &var) {
    if (auto tex = eastl::get_if<ManagedTexView>(&var); tex != nullptr && *tex)
    {
      current_node_resources.emplace(static_cast<const D3dResource *>(tex->getBaseTex()));
    }
    else if (auto buf = eastl::get_if<ManagedBufView>(&var); buf != nullptr && *buf)
    {
      current_node_resources.emplace(static_cast<const D3dResource *>(buf->getBuf()));
    }
  };

  for (const auto &[id, res] : registry.resourceProviderReference.providedResources)
    addResource(res);

  for (const auto &[id, res] : registry.resourceProviderReference.providedHistoryResources)
    addResource(res);
}

void validation_add_resource(const D3dResource *res) { managed_resources.emplace(res); }

void validation_of_external_resources_duplication(
  const IdIndexedMapping<intermediate::ResourceIndex, eastl::optional<ExternalResource>> &resources,
  const IdIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> &resourceNames)
{
  static ska::flat_hash_set<D3DRESID, ResKeyHash> alreadyLoggedResources;

  FRAMEMEM_VALIDATE; // Reserved amount DEFINITELY should be enough
  ska::flat_hash_map<D3DRESID, intermediate::ResourceIndex, ResKeyHash, eastl::equal_to<D3DRESID>, framemem_allocator>
    setOfExternalResources;
  setOfExternalResources.reserve(resources.size());

  for (const auto resIdx : IdRange<intermediate::ResourceIndex>(resources.size()))
  {
    if (!resources[resIdx])
      continue;

    D3DRESID resId;
    static_assert(eastl::variant_size_v<ExternalResource> == 2);
    if (auto tex = eastl::get_if<ManagedTexView>(&*resources[resIdx]))
      resId = tex->getTexId();
    else if (auto buf = eastl::get_if<ManagedBufView>(&*resources[resIdx]))
      resId = buf->getBufId();

    if (auto [it, succ] = setOfExternalResources.emplace(resId, resIdx); !succ)
      // TODO: When all duplications will be fixed change logwarn to logerr. Now we can't becuse have some duplications in master.
      if (alreadyLoggedResources.insert(resId).second)
        logwarn("The physical resource '%s' was registered as an external resource into frame graph"
                " for two different intermediate resources: '%s' and '%s'. This is not allowed and"
                " will lead to broken barriers and resource states!",
          ::get_managed_res_name(resId), resourceNames[resIdx], resourceNames[it->second]);
  }
}

} // namespace dabfg

namespace dabfg
{
void mark_external_resource_for_validation(const D3dResource *resource) { validated_external_resources.insert(resource); }
} // namespace dabfg
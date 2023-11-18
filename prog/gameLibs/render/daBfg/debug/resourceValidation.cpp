#include <backend/intermediateRepresentation.h>
#include <dag/dag_vectorSet.h>
#include <shaders/dag_shaderVar.h>
#include <memory/dag_framemem.h>

#include <frontend/internalRegistry.h>
#include <id/idRange.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <render/daBfg/bfg.h>


// NOTE: this is defined inside scriptSElem.cpp
extern void (*scripted_shader_element_on_before_resource_used)(const D3dResource *, const char *);

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
  eastl::string message = "Shadervars bound to illegally accessed FG resource:";
  for (int varId = 0, n = VariableMap::getGlobalVariablesCount(); varId < n; ++varId)
  {
    int varType = ShaderGlobal::get_var_type(varId);
    if ((varType == SHVT_TEXTURE && D3dResManagerData::getD3dRes(ShaderGlobal::get_tex(varId)) == res) ||
        (varType == SHVT_BUFFER && D3dResManagerData::getD3dRes(ShaderGlobal::get_buf(varId)) == res))
      message.append_sprintf("\n  - '%s'", VariableMap::getVariableName(varId));
  }
  return message;
}

static void validate(const D3dResource *res, const char *current_shader_class)
{
  if (managed_resources.find(res) == managed_resources.end() &&
      validated_external_resources.find(res) == validated_external_resources.end())
    return;

  if (current_node_name.empty())
  {
    if (found_errors.find({res->getResName(), ""}) == found_errors.end())
    {
      found_errors.emplace(res->getResName(), "");
      logerr("Framegraph resource %s was used by %s outside of a node's execute method! "
             "This should never happen, make sure that you are not caching "
             "references to FG resources somewhere.\n%s",
        res->getResName(), current_shader_class, get_res_owning_shadervars(res));
    }
    return;
  }

  if (current_node_resources.find(res) == current_node_resources.end() &&
      found_errors.find({res->getResName(), current_node_name}) == found_errors.end())
  {
    found_errors.emplace(res->getResName(), current_node_name);
    logerr("Framegraph resource %s was used by %s in node %s either without being "
           "requested by the node at all, or without being provided by "
           "framegraph afterwards.\n%s",
      res->getResName(), current_shader_class, current_node_name, get_res_owning_shadervars(res));
  }
}

void validation_restart()
{
  scripted_shader_element_on_before_resource_used = &validate;
  shader_block_on_before_resource_used = &validate;
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
  const IdIndexedMapping<intermediate::ResourceIndex, intermediate::Resource> &resources,
  const IdIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> &resourceNames)
{
  static ska::flat_hash_set<D3DRESID, ResKeyHash> alreadyLoggedResources;

  FRAMEMEM_VALIDATE; // Reserved amount DEFINITELY should be enough
  ska::flat_hash_map<D3DRESID, intermediate::ResourceIndex, ResKeyHash, eastl::equal_to<D3DRESID>, framemem_allocator>
    setOfExternalResources;
  setOfExternalResources.reserve(resources.size());

  for (const auto resIdx : IdRange<intermediate::ResourceIndex>(resources.size()))
  {
    if (!resources[resIdx].isExternal())
      continue;

    const auto resType = resources[resIdx].getResType();

    D3DRESID resId;
    if (resType == ResourceType::Texture)
      resId = resources[resIdx].getExternalTex().getTexId();
    else if (resType == ResourceType::Buffer)
      resId = resources[resIdx].getExternalBuf().getBufId();
    else
      G_ASSERT(false);

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
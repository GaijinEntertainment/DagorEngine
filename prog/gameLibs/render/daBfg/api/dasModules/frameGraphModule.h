#pragma once

#include <nodes/nodeTracker.h>
#include <api/internalRegistry.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorDriver3d.h>
#include <dasModules/dasScriptsLoader.h>
#include <api/dasModules/genericBindings/relocatableFixedVector.h>
#include <api/dasModules/genericBindings/optional.h>
#include <api/dasModules/genericBindings/fixedVectorSet.h>
#include <api/dasModules/genericBindings/fixedVectorMap.h>
#include <api/dasModules/genericBindings/idHierarchicalNameMap.h>
#include <api/dasModules/genericBindings/idIndexedMapping.h>


namespace dabfg
{
inline NodeHandle register_external_node(NodeNameId node_id, uint32_t generation) { return {{node_id, generation, true}}; }
} // namespace dabfg

struct DasNameSpaceRequest
{
  dabfg::NameSpaceNameId nameSpaceId;
  dabfg::NodeNameId nodeId;
  dabfg::InternalRegistry *registry;
};

struct DasRegistry : DasNameSpaceRequest
{};

#define DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(enum_name, das_enum_name) \
  DAS_BIND_ENUM_CAST(enum_name)                                       \
  DAS_BASE_BIND_ENUM_FACTORY(enum_name, #das_enum_name)               \
  namespace das                                                       \
  {                                                                   \
  template <>                                                         \
  struct typeName<enum_name>                                          \
  {                                                                   \
    constexpr static const char *name()                               \
    {                                                                 \
      return #das_enum_name;                                          \
    }                                                                 \
  };                                                                  \
  }

DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::NameSpaceNameId, NameSpaceNameId);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::NodeNameId, NodeNameId);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::ResNameId, ResNameId);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::History, History);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(ResourceActivationAction, ResourceActivationAction);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::SideEffects, SideEffect);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::multiplexing::Mode, MultiplexingMode);

DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::Access, Access);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::Usage, Usage);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::Stage, Stage);

DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::ResourceType, ResourceType);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::AutoResTypeNameId, AutoResTypeNameId);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(VariableRateShadingCombiner, VariableRateShadingCombiner);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dabfg::BindingType, BindingType);

#undef DAS_BASE_BIND_ENUM_CAST_AND_FACTORY

MAKE_TYPE_FACTORY(ResourceData, dabfg::ResourceData)
MAKE_TYPE_FACTORY(VirtualPassRequirements, dabfg::VirtualPassRequirements)
MAKE_TYPE_FACTORY(NodeData, dabfg::NodeData)
MAKE_TYPE_FACTORY(InternalRegistry, dabfg::InternalRegistry)
MAKE_TYPE_FACTORY(ResourceRequest, dabfg::ResourceRequest)
MAKE_TYPE_FACTORY(ResourceProvider, dabfg::ResourceProvider)
MAKE_TYPE_FACTORY(NodeTracker, dabfg::NodeTracker)
MAKE_TYPE_FACTORY(Registry, DasRegistry)
MAKE_TYPE_FACTORY(NameSpaceRequest, DasNameSpaceRequest)
MAKE_TYPE_FACTORY(TextureResourceDescription, TextureResourceDescription)
MAKE_TYPE_FACTORY(VolTextureResourceDescription, VolTextureResourceDescription)
MAKE_TYPE_FACTORY(ArrayTextureResourceDescription, ArrayTextureResourceDescription)
MAKE_TYPE_FACTORY(CubeTextureResourceDescription, CubeTextureResourceDescription)
MAKE_TYPE_FACTORY(ArrayCubeTextureResourceDescription, ArrayCubeTextureResourceDescription)
MAKE_TYPE_FACTORY(NodeStateRequirements, dabfg::NodeStateRequirements)
MAKE_TYPE_FACTORY(ResourceUsage, dabfg::ResourceUsage)
MAKE_TYPE_FACTORY(AutoResolutionData, dabfg::AutoResolutionData)
MAKE_TYPE_FACTORY(BufferResourceDescription, BufferResourceDescription)
MAKE_TYPE_FACTORY(NodeHandle, dabfg::NodeHandle)
MAKE_TYPE_FACTORY(StateRequest, dabfg::StateRequest)
MAKE_TYPE_FACTORY(ShaderBlockLayersInfo, dabfg::ShaderBlockLayersInfo)
MAKE_TYPE_FACTORY(VrsStateRequirements, dabfg::VrsStateRequirements)
MAKE_TYPE_FACTORY(VirtualSubresourceRef, dabfg::VirtualSubresourceRef)
MAKE_TYPE_FACTORY(Binding, dabfg::Binding)
MAKE_TYPE_FACTORY(nullopt, eastl::nullopt_t)

namespace das
{
template <>
struct das_auto_cast_move<dabfg::NodeHandle>
{
  __forceinline static dabfg::NodeHandle cast(dabfg::NodeHandle &&expr) { return eastl::move(expr); }
};
} // namespace das

class ContextTryRestartAndLockGuard
{
  das::Context *ctx;

public:
  ContextTryRestartAndLockGuard(das::Context *ctx_) : ctx(ctx_) { ctx_->tryRestartAndLock(); }
  ContextTryRestartAndLockGuard(const ContextTryRestartAndLockGuard &) = delete;
  ContextTryRestartAndLockGuard(ContextTryRestartAndLockGuard &&other) : ctx(other.ctx) { other.ctx = nullptr; }
  ContextTryRestartAndLockGuard &operator=(const ContextTryRestartAndLockGuard &) = delete;
  ~ContextTryRestartAndLockGuard() { ctx ? (void)ctx->unlock() : (void)0; }
};

template <typename C>
eastl::invoke_result_t<C> callDasFunction(das::Context *context, C &&callable)
{
  ContextTryRestartAndLockGuard contextGuard(context);
  if (!context->ownStack)
  {
    das::SharedStackGuard guard(*context, bind_dascript::get_shared_stack());
    return callable();
  }
  else
    return callable();
}

namespace bind_dascript
{

ManagedTexView getTexView(const dabfg::ResourceProvider &provider, dabfg::ResNameId resId, bool history);
ManagedBufView getBufView(const dabfg::ResourceProvider &provider, dabfg::ResNameId resId, bool history);
const dabfg::ResourceProvider &getProvider(dabfg::InternalRegistry *registry);
dabfg::InternalRegistry &getRegistry(const dabfg::NodeTracker &tracker);
int getShaderVariableId(const char *name);
void setEcsNodeHandleHint(ecs::ComponentsInitializer &init, const char *name, uint32_t hash, dabfg::NodeHandle &handle);
void setEcsNodeHandle(ecs::ComponentsInitializer &init, const char *name, dabfg::NodeHandle &handle);
void fastSetInitChildComp(ecs::ComponentsInitializer &init, const ecs::component_t name, const ecs::FastGetInfo &lt,
  const dabfg::NodeHandle &to, const char *nameStr);
void fillSlot(dabfg::NameSpaceNameId ns, const char *slot, dabfg::NameSpaceNameId res_ns, const char *res_name);
using DasExecutionCallback = das::TLambda<void>;
using DasDeclarationCallBack = das::TLambda<DasExecutionCallback, DasRegistry>;
void registerNodeDeclaration(dabfg::NodeData &node_data, dabfg::NameSpaceNameId ns_id, DasDeclarationCallBack declaration_callback,
  das::Context *context);
void resetNode(dabfg::NodeHandle &handle);
void setTextureInfo(dabfg::ResourceData &res, TextureResourceDescription &desc);
void setResolution(dabfg::ResourceData &res, const dabfg::AutoResolutionData &data);
void set_description(dabfg::ResourceData &res, const BufferResourceDescription &desc);
void setNodeSource(dabfg::NodeData &node, const char *source);

class DaBfgModule final : public das::Module
{
public:
  DaBfgModule();
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override;
  void addStructureAnnotations(das::ModuleLibrary &lib);
  void addNodeDataAnnotation(das::ModuleLibrary &lib);
  void addEnumerations(das::ModuleLibrary &lib);
};

} // namespace bind_dascript

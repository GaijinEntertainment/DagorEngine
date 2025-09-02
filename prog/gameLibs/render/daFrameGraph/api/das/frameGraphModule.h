// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorDriver3d.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/dasScriptsLoader.h>

#include <render/daFrameGraph/das/nameSpaceNameId.h>
#include <render/daFrameGraph/das/nodeHandle.h>

#include <frontend/nodeTracker.h>
#include <frontend/internalRegistry.h>
#include <api/das/genericBindings/relocatableFixedVector.h>
#include <api/das/genericBindings/optional.h>
#include <api/das/genericBindings/fixedVectorSet.h>
#include <api/das/genericBindings/fixedVectorMap.h>
#include <api/das/genericBindings/idHierarchicalNameMap.h>
#include <api/das/genericBindings/idIndexedMapping.h>
#include <api/das/blobView.h>


namespace dafg
{
inline NodeHandle register_external_node(NodeNameId node_id, uint16_t generation) { return {{node_id, generation, true}}; }
} // namespace dafg

#define DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(enum_name, das_enum_name) \
  DAS_BIND_ENUM_CAST(enum_name)                                       \
  DAS_BASE_BIND_ENUM_FACTORY(enum_name, #das_enum_name)               \
  namespace das                                                       \
  {                                                                   \
  template <>                                                         \
  struct typeName<enum_name>                                          \
  {                                                                   \
    constexpr static const char *name() { return #das_enum_name; }    \
  };                                                                  \
  }

DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::NodeNameId, NodeNameId);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::ResNameId, ResNameId);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::History, History);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(ResourceActivationAction, ResourceActivationAction);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::SideEffects, SideEffect);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::multiplexing::Mode, MultiplexingMode);

DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::Access, Access);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::Usage, Usage);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::Stage, Stage);

DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::ResourceType, ResourceType);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::AutoResTypeNameId, AutoResTypeNameId);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(VariableRateShadingCombiner, VariableRateShadingCombiner);
DAS_BASE_BIND_ENUM_CAST_AND_FACTORY(dafg::BindingType, BindingType);

#undef DAS_BASE_BIND_ENUM_CAST_AND_FACTORY

MAKE_TYPE_FACTORY(ResourceData, dafg::ResourceData)
MAKE_TYPE_FACTORY(VirtualPassRequirements, dafg::VirtualPassRequirements)
MAKE_TYPE_FACTORY(NodeData, dafg::NodeData)
MAKE_TYPE_FACTORY(InternalRegistry, dafg::InternalRegistry)
MAKE_TYPE_FACTORY(ResourceRequest, dafg::ResourceRequest)
MAKE_TYPE_FACTORY(ResourceProvider, dafg::ResourceProvider)
MAKE_TYPE_FACTORY(NodeTracker, dafg::NodeTracker)
MAKE_TYPE_FACTORY(TextureResourceDescription, TextureResourceDescription)
MAKE_TYPE_FACTORY(VolTextureResourceDescription, VolTextureResourceDescription)
MAKE_TYPE_FACTORY(ArrayTextureResourceDescription, ArrayTextureResourceDescription)
MAKE_TYPE_FACTORY(CubeTextureResourceDescription, CubeTextureResourceDescription)
MAKE_TYPE_FACTORY(ArrayCubeTextureResourceDescription, ArrayCubeTextureResourceDescription)
MAKE_TYPE_FACTORY(NodeStateRequirements, dafg::NodeStateRequirements)
MAKE_TYPE_FACTORY(ResourceUsage, dafg::ResourceUsage)
MAKE_TYPE_FACTORY(AutoResolutionData, dafg::AutoResolutionData)
MAKE_TYPE_FACTORY(BufferResourceDescription, BufferResourceDescription)
MAKE_TYPE_FACTORY(StateRequest, dafg::StateRequest)
MAKE_TYPE_FACTORY(ShaderBlockLayersInfo, dafg::ShaderBlockLayersInfo)
MAKE_TYPE_FACTORY(VrsStateRequirements, dafg::VrsStateRequirements)
MAKE_TYPE_FACTORY(VirtualSubresourceRef, dafg::VirtualSubresourceRef)
MAKE_TYPE_FACTORY(Binding, dafg::Binding)
MAKE_TYPE_FACTORY(nullopt, eastl::nullopt_t)

using NodeHandleVector = dag::Vector<dafg::NodeHandle>;
DAS_BIND_VECTOR(NodeHandleVector, NodeHandleVector, dafg::NodeHandle, "::dag::Vector<dafg::NodeHandle>")

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
    das::SharedFramememStackGuard guard(*context);
    return callable();
  }
  else
    return callable();
}

namespace bind_dascript
{

ManagedTexView getTexView(const dafg::ResourceProvider *provider, dafg::ResNameId resId, bool history);
ManagedBufView getBufView(const dafg::ResourceProvider *provider, dafg::ResNameId resId, bool history);
dafg::BlobView getBlobView(const dafg::ResourceProvider *provider, dafg::ResNameId resId, bool history);
IPoint2 getResolution2(const dafg::ResourceProvider *provider, dafg::AutoResTypeNameId resId);
IPoint3 getResolution3(const dafg::ResourceProvider *provider, dafg::AutoResTypeNameId resId);
const dafg::ResourceProvider *getProvider(dafg::InternalRegistry *registry);
dafg::NodeTracker &getTracker();
dafg::InternalRegistry *getRegistry();
void registerNode(dafg::NodeTracker &node_tracker, dafg::NodeNameId nodeId, das::Context *context);
void setEcsNodeHandleHint(ecs::ComponentsInitializer &init, const char *name, uint32_t hash, dafg::NodeHandle &handle);
void setEcsNodeHandle(ecs::ComponentsInitializer &init, const char *name, dafg::NodeHandle &handle);
void fastSetInitChildComp(ecs::ComponentsInitializer &init, const ecs::component_t name, const ecs::FastGetInfo &lt,
  const dafg::NodeHandle &to, const char *nameStr);
void fillSlot(dafg::NameSpaceNameId ns, const char *slot, dafg::NameSpaceNameId res_ns, const char *res_name);
using DasExecutionCallback = das::TLambda<void>;
using DasDeclarationCallBack = das::TLambda<DasExecutionCallback>;
void registerNodeDeclaration(dafg::NodeData &node_data, dafg::NameSpaceNameId ns_id, DasDeclarationCallBack declaration_callback,
  das::Context *context);
void resetNode(dafg::NodeHandle &handle);

vec4f getTypeInfo(das::Context &context, das::SimNode_CallBase *call, vec4f *args);

void setResolution(dafg::ResourceData &res, const dafg::AutoResolutionData &data);
void setTextureDescription(dafg::ResourceData &res, TextureResourceDescription &desc);
void setBufferDescription(dafg::ResourceData &res, const BufferResourceDescription &desc);
void setBlobDescription(dafg::ResourceData &res, const char *mangled_name, int size, int align, das::TypeInfo *type_info,
  das::TFunc<void, void *> ctor, das::TFunc<void, void *> dtor, das::TFunc<void, void *, const void *> copy, das::Context *ctx);
void overrideBlobCtor(dafg::ResourceData &res, das::TypeInfo *type_info, das::TLambda<void, void *> ctor, das::Context *ctx);
void setNodeSource(dafg::NodeData &node, const char *source);
bool is_daframegraph_runtime_initialized();
void *getBlobViewData(dafg::BlobView view, const char *mangled_name);
void markWithTag(dafg::ResourceRequest &request, const char *mangled_name);
void useRequestTagForBinding(const dafg::ResourceRequest &request, dafg::Binding &binding);

class DaFgCoreModule final : public das::Module
{
public:
  DaFgCoreModule();
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override;
  void addStructureAnnotations(das::ModuleLibrary &lib);
  void addNodeDataAnnotation(das::ModuleLibrary &lib);
  void addEnumerations(das::ModuleLibrary &lib);
  void addBlobBindings(das::ModuleLibrary &lib);
};

} // namespace bind_dascript

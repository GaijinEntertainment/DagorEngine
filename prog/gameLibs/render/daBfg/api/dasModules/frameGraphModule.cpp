#include <shaders/dag_shaderVar.h>
#include <dasModules/aotEcsContainer.h>
#include <dasModules/dasMacro.h>
#include <dasModules/dagorTexture3d.h>
#include <dasModules/dasShaders.h>
#include <daScript/src/builtin/module_builtin_rtti.h>
#include <api/internalRegistry.h>
#include <api/dasModules/frameGraphModule.h>
#include <api/dasModules/nodeEcsRegistration.h>
#include <api/dasModules/bindingHelper.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <backend.h>


namespace bind_dascript
{

ManagedTexView getTexView(const dabfg::ResourceProvider &provider, dabfg::ResNameId resId, bool history)
{
  auto &storage = history ? provider.providedHistoryResources : provider.providedResources;
  if (auto it = storage.find(resId); it != storage.end())
    return eastl::get<ManagedTexView>(it->second);
  else
    return {};
}

ManagedBufView getBufView(const dabfg::ResourceProvider &provider, dabfg::ResNameId resId, bool history)
{
  auto &storage = history ? provider.providedHistoryResources : provider.providedResources;
  if (auto it = storage.find(resId); it != storage.end())
    return eastl::get<ManagedBufView>(it->second);
  else
    return {};
}

const dabfg::ResourceProvider &getProvider(dabfg::InternalRegistry *registry)
{
  G_ASSERT(registry);
  return registry->resourceProviderReference;
}

dabfg::InternalRegistry &getRegistry(const dabfg::NodeTracker &tracker) { return tracker.registry; }

int getShaderVariableId(const char *name) { return VariableMap::getVariableId(name); }

void setEcsNodeHandleHint(ecs::ComponentsInitializer &init, const char *name, uint32_t hash, dabfg::NodeHandle &handle)
{
  init[ecs::HashedConstString({name, hash})] = eastl::move(handle);
}

void setEcsNodeHandle(ecs::ComponentsInitializer &init, const char *name, dabfg::NodeHandle &handle)
{
  setInitChildComponent(init, name, eastl::move(handle));
}

void fillSlot(dabfg::NameSpaceNameId ns, const char *slot, dabfg::NameSpaceNameId res_ns, const char *res_name)
{
  auto &registry = dabfg::NodeTracker::get().registry;
  const dabfg::ResNameId slotNameId = registry.knownNames.addNameId<dabfg::ResNameId>(ns, slot);
  const dabfg::ResNameId resNameId = registry.knownNames.addNameId<dabfg::ResNameId>(res_ns, res_name);
  registry.resourceSlots.set(slotNameId, dabfg::SlotData{resNameId});

  // TODO: it is a bit ugly that we need to call this here and in C++ API's fillSlot, maybe rework it somehow?
  dabfg::Backend::get().markStageDirty(dabfg::CompilationStage::REQUIRES_NAME_RESOLUTION);
}

void registerNodeDeclaration(dabfg::NodeData &node_data, dabfg::NameSpaceNameId ns_id, DasDeclarationCallBack declaration_callback,
  das::Context *context)
{
  node_data.declare = [context, declaration_callback, ns_id](dabfg::NodeNameId nodeId, dabfg::InternalRegistry *r) {
    DasRegistry registry{ns_id, nodeId, r};
    const auto invokeDeclCb = [context, declaration_callback, &registry]() {
      return das::das_invoke_lambda<DasExecutionCallback>::invoke<DasRegistry &>(context, nullptr, declaration_callback, registry);
    };
    r->nodes[nodeId].nodeSource = context->name;
    const auto execCb = callDasFunction(context, invokeDeclCb);
    return [context, execCb](dabfg::multiplexing::Index) {
      const auto invokeExecCb = [context, &execCb]() { return das::das_invoke_lambda<void>::invoke(context, nullptr, execCb); };
      callDasFunction(context, invokeExecCb);
    };
  };
}

void resetNode(dabfg::NodeHandle &handle) { handle = dabfg::NodeHandle(); }

} // namespace bind_dascript

struct ResourceProviderAnnotation final : das::ManagedStructureAnnotation<dabfg::ResourceProvider>
{
  ResourceProviderAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResourceProvider", ml, "dabfg::ResourceProvider") {}
};

struct NodeHandleAnnotation final : das::ManagedStructureAnnotation<dabfg::NodeHandle>
{
  NodeHandleAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NodeHandle", ml, "dabfg::NodeHandle") {}
  bool canMove() const override { return true; }
};

struct NodeTrackerAnnotation final : das::ManagedStructureAnnotation<dabfg::NodeTracker>
{
  NodeTrackerAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NodeTracker", ml, "dabfg::NodeTracker") {}
};

struct InternalRegistryAnnotation final : das::ManagedStructureAnnotation<dabfg::InternalRegistry>
{
  InternalRegistryAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("InternalRegistry", ml, "dabfg::InternalRegistry")
  {
    addField<DAS_BIND_MANAGED_FIELD(resources)>("resources");
    addField<DAS_BIND_MANAGED_FIELD(nodes)>("nodes");
    addField<DAS_BIND_MANAGED_FIELD(knownNames)>("knownNames");
  }
};

struct DasNameSpaceRequestAnnotation final : das::ManagedStructureAnnotation<DasNameSpaceRequest>
{
  DasNameSpaceRequestAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NameSpaceRequest", ml, "DasNameSpaceRequest")
  {
    addField<DAS_BIND_MANAGED_FIELD(nameSpaceId)>("nameSpaceId");
    addField<DAS_BIND_MANAGED_FIELD(nodeId)>("nodeId");
    addField<DAS_BIND_MANAGED_FIELD(registry)>("registry");
  }
};

struct DasRegistryAnnotation final : das::ManagedStructureAnnotation<DasRegistry>
{
  DasRegistryAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Registry", ml, "DasRegistry")
  {
    addField<DAS_BIND_MANAGED_FIELD(nameSpaceId)>("nameSpaceId");
    addField<DAS_BIND_MANAGED_FIELD(nodeId)>("nodeId");
    addField<DAS_BIND_MANAGED_FIELD(registry)>("registry");
  }
};

static char frameGraphNodes_das[] =
#include "frameGraphModule.das.inl"
  ;

namespace bind_dascript
{
DaBfgModule::DaBfgModule() : das::Module("daBfg")
{
  das::ModuleLibrary lib(this);

  addBuiltinDependency(lib, das::Module::require("DagorResPtr"), true);
  addBuiltinDependency(lib, das::Module::require("DagorDriver3D"), true);
  addBuiltinDependency(lib, das::Module::require("DagorShaders"), true);

  addEnumerations(lib);

  addStructureAnnotations(lib);
  addNodeDataAnnotation(lib);

  addAnnotation(das::make_smart<NodeEcsRegistrationAnnotation>());
  addAnnotation(das::make_smart<ResourceProviderAnnotation>(lib));
  addAnnotation(das::make_smart<InternalRegistryAnnotation>(lib));
  addAnnotation(das::make_smart<DasNameSpaceRequestAnnotation>(lib));
  addAnnotation(das::make_smart<DasRegistryAnnotation>(lib));
  das::setParents(lib.getThisModule(), "Registry", {"NameSpaceRequest"});
  addAnnotation(das::make_smart<NodeTrackerAnnotation>(lib));
  addAnnotation(das::make_smart<NodeHandleAnnotation>(lib));

  das::addUsing<TextureResourceDescription>(*this, lib, "TextureResourceDescription");
  das::addUsing<BufferResourceDescription>(*this, lib, "BufferResourceDescription");
  das::addCtorAndUsing<dabfg::NodeHandle>(*this, lib, "NodeHandle", "dabfg::NodeHandle");
  das::addUsing<dabfg::VrsStateRequirements>(*this, lib, "dabfg::VrsStateRequirements");
  das::addUsing<dabfg::Binding>(*this, lib, "dabfg::Binding");
  das::addUsing<dabfg::ResourceRequest>(*this, lib, "dabfg::ResourceRequest");

  CLASS_MEMBER_SIGNATURE(dabfg::NodeTracker::registerNode, "registerNode", das::SideEffects::modifyArgumentAndExternal,
    void(dabfg::NodeTracker::*)(dabfg::NodeNameId));
  CLASS_MEMBER_SIGNATURE(dabfg::NodeTracker::unregisterNode, "unregisterNode", das::SideEffects::modifyArgumentAndExternal,
    void(dabfg::NodeTracker::*)(dabfg::NodeNameId, uint32_t));

  BIND_FUNCTION(bind_dascript::getShaderVariableId, "get_shader_variable_id", das::SideEffects::accessExternal);
  BIND_FUNCTION(bind_dascript::getTexView, "getTexView", das::SideEffects::accessExternal)
  BIND_FUNCTION(bind_dascript::getBufView, "getBufView", das::SideEffects::accessExternal)
  BIND_FUNCTION(bind_dascript::fillSlot, "fill_slot", das::SideEffects::modifyExternal)
  BIND_FUNCTION(bind_dascript::resetNode, "resetNode", das::SideEffects::modifyArgument)

  das::addExtern<DAS_BIND_FUN(dabfg::NodeTracker::get), das::SimNode_ExtFuncCallRef>(*this, lib, "getTracker",
    das::SideEffects::accessExternal, "dabfg::NodeTracker::get");
  das::addExtern<DAS_BIND_FUN(bind_dascript::getRegistry), das::SimNode_ExtFuncCallRef>(*this, lib, "getRegistry",
    das::SideEffects::accessExternal, "bind_dascript::getRegistry");
  das::addExtern<DAS_BIND_FUN(bind_dascript::getProvider), das::SimNode_ExtFuncCallRef>(*this, lib, "getResourceProvider",
    das::SideEffects::accessExternal, "bind_dascript::getProvider");
  das::addExtern<DAS_BIND_FUN(dabfg::register_external_node), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
    "register_external_node", das::SideEffects::none, "dabfg::register_external_node");
  das::addExtern<DAS_BIND_FUN(bind_dascript::setEcsNodeHandleHint)>(*this, lib, "set", das::SideEffects::modifyArgument,
    "bind_dascript::setEcsNodeHandleHint");
  das::addExtern<DAS_BIND_FUN(bind_dascript::setEcsNodeHandle)>(*this, lib, "set", das::SideEffects::modifyArgument,
    "bind_dascript::setEcsNodeHandle")
    ->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
  das::addExtern<DAS_BIND_FUN(bind_dascript::registerNodeDeclaration)>(*this, lib, "registerNodeDeclaration",
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::registerNodeDeclaration");

  compileBuiltinModule("frameGraphModule.das", (unsigned char *)frameGraphNodes_das, sizeof(frameGraphNodes_das));
  verifyAotReady();
}

das::ModuleAotType DaBfgModule::aotRequire(das::TextWriter &tw) const
{
  tw << "#include <api/dasModules/nodeEcsRegistration.h>\n";
  tw << "#include <api/dasModules/frameGraphModule.h>\n";
  tw << "#include <render/daBfg/ecs/frameGraphNode.h>\n";
  return das::ModuleAotType::cpp;
}

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DaBfgModule, bind_dascript);

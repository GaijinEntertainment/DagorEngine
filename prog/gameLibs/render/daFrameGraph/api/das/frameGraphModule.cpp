// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphModule.h"

#include <render/daFrameGraph/ecs/frameGraphNode.h>

#include <shaders/dag_shaderVar.h>
#include <dasModules/aotEcsContainer.h>
#include <dasModules/dasMacro.h>
#include <dasModules/dagorTexture3d.h>
#include <dasModules/dasDagorResPtr.h>
#include <dasModules/dasShaders.h>
#include <daScript/src/builtin/module_builtin_rtti.h>

#include <frontend/internalRegistry.h>
#include <runtime/runtime.h>
#include <api/das/frameGraphModule.h>
#include <api/das/bindingHelper.h>
#include <render/daFrameGraph/das/registerBlobType.h>


namespace bind_dascript
{

// Das script reloading is multithreaded, so we need to protect our runtime
// Reloading of scripts happens between frames, so we shouldn't
// synchronize reloading and main threads.
das::mutex daframegraphRuntimeMutex;

bool is_daframegraph_runtime_initialized() { return dafg::Runtime::isInitialized(); }

ManagedTexView getTexView(const dafg::ResourceProvider *provider, dafg::ResNameId resId, bool history)
{
  G_ASSERT(provider);
  auto &storage = history ? provider->providedHistoryResources : provider->providedResources;
  if (auto it = storage.find(resId); it != storage.end())
    return eastl::get<ManagedTexView>(it->second);
  else
    return {};
}

ManagedBufView getBufView(const dafg::ResourceProvider *provider, dafg::ResNameId resId, bool history)
{
  G_ASSERT(provider);
  auto &storage = history ? provider->providedHistoryResources : provider->providedResources;
  if (auto it = storage.find(resId); it != storage.end())
    return eastl::get<ManagedBufView>(it->second);
  else
    return {};
}

IPoint2 getResolution2(const dafg::ResourceProvider *provider, dafg::AutoResTypeNameId resId)
{
  G_ASSERT(provider);
  return eastl::get<IPoint2>(provider->resolutions[resId]);
}

IPoint3 getResolution3(const dafg::ResourceProvider *provider, dafg::AutoResTypeNameId resId)
{
  G_ASSERT(provider);
  return eastl::get<IPoint3>(provider->resolutions[resId]);
}

const dafg::ResourceProvider *getProvider(dafg::InternalRegistry *registry)
{
  G_ASSERT(registry);
  return &registry->resourceProviderReference;
}

dafg::NodeTracker &getTracker() { return dafg::Runtime::get().getNodeTracker(); }
dafg::InternalRegistry *getRegistry() { return &dafg::Runtime::get().getInternalRegistry(); }

void registerNode(dafg::NodeTracker &node_tracker, dafg::NodeNameId nodeId, das::Context *context)
{
  node_tracker.registerNode(context, nodeId);
}

void setEcsNodeHandleHint(ecs::ComponentsInitializer &init, const char *name, uint32_t hash, dafg::NodeHandle &handle)
{
  init[ecs::HashedConstString({name, hash})] = eastl::move(handle);
}

void setEcsNodeHandle(ecs::ComponentsInitializer &init, const char *name, dafg::NodeHandle &handle)
{
  setInitChildComponent(init, name, eastl::move(handle));
}

void fillSlot(dafg::NameSpaceNameId ns, const char *slot, dafg::NameSpaceNameId res_ns, const char *res_name)
{
  auto &runtime = dafg::Runtime::get();
  auto &registry = runtime.getInternalRegistry();
  const dafg::ResNameId slotNameId = registry.knownNames.addNameId<dafg::ResNameId>(ns, slot);
  const dafg::ResNameId resNameId = registry.knownNames.addNameId<dafg::ResNameId>(res_ns, res_name);

  auto &slots = registry.resourceSlots;
  dafg::ResNameId prevResNameId = dafg::ResNameId::Invalid;
  if (slots.isMapped(slotNameId) && slots[slotNameId].has_value())
    prevResNameId = slots[slotNameId].value().contents;
  slots.set(slotNameId, dafg::SlotData{resNameId, prevResNameId});

  // TODO: it is a bit ugly that we need to call this here and in C++ API's fillSlot, maybe rework it somehow?
  runtime.markStageDirty(dafg::CompilationStage::REQUIRES_NAME_RESOLUTION);
}

void registerNodeDeclaration(dafg::NodeData &node_data, dafg::NameSpaceNameId ns_id, DasDeclarationCallBack declaration_callback,
  das::Context *context)
{
  G_UNUSED(ns_id);
  node_data.declare = [context, declCb = das::GcRootLambda(eastl::move(declaration_callback), context)] //
    (dafg::NodeNameId nodeId, dafg::InternalRegistry * r) {
      const auto invokeDeclCb = [context, &declCb]() {
        return das::das_invoke_lambda<DasExecutionCallback>::invoke<>(context, nullptr, declCb);
      };
      r->nodes[nodeId].nodeSource = context->name;
      auto executionCallback = callDasFunction(context, invokeDeclCb);
      return [context, execCb = das::GcRootLambda(eastl::move(executionCallback), context)] //
        (dafg::multiplexing::Index) {
          const auto invokeExecCb = [context, &execCb]() { return das::das_invoke_lambda<void>::invoke(context, nullptr, execCb); };
          callDasFunction(context, invokeExecCb);
        };
    };
}

void resetNode(dafg::NodeHandle &handle) { handle = dafg::NodeHandle(); }

} // namespace bind_dascript

struct ResourceProviderAnnotation final : das::ManagedStructureAnnotation<dafg::ResourceProvider>
{
  ResourceProviderAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResourceProvider", ml, "dafg::ResourceProvider") {}
};

struct NodeHandleAnnotation final : das::ManagedStructureAnnotation<dafg::NodeHandle>
{
  NodeHandleAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NodeHandle", ml, "dafg::NodeHandle")
  {
    addProperty<DAS_BIND_MANAGED_PROP(valid)>("valid", "valid");
  }
  bool isLocal() const override { return true; }
  bool canMove() const override { return true; }
  bool canCopy() const override { return false; }
};

struct NodeTrackerAnnotation final : das::ManagedStructureAnnotation<dafg::NodeTracker>
{
  NodeTrackerAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NodeTracker", ml, "dafg::NodeTracker") {}
};

struct InternalRegistryAnnotation final : das::ManagedStructureAnnotation<dafg::InternalRegistry>
{
  InternalRegistryAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("InternalRegistry", ml, "dafg::InternalRegistry")
  {
    addField<DAS_BIND_MANAGED_FIELD(resources)>("resources");
    addField<DAS_BIND_MANAGED_FIELD(nodes)>("nodes");
    addField<DAS_BIND_MANAGED_FIELD(knownNames)>("knownNames");
  }
};


namespace bind_dascript
{

DaFgCoreModule::DaFgCoreModule() : das::Module("daFgCore")
{
  das::ModuleLibrary lib(this);

  // When hot-reloading stuff, we need to wipe nodes preemptively, as
  // we will get stale das lambda references inside nodes otherwise.
  das::onDestroyCppDebugAgent(name.c_str(), [](das::Context *ctx) {
    das::lock_guard<das::mutex> lock{daframegraphRuntimeMutex};
    // Note that this is a global callback that we never unregister,
    // so it might be called after the runtime is destroyed.
    // Unregistering it in the destructor is NOT an option, as modules
    // get created several times while loading scripts and so we do a
    // double-register, and then would immediately unregister it again.
    // One could make a fancy refcount system, but it's not worth it.
    if (!dafg::Runtime::isInitialized())
      return;
    auto &rt = dafg::Runtime::get();
    auto resIdsToWipe = rt.getNodeTracker().wipeContextNodes(ctx);
    if (resIdsToWipe.has_value())
    {
      // Note that with this mechanism we wipe more than we need to: even
      // non das-native blobs will get emergency wiped, but we are OK with that.
      rt.wipeBlobsBetweenFrames(eastl::span<dafg::ResNameId>(resIdsToWipe->data(), resIdsToWipe->size()));
      debug("daFG: Wiped nodes and blobs managed by context %p", ctx);
    }
  });

  addEnumerations(lib);

  addStructureAnnotations(lib);
  addNodeDataAnnotation(lib);

  addAnnotation(das::make_smart<ResourceProviderAnnotation>(lib));
  addAnnotation(das::make_smart<InternalRegistryAnnotation>(lib));
  addAnnotation(das::make_smart<NodeTrackerAnnotation>(lib));

  addAnnotation(das::make_smart<NodeHandleAnnotation>(lib));

  das::addUsing<TextureResourceDescription>(*this, lib, "TextureResourceDescription");
  das::addUsing<BufferResourceDescription>(*this, lib, "BufferResourceDescription");
  das::addCtorAndUsing<dafg::NodeHandle>(*this, lib, "NodeHandle", "dafg::NodeHandle");
  das::addUsing<dafg::VrsStateRequirements>(*this, lib, "dafg::VrsStateRequirements");
  das::addUsing<dafg::Binding>(*this, lib, "dafg::Binding");
  das::addUsing<dafg::ResourceRequest>(*this, lib, "dafg::ResourceRequest");

  BIND_FUNCTION(bind_dascript::registerNode, "registerNode", das::SideEffects::modifyArgumentAndExternal);
  CLASS_MEMBER_SIGNATURE(dafg::NodeTracker::unregisterNode, "unregisterNode", das::SideEffects::modifyArgumentAndExternal,
    void(dafg::NodeTracker::*)(dafg::NodeNameId, uint16_t));

  BIND_FUNCTION(bind_dascript::getTexView, "getTexView", das::SideEffects::accessExternal)
  BIND_FUNCTION(bind_dascript::getBufView, "getBufView", das::SideEffects::accessExternal)
  BIND_FUNCTION(bind_dascript::getResolution2, "getResolution`2", das::SideEffects::accessExternal)
  BIND_FUNCTION(bind_dascript::getResolution3, "getResolution`3", das::SideEffects::accessExternal)
  BIND_FUNCTION(bind_dascript::fillSlot, "fill_slot", das::SideEffects::modifyExternal)
  BIND_FUNCTION(bind_dascript::resetNode, "resetNode", das::SideEffects::modifyArgument)

  das::addExtern<DAS_BIND_FUN(bind_dascript::getTracker), das::SimNode_ExtFuncCallRef>(*this, lib, "getTracker",
    das::SideEffects::accessExternal, "bind_dascript::getTracker");
  BIND_FUNCTION(bind_dascript::getRegistry, "getRegistry", das::SideEffects::accessExternal)
  BIND_FUNCTION(bind_dascript::getProvider, "getProvider", das::SideEffects::accessExternal)
  das::addExtern<DAS_BIND_FUN(dafg::register_external_node), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
    "register_external_node", das::SideEffects::none, "dafg::register_external_node");
  das::addExtern<DAS_BIND_FUN(bind_dascript::setEcsNodeHandleHint)>(*this, lib, "set", das::SideEffects::modifyArgument,
    "bind_dascript::setEcsNodeHandleHint");
  das::addExtern<DAS_BIND_FUN(bind_dascript::setEcsNodeHandle)>(*this, lib, "set", das::SideEffects::modifyArgument,
    "bind_dascript::setEcsNodeHandle")
    ->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
  das::addExtern<DAS_BIND_FUN(bind_dascript::registerNodeDeclaration)>(*this, lib, "registerNodeDeclaration",
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::registerNodeDeclaration");
  das::addExtern<DAS_BIND_FUN(bind_dascript::is_daframegraph_runtime_initialized)>(*this, lib, "is_daframegraph_runtime_initialized",
    das::SideEffects::accessExternal, "bind_dascript::is_daframegraph_runtime_initialized");

  das::typeFactory<NodeHandleVector>::make(lib);
  dafg::das::register_interop_type<d3d::SamplerHandle>(lib);

  addBlobBindings(lib);

  verifyAotReady();
}

das::ModuleAotType DaFgCoreModule::aotRequire(das::TextWriter &tw) const
{
  tw << "#include <api/das/frameGraphModule.h>\n";
  tw << "#include <render/daFrameGraph/ecs/frameGraphNode.h>\n";
  return das::ModuleAotType::cpp;
}

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DaFgCoreModule, bind_dascript);

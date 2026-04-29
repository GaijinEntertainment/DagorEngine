// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>

#include <math/integer/dag_IPoint2.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_driver.h>
#include <generic/dag_initOnDemand.h>

#include <frontend/internalRegistry.h>
#include <frontend/nameResolver.h>
#include <frontend/dependencyDataCalculator.h>
#include <frontend/nodeTracker.h>
#include <frontend/registryValidator.h>
#include <frontend/irGraphBuilder.h>

#include <backend/nodeScheduler.h>
#include <backend/resourceScheduling/resourceScheduler.h>
#include <backend/resourceScheduling/resourceAllocator.h>
#include <backend/passColoring.h>
#include <backend/intermediateRepresentation.h>
#include <backend/nodeStateDeltas.h>

#include <runtime/nodeExecutor.h>
#include <runtime/compilationStage.h>
#include <runtime/typeDb.h>

#include <debug/visualizationManagerInterface.h>


template <typename, bool, typename>
struct InitOnDemand;

namespace dafg
{

class Runtime
{
  friend struct InitOnDemand<Runtime, false>;
  static InitOnDemand<Runtime, false> instance;

public:
  // NOTE: it's good to put this here as everything will be inlined,
  // while the address of the instance in static memory will be resolved
  // by the linker and we will have 0 indirections when accessing stuff
  // inside the backend.
  static void startup()
  {
    debug("daFG: runtime starting up!");
    instance.demandInit();
  }
  static Runtime &get() { return *instance; }
  static bool isInitialized() { return static_cast<bool>(instance); }
  static void shutdown()
  {
    instance.demandDestroy();
    NodeTracker::Alloc::wipe();
    debug("daFG: runtime shut down!");
  }


  // The following functions are called from various APIs to control the library.

  NodeTracker &getNodeTracker() { return nodeTracker; }
  TypeDb &getTypeDb() { return typeDb; }
  InternalRegistry &getInternalRegistry() { return registry; }
  visualization::IVisualizationManager *getVisualizerPtr() { return fgVisManager.get(); }
  DependencyData &getDependencyData() { return dependencyDataCalculator.depData; }
  void updateExternalState(ExternalState state) { nodeExec->externalState = state; }
  void setMultiplexingExtents(multiplexing::Extents extents);
  bool runNodes();

  void markStageDirty(CompilationStage stage)
  {
    if (stage < currentStage)
      currentStage = stage;
  }

  void invalidateHistory();
  void onStaticResolutionChange(AutoResTypeNameId id)
  {
    deltaCalculator.invalidateCachesForAutoResType(id);
    Runtime::get().markStageDirty(CompilationStage::REQUIRES_NODE_DECLARATION_UPDATE);
  }

  void beforeDeviceReset();

  // Should be called for emergency wiping of blobs when some outer
  // system is destroyed and we might have dangling references.
  void wipeBlobsBetweenFrames(eastl::span<ResNameId> resources);

private:
  CompilationStage currentStage = CompilationStage::UP_TO_DATE;
  multiplexing::Extents currentMultiplexingExtents;
  multiplexing::Extents prevMultiplexingExtents;
  multiplexing::Extents historyMultiplexingExtents;

  // === Components of FG backend ===

  // This provider is used by resource handles acquired from
  // resource requests. Should contain all relevant resources when a
  // node gets executed.
  ResourceProvider currentlyProvidedResources;

  // This registry represents the entire user-specified graph with
  // simple encapsulation-less data (at least in theory)
  InternalRegistry registry{currentlyProvidedResources};
  NameResolver nameResolver{registry};

  // Not part of the user graph per se, just immutable partial info about
  // types the user has ever used
  TypeDb typeDb;

  DependencyDataCalculator dependencyDataCalculator{registry, nameResolver};

  NodeTracker nodeTracker{registry};

  RegistryValidator registryValidator{registry, dependencyDataCalculator.depData, nameResolver};

  IrGraphBuilder irGraphBuilder{registry, dependencyDataCalculator.depData, registryValidator.validityInfo, nameResolver};

  PassColorer passColorer;

  NodeScheduler cullingScheduler;

  BarrierScheduler barrierScheduler; //-V730_NOINIT

  BadResolutionTracker badResolutionTracker{intermediateGraph};

  eastl::unique_ptr<ResourceAllocator> resourceAllocator;
  ResourceScheduler resourceScheduler;

  // Populated during resource scheduling, consumed during history update.
  PotentialDeactivationSet pendingDeactivations;

  // ===


  intermediate::Graph unsortedIntermediateGraph;
  intermediate::Graph intermediateGraph;
  intermediate::Mapping irMapping;
  IdIndexedMapping<intermediate::NodeIndex, intermediate::NodeIndex> prevPermutation;

  PassColoring passColoring;
  sd::NodeStateDeltas perNodeStateDeltas;
  BarrierScheduler::EventsCollection allResourceEvents;

  sd::DeltaCalculator deltaCalculator{intermediateGraph};

  // Deferred init
  eastl::optional<NodeExecutor> nodeExec;

  uint32_t frameIndex = 0;

  eastl::unique_ptr<visualization::IVisualizationManager> fgVisManager;

private:
  Runtime();
  ~Runtime();

  void recompile();
  void debugDanglingReferences();
  void testIncrementality();

  void updateDynamicResolution(int curr_frame);

  using NodesChanged = IdIndexedFlags<NodeNameId, framemem_allocator>;
  using ResourcesChanged = IdIndexedFlags<ResNameId, framemem_allocator>;
  using IrNodesChanged = IdIndexedFlags<intermediate::NodeIndex, framemem_allocator>;
  using IrResourcesChanged = IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>;

  auto updateNodeDeclarations();
  auto resolveNames(const NodesChanged &nodes_changed);
  auto calculateDependencyData(const NodesChanged &nodes_changed);
  void validateRegistry(NodesChanged &nodeChanges, ResourcesChanged &resourceChanges);
  auto buildIrGraph(const ResourcesChanged &resources_changed, const NodesChanged &nodes_changed);
  void colorPasses(const IrNodesChanged &irNodesChanged);
  IrNodesChanged scheduleNodes(const IrNodesChanged &irNodesChanged, const IrResourcesChanged &irResourcesChanged);
  IrResourcesChanged scheduleBarriers(const IrNodesChanged &nodesChanged, const IrResourcesChanged &resourcesChanged);
  void recalculateStateDeltas(const IrNodesChanged &nodesChanged, const IrResourcesChanged &resourcesChanged);
  void updateAutoResolutions();
  void scheduleResources(const IrResourcesChanged &lifetimeChangedResources);
  void updateHistory();
  void updateVisualization();
};
} // namespace dafg

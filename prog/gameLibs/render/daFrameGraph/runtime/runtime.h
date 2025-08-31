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
#include <frontend/recompilationData.h>

#include <backend/nodeScheduler.h>
#include <backend/resourceScheduling/resourceScheduler.h>
#include <backend/passColoring.h>
#include <backend/intermediateRepresentation.h>
#include <backend/nodeStateDeltas.h>

#include <runtime/nodeExecutor.h>
#include <runtime/compilationStage.h>
#include <runtime/typeDb.h>

#include <debug/dafgVisualizerInterface.h>


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
    sd::Alloc::wipe();
    debug("daFG: runtime shut down!");
  }


  // The following functions are called from various APIs to control the library.

  NodeTracker &getNodeTracker() { return nodeTracker; }
  TypeDb &getTypeDb() { return typeDb; }
  InternalRegistry &getInternalRegistry() { return registry; }
  visualization::IVisualizer *getVisualizerPtr() { return fgVisualizer.get(); }
  void updateExternalState(ExternalState state) { nodeExec->externalState = state; }
  void setMultiplexingExtents(multiplexing::Extents extents);
  bool runNodes();

  void markStageDirty(CompilationStage stage)
  {
    if (stage < currentStage)
      currentStage = stage;
  }

  void invalidateHistory();
  void onStaticResolutionChange()
  {
    deltaCalculator.invalidateCaches();
    Runtime::get().markStageDirty(CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION);
  }

  void beforeDeviceReset();

  // Should be called for emergency wiping of blobs when some outer
  // system is destroyed and we might have dangling references.
  void wipeBlobsBetweenFrames(eastl::span<ResNameId> resources);

private:
  CompilationStage currentStage = CompilationStage::UP_TO_DATE;
  multiplexing::Extents currentMultiplexingExtents;
  multiplexing::Extents historyMultiplexingExtents;

  // === Components of FG backend ===

  // This provider is used by resource handles acquired from
  // resource requests. Should contain all relevant resources when a
  // node gets executed.
  ResourceProvider currentlyProvidedResources;

  // This registry represents the entire user-specified graph with
  // simple encapsulation-less data (at least in theory)
  InternalRegistry registry{currentlyProvidedResources};
  NameResolver nameResolver{registry, frontendRecompilationData};

  // Not part of the user graph per se, just immutable partial info about
  // types the user has ever used
  TypeDb typeDb;

  DependencyDataCalculator dependencyDataCalculator{registry, nameResolver, frontendRecompilationData};

  FrontendRecompilationData frontendRecompilationData;
  NodeTracker nodeTracker{registry, dependencyDataCalculator.depData, frontendRecompilationData};

  RegistryValidator registryValidator{registry, dependencyDataCalculator.depData, nameResolver};

  IrGraphBuilder irGraphBuilder{registry, dependencyDataCalculator.depData, registryValidator.validityInfo, nameResolver};

  NodeScheduler cullingScheduler;

  BarrierScheduler barrierScheduler; //-V730_NOINIT

  BadResolutionTracker badResolutionTracker{intermediateGraph};

  eastl::unique_ptr<ResourceScheduler> resourceScheduler;

  // ===


  intermediate::Graph intermediateGraph;
  intermediate::Mapping irMapping;

  PassColoring passColoring;
  sd::NodeStateDeltas perNodeStateDeltas;
  BarrierScheduler::EventsCollectionRef allResourceEvents;

  sd::DeltaCalculator deltaCalculator{intermediateGraph};

  // Deferred init
  eastl::optional<NodeExecutor> nodeExec;

  uint32_t frameIndex = 0;

  eastl::unique_ptr<visualization::IVisualizer> fgVisualizer;

private:
  Runtime();
  ~Runtime();

  void updateDynamicResolution(int curr_frame);

  void updateNodeDeclarations();
  void resolveNames();
  void calculateDependencyData();
  void validateRegistry();
  void buildIrGraph();
  void colorPasses();
  void scheduleNodes();
  void scheduleBarriers();
  void recalculateStateDeltas();
  void scheduleResources();
  void initializeHistoryOfNewResources();
  void updateVisualization();
};
} // namespace dafg

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
#include <backend/passColoring.h>
#include <backend/intermediateRepresentation.h>

#include <runtime/nodeExecutor.h>
#include <runtime/compilationStage.h>


template <typename, bool, typename>
struct InitOnDemand;

namespace dabfg
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
  static void startup() { instance.demandInit(); }
  static Runtime &get() { return *instance; }
  static bool isInitialized() { return static_cast<bool>(instance); }
  static void shutdown() { instance.demandDestroy(); }


  // The following functions are called from various APIs to control the library.

  NodeTracker &getNodeTracker() { return nodeTracker; }
  InternalRegistry &getInternalRegistry() { return registry; }
  void updateExternalState(ExternalState state) { nodeExec->externalState = state; }
  void setMultiplexingExtents(multiplexing::Extents extents);
  bool runNodes();

  void markStageDirty(CompilationStage stage)
  {
    if (stage < currentStage)
      currentStage = stage;
  }

  void invalidateHistory();

  void requestCompleteResourceRescheduling();

  // Should be called for emergency wiping of blobs when some outer
  // system is destroyed and we might have dangling references.
  void wipeBlobsBetweenFrames(eastl::span<ResNameId> resources);

  // TODO: remove
  void dumpGraph(const eastl::string &filename) const;

private:
  CompilationStage currentStage = CompilationStage::UP_TO_DATE;
  multiplexing::Extents currentMultiplexingExtents;
  multiplexing::Extents multiplexingExtentsOnPreviousCompilation;

  // === Components of FG backend ===

  // This provider is used by resource handles acquired from
  // resource requests. Should contain all relevant resources when a
  // node gets executed.
  ResourceProvider currentlyProvidedResources;

  // This registry represents the entire user-specified graph with
  // simple encapsulation-less data (at least in theory)
  InternalRegistry registry{currentlyProvidedResources};
  NameResolver nameResolver{registry};


  DependencyDataCalculator dependencyDataCalculator{registry, nameResolver};

  NodeTracker nodeTracker{registry, dependencyDataCalculator.depData};

  RegistryValidator registryValidator{registry, dependencyDataCalculator.depData, nameResolver};

  IrGraphBuilder irGraphBuilder{registry, dependencyDataCalculator.depData, registryValidator.validityInfo, nameResolver};

  NodeScheduler cullingScheduler;

  BarrierScheduler barrierScheduler;

  eastl::unique_ptr<ResourceScheduler> resourceScheduler;

  // ===


  intermediate::Graph intermediateGraph;
  intermediate::Mapping irMapping;

  PassColoring passColoring;
  sd::NodeStateDeltas perNodeStateDeltas;
  BarrierScheduler::EventsCollectionRef allResourceEvents;

  // Deferred init
  eastl::optional<NodeExecutor> nodeExec;

  uint32_t frameIndex = 0;

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
};
} // namespace dabfg

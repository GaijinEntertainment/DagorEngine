#pragma once

#include <EASTL/array.h>

#include <math/integer/dag_IPoint2.h>
#include <dag/dag_vector.h>
#include <3d/dag_drv3d.h>
#include <generic/dag_initOnDemand.h>

#include <nodeScheduling/nodeScheduler.h>
#include <resourceScheduling/resourceScheduler.h>
#include <nodes/nodeTracker.h>
#include <nodes/nodeExecutor.h>
#include <intermediateRepresentation.h>
#include <nameResolver.h>

#include <compilationStage.h>


template <typename, bool>
struct InitOnDemand;

namespace dabfg
{

class Backend
{
  friend struct InitOnDemand<Backend, false>;

  static InitOnDemand<Backend, false> instance;

  friend class NodeTracker;

  CompilationStage currentStage = CompilationStage::UP_TO_DATE;
  multiplexing::Extents currentMultiplexingExtents;

  // === Components of FG backend ===

  // This provider is used by resource handles acquired from
  // resource requests. Should contain all relevant resources when a
  // node gets executed.
  ResourceProvider currentlyProvidedResources;

  // This registry represents the entire user-specified graph with
  // simple encapsulation-less data (at least in theory)
  InternalRegistry registry{currentlyProvidedResources};
  NameResolver nameResolver{registry};

  // Legacy, to be removed

  NodeTracker nodeTracker{registry, nameResolver};

  NodeScheduler cullingScheduler{nodeTracker};

  eastl::unique_ptr<ResourceScheduler> resourceScheduler;

  // ===


  intermediate::Graph intermediateGraph;
  intermediate::Mapping irMapping;
  dag::Vector<NodeStateDelta> perNodeStateDeltas;
  ResourceScheduler::EventsCollectionRef allResourceEvents;

  // Deferred init
  eastl::optional<NodeExecutor> nodeExec;

  uint32_t frameIndex = 0;

private:
  Backend();
  ~Backend();

  void declareNodes();
  void resolveNames();
  void gatherNodeData();
  void gatherGraphIR();
  void scheduleNodes();
  void scheduleResources();
  void handleFirstFrameHistory();

public:
  void markStageDirty(CompilationStage stage)
  {
    if (stage < currentStage)
      currentStage = stage;
  }

  // NOTE: it's good to put this here as everything will be inlined,
  // while the address of the instance in static memory will be resolved
  // by the linker and we will have 0 indirections when accessing stuff
  // inside the backend.
  static void startup() { instance.demandInit(); }
  static Backend &get() { return *instance; }
  static bool isInitialized() { return static_cast<bool>(instance); }
  static void shutdown() { instance.demandDestroy(); }

  void updateExternalState(ExternalState state) { nodeExec->externalState = state; }

  void setMultiplexingExtents(multiplexing::Extents extents);
  void runNodes();
  void dumpGraph(const eastl::string &filename) const;

  void requestCompleteResourceRescheduling();
};
} // namespace dabfg

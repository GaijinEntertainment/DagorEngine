---
name: daFG Library Complete Reference
description: Deep technical reference for the daFG (Dagor Frame Graph) library -- compilation pipeline, resource lifecycle, rename semantics, scheduling, barrier placement, memory aliasing, multiplexing, incremental recompilation, and all internal data structures
type: reference
commit_reference: 542abf2623ae 2026-04-28
---
# daFG Library -- Complete Technical Reference

Source: `prog/gameLibs/render/daFrameGraph/`
Public headers: `prog/gameLibs/publicInclude/render/daFrameGraph/`
Namespace: `dafg` (all public API), `dafg::intermediate` (IR), `dafg::sd` (state deltas)

## 1. Architecture Overview

```
User Code -> API (cpp/das/ecs) -> Frontend -> Backend -> Runtime
                                   |            |          |
                              NodeTracker   NodeScheduler  NodeExecutor
                              NameResolver  PassColorer    barrier events
                              DepDataCalc   BarrierSched   state deltas
                              RegValidator  ResourceSched  resource provider
                              IrGraphBuild  StateDeltas
```

### Compilation Pipeline (in order, each stage tracked by `CompilationStage` enum)

1. **REQUIRES_NODE_DECLARATION_UPDATE** -- `NodeTracker::updateNodeDeclarations()`: calls user declaration callbacks, collects `NodesChanged`/`ResourcesChanged` bitflags
2. **REQUIRES_NAME_RESOLUTION** -- `NameResolver::update()`: resolves hierarchical names, slots, renames; produces `NameResolutionChanged`
3. **REQUIRES_DEPENDENCY_DATA_CALCULATION** -- `DependencyDataCalculator::recalculate()`: builds `VirtualResourceLifetime` per resource (introducedBy, modificationChain, readers, consumedBy, historyReaders)
4. **REQUIRES_REGISTRY_VALIDATION** -- `RegistryValidator::validateRegistry()`: marks invalid nodes/resources
5. **REQUIRES_IR_GRAPH_BUILD** -- `IrGraphBuilder::build()`: builds `intermediate::Graph` with multiplexing expansion
6. **REQUIRES_PASS_COLORING** -- `PassColorer::performColoring()`: colors nodes into potential render passes using framebuffer compatibility
7. **REQUIRES_NODE_SCHEDULING** -- `NodeScheduler::schedule()`: topological sort respecting dependencies + priorities; outputs permutation
8. **REQUIRES_BARRIER_SCHEDULING** -- `BarrierScheduler::scheduleEvents()`: places activation/deactivation/barrier events per node per frame
9. **REQUIRES_STATE_DELTA_RECALCULATION** -- `DeltaCalculator::calculatePerNodeStateDeltas()`: computes minimal state changes between consecutive nodes (render passes, shader blocks, bindings, overrides)
10. **REQUIRES_AUTO_RESOLUTION_UPDATE** -- updates IR resource descriptions from auto-resolution types
11. **REQUIRES_RESOURCE_SCHEDULING** -- `ResourceScheduler::computeSchedule()`: packs resources into heaps with memory aliasing
12. **REQUIRES_HISTORY_UPDATE** -- activates history resources, copies from previous frame if preserved
13. **REQUIRES_VISUALIZATION_UPDATE** -- debug visualization
14. **UP_TO_DATE** -- no work needed

**Incremental recompilation**: `markStageDirty(stage)` sets `currentStage = min(currentStage, stage)`. The `recompile()` method uses a `switch` with `[[fallthrough]]` to run only the dirty stages and everything after them.

## 2. Key Data Structures

### InternalRegistry (`frontend/internalRegistry.h`)
The single source of truth for the user-specified graph:
- `IdIndexedMapping<ResNameId, ResourceData> resources` -- per-resource data (history flag, creation info)
- `IdIndexedMapping<NodeNameId, NodeData> nodes` -- per-node data (execute callback, resource requests, bindings, render pass, etc.)
- `IdIndexedMapping<AutoResTypeNameId, AutoResTypeData> autoResTypes` -- resolution types
- `IdIndexedMapping<ResNameId, optional<SlotData>> resourceSlots` -- named slots
- `IdHierarchicalNameMap<NameSpaceNameId, ResNameId, NodeNameId, AutoResTypeNameId> knownNames` -- hierarchical name->ID mapping
- `IdNameMap<ShaderNameId> knownShaders` -- shader name registry
- `dag::FixedVectorSet<ResNameId, 8> sinkExternalResources` -- never-pruned resources
- `multiplexing::Mode defaultMultiplexingMode` / `defaultHistoryMultiplexingMode`

### NodeData (inside InternalRegistry)
```
NodeData {
  ExecutionCallback execute;          // void(multiplexing::Index)
  DeclarationCallback declare;        // ExecutionCallback(NodeNameId, InternalRegistry*)
  priority_t priority;
  optional<multiplexing::Mode> multiplexingMode;
  SideEffects sideEffect;             // None/Internal/External
  uint16_t generation;                // incremented on re-register

  FixedVectorSet<NodeNameId> precedingNodeIds, followingNodeIds;  // explicit ordering
  FixedVectorSet<ResNameId> createdResources, modifiedResources, readResources;
  FixedVectorMap<ResNameId, ResNameId> renamedResources;  // new -> old
  FixedVectorMap<ResNameId, ResourceRequest> resourceRequests;
  FixedVectorMap<ResNameId, ResourceRequest> historyResourceReadRequests;

  optional<VirtualPassRequirements> renderingRequirements;
  variant<monostate, DispatchRequirements, DrawRequirements> executeRequirements;
  BindingsMap bindings;               // shader var ID -> Binding
  ShaderBlockLayersInfo shaderBlockLayers;
  optional<NodeStateRequirements> stateRequirements;
}
```

### intermediate::Graph (`backend/intermediateRepresentation.h`)
Post-multiplexing IR representation:
- `IdSparseIndexedMapping<NodeIndex, Node> nodes`
- `IdSparseIndexedMapping<ResourceIndex, Resource> resources`
- `IdSparseIndexedMapping<NodeIndex, RequiredNodeState> nodeStates`
- `IdSparseIndexedMapping<NodeIndex, DebugNodeName> nodeNames`
- `IdSparseIndexedMapping<ResourceIndex, DebugResourceName> resourceNames`

`intermediate::Node` contains: `priority`, `resourceRequests` (list of `Request{resource, usage, fromLastFrame}`), `supressedRequests` (resources technically requested but suppressed for `SideEffects::None` nodes), `predecessors`, `frontendNode`, `multiplexingIndex`, `hasSideEffects`.

`intermediate::Resource` contains: `ConcreteResource` (variant of `monostate|ScheduledResource|ExternalResource|DriverDeferredTexture`), `frontendResources` (rename chain of ResNameIds), `multiplexingIndex`.

### Mapping (`intermediate::Mapping`)
Maps frontend IDs to IR indices considering multiplexing:
- `mapRes(ResNameId, MultiplexingIndex) -> ResourceIndex`
- `mapNode(NodeNameId, MultiplexingIndex) -> NodeIndex`

## 3. Resource Lifecycle

### Access Types
- **create(name)** -- introduces a new FG-managed resource. Returns `VirtualResourceCreationSemiRequest`.
- **registerExternal(name)** -- introduces an externally-provided resource (via callback). No history support.
- **read(name)** -- read-only access. Happens AFTER all modifications, BEFORE renaming.
- **modify(name)** -- read-write access. Happens AFTER creation, BEFORE all reads.
- **rename(from, to)** -- consumes `from`, creates `to` as new virtual resource. Happens LAST. The SOURCE (`from`) cannot have history enabled (it's consumed on the current frame). The DESTINATION (`to`) CAN have history (API includes `CanSpecifyHistory`). `to` is created in the current node's namespace; `from` is looked up in the request namespace.
- **historyFor(name)** -- reads previous frame's version of this resource.

### Ordering Invariant
For a single resource within one frame:
**create -> modify (chain) -> read (parallel) -> rename (consumes)**

### VirtualResourceLifetime (`frontend/dependencyData.h`)
```
introducedBy    -- node that creates/renames-into this resource
modificationChain -- ordered chain of modify nodes
readers         -- nodes that read (unordered among themselves)
consumedBy      -- node that renames this resource (mutually exclusive with historyReaders)
historyReaders  -- nodes reading previous frame's version
```

### Resource Types
- **Texture** (2D, 3D, array) -- `Texture2dCreateInfo`, `Texture3dCreateInfo`
- **Buffer** -- `BufferCreateInfo`, or convenience: `byteAddressBuffer()`, `structuredBuffer<T>()`, `indirectBuffer()`
- **Blob** -- arbitrary C++ type, stored in CPU memory. Supports `bindToShaderVar()` for auto-binding.
- **External** -- provided by user callback per multiplexing index
- **DriverDeferredTexture** -- backbuffer (acquired on driver thread)

### History
`History` enum: `No`, `ClearZeroOnFirstFrame`, `DiscardOnFirstFrame`.
- History resources persist across frames via the `SCHEDULE_FRAME_WINDOW = 2` double-buffering scheme.
- On recompilation, history resources are re-activated and optionally copied from the previous frame's version.
- The SOURCE of a rename cannot have history (it's consumed). The DESTINATION of a rename CAN have history.
- Blobs with history are carefully tracked for GC purposes (`wipeBlobsBetweenFrames`).

## 4. Namespaces

`IdHierarchicalNameMap<NameSpaceNameId, ResNameId, NodeNameId, AutoResTypeNameId>` -- tree structure where:
- `NameSpaceNameId` = folder nodes
- `ResNameId`, `NodeNameId`, `AutoResTypeNameId` = leaf nodes

Names are `/`-separated full paths (e.g., `/opaque/depth`). The `NameSpace` class uses `operator/` for child access: `dafg::root() / "opaque"`.

### NameResolver
- Resolves names by walking up parent namespaces using `IdNameResolver` (checks each ancestor folder for a matching short name, only considers valid candidates).
- Resolves **slots**: `NamedSlot` contents are substituted during name mapping (`resourceSlots[id].contents`).
- Processes **renames**: `renamedResources` entries from node data are incorporated into the resolution mapping.
- Tracks inverse mappings `(node, resolved_res) -> [unresolved_res_ids]` for incremental change detection.
- Note: rename CHAINS (multi-hop A->B->C) are tracked separately in `DependencyData::renamingRepresentatives` and `renamingChains`.

## 5. Multiplexing

4 independent axes:
- `superSamples` -- high-res screenshots/cinematics
- `subSamples` -- SSAA without extra VRAM
- `viewports` -- VR/AR eyes
- `subCameras` -- camera-in-camera

`multiplexing::Mode` bitflags: `None(0)`, `SuperSampling(1)`, `SubSampling(2)`, `Viewport(4)`, `CameraInCamera(8)`, `FullMultiplex(15)`.

Each node's mode determines how many times it runs per `run_nodes()` call. `None` = once globally, `FullMultiplex` = once per combination.

**IR expansion**: `IrGraphBuilder` creates separate `intermediate::Node` and `intermediate::Resource` entries for each multiplexing iteration. The `MultiplexingIndex` is a linearized index across all active axes.

**History multiplexing**: `historyMultiplexingExtents` controls which iterations from the previous frame are available. `clamp_and_wrap()` handles up-multiplexing (wraps sub/super samples, clamps viewports).

## 6. Scheduling

### Node Scheduling (`NodeScheduler`)
Reverse Kahn's algorithm topological sort of `intermediate::Graph` nodes (schedules from last-to-execute to first-to-execute). Frontier tie-breaking heuristics (via `max_element` comparator):
1. Multiplexing index -- lower index executes earlier
2. Pass contiguity -- prefer same pass color as last scheduled node
3. Pass cross-edge in-degree -- used to keep passes contiguous
4. Priority (`setPriority()`) -- higher priority executes later (PRIO_AS_LATE_AS_POSSIBLE = max)
5. Node index -- stability tiebreaker

Note: resource dependencies and explicit ordering (`orderMeBefore`/`orderMeAfter`) are already encoded as `predecessors` edges in the IR graph by `IrGraphBuilder`.

`CONSOLE_BOOL_VAL("dafg", randomize_order, false)` -- in `NodeTracker`, shuffles the node DECLARATION order to detect missing dependencies (if something works by accident of declaration order, this breaks it).

### Pass Coloring (`PassColorer`)
Groups nodes into potential render passes based on framebuffer compatibility:
- Nodes sharing the same color can share a physical render pass
- Uses `FbLabel` (framebuffer fingerprint) and `SubpassColor` to detect compatible nodes
- `requiresBarrierBetween()` checks if a GPU barrier is needed between two nodes (which would break a pass)

### Barrier Scheduling (`BarrierScheduler`)
Places GPU barrier events at node boundaries:
- `Activation` -- first use of a resource on a frame (with clear/discard)
- `Barrier` -- state transition between usages
- `Deactivation` -- last use, releases physical memory
- `CpuActivation`/`CpuDeactivation` -- blob construction/destruction

Events are stored per-frame in `EventsCollection = array<FrameEvents, 2>` (double-buffered).

**Grace points**: pass boundaries where activations/deactivations can be batched for efficiency.

### Resource Scheduling (`ResourceScheduler`)
Assigns physical memory locations to virtual resources:
1. Gathers `ResourceAllocationProperties` from the driver
2. Groups resources into heaps by `ResourceHeapGroup`
3. Runs a **packer** algorithm to solve the dynamic storage allocation problem:
   - `GreedyScanlinePacker` (default) -- greedy scanline sweep
   - `BaselinePacker` -- no aliasing, one resource per slot
   - `BoxingPacker` -- experimental
4. Outputs `AllocationLocations` = per-resource `{HeapIndex, offset}`

**Memory aliasing**: resources with non-overlapping lifetimes can share the same physical memory within a heap. The packer produces offsets that avoid temporal conflicts.

**History preservation**: `isResourcePreserved()` checks if a resource from the previous frame can be reused without reactivation.

**Dynamic resolution**: `BadResolutionTracker` detects when dynamic resolution would exceed static allocation, triggers rescheduling. `resizeAutoResTextures()` adjusts in-place on platforms with resource heaps.

## 7. State Deltas (`sd` namespace)

`DeltaCalculator` computes minimal state changes between consecutive nodes:
- `NodeStateDelta` contains optional changes for: `asyncPipelines`, `wire`, `vrs`, `pass`, `shaderOverrides`, `vertexSources`, `indexSource`, `bindings`, `shaderBlockLayers`
- Render pass changes are `variant<PassChange, NextSubpass>`:
  - `PassChange` = end old pass + begin new pass (or just one)
  - `NextSubpass` = advance within same physical render pass
- Render passes (`d3d::RenderPass`) are cached in `rpCache` keyed by `RPsDescKey` (targets + binds)
- Cache eviction is per-auto-resolution-type via `invalidateCachesForAutoResType()`

## 8. Execution (`NodeExecutor`)

`execute(prev_frame, curr_frame, extents, events, state_deltas)`:
1. Gathers external resources via provider callbacks
2. For each node in topological order:
   a. Process barrier events (activations, barriers, deactivations)
   b. Apply state delta (render pass begin/end, shader blocks, overrides, bindings)
   c. Populate `ResourceProvider` with current node's resources
   d. Call user's execution callback with `multiplexing::Index`

**ResourceProvider** (`frontend/resourceProvider.h`):
- `providedResources: VectorMap<ResNameId, ProvidedResource>` -- textures, buffers, or blob views
- `providedHistoryResources` -- previous frame versions
- `resolutions: IdIndexedMapping<AutoResTypeNameId, ProvidedResolution>` -- dynamic resolutions

**VirtualResourceHandle** resolves to physical resources through the provider at execution time. Handles are invalidated between execution callbacks.

## 9. Bindings

`BindingType`: `ShaderVar`, `ViewMatrix`, `ProjMatrix`

Bindings connect resources to shader variables or matrices:
- `bindToShaderVar(name)` -- binds texture/buffer/blob to shader variable
- `bindAsView()`/`bindAsProj()` -- binds blob as view/projection matrix
- `bindAsVertexBuffer(stream, stride)`/`bindAsIndexBuffer()` -- buffer input assembly

**Projectors**: `TypeErasedProjector` allows binding sub-fields of blobs (e.g., `bindToShaderVar<&MyBlob::field>("var")`).

**Reset after use**: `reset_shadervar_after_use(name)` zeros a shader variable after a node completes.

## 10. Render Pass Requests (VirtualPassRequest)

`requestRenderPass()` returns a builder for specifying:
- `.color({...})` -- MRT color attachments (up to 8)
- `.depthRw(attachment)` -- depth with write
- `.depthRo(attachment)` -- depth read-only (deprecated)
- `.depthRoAndBindToShaderVars(attachment, {shader_vars})` -- simultaneous depth test + sampling
- `.resolve(from, to)` -- MSAA resolve
- `.vrsRate(attachment)` -- variable rate shading texture

Attachments can be specified by name (auto-requests modify + appropriate usage) or by explicit `VirtualResourceRequest`.

## 11. Draw/Dispatch Requests

Nodes can declare built-in draw/dispatch operations that replace the execution callback:
- `draw(shader, primitive)` / `drawIndexed(shader, primitive)` -- creates `ShaderMaterial`+`ShaderElement`, calls `d3d::draw_instanced()`
- `dispatchThreads(shader)` / `dispatchGroups(shader)` -- creates `ComputeShader`, calls `shader.dispatchThreads()`/`dispatchGroups()`
- `dispatchIndirect(shader, buffer)` -- indirect compute dispatch
- `dispatchMesh*` variants -- mesh shader dispatch via `MeshShader`
- `postFx(shader)` -- convenience for fullscreen triangle (1 prim, 1 instance)

Draw/dispatch parameters (primitive count, instance count, x/y/z, etc.) can be constant `uint32_t` or `BlobArg` (read from a blob at execution time). Dispatch-only parameters additionally support `DispatchAutoResolutionArg` (from auto-resolution) and `DispatchTextureResolutionArg` (from texture dimensions).

When `executeRequirements` is set (draw/dispatch), the user-provided execution callback is REPLACED by the built-in one. `sideEffect` is forced to `Internal`. If the user also provided a custom execution callback (sideEffect != None), a logerr is emitted.

## 12. Auto-Resolution System

`AutoResTypeData` stores both static and dynamic resolution values:
- `staticResolution` -- maximum allocation size, set via `setResolution()`
- `dynamicResolution` -- current render size, set via `setDynamicResolution()`
- `dynamicResolutionCountdown` -- frames remaining to fully propagate change

`AutoResolutionRequest<D>` (D=2 or 3) with a multiplier creates textures sized as `staticResolution * multiplier`. At execution time, `get()` returns `dynamicResolution * multiplier`.

**Important**: Changing static resolution triggers a full resource rescheduling (memory reallocation). Dynamic resolution changes only resize existing textures in-place (requires heap support).

## 13. Named Slots

`NamedSlot` creates an indirection layer for resource lookup:
```cpp
dafg::fill_slot(NamedSlot("fog_depth"), "downsampled_depth");
// Later, any node can:
registry.read(NamedSlot("fog_depth")).texture()...
```
Slots decouple producers from consumers -- the slot fill can change based on settings without modifying node declarations.

## 14. Side Effects

`SideEffects::None` -- empty callback, node can be skipped entirely
`SideEffects::Internal` -- only touches daFG state, can be pruned if output unused
`SideEffects::External` -- affects state outside daFG, never pruned

Pruning: nodes with `Internal` side effects whose outputs are not consumed by any other node or external sink get removed during IR graph building.

## 15. External Resources & Sinks

- `registerExternal(name)` -- resource provided by callback, no history
- `registerTexture(name, callback)` / `registerBuffer(name, callback)` -- convenience wrappers
- `registerBackBuffer(name)` -- special: actual texture is driver-deferred
- `markResourceExternallyConsumed(name)` -- prevents pruning (marks as sink)
- `updateExternallyConsumedResourceSet(names)` -- bulk update of sink set
- `mark_external_resource_for_validation(resource)` -- enables illegal-access validation for external resources

## 16. Memory Management Details

**Double-buffered frame window**: `SCHEDULE_FRAME_WINDOW = 2`. Resources alternate between even/odd frame slots. History resources span both frames.

**Heap types**:
- GPU heaps: `ResourceHeapGroup` from the driver, sized by packer output
- CPU heaps: `CPU_HEAP_GROUP` sentinel, stored as `Vector<char>` byte arrays

**Resource allocators**:
- `NativeResourceAllocator` -- for APIs with resource heaps (DX12, Vulkan). Uses native gAPI heaps (`ResourceHeap *`) with an LRU cache of D3D resources per heap. Supports dynamic resolution resizing.
- `PoolResourceAllocator` -- for DX11. Simulates heaps via resource pools (`resourcePool/resourcePool.h`). Does NOT support dynamic resolution.

**Packer interface**: `PackerInput{resources[], timelineSize, maxHeapSize}` -> `PackerOutput{offsets[], heapSize}`. Resources can wrap around (start >= end). Pinning forces a resource to a specific offset.

## 17. Debug & Validation

- `CONSOLE_BOOL_VAL("dafg", recompile_graph, false)` -- force full recompilation
- `CONSOLE_BOOL_VAL("dafg", recompile_graph_every_frame, false)` -- stress test
- `CONSOLE_BOOL_VAL("dafg", randomize_order, false)` -- shuffle to find missing deps
- `CONSOLE_INT_VAL("dafg", test_incrementality_nodes, 0)` -- randomly remove N nodes to test incremental recompilation
- `CONSOLE_BOOL_VAL("dafg", pedantic, false)` -- extra validation
- `dafg::show_dependencies_window()` -- visualization
- `globalStatesValidation.cpp` -- validates illegal resource access through global state
- `resourceValidation.cpp` -- validates resource access patterns

## 18. ECS Integration (`api/ecs/frameGraphNodeES.cpp.inl`)

Registers `dafg::NodeHandle` and `dag::Vector<dafg::NodeHandle>` as ECS-compatible relocatable types. This allows ECS entities to hold FG node handles as components. Node lifetime is managed by `NodeHandle`'s RAII destructor (move-only, unregisters on destruction), not by any ES handler.

## 19. DaScript Integration (`api/das/`)

`frameGraphModule.cpp` -- registers the `daFgCore` DaScript module (`DaFgCoreModule` class) with bindings for all API types (enums, structs, resource views, blob access). Also provides `registerNodeDeclaration()` for DaScript-based FG nodes. Generic container bindings in `genericBindings/` expose `IdHierarchicalNameMap`, `IdIndexedMapping`, etc. to script.

## 20. Thread Safety

- `NodeTracker` has `lock()`/`unlock()` (used as mutex via `std::lock_guard<NodeTracker>`). Locked during `runNodes()` to prevent structural changes during execution.
- `checkChangesLock()` -- logerrs if node registration/unregistration attempted while locked.
- Node declaration and execution callbacks run on the main thread.

## 21. Device Reset Handling

`REGISTER_D3D_BEFORE_RESET_FUNC(dafg::before_reset)` -- on GPU device reset:
1. Invalidates temporal resources
2. Shuts down resource allocator
3. Schedules full node redeclaration
4. Marks stage dirty to `REQUIRES_NODE_DECLARATION_UPDATE`

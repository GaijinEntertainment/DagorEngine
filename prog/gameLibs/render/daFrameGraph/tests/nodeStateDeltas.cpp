// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>

#include <backend/intermediateRepresentation.h>
#include <backend/nodeStateDeltas.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <id/idIndexedFlags.h>
#include <catch2/catch_test_macros.hpp>


namespace
{

// Builds a minimal intermediate::Graph and drives DeltaCalculator directly,
// bypassing the full Runtime. This lets us reproduce specific incremental
// recompilation scenarios (mid-schedule node removal in particular) without
// having to thread the bug through the IR builder, scheduler and barrier scheduler.
//
// The fixture mirrors one production invariant the delta calculator relies on: the
// IR graph is always terminated by a destination-sentinel node with empty state.
// finalize() (re-)appends that sentinel at the end of the schedule.
struct DeltaCalculatorFixture
{
  dafg::intermediate::Graph graph;
  dafg::BarrierScheduler::EventsCollection events;
  dafg::sd::NodeStateDeltas result;
  eastl::optional<dafg::sd::DeltaCalculator> calc;

  // Adds a node at the next free user slot with a frame-layer shader block bound to
  // `frame_layer`. The default of -1 means "no block bound" (matches the production default).
  // The destination sentinel (added by finalize()) is removed here and will be re-added
  // on the next finalize(), so you can freely interleave addNode / finalize calls.
  dafg::intermediate::NodeIndex addNode(const char *name, int frame_layer = -1)
  {
    if (dstSentinel)
    {
      graph.nodes.erase(*dstSentinel);
      graph.nodeStates.erase(*dstSentinel);
      graph.nodeNames.erase(*dstSentinel);
      dstSentinel.reset();
    }

    const auto idx = static_cast<dafg::intermediate::NodeIndex>(userNodeCount++);

    graph.nodes.emplaceAt(idx);

    dafg::intermediate::RequiredNodeState st;
    st.shaderBlockLayers.frameLayer = frame_layer;
    graph.nodeStates.emplaceAt(idx, eastl::move(st));

    auto *dn = graph.nodeNames.emplaceAt(idx);
    *dn = name;

    return idx;
  }

  void finalize()
  {
    // (Re-)append the destination sentinel at the current schedule end. It has empty
    // RequiredNodeState, so the final pair (last_user_node, dstSentinel) naturally
    // encodes the end-of-frame state reset.
    if (!dstSentinel)
    {
      dstSentinel = static_cast<dafg::intermediate::NodeIndex>(userNodeCount);
      graph.nodes.emplaceAt(*dstSentinel);
      graph.nodeStates.emplaceAt(*dstSentinel);
      auto *dn = graph.nodeNames.emplaceAt(*dstSentinel);
      *dn = "__dst_sentinel__";
    }

    // Allocate empty event vectors for every node slot in both frames of the schedule window.
    const auto totalKeys = static_cast<dafg::intermediate::NodeIndex>(graph.nodes.totalKeys());
    for (int frame = 0; frame < dafg::BarrierScheduler::SCHEDULE_FRAME_WINDOW; ++frame)
      events[frame].expandMapping(totalKeys);

    calc.emplace(graph);
  }

  void compile(std::initializer_list<dafg::intermediate::NodeIndex> changed_nodes = {})
  {
    FRAMEMEM_REGION;
    const uint32_t totalNodes = graph.nodes.totalKeys();
    const uint32_t totalRes = graph.resources.totalKeys();
    IdIndexedFlags<dafg::intermediate::NodeIndex, framemem_allocator> nodesChanged(totalNodes, false);
    for (auto idx : changed_nodes)
      nodesChanged.set(idx, true);
    IdIndexedFlags<dafg::intermediate::ResourceIndex, framemem_allocator> resourcesChanged(totalRes, false);
    calc->calculatePerNodeStateDeltas(result, events, nodesChanged, resourcesChanged);
  }

  void removeNode(dafg::intermediate::NodeIndex idx)
  {
    graph.nodes.erase(idx);
    graph.nodeStates.erase(idx);
    graph.nodeNames.erase(idx);
  }

  // Index of the destination sentinel -- the node whose delta carries the end-of-frame
  // state reset. Only valid after finalize() has been called.
  dafg::intermediate::NodeIndex dstSentinelIndex() const { return *dstSentinel; }

  uint32_t userNodeCount = 0;
  eastl::optional<dafg::intermediate::NodeIndex> dstSentinel;
};

} // namespace

TEST_CASE("removed mid-schedule node dirties the new successor's delta", "[stateDeltas][incremental]")
{
  DeltaCalculatorFixture f;

  constexpr int FRAME_BLOCK = 42; // arbitrary, mimics ShaderGlobal::getBlockId("global_frame")

  // C is the only node that resets the frame layer back to "no block".
  [[maybe_unused]] auto a = f.addNode("a", -1);
  [[maybe_unused]] auto b = f.addNode("b", FRAME_BLOCK);
  [[maybe_unused]] auto c = f.addNode("c", -1);
  [[maybe_unused]] auto d = f.addNode("d", -1);
  f.finalize();

  f.compile();

  // Sanity check the initial deltas: B sets the frame layer, C resets it,
  // D inherits the reset and thus needs no further set.
  REQUIRE(f.result.isMapped(a));
  CHECK(!f.result[a].shaderBlockLayers.frameLayer.has_value());

  REQUIRE(f.result.isMapped(b));
  REQUIRE(f.result[b].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[b].shaderBlockLayers.frameLayer == FRAME_BLOCK);

  REQUIRE(f.result.isMapped(c));
  REQUIRE(f.result[c].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[c].shaderBlockLayers.frameLayer == -1);

  REQUIRE(f.result.isMapped(d));
  CHECK(!f.result[d].shaderBlockLayers.frameLayer.has_value());

  // Now remove C. We deliberately do NOT report any node as changed: in the real flow
  // apply_node_remap leaves D at its previous slot (try_remap_node_order preserves prev
  // positions when it can) and does not propagate the removed node's change flag, so
  // nothing in nodes_changed reaches DeltaCalculator either.
  f.removeNode(c);
  f.compile();

  // After recompilation D's predecessor in the schedule is B (frameLayer = FRAME_BLOCK)
  // instead of C (frameLayer = -1). D's delta MUST therefore set frameLayer = -1 to
  // undo what B set, otherwise the frame layer leaks past D and the node will fail
  // validate_global_state during execution.
  REQUIRE(f.result.isMapped(d));
  REQUIRE(f.result[d].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[d].shaderBlockLayers.frameLayer == -1);
}

TEST_CASE("removed last real node dirties the dst sentinel's delta", "[stateDeltas][incremental]")
{
  DeltaCalculatorFixture f;

  constexpr int FRAME_BLOCK = 42;

  // C sets the frame layer; D resets it. After the frame, the dst sentinel carries
  // the global state back to {} (frameLayer == -1).
  [[maybe_unused]] auto a = f.addNode("a", -1);
  [[maybe_unused]] auto b = f.addNode("b", -1);
  [[maybe_unused]] auto c = f.addNode("c", FRAME_BLOCK);
  [[maybe_unused]] auto d = f.addNode("d", -1);
  f.finalize();

  f.compile();

  const auto dst = f.dstSentinelIndex();

  // Initial dst delta: D(-1) → {}(-1) = no set.
  REQUIRE(f.result.isMapped(dst));
  CHECK(!f.result[dst].shaderBlockLayers.frameLayer.has_value());

  // Sanity: D resets the frame layer.
  REQUIRE(f.result[d].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[d].shaderBlockLayers.frameLayer == -1);

  // Remove the last real node. The dst sentinel stays at the same slot.
  f.removeNode(d);
  f.compile();

  // C is now the new last real node. The dst sentinel's delta must transition
  // C(FRAME_BLOCK) → {}(-1) and therefore set frameLayer = -1; otherwise the frame
  // layer leaks across frames and the first node of the next frame sees stale state.
  REQUIRE(f.dstSentinelIndex() == dst);
  REQUIRE(f.result.isMapped(dst));
  REQUIRE(f.result[dst].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[dst].shaderBlockLayers.frameLayer == -1);
}

TEST_CASE("removed first node yields a correct delta for the new first node", "[stateDeltas][incremental]")
{
  DeltaCalculatorFixture f;

  constexpr int FRAME_BLOCK = 42;

  // A and B both want the frame layer set to FRAME_BLOCK. With A present the cached
  // delta[B] is "no set" because A has already established the value. After A is
  // removed, delta[B] becomes the FIRST delta of the frame and must take the layer
  // from {} (-1) to FRAME_BLOCK.
  [[maybe_unused]] auto a = f.addNode("a", FRAME_BLOCK);
  [[maybe_unused]] auto b = f.addNode("b", FRAME_BLOCK);
  [[maybe_unused]] auto c = f.addNode("c", -1);
  f.finalize();

  f.compile();

  REQUIRE(f.result[a].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[a].shaderBlockLayers.frameLayer == FRAME_BLOCK);
  CHECK(!f.result[b].shaderBlockLayers.frameLayer.has_value());

  f.removeNode(a);
  f.compile();

  REQUIRE(f.result.isMapped(b));
  REQUIRE(f.result[b].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[b].shaderBlockLayers.frameLayer == FRAME_BLOCK);
}

TEST_CASE("no-op recompile preserves deltas", "[stateDeltas][incremental]")
{
  DeltaCalculatorFixture f;

  constexpr int FRAME_BLOCK = 42;

  [[maybe_unused]] auto a = f.addNode("a", -1);
  [[maybe_unused]] auto b = f.addNode("b", FRAME_BLOCK);
  [[maybe_unused]] auto c = f.addNode("c", -1);
  f.finalize();

  f.compile();

  REQUIRE(f.result[b].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[b].shaderBlockLayers.frameLayer == FRAME_BLOCK);
  REQUIRE(f.result[c].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[c].shaderBlockLayers.frameLayer == -1);

  // Compile again with no changes. All cached deltas should survive untouched.
  f.compile();

  REQUIRE(f.result[b].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[b].shaderBlockLayers.frameLayer == FRAME_BLOCK);
  REQUIRE(f.result[c].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[c].shaderBlockLayers.frameLayer == -1);
}

TEST_CASE("changing a node's state dirties that node and its successor", "[stateDeltas][incremental]")
{
  DeltaCalculatorFixture f;

  constexpr int FRAME_BLOCK_X = 42;
  constexpr int FRAME_BLOCK_Y = 99;

  [[maybe_unused]] auto a = f.addNode("a", -1);
  [[maybe_unused]] auto b = f.addNode("b", FRAME_BLOCK_X);
  [[maybe_unused]] auto c = f.addNode("c", -1);
  [[maybe_unused]] auto d = f.addNode("d", -1);
  f.finalize();

  f.compile();

  // Initially: B sets X, C resets to -1, D no set.
  REQUIRE(f.result[b].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[b].shaderBlockLayers.frameLayer == FRAME_BLOCK_X);
  REQUIRE(f.result[c].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[c].shaderBlockLayers.frameLayer == -1);
  CHECK(!f.result[d].shaderBlockLayers.frameLayer.has_value());

  // Change B's frame layer from X to Y. This is the kind of change apply_node_remap reports
  // by setting nodes_changed[B] = true and re-emplacing B's nodeState.
  f.graph.nodeStates[b].shaderBlockLayers.frameLayer = FRAME_BLOCK_Y;
  f.compile({b});

  // delta[B] tracks the new value...
  REQUIRE(f.result[b].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[b].shaderBlockLayers.frameLayer == FRAME_BLOCK_Y);
  // ...delta[C] still resets to -1 (its predecessor's value changed but C's expected value
  // didn't, so the reset is still required)...
  REQUIRE(f.result[c].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[c].shaderBlockLayers.frameLayer == -1);
  // ...and delta[D] still has no set (C → D is still -1 → -1).
  CHECK(!f.result[d].shaderBlockLayers.frameLayer.has_value());
}

TEST_CASE("appending a node updates dst sentinel and the new node's delta", "[stateDeltas][incremental]")
{
  DeltaCalculatorFixture f;

  constexpr int FRAME_BLOCK = 42;

  [[maybe_unused]] auto a = f.addNode("a", -1);
  [[maybe_unused]] auto b = f.addNode("b", -1);
  f.finalize();

  f.compile();

  const auto oldDst = f.dstSentinelIndex();
  // Initial dst delta does nothing (B already has frameLayer = -1).
  REQUIRE(f.result.isMapped(oldDst));
  CHECK(!f.result[oldDst].shaderBlockLayers.frameLayer.has_value());

  // Append a third user node that sets the frame layer. addNode pops the dst sentinel
  // off the end, finalize() puts it back, so the dst ends up at a new slot.
  [[maybe_unused]] auto c = f.addNode("c", FRAME_BLOCK);
  f.finalize();
  f.compile({c});

  // delta[C] should set FRAME_BLOCK (predecessor B has -1).
  REQUIRE(f.result.isMapped(c));
  REQUIRE(f.result[c].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[c].shaderBlockLayers.frameLayer == FRAME_BLOCK);

  // The dst sentinel moved one slot past its old position, and its delta must reset
  // C's frame layer back to {}'s default (-1).
  const auto newDst = f.dstSentinelIndex();
  CHECK(newDst != oldDst);
  REQUIRE(f.result.isMapped(newDst));
  REQUIRE(f.result[newDst].shaderBlockLayers.frameLayer.has_value());
  CHECK(*f.result[newDst].shaderBlockLayers.frameLayer == -1);
}

TEST_CASE("shrinking the graph does not trip OOB on stale result keys", "[stateDeltas][incremental]")
{
  // Reproduces a crash that happens when IR builder's apply_node_remap hits its
  // fallback path (remap failed -> graph.{nodes,nodeStates,nodeNames}.clear(), then
  // re-emplace with a dense mapping). After that, graph.nodeStates.totalKeys() shrinks,
  // but the cached `result` from the previous compile still holds entries at higher
  // indices. The dirty-detection pass then indexes dirtyDeltas[key] with those stale
  // keys, which is out of bounds now that dirtyDeltas is sized to the new (smaller)
  // totalKeys.
  DeltaCalculatorFixture f;

  constexpr int FRAME_BLOCK = 42;

  // Build a 4-user-node graph and compile it.
  [[maybe_unused]] auto a = f.addNode("a", -1);
  [[maybe_unused]] auto b = f.addNode("b", FRAME_BLOCK);
  [[maybe_unused]] auto c = f.addNode("c", -1);
  [[maybe_unused]] auto d = f.addNode("d", -1);
  f.finalize();
  f.compile();

  // Sanity: result has entries across all five slots (four users + dst sentinel).
  REQUIRE(f.result.isMapped(static_cast<dafg::intermediate::NodeIndex>(4)));

  // Simulate apply_node_remap's fallback: clear the graph entirely and re-populate
  // with fewer nodes. DeltaCalculator's class members (cachedForcePassBreak,
  // firstActivationPosition, ...) are preserved across this, which matches production
  // (the DeltaCalculator instance itself is long-lived).
  f.graph.nodes.clear();
  f.graph.nodeStates.clear();
  f.graph.nodeNames.clear();
  f.userNodeCount = 0;
  f.dstSentinel.reset();

  [[maybe_unused]] auto newA = f.addNode("a", -1);
  f.finalize();

  // This compile should not crash / OOB. The old result entries at slots 2..4 need to
  // be handled cleanly even though they now sit beyond graph.nodeStates.totalKeys().
  f.compile();

  // After the shrink the single user node and the dst sentinel should both have valid,
  // freshly-computed deltas.
  REQUIRE(f.result.isMapped(newA));
  REQUIRE(f.result.isMapped(f.dstSentinelIndex()));
}

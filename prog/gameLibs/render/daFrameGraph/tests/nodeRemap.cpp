// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>

#include <backend/intermediateRepresentation.h>
#include <backend/nodeScheduler.h>
#include <id/idIndexedFlags.h>
#include <catch2/catch_test_macros.hpp>

namespace
{

using dafg::intermediate::NODE_NOT_MAPPED;
using dafg::intermediate::NodeIndex;

NodeIndex node(uint32_t idx) { return static_cast<NodeIndex>(idx); }

dafg::intermediate::Graph make_chain_with_no_effect_middle()
{
  dafg::intermediate::Graph graph;
  for (uint32_t i = 0; i < 3; ++i)
  {
    auto *n = graph.nodes.emplaceAt(node(i));
    n->frontendNode = static_cast<dafg::NodeNameId>(i);
    n->multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
    n->hasSideEffects = i != 1;
    graph.nodeStates.emplaceAt(node(i));
    graph.nodeNames.emplaceAt(node(i));
  }
  graph.nodes[node(2)].predecessors.insert(node(1));
  graph.nodes[node(1)].predecessors.insert(node(0));
  return graph;
}

} // namespace

TEST_CASE("a kept node's predecessors are refreshed after a predecessor is removed", "[nodeRemap][incremental]")
{
  FRAMEMEM_REGION;

  dafg::intermediate::Graph sorted;
  sorted.nodes.emplaceAt(node(0));
  sorted.nodes.emplaceAt(node(1));
  sorted.nodes.emplaceAt(node(2))->predecessors.insert(node(1));
  IdIndexedMapping<NodeIndex, NodeIndex> prevPermutation(3, NODE_NOT_MAPPED);
  for (uint32_t i = 0; i < 3; ++i)
    prevPermutation[node(i)] = node(i);

  dafg::intermediate::Graph unsorted;
  unsorted.nodes.emplaceAt(node(0));
  unsorted.nodes.emplaceAt(node(2))->predecessors.insert(node(0));
  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> desired(3, NODE_NOT_MAPPED);
  desired[node(0)] = node(0);
  desired[node(2)] = node(1);
  IdIndexedFlags<NodeIndex, framemem_allocator> sourceChanged(3, false);
  sourceChanged.set(node(0), true);
  IdIndexedFlags<NodeIndex, framemem_allocator> changed;

  const auto sourceToDst = dafg::intermediate::apply_node_remap(sorted, unsorted, desired, prevPermutation, sourceChanged, changed);

  REQUIRE(sourceToDst[node(2)] == node(2));
  CHECK_FALSE(changed[node(2)]);
  const auto &kPreds = sorted.nodes[node(2)].predecessors;
  REQUIRE(kPreds.size() == 1);
  CHECK(kPreds.contains(sourceToDst[node(0)]));
}

TEST_CASE("a new node in a vacated source slot replaces the removed node's copy", "[nodeRemap][incremental]")
{
  FRAMEMEM_REGION;

  dafg::intermediate::Graph sorted;
  for (uint32_t i = 0; i < 3; ++i)
    sorted.nodes.emplaceAt(node(i));
  IdIndexedMapping<NodeIndex, NodeIndex> prevPermutation(3, NODE_NOT_MAPPED);
  for (uint32_t i = 0; i < 3; ++i)
    prevPermutation[node(i)] = node(i);

  dafg::intermediate::Graph unsorted;
  for (uint32_t i = 0; i < 3; ++i)
    unsorted.nodes.emplaceAt(node(i));
  unsorted.nodes[node(2)].priority = 7;
  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> desired(3, NODE_NOT_MAPPED);
  for (uint32_t i = 0; i < 3; ++i)
    desired[node(i)] = node(i);
  IdIndexedFlags<NodeIndex, framemem_allocator> sourceChanged(3, false);
  sourceChanged.set(node(2), true);
  IdIndexedFlags<NodeIndex, framemem_allocator> changed;

  const auto sourceToDst = dafg::intermediate::apply_node_remap(sorted, unsorted, desired, prevPermutation, sourceChanged, changed);

  REQUIRE(sourceToDst[node(2)] == node(2));
  CHECK(changed[node(2)]);
  CHECK(sorted.nodes[node(2)].priority == 7);
  CHECK_FALSE(changed[node(0)]);
  CHECK_FALSE(changed[node(1)]);
}

TEST_CASE("a node inserted at the front evicts the kept nodes it collides with", "[nodeRemap][incremental]")
{
  FRAMEMEM_REGION;

  dafg::intermediate::Graph sorted;
  for (int i = 0; i < 3; ++i)
    sorted.nodes.emplaceAt(node(i))->priority = i + 1;
  IdIndexedMapping<NodeIndex, NodeIndex> prevPermutation(4, NODE_NOT_MAPPED);
  for (int i = 0; i < 3; ++i)
    prevPermutation[node(i)] = node(i);

  dafg::intermediate::Graph unsorted;
  for (int i = 0; i < 4; ++i)
    unsorted.nodes.emplaceAt(node(i))->priority = i + 1;
  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> desired(4, NODE_NOT_MAPPED);
  desired[node(3)] = node(0);
  for (int i = 0; i < 3; ++i)
    desired[node(i)] = node(i + 1);
  IdIndexedFlags<NodeIndex, framemem_allocator> sourceChanged(4, false);
  sourceChanged.set(node(3), true);
  IdIndexedFlags<NodeIndex, framemem_allocator> changed;

  const auto sourceToDst = dafg::intermediate::apply_node_remap(sorted, unsorted, desired, prevPermutation, sourceChanged, changed);

  for (int i = 0; i < 4; ++i)
  {
    REQUIRE(sourceToDst[node(i)] == desired[node(i)]);
    CHECK(changed[node(i)]);
    CHECK(sorted.nodes[desired[node(i)]].priority == i + 1);
  }
}

TEST_CASE("the node scheduler keeps the previous order of unordered nodes", "[nodeScheduler][incremental]")
{
  FRAMEMEM_REGION;

  using Positions = IdIndexedMapping<NodeIndex, NodeIndex>;
  const auto positionsOf = [](const dafg::NodeScheduler::NodePermutation &order) { return Positions(order.begin(), order.end()); };

  dafg::intermediate::Graph graph;
  for (uint32_t i = 0; i < 3; ++i)
    graph.nodes.emplaceAt(node(i))->multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
  graph.nodes.emplaceAt(node(4))->multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
  for (uint32_t i = 0; i < 3; ++i)
    graph.nodes[node(4)].predecessors.insert(node(i));
  const dafg::PassColoring coloring(5, dafg::PassColor{0});

  dafg::NodeScheduler scheduler;

  const auto fresh = scheduler.schedule(graph, coloring, Positions{});
  CHECK(fresh[node(0)] == node(2));
  CHECK(fresh[node(1)] == node(1));
  CHECK(fresh[node(2)] == node(0));
  CHECK(fresh[node(4)] == node(3));

  Positions previous(5, NODE_NOT_MAPPED);
  previous[node(0)] = node(1);
  previous[node(1)] = node(2);
  previous[node(2)] = node(0);
  previous[node(4)] = node(3);
  const auto kept = scheduler.schedule(graph, coloring, previous);
  for (const auto idx : {node(0), node(1), node(2), node(4)})
  {
    CHECK(kept[idx] == previous[idx]);
  }

  graph.nodes.emplaceAt(node(3))->multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
  graph.nodes[node(4)].predecessors.insert(node(3));
  const auto withNew = scheduler.schedule(graph, coloring, positionsOf(kept));
  for (const auto idx : {node(0), node(1), node(2)})
  {
    CHECK(withNew[idx] == kept[idx]);
  }
  CHECK(withNew[node(3)] == node(3));
  CHECK(withNew[node(4)] == node(4));

  const auto again = scheduler.schedule(graph, coloring, positionsOf(withNew));
  for (const auto idx : graph.nodes.keys())
  {
    CHECK(again[idx] == withNew[idx]);
  }
}

TEST_CASE("the remap hides no-effect nodes from everything but all()", "[nodeRemap][incremental]")
{
  FRAMEMEM_REGION;

  auto unsorted = make_chain_with_no_effect_middle();

  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> desired(3, NODE_NOT_MAPPED);
  for (uint32_t i = 0; i < 3; ++i)
    desired[node(i)] = node(i);
  IdIndexedFlags<NodeIndex, framemem_allocator> sourceChanged(3, true);

  dafg::intermediate::Graph sorted;
  IdIndexedFlags<NodeIndex, framemem_allocator> changed;
  const auto sourceToDst = dafg::intermediate::apply_node_remap(sorted, unsorted, desired, {}, sourceChanged, changed, true);
  sorted.validate();

  const auto hidden = sourceToDst[node(1)];
  CHECK_FALSE(sorted.nodes.isMapped(hidden));
  CHECK_FALSE(sorted.nodeStates.isMapped(hidden));
  CHECK_FALSE(sorted.nodeNames.isMapped(hidden));
  CHECK(sorted.nodes.all().isMapped(hidden));
  CHECK(sorted.nodeStates.all().isMapped(hidden));
  CHECK(sorted.nodeNames.all().isMapped(hidden));
  CHECK(sorted.nodes.used() == 2);
  CHECK(sorted.nodes.getNextUsed(sourceToDst[node(0)]) == sourceToDst[node(2)]);
  CHECK(sorted.nodes.all()[sourceToDst[node(2)]].predecessors.contains(hidden));

  CHECK(changed[sourceToDst[node(0)]]);
  CHECK(changed[sourceToDst[node(2)]]);
  CHECK_FALSE(changed.test(hidden, false));

  const auto mapping = sorted.calculateMapping();
  CHECK(mapping.mapNode(static_cast<dafg::NodeNameId>(1), dafg::intermediate::MultiplexingIndex{0}) == hidden);

  IdIndexedMapping<NodeIndex, NodeIndex> prevPermutation(sourceToDst.begin(), sourceToDst.end());
  unsorted.nodes.erase(node(1));
  unsorted.nodeStates.erase(node(1));
  unsorted.nodeNames.erase(node(1));
  unsorted.nodes[node(2)].predecessors = {node(0)};
  desired[node(1)] = NODE_NOT_MAPPED;
  desired[node(2)] = node(1);
  IdIndexedFlags<NodeIndex, framemem_allocator> noneChanged(3, false);
  IdIndexedFlags<NodeIndex, framemem_allocator> changedAgain;
  dafg::intermediate::apply_node_remap(sorted, unsorted, desired, prevPermutation, noneChanged, changedAgain, true);
  sorted.validate();

  CHECK(sorted.nodes.used() == 2);
  CHECK(sorted.nodes.all().used() == 2);
  CHECK(sorted.nodeStates.all().used() == 2);
  CHECK(sorted.nodeNames.all().used() == 2);
}

TEST_CASE("the remap shows a hidden node again once it gains side effects", "[nodeRemap][incremental]")
{
  FRAMEMEM_REGION;

  auto unsorted = make_chain_with_no_effect_middle();

  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> desired(3, NODE_NOT_MAPPED);
  for (uint32_t i = 0; i < 3; ++i)
    desired[node(i)] = node(i);
  IdIndexedFlags<NodeIndex, framemem_allocator> allChanged(3, true);

  dafg::intermediate::Graph sorted;
  IdIndexedFlags<NodeIndex, framemem_allocator> changed;
  const auto sourceToDst = dafg::intermediate::apply_node_remap(sorted, unsorted, desired, {}, allChanged, changed, true);
  REQUIRE_FALSE(sorted.nodes.isMapped(sourceToDst[node(1)]));

  unsorted.nodes[node(1)].hasSideEffects = true;
  IdIndexedMapping<NodeIndex, NodeIndex> prevPermutation(sourceToDst.begin(), sourceToDst.end());
  IdIndexedFlags<NodeIndex, framemem_allocator> oneChanged(3, false);
  oneChanged.set(node(1), true);
  IdIndexedFlags<NodeIndex, framemem_allocator> changedAgain;
  const auto shownToDst =
    dafg::intermediate::apply_node_remap(sorted, unsorted, desired, prevPermutation, oneChanged, changedAgain, true);
  sorted.validate();

  const auto shown = shownToDst[node(1)];
  CHECK(sorted.nodes.isMapped(shown));
  CHECK(sorted.nodeStates.isMapped(shown));
  CHECK(sorted.nodeNames.isMapped(shown));
  CHECK(sorted.nodes.used() == 3);
  CHECK(changedAgain.test(shown, false));
}

TEST_CASE("the remap keeps an unchanged hidden node hidden", "[nodeRemap][incremental]")
{
  FRAMEMEM_REGION;

  auto unsorted = make_chain_with_no_effect_middle();

  IdIndexedMapping<NodeIndex, NodeIndex, framemem_allocator> desired(3, NODE_NOT_MAPPED);
  for (uint32_t i = 0; i < 3; ++i)
    desired[node(i)] = node(i);
  IdIndexedFlags<NodeIndex, framemem_allocator> allChanged(3, true);

  dafg::intermediate::Graph sorted;
  IdIndexedFlags<NodeIndex, framemem_allocator> changed;
  const auto sourceToDst = dafg::intermediate::apply_node_remap(sorted, unsorted, desired, {}, allChanged, changed, true);
  const auto hidden = sourceToDst[node(1)];
  REQUIRE_FALSE(sorted.nodes.isMapped(hidden));

  IdIndexedMapping<NodeIndex, NodeIndex> prevPermutation(sourceToDst.begin(), sourceToDst.end());
  IdIndexedFlags<NodeIndex, framemem_allocator> noneChanged(3, false);
  IdIndexedFlags<NodeIndex, framemem_allocator> changedAgain;
  const auto keptToDst =
    dafg::intermediate::apply_node_remap(sorted, unsorted, desired, prevPermutation, noneChanged, changedAgain, true);
  sorted.validate();

  REQUIRE(keptToDst[node(1)] == hidden);
  CHECK_FALSE(sorted.nodes.isMapped(hidden));
  CHECK_FALSE(sorted.nodeStates.isMapped(hidden));
  CHECK_FALSE(sorted.nodeNames.isMapped(hidden));
  CHECK(sorted.nodes.all().isMapped(hidden));
  CHECK(sorted.nodeStates.all().isMapped(hidden));
  CHECK(sorted.nodeNames.all().isMapped(hidden));
  CHECK(sorted.nodes.used() == 2);
  CHECK(sorted.nodes.all().used() == 3);
  CHECK_FALSE(changedAgain.test(hidden, false));
}

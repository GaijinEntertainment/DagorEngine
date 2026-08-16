// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testRuntime.h"

#include <render/metronome.h>

#include <catch2/catch_test_macros.hpp>
#include <util/dag_string.h>

void set_logerr_capture(String *target);

namespace metronome = dafg::metronome;
using metronome::UpdateStatus;

namespace
{
struct CountingNode
{
  int executed = 0;

  void registerTo(metronome::SubgraphHandle &sg, const char *name)
  {
    sg.register_node(name, DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      return [this] { ++executed; };
    });
  }
};
} // namespace

TEST_CASE("single request runs once and completes", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode node;

  auto sg = metronome::make_subgraph("single", 90);
  node.registerTo(sg, "single_node");

  auto token = sg.schedule();
  CHECK(token.status() == UpdateStatus::Pending);

  metronome::update();
  CHECK(token.status() == UpdateStatus::Running);
  testRuntime.executeGraph();
  CHECK(node.executed == 1);

  metronome::update();
  CHECK(token.status() == UpdateStatus::Complete);
  CHECK(static_cast<bool>(token == UpdateStatus::Complete)); // == sugar
  testRuntime.executeGraph();
  CHECK(node.executed == 1); // nodes were unregistered, does not run again

  metronome::update();
  testRuntime.executeGraph();
  CHECK(node.executed == 1);
  CHECK(token.status() == UpdateStatus::Complete);
}

TEST_CASE("at most one subgraph per frame, least slack first", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode nodeA, nodeB;

  auto sgA = metronome::make_subgraph("relaxed", 90);
  nodeA.registerTo(sgA, "relaxed_node");
  auto sgB = metronome::make_subgraph("urgent", 5);
  nodeB.registerTo(sgB, "urgent_node");

  auto tokenA = sgA.schedule();
  auto tokenB = sgB.schedule();

  metronome::update();
  CHECK(tokenB.status() == UpdateStatus::Running);
  CHECK(tokenA.status() == UpdateStatus::Pending);
  testRuntime.executeGraph();
  CHECK(nodeB.executed == 1);
  CHECK(nodeA.executed == 0);

  metronome::update();
  CHECK(tokenB.status() == UpdateStatus::Complete);
  CHECK(tokenA.status() == UpdateStatus::Running);
  testRuntime.executeGraph();
  CHECK(nodeA.executed == 1);

  metronome::update();
  CHECK(tokenA.status() == UpdateStatus::Complete);
}

TEST_CASE("staggered deadlines do not collide", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode nodeA, nodeB;

  String captured;
  set_logerr_capture(&captured);

  auto sgA = metronome::make_subgraph("tight", 1);
  nodeA.registerTo(sgA, "tight_node");
  auto sgB = metronome::make_subgraph("loose", 2);
  nodeB.registerTo(sgB, "loose_node");

  auto tokenA = sgA.schedule();
  auto tokenB = sgB.schedule();

  // sgA is forced first; sgB still fits into the next frame, so no logerr.
  metronome::update();
  testRuntime.executeGraph();
  CHECK(nodeA.executed == 1);
  CHECK(nodeB.executed == 0);

  metronome::update();
  testRuntime.executeGraph();
  CHECK(nodeB.executed == 1);
  CHECK(tokenA.status() == UpdateStatus::Complete);
  CHECK(tokenB.status() == UpdateStatus::Running); // active on this frame, completes at the next tick

  metronome::update();
  CHECK(tokenB.status() == UpdateStatus::Complete);
  CHECK(captured.empty());

  set_logerr_capture(nullptr);
}

TEST_CASE("colliding deadlines run together and logerr", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode nodeA, nodeB;

  String captured;
  set_logerr_capture(&captured);

  auto sgA = metronome::make_subgraph("collide_a", 1);
  nodeA.registerTo(sgA, "collide_a_node");
  auto sgB = metronome::make_subgraph("collide_b", 1);
  nodeB.registerTo(sgB, "collide_b_node");

  sgA.schedule();
  sgB.schedule();

  // Both must run on the next frame to honor their deadlines: the flat-cost
  // guarantee is broken, so both run and a logerr names them.
  metronome::update();
  testRuntime.executeGraph();
  CHECK(nodeA.executed == 1);
  CHECK(nodeB.executed == 1);
  CHECK(strstr(captured.str(), "collide_a") != nullptr);
  CHECK(strstr(captured.str(), "collide_b") != nullptr);

  // No unrelated errors leaked in while the capture target was installed.
  int errCount = 0;
  for (const char *p = captured.str(); p && *p;)
    if ((p = strstr(p, "[E] ")) != nullptr)
    {
      ++errCount;
      ++p;
    }
  CHECK(errCount == 2);

  set_logerr_capture(nullptr);
}

TEST_CASE("schedule supersedes a pending request", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode node;

  String captured;
  set_logerr_capture(&captured);

  auto sg = metronome::make_subgraph("superseded", 90);
  node.registerTo(sg, "superseded_node");

  auto stale = sg.schedule();
  auto token = sg.schedule(); // same-frame reschedule: the guard must flag it
  CHECK(stale.status() == UpdateStatus::Superseded);
  CHECK(token.status() == UpdateStatus::Pending);
  CHECK(strstr(captured.str(), "scheduled more than once in the same frame") != nullptr);

  metronome::update();
  testRuntime.executeGraph();
  CHECK(node.executed == 1); // one activation despite two requests

  metronome::update();
  CHECK(stale.status() == UpdateStatus::Superseded);
  CHECK(token.status() == UpdateStatus::Complete);

  set_logerr_capture(nullptr);
}

TEST_CASE("schedule while running supersedes the running token", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode node;

  auto sg = metronome::make_subgraph("reschedule", 90);
  node.registerTo(sg, "reschedule_node");

  auto first = sg.schedule();
  metronome::update();
  testRuntime.executeGraph();
  CHECK(node.executed == 1);
  CHECK(first.status() == UpdateStatus::Running);

  // Scheduling while Running supersedes the running token; the work still
  // completes (nodes were already registered), but the token reports
  // Superseded immediately.
  auto second = sg.schedule();
  CHECK(second.status() == UpdateStatus::Pending);
  CHECK(first.status() == UpdateStatus::Superseded);

  metronome::update();
  CHECK(first.status() == UpdateStatus::Superseded);
  CHECK(second.status() == UpdateStatus::Running);
  testRuntime.executeGraph();
  CHECK(node.executed == 2);

  metronome::update();
  CHECK(first.status() == UpdateStatus::Superseded);
  CHECK(second.status() == UpdateStatus::Complete);
}

TEST_CASE("destroying the handle removes the subgraph", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode pendingNode, activeNode;

  auto pendingToken = metronome::UpdateToken{};
  {
    auto sgPending = metronome::make_subgraph("dying_pending", 90);
    pendingNode.registerTo(sgPending, "dying_pending_node");
    pendingToken = sgPending.schedule();
  }
  CHECK(pendingToken.status() == UpdateStatus::Superseded);

  auto activeToken = metronome::UpdateToken{};
  {
    auto sgActive = metronome::make_subgraph("dying_active", 90);
    activeNode.registerTo(sgActive, "dying_active_node");
    activeToken = sgActive.schedule();
    metronome::update();
    CHECK(activeToken.status() == UpdateStatus::Running);
  } // unregisters the nodes before the graph ran
  CHECK(activeToken.status() == UpdateStatus::Superseded);

  metronome::update();
  testRuntime.executeGraph();
  CHECK(pendingNode.executed == 0);
  CHECK(activeNode.executed == 0);
}

TEST_CASE("subgraph without nodes still completes", "[metronome]")
{
  TestRuntime testRuntime{};

  auto sg = metronome::make_subgraph("empty", 90);
  auto token = sg.schedule();

  metronome::update();
  CHECK(token.status() == UpdateStatus::Running);
  testRuntime.executeGraph();

  metronome::update();
  CHECK(token.status() == UpdateStatus::Complete);
}

TEST_CASE("node with multiplexing index callback and explicit namespace", "[metronome]")
{
  TestRuntime testRuntime{};
  int executed = 0;

  auto sg = metronome::make_subgraph("indexed", 90);
  sg.register_node(dafg::root() / "metronome_test_ns", "indexed_node", DAFG_PP_NODE_SRC, [&executed](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    return [&executed](dafg::multiplexing::Index) { ++executed; };
  });

  auto token = sg.schedule();
  metronome::update();
  testRuntime.executeGraph();
  CHECK(executed == 1);

  metronome::update();
  CHECK(token.status() == UpdateStatus::Complete);
  testRuntime.executeGraph();
  CHECK(executed == 1);
}

TEST_CASE("schedule after complete runs the subgraph again", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode node;

  auto sg = metronome::make_subgraph("reusable", 90);
  node.registerTo(sg, "reusable_node");

  auto first = sg.schedule();
  metronome::update();
  testRuntime.executeGraph();
  CHECK(node.executed == 1);
  metronome::update();
  CHECK(first.status() == UpdateStatus::Complete);

  // Complete -> Pending: the primary caller loop from the task sketch.
  auto second = sg.schedule();
  CHECK(first.status() == UpdateStatus::Superseded);
  CHECK(second.status() == UpdateStatus::Pending);

  metronome::update();
  CHECK(second.status() == UpdateStatus::Running);
  testRuntime.executeGraph();
  CHECK(node.executed == 2);

  metronome::update();
  CHECK(second.status() == UpdateStatus::Complete);
}

TEST_CASE("least slack, not earliest deadline is the selection rule", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode nodeA, nodeB;

  auto sgA = metronome::make_subgraph("older_relaxed", 10);
  nodeA.registerTo(sgA, "older_relaxed_node");
  auto agerDummy = metronome::make_subgraph("ager", 2);
  CountingNode agerNode;
  agerNode.registerTo(agerDummy, "ager_node");

  auto tokenA = sgA.schedule();

  // Age sgA by 7 ticks with a tight dummy that wins every tick (its slack
  // after a fresh schedule is always 1). After 7 ticks sgA has slack 3;
  // a fresh sgB(maxDelayFrames=5) has slack 4 at its first update, so sgA
  // runs first even though its deadline is twice as long.
  for (int i = 0; i < 7; ++i)
  {
    agerDummy.schedule();
    metronome::update();
    testRuntime.executeGraph();
  }

  // Stop re-scheduling the dummy; it will complete on the next update.
  auto sgB = metronome::make_subgraph("newer_tight", 5);
  nodeB.registerTo(sgB, "newer_tight_node");
  auto tokenB = sgB.schedule();

  metronome::update();
  // sgA slack=2, sgB slack=4 -- sgA runs first despite longer maxDelayFrames.
  CHECK(tokenA.status() == UpdateStatus::Running);
  CHECK(tokenB.status() == UpdateStatus::Pending);
  testRuntime.executeGraph();
  CHECK(nodeA.executed == 1);
  CHECK(nodeB.executed == 0);

  metronome::update();
  CHECK(tokenA.status() == UpdateStatus::Complete);
  CHECK(tokenB.status() == UpdateStatus::Running);
  testRuntime.executeGraph();
  CHECK(nodeB.executed == 1);

  metronome::update();
  CHECK(tokenB.status() == UpdateStatus::Complete);
}

TEST_CASE("same slack picks the older subgraph", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode nodeA, nodeB;

  auto sgA = metronome::make_subgraph("older_equal", 6);
  nodeA.registerTo(sgA, "older_equal_node");
  auto agerDummy = metronome::make_subgraph("ager2", 2);
  CountingNode agerNode;
  agerNode.registerTo(agerDummy, "ager2_node");

  auto tokenA = sgA.schedule();

  // Age sgA by 2 ticks: slack goes from 6 to 4.
  for (int i = 0; i < 2; ++i)
  {
    agerDummy.schedule();
    metronome::update();
    testRuntime.executeGraph();
  }

  // sgA maxDelayFrames=6, aged 2 ticks: at the next update age=3, slack=3.
  // sgB maxDelayFrames=4, fresh: age=1, slack=3.
  // Equal slack; sgA is older, so the age tie-break picks sgA first.
  auto sgB = metronome::make_subgraph("newer_equal", 4);
  nodeB.registerTo(sgB, "newer_equal_node");
  auto tokenB = sgB.schedule();

  metronome::update();
  CHECK(tokenA.status() == UpdateStatus::Running);
  CHECK(tokenB.status() == UpdateStatus::Pending);
  testRuntime.executeGraph();
  CHECK(nodeA.executed == 1);
  CHECK(nodeB.executed == 0);

  metronome::update();
  CHECK(tokenA.status() == UpdateStatus::Complete);
  CHECK(tokenB.status() == UpdateStatus::Running);
  testRuntime.executeGraph();
  CHECK(nodeB.executed == 1);

  metronome::update();
  CHECK(tokenB.status() == UpdateStatus::Complete);
}

TEST_CASE("move assignment releases the target subgraph", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode nodeA, nodeB;

  auto sgA = metronome::make_subgraph("move_target", 90);
  nodeA.registerTo(sgA, "move_target_node");
  auto tokenA = sgA.schedule();

  auto sgB = metronome::make_subgraph("move_source", 90);
  nodeB.registerTo(sgB, "move_source_node");
  auto tokenB = sgB.schedule();

  // Move-assign sgB over sgA: sgA's subgraph is released, its token
  // becomes Superseded. sgB's nodes should still execute.
  sgA = eastl::move(sgB);
  CHECK(tokenA.status() == UpdateStatus::Superseded);
  CHECK(tokenB.status() == UpdateStatus::Pending);

  metronome::update();
  CHECK(tokenB.status() == UpdateStatus::Running);
  testRuntime.executeGraph();
  CHECK(nodeB.executed == 1);
  CHECK(nodeA.executed == 0);

  metronome::update();
  CHECK(tokenB.status() == UpdateStatus::Complete);
}

TEST_CASE("default and moved-from handle report invalid", "[metronome]")
{
  CHECK(!metronome::SubgraphHandle{}.valid());
  CHECK(!static_cast<bool>(metronome::SubgraphHandle{}));

  auto sg = metronome::make_subgraph("mv", 90);
  metronome::SubgraphHandle movedTo = eastl::move(sg);
  CHECK(!sg.valid()); // moved-from handle is invalid
  CHECK(movedTo.valid());
}

TEST_CASE("register_node into invalid handle logerrs", "[metronome]")
{
  TestRuntime testRuntime{};
  String captured;
  set_logerr_capture(&captured);

  metronome::SubgraphHandle invalid;
  invalid.register_node("should_fail", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    return [] {};
  });
  CHECK(strstr(captured.str(), "invalid subgraph id") != nullptr);

  set_logerr_capture(nullptr);
}

TEST_CASE("slot reuse preserves generation for stale tokens", "[metronome]")
{
  TestRuntime testRuntime{};
  CountingNode node1, node2;

  auto token = metronome::UpdateToken{};
  {
    auto sg1 = metronome::make_subgraph("first_incarnation", 90);
    node1.registerTo(sg1, "first_node");
    token = sg1.schedule();
    CHECK(token.status() == UpdateStatus::Pending);
  } // sg1 destroyed, slot freed, generation preserved

  CHECK(token.status() == UpdateStatus::Superseded);

  // Reusing the freed slot: stale token still reads Superseded.
  auto sg2 = metronome::make_subgraph("second_incarnation", 90);
  node2.registerTo(sg2, "second_node");
  CHECK(token.status() == UpdateStatus::Superseded);

  auto newToken = sg2.schedule();
  CHECK(token.status() == UpdateStatus::Superseded);
  CHECK(newToken.status() == UpdateStatus::Pending);

  metronome::update();
  testRuntime.executeGraph();
  CHECK(node2.executed == 1);
  CHECK(node1.executed == 0);

  metronome::update();
  CHECK(newToken.status() == UpdateStatus::Complete);
}

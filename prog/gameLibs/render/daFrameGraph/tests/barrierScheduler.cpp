// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>

#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <backend/resourceScheduling/resourceLifetimes.h>
#include <common/resourceUsage.h>
#include <id/idIndexedFlags.h>

#include "testRuntime.h"

#include <catch2/catch_test_macros.hpp>


namespace
{

using dafg::intermediate::NodeIndex;
using dafg::intermediate::ResourceIndex;

NodeIndex node(uint32_t idx) { return static_cast<NodeIndex>(idx); }

bool has_barrier(const dafg::BarrierScheduler::EventsCollection &events, ResourceIndex res, ResourceBarrier flag)
{
  for (const auto &frameEvents : events)
    for (const auto &nodeEvents : frameEvents.values())
      for (const auto &event : nodeEvents)
        if (event.resource == res)
          if (const auto *barrier = eastl::get_if<dafg::BarrierScheduler::Event::Barrier>(&event.data))
            if ((barrier->barrier & flag) != 0)
              return true;
  return false;
}

} // namespace

TEST_CASE("a request change alone rebuilds the resource's barriers", "[barrierScheduler][incremental]")
{
  TestRuntime testRuntime{};
  FRAMEMEM_REGION;

  dafg::intermediate::Graph graph;

  TextureResourceDescription texDesc{};
  texDesc.cFlags = TEXFMT_R8G8B8A8 | TEXCF_RTARGET;
  texDesc.activation = ResourceActivationAction::DISCARD_AS_RTV_DSV;
  texDesc.mipLevels = 1;
  texDesc.width = 4;
  texDesc.height = 4;
  dafg::intermediate::ScheduledResource scheduled;
  scheduled.description = ResourceDescription{texDesc};
  scheduled.resourceType = dafg::ResourceType::Texture;
  scheduled.clearStage = dafg::intermediate::ClearStage::None;
  scheduled.clearValue = ResourceClearValue{};
  scheduled.clearFlags = RESOURCE_CLEAR_ALL_CONTENT;
  scheduled.history = dafg::History::No;
  scheduled.autoMipCount = false;
  auto [texIdx, texRes] = graph.resources.appendNew();
  const ResourceIndex tex = texIdx;
  texRes.resource = eastl::move(scheduled);
  texRes.multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
  texRes.frontendResources.push_back(static_cast<dafg::ResNameId>(0));
  graph.resourceNames.emplaceAt(tex, "tex");

  const auto addNode = [&graph, tex](uint32_t idx, const char *name, dafg::intermediate::ResourceUsage usage) {
    auto *n = graph.nodes.emplaceAt(node(idx));
    n->multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
    n->resourceRequests.push_back({tex, usage, false});
    graph.nodeStates.emplaceAt(node(idx));
    graph.nodeNames.emplaceAt(node(idx), name);
  };
  addNode(0, "producer", {dafg::Usage::COLOR_ATTACHMENT, dafg::Access::READ_WRITE, dafg::Stage::PS});
  addNode(1, "consumer", {dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_ONLY, dafg::Stage::PS});
  graph.nodes[node(1)].predecessors.insert(node(0));
  graph.nodes.emplaceAt(node(2))->predecessors.insert(node(1));
  graph.nodes[node(2)].multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
  graph.nodeStates.emplaceAt(node(2));
  graph.nodeNames.emplaceAt(node(2), "sentinel");

  dafg::PassColoring coloring(3, dafg::PassColor{0});
  coloring[node(1)] = dafg::PassColor{1};
  coloring[node(2)] = dafg::PassColor{2};

  dafg::ResourceLifetimeCalculator lifetimeCalculator;
  dafg::BarrierScheduler scheduler;
  dafg::BarrierScheduler::EventsCollection events;

  IdIndexedFlags<NodeIndex, framemem_allocator> nodesChanged(3, true);
  IdIndexedFlags<ResourceIndex, framemem_allocator> allResources(1, true);
  IdIndexedFlags<ResourceIndex, framemem_allocator> noResources(1, false);

  const auto lifetimesChanged = lifetimeCalculator.recalculate(graph, coloring);
  scheduler.scheduleEvents(events, graph, lifetimeCalculator.lifetimes(), coloring, nodesChanged, allResources, allResources,
    lifetimesChanged);
  REQUIRE(has_barrier(events, tex, RB_RO_SRV));
  REQUIRE(!has_barrier(events, tex, RB_RO_COPY_SOURCE));

  graph.nodes[node(1)].resourceRequests.front().usage = {dafg::Usage::COPY, dafg::Access::READ_ONLY, dafg::Stage::TRANSFER};
  nodesChanged.assign(3, false);
  const auto lifetimesUnchanged = lifetimeCalculator.recalculate(graph, coloring);
  REQUIRE((lifetimesUnchanged.trueKeys().begin() == lifetimesUnchanged.trueKeys().end()));
  scheduler.scheduleEvents(events, graph, lifetimeCalculator.lifetimes(), coloring, nodesChanged, noResources, allResources,
    lifetimesUnchanged);
  CHECK(has_barrier(events, tex, RB_RO_COPY_SOURCE));
  CHECK(!has_barrier(events, tex, RB_RO_SRV));
}

TEST_CASE("a uav write after a uav write gets a uav flush", "[barrierScheduler]")
{
  const dafg::intermediate::ResourceUsage csUav{dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_WRITE, dafg::Stage::CS};
  const dafg::intermediate::ResourceUsage psUav{dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_WRITE, dafg::Stage::PS};
  const dafg::intermediate::ResourceUsage csSrv{dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_ONLY, dafg::Stage::CS};

  const ResourceBarrier csToCs = dafg::barrier_for_transition(csUav, csUav);
  CHECK((csToCs & RB_FLUSH_UAV) != 0);
  CHECK((csToCs & RB_SOURCE_STAGE_COMPUTE) != 0);
  CHECK((csToCs & RB_STAGE_COMPUTE) != 0);

  const ResourceBarrier csToPs = dafg::barrier_for_transition(csUav, psUav);
  CHECK((csToPs & RB_FLUSH_UAV) != 0);
  CHECK((csToPs & RB_SOURCE_STAGE_COMPUTE) != 0);
  CHECK((csToPs & RB_STAGE_PIXEL) != 0);

  CHECK((dafg::barrier_for_transition(csUav, csSrv) & RB_FLUSH_UAV) == 0);
  CHECK(dafg::barrier_for_transition(csSrv, csSrv) == RB_NONE);
}

TEST_CASE("compute nodes of one color get a uav flush between uav writes", "[barrierScheduler]")
{
  TestRuntime testRuntime{};
  FRAMEMEM_REGION;

  dafg::intermediate::Graph graph;

  TextureResourceDescription texDesc{};
  texDesc.cFlags = TEXFMT_R32F | TEXCF_UNORDERED;
  texDesc.activation = ResourceActivationAction::DISCARD_AS_UAV;
  texDesc.mipLevels = 1;
  texDesc.width = 4;
  texDesc.height = 4;
  dafg::intermediate::ScheduledResource scheduled;
  scheduled.description = ResourceDescription{texDesc};
  scheduled.resourceType = dafg::ResourceType::Texture;
  scheduled.clearStage = dafg::intermediate::ClearStage::None;
  scheduled.clearValue = ResourceClearValue{};
  scheduled.clearFlags = RESOURCE_CLEAR_ALL_CONTENT;
  scheduled.history = dafg::History::No;
  scheduled.autoMipCount = false;
  auto [texIdx, texRes] = graph.resources.appendNew();
  const ResourceIndex tex = texIdx;
  texRes.resource = eastl::move(scheduled);
  texRes.multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
  texRes.frontendResources.push_back(static_cast<dafg::ResNameId>(0));
  graph.resourceNames.emplaceAt(tex, "tex");

  const dafg::intermediate::ResourceUsage uav{dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_WRITE, dafg::Stage::CS};
  const auto addNode = [&graph, tex, uav](uint32_t idx, const char *name) {
    auto *n = graph.nodes.emplaceAt(node(idx));
    n->multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
    n->resourceRequests.push_back({tex, uav, false});
    graph.nodeStates.emplaceAt(node(idx));
    graph.nodeNames.emplaceAt(node(idx), name);
  };
  addNode(0, "first_writer");
  addNode(1, "second_writer");
  graph.nodes[node(1)].predecessors.insert(node(0));
  graph.nodes.emplaceAt(node(2))->predecessors.insert(node(1));
  graph.nodes[node(2)].multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
  graph.nodeStates.emplaceAt(node(2));
  graph.nodeNames.emplaceAt(node(2), "sentinel");

  dafg::PassColoring coloring(3, dafg::PassColor{0});
  coloring[node(2)] = dafg::PassColor{1};

  dafg::ResourceLifetimeCalculator lifetimeCalculator;
  dafg::BarrierScheduler scheduler;
  dafg::BarrierScheduler::EventsCollection events;

  IdIndexedFlags<NodeIndex, framemem_allocator> nodesChanged(3, true);
  IdIndexedFlags<ResourceIndex, framemem_allocator> allResources(1, true);

  const auto lifetimesChanged = lifetimeCalculator.recalculate(graph, coloring);
  scheduler.scheduleEvents(events, graph, lifetimeCalculator.lifetimes(), coloring, nodesChanged, allResources, allResources,
    lifetimesChanged);
  CHECK(has_barrier(events, tex, RB_FLUSH_UAV));

  bool flushBeforeSecondWriter = false;
  for (const auto &event : events[0][node(1)])
    if (const auto *barrier = eastl::get_if<dafg::BarrierScheduler::Event::Barrier>(&event.data))
      flushBeforeSecondWriter |= event.resource == tex && (barrier->barrier & RB_FLUSH_UAV) != 0;
  CHECK(flushBeforeSecondWriter);
}

TEST_CASE("a race fallback barrier skips an empty node slot", "[barrierScheduler]")
{
  TestRuntime testRuntime{};
  FRAMEMEM_REGION;

  dafg::intermediate::Graph graph;

  TextureResourceDescription texDesc{};
  texDesc.cFlags = TEXFMT_R8G8B8A8 | TEXCF_RTARGET;
  texDesc.activation = ResourceActivationAction::DISCARD_AS_RTV_DSV;
  texDesc.mipLevels = 1;
  texDesc.width = 4;
  texDesc.height = 4;
  dafg::intermediate::ScheduledResource scheduled;
  scheduled.description = ResourceDescription{texDesc};
  scheduled.resourceType = dafg::ResourceType::Texture;
  scheduled.clearStage = dafg::intermediate::ClearStage::None;
  scheduled.clearValue = ResourceClearValue{};
  scheduled.clearFlags = RESOURCE_CLEAR_ALL_CONTENT;
  scheduled.history = dafg::History::No;
  scheduled.autoMipCount = false;
  auto [texIdx, texRes] = graph.resources.appendNew();
  const ResourceIndex tex = texIdx;
  texRes.resource = eastl::move(scheduled);
  texRes.multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
  texRes.frontendResources.push_back(static_cast<dafg::ResNameId>(0));
  graph.resourceNames.emplaceAt(tex, "tex");

  const auto addNode = [&graph, tex](uint32_t idx, const char *name, dafg::intermediate::ResourceUsage usage) {
    auto *n = graph.nodes.emplaceAt(node(idx));
    n->multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
    n->resourceRequests.push_back({tex, usage, false});
    graph.nodeStates.emplaceAt(node(idx));
    graph.nodeNames.emplaceAt(node(idx), name);
  };
  addNode(0, "producer", {dafg::Usage::COLOR_ATTACHMENT, dafg::Access::READ_WRITE, dafg::Stage::PS});
  addNode(2, "consumer", {dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_ONLY, dafg::Stage::PS});
  graph.nodes[node(2)].predecessors.insert(node(0));
  graph.nodes.emplaceAt(node(3))->predecessors.insert(node(2));
  graph.nodes[node(3)].multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
  graph.nodeStates.emplaceAt(node(3));
  graph.nodeNames.emplaceAt(node(3), "sentinel");

  dafg::PassColoring coloring(4, dafg::PassColor{0});
  coloring[node(3)] = dafg::PassColor{1};

  dafg::ResourceLifetimeCalculator lifetimeCalculator;
  dafg::BarrierScheduler scheduler;
  dafg::BarrierScheduler::EventsCollection events;

  IdIndexedFlags<NodeIndex, framemem_allocator> nodesChanged(4, true);
  IdIndexedFlags<ResourceIndex, framemem_allocator> allResources(1, true);

  const auto lifetimesChanged = lifetimeCalculator.recalculate(graph, coloring);
  scheduler.scheduleEvents(events, graph, lifetimeCalculator.lifetimes(), coloring, nodesChanged, allResources, allResources,
    lifetimesChanged);
  REQUIRE(has_barrier(events, tex, RB_RO_SRV));

  bool srvBarrierAtConsumer = false;
  for (const auto &event : events[0][node(2)])
    if (const auto *barrier = eastl::get_if<dafg::BarrierScheduler::Event::Barrier>(&event.data))
      srvBarrierAtConsumer |= event.resource == tex && (barrier->barrier & RB_RO_SRV) != 0;
  CHECK(srvBarrierAtConsumer);

  for (const auto &frameEvents : events)
    for (const auto &[nodeIdx, nodeEvents] : frameEvents.enumerate())
      if (!nodeEvents.empty() && eastl::to_underlying(nodeIdx) != graph.nodes.totalKeys())
        CHECK(graph.nodes.isMapped(nodeIdx));
}

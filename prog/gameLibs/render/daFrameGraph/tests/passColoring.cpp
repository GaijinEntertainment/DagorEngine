// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>

#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>
#include <backend/simdBitView.h>
#include <id/idIndexedFlags.h>
#include <catch2/catch_test_macros.hpp>

namespace
{

using dafg::intermediate::NodeIndex;
using dafg::intermediate::ResourceIndex;

NodeIndex node(uint32_t idx) { return static_cast<NodeIndex>(idx); }

dafg::intermediate::RenderPass make_pass(uint32_t resource)
{
  dafg::intermediate::RenderPass result;
  result.colorAttachments.push_back(dafg::intermediate::SubresourceRef{
    .resource = static_cast<ResourceIndex>(resource),
    .mipLevel = 0,
    .layer = 0,
  });
  return result;
}

} // namespace

TEST_CASE("a node does not join a color it reaches through a reused older color", "[passColoring]")
{
  FRAMEMEM_REGION;

  constexpr uint32_t FILLER_COUNT = dafg::SIMD_WORD_BIT_SIZE;
  const auto late = node(1);
  const auto reused = node(2 + FILLER_COUNT);
  const auto joinsReused = node(3 + FILLER_COUNT);
  const auto joinsLate = node(4 + FILLER_COUNT);

  dafg::intermediate::Graph graph;
  graph.nodes.emplaceAt(node(0));
  graph.nodeStates.emplaceAt(node(0));
  for (uint32_t i = 1; i <= eastl::to_underlying(reused); ++i)
  {
    graph.nodes.emplaceAt(node(i))->predecessors.insert(node(0));
    graph.nodeStates.emplaceAt(node(i))->pass = make_pass(i);
  }
  graph.nodes.emplaceAt(joinsReused)->predecessors.insert(late);
  graph.nodeStates.emplaceAt(joinsReused)->pass = make_pass(eastl::to_underlying(reused));
  graph.nodes.emplaceAt(joinsLate)->predecessors.insert(joinsReused);
  graph.nodeStates.emplaceAt(joinsLate)->pass = make_pass(eastl::to_underlying(late));

  IdIndexedFlags<NodeIndex, framemem_allocator> changed(graph.nodes.totalKeys(), true);
  dafg::PassColorer colorer;
  const auto coloring = colorer.performColoring(graph, changed);

  REQUIRE(eastl::to_underlying(coloring[late]) / dafg::SIMD_WORD_BIT_SIZE !=
          eastl::to_underlying(coloring[reused]) / dafg::SIMD_WORD_BIT_SIZE);
  REQUIRE(coloring[joinsReused] == coloring[reused]);
  CHECK(coloring[joinsLate] != coloring[late]);
}

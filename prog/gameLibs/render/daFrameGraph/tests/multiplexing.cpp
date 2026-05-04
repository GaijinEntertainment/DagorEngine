// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testRuntime.h"
#include <catch2/catch_test_macros.hpp>

#include <dag/dag_vector.h>
#include <EASTL/sort.h>


static void check_index(const dafg::multiplexing::Index &idx, uint32_t super_sample, uint32_t sub_sample, uint32_t viewport,
  uint32_t sub_camera)
{
  CHECK(idx.superSample == super_sample);
  CHECK(idx.subSample == sub_sample);
  CHECK(idx.viewport == viewport);
  CHECK(idx.subCamera == sub_camera);
}


// --- Standard use-cases (VR-like viewport multiplexing) ---

TEST_CASE("Non-multiplexed node runs once", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});

  uint32_t execCount = 0;
  auto node = dafg::register_node("single", DAFG_PP_NODE_SRC, [&execCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::None);
    return [&execCount](dafg::multiplexing::Index) { ++execCount; };
  });

  testRuntime.executeGraph();
  CHECK(execCount == 1);
}

TEST_CASE("Fully multiplexed node runs per viewport", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});

  uint32_t execCount = 0;
  dag::Vector<dafg::multiplexing::Index> indices;
  auto node = dafg::register_node("full", DAFG_PP_NODE_SRC, [&execCount, &indices](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    return [&execCount, &indices](dafg::multiplexing::Index idx) {
      ++execCount;
      indices.push_back(idx);
    };
  });

  testRuntime.executeGraph();
  CHECK(execCount == 2);
  REQUIRE(indices.size() == 2);
  // NOTE: currently, iterations of multiplexing are required to be executed in the exact order of indices.
  // This should be relaxed in the future, but for now we RELY on it.
  check_index(indices[0], 0, 0, 0, 0);
  check_index(indices[1], 0, 0, 1, 0);
}

TEST_CASE("Viewport-only node ignores other axes", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{2, 1, 2, 1});

  uint32_t execCount = 0;
  auto node = dafg::register_node("viewportOnly", DAFG_PP_NODE_SRC, [&execCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    return [&execCount](dafg::multiplexing::Index) { ++execCount; };
  });

  testRuntime.executeGraph();
  CHECK(execCount == 2);
}

TEST_CASE("Mixed: shadow + scene pattern", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});

  uint32_t shadowCount = 0;
  uint32_t sceneCount = 0;
  dag::Vector<int> sceneBlobValues;

  auto shadow = dafg::register_node("shadow", DAFG_PP_NODE_SRC, [&shadowCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::None);
    registry.create("shadowData").blob<int>(42);
    return [&shadowCount](dafg::multiplexing::Index) { ++shadowCount; };
  });

  auto scene = dafg::register_node("scene", DAFG_PP_NODE_SRC, [&sceneCount, &sceneBlobValues](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    auto blobHandle = registry.readBlob<int>("shadowData").handle();
    return [&sceneCount, &sceneBlobValues, blobHandle](dafg::multiplexing::Index) {
      ++sceneCount;
      sceneBlobValues.push_back(*blobHandle.get());
    };
  });

  testRuntime.executeGraph();
  CHECK(shadowCount == 1);
  CHECK(sceneCount == 2);
  REQUIRE(sceneBlobValues.size() == 2);
  CHECK(sceneBlobValues[0] == 42);
  CHECK(sceneBlobValues[1] == 42);
}

TEST_CASE("Three-stage pipeline: setup -> render -> composite", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});

  uint32_t setupCount = 0;
  uint32_t renderCount = 0;
  uint32_t compositeCount = 0;

  auto setup = dafg::register_node("setup", DAFG_PP_NODE_SRC, [&setupCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::None);
    auto dataHandle = registry.create("pipelineData").blob<int>(0).handle();
    return [&setupCount, dataHandle](dafg::multiplexing::Index) {
      ++setupCount;
      dataHandle.ref() = 0;
    };
  });

  auto render = dafg::register_node("render", DAFG_PP_NODE_SRC, [&renderCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    auto dataHandle = registry.modifyBlob<int>("pipelineData").handle();
    return [&renderCount, dataHandle](dafg::multiplexing::Index) {
      ++renderCount;
      dataHandle.ref() += 1;
    };
  });

  int finalValue = -1;
  auto composite = dafg::register_node("composite", DAFG_PP_NODE_SRC, [&compositeCount, &finalValue](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::None);
    auto dataHandle = registry.readBlob<int>("pipelineData").handle();
    return [&compositeCount, &finalValue, dataHandle](dafg::multiplexing::Index) {
      ++compositeCount;
      finalValue = *dataHandle.get();
    };
  });

  testRuntime.executeGraph();
  CHECK(setupCount == 1);
  CHECK(renderCount == 2);
  CHECK(compositeCount == 1);
  CHECK(finalValue == 2);
}

TEST_CASE("Correct indices with 3 viewports", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 3, 1});

  dag::Vector<dafg::multiplexing::Index> indices;
  auto node = dafg::register_node("threeVp", DAFG_PP_NODE_SRC, [&indices](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    return [&indices](dafg::multiplexing::Index idx) { indices.push_back(idx); };
  });

  testRuntime.executeGraph();
  REQUIRE(indices.size() == 3);
  check_index(indices[0], 0, 0, 0, 0);
  check_index(indices[1], 0, 0, 1, 0);
  check_index(indices[2], 0, 0, 2, 0);
}

TEST_CASE("Multi-axis multiplexing", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{2, 1, 2, 1});

  dag::Vector<dafg::multiplexing::Index> indices;
  auto node = dafg::register_node("multiAxis", DAFG_PP_NODE_SRC, [&indices](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    return [&indices](dafg::multiplexing::Index idx) { indices.push_back(idx); };
  });

  testRuntime.executeGraph();
  REQUIRE(indices.size() == 4);

  // Collect (superSample, viewport) pairs and sort for order-independent comparison
  dag::Vector<eastl::pair<uint32_t, uint32_t>> pairs;
  for (auto &idx : indices)
    pairs.push_back({idx.superSample, idx.viewport});
  eastl::sort(pairs.begin(), pairs.end());

  CHECK(pairs[0].first == 0);
  CHECK(pairs[0].second == 0);
  CHECK(pairs[1].first == 0);
  CHECK(pairs[1].second == 1);
  CHECK(pairs[2].first == 1);
  CHECK(pairs[2].second == 0);
  CHECK(pairs[3].first == 1);
  CHECK(pairs[3].second == 1);
}


// --- Edge cases: toggling extents ---

TEST_CASE("Enable multiplexing after running without", "[multiplexing]")
{
  TestRuntime testRuntime{};

  uint32_t execCount = 0;
  auto node = dafg::register_node("toggled", DAFG_PP_NODE_SRC, [&execCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    return [&execCount](dafg::multiplexing::Index) { ++execCount; };
  });

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 1, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 1);

  execCount = 0;
  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 2);
}

TEST_CASE("Disable multiplexing after running with", "[multiplexing]")
{
  TestRuntime testRuntime{};

  uint32_t execCount = 0;
  auto node = dafg::register_node("toggled", DAFG_PP_NODE_SRC, [&execCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    return [&execCount](dafg::multiplexing::Index) { ++execCount; };
  });

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 2);

  execCount = 0;
  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 1, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 1);
}

TEST_CASE("Toggle on/off/on", "[multiplexing]")
{
  TestRuntime testRuntime{};

  uint32_t execCount = 0;
  auto node = dafg::register_node("toggled", DAFG_PP_NODE_SRC, [&execCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    return [&execCount](dafg::multiplexing::Index) { ++execCount; };
  });

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 1, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 1);

  execCount = 0;
  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 2);

  execCount = 0;
  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 1, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 1);

  execCount = 0;
  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 2);
}

TEST_CASE("Change extent value (2 -> 3 viewports)", "[multiplexing]")
{
  TestRuntime testRuntime{};

  uint32_t execCount = 0;
  dag::Vector<dafg::multiplexing::Index> indices;
  auto node = dafg::register_node("changing", DAFG_PP_NODE_SRC, [&execCount, &indices](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    return [&execCount, &indices](dafg::multiplexing::Index idx) {
      ++execCount;
      indices.push_back(idx);
    };
  });

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 2);

  execCount = 0;
  indices.clear();
  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 3, 1});
  testRuntime.executeGraph();
  CHECK(execCount == 3);
  REQUIRE(indices.size() == 3);
  check_index(indices[0], 0, 0, 0, 0);
  check_index(indices[1], 0, 0, 1, 0);
  check_index(indices[2], 0, 0, 2, 0);
}


// --- Edge cases: changing modes ---

TEST_CASE("Change default mode between frames", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});

  uint32_t execCount = 0;
  auto node = dafg::register_node("defaultMode", DAFG_PP_NODE_SRC, [&execCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    return [&execCount](dafg::multiplexing::Index) { ++execCount; };
  });

  dafg::set_multiplexing_default_mode(dafg::multiplexing::Mode::FullMultiplex, dafg::multiplexing::Mode::None);
  testRuntime.executeGraph();
  CHECK(execCount == 2);

  execCount = 0;
  dafg::set_multiplexing_default_mode(dafg::multiplexing::Mode::None, dafg::multiplexing::Mode::None);
  testRuntime.executeGraph();
  CHECK(execCount == 1);

  execCount = 0;
  dafg::set_multiplexing_default_mode(dafg::multiplexing::Mode::FullMultiplex, dafg::multiplexing::Mode::None);
  testRuntime.executeGraph();
  CHECK(execCount == 2);
}

TEST_CASE("Re-register node with different mode", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});

  uint32_t execCount = 0;
  auto node = dafg::register_node("reregistered", DAFG_PP_NODE_SRC, [&execCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    return [&execCount](dafg::multiplexing::Index) { ++execCount; };
  });

  testRuntime.executeGraph();
  CHECK(execCount == 2);

  execCount = 0;
  node = dafg::register_node("reregistered", DAFG_PP_NODE_SRC, [&execCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::None);
    return [&execCount](dafg::multiplexing::Index) { ++execCount; };
  });

  testRuntime.executeGraph();
  CHECK(execCount == 1);
}

TEST_CASE("Per-node mode overrides default", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});
  dafg::set_multiplexing_default_mode(dafg::multiplexing::Mode::FullMultiplex, dafg::multiplexing::Mode::None);

  uint32_t defaultCount = 0;
  uint32_t overriddenCount = 0;

  auto nodeA = dafg::register_node("nodeA", DAFG_PP_NODE_SRC, [&defaultCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    // Uses default mode (FullMultiplex)
    return [&defaultCount](dafg::multiplexing::Index) { ++defaultCount; };
  });

  auto nodeB = dafg::register_node("nodeB", DAFG_PP_NODE_SRC, [&overriddenCount](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::None);
    return [&overriddenCount](dafg::multiplexing::Index) { ++overriddenCount; };
  });

  testRuntime.executeGraph();
  CHECK(defaultCount == 2);
  CHECK(overriddenCount == 1);
}


// --- Edge cases: resource interactions ---

TEST_CASE("Non-multiplexed producer, multiplexed consumer", "[multiplexing]")
{
  TestRuntime testRuntime{};

  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{1, 1, 2, 1});

  dag::Vector<int> observedValues;
  auto producer = dafg::register_node("producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::None);
    registry.create("data").blob<int>(42);
  });

  auto consumer = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&observedValues](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    auto dataHandle = registry.readBlob<int>("data").handle();
    return [&observedValues, dataHandle](dafg::multiplexing::Index) { observedValues.push_back(*dataHandle.get()); };
  });

  testRuntime.executeGraph();
  REQUIRE(observedValues.size() == 2);
  CHECK(observedValues[0] == 42);
  CHECK(observedValues[1] == 42);
}

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testRuntime.h"
#include <catch2/catch_test_macros.hpp>
#include <shaders/dag_refinedBlock.h>
#include <EASTL/any.h>

static constexpr const char *TEX_VAR = "refined_block_test_tex";
static constexpr const char *BUF_VAR = "refined_block_test_buf";

TEST_CASE("forBlock sets a texture into a registered block", "[refined block]")
{
  TestRuntime testRuntime{};
  refined_block::clear();

  refined_block::PassBlockHandle passBlock = refined_block::get_global().refineBlock("view").refineBlock("pass");
  const int texVarId = VariableMap::getVariableId(TEX_VAR);

  bool consumerRan = false;

  dafg::NodeHandle registrar = dafg::register_node("registrar", DAFG_PP_NODE_SRC,
    [passBlock](dafg::Registry registry) { registry.registerBlock("test_pass", passBlock); });

  dafg::NodeHandle producer = dafg::register_node("producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.create("block_tex")
      .texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}})
      .atStage(dafg::Stage::PS)
      .forBlock("test_pass", TEX_VAR);
  });

  dafg::NodeHandle consumer = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&consumerRan](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.readTexture("block_tex").atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.useBlock("test_pass");
    return [&consumerRan] { consumerRan = true; };
  });

  testRuntime.executeGraph();

  const auto boundTex = passBlock.get<BaseTexture *>(texVarId);
  REQUIRE(boundTex.has_value());
  CHECK(*boundTex != nullptr);

  CHECK(consumerRan);

  refined_block::clear();
}

TEST_CASE("forBlock sets a buffer into a registered block", "[refined block]")
{
  TestRuntime testRuntime{};
  refined_block::clear();

  refined_block::PassBlockHandle passBlock = refined_block::get_global().refineBlock("view").refineBlock("pass");
  const int bufVarId = VariableMap::getVariableId(BUF_VAR);

  bool consumerRan = false;

  dafg::NodeHandle registrar = dafg::register_node("registrar", DAFG_PP_NODE_SRC,
    [passBlock](dafg::Registry registry) { registry.registerBlock("buf_pass", passBlock); });

  dafg::NodeHandle producer = dafg::register_node("producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.create("block_buf").buffer({4, 3, 0, 0}).atStage(dafg::Stage::CS).forBlock("buf_pass", BUF_VAR);
  });

  dafg::NodeHandle consumer = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&consumerRan](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.read("block_buf").buffer().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.useBlock("buf_pass");
    return [&consumerRan] { consumerRan = true; };
  });

  testRuntime.executeGraph();

  const auto boundBuf = passBlock.get<Sbuffer *>(bufVarId);
  REQUIRE(boundBuf.has_value());
  CHECK(*boundBuf != nullptr);

  CHECK(consumerRan);

  refined_block::clear();
}

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testRuntime.h"
#include <catch2/catch_test_macros.hpp>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_interface_table.h>
#include <drv/3d/dag_consts.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>

namespace
{
const auto DEPTH_TEX = dafg::Texture2dCreateInfo{.creationFlags = TEXFMT_DEPTH32, .resolution = IPoint2{4, 4}};

struct CapturedDsBind
{
  int subpass;
  RenderPassTargetAction action;
};

eastl::vector<CapturedDsBind> g_capturedDsBinds;

void reset_capture() { g_capturedDsBinds.clear(); }

d3d::RenderPass *capture_create_render_pass(const RenderPassDesc &rp_desc)
{
  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
  {
    const auto &bind = rp_desc.binds[i];
    if (bind.slot == RP_SLOT_DEPTH_STENCIL)
      g_capturedDsBinds.push_back({bind.subpass, bind.action});
  }
  return nullptr;
}
} // namespace


TEST_CASE("depthReadTestOnly(string) registers node as a depth reader", "[depth attachment]")
{
  TestRuntime testRuntime{};

  bool tRan = false;

  auto w = dafg::register_node("w", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("depth").texture(DEPTH_TEX);
    registry.requestRenderPass().depth("depth");
  });

  auto r = dafg::register_node("r", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.requestRenderPass().depthReadTestAndSample("depth", {});
    registry.create("marker").blob<int>();
  });

  auto t = dafg::register_node("t", DAFG_PP_NODE_SRC, [&tRan](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.requestRenderPass().depthReadTestOnly("depth");
    registry.read("marker").blob<int>();
    return [&tRan] { tRan = true; };
  });

  testRuntime.executeGraph();
  CHECK(tRan);
}


TEST_CASE("depthReadTestAndSample(string) registers node as a depth reader", "[depth attachment]")
{
  TestRuntime testRuntime{};

  bool tRan = false;

  auto w = dafg::register_node("w", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("depth").texture(DEPTH_TEX);
    registry.requestRenderPass().depth("depth");
  });

  auto r = dafg::register_node("r", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.requestRenderPass().depthReadTestOnly("depth");
    registry.create("marker").blob<int>();
  });

  auto t = dafg::register_node("t", DAFG_PP_NODE_SRC, [&tRan](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.requestRenderPass().depthReadTestAndSample("depth", {});
    registry.read("marker").blob<int>();
    return [&tRan] { tRan = true; };
  });

  testRuntime.executeGraph();
  CHECK(tRan);
}

TEST_CASE("requestRenderPass().depth() binds depth as SUBPASS_WRITE/STORE_WRITE", "[depth attachment]")
{
  D3dInterfaceTable interfaceTableCopy = d3di;
  reset_capture();
  d3di.create_render_pass = capture_create_render_pass;

  TestRuntime testRuntime{};

  auto user = dafg::register_node("user", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("depth").texture(DEPTH_TEX);
    registry.requestRenderPass().depth("depth");
  });

  testRuntime.executeGraph();

  REQUIRE_FALSE(g_capturedDsBinds.empty());
  bool foundMain = false;
  bool foundStore = false;
  for (const auto &bind : g_capturedDsBinds)
  {
    if (bind.subpass != RP_SUBPASS_EXTERNAL_END)
    {
      CHECK((bind.action & RP_TA_SUBPASS_MASK) == RP_TA_SUBPASS_WRITE);
      foundMain = true;
    }
    else
    {
      CHECK((bind.action & RP_TA_STORE_MASK) == RP_TA_STORE_WRITE);
      foundStore = true;
    }
  }
  CHECK(foundMain);
  CHECK(foundStore);

  d3di = interfaceTableCopy;
}


TEST_CASE("depthReadTestOnly preserves SUBPASS_WRITE bind to avoid decompression", "[depth attachment]")
{
  D3dInterfaceTable interfaceTableCopy = d3di;
  reset_capture();
  d3di.create_render_pass = capture_create_render_pass;

  TestRuntime testRuntime{};

  auto writer = dafg::register_node("writer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("depth").texture(DEPTH_TEX);
    registry.requestRenderPass().depth("depth");
  });

  auto reader = dafg::register_node("reader", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.requestRenderPass().depthReadTestOnly("depth");
  });

  testRuntime.executeGraph();

  REQUIRE_FALSE(g_capturedDsBinds.empty());
  bool foundMain = false;
  bool foundStore = false;
  for (const auto &bind : g_capturedDsBinds)
  {
    if (bind.subpass != RP_SUBPASS_EXTERNAL_END)
    {
      CHECK((bind.action & RP_TA_SUBPASS_MASK) == RP_TA_SUBPASS_WRITE);
      foundMain = true;
    }
    else
    {
      CHECK((bind.action & RP_TA_STORE_MASK) == RP_TA_STORE_WRITE);
      foundStore = true;
    }
  }
  CHECK(foundMain);
  CHECK(foundStore);

  d3di = interfaceTableCopy;
}


TEST_CASE("depthReadTestAndSample binds depth as SUBPASS_READ/STORE_NONE", "[depth attachment]")
{
  D3dInterfaceTable interfaceTableCopy = d3di;
  reset_capture();
  d3di.create_render_pass = capture_create_render_pass;

  TestRuntime testRuntime{};

  auto writer = dafg::register_node("writer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("depth").texture(DEPTH_TEX);
    registry.requestRenderPass().depth("depth");
  });

  auto sampler = dafg::register_node("sampler", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.requestRenderPass().depthReadTestAndSample("depth", {});
  });

  testRuntime.executeGraph();

  REQUIRE_FALSE(g_capturedDsBinds.empty());
  bool foundReadMain = false;
  bool foundWriteMain = false;
  bool foundNoneStore = false;
  bool foundWriteStore = false;
  for (const auto &bind : g_capturedDsBinds)
  {
    if (bind.subpass != RP_SUBPASS_EXTERNAL_END)
    {
      const auto subpassAction = bind.action & RP_TA_SUBPASS_MASK;
      if (subpassAction == RP_TA_SUBPASS_READ)
        foundReadMain = true;
      else if (subpassAction == RP_TA_SUBPASS_WRITE)
        foundWriteMain = true;
    }
    else
    {
      const auto storeAction = bind.action & RP_TA_STORE_MASK;
      if (storeAction == RP_TA_STORE_NONE)
        foundNoneStore = true;
      else if (storeAction == RP_TA_STORE_WRITE)
        foundWriteStore = true;
    }
  }
  CHECK(foundReadMain);
  CHECK(foundNoneStore);
  CHECK(foundWriteMain);
  CHECK(foundWriteStore);

  d3di = interfaceTableCopy;
}

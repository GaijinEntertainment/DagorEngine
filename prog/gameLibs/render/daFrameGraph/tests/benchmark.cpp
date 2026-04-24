// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testRuntime.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/resourceCreation.h>
#include <util/dag_convar.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_tex3d.h>
#include <util/dag_string.h>


namespace dafg
{
extern ConVarT<bool, false> recompile_graph;
extern ConVarT<bool, false> verbose;
} // namespace dafg

// Number of modifier passes in each chain. Increase to make the graph larger.
static constexpr int OPAQUE_MODIFIER_COUNT = 160;
static constexpr int TRANSPARENT_MODIFIER_COUNT = 80;
static constexpr int EXTRA_MODIFIER_COUNT = 100;

struct GraphHandles
{
  dag::Vector<dafg::NodeHandle> handles;
  void clear() { handles.clear(); }
};

static const auto COLOR_TEX = dafg::Texture2dCreateInfo{.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}};
static const auto DEPTH_TEX = dafg::Texture2dCreateInfo{.creationFlags = TEXFMT_DEPTH32, .resolution = IPoint2{4, 4}};

static void reg_create(GraphHandles &gh, const char *name, const char *tex_name, const dafg::Texture2dCreateInfo &info)
{
  gh.handles.push_back(dafg::register_node(name, DAFG_PP_NODE_SRC, [tex_name, info](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create(tex_name).texture(info).atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
  }));
}

static void reg_modify(GraphHandles &gh, const char *name, const char *tex_name)
{
  gh.handles.push_back(dafg::register_node(name, DAFG_PP_NODE_SRC, [tex_name](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.modify(tex_name).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
  }));
}

static void reg_read_create(GraphHandles &gh, const char *name, const char *read_tex, const char *create_tex,
  const dafg::Texture2dCreateInfo &info)
{
  gh.handles.push_back(dafg::register_node(name, DAFG_PP_NODE_SRC, [read_tex, create_tex, info](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.read(read_tex).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.create(create_tex).texture(info).atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
  }));
}

static void reg_read_modify(GraphHandles &gh, const char *name, const char *read_tex, const char *modify_tex)
{
  gh.handles.push_back(dafg::register_node(name, DAFG_PP_NODE_SRC, [read_tex, modify_tex](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.read(read_tex).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.modify(modify_tex).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
  }));
}

static void reg_modify_n(GraphHandles &gh, const char *prefix, const char *tex_name, int count)
{
  for (int i = 0; i < count; ++i)
  {
    String name(0, "%s_%d", prefix, i);
    reg_modify(gh, name.c_str(), tex_name);
  }
}

static void build_base_graph(GraphHandles &gh)
{
  // -- Main chain --

  gh.handles.push_back(dafg::register_node("camera_setup", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("camera_data").texture(COLOR_TEX).atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
    registry.create("depth").texture(DEPTH_TEX).atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT);
  }));

  reg_read_create(gh, "gbuffer_create", "camera_data", "gbuffer", COLOR_TEX);

  reg_modify_n(gh, "opaque_draw", "gbuffer", OPAQUE_MODIFIER_COUNT);

  reg_read_create(gh, "downsample_depth_create", "depth", "downsampled_depth", DEPTH_TEX);

  gh.handles.push_back(dafg::register_node("deferred_shading", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.read("gbuffer").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.read("downsampled_depth").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.read("ssr_tex").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.read("ssao_tex").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.read("shadow_atlas").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.read("sky_tex").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.create("frame").texture(COLOR_TEX).atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
  }));

  reg_modify(gh, "copy_frame", "frame");

  reg_read_modify(gh, "transparent_draw_0", "water_tex", "frame");
  reg_modify_n(gh, "transparent_draw", "frame", TRANSPARENT_MODIFIER_COUNT);

  reg_modify(gh, "postfx", "frame");

  gh.handles.push_back(dafg::register_node("present", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.read("frame").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
  }));

  // -- SSR branch (3 nodes) --
  reg_read_create(gh, "ssr_create", "gbuffer", "ssr_tex", COLOR_TEX);
  reg_read_modify(gh, "ssr_process_0", "depth", "ssr_tex");
  reg_modify(gh, "ssr_process_1", "ssr_tex");

  // -- SSAO branch (3 nodes) --
  reg_read_create(gh, "ssao_create", "depth", "ssao_tex", COLOR_TEX);
  reg_modify(gh, "ssao_process_0", "ssao_tex");
  reg_modify(gh, "ssao_process_1", "ssao_tex");

  // -- Shadows/CSM branch (3 nodes) --
  reg_read_create(gh, "csm_create", "camera_data", "shadow_atlas", COLOR_TEX);
  reg_modify(gh, "csm_cascade_0", "shadow_atlas");
  reg_modify(gh, "csm_cascade_1", "shadow_atlas");

  // -- Sky branch (3 nodes) --
  reg_read_create(gh, "sky_create", "camera_data", "sky_tex", COLOR_TEX);
  reg_modify(gh, "sky_process_0", "sky_tex");
  reg_modify(gh, "sky_process_1", "sky_tex");

  // -- Water branch (3 nodes) --
  reg_read_create(gh, "water_create", "downsampled_depth", "water_tex", COLOR_TEX);
  reg_modify(gh, "water_process_0", "water_tex");
  reg_modify(gh, "water_process_1", "water_tex");
}

static void build_extra_modifiers(GraphHandles &gh) { reg_modify_n(gh, "extra_opaque", "gbuffer", EXTRA_MODIFIER_COUNT); }

static void build_extra_branch(GraphHandles &gh)
{
  reg_read_create(gh, "fog_create", "downsampled_depth", "fog_tex", COLOR_TEX);
  reg_modify(gh, "fog_process_0", "fog_tex");
  reg_read_modify(gh, "fog_apply", "fog_tex", "frame");
}

TEST_CASE("compilation benchmarks", "[!benchmark]")
{
  dafg::verbose.set(false);

  // -- 1. From-scratch recompilation --
  SECTION("from-scratch recompilation")
  {
    TestRuntime testRuntime{};
    GraphHandles gh;

    BENCHMARK("from-scratch recompilation")
    {
      gh.clear();
      build_base_graph(gh);
      testRuntime.executeGraph();
    };
  }

  // -- 2. Forced no-op recompile --
  SECTION("forced no-op recompile")
  {
    TestRuntime testRuntime{};
    GraphHandles gh;
    build_base_graph(gh);
    testRuntime.executeGraph();

    BENCHMARK("forced no-op recompile")
    {
      dafg::recompile_graph.set(true);
      testRuntime.executeGraph();
    };
  }

  // -- 3. Toggle extra modifiers --
  SECTION("toggle extra modifiers")
  {
    TestRuntime testRuntime{};
    GraphHandles gh;
    build_base_graph(gh);
    testRuntime.executeGraph();

    GraphHandles extraModifierHandles;
    bool modifiersPresent = false;

    BENCHMARK("toggle extra modifiers")
    {
      if (modifiersPresent)
        extraModifierHandles.clear();
      else
        build_extra_modifiers(extraModifierHandles);
      modifiersPresent = !modifiersPresent;
      testRuntime.executeGraph();
    };
  }

  // -- 4. Toggle extra branch --
  SECTION("toggle extra branch")
  {
    TestRuntime testRuntime{};
    GraphHandles gh;
    build_base_graph(gh);
    testRuntime.executeGraph();

    GraphHandles extraBranchHandles;
    bool branchPresent = false;

    BENCHMARK("toggle extra branch")
    {
      if (branchPresent)
        extraBranchHandles.clear();
      else
        build_extra_branch(extraBranchHandles);
      branchPresent = !branchPresent;
      testRuntime.executeGraph();
    };
  }
}

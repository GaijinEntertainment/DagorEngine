// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testRuntime.h"
#include "conVarGuard.h"
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
extern ConVarT<bool, false> log_recompile_stats;
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

static void reg_wide_spine(dafg::Registry &registry)
{
  registry.modify("wide_spine").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
}

static void reg_wide_pair(GraphHandles &gh, int index, bool read)
{
  const String name(0, read ? "wide_read_%d" : "wide_create_%d", index);
  gh.handles.push_back(dafg::register_node(name.c_str(), DAFG_PP_NODE_SRC, [index, read](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    const String tex(0, "wide_tex_%d", index);
    if (read)
      registry.read(tex.c_str()).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    else
      registry.create(tex.c_str()).texture(COLOR_TEX).atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
    reg_wide_spine(registry);
  }));
}

static void reg_wide_history(GraphHandles &gh, int index, bool read)
{
  const String name(0, read ? "wide_history_read_%d" : "wide_history_create_%d", index);
  gh.handles.push_back(dafg::register_node(name.c_str(), DAFG_PP_NODE_SRC, [index, read](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    const String tex(0, "wide_history_%d", index);
    if (read)
      registry.readTextureHistory(tex.c_str()).atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
    else
      registry.create(tex.c_str())
        .texture(COLOR_TEX)
        .withHistory(dafg::History::DiscardOnFirstFrame)
        .atStage(dafg::Stage::PS)
        .useAs(dafg::Usage::COLOR_ATTACHMENT);
    reg_wide_spine(registry);
  }));
}

static void build_wide_base_graph(GraphHandles &gh, int count)
{
  static constexpr int GAPS[4] = {1, 4, 16, 64};

  gh.handles.push_back(dafg::register_node("wide_spine_create", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("wide_spine").texture(COLOR_TEX).atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
  }));

  const int historyCount = count / 10;
  for (int i = 0; i < historyCount; ++i)
    reg_wide_history(gh, i, false);

  struct WideAction
  {
    int slot = 0;
    int resource = 0;
    bool read = false;
  };
  dag::Vector<WideAction> actions;
  actions.reserve(2 * count);
  for (int i = 0; i < count; ++i)
  {
    actions.push_back({
      .slot = i,
      .resource = i,
      .read = false,
    });
    actions.push_back({
      .slot = eastl::min(i + GAPS[i % 4], count - 1) + 1,
      .resource = i,
      .read = true,
    });
  }
  eastl::stable_sort(actions.begin(), actions.end(), [](const WideAction &a, const WideAction &b) { return a.slot < b.slot; });
  for (const auto &action : actions)
    reg_wide_pair(gh, action.resource, action.read);

  for (int i = 0; i < historyCount; ++i)
    reg_wide_history(gh, i, true);
}

static void build_extra_modifiers(GraphHandles &gh) { reg_modify_n(gh, "extra_opaque", "gbuffer", EXTRA_MODIFIER_COUNT); }

static void build_extra_branch(GraphHandles &gh)
{
  reg_read_create(gh, "fog_create", "downsampled_depth", "fog_tex", COLOR_TEX);
  reg_modify(gh, "fog_process_0", "fog_tex");
  reg_read_modify(gh, "fog_apply", "fog_tex", "frame");
}

struct IconTaskCtx
{
  int state = 0;
};

static void read_ps(dafg::Registry &registry, const char *tex)
{
  registry.read(tex).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE);
}

static void build_icon_subgraph(GraphHandles &gh)
{
  gh.handles.push_back(dafg::register_node("animchar_icon_task_prepare_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.orderMeAfter("camera_setup");
    registry.createBlob<IconTaskCtx>("animchar_icon_request_ctx");
  }));

  gh.handles.push_back(dafg::register_node("animchar_icon_render_gbuffer_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    const auto gbufResolution = registry.getResolution<2>("main_view");
    registry.create("icon_animchar_depth")
      .texture({TEXFMT_DEPTH32 | TEXCF_RTARGET, gbufResolution})
      .clear(make_clear_value(0.f, 0))
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::DEPTH_ATTACHMENT);
    for (int i = 0; i < 3; ++i)
    {
      String name(0, "icon_animchar_gbuf%d", i);
      registry.create(name.c_str())
        .texture({TEXFMT_A16B16G16R16F | TEXCF_RTARGET, gbufResolution})
        .clear(make_clear_value(0.f, 0.f, 0.f, 0.f))
        .atStage(dafg::Stage::PS)
        .useAs(dafg::Usage::COLOR_ATTACHMENT);
    }
    registry.requestRenderPass().color({"icon_animchar_gbuf0", "icon_animchar_gbuf1", "icon_animchar_gbuf2"});
    registry.modifyBlob<IconTaskCtx>("animchar_icon_request_ctx");
    return [] {};
  }));

  gh.handles.push_back(dafg::register_node("animchar_icon_resolve_gbuffer_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.create("icon_animchar_resolved_target")
      .texture({TEXFMT_A16B16G16R16F | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT);
    registry.requestRenderPass().color({"icon_animchar_resolved_target"});
    read_ps(registry, "icon_animchar_gbuf0");
    read_ps(registry, "icon_animchar_gbuf1");
    read_ps(registry, "icon_animchar_gbuf2");
    read_ps(registry, "icon_animchar_depth");
    registry.modifyBlob<IconTaskCtx>("animchar_icon_request_ctx");
    return [] {};
  }));

  gh.handles.push_back(dafg::register_node("animchar_icon_transparents_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto rt = registry.renameTexture("icon_animchar_resolved_target", "icon_animchar_transparent_target");
    registry.requestRenderPass().color({rt});
    read_ps(registry, "icon_animchar_depth");
    registry.modifyBlob<IconTaskCtx>("animchar_icon_request_ctx");
    return [] {};
  }));

  gh.handles.push_back(dafg::register_node("animchar_icon_gui_tonemap_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.create("icon_animchar_postfx_target")
      .texture({TEXFMT_A16B16G16R16F | TEXCF_RTARGET, registry.getResolution<2>("post_fx")})
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT);
    registry.requestRenderPass().color({"icon_animchar_postfx_target"});
    read_ps(registry, "icon_animchar_transparent_target");
    registry.create("animchar_icons_final_samplerstate").blob<uint32_t>(0u);
    registry.modifyBlob<IconTaskCtx>("animchar_icon_request_ctx");
    return [] {};
  }));

  gh.handles.push_back(dafg::register_node("animchar_sharpen_filter_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    read_ps(registry, "icon_animchar_postfx_target");
    registry.modifyBlob<IconTaskCtx>("animchar_icon_request_ctx");
  }));

  gh.handles.push_back(dafg::register_node("animchar_icon_render_finalize_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.orderMeBefore("present");
    registry.renameBlob<IconTaskCtx>("animchar_icon_request_ctx", "animchar_icon_finalized_request_ctx");
  }));
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

  SECTION("add icon subgraph")
  {
    TestRuntime testRuntime{};
    dafg::root().setResolution("main_view", IPoint2{4, 4});
    dafg::root().setResolution("post_fx", IPoint2{4, 4});
    GraphHandles gh;
    build_base_graph(gh);
    testRuntime.executeGraph();

    GraphHandles iconHandles;
    BENCHMARK_ADVANCED("add icon subgraph")(Catch::Benchmark::Chronometer meter)
    {
      iconHandles.clear();
      testRuntime.executeGraph();
      meter.measure([&] {
        build_icon_subgraph(iconHandles);
        testRuntime.executeGraph();
      });
    };
  }

  SECTION("remove icon subgraph")
  {
    TestRuntime testRuntime{};
    dafg::root().setResolution("main_view", IPoint2{4, 4});
    dafg::root().setResolution("post_fx", IPoint2{4, 4});
    GraphHandles gh;
    build_base_graph(gh);
    testRuntime.executeGraph();

    GraphHandles iconHandles;
    BENCHMARK_ADVANCED("remove icon subgraph")(Catch::Benchmark::Chronometer meter)
    {
      build_icon_subgraph(iconHandles);
      testRuntime.executeGraph();
      meter.measure([&] {
        iconHandles.clear();
        testRuntime.executeGraph();
      });
    };
  }

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

static void log_toggle_recompile_stats(TestRuntime &runtime, void (*build_subgraph)(GraphHandles &), int wide_count = 0,
  int toggles = 200)
{
  GraphHandles gh;
  build_base_graph(gh);
  if (wide_count > 0)
    build_wide_base_graph(gh, wide_count);
  runtime.executeGraph();

  const ConVarGuard logStats(dafg::log_recompile_stats, true);
  GraphHandles subgraphHandles;
  for (int i = 0; i < toggles; ++i)
  {
    build_subgraph(subgraphHandles);
    runtime.executeGraph();
    subgraphHandles.clear();
    runtime.executeGraph();
  }
}

TEST_CASE("icon subgraph recompile stats", "[.profile]")
{
  dafg::verbose.set(false);
  TestRuntime testRuntime{};
  dafg::root().setResolution("main_view", IPoint2{4, 4});
  dafg::root().setResolution("post_fx", IPoint2{4, 4});
  log_toggle_recompile_stats(testRuntime, build_icon_subgraph);
}

TEST_CASE("extra branch recompile stats", "[.profile]")
{
  dafg::verbose.set(false);
  TestRuntime testRuntime{};
  log_toggle_recompile_stats(testRuntime, build_extra_branch);
}


TEST_CASE("icon subgraph recompile stats wide 50", "[.profile]")
{
  dafg::verbose.set(false);
  TestRuntime testRuntime{};
  dafg::root().setResolution("main_view", IPoint2{4, 4});
  dafg::root().setResolution("post_fx", IPoint2{4, 4});
  log_toggle_recompile_stats(testRuntime, build_icon_subgraph, 50, 200);
}

TEST_CASE("icon subgraph recompile stats wide 200", "[.profile]")
{
  dafg::verbose.set(false);
  TestRuntime testRuntime{};
  dafg::root().setResolution("main_view", IPoint2{4, 4});
  dafg::root().setResolution("post_fx", IPoint2{4, 4});
  log_toggle_recompile_stats(testRuntime, build_icon_subgraph, 200, 100);
}

TEST_CASE("icon subgraph recompile stats wide 800", "[.profile]")
{
  dafg::verbose.set(false);
  TestRuntime testRuntime{};
  dafg::root().setResolution("main_view", IPoint2{4, 4});
  dafg::root().setResolution("post_fx", IPoint2{4, 4});
  log_toggle_recompile_stats(testRuntime, build_icon_subgraph, 800, 50);
}

TEST_CASE("extra branch recompile stats wide 200", "[.profile]")
{
  dafg::verbose.set(false);
  TestRuntime testRuntime{};
  log_toggle_recompile_stats(testRuntime, build_extra_branch, 200, 100);
}

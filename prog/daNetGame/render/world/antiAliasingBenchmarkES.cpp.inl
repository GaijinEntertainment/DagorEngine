// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/antialiasing.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <stdio.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>
#include <debug/dag_debug.h>
#include <math.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/componentType.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>

#include <render/renderEvent.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/externalResourceWrapper/externalResourceWrapper.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/wrDispatcher.h>

namespace
{
struct AaBenchmarkParams
{
  int subsamples = 0;
  int view = 0; // 0 = off, 1 = error heatmap, 2 = ground truth, 3 = AA output, 4 = temporal mean, 5 = temporal stddev
  float metricExpDecay = 0.9f;
  float heatmapScale = 8.f;
  float heatmapOpacity = 0.75f;
};

constexpr int AA_BENCHMARK_FREQ_BANDS = 7;
constexpr int AA_BENCHMARK_FREQ_LEVELS = AA_BENCHMARK_FREQ_BANDS + 1;

inline int aa_benchmark_groups_for(const IPoint2 &r) { return ((r.x + 7) / 8) * ((r.y + 7) / 8); }

struct AaBenchmarkReadback
{
  RingCPUBufferLock ring;
  int issuedFrame = 0;
  uint32_t readFrame = 0;
};
} // namespace

static float g_aa_benchmark_psnr = -1.f;
static float g_aa_benchmark_gmsd = -1.f;
// per octave, vs ground truth: retained amplitude (~MTF) and relative error amplitude
static float g_aa_benchmark_freq_retained[AA_BENCHMARK_FREQ_BANDS];
static float g_aa_benchmark_freq_error[AA_BENCHMARK_FREQ_BANDS];
static bool g_aa_benchmark_freq_valid = false;

static float aa_benchmark_get_psnr() { return g_aa_benchmark_psnr; }

template <typename Callable>
static void aa_benchmark_params_ecs_query(ecs::EntityManager &manager, Callable c);

// Read once per use; the singleton is cheap to look up and this keeps the node
// execute lambdas free of any component handles.
static AaBenchmarkParams get_aa_benchmark_params()
{
  AaBenchmarkParams params;
  aa_benchmark_params_ecs_query(*g_entity_mgr,
    [&params](int aa_benchmark__subsamples, int aa_benchmark__view, float aa_benchmark__metric_exp_decay,
      float aa_benchmark__heatmap_scale, float aa_benchmark__heatmap_opacity) {
      params.subsamples = aa_benchmark__subsamples;
      params.view = aa_benchmark__view;
      params.metricExpDecay = aa_benchmark__metric_exp_decay;
      params.heatmapScale = aa_benchmark__heatmap_scale;
      params.heatmapOpacity = aa_benchmark__heatmap_opacity;
    });
  return params;
}

static shaders::UniqueOverrideStateId gen_accumulate_blend_override()
{
  // Alpha accumulates the Lanczos weight sum, so it must blend additively too.
  shaders::OverrideState state;
  state.set(shaders::OverrideState::BLEND_SRC_DEST | shaders::OverrideState::BLEND_SRC_DEST_A);
  state.sblend = BLEND_ONE;
  state.dblend = BLEND_ONE;
  state.sblenda = BLEND_ONE;
  state.dblenda = BLEND_ONE;
  return shaders::UniqueOverrideStateId(shaders::overrides::create(state));
}

static void build_aa_benchmark_nodes(dag::Vector<dafg::NodeHandle> &nodes)
{
  const char *inputTextureName = "target_for_transparency";
  const char *groundTruthTextureName = "aa_benchmark_ground_truth";
  const char *temporalStatsTextureName = "aa_benchmark_temporal_stats";
  const char *temporalAgeTextureName = "aa_benchmark_temporal_age";

  g_aa_benchmark_psnr = -1.f;
  g_aa_benchmark_gmsd = -1.f;
  g_aa_benchmark_freq_valid = false;
  nodes.clear();
  auto aaBenchmarkNs = dafg::root() / "aa_benchmark";

  // Sizes for the external metric resources, computed once at build time.
  int dw = 0, dh = 0;
  WRDispatcher::getDisplayResolution(dw, dh);
  const IPoint2 displayRes(max(dw, 1), max(dh, 1));
  const int mseGroups = aa_benchmark_groups_for(displayRes);
  IPoint2 levelRes[AA_BENCHMARK_FREQ_LEVELS];
  {
    IPoint2 lr = displayRes;
    for (int i = 0; i < AA_BENCHMARK_FREQ_LEVELS; ++i)
    {
      levelRes[i] = lr;
      lr = IPoint2(max(lr.x / 2, 1), max(lr.y / 2, 1));
    }
  }

  nodes.push_back(aaBenchmarkNs.registerNode("prepare", DAFG_PP_NODE_SRC, [groundTruthTextureName](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    registry.createTexture2d(groundTruthTextureName, {TEXCF_RTARGET | TEXFMT_A16B16G16R16F, registry.getResolution<2>("display")})
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT)
      .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));
    return [] {};
  }));

  nodes.push_back(
    aaBenchmarkNs.registerNode("accumulate", DAFG_PP_NODE_SRC, [groundTruthTextureName, inputTextureName](dafg::Registry registry) {
      registry.requestRenderPass().color({groundTruthTextureName});
      registry.readTexture(inputTextureName).atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_frame_tex");
      registry.createBlob<OrderingToken>("accumulate_ordering_token");
      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
      auto inputResolutionHndl = registry.getResolution<2>("main_view");
      return [cameraHndl, inputResolutionHndl, jitterVarId = ::get_shader_variable_id("aa_benchmark_jitter", true),
               accumulateBlendId = gen_accumulate_blend_override(), accumulateRenderer = PostFxRenderer("aa_benchmark_accumulate")]() {
        const Point2 jitter = cameraHndl.ref().jitterOffset;
        const IPoint2 inputRes = inputResolutionHndl.get();
        ShaderGlobal::set_float4(jitterVarId, jitter.x, jitter.y, inputRes.x, inputRes.y);
        shaders::overrides::set(accumulateBlendId);
        accumulateRenderer.render();
        shaders::overrides::reset();
      };
    }));

  // MSE / PSNR / GMSD
  {
    FGExternalUniqueBuf partials(dag::buffers::create_ua_sr_structured(sizeof(float) * 4, mseGroups, "aa_benchmark_mse_partials"),
      "mse_partials", aaBenchmarkNs, dafg::multiplexing::Mode::Viewport);
    nodes.push_back(aaBenchmarkNs.registerNode("compare_partial", DAFG_PP_NODE_SRC,
      [buf = eastl::move(partials), groundTruthTextureName, displayRes, mseGroups](dafg::Registry registry) {
        registry.multiplex(dafg::multiplexing::Mode::Viewport);
        (void)buf; // owns mse_partials for the node's lifetime
        registry.readTexture(groundTruthTextureName).atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_ref_tex");
        registry.readTexture("frame_after_aa").atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_frame_tex");
        registry.modify("mse_partials").buffer().atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_mse_partials");
        return [cs = ComputeShader("aa_benchmark_mse_partial"), displayRes, mseGroups,
                 partialsCountVarId = ::get_shader_variable_id("aa_benchmark_mse_partials_count", true)]() {
          ShaderGlobal::set_int(partialsCountVarId, mseGroups);
          cs.dispatchThreads(displayRes.x, displayRes.y, 1);
        };
      }));
  }

  nodes.push_back(
    aaBenchmarkNs.registerNode("compare_reduce", DAFG_PP_NODE_SRC, [groundTruthTextureName, mseGroups](dafg::Registry registry) {
      // The reduce shader normalizes by the ground truth's pixel count (GetDimensions).
      registry.multiplex(dafg::multiplexing::Mode::Viewport);
      registry.readTexture(groundTruthTextureName).atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_ref_tex");
      registry.read("mse_partials").buffer().atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_mse_partials");
      registry.modify("mse_result").buffer().atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_mse_result");
      return [cs = ComputeShader("aa_benchmark_mse_reduce"), mseGroups,
               partialsCountVarId = ::get_shader_variable_id("aa_benchmark_mse_partials_count", true)]() {
        ShaderGlobal::set_int(partialsCountVarId, mseGroups);
        cs.dispatchGroups(1, 1, 1);
      };
    }));

  {
    FGExternalUniqueBuf result(dag::buffers::create_ua_sr_structured(sizeof(float), 2, "aa_benchmark_mse_result"), "mse_result",
      aaBenchmarkNs, dafg::multiplexing::Mode::Viewport);
    Sbuffer *resultBuf = result.getBuf();
    nodes.push_back(aaBenchmarkNs.registerNode("compare_readback", DAFG_PP_NODE_SRC,
      [buf = eastl::move(result), resultBuf](dafg::Registry registry) {
        (void)buf; // owns mse_result for the node's lifetime
        registry.executionHas(dafg::SideEffects::External);
        registry.read("mse_result").buffer().atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::COPY);
        auto rb = eastl::make_unique<AaBenchmarkReadback>();
        rb->ring.init(sizeof(float), 2, 4, "aa_benchmark_psnr_readback", SBCF_UA_STRUCTURED_READBACK, 0, false);
        return [rb = eastl::move(rb), resultBuf]() {
          const AaBenchmarkParams params = get_aa_benchmark_params();
          int stride;
          if (float *data = (float *)rb->ring.lock(stride, rb->readFrame, true))
          {
            const float mse = data[0];
            const float gmsd = data[1];
            // Peak is 1 because the metric works on reinhard-mapped colors.
            const float psnr = mse > 1e-10f ? 10.f * log10f(1.f / mse) : 100.f;
            g_aa_benchmark_psnr =
              g_aa_benchmark_psnr < 0.f ? psnr : g_aa_benchmark_psnr * params.metricExpDecay + psnr * (1.f - params.metricExpDecay);
            g_aa_benchmark_gmsd =
              g_aa_benchmark_gmsd < 0.f ? gmsd : g_aa_benchmark_gmsd * params.metricExpDecay + gmsd * (1.f - params.metricExpDecay);
            rb->ring.unlock();
          }
          if (Sbuffer *target = (Sbuffer *)rb->ring.getNewTarget(rb->issuedFrame))
          {
            resultBuf->copyTo(target);
            rb->ring.startCPUCopy();
          }
        };
      }));
  }

  // Frequency pyramid
  {
    FGExternalUniqueTex level0(
      dag::create_tex(nullptr, levelRes[0].x, levelRes[0].y, TEXFMT_G16R16F | TEXCF_UNORDERED, 1, "aa_benchmark_freq_level0"),
      "freq_level0", aaBenchmarkNs, dafg::multiplexing::Mode::Viewport);
    const IPoint2 lr = levelRes[0];
    nodes.push_back(aaBenchmarkNs.registerNode("freq_luma", DAFG_PP_NODE_SRC,
      [buf = eastl::move(level0), groundTruthTextureName, lr](dafg::Registry registry) {
        (void)buf; // owns freq_level0 for the node's lifetime
        registry.readTexture(groundTruthTextureName).atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_ref_tex");
        registry.readTexture("frame_after_aa").atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_frame_tex");
        registry.modifyTexture("freq_level0").atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_freq_dst");
        return [cs = ComputeShader("aa_benchmark_freq_luma"), lr]() { cs.dispatchThreads(lr.x, lr.y, 1); };
      }));
  }

  for (int k = 1; k < AA_BENCHMARK_FREQ_LEVELS; ++k)
  {
    char texName[48], fgName[32], nodeName[32];
    snprintf(texName, sizeof(texName), "aa_benchmark_freq_level%d", k);
    snprintf(fgName, sizeof(fgName), "freq_level%d", k);
    snprintf(nodeName, sizeof(nodeName), "freq_downsample_%d", k);
    FGExternalUniqueTex level(dag::create_tex(nullptr, levelRes[k].x, levelRes[k].y, TEXFMT_G16R16F | TEXCF_UNORDERED, 1, texName),
      fgName, aaBenchmarkNs, dafg::multiplexing::Mode::Viewport);
    const IPoint2 lr = levelRes[k];
    nodes.push_back(aaBenchmarkNs.registerNode(nodeName, DAFG_PP_NODE_SRC, [buf = eastl::move(level), k, lr](dafg::Registry registry) {
      (void)buf; // owns freq_level<k> for the node's lifetime
      char srcName[32], dstName[32];
      snprintf(srcName, sizeof(srcName), "freq_level%d", k - 1);
      snprintf(dstName, sizeof(dstName), "freq_level%d", k);
      registry.readTexture(srcName).atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_freq_src");
      registry.modifyTexture(dstName).atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_freq_dst");
      return [cs = ComputeShader("aa_benchmark_freq_downsample"), lr]() { cs.dispatchThreads(lr.x, lr.y, 1); };
    }));
  }

  for (int k = 0; k < AA_BENCHMARK_FREQ_BANDS; ++k)
  {
    char bufName[48], fgName[32], nodeName[32];
    snprintf(bufName, sizeof(bufName), "aa_benchmark_freq_partials%d", k);
    snprintf(fgName, sizeof(fgName), "freq_partials%d", k);
    snprintf(nodeName, sizeof(nodeName), "freq_band_%d", k);
    FGExternalUniqueBuf partials(
      dag::buffers::create_ua_sr_structured(sizeof(float) * 4, aa_benchmark_groups_for(levelRes[k]), bufName), fgName, aaBenchmarkNs,
      dafg::multiplexing::Mode::Viewport);
    const IPoint2 lr = levelRes[k];
    nodes.push_back(
      aaBenchmarkNs.registerNode(nodeName, DAFG_PP_NODE_SRC, [buf = eastl::move(partials), k, lr](dafg::Registry registry) {
        (void)buf; // owns freq_partials<k> for the node's lifetime
        registry.multiplex(dafg::multiplexing::Mode::Viewport);
        char fineName[32], coarseName[32], partialsName[32];
        snprintf(fineName, sizeof(fineName), "freq_level%d", k);
        snprintf(coarseName, sizeof(coarseName), "freq_level%d", k + 1);
        snprintf(partialsName, sizeof(partialsName), "freq_partials%d", k);
        registry.readTexture(fineName).atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_freq_src");
        registry.readTexture(coarseName).atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_freq_coarse");
        registry.modify(partialsName).buffer().atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_freq_partials");
        return [cs = ComputeShader("aa_benchmark_freq_band"), lr]() { cs.dispatchThreads(lr.x, lr.y, 1); };
      }));
  }

  for (int k = 0; k < AA_BENCHMARK_FREQ_BANDS; ++k)
  {
    char nodeName[32];
    snprintf(nodeName, sizeof(nodeName), "freq_reduce_%d", k);
    const int groups = aa_benchmark_groups_for(levelRes[k]);
    nodes.push_back(aaBenchmarkNs.registerNode(nodeName, DAFG_PP_NODE_SRC, [k, groups](dafg::Registry registry) {
      char partialsName[32];
      snprintf(partialsName, sizeof(partialsName), "freq_partials%d", k);
      registry.read(partialsName).buffer().atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_freq_partials");
      registry.modify("freq_result").buffer().atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_freq_result");
      return [cs = ComputeShader("aa_benchmark_freq_reduce"), k, groups,
               partialsCountVarId = ::get_shader_variable_id("aa_benchmark_freq_partials_count", true),
               bandVarId = ::get_shader_variable_id("aa_benchmark_freq_band", true)]() {
        ShaderGlobal::set_int(partialsCountVarId, groups);
        ShaderGlobal::set_int(bandVarId, k);
        cs.dispatchGroups(1, 1, 1);
      };
    }));
  }

  {
    FGExternalUniqueBuf result(
      dag::buffers::create_ua_sr_structured(sizeof(float), AA_BENCHMARK_FREQ_BANDS * 3, "aa_benchmark_freq_result"), "freq_result",
      aaBenchmarkNs, dafg::multiplexing::Mode::Viewport);
    Sbuffer *resultBuf = result.getBuf();
    nodes.push_back(
      aaBenchmarkNs.registerNode("freq_readback", DAFG_PP_NODE_SRC, [buf = eastl::move(result), resultBuf](dafg::Registry registry) {
        (void)buf; // owns freq_result for the node's lifetime
        registry.executionHas(dafg::SideEffects::External);
        registry.read("freq_result").buffer().atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::COPY);
        auto rb = eastl::make_unique<AaBenchmarkReadback>();
        rb->ring.init(sizeof(float), AA_BENCHMARK_FREQ_BANDS * 3, 4, "aa_benchmark_freq_readback", SBCF_UA_STRUCTURED_READBACK, 0,
          false);
        return [rb = eastl::move(rb), resultBuf]() {
          const AaBenchmarkParams params = get_aa_benchmark_params();
          int stride;
          if (float *data = (float *)rb->ring.lock(stride, rb->readFrame, true))
          {
            for (int b = 0; b < AA_BENCHMARK_FREQ_BANDS; ++b)
            {
              const float gt = data[b * 3 + 0];
              const float retained = gt > 1e-12f ? sqrtf(data[b * 3 + 1] / gt) : 0.f;
              const float errRel = gt > 1e-12f ? sqrtf(data[b * 3 + 2] / gt) : 0.f;
              if (!g_aa_benchmark_freq_valid)
              {
                g_aa_benchmark_freq_retained[b] = retained;
                g_aa_benchmark_freq_error[b] = errRel;
              }
              else
              {
                g_aa_benchmark_freq_retained[b] =
                  g_aa_benchmark_freq_retained[b] * params.metricExpDecay + retained * (1.f - params.metricExpDecay);
                g_aa_benchmark_freq_error[b] =
                  g_aa_benchmark_freq_error[b] * params.metricExpDecay + errRel * (1.f - params.metricExpDecay);
              }
            }
            g_aa_benchmark_freq_valid = true;
            rb->ring.unlock();
          }
          if (Sbuffer *target = (Sbuffer *)rb->ring.getNewTarget(rb->issuedFrame))
          {
            resultBuf->copyTo(target);
            rb->ring.startCPUCopy();
          }
        };
      }));
  }

  nodes.push_back(aaBenchmarkNs.registerNode("temporal", DAFG_PP_NODE_SRC,
    [temporalStatsTextureName, temporalAgeTextureName, groundTruthTextureName](dafg::Registry registry) {
      // Undermultiplexed like the AA output it reads, so it accumulates once per frame.
      registry.multiplex(dafg::multiplexing::Mode::Viewport);
      const auto displayRes = registry.getResolution<2>("display");
      auto statsHndl = registry.createTexture2d(temporalStatsTextureName, {TEXFMT_A32B32G32R32F | TEXCF_RTARGET, displayRes})
                         .withHistory(dafg::History::ClearZeroOnFirstFrame)
                         .atStage(dafg::Stage::POST_RASTER)
                         .useAs(dafg::Usage::COLOR_ATTACHMENT)
                         .handle();
      // Per-pixel age (frames since this surface was last seen) drives the EWMA weight.
      auto ageHndl = registry.createTexture2d(temporalAgeTextureName, {TEXFMT_R16F | TEXCF_RTARGET, displayRes})
                       .withHistory(dafg::History::ClearZeroOnFirstFrame)
                       .atStage(dafg::Stage::POST_RASTER)
                       .useAs(dafg::Usage::COLOR_ATTACHMENT)
                       .handle();
      registry.readTextureHistory(temporalStatsTextureName).atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_temporal_hist");
      registry.readTextureHistory(temporalAgeTextureName).atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_temporal_age_hist");
      registry.readTexture("frame_after_aa").atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_frame_tex");
      registry.readTexture(groundTruthTextureName).atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_ref_tex");
      registry.readTexture("motion_vecs_after_transparency").atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_motion_tex");
      return [statsHndl, ageHndl, paramsVarId = ::get_shader_variable_id("aa_benchmark_temporal_params", true),
               temporalRenderer = PostFxRenderer("aa_benchmark_temporal")] {
        // Fixed EWMA window; per-pixel age (in the shader) clamps each pixel's weight to it.
        constexpr float window = 128.f;
        ShaderGlobal::set_float4(paramsVarId, window, 0, 0, 0);
        d3d::set_render_target({}, DepthAccess::RW, {{statsHndl.view().getTex2D(), 0, 0}, {ageHndl.view().getTex2D(), 0, 0}});
        temporalRenderer.render();
      };
    }));

  nodes.push_back(aaBenchmarkNs.registerNode("debug_view", DAFG_PP_NODE_SRC,
    [groundTruthTextureName, temporalStatsTextureName](dafg::Registry registry) {
      registry.multiplex(dafg::multiplexing::Mode::Viewport);
      auto debugNs = registry.root() / "debug";
      registry.requestRenderPass().color({debugNs.modifyTexture("target_for_debug")});
      registry.readTexture(groundTruthTextureName).atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_ref_tex");
      registry.readTexture("frame_after_aa").atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_frame_tex");
      registry.readTexture(temporalStatsTextureName).atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_temporal_tex");
      return [paramsVarId = ::get_shader_variable_id("aa_benchmark_heatmap_params", true),
               debugViewRenderer = PostFxRenderer("aa_benchmark_debug_view")] {
        const AaBenchmarkParams params = get_aa_benchmark_params();
        if (params.view == 0)
          return;
        ShaderGlobal::set_float4(paramsVarId, params.heatmapScale, params.heatmapOpacity, params.view, 0);
        debugViewRenderer.render();
      };
    }));
}

ECS_TAG(render)
static void init_aa_benchmark_es(const BeforeLoadLevel &, ecs::EntityManager &manager)
{
  manager.getOrCreateSingletonEntity(ECS_HASH("aa_benchmark"));
}

template <typename Callable>
static void aa_benchmark_nodes_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

static void aa_benchmark_update_nodes(bool enabled)
{
  // Non-creating: the singleton is created by init_aa_benchmark_es / the imgui window.
  aa_benchmark_nodes_ecs_query(*g_entity_mgr, g_entity_mgr->getSingletonEntity(ECS_HASH("aa_benchmark")),
    [enabled](dag::Vector<dafg::NodeHandle> &aa_benchmark__nodes) {
      if (enabled)
        build_aa_benchmark_nodes(aa_benchmark__nodes);
      else
        aa_benchmark__nodes.clear();
    });
}

// Repurposes the sub-sample multiplexing dimension to iterate ground-truth
// accumulation, and (re)creates the benchmark nodes accordingly. Only kicks in
// when neither super- nor sub-sampling is active, since it commandeers that
// dimension; rendering with antialiasing off is not handled (undefined) - this
// is a debug feature. State is process-lifetime, mirroring the once-per-process
// WorldRenderer construction that used to own it.
ECS_TAG(render)
static void aa_benchmark_multiplexing_es(const QueryMultiplexingExtents &evt)
{
  static bool nodesBuilt = false;
  const int subsamples = get_aa_benchmark_params().subsamples;
  const bool active = subsamples > 1 && evt.extents->superSamples == 1 && evt.extents->subSamples == 1;
  if (active != nodesBuilt)
  {
    debug("aa_benchmark: %s (subsamples=%d)", active ? "enabled" : "disabled", subsamples);
    nodesBuilt = active;
    aa_benchmark_update_nodes(active);
  }

  if (active)
    evt.extents->subSamples = static_cast<uint32_t>(subsamples);
}

template <typename Callable>
static void aa_benchmark_imgui_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

static void aaBenchmarkImguiWindow()
{
  aa_benchmark_imgui_ecs_query(*g_entity_mgr, g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("aa_benchmark")),
    [](int &aa_benchmark__subsamples, int &aa_benchmark__view, float &aa_benchmark__heatmap_scale,
      float &aa_benchmark__heatmap_opacity) {
      // Remembers the sample count while the benchmark is disabled (subsamples == 0).
      static int subSamples = 8;
      bool enabled = aa_benchmark__subsamples > 1;
      if (enabled)
        subSamples = aa_benchmark__subsamples;

      ImGui::Checkbox("Accumulate ground truth", &enabled);
      ImGui::SliderInt("Subsamples per frame", &subSamples, 2, render::antialiasing::get_jitter_sequence_length());
      aa_benchmark__subsamples = enabled ? subSamples : 0;

      if (enabled)
      {
        ImGui::TextDisabled("Rendering %d subsamples per frame; the whole frame cost scales accordingly.", aa_benchmark__subsamples);
        const float psnr = aa_benchmark_get_psnr();
        if (psnr >= 0.f)
          ImGui::Text("PSNR vs ground truth: %.2f dB", psnr);
        else
          ImGui::TextDisabled("PSNR: waiting for GPU readback...");
        // GMSD: gradient-domain error, more sensitive to edge aliasing. Lower is better.
        if (g_aa_benchmark_gmsd >= 0.f)
          ImGui::Text("GMSD vs ground truth: %.4f (lower is better)", g_aa_benchmark_gmsd);
        else
          ImGui::TextDisabled("GMSD: waiting for GPU readback...");

        if (g_aa_benchmark_freq_valid)
        {
          ImGui::Separator();
          ImGui::TextDisabled("Per-octave vs ground truth (retained: 1 = kept, <1 = blurred, >1 = added):");
          for (int b = 0; b < AA_BENCHMARK_FREQ_BANDS; ++b)
          {
            // octave feature scale ~2^(b+1) px, finest first
            char overlay[48];
            snprintf(overlay, sizeof(overlay), "~%d px   keep %.2f   err %.2f", 1 << (b + 1), g_aa_benchmark_freq_retained[b],
              g_aa_benchmark_freq_error[b]);
            ImGui::ProgressBar(g_aa_benchmark_freq_retained[b], ImVec2(-1.0f, 0), overlay);
          }
        }
        else
          ImGui::TextDisabled("Frequency bands: waiting for GPU readback...");

        ImGui::Separator();
        const char *viewModes[] = {
          "Off", "Difference heatmap", "Ground truth", "AA output", "Temporal mean error", "Temporal stddev error"};
        ImGui::Combo("Debug view", &aa_benchmark__view, viewModes, IM_ARRAYSIZE(viewModes));
        // Difference (1) and the temporal error views (4, 5) are the alpha-blended heatmaps.
        if (aa_benchmark__view == 1 || aa_benchmark__view == 4 || aa_benchmark__view == 5)
        {
          ImGui::SliderFloat("Heatmap scale", &aa_benchmark__heatmap_scale, 1.f, 128.f, "%.1f", ImGuiSliderFlags_Logarithmic);
          ImGui::SliderFloat("Overlay opacity", &aa_benchmark__heatmap_opacity, 0.f, 1.f);
        }
        else if (aa_benchmark__view == 2 || aa_benchmark__view == 3)
          ImGui::TextDisabled("Reinhard-mapped for display; bypasses postfx color grading.");
      }
      else
        ImGui::TextDisabled("Disabled.");
    });
}

REGISTER_IMGUI_WINDOW("Render", "AA benchmark", aaBenchmarkImguiWindow);

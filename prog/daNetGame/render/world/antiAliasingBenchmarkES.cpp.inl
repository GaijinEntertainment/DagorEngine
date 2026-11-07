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
#include <drv/3d/dag_query.h>
#include <stdio.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui/imgui.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <EASTL/algorithm.h>
#include <EASTL/array.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/string.h>
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
#include <render/cameraParams.h>
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
  int filter = 0; // ground truth filter: 0 = gaussian, 1 = Lanczos-3
  int taps = 3;   // ground truth window, input texels per axis
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

// Dataset dump: <dir>/frame_XXXX/ holds meta.json and one raw texture per stream, rows packed: the input
// and camera of the last subsample at render resolution, the ground truth at display resolution.
namespace
{
enum AaBenchmarkDumpStream
{
  DUMP_INPUT_COLOR,
  DUMP_DEPTH,
  DUMP_MOTION,
  DUMP_GROUND_TRUTH,
  DUMP_STREAMS
};
const char *const DUMP_STREAM_NAMES[DUMP_STREAMS] = {"input_color", "depth", "motion", "ground_truth"};

struct AaBenchmarkDumpFormat
{
  const char *metaName;
  uint32_t texfmt;
  int bytesPerTexel;
};
const AaBenchmarkDumpFormat DUMP_FORMATS[] = {{"R11G11B10_FLOAT", TEXFMT_R11G11B10F, 4},
  {"R16G16B16A16_FLOAT", TEXFMT_A16B16G16R16F, 8}, {"R16G16_FLOAT", TEXFMT_G16R16F, 4}, {"R32_FLOAT", TEXFMT_R32F, 4}};

constexpr int DUMP_READBACK_FRAMES = 3;
constexpr int DUMP_WRITER_QUEUE_FRAMES = 4;

struct AaBenchmarkDumpStatus
{
  eastl::string dir;
  int frames = 0;
  int framesLeft = 0;
  volatile int written = 0;
  volatile int failed = 0;
  volatile int queued = 0;
};
} // namespace

static AaBenchmarkDumpStatus g_aa_benchmark_dump;

static void aa_benchmark_start_dump(const char *root, int frames)
{
  DagorDateTime t;
  ::get_local_time(&t);
  g_aa_benchmark_dump.dir.sprintf("%s/aa_dump_%04d.%02d.%02d_%02d.%02d.%02d", root, t.year, t.month, t.day, t.hour, t.minute,
    t.second);
  g_aa_benchmark_dump.frames = g_aa_benchmark_dump.framesLeft = frames;
  g_aa_benchmark_dump.written = 0;
  g_aa_benchmark_dump.failed = 0;
}

static void aa_benchmark_stop_dump()
{
  g_aa_benchmark_dump.frames -= g_aa_benchmark_dump.framesLeft;
  g_aa_benchmark_dump.framesLeft = 0;
}

static bool aa_benchmark_write_file(const eastl::string &path, const void *data, size_t size)
{
  file_ptr_t fp = df_open(path.c_str(), DF_WRITE | DF_CREATE);
  if (!fp)
    return false;
  const bool ok = df_write(fp, data, int(size)) == int(size);
  df_close(fp);
  return ok;
}

static void aa_benchmark_read_texels(Texture *tex, const IPoint2 &res, int bytes_per_texel, dag::Vector<uint8_t> &texels)
{
  uint8_t *data = nullptr;
  int stride = 0;
  if (!tex->lockimg((void **)&data, stride, 0, TEXLOCK_READ) || !data)
  {
    logerr("aa_benchmark: dump readback of %s failed", tex->getName());
    return;
  }
  const size_t row = size_t(res.x) * bytes_per_texel;
  texels.resize_noinit(row * res.y);
  for (int y = 0; y < res.y; ++y)
    memcpy(texels.data() + y * row, data + size_t(y) * stride, row);
  tex->unlockimg();
}

namespace
{
class AaBenchmarkDumpJob final : public cpujobs::IJob
{
public:
  eastl::string frameDir;
  eastl::string meta;
  dag::Vector<uint8_t> texels[DUMP_STREAMS];

  void doJob() override
  {
    const eastl::string metaPath = frameDir + "/meta.json";
    bool ok = dd_mkpath(metaPath.c_str()) && aa_benchmark_write_file(metaPath, meta.data(), meta.size());
    for (int i = 0; i < DUMP_STREAMS; ++i)
    {
      const eastl::string path(eastl::string::CtorSprintf(), "%s/%s.bin", frameDir.c_str(), DUMP_STREAM_NAMES[i]);
      ok = ok && !texels[i].empty() && aa_benchmark_write_file(path, texels[i].data(), texels[i].size());
      dag::Vector<uint8_t>().swap(texels[i]); // freed before `queued` drops, so the queue cap bounds the memory
    }
    if (!ok)
      logerr("aa_benchmark: failed to write %s", frameDir.c_str());
    interlocked_increment(ok ? g_aa_benchmark_dump.written : g_aa_benchmark_dump.failed);
    interlocked_decrement(g_aa_benchmark_dump.queued);
  }
  void releaseJob() override { delete this; }
  const char *getJobName(bool &) const override { return "aa_benchmark_dump"; }
};

// Waits on a full readback ring or writer queue instead of dropping a frame: a dump holds consecutive frames.
class AaBenchmarkDumper
{
public:
  ~AaBenchmarkDumper()
  {
    drain(issued);
    aa_benchmark_stop_dump();
    release();
  }

  void captureInput(Texture *color, Texture *motion, const IPoint2 &input_res, const CameraParams &camera)
  {
    drain(drained);
    if (g_aa_benchmark_dump.framesLeft == 0)
    {
      if (drained == issued)
        release();
      return;
    }
    if (!slots[0].fence && !create(color, motion))
      return;
    drain(issued - DUMP_READBACK_FRAMES + 1);
    Slot &slot = slots[issued % DUMP_READBACK_FRAMES];
    copy(color, slot, DUMP_INPUT_COLOR, input_res);
    copy(motion, slot, DUMP_MOTION, input_res);
    slot.res[DUMP_DEPTH] = slot.res[DUMP_INPUT_COLOR];
    ShaderGlobal::set_texture(depthDstVarId, slot.tex[DUMP_DEPTH].getTexId());
    depthCopy.dispatchThreads(slot.res[DUMP_DEPTH].x, slot.res[DUMP_DEPTH].y, 1);
    ShaderGlobal::set_texture(depthDstVarId, BAD_TEXTUREID);
    d3d::resource_barrier({slot.tex[DUMP_DEPTH].getTex2D(), RB_RO_COPY_SOURCE, 0, 0});
    slot.jitter = camera.jitterOffset;
    slot.znZfar = Point2(camera.znear, camera.zfar);
    inputCaptured = true;
  }

  void finishFrame(Texture *truth, const IPoint2 &display_res)
  {
    if (!inputCaptured || (!slots[0].tex[DUMP_GROUND_TRUTH] && !createStream(DUMP_GROUND_TRUTH, truth)))
      return;
    inputCaptured = false;
    Slot &slot = slots[issued % DUMP_READBACK_FRAMES];
    copy(truth, slot, DUMP_GROUND_TRUTH, display_res);
    for (UniqueTex &tex : slot.tex)
    {
      int stride; // constraint: lockimg without a pointer starts the copy to CPU memory
      if (tex.getTex2D()->lockimg(nullptr, stride, 0, TEXLOCK_READ | TEXLOCK_NOSYSLOCK))
        tex.getTex2D()->unlockimg();
    }
    d3d::issue_event_query(slot.fence);

    slot.meta.sprintf("{\"frame\": %d, \"jitter\": [%.9g, %.9g], \"zn_zfar\": [%.9g, %.9g], \"textures\": {", issued, slot.jitter.x,
      slot.jitter.y, slot.znZfar.x, slot.znZfar.y);
    for (int i = 0; i < DUMP_STREAMS; ++i)
      slot.meta.append_sprintf("%s\"%s\": {\"format\": \"%s\", \"width\": %d, \"height\": %d}", i ? ", " : "", DUMP_STREAM_NAMES[i],
        formats[i]->metaName, slot.res[i].x, slot.res[i].y);
    slot.meta += "}}";
    issued++;
    g_aa_benchmark_dump.framesLeft--;
  }

private:
  struct Slot
  {
    UniqueTex tex[DUMP_STREAMS];
    IPoint2 res[DUMP_STREAMS];
    Point2 jitter;
    Point2 znZfar;
    d3d::EventQuery *fence = nullptr;
    eastl::string meta;
  };
  eastl::array<Slot, DUMP_READBACK_FRAMES> slots;
  const AaBenchmarkDumpFormat *formats[DUMP_STREAMS] = {};
  ComputeShader depthCopy;
  int depthDstVarId = ::get_shader_variable_id("aa_benchmark_dump_depth_dst", true);
  int issued = 0;
  int drained = 0;
  bool inputCaptured = false;

  bool create(Texture *color, Texture *motion)
  {
    depthCopy = ComputeShader("aa_benchmark_dump_depth", true);
    if (!depthCopy)
      return fail("aa_benchmark_dump_depth shader is missing", "");
    if (!createStream(DUMP_INPUT_COLOR, color) || !createStream(DUMP_DEPTH, color) || !createStream(DUMP_MOTION, motion))
      return false;
    for (Slot &slot : slots)
      slot.fence = d3d::create_event_query();
    return true;
  }

  // The ring copies of stream `i` at the size of `src`; the depth arrives as R32F.
  bool createStream(int i, Texture *src)
  {
    TextureInfo info;
    src->getinfo(info);
    const uint32_t texfmt = i == DUMP_DEPTH ? TEXFMT_R32F : info.cflg & TEXFMT_MASK;
    formats[i] = eastl::find_if(eastl::begin(DUMP_FORMATS), eastl::end(DUMP_FORMATS),
      [texfmt](const AaBenchmarkDumpFormat &f) { return f.texfmt == texfmt; });
    if (formats[i] == eastl::end(DUMP_FORMATS))
      return fail("unsupported texture format of", DUMP_STREAM_NAMES[i]);
    for (int s = 0; s < DUMP_READBACK_FRAMES; ++s)
    {
      char name[64];
      snprintf(name, sizeof(name), "aa_benchmark_dump_%s_%d", DUMP_STREAM_NAMES[i], s);
      // constraint: DX12 keeps the staging copy of a read lock and skips the upload on unlock for render targets only
      const uint32_t usage = TEXCF_RTARGET | (i == DUMP_DEPTH ? TEXCF_UNORDERED : 0);
      slots[s].tex[i] = dag::create_tex(nullptr, info.w, info.h, texfmt | usage, 1, name);
      if (!slots[s].tex[i])
        return fail("cannot create a readback texture for", DUMP_STREAM_NAMES[i]);
    }
    return true;
  }

  static bool fail(const char *why, const char *what)
  {
    logerr("aa_benchmark: dataset dump not started: %s %s", why, what);
    aa_benchmark_stop_dump();
    return false;
  }

  void release()
  {
    for (Slot &slot : slots)
    {
      for (UniqueTex &tex : slot.tex)
        tex.close();
      if (slot.fence)
        d3d::release_event_query(slot.fence);
      slot.fence = nullptr;
    }
    issued = drained = 0;
    inputCaptured = false;
  }

  static void copy(Texture *src, Slot &slot, int i, const IPoint2 &res)
  {
    // constraint: dynamic resolution renders into the top left rect of the texture
    TextureInfo info;
    slot.tex[i].getTex2D()->getinfo(info);
    slot.res[i] = IPoint2(min<int>(res.x, info.w), min<int>(res.y, info.h));
    d3d::update_sub_region(src, 0, 0, 0, 0, slot.res[i].x, slot.res[i].y, 1, slot.tex[i].getTex2D(), 0, 0, 0, 0);
  }

  // Hands the copied frames to the writer, oldest first, waiting for the GPU up to frame `until`.
  void drain(int until)
  {
    static const int writer = cpujobs::create_virtual_job_manager(64 << 10, ~uint64_t(0), "aa_benchmark_dump");
    for (; drained < issued; ++drained)
    {
      const Slot &slot = slots[drained % DUMP_READBACK_FRAMES];
      while (!d3d::get_event_query_status(slot.fence, drained < until))
      {
        if (drained >= until)
          return;
        sleep_msec(1);
      }
      auto *job = new AaBenchmarkDumpJob;
      job->frameDir.sprintf("%s/frame_%04d", g_aa_benchmark_dump.dir.c_str(), drained);
      job->meta = slot.meta;
      for (int i = 0; i < DUMP_STREAMS; ++i)
        aa_benchmark_read_texels(slot.tex[i].getTex2D(), slot.res[i], formats[i]->bytesPerTexel, job->texels[i]);
      while (interlocked_acquire_load(g_aa_benchmark_dump.queued) >= DUMP_WRITER_QUEUE_FRAMES)
        sleep_msec(1);
      interlocked_increment(g_aa_benchmark_dump.queued);
      if (!cpujobs::add_job(writer, job))
      {
        job->doJob();
        delete job;
      }
    }
  }
};
} // namespace

template <typename Callable>
static void aa_benchmark_params_ecs_query(ecs::EntityManager &manager, Callable c);

// Read once per use; the singleton is cheap to look up and this keeps the node
// execute lambdas free of any component handles.
static AaBenchmarkParams get_aa_benchmark_params()
{
  AaBenchmarkParams params;
  aa_benchmark_params_ecs_query(*g_entity_mgr,
    [&params](int aa_benchmark__subsamples, int aa_benchmark__filter, int aa_benchmark__taps, int aa_benchmark__view,
      float aa_benchmark__metric_exp_decay, float aa_benchmark__heatmap_scale, float aa_benchmark__heatmap_opacity) {
      params.subsamples = aa_benchmark__subsamples;
      params.filter = aa_benchmark__filter;
      params.taps = aa_benchmark__taps;
      params.view = aa_benchmark__view;
      params.metricExpDecay = aa_benchmark__metric_exp_decay;
      params.heatmapScale = aa_benchmark__heatmap_scale;
      params.heatmapOpacity = aa_benchmark__heatmap_opacity;
    });
  return params;
}

static shaders::UniqueOverrideStateId gen_accumulate_blend_override()
{
  // Alpha accumulates the weight sum, so it must blend additively too.
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
      auto displayResolutionHndl = registry.getResolution<2>("display");
      return [cameraHndl, inputResolutionHndl, jitterVarId = ::get_shader_variable_id("aa_benchmark_jitter", true),
               displayResolutionHndl, displayResVarId = ::get_shader_variable_id("aa_benchmark_display_res", true),
               filterVarId = ::get_shader_variable_id("aa_benchmark_filter", true),
               tapsVarId = ::get_shader_variable_id("aa_benchmark_taps", true), accumulateBlendId = gen_accumulate_blend_override(),
               accumulateRenderer = PostFxRenderer("aa_benchmark_accumulate")]() {
        const Point2 jitter = cameraHndl.ref().jitterOffset;
        const IPoint2 inputRes = inputResolutionHndl.get();
        const IPoint2 displayRes = displayResolutionHndl.get();
        const AaBenchmarkParams params = get_aa_benchmark_params();
        ShaderGlobal::set_float4(jitterVarId, jitter.x, jitter.y, inputRes.x, inputRes.y);
        ShaderGlobal::set_float4(displayResVarId, displayRes.x, displayRes.y, 0, 0);
        ShaderGlobal::set_int(filterVarId, params.filter);
        ShaderGlobal::set_int(tapsVarId, params.taps);
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
        registry.readTexture("frame_after_neural_rendering").atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_frame_tex");
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
        registry.readTexture("frame_after_neural_rendering").atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_frame_tex");
        registry.modifyTexture("freq_level0").atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_freq_dst");
        registry.dispatchThreads("aa_benchmark_freq_luma").x(lr.x).y(lr.y).z(1);
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
      registry.dispatchThreads("aa_benchmark_freq_downsample").x(lr.x).y(lr.y).z(1);
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
        registry.dispatchThreads("aa_benchmark_freq_band").x(lr.x).y(lr.y).z(1);
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
      registry.readTexture("frame_after_neural_rendering").atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_frame_tex");
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
      registry.readTexture("frame_after_neural_rendering").atStage(dafg::Stage::PS).bindToShaderVar("aa_benchmark_frame_tex");
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

  const auto dumper = eastl::make_shared<AaBenchmarkDumper>();
  const auto readCopySource = [](dafg::Registry &registry, const char *name) {
    return registry.readTexture(name).atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::COPY).handle();
  };

  nodes.push_back(
    aaBenchmarkNs.registerNode("dump_input", DAFG_PP_NODE_SRC, [inputTextureName, dumper, readCopySource](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      registry.createBlob<OrderingToken>("dump_input_token");
      auto colorHndl = readCopySource(registry, inputTextureName);
      auto motionHndl = readCopySource(registry, "motion_vecs_after_transparency");
      registry.readTexture("depth_after_transparency").atStage(dafg::Stage::CS).bindToShaderVar("aa_benchmark_dump_depth_src");
      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
      auto inputResolutionHndl = registry.getResolution<2>("main_view");
      return [colorHndl, motionHndl, cameraHndl, inputResolutionHndl, dumper](dafg::multiplexing::Index index) {
        // constraint: the subsamples share the frame targets and nothing orders an earlier instance before
        // the next subsample, so only the last instance reads the input that matches its own camera
        if (index.subSample == get_aa_benchmark_params().subsamples - 1)
          dumper->captureInput(colorHndl.view().getTex2D(), motionHndl.view().getTex2D(), inputResolutionHndl.get(), cameraHndl.ref());
      };
    }));

  nodes.push_back(
    aaBenchmarkNs.registerNode("dump", DAFG_PP_NODE_SRC, [groundTruthTextureName, dumper, readCopySource](dafg::Registry registry) {
      registry.multiplex(dafg::multiplexing::Mode::Viewport);
      registry.executionHas(dafg::SideEffects::External);
      registry.readBlob<OrderingToken>("dump_input_token");
      auto truthHndl = readCopySource(registry, groundTruthTextureName);
      auto displayResolutionHndl = registry.getResolution<2>("display");
      return
        [truthHndl, displayResolutionHndl, dumper] { dumper->finishFrame(truthHndl.view().getTex2D(), displayResolutionHndl.get()); };
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
// is a debug feature. Also excluded during camera-in-camera: tsr's Viewport-only
// reads then fail daFG's multiplexing check against a raised subSamples extent.
// State is process-lifetime, mirroring the once-per-process WorldRenderer
// construction that used to own it.
ECS_TAG(render)
static void aa_benchmark_multiplexing_es(const QueryMultiplexingExtents &evt)
{
  static bool nodesBuilt = false;
  const int subsamples = get_aa_benchmark_params().subsamples;
  const bool active = subsamples > 1 && evt.extents->superSamples == 1 && evt.extents->subSamples == 1 && evt.extents->subCameras == 1;
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
    [](int &aa_benchmark__subsamples, int &aa_benchmark__filter, int &aa_benchmark__taps, int &aa_benchmark__view,
      float &aa_benchmark__heatmap_scale, float &aa_benchmark__heatmap_opacity, ecs::string &aa_benchmark__dump_path,
      int &aa_benchmark__dump_frames) {
      // Remembers the sample count while the benchmark is disabled (subsamples == 0).
      static int subSamples = 8;
      bool enabled = aa_benchmark__subsamples > 1;
      if (enabled)
        subSamples = aa_benchmark__subsamples;

      ImGui::Checkbox("Accumulate ground truth", &enabled);
      ImGui::SliderInt("Subsamples per frame", &subSamples, 2, render::antialiasing::get_jitter_sequence_length());
      aa_benchmark__subsamples = enabled ? subSamples : 0;
      // both filter in luma-tonemapped YCoCg, in display pixels
      const char *filters[] = {"Gaussian", "Lanczos-3"};
      ImGui::Combo("Ground truth filter", &aa_benchmark__filter, filters, IM_ARRAYSIZE(filters));
      ImGui::SliderInt("Ground truth taps per axis", &aa_benchmark__taps, 1, 8);

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

        ImGui::Separator();
        ImGuiDagor::InputText("Dataset folder", &aa_benchmark__dump_path);
        ImGui::InputInt("Dataset frames", &aa_benchmark__dump_frames);
        aa_benchmark__dump_frames = max(aa_benchmark__dump_frames, 1);
        const AaBenchmarkDumpStatus &dump = g_aa_benchmark_dump;
        const int written = interlocked_acquire_load(dump.written);
        const int failed = interlocked_acquire_load(dump.failed);
        if (written + failed < dump.frames)
          ImGui::Text("Dumping: %d / %d frames written to %s", written, dump.frames, dump.dir.c_str());
        else
        {
          ImGui::BeginDisabled(aa_benchmark__dump_path.empty());
          if (ImGui::Button("Dump dataset"))
            aa_benchmark_start_dump(aa_benchmark__dump_path.c_str(), aa_benchmark__dump_frames);
          ImGui::EndDisabled();
          if (dump.frames > 0)
            ImGui::TextDisabled("Last dump: %d frames in %s", written, dump.dir.c_str());
        }
        if (failed > 0)
          ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%d frames failed to write, see the log", failed);
      }
      else
        ImGui::TextDisabled("Disabled.");
    });
}

REGISTER_IMGUI_WINDOW("Render", "AA benchmark", aaBenchmarkImguiWindow);

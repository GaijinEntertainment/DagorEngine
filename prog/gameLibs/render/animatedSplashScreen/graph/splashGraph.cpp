// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splash_graph_api.h"

#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point4.h>
#include <math/dag_mathBase.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_barrier.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <render/noiseTex.h>
#include <debug/dag_debug.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <math.h>

// u0..u7: dx11 caps unordered access views at 8 and is the lowest of our
// targets, so a graph that stays inside it loads everywhere
static constexpr int MAX_CS_OUTPUT_REG = 7;

// A graph scene marches per pixel, so its cost follows the pixels it renders and
// a 4K host would pay four times a 1080p one for a picture the loading screen
// does not need. scale:r resources come off a base capped here; the output node
// keeps drawing at the host size and upscales through the clamp sampler. A graph
// that wants the host size sets max_scaled_size:ip2=0, 0.
static constexpr IPoint2 DEFAULT_MAX_SCALED_SIZE{1920, 1080};

// the fixed slot map of loading_splash_common.dshl, by name like the hosts do
#define SPLASH_GRAPH_CONST_LIST                               \
  VAR(loading_splash_noise_64_tex_l8)                         \
  VAR(loading_splash_noise_128_tex_hash)                      \
  VAR(loading_splash_iGlobalTime_iResolution_iPaperWhiteNits) \
  VAR(loading_splash_iSeed)                                   \
  VAR(loading_splash_exit)                                    \
  VAR(loading_splash_iFrame_history_iPrevTime)                \
  VAR(loading_splash_node_params)                             \
  VAR(loading_splash_linear_clamp)                            \
  VAR(loading_splash_point_clamp)                             \
  VAR(loading_splash_title_logo)
#define VAR(a) static ShaderVariableInfo a##_const_no{#a "_const_no", true};
SPLASH_GRAPH_CONST_LIST
#undef VAR

namespace
{

struct FormatName
{
  const char *name;
  uint32_t fmt;
};
// the TEXFMT names a graph may ask for; parse_tex_format asserts on a typo
static const FormatName format_names[] = {{"A16B16G16R16F", TEXFMT_A16B16G16R16F}, {"A32B32G32R32F", TEXFMT_A32B32G32R32F},
  {"R11G11B10F", TEXFMT_R11G11B10F}, {"G16R16F", TEXFMT_G16R16F}, {"G32R32F", TEXFMT_G32R32F}, {"R16F", TEXFMT_R16F},
  {"R32F", TEXFMT_R32F}, {"A8R8G8B8", TEXFMT_A8R8G8B8}, {"R8G8B8A8", TEXFMT_R8G8B8A8}, {"A2B10G10R10", TEXFMT_A2B10G10R10},
  {"R8G8", TEXFMT_R8G8}, {"R8", TEXFMT_R8}, {"L16", TEXFMT_L16}, {"R16UI", TEXFMT_R16UI}, {"R32UI", TEXFMT_R32UI},
  {"R32G32UI", TEXFMT_R32G32UI}};

static bool parse_format(const char *name, uint32_t &fmt)
{
  for (const FormatName &f : format_names)
    if (strcmp(name, f.name) == 0)
    {
      fmt = f.fmt;
      return true;
    }
  return false;
}

struct Resource
{
  eastl::string name;
  bool isBuf = false;
  bool fixed = false; // size or sizeDwords given; else scaled from the target
  IPoint2 size = IPoint2(0, 0);
  float scale = 1.f;
  int sizeDwords = 0;
  float scaleDwords = 0.f;
  uint32_t format = TEXFMT_A16B16G16R16F;
  bool history = false;
  bool writtenByPs = false, writtenByCs = false;
  UniqueTex tex[2]; // [1] = the history partner
  UniqueBuf buf;
  IPoint2 curSize = IPoint2(0, 0); // allocated size; buffers keep dwords in x
  int cur = 0;                     // history pair: the surface this frame writes
};

struct Binding
{
  int res = -1;
  int reg = 0;
  bool history = false; // input: the surface the previous frame finished
};

struct Node
{
  eastl::string shader;
  bool compute = false;
  eastl::vector<Binding> inputs, outputs;
  bool fixedDispatch = false;
  IPoint3 dispatch = IPoint3(1, 1, 1);
  int dispatchFrom = -1;              // resource that sizes the thread grid; -1 = the first output
  Point4 params = Point4(0, 0, 0, 0); // params:p4, the node's own knobs at c4
  PostFxRenderer ps;
  eastl::unique_ptr<ComputeShaderElement> cs;
};

class SplashGraphImpl final : public SplashGraph
{
public:
  SplashGraphImpl() : noise64(&init_and_get_l8_64_noise()), noise128(&init_and_get_hash_128_noise()) {}
  ~SplashGraphImpl() override
  {
    release_l8_64_noise();
    release_hash_128_noise();
  }

  bool init(const DataBlock &blk, const char *graph_path, const char *scene_name, const char *output_shader_name);
  bool draw(const Frame &frame) override;
  float slowExitSeconds() const override { return slowExitSec; }
  float rushExitSeconds() const override { return rushExitSec; }
  bool opensOntoGame() const override { return opensOntoGameFlag; }

private:
  bool fail(const char *what)
  {
    logerr("[splash] graph %s: %s", path.c_str(), what);
    return false;
  }
  template <typename... Args>
  bool failf(const char *fmt, Args... args)
  {
    return fail(eastl::string(eastl::string::CtorSprintf{}, fmt, args...).c_str());
  }
  int findResource(const char *name) const;
  bool parseResource(const DataBlock &b);
  bool parseNode(const DataBlock &b, Node &n, bool is_output, bool is_preload);
  bool createResources(int w, int h);
  IPoint2 scaledBase(int w, int h) const;
  IPoint2 drivingSize(const Node &n) const;
  void bindFixed(unsigned stage, const Frame &f, IPoint2 out_size, const Point4 &params, bool game_is_under) const;
  void bindInputs(unsigned stage, const Node &n) const;
  void unbindInputs(unsigned stage, const Node &n) const;
  void drawPixelNode(Node &n, const Frame &f, IPoint2 out_size, bool game_is_under);
  void runNode(Node &n, const Frame &f);

  eastl::string path;
  eastl::vector<Resource> resources;
  eastl::vector<Node> preloadNodes, frameNodes;
  Node outputNode;
  const SharedTexWithShaderVar *noise64, *noise128;
  d3d::SamplerHandle noiseDefault = d3d::INVALID_SAMPLER_HANDLE;
  int targetW = 0, targetH = 0;
  IPoint2 maxScaledSize = DEFAULT_MAX_SCALED_SIZE;
  float slowExitSec = 0.f, rushExitSec = 0.f;
  bool opensOntoGameFlag = false;
  unsigned resetCounter = 0;
  bool preloadDone = false;
  int frameIndex = 0; // drawn frames since the resources were created; 0 = no history, no previous time
  float prevTime = 0;
};

int SplashGraphImpl::findResource(const char *name) const
{
  for (int i = 0; i < (int)resources.size(); i++)
    if (resources[i].name == name)
      return i;
  return -1;
}

bool SplashGraphImpl::parseResource(const DataBlock &b)
{
  Resource r;
  r.name = b.getStr("name", "");
  if (r.name.empty())
    return fail("shader_resource without a name");
  if (findResource(r.name.c_str()) >= 0)
    return failf("resource '%s' declared twice", r.name.c_str());
  const char *type = b.getStr("type", "tex2d");
  r.isBuf = strcmp(type, "buf") == 0;
  if (!r.isBuf && strcmp(type, "tex2d") != 0)
    return failf("resource '%s': unknown type '%s'", r.name.c_str(), type);
  const char *fixedKey = r.isBuf ? "sizeDwords" : "size";
  const char *scaleKey = r.isBuf ? "scaleDwords" : "scale";
  r.fixed = b.paramExists(fixedKey);
  if (r.fixed == b.paramExists(scaleKey))
    return failf("resource '%s': give exactly one of %s and %s", r.name.c_str(), fixedKey, scaleKey);
  if (r.isBuf)
  {
    r.sizeDwords = b.getInt("sizeDwords", 0);
    r.scaleDwords = b.getReal("scaleDwords", 0.f);
    if (r.fixed ? r.sizeDwords < 1 : r.scaleDwords <= 0.f)
      return failf("resource '%s': bad buffer size", r.name.c_str());
    if (b.paramExists("format") || b.paramExists("history"))
      return failf("resource '%s': format and history are tex2d only", r.name.c_str());
  }
  else
  {
    r.size = b.getIPoint2("size", IPoint2(0, 0));
    r.scale = b.getReal("scale", 1.f);
    if (r.fixed ? (r.size.x < 1 || r.size.y < 1) : r.scale <= 0.f)
      return failf("resource '%s': bad texture size", r.name.c_str());
    const char *fmt = b.getStr("format", nullptr);
    if (fmt && !parse_format(fmt, r.format))
      return failf("resource '%s': unknown format '%s'", r.name.c_str(), fmt);
    r.history = b.getBool("history", false);
  }
  resources.push_back(eastl::move(r));
  return true;
}

bool SplashGraphImpl::parseNode(const DataBlock &b, Node &n, bool is_output, bool is_preload)
{
  const char *what = is_output ? "output_node" : b.getStr("shader", "");
  n.shader = b.getStr("shader", "");
  if (n.shader.empty())
    return fail(is_output ? "output_node without shader:t, its composite family" : "shader_node without a shader name");
  n.compute = b.getBool("compute", false);
  if (is_output && n.compute)
    return fail("output_node draws with a pixel shader");

  n.params = b.getPoint4("params", Point4(0, 0, 0, 0));
  const int minInputReg = is_output ? 3 : 2; // t0, t1 = noise; t2 = the title logo on the output node
  for (int i = 0; i < b.blockCount(); i++)
  {
    const DataBlock &bb = *b.getBlock(i);
    const char *kind = bb.getBlockName();
    const bool isHistory = strcmp(kind, "history_resource") == 0;
    const bool isInput = isHistory || strcmp(kind, "resource") == 0;
    if (!isInput && (is_output || strcmp(kind, "output") != 0))
      return failf("%s: unknown block '%s'", what, kind);
    Binding bind;
    const char *resName = bb.getStr("name", "");
    bind.res = findResource(resName);
    if (bind.res < 0)
      return failf("%s: unknown resource '%s'", what, resName);
    const Resource &r = resources[bind.res];
    bind.reg = bb.getInt("reg", -1);
    if (isInput)
    {
      bind.history = isHistory;
      if (bind.reg < minInputReg)
        return failf("%s: input '%s': t%d is reserved, inputs start at t%d", what, resName, bind.reg, minInputReg);
      if (isHistory && !r.history)
        return failf("%s: '%s' is not a history:b=yes texture", what, resName);
      if (isHistory && is_preload)
        return failf("%s: preload may not read history texture '%s', no frame has written it", what, resName);
      for (const Binding &o : n.inputs)
        if (o.reg == bind.reg)
          return failf("%s: two inputs at t%d", what, bind.reg);
      n.inputs.push_back(bind);
    }
    else
    {
      // ps outputs fit runNode's four render targets; a cs output past the
      // lowest UAV slot count of the drivers we ship on fails inside the
      // driver at dispatch, where the executor cannot see it and a later node
      // would read the stale surface
      if (bind.reg < 0 || bind.reg > (n.compute ? MAX_CS_OUTPUT_REG : 3))
        return failf("%s: output '%s': bad reg %d", what, resName, bind.reg);
      if (r.isBuf && !n.compute)
        return failf("%s: buffer output '%s' needs a compute node", what, resName);
      if (is_preload && r.history)
        return failf("%s: preload may not write history texture '%s'", what, resName);
      for (const Binding &o : n.outputs)
      {
        if (o.reg == bind.reg)
          return failf("%s: two outputs at %d", what, bind.reg);
        if (o.res == bind.res)
          return failf("%s: output '%s' twice", what, resName);
        const Resource &ro = resources[o.res];
        if (!n.compute && (ro.fixed != r.fixed || (r.fixed ? ro.size != r.size : ro.scale != r.scale)))
          return failf("%s: outputs '%s' and '%s' differ in size", what, ro.name.c_str(), resName);
      }
      n.outputs.push_back(bind);
    }
  }
  for (const Binding &o : n.outputs)
    for (const Binding &in : n.inputs)
      if (in.res == o.res && !in.history)
        return failf("%s: '%s' is both read and written", what, resources[o.res].name.c_str());
  // the output node is a pixel node too: a stray cs key on it fails the same way
  if (!n.compute && (b.paramExists("dispatch") || b.paramExists("dispatchFrom")))
    return failf("%s: dispatch keys need compute:b=yes", what);
  if (is_output)
    return true;

  for (const Binding &o : n.outputs)
    (n.compute ? resources[o.res].writtenByCs : resources[o.res].writtenByPs) = true;
  if (!n.compute)
  {
    unsigned rtMask = 0;
    for (const Binding &o : n.outputs)
      rtMask |= 1u << o.reg;
    if (n.outputs.empty() || rtMask != (1u << n.outputs.size()) - 1)
      return failf("%s: a pixel node writes render targets 0..n-1", what);
    n.ps.init(n.shader.c_str(), /*is_optional*/ true);
    if (!n.ps.getMat())
      return failf("%s: shader not in the dump", what);
    return true;
  }
  // a compute node with no output would dispatch into nothing every frame
  if (n.outputs.empty())
    return failf("%s: a compute node writes at least one output", what);
  n.fixedDispatch = b.paramExists("dispatch");
  n.dispatch = b.getIPoint3("dispatch", IPoint3(1, 1, 1));
  if (n.fixedDispatch && (n.dispatch.x < 1 || n.dispatch.y < 1 || n.dispatch.z < 1))
    return failf("%s: bad dispatch", what);
  if (const char *from = b.getStr("dispatchFrom", nullptr))
  {
    n.dispatchFrom = findResource(from);
    if (n.dispatchFrom < 0)
      return failf("%s: dispatchFrom: unknown resource '%s'", what, from);
  }
  if (n.fixedDispatch && n.dispatchFrom >= 0)
    return failf("%s: dispatch and dispatchFrom together", what);
  n.cs.reset(new_compute_shader(n.shader.c_str(), /*optional*/ true));
  if (!n.cs)
    return failf("%s: shader not in the dump", what);
  return true;
}

// what the host looks up for a scene's single-shader path: loading_splash_<scene>[_N]
static bool is_scene_shader_name(const eastl::string &name, const eastl::string &scene_family)
{
  if (name.find(scene_family) != 0)
    return false;
  const char *rest = name.c_str() + scene_family.size();
  if (!*rest)
    return true;
  if (*rest++ != '_' || !*rest)
    return false;
  for (; *rest; ++rest)
    if (*rest < '0' || *rest > '9')
      return false;
  return true;
}

bool SplashGraphImpl::init(const DataBlock &blk, const char *graph_path, const char *scene_name, const char *output_shader_name)
{
  path = graph_path;
  // one dshl block declares every slot: the three newest names prove the whole map
  if (!bool(loading_splash_iFrame_history_iPrevTime_const_no) || !bool(loading_splash_linear_clamp_const_no) ||
      !bool(loading_splash_point_clamp_const_no))
    return fail("the shader dump lacks the loading_splash slot map with iFrame and the clamp samplers (loading_splash_common.dshl)");

  maxScaledSize = blk.getIPoint2("max_scaled_size", DEFAULT_MAX_SCALED_SIZE);
  // 0, 0 is the lift and the only one: half a cap leaves the other axis free
  // and reads as a cap that is not there, and a negative pair says nothing
  if ((maxScaledSize.x != 0 || maxScaledSize.y != 0) && (maxScaledSize.x < 1 || maxScaledSize.y < 1))
    return failf("max_scaled_size: two positive sizes, or 0, 0 to lift the cap; got %d, %d", maxScaledSize.x, maxScaledSize.y);

  slowExitSec = blk.getReal("slow_exit_seconds", 0.f);
  rushExitSec = blk.getReal("rush_exit_seconds", 0.f);
  if (slowExitSec < 0.f || rushExitSec < 0.f)
    return failf("exit seconds must not be negative; got slow %g, rush %g", slowExitSec, rushExitSec);
  opensOntoGameFlag = blk.getBool("opens_onto_game", false);
  if (opensOntoGameFlag && rushExitSec <= 0.f)
    return fail("opens_onto_game needs a rush to reveal the game in");

  // resources first: nodes reference them by name in any order
  for (int i = 0; i < blk.blockCount(); i++)
    if (strcmp(blk.getBlock(i)->getBlockName(), "shader_resource") == 0 && !parseResource(*blk.getBlock(i)))
      return false;
  int outputCount = 0;
  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock &b = *blk.getBlock(i);
    const char *kind = b.getBlockName();
    if (strcmp(kind, "shader_resource") == 0)
      continue;
    if (strcmp(kind, "preload") == 0)
    {
      for (int j = 0; j < b.blockCount(); j++)
      {
        const DataBlock &nb = *b.getBlock(j);
        if (strcmp(nb.getBlockName(), "shader_node") != 0)
          return failf("preload: unknown block '%s'", nb.getBlockName());
        preloadNodes.emplace_back();
        if (!parseNode(nb, preloadNodes.back(), false, true))
          return false;
      }
    }
    else if (strcmp(kind, "shader_node") == 0)
    {
      frameNodes.emplace_back();
      if (!parseNode(b, frameNodes.back(), false, false))
        return false;
    }
    else if (strcmp(kind, "output_node") == 0)
    {
      if (++outputCount > 1)
        return fail("more than one output_node");
      if (!parseNode(b, outputNode, true, false))
        return false;
    }
    else
      return failf("unknown block '%s'", kind);
  }
  if (outputCount != 1)
    return fail("no output_node");
  for (const Resource &r : resources)
    if (!r.writtenByPs && !r.writtenByCs)
      return failf("resource '%s' is never written", r.name.c_str());
  // a node reads what an earlier node wrote this frame, or its own history
  // partner; anything else is last frame's surface, or nothing on the first
  eastl::vector<bool> produced(resources.size(), false);
  auto readsProduced = [&](const Node &n, const char *what) {
    for (const Binding &in : n.inputs)
      if (!in.history && !produced[in.res])
        return failf("%s reads '%s' before any node writes it", what, resources[in.res].name.c_str());
    for (const Binding &o : n.outputs)
      produced[o.res] = true;
    return true;
  };
  for (const Node &n : preloadNodes)
    if (!readsProduced(n, n.shader.c_str()))
      return false;
  for (const Node &n : frameNodes)
    if (!readsProduced(n, n.shader.c_str()))
      return false;
  if (!readsProduced(outputNode, "output_node"))
    return false;

  // the composite family takes the host's variant suffix (loading_splash_<scene>[_N])
  // and must not be a name the host's single-shader path looks up: it would
  // find the composite under the scene's name and draw it without the graph
  const eastl::string ownFamily(eastl::string::CtorSprintf{}, "loading_splash_%s", scene_name);
  const eastl::string hostShader(output_shader_name);
  if (hostShader.find(ownFamily) != 0)
    return failf("host shader '%s' does not start with '%s'", output_shader_name, ownFamily.c_str());
  if (is_scene_shader_name(outputNode.shader, ownFamily))
    return failf("output_node shader '%s' is the scene's own family or one of its host variants", outputNode.shader.c_str());
  // a node in that family would be found by the host's single-shader lookup and
  // drawn standalone, with none of its graph inputs bound
  for (const Node &n : preloadNodes)
    if (is_scene_shader_name(n.shader, ownFamily))
      return failf("preload node shader '%s' is the scene's own family", n.shader.c_str());
  for (const Node &n : frameNodes)
    if (is_scene_shader_name(n.shader, ownFamily))
      return failf("shader_node shader '%s' is the scene's own family", n.shader.c_str());
  const eastl::string outputShader = outputNode.shader + (hostShader.c_str() + ownFamily.size());
  outputNode.ps.init(outputShader.c_str(), /*is_optional*/ true);
  if (!outputNode.ps.getMat())
    return failf("output shader '%s' not in the dump", outputShader.c_str());
  debug("[splash] graph %s: %d resources, %d preload + %d frame nodes, output shader %s", path.c_str(), (int)resources.size(),
    (int)preloadNodes.size(), (int)frameNodes.size(), outputShader.c_str());

  noiseDefault = d3d::request_sampler({});
  ShaderElement::invalidate_cached_state_block();
  return true;
}

// The host size that scale:r reads, held under maxScaledSize. Both axes take the
// same factor, so a scaled resource keeps square pixels and the ratio between
// two scales - which is what a node's params carries - is untouched.
IPoint2 SplashGraphImpl::scaledBase(int w, int h) const
{
  if (maxScaledSize.x < 1 || maxScaledSize.y < 1)
    return IPoint2(w, h);
  if (w <= maxScaledSize.x && h <= maxScaledSize.y)
    return IPoint2(w, h);
  // which axis binds, decided in integers: maxW/w <= maxH/h is maxW*h <= maxH*w
  const bool widthBinds = int64_t(maxScaledSize.x) * h <= int64_t(maxScaledSize.y) * w;
  const float k = widthBinds ? float(maxScaledSize.x) / float(w) : float(maxScaledSize.y) / float(h);
  // the ratio is a float and ceilf can land the bound axis a pixel over it
  // (3440x1440 under 1920x1080 rounds to 1921), so each axis is held down
  const int sx = (int)ceilf(k * float(w)), sy = (int)ceilf(k * float(h));
  return IPoint2(sx < maxScaledSize.x ? sx : maxScaledSize.x, sy < maxScaledSize.y ? sy : maxScaledSize.y);
}

bool SplashGraphImpl::createResources(int w, int h)
{
  const IPoint2 base = scaledBase(w, h);
  for (Resource &r : resources)
  {
    r.tex[0].close();
    r.tex[1].close();
    r.buf.close();
    if (r.isBuf)
    {
      const int dwords = r.fixed ? r.sizeDwords : (int)ceilf(r.scaleDwords * float(base.x) * float(base.y));
      eastl::string name(eastl::string::CtorSprintf{}, "splash_graph_%s", r.name.c_str());
      r.buf = dag::buffers::create_ua_sr_byte_address(dwords, name.c_str());
      if (!r.buf)
        return failf("buffer '%s' (%d dwords) failed to create", r.name.c_str(), dwords);
      r.curSize = IPoint2(dwords, 1);
      continue;
    }
    const IPoint2 sz = r.fixed ? r.size : IPoint2((int)ceilf(r.scale * float(base.x)), (int)ceilf(r.scale * float(base.y)));
    const unsigned usage = d3d::get_texformat_usage(r.format);
    if ((r.writtenByPs && !(usage & d3d::USAGE_RTARGET)) || (r.writtenByCs && !(usage & d3d::USAGE_UNORDERED)))
      return failf("texture '%s': the format is not writable here (rt %d, uav %d)", r.name.c_str(), (int)r.writtenByPs,
        (int)r.writtenByCs);
    const uint32_t flags = r.format | (r.writtenByPs ? TEXCF_RTARGET : 0) | (r.writtenByCs ? TEXCF_UNORDERED : 0);
    for (int i = 0; i < (r.history ? 2 : 1); i++)
    {
      eastl::string name(eastl::string::CtorSprintf{}, i ? "splash_graph_%s_prev" : "splash_graph_%s", r.name.c_str());
      r.tex[i] = dag::create_tex(nullptr, sz.x, sz.y, flags, 1, name.c_str());
      if (!r.tex[i])
        return failf("texture '%s' %dx%d failed to create", r.name.c_str(), sz.x, sz.y);
      // a history reader trusts history_is_valid, not the writer's coverage: a
      // cs node with a data-dependent early-out leaves texels untouched, and
      // from the second frame they would enter the blend as whatever was here
      if (r.history)
      {
        if (r.writtenByPs)
        {
          SCOPE_RENDER_TARGET;
          d3d::set_render_target({}, DepthAccess::RW, {RenderTarget{r.tex[i].getTex2D(), 0, 0}});
          d3d::clearview(CLEAR_TARGET, 0, 0, 0);
        }
        else if (r.format == TEXFMT_R16UI || r.format == TEXFMT_R32UI || r.format == TEXFMT_R32G32UI)
        {
          // the UAV clear is typed: a float clear on an integer view is undefined
          const uint32_t zero[4] = {0, 0, 0, 0};
          d3d::clear_rwtexi(r.tex[i].getTex2D(), zero, 0, 0);
        }
        else
        {
          const float zero[4] = {0, 0, 0, 0};
          d3d::clear_rwtexf(r.tex[i].getTex2D(), zero, 0, 0);
        }
      }
    }
    r.curSize = sz;
    r.cur = 0;
  }
  targetW = w;
  targetH = h;
  resetCounter = get_d3d_reset_counter();
  preloadDone = false;
  frameIndex = 0;
  debug("[splash] graph resources: host %dx%d, scaled base %dx%d, reset generation %u", w, h, base.x, base.y, resetCounter);
  return true;
}

IPoint2 SplashGraphImpl::drivingSize(const Node &n) const
{
  return resources[n.dispatchFrom >= 0 ? n.dispatchFrom : n.outputs[0].res].curSize;
}

void SplashGraphImpl::bindFixed(unsigned stage, const Frame &f, IPoint2 out_size, const Point4 &params, bool game_is_under) const
{
  // one dshl block declares every slot, so a dump that passed init has them all
  d3d::set_const(stage, loading_splash_node_params_const_no.get_int(), &params.x, 1);
  splash_slots::set_time_const(stage, loading_splash_iGlobalTime_iResolution_iPaperWhiteNits_const_no.get_int(), f.time, out_size.x,
    out_size.y, f.paperWhiteNits);
  const float c1[4] = {f.seed, 0.f, 0.f, 0.f};
  d3d::set_const(stage, loading_splash_iSeed_const_no.get_int(), c1, 1);
  // an unresolved optional name answers register 0, which holds c0
  if (bool(loading_splash_exit_const_no))
    splash_slots::set_exit_const(stage, loading_splash_exit_const_no.get_int(), f.slowExitTime, f.rushExitTime, slowExitSec,
      rushExitSec);
  splash_slots::set_frame_const(stage, loading_splash_iFrame_history_iPrevTime_const_no.get_int(), frameIndex, frameIndex != 0,
    prevTime, f.time, game_is_under);

  const int n64 = loading_splash_noise_64_tex_l8_const_no.get_int();
  const int n128 = loading_splash_noise_128_tex_hash_const_no.get_int();
  d3d::set_tex(stage, n64, noise64->getTex2D());
  d3d::set_sampler(stage, n64, noiseDefault);
  d3d::set_tex(stage, n128, noise128->getTex2D());
  d3d::set_sampler(stage, n128, noiseDefault);
  splash_slots::bind_clamp_samplers(stage, loading_splash_linear_clamp_const_no.get_int(),
    loading_splash_point_clamp_const_no.get_int());
}

void SplashGraphImpl::bindInputs(unsigned stage, const Node &n) const
{
  for (const Binding &b : n.inputs)
  {
    const Resource &r = resources[b.res];
    if (r.isBuf)
      d3d::set_buffer(stage, b.reg, r.buf.getBuf());
    else
      d3d::set_tex(stage, b.reg, r.tex[b.history ? r.cur ^ 1 : r.cur].getTex2D());
  }
}

// a surface left bound as an input would alias the next node's output
void SplashGraphImpl::unbindInputs(unsigned stage, const Node &n) const
{
  for (const Binding &b : n.inputs)
    if (resources[b.res].isBuf)
      d3d::set_buffer(stage, b.reg, nullptr);
    else
      d3d::set_tex(stage, b.reg, nullptr);
}

// binds, draws the fullscreen triangle into the current targets, unbinds
void SplashGraphImpl::drawPixelNode(Node &n, const Frame &f, IPoint2 out_size, bool game_is_under)
{
  bindFixed(STAGE_PS, f, out_size, n.params, game_is_under);
  bindInputs(STAGE_PS, n);
  n.ps.getElem()->setProgram(0);
  d3d::setvsrc(0, 0, 0);
  d3d::draw(PRIM_TRILIST, 0, 1);
  unbindInputs(STAGE_PS, n);
}

void SplashGraphImpl::runNode(Node &n, const Frame &f)
{
  const IPoint2 size = drivingSize(n);
  if (!n.compute)
  {
    RenderTarget colors[4] = {};
    for (const Binding &b : n.outputs)
    {
      Resource &r = resources[b.res];
      colors[b.reg] = {r.tex[r.cur].getTex2D(), 0, 0};
    }
    d3d::set_render_target({}, DepthAccess::RW, make_span_const(colors, n.outputs.size()));
    drawPixelNode(n, f, size, /*game_is_under*/ false);
    // a compute node next may read or write these: leave nothing bound as a target
    d3d::set_render_target({}, DepthAccess::RW, make_span_const(colors, 0));
    return;
  }
  bindFixed(STAGE_CS, f, size, n.params, /*game_is_under*/ false);
  bindInputs(STAGE_CS, n);
  for (const Binding &b : n.outputs)
  {
    Resource &r = resources[b.res];
    if (r.isBuf)
      d3d::set_rwbuffer(STAGE_CS, b.reg, r.buf.getBuf());
    else
      d3d::set_rwtex(STAGE_CS, b.reg, r.tex[r.cur].getTex2D(), 0, 0);
  }
  if (n.fixedDispatch)
    n.cs->dispatch(n.dispatch.x, n.dispatch.y, n.dispatch.z);
  else
    n.cs->dispatchThreads(size.x, size.y, 1);
  for (const Binding &b : n.outputs)
  {
    Resource &r = resources[b.res];
    if (r.isBuf)
    {
      d3d::set_rwbuffer(STAGE_CS, b.reg, nullptr);
      d3d::resource_barrier({r.buf.getBuf(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
    }
    else
    {
      d3d::set_rwtex(STAGE_CS, b.reg, nullptr, 0, 0);
      d3d::resource_barrier({r.tex[r.cur].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    }
  }
  unbindInputs(STAGE_CS, n);
}

bool SplashGraphImpl::draw(const Frame &f)
{
  Driver3dRenderTarget hostRt;
  d3d::get_render_target(hostRt);
  int w = 0, h = 0;
  d3d::get_target_size(w, h);
  if (w < 1 || h < 1)
    return true;
  if (w != targetW || h != targetH || get_d3d_reset_counter() != resetCounter)
    if (!createResources(w, h))
      return false;

  if (!preloadDone)
  {
    for (Node &n : preloadNodes)
      runNode(n, f);
    preloadDone = true;
  }
  for (Node &n : frameNodes)
    runNode(n, f);

  d3d::set_render_target(hostRt);
  // read through the fixed clamp samplers: a sampler at the logo's own register would replace s2
  if (f.titleLogo && bool(loading_splash_title_logo_const_no))
    d3d::set_tex(STAGE_PS, loading_splash_title_logo_const_no.get_int(), f.titleLogo);
  drawPixelNode(outputNode, f, IPoint2(w, h), opensOntoGameFlag && f.overGame);

  for (Resource &r : resources)
    if (r.history)
      r.cur ^= 1;
  prevTime = f.time;
  frameIndex++;
  return true;
}

} // namespace

static eastl::string graph_path(const char *graph_dir, const char *scene_name)
{
  return eastl::string(eastl::string::CtorSprintf{}, "%s/%s.graph.blk", graph_dir, scene_name);
}

eastl::unique_ptr<SplashGraph> SplashGraph::load(const char *graph_dir, const char *scene_name, const char *output_shader_name,
  bool *blk_present)
{
  if (blk_present)
    *blk_present = false;
  if (!graph_dir || !*graph_dir || !scene_name || !*scene_name)
    return nullptr;
  const eastl::string path = graph_path(graph_dir, scene_name);
  if (!dd_file_exists(path.c_str()))
  {
    debug("[splash] no graph for '%s' at %s", scene_name, path.c_str());
    return nullptr;
  }
  if (blk_present)
    *blk_present = true;
  DataBlock blk;
  if (!blk.load(path.c_str()))
  {
    logerr("[splash] graph %s: load failed", path.c_str());
    return nullptr;
  }
  return load_from_blk(blk, path.c_str(), scene_name, output_shader_name);
}

eastl::unique_ptr<SplashGraph> SplashGraph::load_from_blk(const DataBlock &blk, const char *src_name, const char *scene_name,
  const char *output_shader_name)
{
  // the blk naming lives in graph_path: a caller with no file to name should
  // not have to repeat it just to label its messages
  eastl::string embedded;
  if (!src_name)
  {
    embedded.sprintf("embedded %s.graph.blk", scene_name);
    src_name = embedded.c_str();
  }
  eastl::unique_ptr<SplashGraphImpl> graph(new SplashGraphImpl);
  if (!graph->init(blk, src_name, scene_name, output_shader_name))
    return nullptr;
  return graph;
}

void splash_slots::set_time_const(unsigned stage, int reg, float time, int width, int height, float paper_white_nits)
{
  const float c0[4] = {time, float(width), float(height), paper_white_nits};
  d3d::set_const(stage, reg, c0, 1);
}

void splash_slots::set_exit_const(unsigned stage, int reg, float slow_at, float rush_at, float slow_seconds, float rush_seconds)
{
  const float c2[4] = {slow_at, rush_at, slow_seconds, rush_seconds};
  d3d::set_const(stage, reg, c2, 1);
}

void splash_slots::set_frame_const(unsigned stage, int reg, int frame_index, bool history_valid, float prev_time, float time,
  bool game_is_under)
{
  G_ASSERT(!history_valid || frame_index > 0); // history holds a frame only after one was drawn
  const float c3[4] = {float(frame_index), history_valid ? 1.f : 0.f, frame_index ? prev_time : time, game_is_under ? 1.f : 0.f};
  d3d::set_const(stage, reg, c3, 1);
}

int splash_slots::reg_or_none(const ShaderVariableInfo &var) { return bool(var) ? var.get_int() : -1; }

void splash_slots::bind_single_shader_slots(unsigned stage, int frame_reg, int node_params_reg, int linear_reg, int point_reg,
  int frame_index, float prev_time, float time)
{
  if (frame_reg >= 0)
    set_frame_const(stage, frame_reg, frame_index, /*history_valid*/ false, prev_time, time, /*game_is_under*/ false);
  if (node_params_reg >= 0)
  {
    const float zero[4] = {0.f, 0.f, 0.f, 0.f};
    d3d::set_const(stage, node_params_reg, zero, 1);
  }
  if (linear_reg >= 0 && point_reg >= 0)
    bind_clamp_samplers(stage, linear_reg, point_reg);
}

void splash_slots::bind_clamp_samplers(unsigned stage, int linear_reg, int point_reg)
{
  d3d::SamplerInfo smp;
  smp.address_mode_u = smp.address_mode_v = smp.address_mode_w = d3d::AddressMode::Clamp;
  d3d::set_sampler(stage, linear_reg, d3d::request_sampler(smp));
  smp.filter_mode = d3d::FilterMode::Point;
  d3d::set_sampler(stage, point_reg, d3d::request_sampler(smp));
}

// twin of dshl splash_exit_*; keep in step
SplashExitPacing splash_exit_pacing(float t, float slow_at, float rush_at, float slow_seconds, float rush_seconds)
{
  const float EXIT_END = 2.f, DOOR_SHARE = 0.5f;
  // the degenerate shape is 0/0, not x/0
  auto ramp = [](float num, float den) { return den > 0.f ? clamp(num / den, 0.f, 1.f) : 1.f; };
  const float slowPace = slow_at > 0.f && slow_seconds > 0.f ? 1.f / slow_seconds : 0.f;

  SplashExitPacing p;
  if (rush_at > 0.f && t >= rush_at)
  {
    const float a0 = clamp((rush_at - slow_at) * slowPace, 0.f, EXIT_END);
    p.elapsed = t - (slow_at > 0.f ? slow_at : rush_at);
    p.approach = a0 + (EXIT_END - a0) * ramp(t - rush_at, DOOR_SHARE * rush_seconds);
    p.portal = ramp(t - rush_at, rush_seconds);
  }
  // a scene that declared no duration, or a preview standing before the stamp,
  // is still loading: neither is an exit sitting at 0
  else if (slowPace > 0.f && t >= slow_at)
  {
    p.elapsed = t - slow_at;
    p.approach = min((t - slow_at) * slowPace, EXIT_END);
  }
  return p;
}

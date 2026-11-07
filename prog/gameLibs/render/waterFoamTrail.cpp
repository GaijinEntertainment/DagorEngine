// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/waterFoamTrail.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_texPackMgr2.h>
#include <3d/dag_materialData.h>
#include <3d/dag_quadIndexBuffer.h>
#include <3d/dag_resPtr.h>
#include <gameRes/dag_gameResources.h>
#include <shaders/dag_postFxRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_frustum.h>
#include <memory/dag_framemem.h>
#include <stdio.h>

namespace water_trail
{
struct Context;
static Context *g_ctx = NULL;
static Settings g_settings = Settings();

const float tcXremap = 0.05f;
const float tcXremapRcp = 1.0f / tcXremap;

struct WaterTrailRenderNode
{
  Point2 pos;
  uint32_t scaledSideVectorR;
  uint32_t scaledSideVectorL;
  // 1
  float tc;
  uint32_t params;
  uint32_t rotationOpacity;
  uint32_t amplitudeFrequency;
  // 2
};

static uint32_t encode_float2(Point2 float_2) { return (float_to_half(float_2.x) << 16) | float_to_half(float_2.y); }

struct WaterTrailNode
{
  Point2 pos;
  float opacity;
  float tc;
  Point2 expansionLimit;
  Point2 currentExpansion;
  Point2 sideVector;
  Point2 initialExpansion;

  float expansionSpeed; // each node have different expansion speed

  uint32_t amplitudeFrequency; // this one is constant and defined on node spawn

  WaterTrailRenderNode getRenderNode(float cur_tc = 0.f, float blend_dist = 1.f, float alpha_mul = 1.f, float turbulence_mul = 1.f,
    uint32_t params = 0) const
  {
    WaterTrailRenderNode result;
    result.pos = pos;
    float fadedOpacity = opacity * alpha_mul * clamp((cur_tc - tc) / blend_dist, 0.0f, 1.0f);
    result.tc = tc;
    result.params = params;
    float rotation = turbulence_mul * opacity * opacity; // turbulent flows are much faster near ship
    result.rotationOpacity = encode_float2(Point2(rotation, fadedOpacity));
    result.amplitudeFrequency = amplitudeFrequency;
    result.scaledSideVectorR = encode_float2(initialExpansion + sideVector * currentExpansion.x);
    result.scaledSideVectorL = encode_float2(-initialExpansion - sideVector * currentExpansion.y);
    return result;
  }

  WaterTrailRenderNode getPlugNode() const
  {
    WaterTrailRenderNode result;
    result.pos = pos;
    result.tc = 0;
    result.params = 0;
    result.rotationOpacity = 0;
    result.amplitudeFrequency = 0;
    result.scaledSideVectorR = encode_float2(initialExpansion + sideVector * currentExpansion.x);
    result.scaledSideVectorL = encode_float2(-initialExpansion - sideVector * currentExpansion.y);
    return result;
  }

  bool update(float dt, float fade_speed, float water_level, BBox3 &cull_box)
  {
    if (opacity < 0)
      return false;
    opacity -= dt * fade_speed;
    // todo: slowdown expansion near limit?
    currentExpansion.x = min(currentExpansion.x + expansionSpeed * dt, expansionLimit.x);
    currentExpansion.y = min(currentExpansion.y + expansionSpeed * dt, expansionLimit.y);

    cull_box += Point3(pos.x + initialExpansion.x, water_level + 1, pos.y + initialExpansion.y) +
                Point3(sideVector.x, 0, sideVector.y) * currentExpansion.x;
    cull_box += Point3(pos.x - initialExpansion.x, water_level - 1, pos.y + initialExpansion.y) -
                Point3(sideVector.x, 0, sideVector.y) * currentExpansion.y;
    return true;
  }
};

#define MAX_NODES_PER_TRAIL 64
#define MIN_NODES_PER_TRAIL 16
#define MAX_ALLOWED_TRAILS  128

static uint32_t maxTrailNodeRenderCount = MAX_ALLOWED_TRAILS * (4 + MAX_NODES_PER_TRAIL);

struct Emitter
{
  float width;
  float alphaMul;
  float turbulenceMul;
  float waveHeight;
  float foamMul;
  float inscatMul;
  uint32_t packedParams;
  Tab<WaterTrailNode> trail;

  Point3 lastPos;
  bool detached;
  float fadeOutSpeed;
  float minTimeBetweenSpawn;

  int currentIdx;
  float tcX;
  float timer;
  float lastEmitTime;

  int maxNodeCount;

  BBox3 cullBox;

  // start node of trail
  int getStart() const
  {
    int ret;
    if (trail.size() == maxNodeCount)
      ret = currentIdx;
    else
      ret = 0;

    return clamp(ret, 0, max((int)trail.size() - 1, 0));
  }

  bool shouldRender(const Point3 &origin, const Frustum &frustum)
  {
    G_UNUSED(origin);
    return frustum.testBox(cullBox);
  }

  Emitter(float width_, float life_time, int node_count, float alpha_mul, float foam_mul, float inscat_mul, float turbulence_mul,
    float wave_height) :
    width(width_),
    alphaMul(alpha_mul),
    foamMul(foam_mul),
    inscatMul(inscat_mul),
    turbulenceMul(turbulence_mul),
    waveHeight(wave_height),
    fadeOutSpeed(0.02),
    tcX(0.0f),
    currentIdx(0),
    timer(0.0f),
    lastEmitTime(-10000.0f),
    lastPos(-100000.f, -100000.f, -100000.f),
    detached(false)
  {
    fadeOutSpeed = 1.0f / clamp(life_time, 1.0f, 180.0f); // 3 minutes seems like reasonable limit
    maxNodeCount = clamp(node_count, MIN_NODES_PER_TRAIL, MAX_NODES_PER_TRAIL);
    minTimeBetweenSpawn = 1.0f / (fadeOutSpeed * (float)maxNodeCount);
    packedParams = encode_float2(Point2(foamMul, inscatMul));
    cullBox.setempty();
  }

  WaterTrailNode genNewNode(const Point3 &pos, const Point2 &segment_dir, float alpha, const Point2 &left, const Point2 &right,
    const Point2 &limits, float tc_x, float velocity)
  {
    WaterTrailNode result;

    result.pos = Point2(pos.x, pos.z);
    result.sideVector = Point2(segment_dir.y, -segment_dir.x);
    result.currentExpansion = Point2(0, 0);
    result.initialExpansion = right;
    result.expansionLimit = Point2(min(20 * left.length(), limits.x), min(20 * right.length(), limits.y));
    result.expansionSpeed = 1; // const for now, may be changed later
    result.opacity = alpha;
    result.tc = tc_x;

    float dirCos = abs(dot(result.sideVector, normalize(right)));

    // we use floor() to have discrete set of frequencies to avoid wave "jumps" on node update
    result.amplitudeFrequency = encode_float2(Point2(waveHeight * dirCos, tcXremapRcp * 2.75f / max(floor(velocity), 1.0f)));

    return result;
  }

  // emit new trail (start new or continue existing)
  // updates last emited node if emit cooldown is not reached
  bool emit(const Point3 &pos, const Point2 &move_dir, const Point2 &left, const Point2 &right, real opacity, float velocity)
  {
    bool teleported = (pos - lastPos).length() > 100.0f;

    // there is no need to perform all further calculations if passed too low time
    if (timer - lastEmitTime < 0.1 * minTimeBetweenSpawn && !teleported)
      return false;

    if (trail.size() <= 1) // first spawned node should be faded out
      opacity = 0;

    int prevNodeIdx = -1;
    if (trail.size() == maxNodeCount)
      prevNodeIdx = currentIdx == 0 ? maxNodeCount - 1 : currentIdx - 1;
    else if (trail.size() > 1)
      prevNodeIdx = currentIdx - 1;

    bool reverted = false;
    Point2 limits = Point2(1000.0f, 1000.0f);
    if (prevNodeIdx >= 0)
    {
      reverted = dot((pos - lastPos), Point3(lastPos.x - trail[prevNodeIdx].pos.x, 0, lastPos.z - trail[prevNodeIdx].pos.y)) < 0;
      // todo: calculate limits by intersection of node right and left vectors
    }

    bool teleportedOrReverted = teleported || reverted;

    tcX += (pos - lastPos).length() * tcXremap;
    lastPos = pos;

    WaterTrailNode newNode = genNewNode(pos, normalize(move_dir), opacity, left, right, limits, tcX, velocity);

    if (teleportedOrReverted)
    {
      newNode.opacity = 0;
      if (prevNodeIdx >= 0)
      {
        trail[prevNodeIdx].opacity = 0;
      }
    }

    if (timer - lastEmitTime < minTimeBetweenSpawn && !teleportedOrReverted)
    {
      if (prevNodeIdx >= 0)
      {
        trail[prevNodeIdx] = newNode;
      }
      return false;
    }

    if (trail.size() == maxNodeCount)
      trail[currentIdx] = newNode;
    else
      trail.push_back(newNode);

    currentIdx = (currentIdx + 1) % maxNodeCount;
    lastEmitTime = teleportedOrReverted ? -1000.0f : timer; // new node with zero opacity should not be changed
    return true;
  }
};

struct Renderer
{
  SharedTexWithShaderVar maskTex;
  SharedTexWithShaderVar noise64Tex;

  Renderer()
  {
    SharedTex maskTexRes = dag::get_tex_gameres(g_settings.texName);
    G_ASSERT(maskTexRes);

    if (!is_managed_textures_streaming_load_on_demand())
      ddsx::tex_pack2_perform_delayed_data_loading();

    maskTex = SharedTexWithShaderVar(eastl::move(maskTexRes), "water_foam_trail_mask");

    SharedTex noise64TexRes = dag::get_tex_gameres("white_noise_64x64");
    if (noise64TexRes)
      noise64Tex = SharedTexWithShaderVar(eastl::move(noise64TexRes), "tex_noise_foam_trail");

    resetTex();
  }

  ~Renderer() {}

  void resetTex()
  {
    ShaderGlobal::set_sampler(::get_shader_variable_id("water_foam_trail_mask_samplerstate", true),
      d3d::request_sampler(get_sampler_info(get_texture_meta_data(maskTex.getTexId()))));

    if (noise64Tex.getTexId() != BAD_TEXTUREID)
      ShaderGlobal::set_sampler(::get_shader_variable_id("tex_noise_foam_trail_samplerstate", true),
        d3d::request_sampler(get_sampler_info(get_texture_meta_data(noise64Tex.getTexId()))));

    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
    smpInfo.anisotropic_max = ::dgs_tex_anisotropy;
    ShaderGlobal::set_sampler(::get_shader_variable_id("water_foam_trail_samplerstate", true), d3d::request_sampler(smpInfo));
  }
};

struct Context
{
  Tab<Emitter *> activeEmittersPool;
  Tab<int> freeEmittersPool;

  UniqueBufWithShaderVar renderDataVS;

  DynamicShaderHelper foamTrailMaterial;

  uint32_t renderCount;

  bool renderEnabled;
  Renderer *renderer;

  Context() : renderer(NULL), renderEnabled(false), renderCount(0) {}

  ~Context() { reset(); }

  void reset()
  {
    while (!activeEmittersPool.empty())
    {
      del_it(activeEmittersPool.back());
      activeEmittersPool.pop_back();
    }

    freeEmittersPool.clear();

    del_it(renderer);
    renderEnabled = false;
  }

  int createEmitter(float width, float life_time, int node_count, float alpha_mul, float foam_mul, float inscat_mul,
    float turbulence_mul, float wave_height)
  {
    int id = -1;
    if (!freeEmittersPool.empty())
    {
      id = freeEmittersPool.back();
      freeEmittersPool.pop_back();
    }

    if (id == -1)
    {
      id = activeEmittersPool.size();
      activeEmittersPool.push_back();
    }

    activeEmittersPool[id] = new Emitter(width, life_time, node_count, alpha_mul, foam_mul, inscat_mul, turbulence_mul, wave_height);
    return id;
  }

  void destroyEmitter(int id)
  {
    del_it(activeEmittersPool[id]);
    freeEmittersPool.push_back(id);
  }

  // no more new node spawn, but existing nodes should be visible
  void detachEmitter(int id)
  {
    if (activeEmittersPool[id]->trail.size() <= 0)
      destroyEmitter(id);
    else
      activeEmittersPool[id]->detached = true;
  }

  void createTrailMatAndBuffer()
  {
    renderDataVS = dag::create_sbuffer(sizeof(Point4), maxTrailNodeRenderCount * sizeof(WaterTrailRenderNode) / sizeof(Point4),
      SBCF_DYNAMIC | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE, TEXFMT_A32B32G32R32F, String(0, "water_trails_data_vs"),
      RESTAG_FOAM_TRAIL);

    foamTrailMaterial.init("water_trails", NULL, 0, "water_trail shader");
  }

  void update(float dt, float water_level)
  {
    for (int i = 0; i < activeEmittersPool.size(); i++)
    {
      auto &tr = activeEmittersPool[i];

      if (!tr || tr->trail.size() == 0)
        continue;

      tr->timer += dt;

      bool visible = false;

      tr->cullBox.setempty();

      for (int j = 0; j < tr->trail.size(); j++)
      {
        visible |= tr->trail[j].update(dt, tr->fadeOutSpeed, water_level, tr->cullBox);
      }

      if (tr->detached && !visible)
        destroyEmitter(i);
    }
  }

  void before_render(const Point3 &origin, const Frustum &frustum)
  {
    if (!renderDataVS.getBuf())
      return;

    TIME_D3D_PROFILE(water_trail_prepare);

    Tab<WaterTrailRenderNode> renderData(framemem_ptr());

    uint32_t lastSize = 0;
    int trailStartIndex = 0;
    for (const auto &tr : activeEmittersPool)
    {
      if (!tr || tr->trail.size() == 0 || !tr->shouldRender(origin, frustum))
        continue;

      float curTc = tr->tcX;

      {
        // empty node before trail
        int idx = tr->getStart() > tr->trail.size() ? 0 : tr->getStart();
        renderData.push_back(tr->trail[idx].getPlugNode());
      }

      const float blendDist = 1.0f; // distance in tcX to blend start nodes of trail

      for (int j = tr->getStart(); j < tr->trail.size(); j++)
        renderData.push_back(tr->trail[j].getRenderNode(curTc, blendDist, tr->alphaMul, tr->turbulenceMul, tr->packedParams));

      for (int j = 0; j < tr->getStart(); j++)
        renderData.push_back(tr->trail[j].getRenderNode(curTc, blendDist, tr->alphaMul, tr->turbulenceMul, tr->packedParams));

      {
        // empty node after trail
        int idx = tr->getStart() > 0 ? (tr->getStart() - 1) : (tr->trail.size() - 1);
        renderData.push_back(tr->trail[idx].getPlugNode());
      }

      trailStartIndex = renderData.size();
    }

    renderCount = renderData.size() - lastSize; // lastsize will be needed if we'll add some different trails, like wake one
    lastSize = renderData.size();

    uint32_t lockCount = min(renderData.size(), maxTrailNodeRenderCount);

    if (lockCount > 0)
      renderDataVS.getBuf()->updateDataWithLock(0, sizeof(WaterTrailRenderNode) * lockCount, renderData.data(), VBLOCK_DISCARD);
    renderDataVS.setVar();

    renderCount = min(renderCount, lockCount);
  }

  void prepare(float dt, const Point3 &origin, const Frustum &frustum, float water_level)
  {
    if (!renderDataVS && renderEnabled)
      createTrailMatAndBuffer();

    if (renderEnabled && !renderer)
      renderer = new Renderer();
    else if (!renderEnabled)
      del_it(renderer);

    if (!renderer)
      return;

    update(dt, water_level);
    before_render(origin, frustum);
  }

  bool render_strips()
  {
    if (!renderDataVS.getBuf() || !renderEnabled)
      return false;

    index_buffer::Quads32BitUsageLock quads32Lock;

    TIME_D3D_PROFILE(water_trail_render);

    {
      if (renderCount < 2)
        return false;

      d3d::setvsrc_ex(0, NULL, 0, 0);

      if (!foamTrailMaterial.shader->setStates(0, true))
        return false;

      d3d::drawind_instanced(PRIM_TRILIST, 0, 2, 0, g_settings.tessFactor * (renderCount - 1));
    }
    return true;
  }
};

//
// interface
//

void init()
{
  G_ASSERT(!g_ctx);

  g_ctx = new Context();
}

void on_unload_scene()
{
  if (g_ctx)
    g_ctx->reset();
}

void close() { del_it(g_ctx); }

bool available() { return g_ctx != NULL; }

bool hasUnderwaterTrail() { return g_settings.underwaterFoamWidth > 0.f; }

void enable_render(bool v)
{
  if (g_ctx)
    g_ctx->renderEnabled = v;
}

void update_render_settings(const Settings &s)
{
  g_settings = s;
  g_settings.tessFactor = clamp(g_settings.tessFactor, 1, 4);
  ShaderGlobal::set_int(::get_shader_variable_id("water_trail_spline_substeps", true), g_settings.tessFactor);
}

int create_emitter(float width, float life_time, int node_count, float alpha_mul, float foam_mul, float inscat_mul,
  float turbulence_mul, float wave_height)
{
  if (!g_ctx)
    return -1;

  G_ASSERT(width > 0.f);
  G_ASSERT(life_time > 0.f);
  G_ASSERT(alpha_mul > 0.f);

  return g_ctx->createEmitter(width, life_time, node_count, alpha_mul, foam_mul, inscat_mul, turbulence_mul, wave_height);
}

void destroy_emitter(int id)
{
  if (id >= 0 && g_ctx)
  {
    G_ASSERT_RETURN(id < g_ctx->activeEmittersPool.size(), );
    Emitter *em = g_ctx->activeEmittersPool[id];
    G_ASSERT_RETURN(em, );
    g_ctx->destroyEmitter(id);
  }
}

void detach_emitter(int id)
{
  if (id >= 0 && g_ctx)
  {
    G_ASSERT_RETURN(id < g_ctx->activeEmittersPool.size(), );
    Emitter *em = g_ctx->activeEmittersPool[id];
    G_ASSERT_RETURN(em, );
    g_ctx->detachEmitter(id);
  }
}

void emit_point(int id, const Point2 &pt, const Point2 &dir, float alpha, bool is_underwater, float velocity)
{
  if (id < 0)
    return;

  G_ASSERT_RETURN(g_ctx, );
  G_ASSERT_RETURN(id < g_ctx->activeEmittersPool.size(), );
  Emitter *em = g_ctx->activeEmittersPool[id];

  G_ASSERT_RETURN(em, );
  if (is_underwater)
  {
    if (hasUnderwaterTrail())
    {
      const Point2 right = Point2{dir.y, -dir.x} * g_settings.underwaterFoamWidth;
      em->emit(Point3(pt.x, 0, pt.y), dir, -right, right, alpha * g_settings.underwaterFoamAlphaMult, velocity);
    }
  }
  else
  {
    Point2 right = Point2(dir.y, -dir.x) * em->width * 0.5f;
    em->emit(Point3(pt.x, 0, pt.y), dir, -right, right, alpha, velocity);
  }
}

void emit_point_ex(int id, const Point2 &pt, const Point2 &dir, const Point2 &left, const Point2 &right, float alpha, float velocity)
{
  if (id < 0)
    return;

  G_ASSERT_RETURN(g_ctx, );
  G_ASSERT_RETURN(id < g_ctx->activeEmittersPool.size(), );
  Emitter *em = g_ctx->activeEmittersPool[id];

  G_ASSERT_RETURN(em, );

  // currently spawns aligned with ship bbox projected to movement dir, left and right vectors holding intersections of ship hull
  // Node expansion performs in perpendicular direction to ship alignment
  // TODO: handle self-intersection cases
  Point2 center = 0.5f * (right + left);
  Point2 vec = right - left;
  float w = length(vec);
  Point2 rightDir = w > 0.f ? vec / w : Point2(0, 0);
  rightDir *= w * 0.5f;

  // fix for reverse motion
  if (dot(dir, Point2(-rightDir.y, rightDir.x)) < 0)
    rightDir = -rightDir;

  em->emit(Point3(pt.x + center.x, 0, pt.y + center.y), dir, -rightDir, rightDir, alpha, velocity);
}

void finalize_trail(int id)
{
  if (id < 0)
    return;

  G_ASSERT_RETURN(g_ctx, );
}

void before_render(float dt, const Point3 &origin, const Frustum &frustum, float water_level)
{
  if (g_ctx)
  {
    g_ctx->prepare(dt, origin, frustum, water_level);
  }
}

bool render()
{
  if (g_ctx)
  {
    return g_ctx->render_strips();
  }
  return false;
}

const Settings &get_settings() { return g_settings; }
} // namespace water_trail

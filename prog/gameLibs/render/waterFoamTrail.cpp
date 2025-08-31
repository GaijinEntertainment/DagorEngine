// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/waterFoamTrail.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_texPackMgr2.h>
#include <3d/dag_materialData.h>
#include <3d/dag_resPtr.h>
#include <gameRes/dag_gameResources.h>
#include <shaders/dag_postFxRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_frustum.h>
#include <shaders/dag_overrideStates.h>
#include <stdio.h>

namespace water_trail
{
struct Context;
static Context *g_ctx = NULL;
static Settings g_settings = Settings();

static const int g_turns_to_points_ratio = 5;

struct Vertex
{
  Point4 pos; // xy:pos, z:gen, w:dist
  Point4 dir; // xy:dir, z:side + alpha, w: dot(vel, dir)
  Point4 prm; // x:foam mul, y:insat mul, z:1/gen mul, w: step (debug)
};

struct Segment
{
  int id;
  float lastGen;
  float genMul;
  float forcedGenMul;
  float totalLength;

  int totalTurns;

  bbox3f box;
  Tab<Vertex> points;
  int owner; // which cascade is already claimed it
};

static float get_segment_fade_out_time(Segment *p)
{
  return g_settings.fadeOutTime * (get_settings().enableGenMuls ? p->genMul * p->forcedGenMul : 1.f);
}

template <typename Ctx>
struct Emitter
{
  Ctx *ctx;
  int step;
  float width;
  float genMul;
  float alphaMul;
  float foamMul;
  float inscatMul;
  Point2 lastPt;
  Point2 leftOffset;
  Point2 rightOffset;

  Segment *seg;

  Emitter(Ctx *ctx_, float width_, float gen_mul, float alpha_mul, float foam_mul, float inscat_mul) :
    ctx(ctx_),
    seg(NULL),
    step(0),
    width(width_),
    genMul(gen_mul),
    alphaMul(alpha_mul),
    foamMul(foam_mul),
    inscatMul(inscat_mul),
    leftOffset(0.f, 0.f),
    rightOffset(0.f, 0.f),
    lastPt(0.f, 0.f)
  {}

  ~Emitter() { finalize(); }

  void finalize()
  {
    if (seg)
      ctx->finalizeSegment(seg);

    seg = NULL;
    leftOffset.x = leftOffset.y = 0.f;
    rightOffset.x = rightOffset.y = 0.f;
  }

  void emitPoint(const Point2 &pt, const Point2 &dir, float alpha)
  {
    Point2 right = Point2(dir.y, -dir.x) * width * 0.5f;
    emitPointEx(pt, dir, -right, right, alpha);
  }

  void emitPointEx(const Point2 &pt, const Point2 &dir, const Point2 &left, const Point2 &right, float alpha)
  {
    if (!seg)
    {
      leftOffset = left;
      rightOffset = right;
      lastPt = pt;

      if (!pushPair())
        return;
      fillPair(pt, leftOffset, rightOffset, dir, 0.f);
    }

    float leftDistSq = lengthSq(left - leftOffset);
    float rightDistSq = lengthSq(right - rightOffset);

    float quadHeightSq = g_settings.quadHeight * g_settings.quadHeight;
    float widthThresholdSq = g_settings.widthThreshold * g_settings.widthThreshold;

    bool turning = max(leftDistSq, rightDistSq) > widthThresholdSq;

    if (turning) // workaround for extreme overlapping, which can cause some problems with fillrate
      ++seg->totalTurns;

    if (lengthSq(pt - lastPt) >= quadHeightSq || turning)
    {
      seg->totalLength += length(lastPt - pt);
      if (!pushPair())
        return;
      leftOffset = lerp(leftOffset, left, 0.5f);
      rightOffset = lerp(rightOffset, right, 0.5f);
      lastPt = pt;
    }

    fillPair(pt, leftOffset, rightOffset, dir, alpha);

    seg->lastGen = ctx->currentGen;
  }

  bool pushPair()
  {
    step = (step + 1) % 2;

    if (seg && seg->points.size() + 2 <= g_settings.maxPointsPerSegment &&
        seg->totalTurns * g_turns_to_points_ratio < g_settings.maxPointsPerSegment)
    {
      seg->points.push_back();
      seg->points.push_back();
      return true;
    }

    Segment *lastSeg = seg;
    seg = ctx->createSegment();
    if (!seg)
    {
      if (lastSeg)
        ctx->finalizeSegment(lastSeg);
      return false;
    }

    seg->genMul = genMul;
    seg->forcedGenMul = 1.f;
    seg->totalTurns = 0;

    if (lastSeg)
    {
      G_ASSERT(lastSeg->points.size() > 1);

      seg->totalLength = lastSeg->totalLength;

      const Vertex &pl = lastSeg->points[lastSeg->points.size() - 1];
      const Vertex &pr = lastSeg->points[lastSeg->points.size() - 2];

      seg->points.push_back();
      seg->points.push_back();
      fillPairRaw(Point2::xy(pl.pos), Point2::xy(pl.dir), Point2::xy(pr.dir), pl.dir.w, pl.pos.w, fabsf(pl.dir.z));
      seg->points.push_back();
      seg->points.push_back();

      ctx->finalizeSegment(lastSeg);
    }
    else
    {
      seg->totalLength = 0.f;
      seg->points.push_back();
      seg->points.push_back();
      fillPair(lastPt, leftOffset, rightOffset, Point2(0.f, 0.f), 0.f);
      seg->points.push_back();
      seg->points.push_back();
    }

    return true;
  }

  void fillPair(const Point2 &pt, const Point2 &left, const Point2 &right, const Point2 &move_dir, float alpha)
  {
    Point2 center = (right + left) * 0.5f;
    Point2 vec = right - left;
    float w = length(vec);

    Point2 rightDir = w > 0.f ? vec / w : Point2(0, 0);
    float tk = fabsf(dot(rightDir, Point2(move_dir.y, -move_dir.x)));

    tk = min(tk + 0.1f, 1.f);
    tk *= tk;

    rightDir *= w * 0.5f;

    float len = seg->totalLength + length(pt - lastPt);

    fillPairRaw(pt + center, -rightDir, rightDir, w > 0.f ? tk : 0.f, len, w > 0.f ? alpha : 0.f);
  }

  void fillPairRaw(const Point2 &pt, const Point2 &left, const Point2 &right, float turn_k, float len, float alpha)
  {
    Tab<Vertex> &points = seg->points;
    Vertex &v0 = points[points.size() - 2];
    Vertex &v1 = points.back();

    float genMulRcp = 1.f / seg->genMul;
    float st = (step ? 1.f : -1.f) / get_segment_fade_out_time(seg);
    alpha *= alphaMul;
    alpha *= min(len / max(g_settings.tailFadeInLength, 1e-3f), 1.0f);

    alpha = max(alpha, FLT_EPSILON);

    v0.pos = Point4(pt.x, pt.y, ctx->currentGen, len);
    v0.dir = Point4(right.x, right.y, alpha, turn_k);
    v0.prm = Point4(foamMul, inscatMul, genMulRcp, st);

    v1.pos = Point4(pt.x, pt.y, ctx->currentGen, len);
    v1.dir = Point4(left.x, left.y, -alpha, turn_k);
    v1.prm = Point4(foamMul, inscatMul, genMulRcp, st);

    // patch first quad
    if (points.size() >= 4 && seg->totalLength < g_settings.quadHeight)
    {
      points[0].dir = points[2].dir;
      points[1].dir = points[3].dir;
      points[0].dir.z = FLT_EPSILON;
      points[1].dir.z = -FLT_EPSILON;
    }

    Point2 maxLeft = pt + left * g_settings.forwardExpand * 1.f;
    Point2 maxRight = pt + right * g_settings.forwardExpand * 1.f;
    v_bbox3_add_pt(seg->box, v_make_vec4f(maxLeft.x, -1.f, maxLeft.y, 0.f));
    v_bbox3_add_pt(seg->box, v_make_vec4f(maxRight.x, +1.f, maxRight.y, 0.f));
  }
};

template <typename Ctx>
struct Renderer
{
  struct Cascade
  {
    float areaSize;
    bbox3f box;
    TMatrix4 mat;
    int activeBufferTriCount;
    int finalizedBufferTriCount;
  };

  Ctx *ctx;

  Point2 texelAlign;
  Tab<Cascade> cascades;

  Vbuffer *activeBuffer;
  Vbuffer *finalizedBuffer;

  Ptr<ShaderElement> shElem;
  Ptr<ShaderMaterial> shMat;

  int texArrayVarId;
  TEXTUREID texArrayId;

  ArrayTexture *texArray;
  SharedTexHolder maskTex;

  shaders::UniqueOverrideStateId blendOpStateId;

  Renderer(Ctx *ctx_) : ctx(ctx_), texArray(NULL), texArrayId(BAD_TEXTUREID)
  {
    for (int i = 0; i < g_settings.cascadeCount; ++i)
    {
      cascades.push_back();
      Cascade &c = cascades.back();
      c.areaSize = i == 0 ? g_settings.cascadeArea : cascades[i - 1].areaSize * g_settings.cascadeAreaMul;
    }

    Ptr<MaterialData> shMatTemp = new MaterialData();
    shMatTemp->className = "water_foam_trail";
    shMat = new_shader_material(*shMatTemp);

    if (!shMat)
      DAG_FATAL("waterFoamTrail::init - unable to load shader '%s'", shMatTemp->className);

    Tab<CompiledShaderChannelId> shChannels;
    shChannels.resize(3);
    shChannels[0].t = SCTYPE_FLOAT4;
    shChannels[0].vbu = SCUSAGE_POS;
    shChannels[0].vbui = 0;

    shChannels[1].t = SCTYPE_FLOAT4;
    shChannels[1].vbu = SCUSAGE_TC;
    shChannels[1].vbui = 0;

    shChannels[2].t = SCTYPE_FLOAT4;
    shChannels[2].vbu = SCUSAGE_TC;
    shChannels[2].vbui = 1;

    if (!shMat->checkChannels(shChannels.data(), shChannels.size()))
    {
      DAG_FATAL("waterFoamTrail::init - invalid channels for shader '%s'", shMatTemp->className);
    }

    shElem = shMat->make_elem();

    activeBuffer = d3d::create_vb(g_settings.activeVertexCount * sizeof(Vertex), SBCF_DYNAMIC, "water_foam_trail_a");
    finalizedBuffer = d3d::create_vb(g_settings.finalizedVertexCount * sizeof(Vertex), SBCF_DYNAMIC, "water_foam_trail_f");

    SharedTex maskTexRes = dag::get_tex_gameres(g_settings.texName);
    G_ASSERT(maskTexRes);

    if (!is_managed_textures_streaming_load_on_demand())
      ddsx::tex_pack2_perform_delayed_data_loading();

    maskTex = SharedTexHolder(eastl::move(maskTexRes), "water_foam_trail_mask");
    ShaderGlobal::set_sampler(::get_shader_variable_id("water_foam_trail_mask_samplerstate", true),
      get_texture_separate_sampler(maskTex.getTexId()));

    texArrayVarId = ::get_shader_variable_id("water_foam_trail");
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
    ShaderGlobal::set_sampler(::get_shader_variable_id("water_foam_trail_samplerstate", true), d3d::request_sampler(smpInfo));

    shaders::OverrideState blendOpState;
    blendOpState.set(shaders::OverrideState::BLEND_OP | shaders::OverrideState::BLEND_OP_A);
    blendOpState.blendOp = BLENDOP_MAX;
    blendOpState.blendOpA = BLENDOP_MIN;
    blendOpStateId = shaders::overrides::create(blendOpState);

    resetTex();
  }

  ~Renderer()
  {
    blendOpStateId.reset();

    ShaderGlobal::set_texture(texArrayVarId, BAD_TEXTUREID);
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(texArrayId, texArray);

    del_d3dres(activeBuffer);
    del_d3dres(finalizedBuffer);
  }

  void rebuildBuffer(bool active_pass, const Tab<Segment *> &pool, const Frustum &frustum)
  {
    if (pool.size() == 0)
      return;

    Vbuffer *vb = active_pass ? activeBuffer : finalizedBuffer;

    char *data = NULL;
    if (!vb->lock(0, 0, (void **)&data, VBLOCK_DISCARD | VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK))
      return;

    const Segment *prevSeg = NULL;
    const int endCascade = cascades.size();

    for (int c_id = 0; c_id < cascades.size(); ++c_id)
    {
      Cascade &csc = cascades[c_id];
      int &triCount = active_pass ? csc.activeBufferTriCount : csc.finalizedBufferTriCount;

      triCount = 0;

      for (int i = 0; i < pool.size(); ++i)
      {
        Segment *seg = pool[i];

        seg->owner = c_id == 0 ? endCascade : seg->owner; // clear ownership for new frame

        if (!g_settings.useTexArray)
        {
          if (!frustum.testBoxB(seg->box.bmin, seg->box.bmax))
            continue;
        }
        else if (seg->owner < c_id || !v_bbox3_test_box_intersect(csc.box, seg->box))
          continue;

        seg->owner = c_id;

        G_ASSERT(seg->points.size() >= 4);
        size_t size = seg->points.size() * sizeof(Vertex);

        if (prevSeg) // degenerative triangles
        {
          const Tab<Vertex> &prevPoints = prevSeg->points;

          Vertex v[3];
          v[0] = prevPoints.back();
          v[1] = v[2] = seg->points[0];

          memcpy(data, v, 3 * sizeof(Vertex));
          data += 3 * sizeof(Vertex);
          triCount += 5;
        }

        memcpy(data, seg->points.data(), size);
        data += size;
        triCount += seg->points.size() - 2;
        prevSeg = seg;
      }
    }

    vb->unlock();
  }

  void calcCascades()
  {
    static int matVarId[8] = {::get_shader_variable_id("water_foam_trail_mat00"), ::get_shader_variable_id("water_foam_trail_mat01"),
      ::get_shader_variable_id("water_foam_trail_mat10"), ::get_shader_variable_id("water_foam_trail_mat11"),
      ::get_shader_variable_id("water_foam_trail_mat20"), ::get_shader_variable_id("water_foam_trail_mat21"),
      ::get_shader_variable_id("water_foam_trail_mat30"), ::get_shader_variable_id("water_foam_trail_mat31")};

    TMatrix4 view = matrix_look_at_lh(Point3::x0y(ctx->currentOrigin) + Point3(0.f, 1.f, 0.f), Point3::x0y(ctx->currentOrigin),
      Point3(0.f, 0.f, 1.f));

    for (int ci = 0; ci < cascades.size(); ++ci)
    {
      Cascade &c = cascades[ci];

      c.activeBufferTriCount = 0;
      c.finalizedBufferTriCount = 0;

      float s = c.areaSize * g_settings.cascadeAreaScale;
      float hs = s * 0.5f;
      TMatrix4 proj = matrix_ortho_lh_forward(s, s, 0.f, 1.f);
      c.mat = view * proj;
      c.mat._41 = floor(c.mat._41 / texelAlign.x) * texelAlign.x;
      c.mat._42 = floor(c.mat._42 / texelAlign.y) * texelAlign.y;

      Point2 lt = Point2(-hs, -hs) + ctx->currentOrigin;
      Point2 rb = Point2(+hs, +hs) + ctx->currentOrigin;
      v_bbox3_init_empty(c.box);
      v_bbox3_add_pt(c.box, v_make_vec4f(lt.x, -1.f, lt.y, 0.f));
      v_bbox3_add_pt(c.box, v_make_vec4f(rb.x, +1.f, rb.y, 0.f));

      int mat0VarId = matVarId[ci * 2 + 0];
      int mat1VarId = matVarId[ci * 2 + 1];

      ShaderGlobal::set_color4_fast(mat0VarId, Color4(c.mat[0][0], c.mat[1][0], c.mat[2][0], c.mat[3][0]));

      ShaderGlobal::set_color4_fast(mat1VarId, Color4(c.mat[0][1], c.mat[1][1], c.mat[2][1], c.mat[3][1]));
    }
  }

  bool render()
  {
    TIME_D3D_PROFILE(water_trail_render);

    int activeBufferTriCount = 0;
    int finalizedBufferTriCount = 0;

    for (int i = 0; i < cascades.size(); ++i)
    {
      activeBufferTriCount += cascades[i].activeBufferTriCount;
      finalizedBufferTriCount += cascades[i].finalizedBufferTriCount;
    }

    bool needTrail = g_settings.useTrail && (activeBufferTriCount > 0 || finalizedBufferTriCount > 0);

    if (!needTrail)
    {
      ShaderGlobal::set_texture(texArrayVarId, BAD_TEXTUREID);
      return false;
    }

    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);

    shaders::overrides::set(blendOpStateId);

    {
      TIME_D3D_PROFILE(water_trail_main);
      renderTrail();
    }

    shaders::overrides::reset();

    ShaderGlobal::set_texture(texArrayVarId, texArrayId);

    return true;
  }

  void renderTrail()
  {
    int activeCount = 0;
    int finalizedCount = 0;

    for (int ci = 0; ci < cascades.size(); ++ci)
    {
      char name[64];
      SNPRINTF(name, sizeof(name), "water_trail_render_%d", ci);
      TIME_D3D_PROFILE_NAME(water_trail_render_, name);

      const Cascade &c = cascades[ci];

      if (texArray)
      {
        d3d::settm(TM_VIEW, &TMatrix4::IDENT);
        d3d::settm(TM_PROJ, &c.mat);

        ShaderGlobal::set_texture(texArrayVarId, BAD_TEXTUREID);
        d3d::set_render_target(0, texArray, ci, 0);
        d3d::clearview(CLEAR_TARGET, 0x00000000, 0.f, 0);
      }

      shElem->setStates(0, true);

      activeCount += c.activeBufferTriCount;
      finalizedCount += c.finalizedBufferTriCount;

      if (activeCount > 0)
      {
        d3d::setvsrc(0, activeBuffer, sizeof(Vertex));
        d3d::draw(PRIM_TRISTRIP, 0, activeCount);
      }

      if (finalizedCount > 0)
      {
        d3d::setvsrc(0, finalizedBuffer, sizeof(Vertex));
        d3d::draw(PRIM_TRISTRIP, 0, finalizedCount);
      }
    }

    static int currentGenVarId = ::get_shader_variable_id("water_foam_trail_gen");
    ShaderGlobal::set_real(currentGenVarId, ctx->currentGen);

    static int paramsVarId = ::get_shader_variable_id("water_foam_trail_params");
    ShaderGlobal::set_color4(paramsVarId, 1.f / g_settings.fadeInTime, 1.f / g_settings.fadeOutTime, g_settings.forwardExpand, 0.f);
  }

  void resetTex()
  {
    texelAlign = Point2(2.f / g_settings.texSize, 2.f / g_settings.texSize);

    ShaderGlobal::set_texture(texArrayVarId, BAD_TEXTUREID);
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(texArrayId, texArray);

    if (g_settings.useTexArray)
    {
      texArray = d3d::create_array_tex(g_settings.texSize, g_settings.texSize, cascades.size(), TEXFMT_R8 | TEXCF_RTARGET, 1,
        "water_foam_trail");

      texArrayId = register_managed_tex("water_foam_trail", texArray);
    }
  }
};

struct Context
{
  float currentGen;
  Point2 currentOrigin;

  Tab<Emitter<Context> *> activeEmittersPool;
  Tab<int> freeEmittersPool;

  Tab<Segment *> activeSegmentsPool;
  Tab<Segment *> freeSegmentsPool;
  Tab<Segment *> finalizedSegments;

  int totalActiveVertices;
  int totalFinalizedVertices;

  bool needRebuild;
  bool renderEnabled;
  Renderer<Context> *renderer;

  Context() :
    currentGen(0.f), needRebuild(false), renderer(NULL), renderEnabled(false), totalActiveVertices(0), totalFinalizedVertices(0)
  {}

  ~Context() { reset(); }

  void reset()
  {
    while (!activeEmittersPool.empty())
    {
      del_it(activeEmittersPool.back());
      activeEmittersPool.pop_back();
    }

    G_ASSERT(activeSegmentsPool.empty());

    freeEmittersPool.clear();

    while (!finalizedSegments.empty())
    {
      del_it(finalizedSegments.back());
      finalizedSegments.pop_back();
    }

    while (!freeSegmentsPool.empty())
    {
      del_it(freeSegmentsPool.back());
      freeSegmentsPool.pop_back();
    }

    del_it(renderer);
    currentGen = 0.f;
    totalActiveVertices = 0;
    totalFinalizedVertices = 0;
    renderEnabled = false;
  }

  int createEmitter(float width, float gen_mul, float alpha_mul, float foam_mul, float inscat_mul)
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

    activeEmittersPool[id] = new Emitter<Context>(this, width, gen_mul, alpha_mul, foam_mul, inscat_mul);
    return id;
  }

  void destroyEmitter(int id)
  {
    del_it(activeEmittersPool[id]);
    freeEmittersPool.push_back(id);
  }

  Segment *createSegment()
  {
    int requiredVertices = totalActiveVertices + g_settings.maxPointsPerSegment + 4;
    if (requiredVertices > g_settings.activeVertexCount)
    {
      static bool showWarn = true;
      if (showWarn)
      {
        showWarn = false;
        logwarn("water_trail, vbuffer overflow detected (active segments), req:%d, max:%d", requiredVertices,
          g_settings.activeVertexCount);
      }

      return NULL;
    }

    Segment *p = NULL;
    if (!freeSegmentsPool.empty())
    {
      p = freeSegmentsPool.back();
      freeSegmentsPool.pop_back();
    }

    if (!p)
      p = new Segment();

    v_bbox3_init_empty(p->box);
    p->id = activeSegmentsPool.size();
    p->owner = 0;
    activeSegmentsPool.push_back(p);
    totalActiveVertices += g_settings.maxPointsPerSegment + 4;

    return p;
  }

  void removeSegment(Segment *p)
  {
    totalActiveVertices -= g_settings.maxPointsPerSegment + 4;
    G_ASSERT(totalActiveVertices >= 0);

    Segment *endSeg = activeSegmentsPool.back();
    endSeg->id = p->id;
    activeSegmentsPool[p->id] = endSeg;
    activeSegmentsPool.pop_back();
  }

  void destroySegment(Segment *p)
  {
    totalFinalizedVertices -= p->points.size() + 4;
    G_ASSERT(totalFinalizedVertices >= 0);

    finalizedSegments[p->id] = finalizedSegments.back();
    finalizedSegments[p->id]->id = p->id;
    finalizedSegments.pop_back();

    freeSegmentsPool.push_back(p);
    p->id = -1;
    p->points.clear();
  }

  void finalizeSegment(Segment *p)
  {
    G_ASSERT(p->points.size() <= g_settings.finalizedVertexCount);

    int requiredVertices = totalFinalizedVertices + p->points.size() + 4;
    if (requiredVertices > g_settings.finalizedVertexCount)
    {
      static bool showWarn = true;
      if (showWarn)
      {
        showWarn = false;
        logwarn("water_trail, vbuffer overflow detected (finalized segments), req:%d, max:%d", requiredVertices,
          g_settings.finalizedVertexCount);
      }

      if (finalizedSegments.size() == 0)
      {
        logerr("water_trail, first finalized segment size:%d, while max size:%d", p->points.size(), g_settings.finalizedVertexCount);

        removeSegment(p);
        return;
      }

      Segment *oldSeg = finalizedSegments[0];
      for (int i = 1; i < finalizedSegments.size(); ++i)
      {
        Segment *s = finalizedSegments[i];
        oldSeg = oldSeg->lastGen < s->lastGen ? oldSeg : s;
      }

      destroySegment(oldSeg);
      finalizeSegment(p);
      return;
    }

    removeSegment(p);

    if (p->totalTurns * g_turns_to_points_ratio >= g_settings.maxPointsPerSegment)
    {
      p->forcedGenMul = 0.25f;
      float fadeOutTime = get_segment_fade_out_time(p);

      for (int i = 0; i < p->points.size(); ++i)
      {
        Vertex &v = p->points[i];
        v.prm.w = (v.prm.w > 0.0f ? 1.0f : -1.0f) / fadeOutTime;
      }
    }

    p->lastGen = currentGen;
    p->id = finalizedSegments.size();
    finalizedSegments.push_back(p);
    totalFinalizedVertices += p->points.size() + 4;
    needRebuild = true;
  }

  bool cleanupSegments()
  {
    bool updated = false;

    for (int i = 0; i < finalizedSegments.size(); ++i)
    {
      Segment *seg = finalizedSegments[i];
      if (currentGen > seg->lastGen + get_segment_fade_out_time(seg))
      {
        G_ASSERT(i == finalizedSegments[i]->id);
        destroySegment(seg);
        updated = true;
        --i;
      }
    }

    return updated;
  }

  void prepare(float dt, const Point3 &origin, const Frustum &frustum)
  {
    TIME_D3D_PROFILE(water_trail_prepare);

    currentOrigin = Point2::xz(origin);
    currentGen += dt;

    if (activeSegmentsPool.size() == 0 && finalizedSegments.size() == 0)
      currentGen = 0.f;

    if (renderEnabled && !renderer && g_settings.texSize > 0)
      renderer = new Renderer<Context>(this);
    else if (!renderEnabled || g_settings.texSize <= 0)
      del_it(renderer);

    if (!renderer)
    {
      static int texVarId = ::get_shader_variable_id("water_foam_trail");
      ShaderGlobal::set_texture(texVarId, BAD_TEXTUREID);
      return;
    }

    renderer->calcCascades();

    needRebuild |= cleanupSegments();
    // if ( needRebuild )
    // {
    renderer->rebuildBuffer(false, finalizedSegments, frustum);
    // needRebuild = false;
    // }

    renderer->rebuildBuffer(true, activeSegmentsPool, frustum);
  }

  bool render()
  {
    if (renderer)
      return renderer->render();
    return false;
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
  int curTexSize = g_settings.texSize;
  g_settings = s;
  if (g_settings.texSize > 0 && g_settings.texSize != curTexSize && g_ctx && g_ctx->renderer)
    g_ctx->renderer->resetTex();
}

int create_emitter(float width, float gen_mul, float alpha_mul, float foam_mul, float inscat_mul)
{
  if (!g_ctx)
    return -1;

  G_ASSERT(width > 0.f);
  G_ASSERT(gen_mul > 0.f);
  G_ASSERT(alpha_mul > 0.f);

  return g_ctx->createEmitter(width, gen_mul, alpha_mul, foam_mul, inscat_mul);
}

void destroy_emitter(int id)
{
  if (id >= 0 && g_ctx)
  {
    G_ASSERT_RETURN(id < g_ctx->activeEmittersPool.size(), );
    Emitter<Context> *em = g_ctx->activeEmittersPool[id];
    G_ASSERT_RETURN(em, );
    g_ctx->destroyEmitter(id);
  }
}

void emit_point(int id, const Point2 &pt, const Point2 &dir, float alpha, bool is_underwater)
{
  if (id < 0)
    return;

  G_ASSERT_RETURN(g_ctx, );
  G_ASSERT_RETURN(id < g_ctx->activeEmittersPool.size(), );
  Emitter<Context> *em = g_ctx->activeEmittersPool[id];

  G_ASSERT_RETURN(em, );
  if (is_underwater)
  {
    if (hasUnderwaterTrail())
    {
      const Point2 right = Point2{dir.y, -dir.x} * g_settings.underwaterFoamWidth;
      em->emitPointEx(pt, dir, -right, right, alpha * g_settings.underwaterFoamAlphaMult);
    }
  }
  else
    em->emitPoint(pt, dir, alpha);
}

void emit_point_ex(int id, const Point2 &pt, const Point2 &dir, const Point2 &left, const Point2 &right, float alpha)
{
  if (id < 0)
    return;

  G_ASSERT_RETURN(g_ctx, );
  G_ASSERT_RETURN(id < g_ctx->activeEmittersPool.size(), );
  Emitter<Context> *em = g_ctx->activeEmittersPool[id];

  G_ASSERT_RETURN(em, );
  em->emitPointEx(pt, dir, left, right, alpha);
}

void finalize_trail(int id)
{
  if (id < 0)
    return;

  G_ASSERT_RETURN(g_ctx, );
  G_ASSERT_RETURN(id < g_ctx->activeEmittersPool.size(), );
  Emitter<Context> *em = g_ctx->activeEmittersPool[id];

  G_ASSERT_RETURN(em, );
  em->finalize();
}

void before_render(float dt, const Point3 &origin, const Frustum &frustum)
{
  if (g_ctx)
    g_ctx->prepare(dt, origin, frustum);
}

bool render()
{
  if (g_ctx)
    return g_ctx->render();
  return false;
}

const Settings &get_settings() { return g_settings; }
} // namespace water_trail
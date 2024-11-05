// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <render/depthAOAboveRenderer.h>

#include <math/dag_frustum.h>
#include <math/dag_bounds2.h>
#include <memory/dag_framemem.h>

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>

#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_overrideStates.h>

#include <perfMon/dag_statDrv.h>
#include <render/toroidal_update_regions.h>

#define GLOBAL_VARS_LIST         \
  VAR(world_to_depth_ao)         \
  VAR(depth_ao_heights)          \
  VAR(depth_ao_tex_to_blur)      \
  VAR(depth_ao_texture_size)     \
  VAR(depth_ao_texture_rcp_size) \
  VAR(world_to_depth_ao_extra)   \
  VAR(depth_ao_heights_extra)    \
  VAR(blurred_depth)

#define GLOBAL_VARS_LIST_OPT                  \
  VAR(depth_around_transparent)               \
  VAR(depth_around_transparent_samplerstate)  \
  VAR(blurred_depth_transparent)              \
  VAR(blurred_depth_transparent_samplerstate) \
  VAR(depth_around_samplerstate)              \
  VAR(blurred_depth_samplerstate)             \
  VAR(heightmap_min_max)                      \
  VAR(depth_above_copy_layer)                 \
  VAR(depth_ao_extra_enabled)                 \
  VAR(depth_above_blur_layer)                 \
  VAR(depth_ao_tex_to_blur_samplerstate)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST_OPT
#undef VAR

static void init_shader_vars()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST_OPT
#undef VAR
}


DepthAOAboveRenderer::DepthAOAboveRenderer(const int tex_size, const float depth_around_distance, bool render_transparent,
  bool use_extra_cascade, float extra_cascade_mult) :
  texSize(tex_size), sceneMinMaxZ(0, 0), renderTransparent(render_transparent), extraCascadeMult(extra_cascade_mult)
{
  init_shader_vars();

  const int numCascades = use_extra_cascade ? 2 : 1;
  ShaderGlobal::set_int(depth_ao_extra_enabledVarId, use_extra_cascade ? 1 : 0);

  worldAODepth.close();
  worldAODepth = dag::create_array_tex(texSize, texSize, numCascades, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "depth_around");
  if (worldAODepth.getTexId() == BAD_TEXTUREID)
    logerr("DepthAOAboveRenderer: Failed to create 'depth_around' texture");
  else
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(depth_around_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(depth_ao_tex_to_blur_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(depth_around_transparent_samplerstateVarId, sampler);
    worldAODepth->disableSampler();
  }

  blurredDepth.close();
  blurredDepth = dag::create_array_tex(texSize, texSize, numCascades, TEXCF_RTARGET | TEXFMT_L16, 1, "blurred_depth");
  if (blurredDepth.getTexId() == BAD_TEXTUREID)
    logerr("DepthAOAboveRenderer: Failed to create 'blurred_depth' texture");
  else
    blurredDepth->disableSampler();

  if (renderTransparent)
  {
    copyRegionsRenderer.init("depth_above_copy_regions");

    worldAODepthWithTransparency.close();
    worldAODepthWithTransparency =
      dag::create_array_tex(texSize, texSize, numCascades, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "depth_around_transparent");
    if (worldAODepthWithTransparency.getTexId() == BAD_TEXTUREID)
      logerr("DepthAOAboveRenderer: Failed to create 'depth_around_transparent' texture");
    worldAODepthWithTransparency->disableSampler();

    blurredDepthWithTransparency.close();
    blurredDepthWithTransparency =
      dag::create_array_tex(texSize, texSize, numCascades, TEXCF_RTARGET | TEXFMT_L16, 1, "blurred_depth_transparent");
    if (blurredDepthWithTransparency.getTexId() == BAD_TEXTUREID)
      logerr("DepthAOAboveRenderer: Failed to create 'blurred_depth_transparent' texture");
    else
      blurredDepthWithTransparency->disableSampler();
  }
  else
  {
    ShaderGlobal::set_texture(depth_around_transparentVarId, worldAODepth);
    ShaderGlobal::set_texture(blurred_depth_transparentVarId, blurredDepth);
  }
  ShaderGlobal::set_sampler(blurred_depth_samplerstateVarId, d3d::request_sampler({}));
  ShaderGlobal::set_sampler(blurred_depth_transparent_samplerstateVarId, d3d::request_sampler({}));

  cascadeDependantData.resize(numCascades);
  for (int i = 0; i < cascadeDependantData.size(); ++i)
  {
    cascadeDependantData[i].worldAODepthData.curOrigin = IPoint2(-1000000, 1000000);
    cascadeDependantData[i].worldAODepthData.texSize = texSize;
    cascadeDependantData[i].depthAroundDistance = i == 0 ? depth_around_distance : depth_around_distance * extraCascadeMult;
  }


  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_ALWAYS;
  zFuncAlwaysStateId = shaders::overrides::create(state);
  state = shaders::OverrideState();
  state.set(shaders::OverrideState::Z_WRITE_ENABLE);
  zWriteOnStateId = shaders::overrides::create(state);
}

struct GatherRegionsCallback
{
  StaticTab<ToroidalQuadRegion, MAX_UPDATE_REGIONS> regions;
  GatherRegionsCallback() {}
  void renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom)
  {
    regions.push_back(ToroidalQuadRegion(lt, wd, texelsFrom));
  }

  void start(const IPoint2 &) {}
  void end() {}
};

// right side of box is not inclusive, left side is inclusive
template <class T>
inline void wrap_quads(IBBox2 box, int tex_sz, T &quads)
{
  if (box[1].y <= box[0].y || box[1].x <= box[0].x) // isempty for non-inclusive right side
    return;
  if (box[1].y < 0 || box[1].x < 0 || box[0].x >= tex_sz || box[0].y >= tex_sz)
    return;
  if (box[0].x < 0)
  {
    wrap_quads(IBBox2(IPoint2(0, box[0].y), box[1]), tex_sz, quads);
    wrap_quads(IBBox2(IPoint2(tex_sz + box[0].x, box[0].y), IPoint2(tex_sz, box[1].y)), tex_sz, quads);
    return;
  }
  if (box[1].x > tex_sz)
  {
    wrap_quads(IBBox2(box[0], IPoint2(tex_sz, box[1].y)), tex_sz, quads);
    wrap_quads(IBBox2(IPoint2(0, box[0].y), IPoint2(box[1].x - tex_sz, box[1].y)), tex_sz, quads);
    return;
  }
  if (box[0].y < 0)
  {
    wrap_quads(IBBox2(IPoint2(box[0].x, 0), box[1]), tex_sz, quads);
    wrap_quads(IBBox2(IPoint2(box[0].x, tex_sz + box[0].y), IPoint2(box[1].x, tex_sz)), tex_sz, quads);
    return;
  }
  if (box[1].y > tex_sz)
  {
    wrap_quads(IBBox2(box[0], IPoint2(box[1].x, tex_sz)), tex_sz, quads);
    wrap_quads(IBBox2(IPoint2(box[0].x, 0), IPoint2(box[1].x, box[1].y - tex_sz)), tex_sz, quads);
    return;
  }
  quads.push_back(box);
}

DepthAOAboveRenderer::BlurDepthRenderer::BlurDepthRenderer()
{
  CompiledShaderChannelId channels[] = {{SCTYPE_FLOAT2, SCUSAGE_POS, 0, 0}, {SCTYPE_FLOAT4, SCUSAGE_TC, 0, 0}};
  blurDepth.reset(nullptr);
  blurDepth = eastl::make_unique<DynamicShaderHelper>();
  blurDepth->init("single_pass_blur11", channels, countof(channels), 0, false);
}

DepthAOAboveRenderer::BlurDepthRenderer::~BlurDepthRenderer() = default;


void DepthAOAboveRenderer::BlurDepthRenderer::render(BaseTexture *target, TEXTUREID depth_tid, ToroidalHelper &worldAODepthData,
  Tab<Vertex> &tris, int cascade_no)
{

  d3d::set_render_target((Texture *)target, cascade_no, 0);
  ShaderGlobal::set_texture(depth_ao_tex_to_blurVarId, depth_tid);
  ShaderGlobal::set_real(depth_ao_texture_sizeVarId, worldAODepthData.texSize);
  ShaderGlobal::set_real(depth_ao_texture_rcp_sizeVarId, 1.0f / worldAODepthData.texSize);
  ShaderGlobal::set_int(depth_above_blur_layerVarId, cascade_no);
  blurDepth->shader->setStates(0, true);
  d3d::draw_up(PRIM_TRILIST, tris.size() / 3, tris.data(), elem_size(tris));
  d3d::resource_barrier({target, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
}


void DepthAOAboveRenderer::renderAODepthQuads(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb,
  BaseTexture *depthTex, RenderDepthAOType type, RenderDepthAOClearFirst clear_mode, int cascade_no)
{
  TIME_D3D_PROFILE(render_depth_above_quads);

  const CascadeDependantData &cascadeData = cascadeDependantData[cascade_no];

  const float fullDistance = 2.0f * cascadeData.depthAroundDistance;
  float texelSize = (fullDistance / texSize);

  d3d::set_render_target();
  d3d::set_render_target(0, (Texture *)NULL, 0);
  d3d::set_depth(depthTex, cascade_no, DepthAccess::RW);

  for (int i = 0; i < regions.size(); ++i)
  {
    const RegionToRender &region = regions[i];
    if (region.reg.wd.x <= 0 || region.reg.wd.y <= 0)
      continue;
    const IPoint2 &lt = region.reg.lt;
    const IPoint2 &wd = region.reg.wd;
    const IPoint2 viewLt(lt.x, texSize - lt.y - wd.y);
    const IPoint2 &texelsFrom = region.reg.texelsFrom;
    BBox2 reg(point2(texelsFrom) * texelSize, point2(texelsFrom + wd) * texelSize);
    d3d::setglobtm((mat44f_cref)region.cullViewProj);

    d3d::setview(viewLt.x, viewLt.y, wd.x, wd.y, 0, 1);
    if (clear_mode == RenderDepthAOClearFirst::Yes)
      d3d::clearview(CLEAR_ZBUFFER, 0, 0.0f, 0);

    if (type & RenderDepthAOType::Transparent)
      shaders::overrides::set(zWriteOnStateId);
    Point3 origin = Point3::xVy(reg.center(), 0.5 * (sceneMinMaxZ.x + sceneMinMaxZ.y));
    renderDepthCb.renderDepthAO(origin, (mat44f_cref)regions[i].cullViewProj, cascadeData.depthAroundDistance, i, type, cascade_no);
    if (type & RenderDepthAOType::Transparent)
      shaders::overrides::reset();
  }

  d3d::resource_barrier({depthTex, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
}

void DepthAOAboveRenderer::copyDepthAboveRegions(dag::ConstSpan<RegionToRender> regions, BaseTexture *depthTex, int cascade_no)
{
  TIME_D3D_PROFILE(copy_depth_above_regions)
  d3d::set_render_target();
  d3d::set_render_target(0, (Texture *)NULL, 0);
  shaders::overrides::set(zFuncAlwaysStateId);
  d3d::set_depth(depthTex, cascade_no, DepthAccess::RW);
  ShaderGlobal::set_int(depth_above_copy_layerVarId, cascade_no);

  for (const RegionToRender &region : regions)
  {
    if (region.reg.wd.x <= 0 || region.reg.wd.y <= 0)
      continue;

    TIME_D3D_PROFILE(copy_one_region)

    IPoint2 wd = region.reg.wd;
    IPoint2 lt = region.reg.lt;
    IPoint2 from = IPoint2(lt.x, texSize - lt.y - wd.y);

    d3d::setview(from.x, from.y, wd.x, wd.y, 0, 1);
    copyRegionsRenderer.render();
  }

  shaders::overrides::reset();
}

void DepthAOAboveRenderer::renderAODepthQuadsTransparent(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb,
  RenderDepthAOClearFirst clear_mode, int cascade_no)
{
  if (worldAODepthWithTransparency.getTexId() != BAD_TEXTUREID)
  {
    copyDepthAboveRegions(regions, worldAODepthWithTransparency.getArrayTex(), cascade_no);
    renderAODepthQuads(regions, renderDepthCb, worldAODepthWithTransparency.getArrayTex(), RenderDepthAOType::Transparent, clear_mode,
      cascade_no);
  }
}

void DepthAOAboveRenderer::renderAODepthQuads(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb, int cascade_no)
{
  if (!worldAODepth || !regions.size())
    return;

  TIME_D3D_PROFILE(depth_ao);
  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;
  const float dimensions = (sceneMinMaxZ.y - sceneMinMaxZ.x);
  ShaderGlobal::set_color4(heightmap_min_maxVarId, 1.f / dimensions, -sceneMinMaxZ.x / dimensions, dimensions, 0);

  CascadeDependantData &cascadeData = cascadeDependantData[cascade_no];

  renderAODepthQuads(regions, renderDepthCb, worldAODepth.getArrayTex(),
    (RenderDepthAOType)(RenderDepthAOType::Terrain | RenderDepthAOType::Opaque), RenderDepthAOClearFirst::Yes, cascade_no);
  renderAODepthQuadsTransparent(regions, renderDepthCb, RenderDepthAOClearFirst::No, cascade_no);

  TIME_D3D_PROFILE(depth_blur);
  Tab<IBBox2> blurBoxes(framemem_ptr());
  IPoint2 splitLine = (cascadeData.worldAODepthData.curOrigin - cascadeData.worldAODepthData.mainOrigin) % texSize;
  splitLine.y = -splitLine.y; // since we have flipped texture
  splitLine = (splitLine + IPoint2(texSize, texSize)) % texSize;
  for (int i = 0; i < regions.size(); ++i)
  {
    if (regions[i].reg.wd.x <= 0 || regions[i].reg.wd.y <= 0)
      continue;
    const IPoint2 &lt = regions[i].reg.lt;
    const IPoint2 &wd = regions[i].reg.wd;
    const IPoint2 viewLt(lt.x, texSize - lt.y - wd.y);
    const IPoint2 viewRb = viewLt + wd;
    const int blurKernel = 11; // we have 11x11 gaussian blur
    const int halfKernel = blurKernel / 2;
    const IPoint2 fblurLt =
      IPoint2(viewLt.x - (splitLine.x == viewLt.x ? 0 : halfKernel), viewLt.y - (splitLine.y == viewLt.y ? 0 : halfKernel));
    const IPoint2 fblurRb = IPoint2(viewRb.x + (splitLine.x == ((viewLt.x + wd.x) % texSize) ? 0 : halfKernel),
      viewRb.y + (splitLine.y == ((viewLt.y + wd.y) % texSize) ? 0 : halfKernel));

    // if unclamped (blurLt, blurWd) is out of texture, we actually have to render these (wrapped) regions either!
    // split quads
    StaticTab<IBBox2, 9> blurQuads;
    wrap_quads(IBBox2(fblurLt, fblurRb), texSize, blurQuads);
    for (int j = 0; j < blurQuads.size(); ++j)
      add_non_intersected_box(blurBoxes, IBBox2(blurQuads[j][0], blurQuads[j][1] - IPoint2(1, 1))); // we add inclusive ibbox2, so we
                                                                                                    // need to substract 1 from right
                                                                                                    // non-inclusive side
  }
  Tab<BlurDepthRenderer::Vertex> tris(framemem_ptr());
  for (int j = 0; j < blurBoxes.size(); ++j)
  {
    Point2 scrLt = mul(Point2::xy(blurBoxes[j][0]) / texSize, Point2(2, -2)) - Point2(1, -1);
    Point2 scrRb = mul(Point2::xy(blurBoxes[j][1] + IPoint2(1, 1)) / texSize, Point2(2, -2)) - Point2(1, -1);
    Point4 clampTc;
    clampTc = Point4((blurBoxes[j][0].x < splitLine.x) ? -1 : (splitLine.x + 0.5) / texSize,
      (blurBoxes[j][0].x < splitLine.x) ? (splitLine.x - 0.5) / texSize : 2,
      (blurBoxes[j][0].y < splitLine.y) ? -1 : (splitLine.y + 0.5) / texSize,
      (blurBoxes[j][0].y < splitLine.y) ? (splitLine.y - 0.5) / texSize : 2);
    BlurDepthRenderer::Vertex pt[6] = {{scrLt, clampTc}, {Point2(scrRb.x, scrLt.y), clampTc}, {Point2(scrLt.x, scrRb.y), clampTc},
      {Point2(scrLt.x, scrRb.y), clampTc}, {Point2(scrRb.x, scrLt.y), clampTc}, {scrRb, clampTc}};
    append_items(tris, 6, pt);
  }

  blurDepthRenderer.render(blurredDepth.getArrayTex(), worldAODepth.getTexId(), cascadeData.worldAODepthData, tris, cascade_no);
  if (renderTransparent)
    blurDepthRenderer.render(blurredDepthWithTransparency.getArrayTex(), worldAODepthWithTransparency.getTexId(),
      cascadeData.worldAODepthData, tris, cascade_no);
}

void DepthAOAboveRenderer::prepareRenderRegions(const Point3 &origin, float scene_min_z, float scene_max_z, float splitThreshold)
{
  for (int i = 0; i < cascadeDependantData.size(); ++i)
    prepareRenderRegionsForCascade(origin, scene_min_z, scene_max_z, splitThreshold, i);
}

void DepthAOAboveRenderer::prepareRenderRegionsForCascade(const Point3 &origin, float scene_min_z, float scene_max_z,
  float splitThreshold, int cascade_no)
{
  if (!worldAODepth)
    return;

  G_ASSERT(cascade_no < cascadeDependantData.size());
  G_ASSERT_RETURN(scene_min_z != scene_max_z, );
  sceneMinMaxZ = Point2(scene_min_z, scene_max_z);

  CascadeDependantData &cascadeData = cascadeDependantData[cascade_no];
  cascadeData.regionsToRender.clear();

  Point2 alignedOrigin = Point2::xz(origin);
  const float fullDistance = 2.0f * cascadeData.depthAroundDistance;
  float texelSize = (fullDistance / texSize);
  static constexpr int TEXEL_ALIGN = 4;
  IPoint2 newTexelsOrigin = (ipoint2(floor(alignedOrigin / (texelSize))) + IPoint2(TEXEL_ALIGN / 2, TEXEL_ALIGN / 2));
  newTexelsOrigin = newTexelsOrigin - (newTexelsOrigin % TEXEL_ALIGN);

  static constexpr int THRESHOLD = TEXEL_ALIGN * 4;
  IPoint2 move = abs(cascadeData.worldAODepthData.curOrigin - newTexelsOrigin);
  if (move.x >= THRESHOLD || move.y >= THRESHOLD)
  {
    TIME_D3D_PROFILE(depth_above);
    const float fullUpdateThreshold = 0.45;
    const int fullUpdateThresholdTexels = fullUpdateThreshold * texSize;
    if (max(move.x, move.y) < fullUpdateThresholdTexels) // if distance travelled is too big, there is no need to update movement in
                                                         // two steps
    {
      if (move.x < move.y)
        newTexelsOrigin.x = cascadeData.worldAODepthData.curOrigin.x;
      else
        newTexelsOrigin.y = cascadeData.worldAODepthData.curOrigin.y;
    }
    bool forcedUpdate = cascadeData.worldAODepthData.curOrigin.x == -1000000 && cascadeData.worldAODepthData.curOrigin.y == 1000000;
    GatherRegionsCallback cb;
    int texelsUpdated = toroidal_update(newTexelsOrigin, cascadeData.worldAODepthData, fullUpdateThresholdTexels, cb);
    toroidal_clip_regions(cascadeData.worldAODepthData, cascadeData.invalidAORegions);

    Point2 ofs = point2((cascadeData.worldAODepthData.mainOrigin - cascadeData.worldAODepthData.curOrigin) % texSize) / texSize;
    alignedOrigin = point2(cascadeData.worldAODepthData.curOrigin) * texelSize;

    int worldToAoVarId = cascade_no == 0 ? world_to_depth_aoVarId : world_to_depth_ao_extraVarId;
    int depthAoHeightsVarId = cascade_no == 0 ? depth_ao_heightsVarId : depth_ao_heights_extraVarId;
    ShaderGlobal::set_color4(worldToAoVarId, 1.0f / fullDistance, -1.0f / fullDistance, -alignedOrigin.x / fullDistance + 0.5,
      alignedOrigin.y / fullDistance + 0.5);

    ShaderGlobal::set_color4(depthAoHeightsVarId, sceneMinMaxZ.y - sceneMinMaxZ.x, sceneMinMaxZ.x, ofs.x, -ofs.y);

    bool bigUpdate = texelsUpdated > texSize * texSize * splitThreshold;
    if (splitThreshold != -1.0f && bigUpdate && !forcedUpdate)
    {
      for (ToroidalQuadRegion &r : cb.regions)
      {
        IBBox2 b{r.texelsFrom, r.texelsFrom + r.wd};
        toroidal_clip_region(cascadeData.worldAODepthData, b);
        add_non_intersected_box(cascadeData.invalidAORegions, b);
      }
    }
    else
    {
      for (ToroidalQuadRegion &r : cb.regions)
        cascadeData.regionsToRender.emplace_back(r);
    }
  }

  if (cascadeData.regionsToRender.empty())
  {
    int id = get_closest_region_and_split(cascadeData.worldAODepthData, cascadeData.invalidAORegions);
    if (id >= 0)
    {
      // split region if it is too big, along it's longest axis
      IBBox2 restRegion[4];
      int rest = 0;

      if (splitThreshold != -1.0f)
      {
        int desiredSide = sqrtf(texSize * texSize * splitThreshold);
        rest += split_region_to_size_linear(desiredSide, cascadeData.invalidAORegions[id], cascadeData.invalidAORegions[id],
          restRegion, cascadeData.worldAODepthData.curOrigin);
        rest += split_region_to_size_linear(desiredSide, cascadeData.invalidAORegions[id], cascadeData.invalidAORegions[id],
          &restRegion[rest], cascadeData.worldAODepthData.curOrigin);
      }
      else
      {
        rest += split_region_to_size(THRESHOLD * texSize, cascadeData.invalidAORegions[id], cascadeData.invalidAORegions[id],
          restRegion, cascadeData.worldAODepthData.curOrigin);
      }
      if (rest)
        append_items(cascadeData.invalidAORegions, rest, restRegion);

      // finally, update chosen invalid region.
      ToroidalQuadRegion quad(transform_point_to_viewport(cascadeData.invalidAORegions[id][0], cascadeData.worldAODepthData),
        cascadeData.invalidAORegions[id].width() + IPoint2(1, 1), cascadeData.invalidAORegions[id][0]);
      cascadeData.regionsToRender.emplace_back(quad);

      // and remove it from update list
      erase_items(cascadeData.invalidAORegions, id, 1);
    }
  }

  if (cascadeData.regionsToRender.empty())
    return;

  // prepare matrices
  SCOPE_VIEW_PROJ_MATRIX;

  TMatrix vtm = TMatrix::IDENT;
  vtm.setcol(0, 1, 0, 0);
  vtm.setcol(1, 0, 0, 1);
  vtm.setcol(2, 0, 1, 0);
  d3d::settm(TM_VIEW, vtm);

  for (RegionToRender &i : cascadeData.regionsToRender)
  {
    if (i.reg.wd.x <= 0 || i.reg.wd.y <= 0)
      continue;
    const IPoint2 &lt = i.reg.lt;
    const IPoint2 &wd = i.reg.wd;
    const IPoint2 viewLt(lt.x, texSize - lt.y - wd.y);
    const IPoint2 &texelsFrom = i.reg.texelsFrom;

    BBox2 region(point2(texelsFrom) * texelSize, point2(texelsFrom + wd) * texelSize);
    TMatrix4 proj;
    proj = matrix_ortho_off_center_lh(region[0].x, region[1].x, region[0].y, region[1].y, scene_min_z, scene_max_z);
    d3d::settm(TM_PROJ, &proj);
    i.cullViewProj = TMatrix4(vtm) * proj;
  }
}

void DepthAOAboveRenderer::renderPreparedRegions(IRenderDepthAOCB &renderDepthCb)
{
  for (int i = 0; i < cascadeDependantData.size(); ++i)
    renderAODepthQuads(cascadeDependantData[i].regionsToRender, renderDepthCb, i);
}

void DepthAOAboveRenderer::renderPreparedRegionsForCascade(IRenderDepthAOCB &renderDepthCb, int cascade_no)
{
  G_ASSERT(cascade_no < cascadeDependantData.size());
  renderAODepthQuads(cascadeDependantData[cascade_no].regionsToRender, renderDepthCb, cascade_no);
}

void DepthAOAboveRenderer::invalidateAO(bool force)
{
  for (auto &cascade : cascadeDependantData)
  {
    cascade.invalidAORegions.clear();
    if (!force)
    {
      cascade.invalidAORegions.push_back(get_texture_box(cascade.worldAODepthData));
      return;
    }
    // forced update!
    cascade.worldAODepthData.curOrigin = IPoint2(-1000000, 1000000);
  }
}

void DepthAOAboveRenderer::invalidateAO(const BBox3 &box)
{
  TIME_PROFILE_DEV(DepthAOAbove_invalidateBox);
  for (auto &cascade : cascadeDependantData)
  {
    const float fullDistance = 2.f * cascade.depthAroundDistance;
    const float texelSize = (fullDistance / texSize);
    IBBox2 ibox(ipoint2(floor(Point2::xz(box[0]) / texelSize)), ipoint2(ceil(Point2::xz(box[1]) / texelSize)));

    toroidal_clip_region(cascade.worldAODepthData, ibox);
    if (ibox.isEmpty())
      return;
    add_non_intersected_box(cascade.invalidAORegions, ibox);
  }
}

void DepthAOAboveRenderer::setDistanceAround(const float distance_around)
{
  if (cascadeDependantData[0].depthAroundDistance == distance_around)
    return;

  for (int i = 0; i < cascadeDependantData.size(); ++i)
    cascadeDependantData[i].depthAroundDistance = i == 0 ? distance_around : distance_around * extraCascadeMult;
  invalidateAO(true);
}

float DepthAOAboveRenderer::getTexelSize(int cascade_no) const
{
  G_ASSERT(cascade_no < cascadeDependantData.size());
  return 2.f * cascadeDependantData[cascade_no].depthAroundDistance / texSize;
}

void DepthAOAboveRenderer::setInvalidVars()
{
  ShaderGlobal::set_texture(worldAODepth.getVarId(), BAD_TEXTUREID);
  ShaderGlobal::set_texture(worldAODepthWithTransparency.getVarId(), BAD_TEXTUREID);
  ShaderGlobal::set_texture(blurredDepth.getVarId(), BAD_TEXTUREID);
  ShaderGlobal::set_texture(blurredDepthWithTransparency.getVarId(), BAD_TEXTUREID);
}

void DepthAOAboveRenderer::setVars()
{
  worldAODepth.setVar();
  blurredDepth.setVar();
  if (renderTransparent)
  {
    blurredDepthWithTransparency.setVar();
    worldAODepthWithTransparency.setVar();
  }
  else
  {
    ShaderGlobal::set_texture(depth_around_transparentVarId, worldAODepth);
    ShaderGlobal::set_texture(blurred_depth_transparentVarId, blurredDepth);
  }
  ShaderGlobal::set_real(depth_ao_texture_sizeVarId, texSize);
  ShaderGlobal::set_real(depth_ao_texture_rcp_sizeVarId, 1.0f / texSize);
}

bool DepthAOAboveRenderer::isValid() const
{
  bool valid = true;
  for (const auto &cascade : cascadeDependantData)
    valid &= cascade.regionsToRender.empty();
  return valid;
}
#define INSIDE_RENDERER 1 // fixme: move to jam

#include <render/depthAOAboveRenderer.h>

#include <math/dag_frustum.h>
#include <math/dag_bounds2.h>
#include <memory/dag_framemem.h>

#include <3d/dag_drv3d.h>

#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_DynamicShaderHelper.h>

#include <perfMon/dag_statDrv.h>
#include <render/toroidal_update_regions.h>

static int world_to_depth_aoVarId = -1;
static int depth_ao_heightsVarId = -1;
static int depth_ao_tex_to_blurVarId = -1;
static int depth_ao_texture_rcp_sizeVarId = -1;
static int depth_ao_texture_sizeVarId = -1;
static int depth_above_transparentVarId = -1;
static int blurred_depth_transparentVarId = -1;


DepthAOAboveRenderer::DepthAOAboveRenderer(const int texSize, const float depth_around_distance, bool render_transparent) :
  depthAroundDistance(depth_around_distance), sceneMinMaxZ(0, 0), renderTransparent(render_transparent)
{
  worldAODepth = dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "depth_around");
  worldAODepth->texfilter(TEXFILTER_POINT);
  blurredDepth = dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_L16, 1, "blurred_depth");


  depth_above_transparentVarId = ::get_shader_glob_var_id("depth_around_transparent", true);
  blurred_depth_transparentVarId = ::get_shader_glob_var_id("blurred_depth_transparent", true);
  if (renderTransparent)
  {
    copyRegionsRenderer.init("depth_above_copy_regions");
    worldAODepthWithTransparency =
      dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "depth_around_transparent");
    worldAODepthWithTransparency->texfilter(TEXFILTER_POINT);
    blurredDepthWithTransparency = dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_L16, 1, "blurred_depth_transparent");
  }
  else
  {
    ShaderGlobal::set_texture(depth_above_transparentVarId, worldAODepth);
    ShaderGlobal::set_texture(blurred_depth_transparentVarId, blurredDepth);
  }

  // worldAODepth.getTex2D()->texaddr(TEXADDR_WRAP);
  // displacementData.curOrigin = IPoint2(-1000000, 1000000);
  // displacementData.texSize = texSize;
  worldAODepthData.curOrigin = IPoint2(-1000000, 1000000);
  worldAODepthData.texSize = texSize;
  world_to_depth_aoVarId = ::get_shader_variable_id("world_to_depth_ao");
  depth_ao_heightsVarId = ::get_shader_glob_var_id("depth_ao_heights");
  depth_ao_tex_to_blurVarId = ::get_shader_glob_var_id("depth_ao_tex_to_blur");
  depth_ao_texture_sizeVarId = ::get_shader_glob_var_id("depth_ao_texture_size");
  depth_ao_texture_rcp_sizeVarId = ::get_shader_glob_var_id("depth_ao_texture_rcp_size");

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_ALWAYS;
  zFuncAlwaysStateId = shaders::overrides::create(state);
  state = shaders::OverrideState();
  state.set(shaders::OverrideState::Z_WRITE_ENABLE);
  zWriteOnStateId = shaders::overrides::create(state);
}

/*struct WorldDepthCallback
{
  float texelSize;
  DisplacementCallback(float texel, LandMeshRenderer &lmRenderer, LandMeshManager &lmMgr):texelSize(texel), cm(lmRenderer, lmMgr){}
  float minZ, maxZ;
  ViewProjMatrixContainer oviewproj;
  void start(const IPoint2 &texelOrigin)
  {
    Point2 center = point2(texelOrigin)*texelSize;
    Point3 pos(center.x, ::grs_cur_view.pos.y, center.y);
    BBox3 landBox = provider.getBBox();
    minZ = landBox[0].y-10;
    maxZ = landBox[1].y+10;

    TMatrix vtm = TMatrix::IDENT;
    d3d_get_view_proj(oviewproj);
    vtm.setcol(0, 1, 0, 0);
    vtm.setcol(1, 0, 0, 1);
    vtm.setcol(2, 0, 1, 0);
    d3d::settm(TM_VIEW, vtm);

  }
  virtual void end()
  {
    renderer.setRenderInBBox(BBox3());
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
    WorldRenderer::set_lmesh_rendering_mode(omode);
    d3d_set_view_proj(oviewproj);
  }

  virtual void renderTile(const BBox2 &region)
  {
    TMatrix4 proj;
    proj = matrix_ortho_off_center_lh(region[0].x, region[1].x,
                                      region[1].y, region[0].y,
                                      maxZ, minZ);
    d3d::settm(TM_PROJ, &proj);
    BBox3 region3(Point3(region[0].x, minZ, region[0].y), Point3(region[1].x, maxZ+500, region[1].y));
    renderer.setRenderInBBox(region3);
    //d3d::clearview(CLEAR_TARGET,0xFF00FF00, 0,0);
  }
  void renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom)
  {
    d3d::setview(lt.x, lt.y, wd.x, wd.y, 0, 1);
    d3d::clearview(CLEAR_ZBUFFER,0,0,0);
    BBox2 box(point2(texelsFrom)*texelSize, point2(texelsFrom+wd)*texelSize);
    cm.renderTile(box);
  }
};*/

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
  blurred_depthVarId = ::get_shader_variable_id("blurred_depth");
  CompiledShaderChannelId channels[] = {{SCTYPE_FLOAT2, SCUSAGE_POS, 0, 0}, {SCTYPE_FLOAT4, SCUSAGE_TC, 0, 0}};
  blurDepth.reset(nullptr);
  blurDepth = eastl::make_unique<DynamicShaderHelper>();
  blurDepth->init("single_pass_blur11", channels, countof(channels), 0, false);
}

DepthAOAboveRenderer::BlurDepthRenderer::~BlurDepthRenderer() = default;


void DepthAOAboveRenderer::BlurDepthRenderer::render(BaseTexture *target, TEXTUREID depth_tid, ToroidalHelper &worldAODepthData,
  Tab<Vertex> &tris)
{

  d3d::set_render_target((Texture *)target, 0);
  ShaderGlobal::set_texture(depth_ao_tex_to_blurVarId, depth_tid);
  ShaderGlobal::set_real(depth_ao_texture_sizeVarId, worldAODepthData.texSize);
  ShaderGlobal::set_real(depth_ao_texture_rcp_sizeVarId, 1.0f / worldAODepthData.texSize);
  blurDepth->shader->setStates(0, true);
  d3d::draw_up(PRIM_TRILIST, tris.size() / 3, tris.data(), elem_size(tris));
  d3d::resource_barrier({target, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
}


void DepthAOAboveRenderer::renderAODepthQuads(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb,
  BaseTexture *depthTex, bool transparent)
{
  TIME_D3D_PROFILE(render_depth_above_quads);

  const float fullDistance = 2.0f * depthAroundDistance;
  float texelSize = (fullDistance / worldAODepthData.texSize);

  d3d::set_render_target();
  d3d::set_render_target(0, (Texture *)NULL, 0);
  d3d::set_depth(depthTex, DepthAccess::RW);

  for (int i = 0; i < regions.size(); ++i)
  {
    auto region = regions[i];
    if (region.reg.wd.x <= 0 || region.reg.wd.y <= 0)
      continue;
    const IPoint2 &lt = region.reg.lt;
    const IPoint2 &wd = region.reg.wd;
    const IPoint2 viewLt(lt.x, worldAODepthData.texSize - lt.y - wd.y);
    const IPoint2 &texelsFrom = region.reg.texelsFrom;
    BBox2 reg(point2(texelsFrom) * texelSize, point2(texelsFrom + wd) * texelSize);
    d3d::setglobtm((mat44f_cref)region.cullViewProj);

    d3d::setview(viewLt.x, viewLt.y, wd.x, wd.y, 0, 1);
    if (!transparent)
      d3d::clearview(CLEAR_ZBUFFER, 0, 0.0f, 0);
    else
      shaders::overrides::set(zWriteOnStateId);
    Point3 origin = Point3::xVy(reg.center(), 0.5 * (sceneMinMaxZ.x + sceneMinMaxZ.y));
    renderDepthCb.renderDepthAO(origin, (mat44f_cref)regions[i].cullViewProj, depthAroundDistance, i, transparent);
    shaders::overrides::reset();
  }

  d3d::resource_barrier({depthTex, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
}

void DepthAOAboveRenderer::copyDepthAboveRegions(dag::ConstSpan<RegionToRender> regions)
{
  TIME_D3D_PROFILE(copy_depth_above_regions)
  d3d::set_render_target();
  d3d::set_render_target(0, (Texture *)NULL, 0);
  shaders::overrides::set(zFuncAlwaysStateId);
  d3d::set_depth(worldAODepthWithTransparency.getTex2D(), DepthAccess::RW);

  for (auto region : regions)
  {
    if (region.reg.wd.x <= 0 || region.reg.wd.y <= 0)
      continue;
    {
      TIME_D3D_PROFILE(copy_one_region)

      IPoint2 wd = region.reg.wd;
      IPoint2 lt = region.reg.lt;
      IPoint2 from = IPoint2(lt.x, worldAODepthData.texSize - lt.y - wd.y);

      d3d::setview(from.x, from.y, wd.x, wd.y, 0, 1);
      copyRegionsRenderer.render();
    }
  }

  shaders::overrides::reset();
}

void DepthAOAboveRenderer::renderAODepthQuadsTransparent(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb)
{
  if (worldAODepthWithTransparency.getTexId() != BAD_TEXTUREID)
  {
    copyDepthAboveRegions(regions);
    renderAODepthQuads(regions, renderDepthCb, worldAODepthWithTransparency.getTex2D(), true); // render transparent
  }
}

void DepthAOAboveRenderer::renderAODepthQuads(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb)
{
  if (!worldAODepth || !regions.size())
    return;

  TIME_D3D_PROFILE(depth_ao);
  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;
  const float dimensions = (sceneMinMaxZ.y - sceneMinMaxZ.x);
  ShaderGlobal::set_color4(get_shader_variable_id("heightmap_min_max", true), 1.f / dimensions, -sceneMinMaxZ.x / dimensions,
    dimensions, 0);

  renderAODepthQuads(regions, renderDepthCb, worldAODepth.getTex2D(), false);
  renderAODepthQuadsTransparent(regions, renderDepthCb);

  TIME_D3D_PROFILE(depth_blur);
  Tab<IBBox2> blurBoxes(framemem_ptr());
  IPoint2 splitLine = (worldAODepthData.curOrigin - worldAODepthData.mainOrigin) % worldAODepthData.texSize;
  splitLine.y = -splitLine.y; // since we have flipped texture
  splitLine = (splitLine + IPoint2(worldAODepthData.texSize, worldAODepthData.texSize)) % worldAODepthData.texSize;
  for (int i = 0; i < regions.size(); ++i)
  {
    if (regions[i].reg.wd.x <= 0 || regions[i].reg.wd.y <= 0)
      continue;
    const IPoint2 &lt = regions[i].reg.lt;
    const IPoint2 &wd = regions[i].reg.wd;
    const IPoint2 viewLt(lt.x, worldAODepthData.texSize - lt.y - wd.y);
    const IPoint2 viewRb = viewLt + wd;
    const int blurKernel = 11; // we have 11x11 gaussian blur
    const int halfKernel = blurKernel / 2;
    const IPoint2 fblurLt =
      IPoint2(viewLt.x - (splitLine.x == viewLt.x ? 0 : halfKernel), viewLt.y - (splitLine.y == viewLt.y ? 0 : halfKernel));
    const IPoint2 fblurRb = IPoint2(viewRb.x + (splitLine.x == ((viewLt.x + wd.x) % worldAODepthData.texSize) ? 0 : halfKernel),
      viewRb.y + (splitLine.y == ((viewLt.y + wd.y) % worldAODepthData.texSize) ? 0 : halfKernel));

    // if unclamped (blurLt, blurWd) is out of texture, we actually have to render these (wrapped) regions either!
    // split quads
    StaticTab<IBBox2, 9> blurQuads;
    wrap_quads(IBBox2(fblurLt, fblurRb), worldAODepthData.texSize, blurQuads);
    for (int j = 0; j < blurQuads.size(); ++j)
      add_non_intersected_box(blurBoxes, IBBox2(blurQuads[j][0], blurQuads[j][1] - IPoint2(1, 1))); // we add inclusive ibbox2, so we
                                                                                                    // need to substract 1 from right
                                                                                                    // non-inclusive side
  }
  Tab<BlurDepthRenderer::Vertex> tris(framemem_ptr());
  for (int j = 0; j < blurBoxes.size(); ++j)
  {
    Point2 scrLt = mul(Point2::xy(blurBoxes[j][0]) / worldAODepthData.texSize, Point2(2, -2)) - Point2(1, -1);
    Point2 scrRb = mul(Point2::xy(blurBoxes[j][1] + IPoint2(1, 1)) / worldAODepthData.texSize, Point2(2, -2)) - Point2(1, -1);
    Point4 clampTc;
    clampTc = Point4((blurBoxes[j][0].x < splitLine.x) ? -1 : (splitLine.x + 0.5) / worldAODepthData.texSize,
      (blurBoxes[j][0].x < splitLine.x) ? (splitLine.x - 0.5) / worldAODepthData.texSize : 2,
      (blurBoxes[j][0].y < splitLine.y) ? -1 : (splitLine.y + 0.5) / worldAODepthData.texSize,
      (blurBoxes[j][0].y < splitLine.y) ? (splitLine.y - 0.5) / worldAODepthData.texSize : 2);
    BlurDepthRenderer::Vertex pt[6] = {{scrLt, clampTc}, {Point2(scrRb.x, scrLt.y), clampTc}, {Point2(scrLt.x, scrRb.y), clampTc},
      {Point2(scrLt.x, scrRb.y), clampTc}, {Point2(scrRb.x, scrLt.y), clampTc}, {scrRb, clampTc}};
    append_items(tris, 6, pt);
  }

  blurDepthRenderer.render(blurredDepth.getTex2D(), worldAODepth.getTexId(), worldAODepthData, tris);
  if (renderTransparent)
    blurDepthRenderer.render(blurredDepthWithTransparency.getTex2D(), worldAODepthWithTransparency.getTexId(), worldAODepthData, tris);
}

void DepthAOAboveRenderer::prepareRenderRegions(const Point3 &origin, float scene_min_z, float scene_max_z, float splitThreshold)
{
  if (!worldAODepth)
    return;

  G_ASSERT_RETURN(scene_min_z != scene_max_z, );
  sceneMinMaxZ = Point2(scene_min_z, scene_max_z);

  regionsToRender.clear();

  Point2 alignedOrigin = Point2::xz(origin);
  const float fullDistance = 2.0f * depthAroundDistance;
  float texelSize = (fullDistance / worldAODepthData.texSize);
  static constexpr int TEXEL_ALIGN = 4;
  IPoint2 newTexelsOrigin = (ipoint2(floor(alignedOrigin / (texelSize))) + IPoint2(TEXEL_ALIGN / 2, TEXEL_ALIGN / 2));
  newTexelsOrigin = newTexelsOrigin - (newTexelsOrigin % TEXEL_ALIGN);

  static constexpr int THRESHOLD = TEXEL_ALIGN * 4;
  IPoint2 move = abs(worldAODepthData.curOrigin - newTexelsOrigin);
  if (move.x >= THRESHOLD || move.y >= THRESHOLD)
  {
    TIME_D3D_PROFILE(depth_above);
    const float fullUpdateThreshold = 0.45;
    const int fullUpdateThresholdTexels = fullUpdateThreshold * worldAODepthData.texSize;
    if (max(move.x, move.y) < fullUpdateThresholdTexels) // if distance travelled is too big, there is no need to update movement in
                                                         // two steps
    {
      if (move.x < move.y)
        newTexelsOrigin.x = worldAODepthData.curOrigin.x;
      else
        newTexelsOrigin.y = worldAODepthData.curOrigin.y;
    }
    bool forcedUpdate = worldAODepthData.curOrigin.x == -1000000 && worldAODepthData.curOrigin.y == 1000000;
    GatherRegionsCallback cb;
    int texelsUpdated = toroidal_update(newTexelsOrigin, worldAODepthData, fullUpdateThresholdTexels, cb);
    toroidal_clip_regions(worldAODepthData, invalidAORegions);

    Point2 ofs =
      point2((worldAODepthData.mainOrigin - worldAODepthData.curOrigin) % worldAODepthData.texSize) / worldAODepthData.texSize;
    alignedOrigin = point2(worldAODepthData.curOrigin) * texelSize;

    ShaderGlobal::set_color4(world_to_depth_aoVarId, 1.0f / fullDistance, -1.0f / fullDistance, -alignedOrigin.x / fullDistance + 0.5,
      alignedOrigin.y / fullDistance + 0.5);

    ShaderGlobal::set_color4(depth_ao_heightsVarId, sceneMinMaxZ.y - sceneMinMaxZ.x, sceneMinMaxZ.x, ofs.x, -ofs.y);

    bool bigUpdate = texelsUpdated > worldAODepthData.texSize * worldAODepthData.texSize * splitThreshold;
    if (splitThreshold != -1.0f && bigUpdate && !forcedUpdate)
    {
      for (ToroidalQuadRegion &r : cb.regions)
      {
        IBBox2 b{r.texelsFrom, r.texelsFrom + r.wd};
        toroidal_clip_region(worldAODepthData, b);
        add_non_intersected_box(invalidAORegions, b);
      }
    }
    else
    {
      for (ToroidalQuadRegion &r : cb.regions)
        regionsToRender.emplace_back(r);
    }
  }

  if (regionsToRender.empty())
  {
    int id = get_closest_region_and_split(worldAODepthData, invalidAORegions);
    if (id >= 0)
    {
      // split region if it is too big, along it's longest axis
      IBBox2 restRegion[4];
      int rest = 0;

      if (splitThreshold != -1.0f)
      {
        int desiredSide = sqrtf(worldAODepthData.texSize * worldAODepthData.texSize * splitThreshold);
        rest +=
          split_region_to_size_linear(desiredSide, invalidAORegions[id], invalidAORegions[id], restRegion, worldAODepthData.curOrigin);
        rest += split_region_to_size_linear(desiredSide, invalidAORegions[id], invalidAORegions[id], &restRegion[rest],
          worldAODepthData.curOrigin);
      }
      else
      {
        rest += split_region_to_size(THRESHOLD * worldAODepthData.texSize, invalidAORegions[id], invalidAORegions[id], restRegion,
          worldAODepthData.curOrigin);
      }
      if (rest)
        append_items(invalidAORegions, rest, restRegion);

      // finally, update chosen invalid region.
      ToroidalQuadRegion quad(transform_point_to_viewport(invalidAORegions[id][0], worldAODepthData),
        invalidAORegions[id].width() + IPoint2(1, 1), invalidAORegions[id][0]);
      regionsToRender.emplace_back(quad);

      // and remove it from update list
      erase_items(invalidAORegions, id, 1);
    }
  }

  if (regionsToRender.empty())
    return;

  // prepare matrices
  SCOPE_VIEW_PROJ_MATRIX;

  TMatrix vtm = TMatrix::IDENT;
  vtm.setcol(0, 1, 0, 0);
  vtm.setcol(1, 0, 0, 1);
  vtm.setcol(2, 0, 1, 0);
  d3d::settm(TM_VIEW, vtm);

  for (RegionToRender &i : regionsToRender)
  {
    if (i.reg.wd.x <= 0 || i.reg.wd.y <= 0)
      continue;
    const IPoint2 &lt = i.reg.lt;
    const IPoint2 &wd = i.reg.wd;
    const IPoint2 viewLt(lt.x, worldAODepthData.texSize - lt.y - wd.y);
    const IPoint2 &texelsFrom = i.reg.texelsFrom;

    BBox2 region(point2(texelsFrom) * texelSize, point2(texelsFrom + wd) * texelSize);
    TMatrix4 proj;
    proj = matrix_ortho_off_center_lh(region[0].x, region[1].x, region[0].y, region[1].y, scene_min_z, scene_max_z);
    d3d::settm(TM_PROJ, &proj);
    d3d::getglobtm(i.cullViewProj);
  }
}

void DepthAOAboveRenderer::prepareAO(const Point3 &origin, IRenderDepthAOCB &renderDepthCb)
{
  renderDepthCb.getMinMaxZ(sceneMinMaxZ.x, sceneMinMaxZ.y);
  prepareRenderRegions(origin, sceneMinMaxZ.x, sceneMinMaxZ.y);
  renderAODepthQuads(regionsToRender, renderDepthCb);
}

void DepthAOAboveRenderer::renderPreparedRegions(IRenderDepthAOCB &renderDepthCb)
{
  renderAODepthQuads(regionsToRender, renderDepthCb);
}

void DepthAOAboveRenderer::invalidateAO(bool force)
{
  invalidAORegions.clear();
  if (!force)
  {
    invalidAORegions.push_back(get_texture_box(worldAODepthData));
    return;
  }
  // forced update!
  worldAODepthData.curOrigin = IPoint2(-1000000, 1000000);
}

void DepthAOAboveRenderer::invalidateAO(const BBox3 &box)
{
  const float fullDistance = 2.f * depthAroundDistance;
  const float texelSize = (fullDistance / worldAODepthData.texSize);
  IBBox2 ibox(ipoint2(floor(Point2::xz(box[0]) / texelSize)), ipoint2(ceil(Point2::xz(box[1]) / texelSize)));

  toroidal_clip_region(worldAODepthData, ibox);
  if (ibox.isEmpty())
    return;
  add_non_intersected_box(invalidAORegions, ibox);
}

void DepthAOAboveRenderer::setDistanceAround(const float distance_around)
{
  if (depthAroundDistance == distance_around)
    return;
  depthAroundDistance = distance_around;
  invalidateAO(true);
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
    ShaderGlobal::set_texture(depth_above_transparentVarId, worldAODepth);
    ShaderGlobal::set_texture(blurred_depth_transparentVarId, blurredDepth);
  }
  ShaderGlobal::set_real(depth_ao_texture_sizeVarId, worldAODepthData.texSize);
  ShaderGlobal::set_real(depth_ao_texture_rcp_sizeVarId, 1.0f / worldAODepthData.texSize);
}
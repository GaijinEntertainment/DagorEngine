#include <math/dag_Point2.h>
#include <render/grassTranslucency.h>
#include <render/toroidal_update.h>
#include <math/dag_bounds2.h>
#include <shaders/dag_overrideStates.h>
/*class MyGrassTranlucencyCB : public GrassTranlucencyCB
{
public:
  int omode;
  uint32_t oldflags;
  void start(const BBox3 &box)
  {
    if (::grs_draw_wire)
      d3d::setwire(0);
    oldflags = LandMeshRenderer::lmesh_render_flags; omode = set_lmesh_rendering_mode(RENDERING_CLIPMAP);
    ::app->getCurrentScene()->lmeshRenderer->setRenderInBBox(box);
  }
  void renderTranslucencyColor()
  {
    LandMeshRenderer::lmesh_render_flags &= ~(LandMeshRenderer::RENDER_DECALS|LandMeshRenderer::RENDER_COMBINED);
    set_lmesh_rendering_mode(RENDERING_CLIPMAP);
    ::app->getCurrentScene()->lmeshRenderer->render(*::app->getCurrentScene()->lmeshMgr,
      LandMeshRenderer::RENDER_CLIPMAP);
  }
  void renderTranslucencyMask()
  {
    LandMeshRenderer::lmesh_render_flags = oldflags;
    set_lmesh_rendering_mode(GRASS_MASK);
    ::app->getCurrentScene()->lmeshRenderer->render(*::app->getCurrentScene()->lmeshMgr,
      LandMeshRenderer::RENDER_GRASS_MASK);
  }
  void finish()
  {
    LandMeshRenderer::lmesh_render_flags = oldflags; set_lmesh_rendering_mode(omode);
    ::app->getCurrentScene()->lmeshRenderer->setRenderInBBox(BBox3());
  }
}*/

#include <3d/dag_drv3d.h>
#include <3d/dag_tex3d.h>
#include <math/dag_Point2.h>
#include <shaders/dag_shaders.h>
#include <math/dag_TMatrix4.h>

int render_land_translucencyVarId = -1;
int translucency_vignetteVarId = -1;

void GrassTranslucency::close()
{
  grass_color_tex.close();
  grass_mask_tex.close();
  decode_grass_mask.clear();
}

void GrassTranslucency::setGrassAmount(int grass_amount)
{
  if (grassAmount == grass_amount)
    return;
  grassAmount = grass_amount;
  G_ASSERT(grass_tex_size > 0);
  recreateTex(grassAmount == NO ? 1 : grass_tex_size);

  ShaderGlobal::set_real(translucency_vignetteVarId, grassAmount == GRASSY ? 1 : 0);
}

void GrassTranslucency::recreateTex(int sz)
{
  TextureInfo tinfo;
  if (grass_color_tex.getTex2D())
  {
    grass_color_tex.getTex2D()->getinfo(tinfo, 0);
    if (tinfo.w == tinfo.h && tinfo.h == sz)
      return;
  }

  grass_color_tex.close();
  grass_mask_tex.close();
  grass_color_tex.set(d3d::create_tex(NULL, sz, sz, TEXCF_SRGBWRITE | TEXCF_SRGBREAD | TEXCF_RTARGET, 1, "grass_color_tex"),
    "grass_color_tex");

  if (sz > 1)
  {
    uint32_t l8fmt = TEXFMT_L8;
    if (!(d3d::get_texformat_usage(l8fmt) & d3d::USAGE_RTARGET))
      l8fmt = 0;
    grass_mask_tex.set(d3d::create_tex(NULL, sz, sz, l8fmt | TEXCF_RTARGET, 1, "grass_mask_tex"), "grass_mask_tex");
    grass_mask_tex.getTex2D()->texfilter(TEXFILTER_POINT);
  }

  torHelper.texSize = sz;
}

void GrassTranslucency::init(int tex_size)
{
  close();
  grass_tex_size = tex_size;
  recreateTex(grass_tex_size);
  grassAmount = GRASSY;
  last_grass_color_box.setempty();
  decode_grass_mask.init("decode_grass_mask");
  render_land_translucencyVarId = get_shader_variable_id("render_land_translucency", true);
  translucency_vignetteVarId = get_shader_variable_id("translucency_vignette", true);
  torHelper.curOrigin = IPoint2(-1000000, 100000);

  shaders::OverrideState state;
  state.set(shaders::OverrideState::FLIP_CULL);
  flipCullOverride.reset(shaders::overrides::create(state));
  state.colorWr = WRITEMASK_ALPHA;
  flipCullAlphaOverride.reset(shaders::overrides::create(state));
}

bool GrassTranslucency::update(const Point3 &view_pos, float half_size, GrassTranlucencyCB &cb)
{
  if (!grass_mask_tex.getTex2D())
    return false;
  if (grassAmount == NO && last_grass_color_box.isempty())
  {
    Driver3dRenderTarget prevRt;

    d3d::get_render_target(prevRt);
    d3d::set_render_target(grass_color_tex.getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    d3d::set_render_target(prevRt);
    grass_color_tex.setVar();
    last_grass_color_box = BBox3(Point3(-1, -10000, -1), Point3(+1, +10000, +1));
  }
  Point2 origin = Point2::xz(view_pos);
  float texelSize = 2 * half_size / grass_tex_size;
  origin = Point2(floorf(origin.x / texelSize + 0.5f) * texelSize, floorf(origin.y / texelSize + 0.5f) * texelSize);

  IPoint2 newTexelOrigin = ipoint2(floor(origin / texelSize));
  int texelThreshold = 0.1f * grass_tex_size;
  bool forceUpdate = last_grass_color_box.isempty();
  forceUpdate |= fabsf(last_grass_color_box.width().x - half_size * 2.f) > 1.f;

  if (forceUpdate)
    torHelper.curOrigin = IPoint2(-1000000, 100000);

  IPoint2 move = abs(torHelper.curOrigin - newTexelOrigin);
  if (forceUpdate || move.x >= texelThreshold || move.y >= texelThreshold)
  {
    if (!forceUpdate && move.x < texelThreshold)
      newTexelOrigin.x = torHelper.curOrigin.x;
    if (!forceUpdate && move.y < texelThreshold)
      newTexelOrigin.y = torHelper.curOrigin.y;
  }
  else
  {
    return false;
  }


  ToroidalGatherCallback::RegionTab regions;
  ToroidalGatherCallback torCb(regions);
  toroidal_update(newTexelOrigin, torHelper, 0.33f * grass_tex_size, torCb);

  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  TMatrix vtm;
  vtm.setcol(0, 1, 0, 0);
  vtm.setcol(1, 0, 0, 1);
  vtm.setcol(2, 0, 1, 0);
  vtm.setcol(3, 0, 0, 0);
  d3d::settm(TM_VIEW, orthonormalized_inverse(vtm));

  origin.x = torHelper.curOrigin.x * texelSize;
  origin.y = torHelper.curOrigin.y * texelSize;

  BBox3 fullBox =
    BBox3(Point3(origin.x - half_size, -1000, origin.y - half_size), Point3(origin.x + half_size, +1000, origin.y + half_size));
  last_grass_color_box = fullBox;

  Point2 ofs = point2((torHelper.mainOrigin - torHelper.curOrigin) % torHelper.texSize) / torHelper.texSize;

  static int world_to_grass_colorVarId = get_shader_variable_id("world_to_grass_color");
  static int world_to_grass_color_ofsVarId = get_shader_variable_id("world_to_grass_color_ofs");
  static int grass_mask_tex_viewportVarId = get_shader_variable_id("grass_mask_tex_viewport");

  ShaderGlobal::set_color4(world_to_grass_colorVarId, 1.0 / fullBox.width().x, 1.0 / fullBox.width().z,
    -fullBox[0].x / fullBox.width().x, -fullBox[0].z / fullBox.width().z);

  ShaderGlobal::set_color4(world_to_grass_color_ofsVarId, -ofs.x, -ofs.y, 0, 0);

  float invTexSize = safeinv((float)grass_tex_size);

  for (int i = 0; i < regions.size(); ++i)
  {
    const ToroidalQuadRegion &reg = regions[i];
    BBox2 box(point2(reg.texelsFrom) * texelSize, point2(reg.texelsFrom + reg.wd) * texelSize);

    BBox3 box3(Point3::xVy(box[0], -1000), Point3::xVy(box[1], +1000));
    TMatrix4 proj = matrix_ortho_off_center_lh(box3[0].x, box3[1].x, box3[1].z, box3[0].z, box3[0].y, box3[1].y);

    ShaderGlobal::set_color4(grass_mask_tex_viewportVarId, reg.lt.x * invTexSize, reg.lt.y * invTexSize, reg.wd.x * invTexSize,
      reg.wd.y * invTexSize);

    d3d::settm(TM_PROJ, &proj);

    cb.start(last_grass_color_box);

    d3d::set_render_target(grass_color_tex.getTex2D(), 0);
    d3d::setview(reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    shaders::overrides::set(flipCullOverride);
    cb.renderTranslucencyColor();
    shaders::overrides::reset();

    d3d::set_render_target(grass_mask_tex.getTex2D(), 0);
    d3d::setview(reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    shaders::overrides::set(flipCullAlphaOverride);
    ShaderGlobal::set_int(render_land_translucencyVarId, 1);

    cb.renderTranslucencyMask();
    grass_mask_tex.setVar();
    ShaderGlobal::set_int(render_land_translucencyVarId, 0);
    d3d::set_render_target(grass_color_tex.getTex2D(), 0);
    d3d::setview(reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);
    decode_grass_mask.render();

    // restore
    // app->getCurrentScene()->lmeshRenderer->forceTrivial(wasTrivial);

    shaders::overrides::reset();

    cb.finish();
  }

  grass_color_tex.setVar();
  return true;
}

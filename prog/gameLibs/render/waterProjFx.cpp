#include <render/waterProjFx.h>
#include <render/viewVecs.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>

#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_frustum.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_render.h>

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <generic/dag_carray.h>

#define MIN_WAVE_HEIGHT               3.0f
#define CAMERA_PLANE_ELEVATION        3.0f
#define CAMERA_PLANE_BOTTOM_MIN_ANGLE 0.999f


WaterProjectedFx::WaterProjectedFx(int tex_width, int tex_height, bool temporal_repojection) :
  temporalRepojection(temporal_repojection),
  newProjTM(TMatrix4::IDENT),
  newProjTMJittered(TMatrix4::IDENT),
  newViewTM(TMatrix::IDENT),
  newViewItm(TMatrix::IDENT),
  waterLevel(0),
  numIntersections(0),
  savedCamPos(0, 0, 0)
{
  combinedWaterFoam.init("combined_water_proj_fx");

  // create screen size div N texture
  unsigned texFlags = TEXCF_SRGBREAD | TEXCF_SRGBWRITE | TEXCF_RTARGET;

  // some Adreno drivers fail to properly load src color
  // at blending while using indirect draw in some conditions
  // when format is ARGB8U, use ARGB16F as workaround
  if (d3d::get_driver_desc().issues.hasRenderPassClearDataRace)
    texFlags |= TEXFMT_A16B16G16R16F;
  else
    texFlags |= TEXFMT_A8R8G8B8;

  waterProjectionFxTexWidth = tex_width;
  waterProjectionFxTexHeight = tex_height;

  globalFrameId = ShaderGlobal::getBlockId("global_frame");

  String texName(0, "projected_on_water_effects_tex");
  waterProjectionFxTex = dag::create_tex(NULL, waterProjectionFxTexWidth, waterProjectionFxTexHeight, texFlags, 1, texName.str());
  if (waterProjectionFxTex)
  {
    waterProjectionFxTex->texfilter(TEXFILTER_SMOOTH);
    waterProjectionFxTex->texaddr(TEXADDR_CLAMP);
  }

  prevGlobTm = TMatrix4::IDENT;

  if (temporalRepojection)
  {
    rtTemp = RTargetPool::get(waterProjectionFxTexWidth, waterProjectionFxTexHeight, texFlags, 1);
  }
}

void WaterProjectedFx::setTexture() { waterProjectionFxTex.setVar(); }

void WaterProjectedFx::setView() { setView(newViewTM, newProjTM, newViewItm); }

bool WaterProjectedFx::getView(TMatrix4 &view_tm, TMatrix4 &proj_tm, Point3 &camera_pos)
{
  if (!isValidView())
    return false;

  view_tm = newViewTM;
  proj_tm = newProjTM;
  camera_pos = newViewItm.getcol(3);
  return true;
}

bool WaterProjectedFx::isValidView() const { return numIntersections > 0 && savedCamPos.y > waterLevel; }

void WaterProjectedFx::prepare(const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, float water_level,
  float significant_wave_height, int frame_no)
{
  waterLevel = water_level;
  numIntersections = 0;

  newViewTM = view_tm;
  newProjTM = proj_tm;
  newViewItm = ::grs_cur_view.itm;
  savedCamPos = ::grs_cur_view.pos;

  float wavesDeltaH = max(significant_wave_height * 2.2f, MIN_WAVE_HEIGHT);
  float waterHeightTop = water_level + wavesDeltaH;
  float waterHeightBottom = water_level - wavesDeltaH;

  Point3 cameraDir = newViewItm.getcol(2);
  Point3 cameraPos = newViewItm.getcol(3);
  Point4 bottomPlane;
  v_stu(&bottomPlane.x, Frustum(glob_tm).camPlanes[Frustum::BOTTOM]);
  float cosA = min(bottomPlane.y, CAMERA_PLANE_BOTTOM_MIN_ANGLE);
  cameraPos += -normalize(Point3(cameraDir.x, 0.0f, cameraDir.z)) * max(waterHeightTop + CAMERA_PLANE_ELEVATION - cameraPos.y, 0.0f) *
               safediv(cosA, safe_sqrt(1.0f - SQR(cosA)));
  cameraPos.y = max(cameraPos.y, waterHeightTop + CAMERA_PLANE_ELEVATION);
  newViewItm.setcol(3, cameraPos);

  newViewTM = orthonormalized_inverse(newViewItm);
  TMatrix4 currentGlobTm = TMatrix4(newViewTM) * newProjTM;

  // We have current camera frustum - find intersection with water bounds (upper & lower water planes)
  int i, j;
  vec4f points[8];
  Point3 frustumPoints[8];
  Frustum frustum = Frustum(currentGlobTm);
  // Points order (it is important) - for edges_vid
  // -1 1 1
  // -1 1 0
  // -1 -1 1
  // -1 -1 0
  // 1 1 1
  // 1 1 0
  // 1 -1 1
  // 1 -1 0
  frustum.generateAllPointFrustm(points);

  Point4 p;
  for (i = 0; i < 8; i++)
  {
    v_stu(&p.x, points[i]);
    frustumPoints[i] = Point3::xyz(p);
  }
  int edges_vid[] = {0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 4, 6, 6, 2, 2, 0, 1, 5, 5, 7, 7, 3, 3, 1};

  // Total 12 frustum edges in frustum * 2 planes
  carray<Point3, 12 + 12> intersectionPoints;

  for (j = 0; j < 2; j++)
  {
    float waterPlaneHeight = j == 0 ? waterHeightTop : waterHeightBottom;
    for (i = 0; i < 24; i += 2)
    {
      // Frustum edge
      Point3 &p1 = frustumPoints[edges_vid[i]];
      Point3 &p2 = frustumPoints[edges_vid[i + 1]];

      float planeDist1 = p1.y - waterPlaneHeight;
      float planeDist2 = p2.y - waterPlaneHeight;

      if (planeDist1 * planeDist2 < 0.f) // Points are opposite side of the plane? - then edge intersects the plane
      {
        float k = safediv(planeDist1, planeDist1 - planeDist2); // Should be positive
        Point3 intersectionPoint = p1 + (p2 - p1) * k;          // Frustum-edge intersection point with water plane
        intersectionPoint.y = water_level;                      // Project point exactly on plane which we use for rendering
        intersectionPoints[numIntersections++] = intersectionPoint;
      }
    }
  }

  if (numIntersections > 0)
  {
    // Project all points on screen & calculate x&y bounds
    Point2 boxMin = Point2(10000.f, 10000.f);
    Point2 boxMax = Point2(-10000.f, -10000.f);
    for (i = 0; i < numIntersections; i++)
    {
      currentGlobTm.transform(intersectionPoints[i], p);
      boxMin.x = min(boxMin.x, safediv(p.x, p.w));
      boxMin.y = min(boxMin.y, safediv(p.y, p.w));

      boxMax.x = max(boxMax.x, safediv(p.x, p.w));
      boxMax.y = max(boxMax.y, safediv(p.y, p.w));
    }
    boxMin.x = clamp(boxMin.x, -2.0f, 2.0f);
    boxMin.y = clamp(boxMin.y, -2.0f, 2.0f);
    boxMax.x = clamp(boxMax.x, -2.0f, 2.0f) + 0.001f;
    boxMax.y = clamp(boxMax.y, -2.0f, 2.0f) + 0.001f;
    TMatrix4 cropMatrix = matrix_perspective_crop(boxMin.x, boxMax.x, boxMin.y, boxMax.y, 0.0f, 1.0f);
    newProjTM = newProjTM * cropMatrix;

    const float watJitterx = 0.2f / ((float)waterProjectionFxTexWidth);
    const float watJittery = 0.2f / ((float)waterProjectionFxTexHeight);

    float xShift = (float)(frame_no % 2);
    float yShift = (float)((frame_no % 4) / 2);
    curJitter = Point2((-3.0f + 4.0f * xShift + 2.0f * yShift) * watJitterx, (-1.0f - 2.0f * xShift + 4.0f * yShift) * watJittery);
    TMatrix4 jitterMatrix;
    jitterMatrix.setcol(0, 1.f, 0.f, 0.f, 0.f);
    jitterMatrix.setcol(1, 0.f, 1.f, 0.f, 0.f);
    jitterMatrix.setcol(2, 0.f, 0.f, 1.f, 0.f);
    jitterMatrix.setcol(3, curJitter.x, curJitter.y, 0.f, 1.f);
    newProjTMJittered = newProjTM * jitterMatrix.transpose();
  }

  currentGlobTm = TMatrix4(newViewTM) * newProjTM;
  process_tm_for_drv_consts(currentGlobTm);
  setWaterMatrix(currentGlobTm);
}

void WaterProjectedFx::clear()
{
  SCOPE_RENDER_TARGET;
  d3d::set_render_target(waterProjectionFxTex.getTex2D(), 0);
  d3d::clearview(CLEAR_TARGET, 0xFF000000, 0.f, 0);
  d3d::resource_barrier({waterProjectionFxTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}

void WaterProjectedFx::render(IWwaterProjFxRenderHelper *render_helper)
{
  G_ASSERT(render_helper);

  if (!isValidView())
  {
    setWaterMatrix(TMatrix4::IDENT);
    return;
  }

  RTarget::Ptr rtTemp0;
  if (temporalRepojection)
  {
    rtTemp0 = rtTemp->acquire();
    rtTemp0->getTex2D()->texfilter(TEXFILTER_SMOOTH);
    rtTemp0->getTex2D()->texaddr(TEXADDR_CLAMP);
  }

  SCOPE_VIEW_PROJ_MATRIX;
  DagorCurView savedView = ::grs_cur_view;

  setView(newViewTM, newProjTMJittered, newViewItm);

  SCOPE_RENDER_TARGET;
  d3d::set_render_target(temporalRepojection ? rtTemp0->getTex2D() : waterProjectionFxTex.getTex2D(), 0);
  d3d::clearview(CLEAR_TARGET, 0xFF000000, 0.f, 0);

  static int renderWaterProjectibleDecalsVarId = get_shader_variable_id("render_water_projectible_decals", true);
  ShaderGlobal::set_int(renderWaterProjectibleDecalsVarId, 1);

  render_helper->render_geometry();
  if (!temporalRepojection)
    render_helper->render_geometry_without_aa();

  ShaderGlobal::set_int(renderWaterProjectibleDecalsVarId, 0);
  ::grs_cur_view = savedView;

  if (temporalRepojection)
  {
    RTarget::Ptr rtTemp1 = rtTemp->acquire();
    rtTemp1->getTex2D()->texfilter(TEXFILTER_SMOOTH);
    rtTemp1->getTex2D()->texaddr(TEXADDR_CLAMP);

    static const int prev_globtm0VarId = ::get_shader_variable_id("prev_globtm_0");
    static const int prev_globtm1VarId = ::get_shader_variable_id("prev_globtm_1");
    static const int prev_globtm2VarId = ::get_shader_variable_id("prev_globtm_2");
    static const int prev_globtm3VarId = ::get_shader_variable_id("prev_globtm_3");
    static const int world_view_posVarId = ::get_shader_variable_id("world_view_pos");
    static const int cur_jitterVarId = ::get_shader_variable_id("cur_jitter");
    static const int wfx_prev_frame_texVarId = ::get_shader_variable_id("wfx_prev_frame_tex");
    static const int wfx_frame_texVarId = ::get_shader_variable_id("wfx_frame_tex");


    d3d::set_render_target(rtTemp1->getTex2D(), 0); // set render target

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    ShaderGlobal::set_texture(wfx_prev_frame_texVarId, waterProjectionFxTex.getTexId());
    ShaderGlobal::set_texture(wfx_frame_texVarId, rtTemp0->getTexId());

    ShaderGlobal::set_color4(prev_globtm0VarId, Color4(prevGlobTm[0]));
    ShaderGlobal::set_color4(prev_globtm1VarId, Color4(prevGlobTm[1]));
    ShaderGlobal::set_color4(prev_globtm2VarId, Color4(prevGlobTm[2]));
    ShaderGlobal::set_color4(prev_globtm3VarId, Color4(prevGlobTm[3]));

    prevGlobTm = TMatrix4(newViewTM) * newProjTM;
    process_tm_for_drv_consts(prevGlobTm);

    set_viewvecs_to_shader(newViewTM, newProjTM);
    Color4 prevWorldViewPos = ShaderGlobal::get_color4(world_view_posVarId);
    ShaderGlobal::set_color4(world_view_posVarId, Color4::xyz1(newViewItm.getcol(3)));
    ShaderGlobal::set_color4(cur_jitterVarId, curJitter.x, curJitter.y, 0, 0);

    combinedWaterFoam.render();

    ShaderGlobal::setBlock(globalFrameId, ShaderGlobal::LAYER_FRAME);

    waterProjectionFxTex.getTex2D()->update(rtTemp1->getTex2D());

    d3d::set_render_target(waterProjectionFxTex.getTex2D(), 0);
    render_helper->render_geometry_without_aa();

    ShaderGlobal::set_color4(world_view_posVarId, prevWorldViewPos);
    ShaderGlobal::setBlock(globalFrameId, ShaderGlobal::LAYER_FRAME);
  }

  d3d::resource_barrier({waterProjectionFxTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  render_helper->finish_rendering();
}

void WaterProjectedFx::setView(const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix &view_itm)
{
  d3d::settm(TM_VIEW, view_tm);
  d3d::settm(TM_PROJ, &proj_tm);
  ::grs_cur_view.itm = view_itm;
  ::grs_cur_view.tm = view_tm;
  ::grs_cur_view.pos = view_itm.getcol(3);
}

void WaterProjectedFx::setWaterMatrix(const TMatrix4 &glob_tm)
{
  // set matrix to water shader
  static int waterEeffectsProjTmLine0VarId = get_shader_variable_id("water_effects_proj_tm_line_0", true);
  static int waterEeffectsProjTmLine1VarId = get_shader_variable_id("water_effects_proj_tm_line_1", true);
  static int waterEeffectsProjTmLine2VarId = get_shader_variable_id("water_effects_proj_tm_line_2", true);
  static int waterEeffectsProjTmLine3VarId = get_shader_variable_id("water_effects_proj_tm_line_3", true);

  ShaderGlobal::set_color4(waterEeffectsProjTmLine0VarId, Color4(glob_tm[0]));
  ShaderGlobal::set_color4(waterEeffectsProjTmLine1VarId, Color4(glob_tm[1]));
  ShaderGlobal::set_color4(waterEeffectsProjTmLine2VarId, Color4(glob_tm[2]));
  ShaderGlobal::set_color4(waterEeffectsProjTmLine3VarId, Color4(glob_tm[3]));
}

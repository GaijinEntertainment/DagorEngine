#include <render/waterProjFx.h>
#include <render/viewVecs.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_textureIDHolder.h>

#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_frustum.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_render.h>

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <generic/dag_carray.h>

#include <perfMon/dag_statDrv.h>

#define MIN_WAVE_HEIGHT               3.0f
#define CAMERA_PLANE_ELEVATION        3.0f
#define CAMERA_PLANE_BOTTOM_MIN_ANGLE 0.999f

WaterProjectedFx::WaterProjectedFx(int frame_width, int frame_height, dag::Span<TargetDesc> target_descs,
  const char *taa_reprojection_blend_shader_name, bool own_textures) :
  newProjTM(TMatrix4::IDENT),
  newProjTMJittered(TMatrix4::IDENT),
  newViewTM(TMatrix::IDENT),
  newViewItm(TMatrix::IDENT),
  waterLevel(0),
  numIntersections(0),
  savedCamPos(0, 0, 0),
  prevGlobTm(TMatrix4::IDENT),
  frameWidth(frame_width),
  frameHeight(frame_height),
  nTargets(target_descs.size()),
  taaEnabled(taa_reprojection_blend_shader_name),
  ownTextures(own_textures),
  globalFrameId(ShaderGlobal::getBlockId("global_frame")),
  targetsCleared(false)
{
  G_ASSERT_RETURN(nTargets <= MAX_TARGETS, );

  if (taaEnabled)
    taaRenderer.init(taa_reprojection_blend_shader_name);

  for (int i = 0; i < nTargets; ++i)
  {
    targetClearColors[i] = target_descs[i].clearColor;
    if (own_textures)
    {
      uint32_t texCreationFlags = getTargetAdditionalFlags() | (target_descs[i].texFmt & TEXFMT_MASK) | TEXCF_CLEAR_ON_CREATE;
      internalTargets[i] = dag::create_tex(NULL, frameWidth, frameHeight, texCreationFlags, 1, target_descs[i].texName);
      if (internalTargets[i])
      {
        internalTargets[i]->texfilter(TEXFILTER_SMOOTH);
        internalTargets[i]->texaddr(TEXADDR_CLAMP);
      }
      if (taaEnabled)
        taaRtTempPools[i] = RTargetPool::get(frameWidth, frameHeight, texCreationFlags, 1);
    }
  }
}

uint32_t WaterProjectedFx::getTargetAdditionalFlags() const
{
  uint32_t texFlags = TEXCF_SRGBREAD | TEXCF_SRGBWRITE | TEXCF_RTARGET;
  return texFlags;
}

void WaterProjectedFx::setTextures()
{
  G_ASSERT(ownTextures);
  for (int i = 0; i < nTargets; ++i)
    internalTargets[i].setVar();
}

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

bool WaterProjectedFx::isValidView() const { return numIntersections > 0; }

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
  cameraPos += -normalize(Point3(cameraDir.x, 0.0f, cameraDir.z)) *
               max(waterHeightTop + CAMERA_PLANE_ELEVATION - abs(cameraPos.y), 0.0f) * safediv(cosA, safe_sqrt(1.0f - SQR(cosA)));
  cameraPos.y = max(abs(cameraPos.y), waterHeightTop + CAMERA_PLANE_ELEVATION) * (cameraPos.y > 0 ? 1.0f : -1.0f);
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

    if (taaEnabled)
    {
      const float watJitterx = 0.2f / ((float)frameWidth);
      const float watJittery = 0.2f / ((float)frameHeight);

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
  }

  currentGlobTm = TMatrix4(newViewTM) * newProjTM;
  process_tm_for_drv_consts(currentGlobTm);
  setWaterMatrix(currentGlobTm);
}

void WaterProjectedFx::clear(bool forceClear)
{
  G_ASSERT(ownTextures);
  if (!forceClear && targetsCleared)
    return;

  SCOPE_RENDER_TARGET;
  for (int i = 0; i < nTargets; ++i)
  {
    d3d::set_render_target(internalTargets[i].getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, targetClearColors[i], 0.f, 0);
    d3d::resource_barrier({internalTargets[i].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }
  targetsCleared = true;
}

bool WaterProjectedFx::render(IWwaterProjFxRenderHelper *render_helper)
{
  G_ASSERT(ownTextures);
  G_ASSERT(render_helper);

  TextureIDPair internalTargetsTex[MAX_TARGETS];
  for (int i = 0; i < nTargets; ++i)
    internalTargetsTex[i] = {internalTargets[i].getTex2D(), internalTargets[i].getTexId()};

  bool renderedAnything = false;
  if (taaEnabled)
  {
    RTarget::Ptr rtTemp0[MAX_TARGETS];
    TextureIDPair rtTemp0Tex[MAX_TARGETS];
    RTarget::Ptr rtTemp1[MAX_TARGETS];
    TextureIDPair rtTemp1Tex[MAX_TARGETS];

    for (int i = 0; i < nTargets; ++i)
    {
      rtTemp0[i] = taaRtTempPools[i]->acquire();
      rtTemp0[i]->getTex2D()->texfilter(TEXFILTER_SMOOTH);
      rtTemp0[i]->getTex2D()->texaddr(TEXADDR_CLAMP);
      rtTemp0Tex[i] = {rtTemp0[i]->getTex2D(), rtTemp0[i]->getTexId()};
    }
    for (int i = 0; i < nTargets; ++i)
    {
      rtTemp1[i] = taaRtTempPools[i]->acquire();
      rtTemp1[i]->getTex2D()->texfilter(TEXFILTER_SMOOTH);
      rtTemp1[i]->getTex2D()->texaddr(TEXADDR_CLAMP);
      rtTemp1Tex[i] = {rtTemp1[i]->getTex2D(), rtTemp1[i]->getTexId()};
    }

    renderedAnything = render(render_helper, {internalTargetsTex, nTargets}, {rtTemp0Tex, nTargets}, {rtTemp1Tex, nTargets});
  }
  else
  {
    renderedAnything = render(render_helper, {internalTargetsTex, nTargets});
  }

  for (int i = 0; i < nTargets; ++i)
    d3d::resource_barrier({internalTargets[i].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  return renderedAnything;
}

bool WaterProjectedFx::render(IWwaterProjFxRenderHelper *render_helper, dag::Span<const TextureIDPair> targets,
  dag::Span<const TextureIDPair> taaTemp0, dag::Span<const TextureIDPair> taaTemp1)
{
  G_ASSERT(render_helper && nTargets <= MAX_TARGETS);
  G_ASSERT(targets.size() == nTargets && (taaTemp0.size() == nTargets && taaTemp1.size() == nTargets && taaEnabled ||
                                           taaTemp0.empty() && taaTemp1.empty() && !taaEnabled));

  if (!isValidView())
  {
    setWaterMatrix(TMatrix4::IDENT);
    return false;
  }

  // We don't need TAA if there is nothing to antialiase.
  // There will be a delay in one frame, but it's Ok, cause it's just one frame without AA.
  bool taaEnabledForThisFrame = taaEnabled && !targetsCleared;

  SCOPE_VIEW_PROJ_MATRIX;
  DagorCurView savedView = ::grs_cur_view;

  setView(newViewTM, taaEnabledForThisFrame ? newProjTMJittered : newProjTM, newViewItm);

  SCOPE_RENDER_TARGET;
  if (!targetsCleared)
  {
    TIME_D3D_PROFILE(clear);
    // Since d3d::clearview() clears all targets with the same color,
    // we have to clear each target separately.
    for (int i = 0; i < nTargets; ++i)
    {
      d3d::set_render_target(taaEnabledForThisFrame ? taaTemp0[i].getTex2D() : targets[i].getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, targetClearColors[i], 0.f, 0);
    }
  }
  for (int i = 0; i < nTargets; ++i)
    d3d::set_render_target(i, taaEnabledForThisFrame ? taaTemp0[i].getTex2D() : targets[i].getTex2D(), 0);

  static int renderWaterProjectibleDecalsVarId = get_shader_variable_id("render_water_projectible_decals", true);
  ShaderGlobal::set_int(renderWaterProjectibleDecalsVarId, 1);

  bool renderedAnything = false;
  renderedAnything |= render_helper->render_geometry();
  if (!taaEnabledForThisFrame)
    renderedAnything |= render_helper->render_geometry_without_aa();

  ShaderGlobal::set_int(renderWaterProjectibleDecalsVarId, 0);
  ::grs_cur_view = savedView;

  if (taaEnabledForThisFrame)
  {
    if (renderedAnything)
    {
      static const int prev_globtm0VarId = ::get_shader_variable_id("prev_globtm_0");
      static const int prev_globtm1VarId = ::get_shader_variable_id("prev_globtm_1");
      static const int prev_globtm2VarId = ::get_shader_variable_id("prev_globtm_2");
      static const int prev_globtm3VarId = ::get_shader_variable_id("prev_globtm_3");
      static const int world_view_posVarId = ::get_shader_variable_id("world_view_pos");
      static const int cur_jitterVarId = ::get_shader_variable_id("cur_jitter");

      for (int i = 0; i < nTargets; ++i)
        d3d::set_render_target(i, taaTemp1[i].getTex2D(), 0);

      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

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

      TEXTUREID prevFrameTargets[MAX_TARGETS];
      TEXTUREID curFrameTargets[MAX_TARGETS];
      for (int i = 0; i < nTargets; ++i)
      {
        prevFrameTargets[i] = targets[i].getId();
        curFrameTargets[i] = taaTemp0[i].getId();
      }
      render_helper->prepare_taa_reprojection_blend(prevFrameTargets, curFrameTargets);
      {
        TIME_D3D_PROFILE(taa);
        taaRenderer.render();
      }

      {
        TIME_D3D_PROFILE(copy_targets);
        for (int i = 0; i < nTargets; ++i)
          targets[i].getTex2D()->update(taaTemp1[i].getTex2D());
      }

      ShaderGlobal::set_color4(world_view_posVarId, prevWorldViewPos);
    }
    else if (!targetsCleared)
    {
      TIME_D3D_PROFILE(clear);
      for (int i = 0; i < nTargets; ++i)
      {
        d3d::set_render_target(targets[i].getTex2D(), 0);
        d3d::clearview(CLEAR_TARGET, targetClearColors[i], 0.f, 0);
      }
    }

    ShaderGlobal::setBlock(globalFrameId, ShaderGlobal::LAYER_FRAME);

    for (int i = 0; i < nTargets; ++i)
      d3d::set_render_target(i, targets[i].getTex2D(), 0);

    setView(newViewTM, newProjTM, newViewItm);
    renderedAnything |= render_helper->render_geometry_without_aa();

    ShaderGlobal::setBlock(globalFrameId, ShaderGlobal::LAYER_FRAME);
  }

  targetsCleared = !renderedAnything && ownTextures; // Preserve 'false' state if the textures aren't owned.

  return renderedAnything;
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

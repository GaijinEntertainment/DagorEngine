// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/waterProjFx.h>
#include <render/viewVecs.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
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

#define MIN_WAVE_HEIGHT               0.5f
#define MAX_WAKE_HEIGHT               1.0f
#define CAMERA_PLANE_ELEVATION        3.0f
#define CAMERA_PLANE_BOTTOM_MIN_ANGLE 0.999f
#define FRUSTUM_CROP_MIN_HEIGHT       0.6f


WaterProjectedFxRenderer::WaterProjectedFxRenderer() :
  newProjTM(TMatrix4::IDENT), newViewTM(TMatrix::IDENT), newViewItm(TMatrix::IDENT), waterLevel(0), numIntersections(0)
{}

WaterProjectedFx::WaterProjectedFx(int frame_width, int frame_height, dag::Span<TargetDesc> target_descs) :
  WaterProjectedFxRenderer(),
  frameWidth(frame_width),
  frameHeight(frame_height),
  nTargets(target_descs.size()),
  internalTargetsCleared(false),
  internalTargetsTexVarIds()
{
  G_ASSERT_RETURN(nTargets <= MAX_TARGETS, );

  for (int i = 0; i < nTargets; ++i)
  {
    targetClearColors[i] = target_descs[i].clearColor;
    internalTargetsTexVarIds[i] = ::get_shader_variable_id(target_descs[i].texName, true);

    uint32_t texCreationFlags = TEXCF_RTARGET | target_descs[i].texFmt;
    String emptyTexName(target_descs[i].texName);
    emptyTexName.append("_empty");
    emptyInternalTextures[i] = dag::create_tex(nullptr, 4, 4, texCreationFlags, 1, emptyTexName.c_str(), RESTAG_WATER);

    tempRTPools[i] = ResizableRTargetPool::get(frameWidth, frameHeight, texCreationFlags, 1);
  }

  // Since clear colors can be anything, we can't get by with just TEXCF_CLEAR_ON_CREATE.
  d3d::GpuAutoLock gpuLock;
  clear();
}

void WaterProjectedFx::setResolution(int frame_width, int frame_height)
{
  frameWidth = frame_width;
  frameHeight = frame_height;
  for (int i = 0; i < nTargets; ++i)
    if (internalTargets[i])
      internalTargets[i]->resize(frameWidth, frameHeight);
}

void WaterProjectedFx::setTextures()
{
  for (int i = 0; i < nTargets; ++i)
  {
    if (internalTargets[i])
    {
      ShaderGlobal::set_texture(internalTargetsTexVarIds[i], internalTargets[i]->getTexId());
    }
    else
    {
      ShaderGlobal::set_texture(internalTargetsTexVarIds[i], emptyInternalTextures[i].getTexId());
    }
  }
}

void WaterProjectedFxRenderer::setView() { setView(newViewTM, newProjTM, newViewItm.getcol(3)); }

bool WaterProjectedFxRenderer::getView(TMatrix4 &view_tm, TMatrix4 &proj_tm, Point3 &camera_pos)
{
  if (!isValidView())
    return false;

  view_tm = newViewTM;
  proj_tm = newProjTM;
  camera_pos = newViewItm.getcol(3);
  return true;
}

void WaterProjectedFxRenderer::calcIntersections(float water_level, float wavesDeltaH, const TMatrix4 &currentGlobTm,
  IntersectPointsVector &intersectionPoints) const
{
  float waterHeightTop = water_level + wavesDeltaH;
  float waterHeightBottom = water_level - wavesDeltaH;
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
        intersectionPoints.emplace_back(eastl::move(intersectionPoint));
      }
    }
  }
}

TMatrix4 WaterProjectedFxRenderer::calcCropedPerspective(const TMatrix4 &currentGlobTm,
  const IntersectPointsVector &intersectionPoints) const
{
  Point4 p;
  // Project all points on screen & calculate x&y bounds
  const float maxFloat = eastl::numeric_limits<float>::max();
  Point2 boxMin = Point2(maxFloat, maxFloat);
  Point2 boxMax = Point2(-maxFloat, -maxFloat);
  for (size_t i = 0; i < intersectionPoints.size(); i++)
  {
    currentGlobTm.transform(intersectionPoints[i], p);
    boxMin.x = min(boxMin.x, safediv(p.x, p.w));
    boxMin.y = min(boxMin.y, safediv(p.y, p.w));

    boxMax.x = max(boxMax.x, safediv(p.x, p.w));
    boxMax.y = max(boxMax.y, safediv(p.y, p.w));
  }

  // When camera is low the resulting frustum crop can sometimes degenerate into almost a line
  // which leads to some of the projected fx missing from the final image.
  // Having a minimum height for the frustum crop box helps to avoid this most of the time.
  float boxYAvg = (boxMin.y + boxMax.y) * 0.5f;
  float boxYSize = boxMax.y - boxMin.y;
  if (boxYSize < FRUSTUM_CROP_MIN_HEIGHT)
  {
    boxMin.y = boxYAvg - FRUSTUM_CROP_MIN_HEIGHT / 2.f;
    boxMax.y = boxYAvg + FRUSTUM_CROP_MIN_HEIGHT / 2.f;
  }

  boxMin.x = clamp(boxMin.x, -2.0f, 2.0f);
  boxMin.y = clamp(boxMin.y, -2.0f, 2.0f);
  boxMax.x = clamp(boxMax.x, -2.0f, 2.0f) + 0.001f;
  boxMax.y = clamp(boxMax.y, -2.0f, 2.0f) + 0.001f;

  return matrix_perspective_crop(boxMin.x, boxMax.x, boxMin.y, boxMax.y, 0.0f, 1.0f);
}


bool WaterProjectedFxRenderer::isValidView() const { return numIntersections > 0; }

void WaterProjectedFxRenderer::prepare(const TMatrix &view_tm, const TMatrix &view_itm, const TMatrix4 &proj_tm,
  const TMatrix4 &glob_tm, float water_level, float significant_wave_height, bool change_projection)
{
  waterLevel = water_level;
  numIntersections = 0;

  newViewTM = view_tm;
  newProjTM = proj_tm;
  newViewItm = view_itm;

  if (change_projection)
  {
    float wavesDeltaH = max(significant_wave_height * 2.2f + MAX_WAKE_HEIGHT, MIN_WAVE_HEIGHT);
    float waterHeightTop = water_level + wavesDeltaH;

    Point3 cameraDir = newViewItm.getcol(2);
    Point3 cameraPos = newViewItm.getcol(3);
    Point4 bottomPlane;
    v_stu(&bottomPlane.x, Frustum(glob_tm).camPlanes[Frustum::BOTTOM]);
    float cosA = min(bottomPlane.y, CAMERA_PLANE_BOTTOM_MIN_ANGLE);
    cameraPos += -normalize(Point3(cameraDir.x, 0.0f, cameraDir.z)) *
                 max(waterHeightTop + CAMERA_PLANE_ELEVATION - abs(cameraPos.y), 0.0f) * safediv(cosA, safe_sqrt(1.0f - sqr(cosA)));
    cameraPos.y = max(abs(cameraPos.y), waterHeightTop + CAMERA_PLANE_ELEVATION) * (cameraPos.y > 0 ? 1.0f : -1.0f);
    newViewItm.setcol(3, cameraPos);

    newViewTM = orthonormalized_inverse(newViewItm);
    TMatrix4 currentGlobTm = TMatrix4(newViewTM) * newProjTM;

    IntersectPointsVector intersectionPoints;
    calcIntersections(water_level, wavesDeltaH, currentGlobTm, intersectionPoints);

    if (!intersectionPoints.empty())
      newProjTM = newProjTM * calcCropedPerspective(currentGlobTm, intersectionPoints);

    numIntersections = intersectionPoints.size();
  }
  else
  {
    numIntersections = 1;
  }

  TMatrix4 currentGlobTm = TMatrix4(newViewTM) * newProjTM;
  process_tm_for_drv_consts(currentGlobTm);
  setWaterMatrix(currentGlobTm);
}

void WaterProjectedFx::clear(bool forceClear)
{
  if (!forceClear && internalTargetsCleared)
    return;

  SCOPE_RENDER_TARGET;
  for (int i = 0; i < nTargets; ++i)
  {
    if (internalTargets[i])
    {
      d3d::set_render_target(internalTargets[i]->getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, targetClearColors[i], 0.f, 0);
      d3d::resource_barrier({internalTargets[i]->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    if (emptyInternalTextures[i])
    {
      d3d::set_render_target(emptyInternalTextures[i].getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, targetClearColors[i], 0, 0);
      d3d::resource_barrier({emptyInternalTextures[i].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
  }
  internalTargetsCleared = true;
}

bool WaterProjectedFx::render(IWwaterProjFxRenderHelper *render_helper)
{
  G_ASSERT(render_helper);

  // With TBR it's more effective to clear the texture. But it makes sense only if we have just one target
  // (so clearing and subsequent drawing will be in one render pass).
  bool isTileBasedRendering = d3d::get_driver_code().is(d3d::vulkan) || d3d::get_driver_code().is(d3d::metal);
  bool needClearTargets = !internalTargetsCleared || (isTileBasedRendering && nTargets == 1);

  for (int i = 0; i < nTargets; ++i)
  {
    if (internalTargets[i] == nullptr)
    {
      if ((internalTargets[i] = tempRTPools[i]->acquire()) != nullptr)
      {
        internalTargets[i]->resize(frameWidth, frameHeight);
      }
      else
        return false; // device is in broken state
      needClearTargets = true;
    }
  }

  SCOPE_RENDER_TARGET;
  if (needClearTargets)
  {
    TIME_D3D_PROFILE(clear);
    // Since d3d::clearview() clears all targets with the same color,
    // we have to clear each target separately.
    for (int i = 0; i < nTargets; ++i)
    {
      d3d::set_render_target(internalTargets[i]->getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, targetClearColors[i], 0.f, 0);
    }
  }
  for (int i = 0; i < nTargets; ++i)
    d3d::set_render_target(i, internalTargets[i]->getTex2D(), 0);

  bool renderedAnything = renderImpl(render_helper);

  internalTargetsCleared = !renderedAnything;
  if (internalTargetsCleared)
  {
    for (int i = 0; i < nTargets; ++i)
      internalTargets[i] = nullptr;
  }

  for (int i = 0; i < nTargets; ++i)
  {
    if (internalTargets[i])
      d3d::resource_barrier({internalTargets[i]->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  return renderedAnything;
}

bool WaterProjectedFxRenderer::renderImpl(IWwaterProjFxRenderHelper *render_helper)
{
  G_ASSERT(render_helper);

  if (!isValidView())
  {
    setWaterMatrix(TMatrix4::IDENT);
    return false;
  }

  SCOPE_VIEW_PROJ_MATRIX;
  setView(newViewTM, newProjTM, newViewItm.getcol(3));

  static int renderWaterProjectibleDecalsVarId = get_shader_variable_id("render_water_projectible_decals", true);
  ShaderGlobal::set_int(renderWaterProjectibleDecalsVarId, 1);

  bool renderedAnything = render_helper->render_geometry();

  ShaderGlobal::set_int(renderWaterProjectibleDecalsVarId, 0);

  return renderedAnything;
}

void WaterProjectedFxRenderer::setView(const TMatrix &view_tm, const TMatrix4 &proj_tm, const Point3 &world_pos)
{
  d3d::settm(TM_VIEW, view_tm);
  d3d::settm(TM_PROJ, &proj_tm);

  static const int world_view_posVarId = ::get_shader_variable_id("world_view_pos");
  ShaderGlobal::set_float4(world_view_posVarId, Color4::xyz1(world_pos));
}

void WaterProjectedFxRenderer::setWaterMatrix(const TMatrix4 &glob_tm)
{
  // set matrix to water shader
  static int waterEeffectsProjTmLine0VarId = get_shader_variable_id("water_effects_proj_tm_line_0", true);
  static int waterEeffectsProjTmLine1VarId = get_shader_variable_id("water_effects_proj_tm_line_1", true);
  static int waterEeffectsProjTmLine2VarId = get_shader_variable_id("water_effects_proj_tm_line_2", true);
  static int waterEeffectsProjTmLine3VarId = get_shader_variable_id("water_effects_proj_tm_line_3", true);

  ShaderGlobal::set_float4(waterEeffectsProjTmLine0VarId, Color4(glob_tm[0]));
  ShaderGlobal::set_float4(waterEeffectsProjTmLine1VarId, Color4(glob_tm[1]));
  ShaderGlobal::set_float4(waterEeffectsProjTmLine2VarId, Color4(glob_tm[2]));
  ShaderGlobal::set_float4(waterEeffectsProjTmLine3VarId, Color4(glob_tm[3]));
}

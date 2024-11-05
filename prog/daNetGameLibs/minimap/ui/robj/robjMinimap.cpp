// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "robjMinimap.h"
#include "../minimapContext.h"
#include "../minimapState.h"

#include <daRg/dag_uiShader.h>
#include <daRg/dag_element.h>
#include <daRg/dag_helpers.h>
#include <daRg/dag_renderState.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_sceneRender.h>

#include <math/dag_mathUtils.h>

#include <gui/dag_stdGuiRender.h>

#include <workCycle/dag_workCycle.h>
#include <perfMon/dag_cpuFreq.h>
#include "game/player.h"
#include "game/team.h"

#include <ecs/game/zones/levelRegions.h>

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>


void RendObjMinimapRegionsGeometryParams::colorsTabFromTable(const Sqrat::Object &table,
  eastl::hash_map<eastl::string, E3DCOLOR> &colors)
{
  colors.clear();
  Sqrat::Object::iterator it;
  while (table.Next(it))
  {
    HSQOBJECT val = it.getValue();
    colors[it.getName()] = darg::script_decode_e3dcolor(sq_objtointeger(&val));
  }
}

bool RendObjMinimapRegionsGeometryParams::load(const darg::Element *elem)
{
  colorsTabFromTable(elem->props.scriptDesc.GetSlot("regionLineColors"), lineColors);
  colorsTabFromTable(elem->props.scriptDesc.GetSlot("regionFillColors"), fillColors);
  return true;
}

void RobjMinimap::render(
  StdGuiRender::GuiContext &ctx, const darg::Element *elem, const darg::ElemRenderData *rdata, const darg::RenderState &render_state)
{
  MinimapState *mmState = MinimapState::get_from_element(elem);
  if (!mmState || !mmState->ctx)
  {
    LOGERR_ONCE("%s: MinimapState = %p, ctx == %p", __FUNCTION__, mmState, mmState ? mmState->ctx : nullptr);
    return;
  }

  if (!mmState->isSquare)
  {
    const Point2 &pos = elem->screenCoord.screenPos;
    const Point2 &size = elem->screenCoord.size;
    darg::uishader::set_mask(ctx, mmState->ctx->maskTexId, mmState->ctx->maskSampler, pos + size / 2, 0,
      Point2(2 * size.x / StdGuiRender::screen_width(), 2 * size.y / StdGuiRender::screen_height()));
  }

  bool enableBlendMask = mmState->ctx->blendMaskTexId && mmState->ctx->backTexId && getMinimapStage() == MINIMAP_STAGE_MAP;
  if (enableBlendMask)
  {
    TMatrix itm;
    float vis_rad = mmState->getVisibleRadius();
    mmState->calcItmFromView(itm, vis_rad);
    Point3 w = itm.getcol(3);

    Point2 worldLeftTop = mmState->ctx->worldLeftTop;
    Point2 worldRightBottom = mmState->ctx->worldRightBottom;
    Point2 worldSize = worldRightBottom - worldLeftTop;

    Point2 size = elem->screenCoord.size / (vis_rad * 2.0f);
    Point2 pos = Point2(w.x, w.z);

    pos -= worldLeftTop;
    pos = mul(pos, size);

    size = mul(size, worldSize);

    pos -= size * 0.5f;
    pos += elem->screenCoord.size * 0.5;

    darg::uishader::set_mask(ctx, mmState->ctx->blendMaskTexId, mmState->ctx->blendMaskSampler,
      elem->screenCoord.screenPos + Point2(pos.y, pos.x), 0,
      Point2(2 * size.x / StdGuiRender::screen_width(), 2 * size.y / StdGuiRender::screen_height()));
  }
  renderMeshAndFov(mmState, ctx, elem, rdata, render_state);

  if (enableBlendMask)
    darg::uishader::set_mask(ctx, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, Point3(1, 0, 0), Point3(0, 1, 0));
}

void RobjMinimap::postRender(StdGuiRender::GuiContext &ctx, const darg::Element *elem)
{
  MinimapState *mmState = MinimapState::get_from_element(elem);
  if (!mmState || !mmState->ctx)
  {
    LOGERR_ONCE("%s: MinimapState = %p, ctx == %p", __FUNCTION__, mmState, mmState ? mmState->ctx : nullptr);
    return;
  }

  if (!mmState->isSquare && getMinimapStage() == MINIMAP_STAGE_MAP)
  {
    darg::uishader::set_mask(ctx, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, Point3(1, 0, 0), Point3(0, 1, 0));
  }
}


Point2 RobjMinimap::calcTexCoord(MinimapContext *mm_ctx, const Point2 &screen, TMatrix &itm, float vis_rad)
{
  bool isBackMap = getMinimapStage() == MINIMAP_STAGE_BACK_MAP;
  Point2 worldLeftTop = isBackMap ? mm_ctx->backWorldLeftTop : mm_ctx->worldLeftTop;
  Point2 worldRightBottom = isBackMap ? mm_ctx->backWorldRightBottom : mm_ctx->worldRightBottom;

  if (lengthSq(worldLeftTop - worldRightBottom) < 1e-3f)
    return Point2(0, 0);

  Point3 v(screen.x * vis_rad, 0, screen.y * vis_rad);
  Point3 w = itm * v;

  float x = (w.x - worldLeftTop.x) / (worldRightBottom.x - worldLeftTop.x);
  float y = (w.z - worldLeftTop.y) / (worldRightBottom.y - worldLeftTop.y);
  return Point2(x, y);
}


void RobjMinimap::renderMeshAndFov(MinimapState *mm_state,
  StdGuiRender::GuiContext &ctx,
  const darg::Element *elem,
  const darg::ElemRenderData *rdata,
  const darg::RenderState &render_state)
{
  const int nSectors = 32;
  const int nVertices = 1 + nSectors * 2;
  const int nIndices = nSectors * 3;

  const E3DCOLOR defLineColor(180, 180, 180, 255);
  const E3DCOLOR defFillColor(45, 45, 45, 45);

  E3DCOLOR color = (mm_state->ctx->mapTexId != BAD_TEXTUREID) ? elem->props.getColor(elem->csk->color, mm_state->ctx->colorMap)
                                                              : E3DCOLOR(50, 50, 50, 80);
  color = darg::color_apply_opacity(color, render_state.opacity);

  TMatrix itm;
  float vis_rad = mm_state->getVisibleRadius();
  mm_state->calcItmFromView(itm, vis_rad);

  // float panX = elem->props.storage.RawGetSlotValue("mm:panx", 0.0f);
  // float panY = elem->props.storage.RawGetSlotValue("mm:pany", 0.0f);
  // itm.setcol(3, itm.getcol(3)+Point3(panX, 0, panY));

  Point2 r = elem->screenCoord.size * 0.5f;
  Point2 c = elem->screenCoord.screenPos + r;

  if (getMinimapStage() == MINIMAP_STAGE_MAP || getMinimapStage() == MINIMAP_STAGE_BACK_MAP)
  {
    GuiVertex v[nVertices];
    StdGuiRender::IndexType indices[nIndices];

    GuiVertexTransform xf;
    ctx.getViewTm(xf.vtm);

    Point2 tc0 = calcTexCoord(mm_state->ctx, Point2(0, 0), itm, vis_rad);

    v[0].setTc0(tc0);
    v[0].zeroTc1();
    v[0].color = color;
    v[0].setPos(xf, c);

    float angStep = TWOPI / nSectors;

    for (int iSector = 0; iSector < nSectors; ++iSector)
    {
      float angle0 = iSector * angStep;
      float angle1 = (iSector + 1) * angStep;

      float s0, c0, s1, c1;
      sincos(angle0, s0, c0);
      sincos(angle1, s1, c1);

      if (mm_state->isSquare)
      {
        s0 *= 1.5f;
        c0 *= 1.5f;
        s1 *= 1.5f;
        c1 *= 1.5f;
        MinimapContext::clamp_square(c0, s0, 1.0f);
        MinimapContext::clamp_square(c1, s1, 1.0f);
      }

      int iv = 1 + iSector * 2;
      int ii = iSector * 3;

      v[iv].setTc0(calcTexCoord(mm_state->ctx, Point2(c0, -s0), itm, vis_rad));
      v[iv].zeroTc1();
      v[iv + 1].setTc0(calcTexCoord(mm_state->ctx, Point2(c1, -s1), itm, vis_rad));
      v[iv + 1].zeroTc1();

      v[iv].color = v[iv + 1].color = color;

      v[iv].setPos(xf, c + Point2(c0 * r.x, s0 * r.y));
      v[iv + 1].setPos(xf, c + Point2(c1 * r.x, s1 * r.y));

      indices[ii] = 0;
      indices[ii + 1] = iv;
      indices[ii + 2] = iv + 1;
    }

    if (getMinimapStage() == MINIMAP_STAGE_BACK_MAP)
      ctx.set_texture(mm_state->ctx->backTexId, mm_state->ctx->backSampler);
    else
      ctx.set_texture(mm_state->ctx->mapTexId, mm_state->ctx->mapSampler);
    ctx.set_color(255, 255, 255);

    ctx.set_alpha_blend(NONPREMULTIPLIED);
    ctx.start_raw_layer();
    ctx.draw_faces(v, nVertices, indices, nSectors);
    ctx.end_raw_layer();
  }


  if (getMinimapStage() == MINIMAP_STAGE_REGIONS_GEOMETRY)
  {
    RendObjMinimapRegionsGeometryParams *params = static_cast<RendObjMinimapRegionsGeometryParams *>(rdata->params);
    G_ASSERT(params);
    if (!params)
      return;

    ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("level_regions"));
    const LevelRegions *level_regions = g_entity_mgr->getNullable<LevelRegions>(eid, ECS_HASH("level_regions"));
    if (level_regions)
    {
      GuiVertexTransform xf;
      ctx.getViewTm(xf.vtm);
      ctx.resetViewTm();

      // TODO: mudify current GuiVertexTransform
      Point2 p00 = mm_state->worldToMap(Point3(0, 0, 0));
      Point2 p10 = mm_state->worldToMap(Point3(100, 0, 0));
      Point2 p01 = mm_state->worldToMap(Point3(0, 0, 100));
      Point2 axisX = (p01 - p00) * -0.01f * 0.5f;
      Point2 axisY = (p10 - p00) * -0.01f * 0.5f;

      // use transformed bbox for getting absolute screen coordinates
      // to properly include transforms inside element tree
      // NOTE: will not work with rotational transforms!
      BBox2 absBB = elem->transformedBbox;
      Point2 absSize = absBB.rightBottom() - absBB.leftTop();

      axisX.x *= absSize.x;
      axisX.y *= absSize.y;
      axisY.x *= absSize.x;
      axisY.y *= absSize.y;
      p00 = Point2(p00.y * absSize.x, p00.x * absSize.y) * 0.5f;
      p00 += absBB.leftTop() + absSize * 0.5f;
      ///

      const float lineWidth = ctx.hdpx(1) * 1.1f;

      Tab<Point2> points(framemem_ptr());
      points.reserve(64);

      ctx.reset_textures();
      ctx.set_alpha_blend(PREMULTIPLIED);

      for (const splineregions::SplineRegion &reg : *level_regions)
      {
        if (!reg.isVisible)
          continue;
        points.clear();
        for (int i = 0; i < reg.border.size(); i++)
        {
          Point2 p = Point2::xz(reg.border[i]);
          points.push_back(p.x * axisX + p.y * axisY + p00);
        }

        E3DCOLOR lineColor = defLineColor;
        E3DCOLOR fillColor = defFillColor;
        auto it = params->lineColors.find_as(reg.getNameStr());
        if (it != params->lineColors.end())
          lineColor = it->second;
        it = params->fillColors.find_as(reg.getNameStr());
        if (it != params->fillColors.end())
          fillColor = it->second;

        lineColor = darg::color_apply_opacity(lineColor, render_state.opacity);
        fillColor = darg::color_apply_opacity(fillColor, render_state.opacity);

        ctx.render_poly(points, fillColor);
        ctx.render_line_aa(points, true, lineWidth, Point2(0, 0), lineColor);
      }

      ctx.setViewTm(xf.vtm);
    }
  }


  if (mm_state->ctx->isPersp && getMinimapStage() == MINIMAP_STAGE_VIS_CONE)
  {
    TMatrix tm = inverse(itm);
    const TMatrix &itmCam = mm_state->ctx->getCurViewItm();
    Point3 lookDir = (tm % itmCam.getcol(2));
    float lookAng = atan2f(lookDir.x, lookDir.z);

    Point3 tp = tm * itmCam.getcol(3) / vis_rad;
    Point2 c2 = c + Point2(tp.x * r.x, -tp.z * r.y);

    Point2 lt = elem->screenCoord.screenPos;
    Point2 rb = lt + elem->screenCoord.size;
    if (mm_state->isSquare)
      darg::scenerender::setTransformedViewPort(ctx, lt, rb, render_state);
    renderFovOuterSector(mm_state->ctx, ctx, elem, render_state, c2, r, lookAng);
    if (mm_state->isSquare)
      darg::scenerender::restoreTransformedViewPort(ctx);
  }
  StdGuiRender::end_raw_layer();
}


void RobjMinimap::renderFovOuterSector(MinimapContext *mm_ctx,
  StdGuiRender::GuiContext &ctx,
  const darg::Element * /*elem*/,
  const darg::RenderState &render_state,
  const Point2 &center,
  const Point2 &radius,
  float look_angle)
{
  if (!mm_ctx->isPersp)
    return;

  const int nSectors = 32;
  const int nVertices = 1 + nSectors * 2;
  const int nIndices = nSectors * 3;

  GuiVertex v[nVertices];
  StdGuiRender::IndexType indices[nIndices];

  GuiVertexTransform xf;
  ctx.getViewTm(xf.vtm);

  E3DCOLOR fovColor = (mm_ctx->mapTexId != BAD_TEXTUREID) ? mm_ctx->colorFov : E3DCOLOR(60, 60, 60, 60);
  fovColor = darg::color_apply_opacity(fovColor, render_state.opacity);

  float halfFov = (mm_ctx->perspWk > 0) ? atan(1.0f / mm_ctx->perspWk) : 0;
  float fovStep = (TWOPI - 2 * halfFov) / nSectors;

  v[0].zeroTc0();
  v[0].zeroTc1();
  v[0].color = fovColor;
  v[0].setPos(xf, center);

  for (int iSector = 0; iSector < nSectors; ++iSector)
  {
    float angle0 = look_angle + halfFov + iSector * fovStep - HALFPI;
    float angle1 = angle0 + fovStep;

    float s0, c0, s1, c1;
    sincos(angle0, s0, c0);
    sincos(angle1, s1, c1);

    int iv = 1 + iSector * 2;
    int ii = iSector * 3;

    v[iv].zeroTc0();
    v[iv].zeroTc1();
    v[iv + 1].zeroTc0();
    v[iv + 1].zeroTc1();

    v[iv].color = v[iv + 1].color = E3DCOLOR(0, 0, 0, 0);

    v[iv].setPos(xf, center + Point2(c0 * radius.x, s0 * radius.y));
    v[iv + 1].setPos(xf, center + Point2(c1 * radius.x, s1 * radius.y));

    indices[ii] = 0;
    indices[ii + 1] = iv;
    indices[ii + 2] = iv + 1;
  }

  ctx.reset_textures();
  ctx.set_color(255, 255, 255);

  ctx.set_alpha_blend(NONPREMULTIPLIED);
  ctx.start_raw_layer();
  ctx.draw_faces(v, nVertices, indices, nSectors);

  {
    float angle0 = look_angle + halfFov + 0 * fovStep - HALFPI;
    float s0, c0;
    sincos(angle0, s0, c0);

    Point2 p = center + Point2(c0 * radius.x, s0 * radius.y);
    Point2 dir(-s0, c0);

    ctx.render_quad_color(p, p - dir, center - dir, center, Point2(0, 0), Point2(0, 0), Point2(0, 0), Point2(0, 0),
      E3DCOLOR(0, 0, 0, 0), E3DCOLOR(0, 0, 0, 0), E3DCOLOR(0, 0, 0, 0), fovColor);
  }

  {
    float angle0 = look_angle + halfFov + (nSectors - 1) * fovStep - HALFPI;
    float angle1 = angle0 + fovStep;

    float s1, c1;
    sincos(angle1, s1, c1);

    Point2 p = center + Point2(c1 * radius.x, s1 * radius.y);
    Point2 dir(-s1, c1);

    ctx.render_quad_color(p, p + dir, center + dir, center, Point2(0, 0), Point2(0, 0), Point2(0, 0), Point2(0, 0),
      E3DCOLOR(0, 0, 0, 0), E3DCOLOR(0, 0, 0, 0), E3DCOLOR(0, 0, 0, 0), fovColor);
  }

  ctx.end_raw_layer();
}

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../tiledMapContext.h"
#include "robjTiledMap.h"
#include <daRg/dag_element.h>
#include <daRg/dag_helpers.h>
#include <daRg/dag_renderState.h>
#include <daRg/dag_sceneRender.h>
#include <daRg/dag_uiShader.h>
#include <math/dag_color.h>
#include <math/dag_easingFunctions.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <generic/dag_sort.h>
#include <memory/dag_framemem.h>
#include <generic/dag_relocatableFixedVector.h>

using SortedTiles = dag::RelocatableFixedVector<const TilesHashMap::value_type *, 64, true, framemem_allocator>;

static const Point2 tcCoords[4] = {Point2(0, 0), Point2(1, 0), Point2(1, 1), Point2(0, 1)};

static ShaderVariableInfo bgColorVar("fog_of_war_bg_color", true);

static Point2 tile_grid_to_screen(const TiledMapContext &tiled_map_ctx, IPoint2 grid_xy, float resolution, Point2 screen_center)
{
  Point2 world = Point2(grid_xy.x, grid_xy.y) * tiled_map_ctx.tileWidth * resolution + tiled_map_ctx.worldLeftTop;
  return floor(screen_center + tiled_map_ctx.worldToMap(Point3(world.x, 0, world.y)));
}

static void calc_tile_screen_corners(
  TiledMapContext &tiled_map_ctx, const QuadKey &quad_key, Point2 screen_center, Point2 (&corners)[4])
{
  int z = quad_key.size(); // the quadkey property
                           // https://learn.microsoft.com/en-us/azure/azure-maps/zoom-levels-and-tile-grid?tabs=csharp#quadkey-indices
  IPoint2 xy = TiledMapContext::quadKeyToTileXY(quad_key, z);
  xy.y = (1 << z) - 1 - xy.y; // flip y because of left handed world coordinate system

  const float resolution = tiled_map_ctx.getTileResolution(z);
  // note: worldToMap converts left handed world coordinates to right handed screen coordinates, so lt -> screenlb and rb -> screenrt
  Point2 screenlb = tile_grid_to_screen(tiled_map_ctx, xy, resolution, screen_center);
  Point2 screenrt = tile_grid_to_screen(tiled_map_ctx, xy + IPoint2(1, 1), resolution, screen_center);

  // same order as tcCoords
  corners[0] = Point2(screenlb.x, screenrt.y);
  corners[1] = screenrt;
  corners[2] = Point2(screenrt.x, screenlb.y);
  corners[3] = screenlb;
}

void RobjTiledMap::render(
  StdGuiRender::GuiContext &ctx, const darg::Element *elem, const darg::ElemRenderData *rdata, const darg::RenderState &render_state)
{
  G_UNUSED(rdata);

  TIME_PROFILE(RobjTiledMap_render);

  TiledMapContext *tiledMapCtx = TiledMapContext::get_from_element(elem);
  if (!tiledMapCtx)
  {
    LOGERR_ONCE("%s: TiledMapContext = %p", __FUNCTION__, tiledMapCtx);
    return;
  }

  if (tiledMapCtx->tileWidth <= 0)
    return;

  E3DCOLOR white = E3DCOLOR(255, 255, 255, 255);
  E3DCOLOR color = darg::color_apply_opacity(white, inQuintic(render_state.opacity));

  Point2 wh = elem->screenCoord.size;
  Point2 c = elem->screenCoord.screenPos + wh / 2.0f;

  GuiVertexTransform xf;
  ctx.getViewTm(xf.vtm);
  ctx.set_color(255, 255, 255);
  ctx.start_raw_layer();

  SortedTiles sortedTiles;
  sortedTiles.reserve(tiledMapCtx->tiles.size());
  for (auto &tile : tiledMapCtx->tiles)
  {
    sortedTiles.push_back(&tile);
  }
  fast_sort_branchless(sortedTiles.begin(), sortedTiles.end(), [](auto a, auto b) { return a->first.size() < b->first.size(); });

  for (const TilesHashMap::value_type *tile : sortedTiles)
  {
    const QuadKey &quadKey = tile->first;
    const TileHandle tileHandle = tile->second;

    if (tileHandle.texId == BAD_TEXTUREID)
      continue;

    Point2 corners[4];
    calc_tile_screen_corners(*tiledMapCtx, quadKey, c, corners);

    ctx.set_texture(tileHandle.texId, tiledMapCtx->clampSampler);

    GuiVertex v[4];
    for (int i = 0; i < 4; ++i)
    {
      v[i].setPos(xf, corners[i]);
      v[i].setTc0(tcCoords[i]);
      v[i].zeroTc1();
      v[i].color = color;
    }
    ctx.draw_quads(v, 1);
  }
  ctx.end_raw_layer();
}

void RobjTiledMapVisCone::render(
  StdGuiRender::GuiContext &ctx, const darg::Element *elem, const darg::ElemRenderData *rdata, const darg::RenderState &render_state)
{
  G_UNUSED(rdata);

  TiledMapContext *tiledMapCtx = TiledMapContext::get_from_element(elem);
  if (!tiledMapCtx)
  {
    LOGERR_ONCE("%s: TiledMapContext = %p", __FUNCTION__, tiledMapCtx);
    return;
  }

  const TMatrix &itmCam = tiledMapCtx->getCurViewItm();
  TMatrix tm = TMatrix::IDENT;
  tiledMapCtx->calcTmFromView(tm);
  Point3 lookDir = tm % itmCam.getcol(2);
  float lookAng = atan2f(lookDir.z, lookDir.x);
  Point2 center = tiledMapCtx->worldToMap(itmCam.getcol(3));

  Point2 wh = elem->screenCoord.size;
  Point2 c = elem->screenCoord.screenPos + wh / 2.0f;
  float radius = min(wh.x, wh.y) * 0.5f;

  const int nSectors = 32;
  const int nVertices = 1 + nSectors * 2;
  const int nIndices = nSectors * 3;

  GuiVertex v[nVertices];
  StdGuiRender::IndexType indices[nIndices];

  GuiVertexTransform xf;
  ctx.getViewTm(xf.vtm);

  E3DCOLOR fovColor = E3DCOLOR(10, 0, 0, 200);
  fovColor = darg::color_apply_opacity(fovColor, render_state.opacity);

  float halfFov = (tiledMapCtx->perspWk > 0) ? atan(1.0f / tiledMapCtx->perspWk) : 0;
  float fovStep = (TWOPI - 2 * halfFov) / nSectors;

  v[0].zeroTc0();
  v[0].zeroTc1();
  v[0].color = fovColor;
  v[0].setPos(xf, c + center);

  for (int iSector = 0; iSector < nSectors; ++iSector)
  {
    float angle0 = lookAng + halfFov + iSector * fovStep;
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

    v[iv].setPos(xf, c + center + Point2(c0, s0) * radius);
    v[iv + 1].setPos(xf, c + center + Point2(c1, s1) * radius);

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
    float angle0 = lookAng + halfFov + 0 * fovStep;
    float s0, c0;
    sincos(angle0, s0, c0);

    Point2 p = c + center + Point2(c0, s0) * radius;
    Point2 dir(-s0, c0);

    ctx.render_quad_color(p, p - dir, c + center - dir, c + center, Point2(0, 0), Point2(0, 0), Point2(0, 0), Point2(0, 0),
      E3DCOLOR(0, 0, 0, 0), E3DCOLOR(0, 0, 0, 0), E3DCOLOR(0, 0, 0, 0), fovColor);
  }

  {
    float angle0 = lookAng + halfFov + (nSectors - 1) * fovStep;
    float angle1 = angle0 + fovStep;

    float s1, c1;
    sincos(angle1, s1, c1);

    Point2 p = c + center + Point2(c1, s1) * radius;
    Point2 dir(-s1, c1);

    ctx.render_quad_color(p, p + dir, c + center + dir, c + center, Point2(0, 0), Point2(0, 0), Point2(0, 0), Point2(0, 0),
      E3DCOLOR(0, 0, 0, 0), E3DCOLOR(0, 0, 0, 0), E3DCOLOR(0, 0, 0, 0), fovColor);
  }

  ctx.end_raw_layer();
}

void RobjTiledMapFogOfWar::render(
  StdGuiRender::GuiContext &ctx, const darg::Element *elem, const darg::ElemRenderData *rdata, const darg::RenderState &render_state)
{
  G_UNUSED(rdata);

  TiledMapContext *tiledMapCtx = TiledMapContext::get_from_element(elem);
  if (!tiledMapCtx)
  {
    LOGERR_ONCE("%s: TiledMapContext = %p", __FUNCTION__, tiledMapCtx);
    return;
  }

  if (tiledMapCtx->fogOfWarEnabled == false)
    return;

  if (tiledMapCtx->tileWidth <= 0)
    return;

  Color4 bg = ShaderGlobal::get_float4(bgColorVar);
  E3DCOLOR bgColor = e3dcolor(bg);
  E3DCOLOR color = E3DCOLOR(255, 255, 255, 255);
  color = darg::color_apply_opacity(color, outQuintic(render_state.opacity));
  bgColor = darg::color_apply_opacity(bgColor, outQuintic(render_state.opacity));

  Point2 wh = elem->screenCoord.size;
  if (wh.x <= 1e-3f || wh.y <= 1e-3f)
    return;
  Point2 c = elem->screenCoord.screenPos + wh / 2.0f;

  ctx.start_raw_layer();
  ctx.reset_textures();
  ctx.set_alpha_blend(NONPREMULTIPLIED);

  GuiVertexTransform xf;
  ctx.getViewTm(xf.vtm);
  ctx.set_color(bgColor);

  const bool hasViewport = tiledMapCtx->getViewportWidth() > 0 && tiledMapCtx->getViewportHeight() > 0;
  const Point2 fogMaskSize = hasViewport ? Point2(tiledMapCtx->getViewportWidth(), tiledMapCtx->getViewportHeight()) : wh;

  // setShader() resets the GuiContext layer params (including the mask), so the
  // mask must be re-applied after every shader switch, not just once up front.
  auto applyFogMask = [&] {
    if (tiledMapCtx->fogOfWarTex.getTexId() != BAD_TEXTUREID && tiledMapCtx->fogOfWarTexInited)
      darg::uishader::set_mask(ctx, tiledMapCtx->fogOfWarTex.getTexId(), tiledMapCtx->clampSampler, c, 0,
        Point2(2.0f * fogMaskSize.x / StdGuiRender::screen_width(), -2.0f * fogMaskSize.y / StdGuiRender::screen_height()));
  };
  applyFogMask();

  // ======================== render basic fog for current zoom level ========================
  // this should cover the gaps between tiles if no fog tile is loaded yet
  GuiVertex v[4];
  for (int i = 0; i < 4; ++i)
  {
    Point2 pos = Point2(tcCoords[i].x * wh.x, tcCoords[i].y * wh.y);
    v[i].setPos(xf, elem->screenCoord.screenPos + pos);
    v[i].setTc0(tcCoords[i]);
    v[i].zeroTc1();
    v[i].color = bgColor;
  }
  ctx.draw_quads(v, 1);

  // ======================== render blended fog for current zoom level ========================
  ctx.setShader(&tiledMapCtx->fogOfWarTileShader);
  applyFogMask(); // setShader reset the layer params above; reinstall the mask for the tiles

  SortedTiles sortedTiles;
  sortedTiles.reserve(tiledMapCtx->tiles.size());
  for (auto &tile : tiledMapCtx->tiles)
  {
    sortedTiles.push_back(&tile);
  }
  fast_sort_branchless(sortedTiles.begin(), sortedTiles.end(), [](auto a, auto b) { return a->first.size() < b->first.size(); });
  for (const TilesHashMap::value_type *tile : sortedTiles)
  {
    const QuadKey &quadKey = tile->first;
    const TileHandle tileHandle = tile->second;

    if (tileHandle.texId == BAD_TEXTUREID)
      continue;

    Point2 corners[4];
    calc_tile_screen_corners(*tiledMapCtx, quadKey, c, corners);

    ctx.set_texture(tileHandle.texId, tiledMapCtx->clampSampler);

    GuiVertex v[4];
    for (int i = 0; i < 4; ++i)
    {
      v[i].setPos(xf, corners[i]);
      v[i].setTc0(tcCoords[i]);
      v[i].zeroTc1();
      v[i].color = color;
    }
    ctx.draw_quads(v, 1);
  }
  ctx.setShader(nullptr);
  darg::uishader::set_mask(ctx, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, Point3(1, 0, 0), Point3(0, 1, 0));
  ctx.reset_textures();
  ctx.end_raw_layer();
}

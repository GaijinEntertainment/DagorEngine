// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../tiledMapContext.h"
#include "robjTiledMap.h"
#include <daRg/dag_element.h>
#include <daRg/dag_helpers.h>
#include <daRg/dag_renderState.h>
#include <daRg/dag_sceneRender.h>
#include <daRg/dag_uiShader.h>
#include <math/dag_easingFunctions.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_sort.h>

static const Point2 tcCoords[4] = {Point2(0, 0), Point2(1, 0), Point2(1, 1), Point2(0, 1)};

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

  eastl::vector<QuadKey> sortedTiles;
  sortedTiles.reserve(tiledMapCtx->tiles.size());
  for (auto &tile : tiledMapCtx->tiles)
  {
    sortedTiles.push_back(tile.first);
  }
  eastl::sort(sortedTiles.begin(), sortedTiles.end(), [](const QuadKey &a, const QuadKey &b) { return a.size() < b.size(); });

  for (auto quadKey : sortedTiles)
  {
    const TileHandle tileHandle = tiledMapCtx->tiles[quadKey];

    if (tileHandle.texId == BAD_TEXTUREID)
      continue;

    int z = quadKey.size(); // the quadkey property
                            // https://learn.microsoft.com/en-us/azure/azure-maps/zoom-levels-and-tile-grid?tabs=csharp#quadkey-indices
    IPoint2 xy = TiledMapContext::quadKeyToTileXY(quadKey, z);
    xy.y = (1 << z) - 1 - xy.y; // flip y because of left handed world coordinate system

    const float resolution = tiledMapCtx->getTileResolution(z);
    Point2 lt = Point2(xy.x, xy.y) * tiledMapCtx->tileWidth * resolution + tiledMapCtx->worldLeftTop;
    Point2 rb = lt + Point2(tiledMapCtx->tileWidth, tiledMapCtx->tileWidth) * resolution;

    // note: worldToMap converts left handed world coordinates to right handed screen coordinates, so lt -> screenlb and rb -> screenrt
    Point2 screenlb = tiledMapCtx->worldToMap(Point3(lt.x, 0, lt.y));
    Point2 screenrt = tiledMapCtx->worldToMap(Point3(rb.x, 0, rb.y));
    Point2 screenlt = Point2(screenlb.x, screenrt.y);
    Point2 tileWH = Point2(screenrt.x - screenlb.x, screenlb.y - screenrt.y);

    ctx.set_texture(tileHandle.texId, tileHandle.smpId);

    GuiVertex v[4];
    for (int i = 0; i < 4; ++i)
    {
      Point2 pos = screenlt + Point2(tcCoords[i].x * tileWH.x, tcCoords[i].y * tileWH.y);
      v[i].setPos(xf, floor(c + pos));
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

  E3DCOLOR color = E3DCOLOR(255, 255, 255, 255);
  E3DCOLOR bgColor = E3DCOLOR(22, 18, 17, 255); // todo: bring this color from entity config
  color = darg::color_apply_opacity(color, outQuintic(render_state.opacity));
  bgColor = darg::color_apply_opacity(bgColor, outQuintic(render_state.opacity));

  Point2 wh = elem->screenCoord.size;
  Point2 c = elem->screenCoord.screenPos + wh / 2.0f;

  ctx.start_raw_layer();
  ctx.reset_textures();
  ctx.set_alpha_blend(NONPREMULTIPLIED);

  GuiVertexTransform xf;
  ctx.getViewTm(xf.vtm);
  ctx.set_color(bgColor);

  if (tiledMapCtx->fogOfWarTex.getTexId() != BAD_TEXTUREID)
    darg::uishader::set_mask(ctx, tiledMapCtx->fogOfWarTex.getTexId(), tiledMapCtx->fogOfWarSampler, c, 0,
      Point2(2.0f * wh.x / StdGuiRender::screen_width(), -2.0f * wh.y / StdGuiRender::screen_height()));

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
  eastl::vector<QuadKey> sortedTiles;
  sortedTiles.reserve(tiledMapCtx->fogTiles.size());
  for (auto &tile : tiledMapCtx->fogTiles)
  {
    sortedTiles.push_back(tile.first);
  }
  eastl::sort(sortedTiles.begin(), sortedTiles.end(), [](const QuadKey &a, const QuadKey &b) { return a.size() < b.size(); });
  for (auto quadKey : sortedTiles)
  {
    const TileHandle tileHandle = tiledMapCtx->fogTiles[quadKey];

    if (tileHandle.texId == BAD_TEXTUREID)
      continue;

    int z = quadKey.size(); // the quadkey property
                            // https://learn.microsoft.com/en-us/azure/azure-maps/zoom-levels-and-tile-grid?tabs=csharp#quadkey-indices
    IPoint2 xy = TiledMapContext::quadKeyToTileXY(quadKey, z);
    xy.y = (1 << z) - 1 - xy.y; // flip y because of left handed world coordinate system

    const float resolution = tiledMapCtx->getTileResolution(z);
    Point2 lt = Point2(xy.x, xy.y) * tiledMapCtx->tileWidth * resolution + tiledMapCtx->worldLeftTop;
    Point2 rb = lt + Point2(tiledMapCtx->tileWidth, tiledMapCtx->tileWidth) * resolution;

    // note: worldToMap converts left handed world coordinates to right handed screen coordinates, so lt -> screenlb and rb -> screenrt
    Point2 screenlb = tiledMapCtx->worldToMap(Point3(lt.x, 0, lt.y));
    Point2 screenrt = tiledMapCtx->worldToMap(Point3(rb.x, 0, rb.y));
    Point2 screenlt = Point2(screenlb.x, screenrt.y);
    Point2 tileWH = Point2(screenrt.x - screenlb.x, screenlb.y - screenrt.y);

    ctx.set_texture(tileHandle.texId, tileHandle.smpId);

    GuiVertex v[4];
    for (int i = 0; i < 4; ++i)
    {
      Point2 pos = screenlt + Point2(tcCoords[i].x * tileWH.x, tcCoords[i].y * tileWH.y);
      v[i].setPos(xf, floor(c + pos));
      v[i].setTc0(tcCoords[i]);
      v[i].zeroTc1();
      v[i].color = color;
    }
    ctx.draw_quads(v, 1);
  }

  darg::uishader::set_mask(ctx, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, Point3(1, 0, 0), Point3(0, 1, 0));
  ctx.reset_textures();
  ctx.end_raw_layer();
}

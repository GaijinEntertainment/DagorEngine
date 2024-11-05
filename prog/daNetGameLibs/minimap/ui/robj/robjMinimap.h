// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_renderObject.h>
#include <math/dag_Point2.h>
#include <EASTL/hash_map.h>


class MinimapContext;
class TMatrix;
class MinimapState;

enum
{
  MINIMAP_STAGE_MAP,
  MINIMAP_STAGE_BACK_MAP,
  MINIMAP_STAGE_REGIONS_GEOMETRY,
  MINIMAP_STAGE_VIS_CONE,
};

class RendObjMinimapRegionsGeometryParams : public darg::RendObjParams
{
public:
  eastl::hash_map<eastl::string, E3DCOLOR> lineColors;
  eastl::hash_map<eastl::string, E3DCOLOR> fillColors;

  virtual bool load(const darg::Element *elem) override;
  void colorsTabFromTable(const Sqrat::Object &table, eastl::hash_map<eastl::string, E3DCOLOR> &colors);
};

class RobjMinimap : public darg::RenderObject
{
  virtual int getMinimapStage() { return MINIMAP_STAGE_MAP; }

  virtual void render(StdGuiRender::GuiContext &ctx,
    const darg::Element *elem,
    const darg::ElemRenderData * /*rdata*/,
    const darg::RenderState &render_state) override;
  virtual void postRender(StdGuiRender::GuiContext &ctx, const darg::Element *) override;

  void renderMeshAndFov(MinimapState *mm_state,
    StdGuiRender::GuiContext &ctx,
    const darg::Element *elem,
    const darg::ElemRenderData *rdata,
    const darg::RenderState &render_state);

  void renderFovOuterSector(MinimapContext *mm_ctx,
    StdGuiRender::GuiContext &ctx,
    const darg::Element *elem,
    const darg::RenderState &render_state,
    const Point2 &center,
    const Point2 &radius,
    float look_angle);
  Point2 calcTexCoord(MinimapContext *mm_ctx, const Point2 &screen, TMatrix &itm, float vis_rad);
};


class RobjMinimapBackMap : public RobjMinimap
{
  virtual int getMinimapStage() { return MINIMAP_STAGE_BACK_MAP; }
};

class RobjMinimapRegionsGeometry : public RobjMinimap
{
  virtual int getMinimapStage() { return MINIMAP_STAGE_REGIONS_GEOMETRY; }
};


class RobjMinimapVisCone : public RobjMinimap
{
  virtual int getMinimapStage() { return MINIMAP_STAGE_VIS_CONE; }
};

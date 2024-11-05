// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "minimapState.h"
#include "minimapContext.h"
#include <ui/scriptStrings.h>

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>


static const float min_visible_radius = 10.0f;
SQ_PRECACHED_STRINGS_IMPLEMENT(MinimapState, //
  ctx,
  center,
  shape,
  square,
  isViewUp,
  visibleRadius,
  minimapState);


MinimapState::MinimapState(const Sqrat::Object &data)
{
  auto strings = ui_strings.resolveVm(data.GetVM());
  G_ASSERT_RETURN(strings, );

  ctx = data.RawGetSlotValue<MinimapContext *>(strings->ctx, nullptr);
  G_ASSERT(ctx);

  Sqrat::Object center = data.RawGetSlot(strings->center);
  if (!center.IsNull())
  {
    isHeroCentered = false;
    panWolrdPos = data.RawGetSlotValue(center, Point3(0, 0, 0));
  }

  Sqrat::Object shape = data.RawGetSlot(strings->shape);
  G_ASSERT(!shape.IsNull());
  isSquare = sq_direct_is_equal(shape.GetVM(), &shape.GetObject(), &const_cast<Sqrat::Object &>(strings->square).GetObject());
  isViewUp = data.RawGetSlotValue(strings->isViewUp, !isSquare);
  float radius = data.RawGetSlotValue(strings->visibleRadius, MinimapContext::def_visible_radius);
  baseVisibleRadius = currentVisibleRadius = ::max(radius, min_visible_radius);
}


MinimapState *MinimapState::get_from_element(const darg::Element *elem)
{
  auto strings = ui_strings.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, nullptr);

  return elem->props.scriptDesc.RawGetSlotValue<MinimapState *>(strings->minimapState, nullptr);
}


void MinimapState::calcItmFromView(TMatrix &itm_out, float vis_rad) const
{
  const TMatrix &itm = ctx->getCurViewItm();

  Point3 pos = (isInteractive || !isHeroCentered) ? panWolrdPos : itm.getcol(3);

  if (isViewUp)
  {
    Point3 up(0, 1, 0);
    Point3 right(up % itm.getcol(2));
    Point3 fwd;

    if (lengthSq(right) > 0.05f)
    {
      right = normalize(right);
      fwd = right % up;
    }
    else
    {
      right = itm.getcol(0);
      fwd = normalize(right % up);
      right = up % fwd;
    }

    itm_out.setcol(0, right);
    itm_out.setcol(1, up);
    itm_out.setcol(2, fwd);
    itm_out.setcol(3, pos);
  }
  else
  {
    Point3 up(0, 1, 0);
    Point3 fwd(1, 0, 0);
    Point3 right(up % fwd);

    BBox2 bbox;
    bbox += ctx->worldLeftTop;
    bbox += ctx->worldRightBottom;
    bbox += ctx->backWorldLeftTop;
    bbox += ctx->backWorldRightBottom;

    bbox.inflate(-vis_rad);
    if (!bbox.isempty())
    {
      pos.x = clamp(pos.x, bbox.left(), bbox.right());
      pos.z = clamp(pos.z, bbox.top(), bbox.bottom());
    }

    itm_out.setcol(0, right);
    itm_out.setcol(1, up);
    itm_out.setcol(2, fwd);
    itm_out.setcol(3, pos);
  }
}


void MinimapState::calcTmFromView(TMatrix &tm_out, float vis_rad) const
{
  TMatrix itm;
  calcItmFromView(itm, vis_rad);
  tm_out = inverse(itm);
}


float MinimapState::getVisibleRadius() const { return ctx->limitVisibleRadius(currentVisibleRadius); }


float MinimapState::getTargetVisibleRadius() const { return targetVisibleRadius; }

float MinimapState::getMaxVisibleRadius() const { return ctx->maxVisibleRadius(); }

void MinimapState::setTargetVisibleRadius(float r) { targetVisibleRadius = r; }


float MinimapState::setVisibleRadius(float r)
{
  currentVisibleRadius = ctx->limitVisibleRadius(::max(r, min_visible_radius));
  panWolrdPos = ctx->clampPan(panWolrdPos, currentVisibleRadius);
  return currentVisibleRadius;
}


void MinimapState::setInteractive(bool on)
{
  if (on != isInteractive)
  {
    isInteractive = on;
    if (isInteractive && isHeroCentered)
      panWolrdPos = ctx->getCurViewItm().getcol(3);
  }
}


Point3 MinimapState::mapToWorld(float x, float y)
{
  TMatrix itm;
  float visibleRadius = getVisibleRadius();
  calcItmFromView(itm, visibleRadius);

  Point3 worldPos = itm * Point3(x * visibleRadius, 0, y * visibleRadius);
  return worldPos;
}

Point2 MinimapState::worldToMap(const Point3 &world_pos)
{
  TMatrix tm;
  float visibleRadius = getVisibleRadius();
  calcTmFromView(tm, visibleRadius);

  Point3 mapPos = tm * world_pos;
  return Point2(mapPos.x, mapPos.z) / visibleRadius;
}


SQ_DEF_AUTO_BINDING_MODULE_EX(bind_minimap_classes, "minimap", sq::VM_UI_ALL)
{
  Sqrat::Table exports(vm);

  Sqrat::Class<MinimapContext, Sqrat::NoCopy<MinimapContext>> sqMinimapContext(vm, "MinimapContext");
  sqMinimapContext //
    .Func("setup", &MinimapContext::setup)
    .Func("limitVisibleRadius", &MinimapContext::limitVisibleRadius)
    /**/;

  Sqrat::Class<MinimapState> sqMinimapState(vm, "MinimapState");
  sqMinimapState //
    .Ctor<const Sqrat::Object &>()
    .Func("getVisibleRadius", &MinimapState::getVisibleRadius)
    .Func("setVisibleRadius", &MinimapState::setVisibleRadius)
    .Func("setTargetVisibleRadius", &MinimapState::setTargetVisibleRadius)
    .Func("mapToWorld", &MinimapState::mapToWorld)
    .Func("worldToMap", &MinimapState::worldToMap)
    .Var("isHeroCentered", &MinimapState::isHeroCentered)
    .Var("panWolrdPos", &MinimapState::panWolrdPos)
    /**/;

  exports.Bind("MinimapContext", sqMinimapContext);
  exports.Bind("MinimapState", sqMinimapState);
  return exports;
}

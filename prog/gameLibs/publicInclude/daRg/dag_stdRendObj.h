//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daRg/dag_renderObject.h>
#include <math/dag_math3d.h>


namespace darg
{

class Picture;
class GuiScene;


class RobjParamsBox : public RendObjParams
{
public:
  E3DCOLOR fillColor;
  E3DCOLOR borderColor;
  Point4 borderWidth;
  Point4 borderRadius;
  Picture *image = nullptr;
  Picture *fallbackImage = nullptr;
  bool flipX = false;
  bool flipY = false;
  float saturateFactor = 1.0f;
  float brightness = 1.0f;

  virtual bool load(const Element *elem) override;
  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override;
  virtual bool getAnimFloat(AnimProp prop, float **ptr) override;
};


class RenderObjectBox : public RenderObject
{
protected:
  virtual void renderCustom(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *,
    const RenderState &render_state);
  void renderNoRadius(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
    const RenderState &render_state);
};


class RobjParamsVectorCanvas : public RendObjParams
{
public:
  float lineWidth = 1.0f;
  E3DCOLOR color;
  E3DCOLOR fillColor;
  Sqrat::Object commands;
  Sqrat::Function draw;
  Sqrat::Object canvasScriptRef;
  Sqrat::Table rectCache;
  float brightness = 1.0f;

  virtual bool load(const Element *elem) override;

  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override;

private:
  void setupDrawApi(GuiScene *scene);
};


class RenderObjectVectorCanvas : public RenderObject
{
protected:
  virtual void renderCustom(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *,
    const RenderState &render_state);
};


} // namespace darg

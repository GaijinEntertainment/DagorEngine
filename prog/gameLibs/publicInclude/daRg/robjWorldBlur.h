//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daRg/dag_renderObject.h>
#include <daRg/dag_stdRendObj.h>
#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>
#include <gui/dag_stdGuiRender.h>


namespace darg
{

class RobjParamsBlur : public RobjParamsBox
{
public:
  E3DCOLOR color = E3DCOLOR(255, 255, 255, 255);

  virtual bool load(const Element *elem) override
  {
    RobjParamsBox::load(elem);

    color = elem->props.getColor(elem->csk->color, E3DCOLOR(255, 255, 255, 255));
    fillColor = elem->props.getColor(elem->csk->fillColor, E3DCOLOR(0, 0, 0, 0));

    if (!elem->props.scriptDesc.HasKey(elem->csk->borderWidth))
    {
      borderWidth.zero();
      borderColor.u = 0;
    }

    return true;
  }

  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override
  {
    if (prop == AP_COLOR)
    {
      *ptr = &color;
      return true;
    }
    if (prop == AP_FILL_COLOR)
    {
      *ptr = &fillColor;
      return true;
    }
    return false;
  }
};


class RobjBlurBase : public RenderObjectBox
{
protected:
  void renderTexRect(StdGuiRender::GuiContext &ctx, const ElemRenderData *rdata, const RenderState &render_state, BlendMode blend_mode,
    float max_mip);
};

class RobjWorldBlur : public RobjBlurBase
{
public:
  virtual void renderCustom(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
    const RenderState &render_state) override;
};


class RobjWorldBlurPanel : public RobjBlurBase
{
public:
  void execBlur(const Point2 &lt, const Point2 &size) const;
  virtual void renderCustom(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
    const RenderState &render_state) override;
};

// WT-specific
class RobjWorldBlurPanelStub : public RobjBlurBase
{
public:
  virtual void renderCustom(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
    const RenderState &render_state) override;
};

} // namespace darg

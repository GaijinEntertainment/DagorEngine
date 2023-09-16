#pragma once

#include <daRg/dag_renderObject.h>
#include <3d/dag_texMgr.h>


namespace darg
{

class Picture;

class RobjMaskParams : public RendObjParams
{
public:
  virtual bool load(const Element *elem) override;

  Picture *image = nullptr;
};

class RobjMask : public RenderObject
{
  virtual void renderCustom(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData * /*rdata*/,
    const RenderState &render_state) override;
  virtual void postRender(StdGuiRender::GuiContext &ctx, const Element *) override;
};


} // namespace darg

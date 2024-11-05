// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_renderObject.h>
#include <3d/dag_texMgr.h>
#include <dag/dag_vector.h>

namespace das
{
struct SimFunction;
struct TypeInfo;
class Context;
} // namespace das

namespace darg
{

class Picture;

class RobjDasCanvasParams : public RendObjParams
{
public:
  virtual bool load(const Element *elem) override;

  das::SimFunction *drawFunc = nullptr;
  das::Context *dasCtx = nullptr;
  Sqrat::Object dasScriptObj;
  das::TypeInfo *dataArgType = nullptr;
  dag::Vector<char> data;
};

class RobjDasCanvas : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData * /*rdata*/,
    const RenderState &render_state) override;
};


} // namespace darg

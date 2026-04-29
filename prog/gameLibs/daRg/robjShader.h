// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_renderObject.h>
#include <shaders/dag_shaders.h>
#include <math/dag_color.h>
#include <drv/3d/dag_consts.h>
#include <util/dag_simpleString.h>


namespace darg
{

class RobjShaderParams : public RendObjParams
{
public:
  SimpleString shaderName;
  SimpleString shaderSource;

  Ptr<ShaderMaterial> material;
  Ptr<ShaderElement> shaderElem;
  PROGRAM rtProgram = BAD_PROGRAM;

  Color4 params[4];
  float brightness = 1.0f;
  float cachedOpacity = 1.0f; // set during render(), read during deferred callback
  Point2 cursorPos = {};

  bool load(const Element *elem) override;
  bool getAnimFloat(AnimProp prop, float **ptr) override;
  ~RobjShaderParams();
};


class RenderObjectShader : public RenderObject
{
public:
  void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
    const RenderState &render_state) override;
};


void register_shader_rendobj_factory();

} // namespace darg

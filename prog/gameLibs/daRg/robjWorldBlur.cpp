// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/robjWorldBlur.h>

#include <daRg/dag_element.h>
#include <daRg/dag_helpers.h>
#include <daRg/dag_renderState.h>

#include <gui/dag_stdGuiRender.h>

#include <math/integer/dag_IPoint2.h>
#include <memory/dag_framemem.h>

#include <render/fx/uiPostFxManager.h>
#include <render/hdrRender.h>

#include "guiGlobals.h"

namespace darg
{

static float gui_reverse_scale(float x) { return (x - 0.5f) * GUI_POS_SCALE_INV; }

void RobjWorldBlur::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjParamsBlur *params = static_cast<RobjParamsBlur *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  renderTexRect(ctx, rdata, render_state, PREMULTIPLIED, 0);
  if (params->fillColor.u != 0 || params->borderColor.u != 0)
    RenderObjectBox::render(ctx, elem, rdata, render_state);
}

// WT-specific
void RobjWorldBlurPanelStub::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjParamsBlur *params = static_cast<RobjParamsBlur *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  E3DCOLOR tmp = params->color;
  params->color = E3DCOLOR(0xFF7A7A7A);

  renderTexRect(ctx, rdata, render_state, NONPREMULTIPLIED, 1.f);
  if (params->fillColor.u != 0 || params->borderColor.u != 0)
    RenderObjectBox::render(ctx, elem, rdata, render_state);

  params->color = tmp;
}


void RobjBlurBase::renderTexRect(StdGuiRender::GuiContext &ctx, const ElemRenderData *rdata, const RenderState &render_state,
  BlendMode blend_mode, float max_mip)
{
  RobjParamsBlur *params = static_cast<RobjParamsBlur *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  const auto uiBlurTexId = UiPostFxManager::getUiBlurTexId();
  const auto uiBlurTexSdrId = UiPostFxManager::getUiBlurSdrTexId();
  const d3d::SamplerHandle uiBlurTexSampler = UiPostFxManager::getUiBlurSampler();
  const d3d::SamplerHandle uiBlurTexSdrSampler = UiPostFxManager::getUiBlurSdrSampler();

  float sw = StdGuiRender::screen_width();
  float sh = StdGuiRender::screen_height();
  G_ASSERT_RETURN(sw >= 1 && sh >= 1, );

  E3DCOLOR color = color_apply_opacity(params->color, (uiBlurTexId == BAD_TEXTUREID) ? 0.25f : render_state.opacity);
  ctx.set_alpha_blend(blend_mode);
  ctx.set_color(color);
  ctx.set_textures(uiBlurTexId, uiBlurTexSampler, uiBlurTexSdrId, uiBlurTexSdrSampler, false, hdrrender::is_hdr_enabled());
  if (params->saturateFactor != 1.0f)
    ctx.set_picquad_color_matrix_saturate(params->saturateFactor);

  Point2 lt = rdata->pos;
  Point2 rb = lt + rdata->size;

  float vtm[2][3];
  ctx.getViewTm(vtm);

  Point2 tc[3] = {
    lt,
    Point2(rb.x, lt.y),
    Point2(lt.x, rb.y),
  };

  for (int i = 0; i < 3; ++i)
  {
    Point2 &p = tc[i];
    float x = gui_reverse_scale(vtm[0][0] * p.x + vtm[0][1] * p.y + vtm[0][2]);
    float y = gui_reverse_scale(vtm[1][0] * p.x + vtm[1][1] * p.y + vtm[1][2]);
    p.x = x / sw;
    p.y = y / sh;
  }
  const int gui_mip_no = 0; // or gui_mip_no = ShaderGlobal::get_int_fast(get_shader_variable_id("gui_tex_mip", true)); to be data
                            // driven
  ctx.set_user_state(gui_mip_no, clamp((tc[2].y - tc[0].y - 0.05f) * 2.f, 0.f, max_mip));
  // ctx.render_rect(lt, rb, tc[0], tc[1]-tc[0], tc[2]-tc[0]);
  ctx.render_rounded_image(lt, rb, tc[0], tc[1] - tc[0], tc[2] - tc[0], color, params->borderRadius);
  ctx.set_user_state(gui_mip_no, 0.f);
  if (params->saturateFactor != 1.0f)
    ctx.reset_picquad_color_matrix();
}


void RobjWorldBlurPanel::execBlur(const Point2 &lt, const Point2 &size) const
{
  UiPostFxManager::updateFinalBlurred(ipoint2(lt), ipoint2(lt + size), 1);
}


static constexpr int GUI_BLUR_PANEL = 13;


static void render_callback_run(StdGuiRender::CallBackState &cbs)
{
  G_ASSERT_RETURN(cbs.command == GUI_BLUR_PANEL, );
  RobjWorldBlurPanel *self = (RobjWorldBlurPanel *)(cbs.data);
  self->execBlur(cbs.pos, cbs.size);
}


void RobjWorldBlurPanel::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjParamsBlur *params = static_cast<RobjParamsBlur *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  // check that we can update ui blur, if not render it without updates
  if (UiPostFxManager::isBloomEnabled())
  {
    auto blurBox = elem->calcTransformedBbox();
    ctx.execCommand(GUI_BLUR_PANEL, blurBox.leftTop(), blurBox.size(), render_callback_run, uintptr_t(this));
  }

  renderTexRect(ctx, rdata, render_state, NONPREMULTIPLIED, 4.0);

  if (params->fillColor.u != 0 || params->borderColor.u != 0)
    RenderObjectBox::render(ctx, elem, rdata, render_state);
}

ROBJ_FACTORY_IMPL(RobjWorldBlur, RobjParamsBlur)
ROBJ_FACTORY_IMPL(RobjWorldBlurPanel, RobjParamsBlur)
ROBJ_FACTORY_IMPL(RobjWorldBlurPanelStub, RobjParamsBlur) // WT-specific

void do_ui_blur_impl(const Tab<BBox2> &boxes)
{
  if (UiPostFxManager::isBloomEnabled())
  {
    dag::Vector<IBBox2, framemem_allocator> iboxes;
    iboxes.reserve(boxes.size());
    for (const BBox2 &bbox : boxes)
      if (bbox.left() != bbox.right() && bbox.top() != bbox.bottom())
        iboxes.push_back(IBBox2(ipoint2(bbox.leftTop()), ipoint2(bbox.rightBottom())));
    UiPostFxManager::updateFinalBlurred(iboxes.begin(), iboxes.end(), 0);
  }
}

void register_blur_rendobj_factories(bool wt_compatibility_mode)
{
  G_ASSERT(rendobj_world_blur_id < 0);
  rendobj_world_blur_id = add_rendobj_factory("ROBJ_WORLD_BLUR", ROBJ_FACTORY_PTR(RobjWorldBlur));
  if (wt_compatibility_mode)
    add_rendobj_factory("ROBJ_WORLD_BLUR_PANEL", ROBJ_FACTORY_PTR(RobjWorldBlurPanelStub));
  else
    add_rendobj_factory("ROBJ_WORLD_BLUR_PANEL", ROBJ_FACTORY_PTR(RobjWorldBlurPanel));

  do_ui_blur = do_ui_blur_impl;
}


} // namespace darg

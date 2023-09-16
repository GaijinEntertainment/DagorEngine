#include <daRg/dag_renderObject.h>
#include <daRg/dag_renderState.h>
#include <daRg/dag_element.h>
#include <daRg/dag_helpers.h>
#include <daRg/dag_stringKeys.h>

#include <gui/dag_stdGuiRender.h>
#include "guiScene.h"
#include "guiGlobals.h"


namespace darg
{

class RobjBlurParams : public RendObjParams
{
public:
  E3DCOLOR color = E3DCOLOR(120, 120, 120, 160);

  virtual bool load(const Element *elem) override
  {
    color = elem->props.getColor(elem->csk->color);
    return true;
  }
  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override
  {
    if (prop == AP_COLOR)
    {
      *ptr = &color;
      return true;
    }
    return false;
  }
};


class RobjWorldBlurStub : public RenderObject
{
  virtual void renderCustom(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
    const RenderState &render_state) override
  {
    RobjBlurParams *params = static_cast<RobjBlurParams *>(rdata->params);
    G_ASSERT(params);
    if (!params)
      return;

    GuiScene *scene = GuiScene::get_from_elem(elem);
    E3DCOLOR color = e3dcolor_mul(params->color, scene->getConfig()->defSceneBgColor);
    color = color_apply_opacity(color, render_state.opacity);

    ctx.set_color(color);
    ctx.set_texture(BAD_TEXTUREID);
    ctx.render_rect(rdata->pos, rdata->pos + rdata->size);
  }
};


void do_ui_blur_stub(const Tab<BBox2> &) {}


ROBJ_FACTORY_IMPL(RobjWorldBlurStub, RobjBlurParams)

void register_blur_stubs()
{
  rendobj_world_blur_id = add_rendobj_factory("ROBJ_WORLD_BLUR", ROBJ_FACTORY_PTR(RobjWorldBlurStub));
  add_rendobj_factory("ROBJ_WORLD_BLUR_PANEL", ROBJ_FACTORY_PTR(RobjWorldBlurStub));
  do_ui_blur = do_ui_blur_stub;
}

} // namespace darg

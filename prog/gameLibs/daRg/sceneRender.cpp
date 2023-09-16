#include <daRg/dag_sceneRender.h>
#include <daRg/dag_renderState.h>

namespace darg
{

namespace scenerender
{

static int viewportDepth = 0;

static float gui_reverse_scale(float x) { return (x - 0.5f) * GUI_POS_SCALE_INV; }


static BBox2 calc_bbox(const Point2 lt, const Point2 rb, const float vtm[2][3])
{
  BBox2 bbox;
  Point2 pts[4] = {lt, rb, Point2(lt.x, rb.y), Point2(rb.x, lt.y)};

  for (int i = 0; i < 4; ++i)
  {
    const Point2 &p = pts[i];
    float x = vtm[0][0] * p.x + vtm[0][1] * p.y + vtm[0][2];
    float y = vtm[1][0] * p.x + vtm[1][1] * p.y + vtm[1][2];
    if (bbox.lim[0].x > x)
      bbox.lim[0].x = x;
    if (bbox.lim[1].x < x)
      bbox.lim[1].x = x;
    if (bbox.lim[0].y > y)
      bbox.lim[0].y = y;
    if (bbox.lim[1].y < y)
      bbox.lim[1].y = y;
  }
  bbox.lim[0].x = gui_reverse_scale(bbox.lim[0].x);
  bbox.lim[0].y = gui_reverse_scale(bbox.lim[0].y);
  bbox.lim[1].x = gui_reverse_scale(bbox.lim[1].x);
  bbox.lim[1].y = gui_reverse_scale(bbox.lim[1].y);
  return bbox;
}


BBox2 calcTransformedBbox(StdGuiRender::GuiContext &ctx, const Point2 lt, const Point2 rb, const RenderState &render_state)
{
  BBox2 bbox;
  if (lt.x > rb.x || lt.y > rb.y)
    return bbox;

  if (render_state.transformActive)
  {
    float vtm[2][3];
    ctx.getViewTm(vtm);
    bbox = calc_bbox(lt, rb, vtm);
  }
  else
  {
    bbox.lim[0] = lt;
    bbox.lim[1] = rb;
  }
  return bbox;
}


BBox2 calcTransformedBbox(const Point2 lt, const Point2 rb, const GuiVertexTransform &gvtm)
{
  if (lt.x > rb.x || lt.y > rb.y)
    return BBox2();
  return calc_bbox(lt, rb, gvtm.vtm);
}


void setTransformedViewPort(StdGuiRender::GuiContext &ctx, const Point2 lt, const Point2 rb, const RenderState &render_state)
{
  viewportDepth++;
  G_ASSERTF(viewportDepth < 100,
    "After setTransformedViewPort must be called restoreTransformedViewPort, NOT StdGuiRender::restore_viewport");

  BBox2 bbox = calcTransformedBbox(ctx, lt, rb, render_state);
  ctx.set_viewport(bbox.left(), bbox.top(), bbox.right(), bbox.bottom());
}


void restoreTransformedViewPort(StdGuiRender::GuiContext &ctx)
{
  viewportDepth--;
  G_ASSERT(viewportDepth >= 0);

  ctx.restore_viewport();
}


} // namespace scenerender

} // namespace darg

#include "renderList.h"

#include <EASTL/sort.h>

#include <memory/dag_framemem.h>
#include <perfMon/dag_statDrv.h>

#include <daRg/dag_element.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_sceneRender.h>

#include <util/dag_convar.h>

#define DEBUG_BOX_CALC 0
#define P2FMT          "%.1f, %.1f"
#if DEBUG_BOX_CALC
#define BOXDBG debug
#else
#define BOXDBG(...) (void)0
#endif


namespace darg
{


CONSOLE_BOOL_VAL("darg", debug_blur_overlaps, false);


void RenderList::push(const RenderEntry &e)
{
  // list.insert(e);
  list.push_back(e);
  if (e.cmd == RCMD_ELEM_RENDER && rendobj_world_blur_id >= 0 && e.elem->rendObjType == rendobj_world_blur_id)
    worldBlurElems.push_back(e.elem);
}


void RenderList::clear()
{
  list.clear();
  worldBlurElems.clear();
}


void RenderList::afterRebuild() { eastl::insertion_sort(list.begin(), list.end(), RenderEntryCompare()); }


void RenderList::render(StdGuiRender::GuiContext &ctx)
{
  TIME_PROFILE(darg_renderlist_render);

  renderState.reset();

  Point2 summaryScrollOffs(0, 0);

  int prevHierOrder = -1;

  BOXDBG("\n\n@#@ render(), vp = " P2FMT " - " P2FMT, P2D(ctx.currentViewPort.leftTop), P2D(ctx.currentViewPort.rightBottom));

  for (RListData::iterator it = list.begin(); it != list.end(); ++it)
  {
    const RenderEntry &re = *it;
    bool hierBreak = re.hierOrder != prevHierOrder + 1;

    // Layout-related actions
    // Actually needed to calculate real screen coordinates for hit testing
    switch (re.cmd)
    {
      case RCMD_ELEM_RENDER:
      {
#if DEBUG_BOX_CALC
        Point2 lt = re.elem->screenCoord.screenPos - summaryScrollOffs;
        Point2 rb = lt + re.elem->screenCoord.size;
        BOXDBG("RCMD_ELEM_RENDER %p: " P2FMT " - " P2FMT " | vp = " P2FMT " - " P2FMT, re.elem, P2D(lt), P2D(rb),
          P2D(ctx.currentViewPort.leftTop), P2D(ctx.currentViewPort.rightBottom));
#endif
        if (ctx.currentViewPort.isNull)
        {
          re.elem->clippedScreenRect.setempty();
          re.elem->transformedBbox.setempty();
          re.elem->updFlags(Element::F_SCREEN_BOX_CLIPPED_OUT, true);
        }
        else
        {
          BBox2 vp(ctx.currentViewPort.leftTop, ctx.currentViewPort.rightBottom);
          Point2 ltRaw = re.elem->screenCoord.screenPos;
          Point2 rbRaw = ltRaw + re.elem->screenCoord.size;
          if (hierBreak || re.elem->transform)
          {
            GuiVertexTransform gvtm;
            re.elem->calcFullTransform(gvtm);
            re.elem->transformedBbox = scenerender::calcTransformedBbox(ltRaw, rbRaw, gvtm);
          }
          else
            re.elem->transformedBbox = scenerender::calcTransformedBbox(ctx, ltRaw, rbRaw, renderState);

          BBox2 &transformed = re.elem->transformedBbox;

          re.elem->clippedScreenRect.lim[0].x = max(transformed.lim[0].x, vp.lim[0].x);
          re.elem->clippedScreenRect.lim[0].y = max(transformed.lim[0].y, vp.lim[0].y);
          re.elem->clippedScreenRect.lim[1].x = min(transformed.lim[1].x, vp.lim[1].x);
          re.elem->clippedScreenRect.lim[1].y = min(transformed.lim[1].y, vp.lim[1].y);
          bool clippedOut = !(re.elem->transformedBbox & vp) && !re.elem->hasFlags(Element::F_IGNORE_EARLY_CLIP);
          re.elem->updFlags(Element::F_SCREEN_BOX_CLIPPED_OUT, clippedOut);
        }
        BOXDBG("Boxes: " P2FMT " - " P2FMT " | " P2FMT " - " P2FMT, P2D(re.elem->transformedBbox.leftTop()),
          P2D(re.elem->transformedBbox.rightBottom()), P2D(re.elem->clippedScreenRect.leftTop()),
          P2D(re.elem->clippedScreenRect.rightBottom()));

        summaryScrollOffs += re.elem->screenCoord.scrollOffs;
        break;
      }
      case RCMD_ELEM_POSTRENDER:
      {
        summaryScrollOffs -= re.elem->screenCoord.scrollOffs;
        break;
      }
      default: break;
    }

    BOXDBG("re.elem->bboxIsClippedOut = %d", re.elem->bboxIsClippedOut());

    // Render and render-related code
    switch (re.cmd)
    {
      case RCMD_ELEM_RENDER:
      {
        if (!re.elem->bboxIsClippedOut())
          re.elem->render(ctx, this, hierBreak);
        else
        {
          GuiVertexTransform prevGvtm;
          ctx.getViewTm(prevGvtm.vtm);
          if (re.elem->updateCtxTransform(ctx, prevGvtm, hierBreak))
          {
            TransformStackItem tsi;
            tsi.elem = re.elem;
            tsi.gvtm = prevGvtm;
            transformStack.push_back(tsi);
          }
        }
        break;
      }
      case RCMD_ELEM_POSTRENDER:
      {
        if (!re.elem->bboxIsClippedOut())
          re.elem->postRender(ctx, this);
        else
        {
          if (transformStack.size() && transformStack.back().elem == re.elem)
          {
            ctx.setViewTm(transformStack.back().gvtm.vtm);
            transformStack.pop_back();
            renderState.transformActive = !transformStack.empty();
          }
        }
        break;
      }
      case RCMD_SET_VIEWPORT:
      {
        Point2 pos = re.elem->screenCoord.screenPos;
        Point2 size = re.elem->screenCoord.size;
        const Offsets &padding = re.elem->layout.padding;
        scenerender::setTransformedViewPort(ctx, pos + padding.lt(), pos + size - padding.rb(), renderState);
        BOXDBG("viewport set: %@ - %@ (pos=%@ size=%@)", ctx.currentViewPort.leftTop, ctx.currentViewPort.rightBottom, pos, size);
        break;
      }
      case RCMD_RESTORE_VIEWPORT:
      {
        scenerender::restoreTransformedViewPort(ctx);
        BOXDBG("viewport restored: " P2FMT " - " P2FMT, P2D(ctx.currentViewPort.leftTop), P2D(ctx.currentViewPort.rightBottom));
        break;
      }
      default: G_ASSERTF(0, "Unexpected render command %d", re.cmd);
    }

    prevHierOrder = re.hierOrder;
  }

  G_ASSERT(transformStack.empty());
  G_ASSERT(opacityStack.empty());
  BOXDBG("\n\n");

#if DEBUG_BOX_CALC
  recalcBoxes(BBox2(ctx.currentViewPort.leftTop, ctx.currentViewPort.rightBottom));
#endif

  if (debug_blur_overlaps)
    debugBlurPanelsOverlap(ctx);
}


void RenderList::debugBlurPanelsOverlap(StdGuiRender::GuiContext &ctx)
{
  int robjId = find_rendobj_factory_id("ROBJ_WORLD_BLUR_PANEL");
  Tab<Element *> collectedPanels(framemem_ptr());
  G_ASSERT(robjId >= 0);

  ctx.set_color(200, 0, 0, 200);
  int nOverlaps = 0;

  for (RListData::iterator it = list.begin(); it != list.end(); ++it)
  {
    const RenderEntry &re = *it;
    if (re.cmd != RCMD_ELEM_RENDER)
      continue;
    if (re.elem->rendObjType == robjId)
    {
      for (Element *prevElem : collectedPanels)
      {
        if (prevElem->clippedScreenRect & re.elem->clippedScreenRect)
        {
          BBox2 b = prevElem->clippedScreenRect;
          b += re.elem->clippedScreenRect;
          ctx.render_box(b.leftTop(), b.rightBottom());
          ++nOverlaps;
        }
      }

      collectedPanels.push_back(re.elem);
    }
  }

  ctx.goto_xy(50, 50);
  ctx.set_font(0);
  String msg(0, "%d blur overlaps", nOverlaps);
  ctx.draw_str(msg);
}


void RenderList::recalcBoxes(const BBox2 &container_viewport)
{
  renderState.reset();

  Point2 summaryScrollOffs(0, 0);

  Tab<BBox2> viewportStack(framemem_ptr());
  BBox2 curViewPort = container_viewport;

  BOXDBG("\n\n@#@ recalcBoxes(), vp = " P2FMT " - " P2FMT, P2D(curViewPort.leftTop()), P2D(curViewPort.rightBottom()));

  bool mismatch = false;
  G_UNUSED(mismatch);

  for (RListData::iterator it = list.begin(); it != list.end(); ++it)
  {
    const RenderEntry &re = *it;

    switch (re.cmd)
    {
      case RCMD_ELEM_RENDER:
      {
#if DEBUG_BOX_CALC
        Point2 lt = re.elem->screenCoord.screenPos - summaryScrollOffs;
        Point2 rb = lt + re.elem->screenCoord.size;

        BOXDBG("RCMD_ELEM_RENDER %p: " P2FMT " - " P2FMT " | vp = " P2FMT " - " P2FMT, re.elem, P2D(lt), P2D(rb),
          P2D(curViewPort.leftTop()), P2D(curViewPort.rightBottom()));

        BBox2 prevClippedRect = re.elem->clippedScreenRect;
        BBox2 prevTransformedRect = re.elem->transformedBbox;
#endif

        if (curViewPort.isempty() || curViewPort.width().lengthSq() < 1e-3f)
        {
          re.elem->clippedScreenRect.setempty();
          re.elem->transformedBbox.setempty();
          re.elem->updFlags(Element::F_SCREEN_BOX_CLIPPED_OUT, true);
        }
        else
        {
          const BBox2 &vp = curViewPort;
          Point2 ltRaw = re.elem->screenCoord.screenPos;
          Point2 rbRaw = ltRaw + re.elem->screenCoord.size;
          GuiVertexTransform gvtm;
          re.elem->calcFullTransform(gvtm);
          re.elem->transformedBbox = scenerender::calcTransformedBbox(ltRaw, rbRaw, gvtm);

          BBox2 &transformed = re.elem->transformedBbox;

          re.elem->clippedScreenRect.lim[0].x = max(transformed.lim[0].x, vp.lim[0].x);
          re.elem->clippedScreenRect.lim[0].y = max(transformed.lim[0].y, vp.lim[0].y);
          re.elem->clippedScreenRect.lim[1].x = min(transformed.lim[1].x, vp.lim[1].x);
          re.elem->clippedScreenRect.lim[1].y = min(transformed.lim[1].y, vp.lim[1].y);
          bool clippedOut = !(re.elem->transformedBbox & vp) && !re.elem->hasFlags(Element::F_IGNORE_EARLY_CLIP);
          re.elem->updFlags(Element::F_SCREEN_BOX_CLIPPED_OUT, clippedOut);
        }

        BOXDBG("Boxes: " P2FMT " - " P2FMT " | " P2FMT " - " P2FMT, P2D(re.elem->transformedBbox.leftTop()),
          P2D(re.elem->transformedBbox.rightBottom()), P2D(re.elem->clippedScreenRect.leftTop()),
          P2D(re.elem->clippedScreenRect.rightBottom()));

#if DEBUG_BOX_CALC
        // G_ASSERT(lengthSq(prevClippedRect.leftTop()-re.elem->clippedScreenRect.leftTop()) < 1e-3f);
        // G_ASSERT(lengthSq(prevClippedRect.rightBottom()-re.elem->clippedScreenRect.rightBottom()) < 1e-3f);
        // G_ASSERT(lengthSq(prevTransformedRect.leftTop()-re.elem->transformedBbox.leftTop()) < 1e-3f);
        // G_ASSERT(lengthSq(prevTransformedRect.leftTop()-re.elem->transformedBbox.leftTop()) < 1e-3f);

        if (!((lengthSq(prevClippedRect.leftTop() - re.elem->clippedScreenRect.leftTop()) < 1e-3f) &&
              (lengthSq(prevClippedRect.rightBottom() - re.elem->clippedScreenRect.rightBottom()) < 1e-3f) &&
              (lengthSq(prevTransformedRect.leftTop() - re.elem->transformedBbox.leftTop()) < 1e-3f) &&
              (lengthSq(prevTransformedRect.leftTop() - re.elem->transformedBbox.leftTop()) < 1e-3f)))
        {
          mismatch = true;
          debug("mismatch prevClippedRect=%@ re.elem->clippedScreenRect=%@ prevTransformedRect=%@ re.elem->transformedBbox=%@",
            prevClippedRect, re.elem->clippedScreenRect, prevTransformedRect, re.elem->transformedBbox);
        }
#endif

        summaryScrollOffs += re.elem->screenCoord.scrollOffs;
        break;
      }
      case RCMD_ELEM_POSTRENDER:
      {
        summaryScrollOffs -= re.elem->screenCoord.scrollOffs;
        break;
      }
      default: break;
    }

    BOXDBG("re.elem->bboxIsClippedOut = %d", re.elem->bboxIsClippedOut());

    if (!re.elem->bboxIsClippedOut())
    {
      switch (re.cmd)
      {
        case RCMD_SET_VIEWPORT:
        {
          Point2 pos = re.elem->screenCoord.screenPos;
          Point2 size = re.elem->screenCoord.size;
          const Offsets &padding = re.elem->layout.padding;

          GuiVertexTransform gvtm;
          re.elem->calcFullTransform(gvtm);

          Point2 lt = pos + padding.lt();
          Point2 rb = pos + size - padding.rb();
          viewportStack.push_back(curViewPort);
          BBox2 new_vp = scenerender::calcTransformedBbox(lt, rb, gvtm);
          curViewPort[0].x = floorf(max(curViewPort[0].x, new_vp[0].x));
          curViewPort[0].y = floorf(max(curViewPort[0].y, new_vp[0].y));
          curViewPort[1].x = floorf(min(curViewPort[1].x, new_vp[1].x));
          curViewPort[1].y = floorf(min(curViewPort[1].y, new_vp[1].y));
          if (curViewPort[0].x >= curViewPort[1].x || curViewPort[0].y >= curViewPort[1].y)
            curViewPort[0] = curViewPort[1] = StdGuiRender::P2::ZERO;

          BOXDBG("viewport set: %@ - %@ (pos=%@ size=%@)", curViewPort[0], curViewPort[1], pos, size);
          break;
        }
        case RCMD_RESTORE_VIEWPORT:
        {
          G_ASSERT_CONTINUE(!viewportStack.empty());
          curViewPort = viewportStack.back();
          BOXDBG("viewport restored: " P2FMT " - " P2FMT, P2D(curViewPort.leftTop()), P2D(curViewPort.rightBottom()));
          viewportStack.pop_back();
          break;
        }
        default: break;
      }
    }
  }

  G_ASSERT(transformStack.empty());
  BOXDBG("\n\n");

#if DEBUG_BOX_CALC
  if (mismatch)
  {
    BOXDBG("@#@ MISMATCH");
    G_ASSERT(!"BOX MISMATCH");
  }
#endif
}

} // namespace darg

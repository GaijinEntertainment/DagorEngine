// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "screen.h"
#include "panel.h"
#include "panelMath3d.h"
#include "guiScene.h"
#include "xmb.h"

#include <util/dag_convar.h>

namespace darg
{

CONSOLE_BOOL_VAL("darg", debug_xmb, false);
CONSOLE_BOOL_VAL("darg", debug_dirpad_nav, false);
CONSOLE_BOOL_VAL("darg", debug_input_stack, false);


Screen::Screen(GuiScene *gui_scene) : etree(gui_scene, this), renderList() {}


void Screen::clear()
{
  etree.clear();
  renderList.clear();
  inputStack.clear();
  cursorStack.clear();
  eventHandlersStack.clear();
}


void Screen::rebuildXmb()
{
  if (etree.root)
    xmb::rebuild_xmb(etree.root, nullptr);
}


void Screen::rebuildStacks()
{
  renderList.clear();
  inputStack.clear();
  cursorStack.clear();
  eventHandlersStack.clear();

  if (etree.root)
  {
    ElemStacks stacks;
    ElemStackCounters counters;
    stacks.rlist = &renderList;
    stacks.input = &inputStack;
    stacks.cursors = &cursorStack;
    stacks.eventHandlers = &eventHandlersStack;
    etree.root->putToSortedStacks(stacks, counters, 0, false);
  }

  renderList.afterRebuild();
}


bool Screen::localToDisplay(const Point2 &panel_pos, const TMatrix &camera_tm, const Frustum &camera_frustum,
  Point2 &out_display_pos) const
{
  if (!panel || !panel->renderInfo.isValid || !panel->spatialInfo.visible || panel->spatialInfo.geometry != PanelGeometry::Rectangle ||
      !panel->spatialInfo.allowDisplayCursorProjection)
  {
    out_display_pos = panel_pos;
    return true;
  }

  G_ASSERT_RETURN(panel->canvas, false);

  return panelmath3d::panel_to_screen_pixels(panel_pos, *panel, camera_tm, camera_frustum, out_display_pos);
}


bool Screen::displayToLocal(const Point2 &display_pos, const TMatrix &camera_tm, const Frustum &camera_frustum,
  Point2 &out_panel_pos) const
{
  if (!panel || !panel->renderInfo.isValid || !panel->spatialInfo.visible || panel->spatialInfo.geometry != PanelGeometry::Rectangle ||
      !panel->spatialInfo.allowDisplayCursorProjection)
  {
    out_panel_pos = display_pos;
    return true;
  }

  G_ASSERT_RETURN(panel->canvas, false);

  return panelmath3d::screen_to_panel_pixels(display_pos, *panel, camera_tm, camera_frustum, out_panel_pos);
}


IPoint2 Screen::getCanvasSize() const
{
  if (panel)
  {
    IPoint2 sz;
    panel->getCanvasSize(sz);
    return sz;
  }
  else
    return {int(StdGuiRender::screen_width()), int(StdGuiRender::screen_height())};
}


void Screen::spawnDebugRenderBox(const BBox2 &box, E3DCOLOR fillColor, E3DCOLOR borderColor, float life_time)
{
  debugRenderBoxes.push_back(DebugRenderBox{box, fillColor, borderColor, life_time, life_time});
}


void Screen::updateDebugRenderBoxes(float dt)
{
  for (int i = int(debugRenderBoxes.size()) - 1; i >= 0; --i)
  {
    debugRenderBoxes[i].timeLeft -= dt;
    if (debugRenderBoxes[i].timeLeft < 0)
      debugRenderBoxes.erase(debugRenderBoxes.begin() + i);
  }
}


void Screen::drawDebugRenderBoxes()
{
  StdGuiRender::GuiContext *guiContext = etree.guiScene->getGuiContext();

  for (DebugRenderBox &box : debugRenderBoxes)
  {
    float k = box.timeLeft / box.lifeTime;
    k = (1 - 2 * k);
    k = k * k;
    k = k * k * k;
    k = 1.0f - k; // apply easing
    k = clamp(k, 0.0f, 1.0f);

    guiContext->set_color(e3dcolor_lerp(0, box.fillColor, k));
    guiContext->render_box(box.box.left(), box.box.top(), box.box.right(), box.box.bottom());

    guiContext->set_color(e3dcolor_lerp(0, box.borderColor, k));
    guiContext->render_frame(box.box.left(), box.box.top(), box.box.right(), box.box.bottom(), 2);
  }
}


void Screen::renderXmbDebug()
{
  StdGuiRender::GuiContext *guiContext = etree.guiScene->getGuiContext();

  for (const InputEntry &ie : inputStack.stack)
  {
    if (ie.elem->xmb)
    {
      BBox2 bbox = ie.elem->calcTransformedBbox();
      if (!bbox.isempty())
      {
        guiContext->set_color(160, 130, 120, 120);
        guiContext->render_frame(bbox.left() + 1, bbox.top() + 1, bbox.right() - 1, bbox.bottom() - 1, 2);
      }
    }
  }

  if (etree.guiScene->xmbFocus && etree.guiScene->xmbFocus->etree == &etree)
  {
    BBox2 bbox = etree.guiScene->xmbFocus->calcTransformedBbox();
    bool isInInputStack = false;
    for (const InputEntry &ie : inputStack.stack)
    {
      if (ie.elem == etree.guiScene->xmbFocus)
      {
        isInInputStack = true;
        break;
      }
    }
    if (isInInputStack)
      guiContext->set_color(220, 200, 0, 180);
    else
      guiContext->set_color(240, 50, 0, 180);
    guiContext->render_frame(bbox.left(), bbox.top(), bbox.right(), bbox.bottom(), 2);

    BBox2 viewport = xmb::calc_xmb_viewport(etree.guiScene->xmbFocus, nullptr);
    guiContext->set_color(20, 200, 0, 180);
    guiContext->render_frame(viewport.left(), viewport.top(), viewport.right(), viewport.bottom(), 2);
  }
}


void Screen::renderDirPadNavDebug()
{
  StdGuiRender::GuiContext *guiContext = etree.guiScene->getGuiContext();

  static E3DCOLOR colors[] = {
    E3DCOLOR(120, 120, 0, 160),
    E3DCOLOR(120, 0, 120, 160),
    E3DCOLOR(0, 120, 120, 160),
    E3DCOLOR(20, 100, 160, 160),
    E3DCOLOR(160, 20, 100, 160),
    E3DCOLOR(100, 160, 20, 160),
  };

  for (const InputEntry &ie : inputStack.stack)
  {
    Element *elem = ie.elem;

    if (!elem->isDirPadNavigable())
      continue;

    const BBox2 &bbox = elem->clippedScreenRect;
    if (bbox.isempty() || bbox.width().x < 1 || bbox.width().y < 1)
      continue;

    const int nColors = sizeof(colors) / sizeof(colors[0]);
    guiContext->set_color(colors[elem->getHierDepth() % nColors]);
    guiContext->render_frame(bbox.left(), bbox.top(), bbox.right(), bbox.bottom(), 2);
  }
}


void Screen::renderInputStackDebug()
{
  StdGuiRender::GuiContext *guiContext = etree.guiScene->getGuiContext();

  for (const InputEntry &ie : inputStack.stack)
  {
    Element *elem = ie.elem;
    if (!elem->behaviorsSummaryFlags)
      continue;

    const BBox2 &bbox = elem->clippedScreenRect;
    if (bbox.isempty())
      continue;

    E3DCOLOR color(20, 20, 20, 200);
    if (elem->hasFlags(Element::F_INPUT_PASSIVE))
      color = E3DCOLOR(120, 160, 160, 200);
    else
    {
      if (elem->hasBehaviors(Behavior::F_HANDLE_MOUSE))
        color.r = 200;
      if (elem->hasBehaviors(Behavior::F_HANDLE_KEYBOARD))
        color.g = 200;
      if (!elem->hasBehaviors(Behavior::F_HANDLE_MOUSE | Behavior::F_HANDLE_KEYBOARD))
        color.b = 200;
    }

    guiContext->set_color(color);
    guiContext->render_frame(bbox.left(), bbox.top(), bbox.right(), bbox.bottom(), 2);
  }
}


void Screen::renderDebugInfo()
{
  if (debug_xmb)
    renderXmbDebug();
  if (debug_dirpad_nav)
    renderDirPadNavDebug();
  if (debug_input_stack)
    renderInputStackDebug();

  drawDebugRenderBoxes();
}


} // namespace darg

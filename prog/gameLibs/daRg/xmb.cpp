// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "xmb.h"
#include "screen.h"
#include "inputStack.h"
#include "dirPadNav.h"
#include <daRg/dag_element.h>
#include <math/integer/dag_IPoint2.h>

namespace darg
{

namespace xmb
{

void rebuild_xmb(Element *node, Element *xmb_parent)
{
  if (xmb_parent)
    G_ASSERT(xmb_parent->xmb);

  if (node->xmb)
  {
    if (xmb_parent)
      xmb_parent->xmb->xmbChildren.push_back(node);
    node->xmb->xmbParent = xmb_parent;
    node->xmb->xmbChildren.clear();
  }

  Element *nextXmbParent = node->xmb ? node : xmb_parent;
  for (Element *child : node->children)
    rebuild_xmb(child, nextXmbParent);
}


BBox2 calc_xmb_viewport(Element *node, Element **nearest_xmb_viewport)
{
  Screen *screen = node->etree->screen;

  IPoint2 canvasSize = screen->getCanvasSize();
  if (canvasSize.x <= 0 || canvasSize.y <= 0)
    return BBox2();

  BBox2 viewport(0, 0, canvasSize.x, canvasSize.y);
  if (!node->xmb)
    return viewport;
  for (Element *e = node->xmb->xmbParent; e; e = e->parent)
  {
    bool isXmbViewport = e->xmb && e->xmb->nodeDesc.RawGetSlotValue("isViewport", false);

    if (isXmbViewport && nearest_xmb_viewport && !*nearest_xmb_viewport)
      *nearest_xmb_viewport = e;

    if (e->hasFlags(Element::F_CLIP_CHILDREN) || isXmbViewport)
    {
      BBox2 b = e->calcTransformedBboxPadded();
      viewport.lim[0] = ::max(viewport.lim[0], b.lim[0]);
      viewport.lim[1] = ::min(viewport.lim[1], b.lim[1]);
    }
  }
  return viewport;
}


bool is_xmb_node_input_covered(Element *node)
{
  Screen *screen = node->etree->screen;

  int focusInputStackDepth = -1, focusZOrder = 0;
  for (size_t iItem = 0, nItems = screen->inputStack.stack.size(); iItem < nItems; ++iItem)
  {
    const InputEntry &ie = screen->inputStack.stack[iItem];
    if (ie.elem == node)
    {
      focusInputStackDepth = iItem;
      focusZOrder = ie.zOrder;
      break;
    }
  }

  if (focusInputStackDepth < 0)
    return false;

  BBox2 viewport = calc_xmb_viewport(node, nullptr);
  if (viewport.isempty())
    return true;

  const float edgeThreshold = StdGuiRender::screen_height() * 0.01f;

  for (size_t iItem = 0; iItem < focusInputStackDepth; ++iItem)
  {
    const InputEntry &ie = screen->inputStack.stack[iItem];
    if (ie.zOrder >= focusZOrder)
    {
      if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK) || ie.elem->hasFlags(Element::F_STOP_POINTING))
      {
        BBox2 bbox = ie.elem->transformedBbox;
        if (bbox.isempty() || bbox.width().x < 0.1f || bbox.width().y < 0.1f)
          bbox = ie.elem->calcTransformedBbox();

        if (bbox.left() <= viewport.left() + edgeThreshold && bbox.top() <= viewport.top() + edgeThreshold &&
            bbox.right() >= viewport.right() - edgeThreshold && bbox.bottom() >= viewport.bottom() - edgeThreshold)
        {
          return true;
        }
      }
    }
  }
  return false;
}


Element *xmb_grid_navigate(Element *cur_focus, Direction dir)
{
  G_ASSERT_RETURN(cur_focus, nullptr);
  G_ASSERT_RETURN(cur_focus->xmb, nullptr);
  Element *xmbParent = cur_focus->xmb->xmbParent;
  G_ASSERT_RETURN(xmbParent, nullptr);
  G_ASSERT_RETURN(xmbParent->xmb, nullptr);
  Element *xmbGrandParent = xmbParent->xmb->xmbParent;
  if (!xmbGrandParent || !xmbGrandParent->xmb)
    return nullptr;
  if (!xmbParent->xmb->nodeDesc.RawGetSlotValue("isGridLine", false))
    return nullptr;
  G_ASSERT_RETURN(xmbParent->layout.flowType == FLOW_HORIZONTAL || xmbParent->layout.flowType == FLOW_VERTICAL, nullptr);

  bool horiz = (dir == DIR_LEFT || dir == DIR_RIGHT);
  if ((xmbParent->layout.flowType == FLOW_HORIZONTAL) == horiz)
    return nullptr;
  if ((xmbGrandParent->layout.flowType == FLOW_HORIZONTAL) != horiz)
    return nullptr;

  auto curCellIt = eastl::find(xmbParent->xmb->xmbChildren.begin(), xmbParent->xmb->xmbChildren.end(), cur_focus);
  G_ASSERT_RETURN(curCellIt != xmbParent->xmb->xmbChildren.end(), nullptr);
  ptrdiff_t curCellIdx = eastl::distance(xmbParent->xmb->xmbChildren.begin(), curCellIt);

  auto curLineIt = eastl::find(xmbGrandParent->xmb->xmbChildren.begin(), xmbGrandParent->xmb->xmbChildren.end(), xmbParent);
  G_ASSERT_RETURN(curLineIt != xmbGrandParent->xmb->xmbChildren.end(), nullptr);
  ptrdiff_t curLineIdx = eastl::distance(xmbGrandParent->xmb->xmbChildren.begin(), curLineIt);

  int delta = (dir == DIR_LEFT || dir == DIR_UP) ? -1 : 1;

  int newLineIdx = curLineIdx + delta;
  bool wrap = xmbGrandParent->xmb->nodeDesc.RawGetSlotValue<bool>("wrap", true);
  if (wrap)
    newLineIdx = (newLineIdx + xmbGrandParent->xmb->xmbChildren.size()) % xmbGrandParent->xmb->xmbChildren.size();

  if (newLineIdx < 0 || newLineIdx >= xmbGrandParent->xmb->xmbChildren.size())
    return nullptr;

  Element *newLine = xmbGrandParent->xmb->xmbChildren[newLineIdx];
  if (newLine->xmb->xmbChildren.empty())
    return nullptr;
  if ((newLine->layout.flowType == FLOW_HORIZONTAL) == horiz)
    return nullptr;

  Element *newCell = newLine->xmb->xmbChildren[::clamp(int(curCellIdx), 0, int(newLine->xmb->xmbChildren.size() - 1))];
  return newCell;
}


Element *xmb_siblings_navigate(Element *cur_focus, Direction dir)
{
  G_ASSERT_RETURN(cur_focus, nullptr);
  G_ASSERT_RETURN(cur_focus->xmb, nullptr);
  Element *xmbParent = cur_focus->xmb->xmbParent;
  G_ASSERT_RETURN(xmbParent, nullptr);
  G_ASSERT_RETURN(xmbParent->xmb, nullptr);

  bool horiz = (dir == DIR_LEFT || dir == DIR_RIGHT);
  int delta = (dir == DIR_LEFT || dir == DIR_UP) ? -1 : 1;

  int newIdx = -1;
  if ((xmbParent->layout.flowType == FLOW_HORIZONTAL && horiz) || (xmbParent->layout.flowType == FLOW_VERTICAL && !horiz))
  {
    XmbData *parentXmbData = xmbParent->xmb;
    G_ASSERT(parentXmbData);
    auto prevIt = eastl::find(parentXmbData->xmbChildren.begin(), parentXmbData->xmbChildren.end(), cur_focus);
    G_ASSERT(prevIt != parentXmbData->xmbChildren.end());
    if (prevIt != parentXmbData->xmbChildren.end())
    {
      ptrdiff_t prevIdx = eastl::distance(parentXmbData->xmbChildren.begin(), prevIt);
      newIdx = prevIdx + delta;
      bool wrap = parentXmbData->nodeDesc.RawGetSlotValue<bool>("wrap", true);
      if (wrap)
        newIdx = (newIdx + parentXmbData->xmbChildren.size()) % parentXmbData->xmbChildren.size();
      else if (newIdx < 0 || newIdx >= parentXmbData->xmbChildren.size())
        newIdx = -1;
    }
  }

  if (newIdx >= 0)
    return xmbParent->xmb->xmbChildren[newIdx];
  else
    return nullptr;
}


Element *xmb_screen_space_navigate(Element *cur_focus, Direction dir)
{
  G_ASSERT_RETURN(cur_focus, nullptr);
  G_ASSERT_RETURN(cur_focus->xmb, nullptr);
  Element *xmbParent = cur_focus->xmb->xmbParent;
  G_ASSERT_RETURN(xmbParent, nullptr);
  G_ASSERT_RETURN(xmbParent->xmb, nullptr);

  float minDistance = 1e9f;
  Element *target = nullptr;
  Point2 srcPos = cur_focus->calcTransformedBbox().center();
  for (Element *elem : xmbParent->xmb->xmbChildren)
  {
    G_ASSERT_CONTINUE(elem->xmb);
    if (elem == cur_focus)
      continue;
    if (!elem->xmb->calcCanFocus())
      continue;

    const BBox2 &bbox = elem->calcTransformedBbox();
    if (bbox.isempty() || bbox.width().x < 1 || bbox.width().y < 1)
      continue;

    float distance = dirpadnav::calc_dir_nav_distance(srcPos, bbox, dir, false);
    if (distance < minDistance)
    {
      target = elem;
      minDistance = distance;
    }
  }

  return target;
}

} // namespace xmb

} // namespace darg

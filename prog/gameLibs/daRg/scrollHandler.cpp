// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scrollHandler.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>

#include "guiScene.h"
#include "dargDebugUtils.h"


#define QUIET_WARNING 1


namespace darg
{


ScrollHandler::ScrollHandler(HSQUIRRELVM vm) : sqfrp::BaseObservable(GuiScene::get_from_sqvm(vm)->frpGraph.get()), elem(nullptr)
{
  initInfo.init(vm);
}


ScrollHandler::~ScrollHandler()
{
  if (elem)
    elem->onScrollHandlerRelease();
}


void ScrollHandler::subscribe(Element *client)
{
  G_ASSERT_RETURN(client, );
  if (elem)
  {
#if QUIET_WARNING
    logwarn("[daRg] ScrollHandler can be bound to one element only. Current: %p, new: %p", elem, client);
#else
    darg_assert_trace_var(String(0, "ScrollHandler can be bound to one element only. Current %p:", elem), elem->props.scriptDesc,
      elem->csk->scrollHandler);
    darg_assert_trace_var(String(0, "ScrollHandler can be bound to one element only. New %p:", client), client->props.scriptDesc,
      client->csk->scrollHandler);
#endif // QUIET_WARNING
    return;
  }
  elem = client;
}


void ScrollHandler::unsubscribe(Element *client)
{
  if (client != elem)
  {
#if QUIET_WARNING
    logwarn("[daRg] ScrollHandler: Bound elem = %p, unsubscribe request for %p", elem, client);
#else
    if (elem)
      darg_assert_trace_var(String(0, "ScrollHandler bound elem %p:", elem), elem->props.scriptDesc, elem->csk->scrollHandler);
    if (client)
      darg_assert_trace_var(String(0, "ScrollHandler requested elem %p:", client), client->props.scriptDesc,
        client->csk->scrollHandler);
    G_ASSERTF(0, "Bound elem = %p, unsubscribe request for %p", elem, client);
#endif // QUIET_WARNING
    return;
  }
  elem = nullptr;
}


void ScrollHandler::scrollToX(float x)
{
  if (elem)
    elem->scrollTo(Point2(x, elem->screenCoord.scrollOffs.y));
}


void ScrollHandler::scrollToY(float y)
{
  if (elem)
    elem->scrollTo(Point2(elem->screenCoord.scrollOffs.x, y));
}


static BBox2 calc_children_bbox(const Element *elem, Sqrat::Function &finder, int depth_left, const Point2 &rel_pos_offs)
{
  BBox2 bbox;
  for (const Element *child : elem->children)
  {
    bool select = false;
    if (!finder.Evaluate(child->props.scriptDesc, select))
      continue;

    if (select)
    {
      bbox += rel_pos_offs + child->screenCoord.relPos;
      bbox += rel_pos_offs + child->screenCoord.relPos + child->screenCoord.size;
    }

    if (depth_left > 1)
    {
      BBox2 sub = calc_children_bbox(child, finder, depth_left - 1, child->screenCoord.relPos);
      bbox += sub;
    }
  }
  return bbox;
}


void ScrollHandler::scrollToChildren(Sqrat::Object finder, int depth, bool x, bool y)
{
  if (!elem)
    return;

  Sqrat::Function f(finder.GetVM(), Sqrat::Object(finder.GetVM()), finder.GetObject());

  Point2 summaryRelPos(0, 0);
  BBox2 bbox = calc_children_bbox(elem, f, depth, Point2(0, 0));
  if (!bbox.isempty())
  {
    ScreenCoord &elSc = elem->screenCoord;
    if (x)
    {
      if (bbox.right() > elSc.scrollOffs.x + elSc.size.x)
        elem->scrollTo(Point2(bbox.right() - elSc.size.x, elSc.scrollOffs.y));
      else if (bbox.left() < elSc.scrollOffs.x)
        elem->scrollTo(Point2(bbox.left(), elSc.scrollOffs.y));
    }
    if (y)
    {
      if (bbox.bottom() > elSc.scrollOffs.y + elSc.size.y)
        elem->scrollTo(Point2(elSc.scrollOffs.x, bbox.bottom() - elSc.size.y));
      else if (bbox.top() < elSc.scrollOffs.y)
        elem->scrollTo(Point2(elSc.scrollOffs.x, bbox.top()));
    }
  }
}


Sqrat::Object ScrollHandler::getValueForNotify() const { return Sqrat::Object(graph->vm); }


void ScrollHandler::fillInfo(Sqrat::Table &t) const { initInfo.fillInfo(t); }

void ScrollHandler::fillInfo(String &s) const { initInfo.toString(s); }


Sqrat::Object ScrollHandler::getElem() const
{
  if (!elem)
    return Sqrat::Object(graph->vm);

  return elem->getRef(graph->vm);
}


} // namespace darg

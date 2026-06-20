// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_guiConstants.h>
#include <quirrel/frp/dag_frp.h>


namespace darg
{

class Element;

class ScrollHandler : public sqfrp::WatchedHandle
{
public:
  ScrollHandler(HSQUIRRELVM vm);
  virtual ~ScrollHandler();

  void subscribe(Element *client);
  void unsubscribe(Element *client);

  void scrollToX(float x);
  void scrollToY(float y);
  // align_x / align_y use the ElemAlign enum. PLACE_DEFAULT preserves the
  // "minimal scroll to make visible" behavior; ALIGN_LEFT_OR_TOP, ALIGN_CENTER
  // and ALIGN_RIGHT_OR_BOTTOM force the matched bbox to that viewport edge.
  void scrollToChildren(Sqrat::Object finder, int depth, bool x, bool y, int align_x = PLACE_DEFAULT, int align_y = PLACE_DEFAULT);

  Sqrat::Object getElem() const;

private:
  Element *elem;
  sqfrp::ScriptSourceInfo initInfo;
};

} // namespace darg

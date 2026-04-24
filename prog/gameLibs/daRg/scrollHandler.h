// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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
  void scrollToChildren(Sqrat::Object finder, int depth, bool x, bool y);

  Sqrat::Object getElem() const;

private:
  Element *elem;
  sqfrp::ScriptSourceInfo initInfo;
};

} // namespace darg

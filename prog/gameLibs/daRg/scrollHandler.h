#pragma once

#include <quirrel/frp/dag_frp.h>


namespace darg
{

class Element;

class ScrollHandler : public sqfrp::BaseObservable
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

  virtual Sqrat::Object getValueForNotify() const override;

  virtual void fillInfo(Sqrat::Table &) const override;
  virtual void fillInfo(String &) const override;

private:
  Element *elem;
  sqfrp::ScriptSourceInfo initInfo;
};

} // namespace darg

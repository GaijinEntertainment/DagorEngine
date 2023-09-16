#pragma once

#include <sqrat.h>

namespace darg
{

class Element;

class ElementRef
{
public:
  ElementRef();
  ~ElementRef();

  Sqrat::Object createScriptInstance(HSQUIRRELVM vm);
  static ElementRef *get_from_stack(HSQUIRRELVM vm, int idx);

  static void bind_script(HSQUIRRELVM vm);

public:
  Element *elem = nullptr;
  int scriptInstancesCount = 0;
};

} // namespace darg

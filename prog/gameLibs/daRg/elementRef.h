// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat.h>

namespace darg
{

class Element;

class ElementRef
{
public:
  ElementRef(Element *);
  ~ElementRef();

  Sqrat::Object createScriptInstance(HSQUIRRELVM vm);
  static ElementRef *get_from_stack(HSQUIRRELVM vm, int idx);
  static ElementRef *cast_from_sqrat_obj(const Sqrat::Object &obj);

  static void bind_script(HSQUIRRELVM vm);

public:
  Element *elem = nullptr;
  int scriptInstancesCount = 0;
};

} // namespace darg

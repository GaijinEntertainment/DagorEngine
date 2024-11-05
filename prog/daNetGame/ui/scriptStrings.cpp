// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scriptStrings.h"

void ui::preregister_strings(HSQUIRRELVM vm, Sqrat::Object *strings, unsigned strings_count, const char *comma_sep_string_names)
{
  // debug("strings=%p,%d <%s>", strings, strings_count, comma_sep_string_names);
  for (; strings_count > 0; --strings_count, ++strings)
  {
    const char *pcomma = strchr(comma_sep_string_names, ',');
    if (!pcomma)
      pcomma = comma_sep_string_names + strlen(comma_sep_string_names);
    sq_pushstring(vm, comma_sep_string_names, pcomma - comma_sep_string_names);
    HSQOBJECT hobj;
    G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &hobj)));
    *strings = Sqrat::Object(hobj, vm);
    sq_pop(vm, 1);
    // debug("obj=%p =%s (%.*s)", //
    //   strings, strings->GetVar<const SQChar *>().value, pcomma - comma_sep_string_names, comma_sep_string_names);
    if (strings_count > 1)
    {
      comma_sep_string_names = pcomma + 1;
      while (*comma_sep_string_names == ' ')
        comma_sep_string_names++;
    }
    else
      comma_sep_string_names = nullptr;
  }
}

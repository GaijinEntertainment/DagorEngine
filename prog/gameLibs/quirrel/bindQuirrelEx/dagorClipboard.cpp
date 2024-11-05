// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqModules/sqModules.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include <osApiWrappers/dag_clipboard.h>
#include <memory/dag_framemem.h>
#include <EASTL/vector.h>


static SQInteger set_clipboard_text(HSQUIRRELVM vm)
{
  const char *text = nullptr;
  sq_getstring(vm, 2, &text);
  if (text)
    clipboard::set_clipboard_utf8_text(text);
  return 0;
}


static SQInteger get_clipboard_text(HSQUIRRELVM vm)
{
  const SQInteger maxLen = 32 << 10;
  eastl::vector<char, framemem_allocator> buf;
  buf.resize(maxLen);
  if (clipboard::get_clipboard_utf8_text(&buf[0], buf.size()))
    sq_pushstring(vm, &buf[0], -1);
  else
    sq_pushstring(vm, "", 0);
  return 1;
}


namespace bindquirrel
{

void register_dagor_clipboard(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  ///@module dagor.clipboard
  exports //
    .SquirrelFunc("set_clipboard_text", set_clipboard_text, 2, ".s")
    .SquirrelFunc("get_clipboard_text", get_clipboard_text, 1, ".")
    /**/;
  module_mgr->addNativeModule("dagor.clipboard", exports);
}


} // namespace bindquirrel
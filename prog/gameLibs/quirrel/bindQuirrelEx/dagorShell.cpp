// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqmodules/sqmodules.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include <osApiWrappers/dag_shellExecute.h>
#include <osApiWrappers/dag_unicode.h>
#include <EASTL/string.h>

static eastl::wstring utf8_to_wstring(const eastl::string_view &text)
{
  eastl::wstring textU16;
  textU16.resize(text.size() + 1);

  int used = utf8_to_wcs_ex(text.data(), text.size(), textU16.data(), textU16.size());
  G_ASSERT(used <= text.size());
  textU16.resize(used);
  return textU16;
}

static SQInteger shell_execute(HSQUIRRELVM vm)
{
  bool force_sync = true;
  bool hidden = false;
  bool convert_to_wide_string = false;
  eastl::string cmd = "open";
  eastl::string file = "";
  eastl::string dir = "";
  eastl::string params = "";

  Sqrat::Table args = Sqrat::Var<Sqrat::Table>(vm, 2).value;
  force_sync = args.RawGetSlotValue<bool>("force_sync", force_sync);
  hidden = args.RawGetSlotValue<bool>("hidden", hidden);
  convert_to_wide_string = args.RawGetSlotValue<bool>("convert_to_wide_string", convert_to_wide_string);
  cmd = args.RawGetSlotValue<eastl::string>("cmd", cmd);
  file = args.RawGetSlotValue<eastl::string>("file", file);
  params = args.RawGetSlotValue<eastl::string>("params", params);
  dir = args.RawGetSlotValue<eastl::string>("dir", dir);

  if (cmd.empty())
    return sq_throwerror(vm, "cmd should be set!");

  if (file.empty() && dir.empty())
    return sq_throwerror(vm, "file or/and dir should be set!");

  if (!convert_to_wide_string)
    os_shell_execute(cmd.c_str(), file.c_str(), params.c_str(), dir.c_str(), force_sync,
      hidden ? OpenConsoleMode::Hide : OpenConsoleMode::Show);
  else
    os_shell_execute_w(utf8_to_wstring(cmd).c_str(), utf8_to_wstring(file).c_str(), utf8_to_wstring(params).c_str(),
      utf8_to_wstring(dir).c_str(), force_sync, hidden ? OpenConsoleMode::Hide : OpenConsoleMode::Show);

  return 0;
}

namespace bindquirrel
{

void register_dagor_shell(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  ///@module dagor.shell
  exports.SquirrelFuncDeclString(shell_execute, "shell_execute(params: table): null",
    "params are: {cmd:string, file:string, dir:string, params:string, force_sync:boolean, hidden:boolean}\nfile or dir should be set. "
    "params is optional");
  /* qdox
  @function shell_execute
  @param params t : table to get result
  @code params sq
    {cmd:string, file:string, dir:string, params:string, force_sync:boolean, hidden:boolean}
  code@
  file or dir should be set. params is optional
  */
  module_mgr->addNativeModule("dagor.shell", exports);
}

} // namespace bindquirrel
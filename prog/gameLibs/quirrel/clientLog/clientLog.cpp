// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqModules/sqModules.h>
#include <eventLog/errorLog.h>
#include <quirrel_json/jsoncpp.h>

namespace clientlog
{

void bind_script(SqModules *moduleMgr, const char *vm_name)
{
  HSQUIRRELVM vm = moduleMgr->getVM();
  Sqrat::Table tbl(vm);
  tbl.Func("send_error_log", [vm_name = eastl::string(vm_name)](const char *log_data, Sqrat::Table params) {
    event_log::ErrorLogSendParams eparams;
    eparams.attach_game_log = params.GetSlotValue("attach_game_log", false);
    eastl::string collection = params.GetSlotValue<eastl::string>("collection", "sqassert");
    eparams.collection = collection.c_str();
    eparams.meta = quirrel_to_jsoncpp(params.GetSlot("meta"));
    eparams.meta["vm"] = vm_name.c_str();
    event_log::send_error_log(log_data, eparams);
  });
  moduleMgr->addNativeModule("clientlog", tbl);
}

} // namespace clientlog

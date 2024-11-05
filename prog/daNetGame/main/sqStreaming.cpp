// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqrat.h>
#include <streaming/streaming.h>
#include <bindQuirrelEx/autoBind.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>


SQ_DEF_AUTO_BINDING_MODULE(bind_streaming, "streaming")
{
  Sqrat::Table exports(vm);
  exports //
    .Func("set_streaming_now", streaming::set_streaming_now)
    .Func("is_streaming_now", streaming::is_streaming_now)
    /**/;
  return exports;
}

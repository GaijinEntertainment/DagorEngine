// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_stdint.h>
#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>
void pull_input_das()
{
  NEED_MODULE(DagorInputModule)
  NEED_MODULE(TouchInputModule)
}

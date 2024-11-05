// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_stdint.h>
#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>
void pull_imgui_das()
{
  NEED_MODULE(Module_dasIMGUI)
  NEED_MODULE(Module_dasIMGUI_NODE_EDITOR)
  NEED_MODULE(DagorImguiModule)
}

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_IEditorCore.h>

namespace console
{

int collector_cmp(const char *arg, int ac, const char *cmd, int min_ac, int max_ac, const char *description,
  const char *argsDescription, const char *varValue, eastl::vector<console::CommandOptions> &&cmdOptions)
{
  G_ASSERT(editorcore_extapi::dagConsole);
  return editorcore_extapi::dagConsole->conCollectorCmp(arg, ac, cmd, min_ac, max_ac, description, argsDescription, varValue,
    eastl::move(cmdOptions));
}

} // namespace console

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_command_text.h"

#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_interface.h>

const char *command_hint(const char *command_id)
{
  IEditorCommandSystem *const commandSystem = EDITORCORE->queryEditorInterface<IEditorCommandSystem>();
  const char *const text = commandSystem ? commandSystem->getCommandKeyChordsAsText(command_id) : nullptr;
  return text ? text : "";
}

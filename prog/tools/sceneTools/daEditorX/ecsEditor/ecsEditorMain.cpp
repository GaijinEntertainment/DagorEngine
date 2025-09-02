// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ecsEditorMain.h"
#include "ecsEditorPlugin.h"

static ECSEditorPlugin *plugin = nullptr;

void init_plugin_ecs_editor()
{
  plugin = new (inimem) ECSEditorPlugin;
  if (!DAGORED2->registerPlugin(plugin))
    del_it(plugin);
}

void ecs_editor_set_viewport_message(const char *message, bool auto_hide)
{
  G_ASSERT(plugin);
  plugin->setViewportMessage(message, auto_hide);
}

ECSObjectEditor &get_ecs_object_editor()
{
  G_ASSERT(plugin);
  ECSObjectEditor *editor = plugin->getObjectEditor();
  G_ASSERT(editor);
  return *editor;
}
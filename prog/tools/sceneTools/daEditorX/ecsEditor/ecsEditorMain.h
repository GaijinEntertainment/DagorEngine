// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class ECSObjectEditor;

void ecs_editor_set_viewport_message(const char *message, bool auto_hide = false);
ECSObjectEditor &get_ecs_object_editor();
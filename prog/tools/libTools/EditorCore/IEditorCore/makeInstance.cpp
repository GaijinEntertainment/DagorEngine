// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "implementIEditorCore.h"
#include <EditorCore/ec_wndGlobal.h>

static EditorCoreImpl editorCoreImpl;
IEditorCore &IEditorCore::make_instance() { return editorCoreImpl; }

const char *EditorCoreImpl::getExePath() { return sgg::get_exe_path_full(); }

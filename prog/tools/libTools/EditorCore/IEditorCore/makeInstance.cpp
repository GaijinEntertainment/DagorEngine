#include "implementIEditorCore.h"
#include <sepGui/wndGlobal.h>

static EditorCoreImpl editorCoreImpl;
IEditorCore &IEditorCore::make_instance() { return editorCoreImpl; }

const char *EditorCoreImpl::getExePath() { return sgg::get_exe_path_full(); }

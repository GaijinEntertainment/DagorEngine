#include <daEditorE/daEditorE.h>
#include <daEditorE/de_objEditor.h>
#include <daEditorE/de_interface.h>
#include <squirrel.h>
#include <sqrat.h>
#include <quirrel/sqModules/sqModules.h>
#include <quirrel/sqEventBus/sqEventBus.h>


namespace
{
static ObjectEditor **curObjEdRef;
static inline ObjectEditor *objEd() { return *curObjEdRef; }
} // namespace


void register_da_editor4_objed_ptr(ObjectEditor **oe_ptr) { curObjEdRef = oe_ptr; }

static void set_work_mode_sq(const char *mode)
{
  if (objEd())
    objEd()->setWorkMode(mode);
}
static const char *get_work_mode_sq() { return objEd() ? objEd()->getWorkMode() : ""; }

static void set_edit_mode_sq(int mode) { objEd()->setEditMode(mode); }
static int get_edit_mode_sq() { return objEd() ? objEd()->getEditMode() : CM_OBJED_MODE_SELECT; }
static bool is_free_cam_mode_sq() { return DAEDITOR4.isFreeCameraActive(); }


void set_point_action_preview_sq(const char *shape, float param)
{
  if (objEd())
    objEd()->setPointActionPreview(shape, param);
}


/// @module daEditorEmbedded

void register_da_editor4_script(SqModules *module_mgr)
{
  if (!curObjEdRef)
    return;

  Sqrat::Table daEditor(module_mgr->getVM());
  daEditor.Func("setEditMode", &set_edit_mode_sq)
    .Func("getEditMode", &get_edit_mode_sq)
    .Func("isFreeCamMode", &is_free_cam_mode_sq)
    .SetValue("DE4_MODE_SELECT", CM_OBJED_MODE_SELECT)
    .SetValue("DE4_MODE_MOVE", CM_OBJED_MODE_MOVE)
    .SetValue("DE4_MODE_MOVE_SURF", CM_OBJED_MODE_SURF_MOVE)
    .SetValue("DE4_MODE_ROTATE", CM_OBJED_MODE_ROTATE)
    .SetValue("DE4_MODE_SCALE", CM_OBJED_MODE_SCALE)
    .SetValue("DE4_MODE_POINT_ACTION", CM_OBJED_MODE_POINT_ACTION)
    .SetValue("DE4_CMD_DROP", CM_OBJED_DROP)
    .SetValue("DE4_CMD_DEL", CM_OBJED_DELETE)
    .Func("setWorkMode", &set_work_mode_sq)
    .Func("getWorkMode", &get_work_mode_sq)
    .Func("setPointActionPreview", &set_point_action_preview_sq);
  module_mgr->addNativeModule("daEditorEmbedded", daEditor);
}


void ObjectEditor::updateToolbarButtons()
{
  if (HSQUIRRELVM vm = sqeventbus::get_vm())
  {
    sqeventbus::send_event("daEditorEmbedded.onDeSetWorkMode", Sqrat::Object(workMode.c_str(), vm));
    sqeventbus::send_event("daEditorEmbedded.onDeSetEditMode", Sqrat::Object(editMode, vm));
  }
}

// this file is generated via Daslang automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasIMGUI_NODE_EDITOR.h"
#include "need_dasIMGUI_NODE_EDITOR.h"
#include "dasIMGUI_NODE_EDITOR.struct.impl.inc"
namespace das {
#include "dasIMGUI_NODE_EDITOR.enum.class.inc"
#include "dasIMGUI_NODE_EDITOR.struct.class.inc"
#include "dasIMGUI_NODE_EDITOR.func.aot.inc"
Module_dasIMGUI_NODE_EDITOR::Module_dasIMGUI_NODE_EDITOR() : Module("imgui_node_editor") {
}
bool Module_dasIMGUI_NODE_EDITOR::initDependencies() {
	if ( initialized ) return true;
	auto mod_imgui = Module::require("imgui");
	if ( !mod_imgui ) return false;
	if ( !mod_imgui->initDependencies() ) return false;
	initialized = true;
	lib.addModule(this);
	lib.addBuiltInModule();
	lib.addModule(mod_imgui);
	initAotAlias();
	#include "dasIMGUI_NODE_EDITOR.const.inc"
	#include "dasIMGUI_NODE_EDITOR.enum.add.inc"
	#include "dasIMGUI_NODE_EDITOR.dummy.add.inc"
	#include "dasIMGUI_NODE_EDITOR.struct.add.inc"
	#include "dasIMGUI_NODE_EDITOR.struct.postadd.inc"
	#include "dasIMGUI_NODE_EDITOR.alias.add.inc"
	#include "dasIMGUI_NODE_EDITOR.func.reg.inc"
	initMain();
	return true;
}
}
REGISTER_MODULE_IN_NAMESPACE(Module_dasIMGUI_NODE_EDITOR,das);


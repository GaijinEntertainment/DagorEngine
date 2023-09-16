// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasIMGUI.h"
#include "need_dasIMGUI.h"
#include "dasIMGUI.struct.impl.inc"
namespace das {
#include "dasIMGUI.enum.class.inc"
#include "dasIMGUI.struct.class.inc"
#include "dasIMGUI.func.aot.inc"
Module_dasIMGUI::Module_dasIMGUI() : Module("imgui") {
}
bool Module_dasIMGUI::initDependencies() {
	if ( initialized ) return true;
	initialized = true;
	lib.addModule(this);
	lib.addBuiltInModule();
	initAotAlias();
	#include "dasIMGUI.const.inc"
	#include "dasIMGUI.enum.add.inc"
	#include "dasIMGUI.dummy.add.inc"
	#include "dasIMGUI.struct.add.inc"
	#include "dasIMGUI.struct.postadd.inc"
	#include "dasIMGUI.alias.add.inc"
	#include "dasIMGUI.func.reg.inc"
	initMain();
	return true;
}
}
REGISTER_MODULE_IN_NAMESPACE(Module_dasIMGUI,das);


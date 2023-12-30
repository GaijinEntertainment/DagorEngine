// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasLLVM.h"
#include "need_dasLLVM.h"
#include "dasLLVM.struct.impl.inc"
namespace das {
#include "dasLLVM.enum.class.inc"
#include "dasLLVM.struct.class.inc"
#include "dasLLVM.func.aot.inc"
Module_dasLLVM::Module_dasLLVM() : Module("llvm") {
}
bool Module_dasLLVM::initDependencies() {
	if ( initialized ) return true;
	initialized = true;
	lib.addModule(this);
	lib.addBuiltInModule();
	#include "dasLLVM.enum.add.inc"
	#include "dasLLVM.dummy.add.inc"
	#include "dasLLVM.struct.add.inc"
	#include "dasLLVM.struct.postadd.inc"
	#include "dasLLVM.alias.add.inc"
	#include "dasLLVM.func.reg.inc"
	initMain();
	return true;
}
}
REGISTER_MODULE_IN_NAMESPACE(Module_dasLLVM,das);


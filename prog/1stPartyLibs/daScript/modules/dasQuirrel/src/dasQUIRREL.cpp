// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasQUIRREL.h"
#include "need_dasQUIRREL.h"
#include "dasQUIRREL.struct.impl.inc"
namespace das {
#include "dasQUIRREL.enum.class.inc"
#include "dasQUIRREL.struct.class.inc"
#include "dasQUIRREL.func.aot.inc"
Module_dasQUIRREL::Module_dasQUIRREL() : Module("quirrel") {
}
bool Module_dasQUIRREL::initDependencies() {
	if ( initialized ) return true;
	initialized = true;
	lib.addModule(this);
	lib.addBuiltInModule();
	#include "dasQUIRREL.enum.add.inc"
	#include "dasQUIRREL.dummy.add.inc"
	#include "dasQUIRREL.struct.add.inc"
	#include "dasQUIRREL.struct.postadd.inc"
	#include "dasQUIRREL.alias.add.inc"
	#include "dasQUIRREL.func.reg.inc"
	initMain();
	initBind();
	return true;
}
}
REGISTER_MODULE_IN_NAMESPACE(Module_dasQUIRREL,das);


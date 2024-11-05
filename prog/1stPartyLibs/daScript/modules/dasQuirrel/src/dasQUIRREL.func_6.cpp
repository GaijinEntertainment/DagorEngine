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
namespace das {
#include "dasQUIRREL.func.aot.decl.inc"
void Module_dasQUIRREL::initFunctions_6() {
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:331:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_getdelegate >(*this,lib,"sq_getdelegate",SideEffects::worstDefault,"sq_getdelegate")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:332:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_clone >(*this,lib,"sq_clone",SideEffects::worstDefault,"sq_clone")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:333:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQUnsignedInteger) , sq_setfreevariable >(*this,lib,"sq_setfreevariable",SideEffects::worstDefault,"sq_setfreevariable")
		->args({"v","idx","nval"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:334:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_next >(*this,lib,"sq_next",SideEffects::worstDefault,"sq_next")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:335:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_getweakrefval >(*this,lib,"sq_getweakrefval",SideEffects::worstDefault,"sq_getweakrefval")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:336:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQBool) , sq_clear >(*this,lib,"sq_clear",SideEffects::worstDefault,"sq_clear")
		->args({"v","idx","freemem"})->arg_init(2, make_smart<ExprConstUInt>(1u));
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:337:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_freeze >(*this,lib,"sq_freeze",SideEffects::worstDefault,"sq_freeze")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:340:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQBool,SQBool) , sq_call >(*this,lib,"sq_call",SideEffects::worstDefault,"sq_call")
		->args({"v","params","retval","invoke_err_handler"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:341:23
	addExtern< SQRESULT (*)(SQVM *,SQBool,SQBool) , sq_resume >(*this,lib,"sq_resume",SideEffects::worstDefault,"sq_resume")
		->args({"v","retval","invoke_err_handler"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:342:28
	addExtern< const char * (*)(SQVM *,SQUnsignedInteger,SQUnsignedInteger) , sq_getlocal >(*this,lib,"sq_getlocal",SideEffects::worstDefault,"sq_getlocal")
		->args({"v","level","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:343:23
	addExtern< SQRESULT (*)(SQVM *) , sq_getcallee >(*this,lib,"sq_getcallee",SideEffects::worstDefault,"sq_getcallee")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:344:28
	addExtern< const char * (*)(SQVM *,SQInteger,SQUnsignedInteger) , sq_getfreevariable >(*this,lib,"sq_getfreevariable",SideEffects::worstDefault,"sq_getfreevariable")
		->args({"v","idx","nval"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:345:23
	addExtern< SQRESULT (*)(SQVM *,const char *) , sq_throwerror >(*this,lib,"sq_throwerror",SideEffects::worstDefault,"sq_throwerror")
		->args({"v","err"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:346:23
	addExtern< SQRESULT (*)(SQVM *) , sq_throwobject >(*this,lib,"sq_throwobject",SideEffects::worstDefault,"sq_throwobject")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:347:19
	addExtern< void (*)(SQVM *) , sq_reseterror >(*this,lib,"sq_reseterror",SideEffects::worstDefault,"sq_reseterror")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:348:19
	addExtern< void (*)(SQVM *) , sq_getlasterror >(*this,lib,"sq_getlasterror",SideEffects::worstDefault,"sq_getlasterror")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:349:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_tailcall >(*this,lib,"sq_tailcall",SideEffects::worstDefault,"sq_tailcall")
		->args({"v","nparams"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:352:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,tagSQObject *) , sq_getstackobj >(*this,lib,"sq_getstackobj",SideEffects::worstDefault,"sq_getstackobj")
		->args({"v","idx","po"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:353:19
	addExtern< void (*)(SQVM *,tagSQObject) , sq_pushobject >(*this,lib,"sq_pushobject",SideEffects::worstDefault,"sq_pushobject")
		->args({"v","obj"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:354:19
	addExtern< void (*)(SQVM *,tagSQObject *) , sq_addref >(*this,lib,"sq_addref",SideEffects::worstDefault,"sq_addref")
		->args({"v","po"});
}
}


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
void Module_dasQUIRREL::initFunctions_1() {
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:210:26
	addExtern< SQVM * (*)(SQInteger) , sq_open >(*this,lib,"sq_open",SideEffects::worstDefault,"sq_open")
		->args({"initialstacksize"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:211:26
	addExtern< SQVM * (*)(SQVM *,SQInteger) , sq_newthread >(*this,lib,"sq_newthread",SideEffects::worstDefault,"sq_newthread")
		->args({"friendvm","initialstacksize"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:212:19
	addExtern< void (*)(SQVM *) , sq_seterrorhandler >(*this,lib,"sq_seterrorhandler",SideEffects::worstDefault,"sq_seterrorhandler")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:213:24
	addExtern< tagSQObject (*)(SQVM *) , sq_geterrorhandler ,SimNode_ExtFuncCallAndCopyOrMove>(*this,lib,"sq_geterrorhandler",SideEffects::worstDefault,"sq_geterrorhandler")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:214:19
	addExtern< void (*)(SQVM *) , sq_close >(*this,lib,"sq_close",SideEffects::worstDefault,"sq_close")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:215:19
	addExtern< void (*)(SQVM *,void *) , sq_setforeignptr >(*this,lib,"sq_setforeignptr",SideEffects::worstDefault,"sq_setforeignptr")
		->args({"v","p"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:216:28
	addExtern< void * (*)(SQVM *) , sq_getforeignptr >(*this,lib,"sq_getforeignptr",SideEffects::worstDefault,"sq_getforeignptr")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:217:19
	addExtern< void (*)(SQVM *,void *) , sq_setsharedforeignptr >(*this,lib,"sq_setsharedforeignptr",SideEffects::worstDefault,"sq_setsharedforeignptr")
		->args({"v","p"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:218:28
	addExtern< void * (*)(SQVM *) , sq_getsharedforeignptr >(*this,lib,"sq_getsharedforeignptr",SideEffects::worstDefault,"sq_getsharedforeignptr")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:226:23
	addExtern< SQRESULT (*)(SQVM *) , sq_suspendvm >(*this,lib,"sq_suspendvm",SideEffects::worstDefault,"sq_suspendvm")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:227:23
	addExtern< SQRESULT (*)(SQVM *,SQBool,SQBool,SQBool,SQBool) , sq_wakeupvm >(*this,lib,"sq_wakeupvm",SideEffects::worstDefault,"sq_wakeupvm")
		->args({"v","resumedret","retval","invoke_err_handler","throwerror"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:228:24
	addExtern< SQInteger (*)(SQVM *) , sq_getvmstate >(*this,lib,"sq_getvmstate",SideEffects::worstDefault,"sq_getvmstate")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:229:23
	addExtern< SQRESULT (*)(SQVM *) , sq_registerbaselib >(*this,lib,"sq_registerbaselib",SideEffects::worstDefault,"sq_registerbaselib")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:233:23
	addExtern< SQRESULT (*)(SQVM *,const char *,SQInteger,const char *,SQBool,const tagSQObject *) , sq_compile >(*this,lib,"sq_compile",SideEffects::worstDefault,"sq_compile")
		->args({"v","s","size","sourcename","raiseerror","bindings"})
		->arg_init(5,make_smart<ExprConstPtr>());
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:234:19
	addExtern< void (*)(SQVM *,SQBool) , sq_enabledebuginfo >(*this,lib,"sq_enabledebuginfo",SideEffects::worstDefault,"sq_enabledebuginfo")
		->args({"v","enable"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:235:19
	addExtern< void (*)(SQVM *,SQBool) , sq_enablevartrace >(*this,lib,"sq_enablevartrace",SideEffects::worstDefault,"sq_enablevartrace")
		->args({"v","enable"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:236:21
	addExtern< SQBool (*)() , sq_isvartracesupported >(*this,lib,"sq_isvartracesupported",SideEffects::worstDefault,"sq_isvartracesupported");
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:237:19
	addExtern< void (*)(SQVM *,SQBool) , sq_lineinfo_in_expressions >(*this,lib,"sq_lineinfo_in_expressions",SideEffects::worstDefault,"sq_lineinfo_in_expressions")
		->args({"v","enable"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:238:19
	addExtern< void (*)(SQVM *,SQBool) , sq_notifyallexceptions >(*this,lib,"sq_notifyallexceptions",SideEffects::worstDefault,"sq_notifyallexceptions")
		->args({"v","enable"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:243:19
	addExtern< void (*)(SQVM *,SQInteger) , sq_push >(*this,lib,"sq_push",SideEffects::worstDefault,"sq_push")
		->args({"v","idx"});
}
}


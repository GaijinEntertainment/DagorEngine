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
void Module_dasQUIRREL::initFunctions_5() {
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:311:23
	addExtern< SQRESULT (*)(SQVM *) , sq_setroottable >(*this,lib,"sq_setroottable",SideEffects::worstDefault,"sq_setroottable")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:312:23
	addExtern< SQRESULT (*)(SQVM *) , sq_setconsttable >(*this,lib,"sq_setconsttable",SideEffects::worstDefault,"sq_setconsttable")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:313:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQBool) , sq_newslot >(*this,lib,"sq_newslot",SideEffects::worstDefault,"sq_newslot")
		->args({"v","idx","bstatic"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:314:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQBool) , sq_deleteslot >(*this,lib,"sq_deleteslot",SideEffects::worstDefault,"sq_deleteslot")
		->args({"v","idx","pushval"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:315:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_set >(*this,lib,"sq_set",SideEffects::worstDefault,"sq_set")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:316:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_get >(*this,lib,"sq_get",SideEffects::worstDefault,"sq_get")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:317:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_get_noerr >(*this,lib,"sq_get_noerr",SideEffects::worstDefault,"sq_get_noerr")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:318:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_rawget >(*this,lib,"sq_rawget",SideEffects::worstDefault,"sq_rawget")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:319:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_rawget_noerr >(*this,lib,"sq_rawget_noerr",SideEffects::worstDefault,"sq_rawget_noerr")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:320:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_rawset >(*this,lib,"sq_rawset",SideEffects::worstDefault,"sq_rawset")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:321:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQBool) , sq_rawdeleteslot >(*this,lib,"sq_rawdeleteslot",SideEffects::worstDefault,"sq_rawdeleteslot")
		->args({"v","idx","pushval"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:322:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQBool) , sq_newmember >(*this,lib,"sq_newmember",SideEffects::worstDefault,"sq_newmember")
		->args({"v","idx","bstatic"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:324:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_arrayappend >(*this,lib,"sq_arrayappend",SideEffects::worstDefault,"sq_arrayappend")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:325:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQBool) , sq_arraypop >(*this,lib,"sq_arraypop",SideEffects::worstDefault,"sq_arraypop")
		->args({"v","idx","pushval"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:326:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQInteger) , sq_arrayresize >(*this,lib,"sq_arrayresize",SideEffects::worstDefault,"sq_arrayresize")
		->args({"v","idx","newsize"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:327:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_arrayreverse >(*this,lib,"sq_arrayreverse",SideEffects::worstDefault,"sq_arrayreverse")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:328:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQInteger) , sq_arrayremove >(*this,lib,"sq_arrayremove",SideEffects::worstDefault,"sq_arrayremove")
		->args({"v","idx","itemidx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:329:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQInteger) , sq_arrayinsert >(*this,lib,"sq_arrayinsert",SideEffects::worstDefault,"sq_arrayinsert")
		->args({"v","idx","destpos"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:330:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_setdelegate >(*this,lib,"sq_setdelegate",SideEffects::worstDefault,"sq_setdelegate")
		->args({"v","idx"});
}
}


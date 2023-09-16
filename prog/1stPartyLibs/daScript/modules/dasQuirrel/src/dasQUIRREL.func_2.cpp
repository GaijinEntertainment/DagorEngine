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
void Module_dasQUIRREL::initFunctions_2() {
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:244:19
	addExtern< void (*)(SQVM *,SQInteger) , sq_pop >(*this,lib,"sq_pop",SideEffects::worstDefault,"sq_pop")
		->args({"v","nelemstopop"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:245:19
	addExtern< void (*)(SQVM *) , sq_poptop >(*this,lib,"sq_poptop",SideEffects::worstDefault,"sq_poptop")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:246:19
	addExtern< void (*)(SQVM *,SQInteger) , sq_remove >(*this,lib,"sq_remove",SideEffects::worstDefault,"sq_remove")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:247:24
	addExtern< SQInteger (*)(SQVM *) , sq_gettop >(*this,lib,"sq_gettop",SideEffects::worstDefault,"sq_gettop")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:248:19
	addExtern< void (*)(SQVM *,SQInteger) , sq_settop >(*this,lib,"sq_settop",SideEffects::worstDefault,"sq_settop")
		->args({"v","newtop"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:249:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_reservestack >(*this,lib,"sq_reservestack",SideEffects::worstDefault,"sq_reservestack")
		->args({"v","nsize"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:250:24
	addExtern< SQInteger (*)(SQVM *) , sq_cmp >(*this,lib,"sq_cmp",SideEffects::worstDefault,"sq_cmp")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:251:19
	addExtern< void (*)(SQVM *,SQVM *,SQInteger) , sq_move >(*this,lib,"sq_move",SideEffects::worstDefault,"sq_move")
		->args({"dest","src","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:254:28
	addExtern< void * (*)(SQVM *,SQUnsignedInteger) , sq_newuserdata >(*this,lib,"sq_newuserdata",SideEffects::worstDefault,"sq_newuserdata")
		->args({"v","size"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:255:19
	addExtern< void (*)(SQVM *) , sq_newtable >(*this,lib,"sq_newtable",SideEffects::worstDefault,"sq_newtable")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:256:19
	addExtern< void (*)(SQVM *,SQInteger) , sq_newtableex >(*this,lib,"sq_newtableex",SideEffects::worstDefault,"sq_newtableex")
		->args({"v","initialcapacity"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:257:19
	addExtern< void (*)(SQVM *,SQInteger) , sq_newarray >(*this,lib,"sq_newarray",SideEffects::worstDefault,"sq_newarray")
		->args({"v","size"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:259:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,const char *) , sq_setparamscheck >(*this,lib,"sq_setparamscheck",SideEffects::worstDefault,"sq_setparamscheck")
		->args({"v","nparamscheck","typemask"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:260:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_bindenv >(*this,lib,"sq_bindenv",SideEffects::worstDefault,"sq_bindenv")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:263:19
	addExtern< void (*)(SQVM *,const char *,SQInteger) , sq_pushstring >(*this,lib,"sq_pushstring",SideEffects::worstDefault,"sq_pushstring")
		->args({"v","s","len"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:264:19
	addExtern< void (*)(SQVM *,float) , sq_pushfloat >(*this,lib,"sq_pushfloat",SideEffects::worstDefault,"sq_pushfloat")
		->args({"v","f"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:265:19
	addExtern< void (*)(SQVM *,SQInteger) , sq_pushinteger >(*this,lib,"sq_pushinteger",SideEffects::worstDefault,"sq_pushinteger")
		->args({"v","n"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:266:19
	addExtern< void (*)(SQVM *,SQBool) , sq_pushbool >(*this,lib,"sq_pushbool",SideEffects::worstDefault,"sq_pushbool")
		->args({"v","b"});
}
}


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
void Module_dasQUIRREL::initFunctions_9() {
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:48:23
	addExtern< SQRESULT (*)(SQVM *) , sqstd_register_iolib >(*this,lib,"sqstd_register_iolib",SideEffects::worstDefault,"sqstd_register_iolib")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdmath.h:9:23
	addExtern< SQRESULT (*)(SQVM *) , sqstd_register_mathlib >(*this,lib,"sqstd_register_mathlib",SideEffects::worstDefault,"sqstd_register_mathlib")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:18:21
	addExtern< SQRex * (*)(SQAllocContextT *,const char *,const char **) , sqstd_rex_compile >(*this,lib,"sqstd_rex_compile",SideEffects::worstDefault,"sqstd_rex_compile")
		->args({"ctx","pattern","error"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:19:19
	addExtern< void (*)(SQRex *) , sqstd_rex_free >(*this,lib,"sqstd_rex_free",SideEffects::worstDefault,"sqstd_rex_free")
		->args({"exp"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:20:21
	addExtern< SQBool (*)(SQRex *,const char *) , sqstd_rex_match >(*this,lib,"sqstd_rex_match",SideEffects::worstDefault,"sqstd_rex_match")
		->args({"exp","text"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:21:21
	addExtern< SQBool (*)(SQRex *,const char *,const char **,const char **) , sqstd_rex_search >(*this,lib,"sqstd_rex_search",SideEffects::worstDefault,"sqstd_rex_search")
		->args({"exp","text","out_begin","out_end"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:22:21
	addExtern< SQBool (*)(SQRex *,const char *,const char *,const char **,const char **) , sqstd_rex_searchrange >(*this,lib,"sqstd_rex_searchrange",SideEffects::worstDefault,"sqstd_rex_searchrange")
		->args({"exp","text_begin","text_end","out_begin","out_end"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:23:24
	addExtern< SQInteger (*)(SQRex *) , sqstd_rex_getsubexpcount >(*this,lib,"sqstd_rex_getsubexpcount",SideEffects::worstDefault,"sqstd_rex_getsubexpcount")
		->args({"exp"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:24:21
	addExtern< SQBool (*)(SQRex *,SQInteger,SQRexMatch *) , sqstd_rex_getsubexp >(*this,lib,"sqstd_rex_getsubexp",SideEffects::worstDefault,"sqstd_rex_getsubexp")
		->args({"exp","n","subexp"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:26:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQInteger *,char **) , sqstd_format >(*this,lib,"sqstd_format",SideEffects::worstDefault,"sqstd_format")
		->args({"v","nformatstringidx","outlen","output"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:30:23
	addExtern< SQRESULT (*)(SQVM *) , sqstd_register_stringlib >(*this,lib,"sqstd_register_stringlib",SideEffects::worstDefault,"sqstd_register_stringlib")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdsystem.h:9:23
	addExtern< SQRESULT (*)(SQVM *,int,char **) , sqstd_register_command_line_args >(*this,lib,"sqstd_register_command_line_args",SideEffects::worstDefault,"sqstd_register_command_line_args")
		->args({"v","argc","argv"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdsystem.h:10:23
	addExtern< SQRESULT (*)(SQVM *) , sqstd_register_systemlib >(*this,lib,"sqstd_register_systemlib",SideEffects::worstDefault,"sqstd_register_systemlib")
		->args({"v"});
}
}


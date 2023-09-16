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
void Module_dasQUIRREL::initFunctions_8() {
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdaux.h:10:19
	addExtern< void (*)(SQVM *) , sqstd_printcallstack >(*this,lib,"sqstd_printcallstack",SideEffects::worstDefault,"sqstd_printcallstack")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdaux.h:11:23
	addExtern< SQRESULT (*)(SQVM *) , sqstd_formatcallstackstring >(*this,lib,"sqstd_formatcallstackstring",SideEffects::worstDefault,"sqstd_formatcallstackstring")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdblob.h:9:28
	addExtern< void * (*)(SQVM *,SQInteger) , sqstd_createblob >(*this,lib,"sqstd_createblob",SideEffects::worstDefault,"sqstd_createblob")
		->args({"v","size"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdblob.h:10:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,void **) , sqstd_getblob >(*this,lib,"sqstd_getblob",SideEffects::worstDefault,"sqstd_getblob")
		->args({"v","idx","ptr"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdblob.h:11:24
	addExtern< SQInteger (*)(SQVM *,SQInteger) , sqstd_getblobsize >(*this,lib,"sqstd_getblobsize",SideEffects::worstDefault,"sqstd_getblobsize")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdblob.h:13:23
	addExtern< SQRESULT (*)(SQVM *) , sqstd_register_bloblib >(*this,lib,"sqstd_register_bloblib",SideEffects::worstDefault,"sqstd_register_bloblib")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:30:21
	addExtern< void * (*)(const char *,const char *) , sqstd_fopen >(*this,lib,"sqstd_fopen",SideEffects::worstDefault,"sqstd_fopen")
		->args({"",""});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:31:24
	addExtern< SQInteger (*)(void *,SQInteger,SQInteger,void *) , sqstd_fread >(*this,lib,"sqstd_fread",SideEffects::worstDefault,"sqstd_fread")
		->args({"","","",""});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:32:24
	addExtern< SQInteger (*)(void *const,SQInteger,SQInteger,void *) , sqstd_fwrite >(*this,lib,"sqstd_fwrite",SideEffects::worstDefault,"sqstd_fwrite")
		->args({"","","",""});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:33:24
	addExtern< SQInteger (*)(void *,SQInteger,SQInteger) , sqstd_fseek >(*this,lib,"sqstd_fseek",SideEffects::worstDefault,"sqstd_fseek")
		->args({"","",""});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:34:24
	addExtern< SQInteger (*)(void *) , sqstd_ftell >(*this,lib,"sqstd_ftell",SideEffects::worstDefault,"sqstd_ftell")
		->args({""});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:35:24
	addExtern< SQInteger (*)(void *) , sqstd_fflush >(*this,lib,"sqstd_fflush",SideEffects::worstDefault,"sqstd_fflush")
		->args({""});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:36:24
	addExtern< SQInteger (*)(void *) , sqstd_fclose >(*this,lib,"sqstd_fclose",SideEffects::worstDefault,"sqstd_fclose")
		->args({""});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:37:24
	addExtern< SQInteger (*)(void *) , sqstd_feof >(*this,lib,"sqstd_feof",SideEffects::worstDefault,"sqstd_feof")
		->args({""});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:39:23
	addExtern< SQRESULT (*)(SQVM *,void *,SQBool) , sqstd_createfile >(*this,lib,"sqstd_createfile",SideEffects::worstDefault,"sqstd_createfile")
		->args({"v","file","own"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:40:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,void **) , sqstd_getfile >(*this,lib,"sqstd_getfile",SideEffects::worstDefault,"sqstd_getfile")
		->args({"v","idx","file"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:43:23
	addExtern< SQRESULT (*)(SQVM *,const char *,SQBool) , sqstd_loadfile >(*this,lib,"sqstd_loadfile",SideEffects::worstDefault,"sqstd_loadfile")
		->args({"v","filename","printerror"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:44:23
	addExtern< SQRESULT (*)(SQVM *,const char *,SQBool,SQBool) , sqstd_dofile >(*this,lib,"sqstd_dofile",SideEffects::worstDefault,"sqstd_dofile")
		->args({"v","filename","retval","printerror"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:45:23
	addExtern< SQRESULT (*)(SQVM *,const char *) , sqstd_writeclosuretofile >(*this,lib,"sqstd_writeclosuretofile",SideEffects::worstDefault,"sqstd_writeclosuretofile")
		->args({"v","filename"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:47:23
	addExtern< SQRESULT (*)(SQVM *) , sqstd_init_streamclass >(*this,lib,"sqstd_init_streamclass",SideEffects::worstDefault,"sqstd_init_streamclass")
		->args({"v"});
}
}


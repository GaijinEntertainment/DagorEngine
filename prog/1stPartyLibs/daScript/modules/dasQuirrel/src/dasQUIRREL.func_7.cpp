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
void Module_dasQUIRREL::initFunctions_7() {
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:355:21
	addExtern< SQBool (*)(SQVM *,tagSQObject *) , sq_release >(*this,lib,"sq_release",SideEffects::worstDefault,"sq_release")
		->args({"v","po"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:356:32
	addExtern< SQUnsignedInteger (*)(SQVM *,tagSQObject *) , sq_getrefcount >(*this,lib,"sq_getrefcount",SideEffects::worstDefault,"sq_getrefcount")
		->args({"v","po"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:357:19
	addExtern< void (*)(tagSQObject *) , sq_resetobject >(*this,lib,"sq_resetobject",SideEffects::worstDefault,"sq_resetobject")
		->args({"po"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:358:28
	addExtern< const char * (*)(const tagSQObject *) , sq_objtostring >(*this,lib,"sq_objtostring",SideEffects::worstDefault,"sq_objtostring")
		->args({"o"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:359:21
	addExtern< SQBool (*)(const tagSQObject *) , sq_objtobool >(*this,lib,"sq_objtobool",SideEffects::worstDefault,"sq_objtobool")
		->args({"o"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:360:24
	addExtern< SQInteger (*)(const tagSQObject *) , sq_objtointeger >(*this,lib,"sq_objtointeger",SideEffects::worstDefault,"sq_objtointeger")
		->args({"o"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:361:22
	addExtern< float (*)(const tagSQObject *) , sq_objtofloat >(*this,lib,"sq_objtofloat",SideEffects::worstDefault,"sq_objtofloat")
		->args({"o"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:362:28
	addExtern< void * (*)(const tagSQObject *) , sq_objtouserpointer >(*this,lib,"sq_objtouserpointer",SideEffects::worstDefault,"sq_objtouserpointer")
		->args({"o"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:363:23
	addExtern< SQRESULT (*)(const tagSQObject *,void **) , sq_getobjtypetag >(*this,lib,"sq_getobjtypetag",SideEffects::worstDefault,"sq_getobjtypetag")
		->args({"o","typetag"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:364:32
	addExtern< SQUnsignedInteger (*)(SQVM *,const tagSQObject *) , sq_getvmrefcount >(*this,lib,"sq_getvmrefcount",SideEffects::worstDefault,"sq_getvmrefcount")
		->args({"v","po"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:366:21
	addExtern< SQBool (*)(SQVM *,const tagSQObject *,const tagSQObject *,char *,int) , sq_tracevar >(*this,lib,"sq_tracevar",SideEffects::worstDefault,"sq_tracevar")
		->args({"v","container","key","buf","buf_size"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:369:24
	addExtern< SQInteger (*)(SQVM *) , sq_collectgarbage >(*this,lib,"sq_collectgarbage",SideEffects::worstDefault,"sq_collectgarbage")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:370:23
	addExtern< SQRESULT (*)(SQVM *) , sq_resurrectunreachable >(*this,lib,"sq_resurrectunreachable",SideEffects::worstDefault,"sq_resurrectunreachable")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:378:29
	addExtern< SQAllocContextT * (*)(SQVM *) , sq_getallocctx >(*this,lib,"sq_getallocctx",SideEffects::worstDefault,"sq_getallocctx")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:381:20
	addExtern< void * (*)(SQAllocContextT *,SQUnsignedInteger) , sq_malloc >(*this,lib,"sq_malloc",SideEffects::worstDefault,"sq_malloc")
		->args({"ctx","size"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:382:20
	addExtern< void * (*)(SQAllocContextT *,void *,SQUnsignedInteger,SQUnsignedInteger) , sq_realloc >(*this,lib,"sq_realloc",SideEffects::worstDefault,"sq_realloc")
		->args({"ctx","p","oldsize","newsize"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:383:19
	addExtern< void (*)(SQAllocContextT *,void *,SQUnsignedInteger) , sq_free >(*this,lib,"sq_free",SideEffects::worstDefault,"sq_free")
		->args({"ctx","p","size"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:386:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,tagSQStackInfos *) , sq_stackinfos >(*this,lib,"sq_stackinfos",SideEffects::worstDefault,"sq_stackinfos")
		->args({"v","level","si"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:387:19
	addExtern< void (*)(SQVM *) , sq_setdebughook >(*this,lib,"sq_setdebughook",SideEffects::worstDefault,"sq_setdebughook")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdaux.h:9:19
	addExtern< void (*)(SQVM *) , sqstd_seterrorhandlers >(*this,lib,"sqstd_seterrorhandlers",SideEffects::worstDefault,"sqstd_seterrorhandlers")
		->args({"v"});
}
}


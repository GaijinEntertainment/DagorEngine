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
void Module_dasQUIRREL::initFunctions_3() {
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:267:19
	addExtern< void (*)(SQVM *,void *) , sq_pushuserpointer >(*this,lib,"sq_pushuserpointer",SideEffects::worstDefault,"sq_pushuserpointer")
		->args({"v","p"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:268:19
	addExtern< void (*)(SQVM *) , sq_pushnull >(*this,lib,"sq_pushnull",SideEffects::worstDefault,"sq_pushnull")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:269:19
	addExtern< void (*)(SQVM *,SQVM *) , sq_pushthread >(*this,lib,"sq_pushthread",SideEffects::worstDefault,"sq_pushthread")
		->args({"v","thread"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:270:27
	addExtern< tagSQObjectType (*)(SQVM *,SQInteger) , sq_gettype >(*this,lib,"sq_gettype",SideEffects::worstDefault,"sq_gettype")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:271:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_typeof >(*this,lib,"sq_typeof",SideEffects::worstDefault,"sq_typeof")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:272:24
	addExtern< SQInteger (*)(SQVM *,SQInteger) , sq_getsize >(*this,lib,"sq_getsize",SideEffects::worstDefault,"sq_getsize")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:273:21
	addExtern< SQHash (*)(SQVM *,SQInteger) , sq_gethash >(*this,lib,"sq_gethash",SideEffects::worstDefault,"sq_gethash")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:274:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_getbase >(*this,lib,"sq_getbase",SideEffects::worstDefault,"sq_getbase")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:275:21
	addExtern< SQBool (*)(SQVM *) , sq_instanceof >(*this,lib,"sq_instanceof",SideEffects::worstDefault,"sq_instanceof")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:276:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_tostring >(*this,lib,"sq_tostring",SideEffects::worstDefault,"sq_tostring")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:277:19
	addExtern< void (*)(SQVM *,SQInteger,SQBool *) , sq_tobool >(*this,lib,"sq_tobool",SideEffects::worstDefault,"sq_tobool")
		->args({"v","idx","b"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:278:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,const char **,SQInteger *) , sq_getstringandsize >(*this,lib,"sq_getstringandsize",SideEffects::worstDefault,"sq_getstringandsize")
		->args({"v","idx","c","size"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:279:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,const char **) , sq_getstring >(*this,lib,"sq_getstring",SideEffects::worstDefault,"sq_getstring")
		->args({"v","idx","c"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:280:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQInteger *) , sq_getinteger >(*this,lib,"sq_getinteger",SideEffects::worstDefault,"sq_getinteger")
		->args({"v","idx","i"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:281:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,float *) , sq_getfloat >(*this,lib,"sq_getfloat",SideEffects::worstDefault,"sq_getfloat")
		->args({"v","idx","f"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:282:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger, SQBool *) , sq_getbool >(*this,lib,"sq_getbool",SideEffects::worstDefault,"sq_getbool")
		->args({"v","idx","b"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:283:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQVM **) , sq_getthread >(*this,lib,"sq_getthread",SideEffects::worstDefault,"sq_getthread")
		->args({"v","idx","thread"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:284:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,void **) , sq_getuserpointer >(*this,lib,"sq_getuserpointer",SideEffects::worstDefault,"sq_getuserpointer")
		->args({"v","idx","p"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:285:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,void **,void **) , sq_getuserdata >(*this,lib,"sq_getuserdata",SideEffects::worstDefault,"sq_getuserdata")
		->args({"v","idx","p","typetag"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:286:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,void *) , sq_settypetag >(*this,lib,"sq_settypetag",SideEffects::worstDefault,"sq_settypetag")
		->args({"v","idx","typetag"});
}
}


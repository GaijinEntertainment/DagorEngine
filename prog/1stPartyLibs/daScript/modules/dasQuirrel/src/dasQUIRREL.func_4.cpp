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
void Module_dasQUIRREL::initFunctions_4() {
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:287:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,void **) , sq_gettypetag >(*this,lib,"sq_gettypetag",SideEffects::worstDefault,"sq_gettypetag")
		->args({"v","idx","typetag"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:290:22
	addExtern< char * (*)(SQVM *,SQInteger) , sq_getscratchpad >(*this,lib,"sq_getscratchpad",SideEffects::worstDefault,"sq_getscratchpad")
		->args({"v","minsize"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:291:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,tagSQFunctionInfo *) , sq_getfunctioninfo >(*this,lib,"sq_getfunctioninfo",SideEffects::worstDefault,"sq_getfunctioninfo")
		->args({"v","level","fi"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:292:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQInteger *,SQInteger *) , sq_getclosureinfo >(*this,lib,"sq_getclosureinfo",SideEffects::worstDefault,"sq_getclosureinfo")
		->args({"v","idx","nparams","nfreevars"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:293:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_getclosurename >(*this,lib,"sq_getclosurename",SideEffects::worstDefault,"sq_getclosurename")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:294:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,const char *) , sq_setnativeclosurename >(*this,lib,"sq_setnativeclosurename",SideEffects::worstDefault,"sq_setnativeclosurename")
		->args({"v","idx","name"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:295:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,void *) , sq_setinstanceup >(*this,lib,"sq_setinstanceup",SideEffects::worstDefault,"sq_setinstanceup")
		->args({"v","idx","p"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:296:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,void **,void *) , sq_getinstanceup >(*this,lib,"sq_getinstanceup",SideEffects::worstDefault,"sq_getinstanceup")
		->args({"v","idx","p","typetag"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:297:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,SQInteger) , sq_setclassudsize >(*this,lib,"sq_setclassudsize",SideEffects::worstDefault,"sq_setclassudsize")
		->args({"v","idx","udsize"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:298:23
	addExtern< SQRESULT (*)(SQVM *,SQBool) , sq_newclass >(*this,lib,"sq_newclass",SideEffects::worstDefault,"sq_newclass")
		->args({"v","hasbase"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:299:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_createinstance >(*this,lib,"sq_createinstance",SideEffects::worstDefault,"sq_createinstance")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:300:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger) , sq_getclass >(*this,lib,"sq_getclass",SideEffects::worstDefault,"sq_getclass")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:301:19
	addExtern< void (*)(SQVM *,SQInteger) , sq_weakref >(*this,lib,"sq_weakref",SideEffects::worstDefault,"sq_weakref")
		->args({"v","idx"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:302:23
	addExtern< SQRESULT (*)(SQVM *,tagSQObjectType) , sq_getdefaultdelegate >(*this,lib,"sq_getdefaultdelegate",SideEffects::worstDefault,"sq_getdefaultdelegate")
		->args({"v","t"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:303:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,tagSQMemberHandle *) , sq_getmemberhandle >(*this,lib,"sq_getmemberhandle",SideEffects::worstDefault,"sq_getmemberhandle")
		->args({"v","idx","handle"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:304:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,const tagSQMemberHandle *) , sq_getbyhandle >(*this,lib,"sq_getbyhandle",SideEffects::worstDefault,"sq_getbyhandle")
		->args({"v","idx","handle"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:305:23
	addExtern< SQRESULT (*)(SQVM *,SQInteger,const tagSQMemberHandle *) , sq_setbyhandle >(*this,lib,"sq_setbyhandle",SideEffects::worstDefault,"sq_setbyhandle")
		->args({"v","idx","handle"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:308:19
	addExtern< void (*)(SQVM *) , sq_pushroottable >(*this,lib,"sq_pushroottable",SideEffects::worstDefault,"sq_pushroottable")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:309:19
	addExtern< void (*)(SQVM *) , sq_pushregistrytable >(*this,lib,"sq_pushregistrytable",SideEffects::worstDefault,"sq_pushregistrytable")
		->args({"v"});
// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:310:19
	addExtern< void (*)(SQVM *) , sq_pushconsttable >(*this,lib,"sq_pushconsttable",SideEffects::worstDefault,"sq_pushconsttable")
		->args({"v"});
}
}


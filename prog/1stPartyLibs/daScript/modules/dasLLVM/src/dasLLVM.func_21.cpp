// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasLLVM.h"
#include "need_dasLLVM.h"
namespace das {
#include "dasLLVM.func.aot.decl.inc"
void Module_dasLLVM::initFunctions_21() {
// from D:\Work\libclang\include\llvm-c/Core.h:2388:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetInitializer >(*this,lib,"LLVMGetInitializer",SideEffects::worstDefault,"LLVMGetInitializer")
		->args({"GlobalVar"});
// from D:\Work\libclang\include\llvm-c/Core.h:2389:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetInitializer >(*this,lib,"LLVMSetInitializer",SideEffects::worstDefault,"LLVMSetInitializer")
		->args({"GlobalVar","ConstantVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:2390:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsThreadLocal >(*this,lib,"LLVMIsThreadLocal",SideEffects::worstDefault,"LLVMIsThreadLocal")
		->args({"GlobalVar"});
// from D:\Work\libclang\include\llvm-c/Core.h:2391:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetThreadLocal >(*this,lib,"LLVMSetThreadLocal",SideEffects::worstDefault,"LLVMSetThreadLocal")
		->args({"GlobalVar","IsThreadLocal"});
// from D:\Work\libclang\include\llvm-c/Core.h:2392:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsGlobalConstant >(*this,lib,"LLVMIsGlobalConstant",SideEffects::worstDefault,"LLVMIsGlobalConstant")
		->args({"GlobalVar"});
// from D:\Work\libclang\include\llvm-c/Core.h:2393:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetGlobalConstant >(*this,lib,"LLVMSetGlobalConstant",SideEffects::worstDefault,"LLVMSetGlobalConstant")
		->args({"GlobalVar","IsConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2394:21
	addExtern< LLVMThreadLocalMode (*)(LLVMOpaqueValue *) , LLVMGetThreadLocalMode >(*this,lib,"LLVMGetThreadLocalMode",SideEffects::worstDefault,"LLVMGetThreadLocalMode")
		->args({"GlobalVar"});
// from D:\Work\libclang\include\llvm-c/Core.h:2395:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMThreadLocalMode) , LLVMSetThreadLocalMode >(*this,lib,"LLVMSetThreadLocalMode",SideEffects::worstDefault,"LLVMSetThreadLocalMode")
		->args({"GlobalVar","Mode"});
// from D:\Work\libclang\include\llvm-c/Core.h:2396:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsExternallyInitialized >(*this,lib,"LLVMIsExternallyInitialized",SideEffects::worstDefault,"LLVMIsExternallyInitialized")
		->args({"GlobalVar"});
// from D:\Work\libclang\include\llvm-c/Core.h:2397:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetExternallyInitialized >(*this,lib,"LLVMSetExternallyInitialized",SideEffects::worstDefault,"LLVMSetExternallyInitialized")
		->args({"GlobalVar","IsExtInit"});
// from D:\Work\libclang\include\llvm-c/Core.h:2414:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,LLVMOpaqueType *,LLVMOpaqueValue *,const char *) , LLVMAddAlias >(*this,lib,"LLVMAddAlias",SideEffects::worstDefault,"LLVMAddAlias")
		->args({"M","Ty","Aliasee","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:2423:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,LLVMOpaqueType *,unsigned int,LLVMOpaqueValue *,const char *) , LLVMAddAlias2 >(*this,lib,"LLVMAddAlias2",SideEffects::worstDefault,"LLVMAddAlias2")
		->args({"M","ValueTy","AddrSpace","Aliasee","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:2434:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetNamedGlobalAlias >(*this,lib,"LLVMGetNamedGlobalAlias",SideEffects::worstDefault,"LLVMGetNamedGlobalAlias")
		->args({"M","Name","NameLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:2442:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetFirstGlobalAlias >(*this,lib,"LLVMGetFirstGlobalAlias",SideEffects::worstDefault,"LLVMGetFirstGlobalAlias")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:2449:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetLastGlobalAlias >(*this,lib,"LLVMGetLastGlobalAlias",SideEffects::worstDefault,"LLVMGetLastGlobalAlias")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:2457:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextGlobalAlias >(*this,lib,"LLVMGetNextGlobalAlias",SideEffects::worstDefault,"LLVMGetNextGlobalAlias")
		->args({"GA"});
// from D:\Work\libclang\include\llvm-c/Core.h:2465:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousGlobalAlias >(*this,lib,"LLVMGetPreviousGlobalAlias",SideEffects::worstDefault,"LLVMGetPreviousGlobalAlias")
		->args({"GA"});
// from D:\Work\libclang\include\llvm-c/Core.h:2470:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMAliasGetAliasee >(*this,lib,"LLVMAliasGetAliasee",SideEffects::worstDefault,"LLVMAliasGetAliasee")
		->args({"Alias"});
// from D:\Work\libclang\include\llvm-c/Core.h:2475:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMAliasSetAliasee >(*this,lib,"LLVMAliasSetAliasee",SideEffects::worstDefault,"LLVMAliasSetAliasee")
		->args({"Alias","Aliasee"});
// from D:\Work\libclang\include\llvm-c/Core.h:2497:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMDeleteFunction >(*this,lib,"LLVMDeleteFunction",SideEffects::worstDefault,"LLVMDeleteFunction")
		->args({"Fn"});
}
}


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
// from D:\Work\libclang\include\llvm-c/Core.h:2399:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetInitializer , SimNode_ExtFuncCall >(lib,"LLVMSetInitializer","LLVMSetInitializer")
		->args({"GlobalVar","ConstantVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2400:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsThreadLocal , SimNode_ExtFuncCall >(lib,"LLVMIsThreadLocal","LLVMIsThreadLocal")
		->args({"GlobalVar"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2401:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetThreadLocal , SimNode_ExtFuncCall >(lib,"LLVMSetThreadLocal","LLVMSetThreadLocal")
		->args({"GlobalVar","IsThreadLocal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2402:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsGlobalConstant , SimNode_ExtFuncCall >(lib,"LLVMIsGlobalConstant","LLVMIsGlobalConstant")
		->args({"GlobalVar"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2403:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetGlobalConstant , SimNode_ExtFuncCall >(lib,"LLVMSetGlobalConstant","LLVMSetGlobalConstant")
		->args({"GlobalVar","IsConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2404:21
	makeExtern< LLVMThreadLocalMode (*)(LLVMOpaqueValue *) , LLVMGetThreadLocalMode , SimNode_ExtFuncCall >(lib,"LLVMGetThreadLocalMode","LLVMGetThreadLocalMode")
		->args({"GlobalVar"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2405:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMThreadLocalMode) , LLVMSetThreadLocalMode , SimNode_ExtFuncCall >(lib,"LLVMSetThreadLocalMode","LLVMSetThreadLocalMode")
		->args({"GlobalVar","Mode"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2406:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsExternallyInitialized , SimNode_ExtFuncCall >(lib,"LLVMIsExternallyInitialized","LLVMIsExternallyInitialized")
		->args({"GlobalVar"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2407:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetExternallyInitialized , SimNode_ExtFuncCall >(lib,"LLVMSetExternallyInitialized","LLVMSetExternallyInitialized")
		->args({"GlobalVar","IsExtInit"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2428:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,LLVMOpaqueType *,unsigned int,LLVMOpaqueValue *,const char *) , LLVMAddAlias2 , SimNode_ExtFuncCall >(lib,"LLVMAddAlias2","LLVMAddAlias2")
		->args({"M","ValueTy","AddrSpace","Aliasee","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2439:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetNamedGlobalAlias , SimNode_ExtFuncCall >(lib,"LLVMGetNamedGlobalAlias","LLVMGetNamedGlobalAlias")
		->args({"M","Name","NameLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2447:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetFirstGlobalAlias , SimNode_ExtFuncCall >(lib,"LLVMGetFirstGlobalAlias","LLVMGetFirstGlobalAlias")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2454:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetLastGlobalAlias , SimNode_ExtFuncCall >(lib,"LLVMGetLastGlobalAlias","LLVMGetLastGlobalAlias")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2462:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextGlobalAlias , SimNode_ExtFuncCall >(lib,"LLVMGetNextGlobalAlias","LLVMGetNextGlobalAlias")
		->args({"GA"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2470:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousGlobalAlias , SimNode_ExtFuncCall >(lib,"LLVMGetPreviousGlobalAlias","LLVMGetPreviousGlobalAlias")
		->args({"GA"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2475:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMAliasGetAliasee , SimNode_ExtFuncCall >(lib,"LLVMAliasGetAliasee","LLVMAliasGetAliasee")
		->args({"Alias"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2480:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMAliasSetAliasee , SimNode_ExtFuncCall >(lib,"LLVMAliasSetAliasee","LLVMAliasSetAliasee")
		->args({"Alias","Aliasee"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2502:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMDeleteFunction , SimNode_ExtFuncCall >(lib,"LLVMDeleteFunction","LLVMDeleteFunction")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2509:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMHasPersonalityFn , SimNode_ExtFuncCall >(lib,"LLVMHasPersonalityFn","LLVMHasPersonalityFn")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2516:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPersonalityFn , SimNode_ExtFuncCall >(lib,"LLVMGetPersonalityFn","LLVMGetPersonalityFn")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


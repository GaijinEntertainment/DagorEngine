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
void Module_dasLLVM::initFunctions_23() {
// from D:\Work\libclang\include\llvm-c/Core.h:2640:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMRemoveEnumAttributeAtIndex >(*this,lib,"LLVMRemoveEnumAttributeAtIndex",SideEffects::worstDefault,"LLVMRemoveEnumAttributeAtIndex")
		->args({"F","Idx","KindID"});
// from D:\Work\libclang\include\llvm-c/Core.h:2642:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,const char *,unsigned int) , LLVMRemoveStringAttributeAtIndex >(*this,lib,"LLVMRemoveStringAttributeAtIndex",SideEffects::worstDefault,"LLVMRemoveStringAttributeAtIndex")
		->args({"F","Idx","K","KLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:2649:6
	addExtern< void (*)(LLVMOpaqueValue *,const char *,const char *) , LLVMAddTargetDependentFunctionAttr >(*this,lib,"LLVMAddTargetDependentFunctionAttr",SideEffects::worstDefault,"LLVMAddTargetDependentFunctionAttr")
		->args({"Fn","A","V"});
// from D:\Work\libclang\include\llvm-c/Core.h:2668:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMCountParams >(*this,lib,"LLVMCountParams",SideEffects::worstDefault,"LLVMCountParams")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:2681:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue **) , LLVMGetParams >(*this,lib,"LLVMGetParams",SideEffects::worstDefault,"LLVMGetParams")
		->args({"Fn","Params"});
// from D:\Work\libclang\include\llvm-c/Core.h:2690:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetParam >(*this,lib,"LLVMGetParam",SideEffects::worstDefault,"LLVMGetParam")
		->args({"Fn","Index"});
// from D:\Work\libclang\include\llvm-c/Core.h:2701:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetParamParent >(*this,lib,"LLVMGetParamParent",SideEffects::worstDefault,"LLVMGetParamParent")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:2708:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetFirstParam >(*this,lib,"LLVMGetFirstParam",SideEffects::worstDefault,"LLVMGetFirstParam")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:2715:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetLastParam >(*this,lib,"LLVMGetLastParam",SideEffects::worstDefault,"LLVMGetLastParam")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:2724:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextParam >(*this,lib,"LLVMGetNextParam",SideEffects::worstDefault,"LLVMGetNextParam")
		->args({"Arg"});
// from D:\Work\libclang\include\llvm-c/Core.h:2731:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousParam >(*this,lib,"LLVMGetPreviousParam",SideEffects::worstDefault,"LLVMGetPreviousParam")
		->args({"Arg"});
// from D:\Work\libclang\include\llvm-c/Core.h:2739:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMSetParamAlignment >(*this,lib,"LLVMSetParamAlignment",SideEffects::worstDefault,"LLVMSetParamAlignment")
		->args({"Arg","Align"});
// from D:\Work\libclang\include\llvm-c/Core.h:2761:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *,size_t,LLVMOpaqueType *,unsigned int,LLVMOpaqueValue *) , LLVMAddGlobalIFunc >(*this,lib,"LLVMAddGlobalIFunc",SideEffects::worstDefault,"LLVMAddGlobalIFunc")
		->args({"M","Name","NameLen","Ty","AddrSpace","Resolver"});
// from D:\Work\libclang\include\llvm-c/Core.h:2773:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetNamedGlobalIFunc >(*this,lib,"LLVMGetNamedGlobalIFunc",SideEffects::worstDefault,"LLVMGetNamedGlobalIFunc")
		->args({"M","Name","NameLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:2781:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetFirstGlobalIFunc >(*this,lib,"LLVMGetFirstGlobalIFunc",SideEffects::worstDefault,"LLVMGetFirstGlobalIFunc")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:2788:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetLastGlobalIFunc >(*this,lib,"LLVMGetLastGlobalIFunc",SideEffects::worstDefault,"LLVMGetLastGlobalIFunc")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:2796:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextGlobalIFunc >(*this,lib,"LLVMGetNextGlobalIFunc",SideEffects::worstDefault,"LLVMGetNextGlobalIFunc")
		->args({"IFunc"});
// from D:\Work\libclang\include\llvm-c/Core.h:2804:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousGlobalIFunc >(*this,lib,"LLVMGetPreviousGlobalIFunc",SideEffects::worstDefault,"LLVMGetPreviousGlobalIFunc")
		->args({"IFunc"});
// from D:\Work\libclang\include\llvm-c/Core.h:2812:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetGlobalIFuncResolver >(*this,lib,"LLVMGetGlobalIFuncResolver",SideEffects::worstDefault,"LLVMGetGlobalIFuncResolver")
		->args({"IFunc"});
// from D:\Work\libclang\include\llvm-c/Core.h:2819:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetGlobalIFuncResolver >(*this,lib,"LLVMSetGlobalIFuncResolver",SideEffects::worstDefault,"LLVMSetGlobalIFuncResolver")
		->args({"IFunc","Resolver"});
}
}


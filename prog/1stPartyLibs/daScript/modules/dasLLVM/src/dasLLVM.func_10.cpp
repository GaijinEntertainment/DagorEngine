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
void Module_dasLLVM::initFunctions_10() {
// from D:\Work\libclang\include\llvm-c/Core.h:1705:15
	addExtern< LLVMValueKind (*)(LLVMOpaqueValue *) , LLVMGetValueKind >(*this,lib,"LLVMGetValueKind",SideEffects::worstDefault,"LLVMGetValueKind")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1712:13
	addExtern< const char * (*)(LLVMOpaqueValue *,size_t *) , LLVMGetValueName2 >(*this,lib,"LLVMGetValueName2",SideEffects::worstDefault,"LLVMGetValueName2")
		->args({"Val","Length"});
// from D:\Work\libclang\include\llvm-c/Core.h:1719:6
	addExtern< void (*)(LLVMOpaqueValue *,const char *,size_t) , LLVMSetValueName2 >(*this,lib,"LLVMSetValueName2",SideEffects::worstDefault,"LLVMSetValueName2")
		->args({"Val","Name","NameLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:1726:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMDumpValue >(*this,lib,"LLVMDumpValue",SideEffects::worstDefault,"LLVMDumpValue")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1734:7
	addExtern< char * (*)(LLVMOpaqueValue *) , LLVMPrintValueToString >(*this,lib,"LLVMPrintValueToString",SideEffects::worstDefault,"LLVMPrintValueToString")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1741:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMReplaceAllUsesWith >(*this,lib,"LLVMReplaceAllUsesWith",SideEffects::worstDefault,"LLVMReplaceAllUsesWith")
		->args({"OldVal","NewVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:1746:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsConstant >(*this,lib,"LLVMIsConstant",SideEffects::worstDefault,"LLVMIsConstant")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1751:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsUndef >(*this,lib,"LLVMIsUndef",SideEffects::worstDefault,"LLVMIsUndef")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1756:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsPoison >(*this,lib,"LLVMIsPoison",SideEffects::worstDefault,"LLVMIsPoison")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAArgument >(*this,lib,"LLVMIsAArgument",SideEffects::worstDefault,"LLVMIsAArgument")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABasicBlock >(*this,lib,"LLVMIsABasicBlock",SideEffects::worstDefault,"LLVMIsABasicBlock")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInlineAsm >(*this,lib,"LLVMIsAInlineAsm",SideEffects::worstDefault,"LLVMIsAInlineAsm")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUser >(*this,lib,"LLVMIsAUser",SideEffects::worstDefault,"LLVMIsAUser")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstant >(*this,lib,"LLVMIsAConstant",SideEffects::worstDefault,"LLVMIsAConstant")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABlockAddress >(*this,lib,"LLVMIsABlockAddress",SideEffects::worstDefault,"LLVMIsABlockAddress")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantAggregateZero >(*this,lib,"LLVMIsAConstantAggregateZero",SideEffects::worstDefault,"LLVMIsAConstantAggregateZero")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantArray >(*this,lib,"LLVMIsAConstantArray",SideEffects::worstDefault,"LLVMIsAConstantArray")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantDataSequential >(*this,lib,"LLVMIsAConstantDataSequential",SideEffects::worstDefault,"LLVMIsAConstantDataSequential")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantDataArray >(*this,lib,"LLVMIsAConstantDataArray",SideEffects::worstDefault,"LLVMIsAConstantDataArray")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantDataVector >(*this,lib,"LLVMIsAConstantDataVector",SideEffects::worstDefault,"LLVMIsAConstantDataVector")
		->args({"Val"});
}
}


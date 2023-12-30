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
void Module_dasLLVM::initFunctions_13() {
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABranchInst >(*this,lib,"LLVMIsABranchInst",SideEffects::worstDefault,"LLVMIsABranchInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAIndirectBrInst >(*this,lib,"LLVMIsAIndirectBrInst",SideEffects::worstDefault,"LLVMIsAIndirectBrInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInvokeInst >(*this,lib,"LLVMIsAInvokeInst",SideEffects::worstDefault,"LLVMIsAInvokeInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAReturnInst >(*this,lib,"LLVMIsAReturnInst",SideEffects::worstDefault,"LLVMIsAReturnInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsASwitchInst >(*this,lib,"LLVMIsASwitchInst",SideEffects::worstDefault,"LLVMIsASwitchInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUnreachableInst >(*this,lib,"LLVMIsAUnreachableInst",SideEffects::worstDefault,"LLVMIsAUnreachableInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAResumeInst >(*this,lib,"LLVMIsAResumeInst",SideEffects::worstDefault,"LLVMIsAResumeInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACleanupReturnInst >(*this,lib,"LLVMIsACleanupReturnInst",SideEffects::worstDefault,"LLVMIsACleanupReturnInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACatchReturnInst >(*this,lib,"LLVMIsACatchReturnInst",SideEffects::worstDefault,"LLVMIsACatchReturnInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACatchSwitchInst >(*this,lib,"LLVMIsACatchSwitchInst",SideEffects::worstDefault,"LLVMIsACatchSwitchInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACallBrInst >(*this,lib,"LLVMIsACallBrInst",SideEffects::worstDefault,"LLVMIsACallBrInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFuncletPadInst >(*this,lib,"LLVMIsAFuncletPadInst",SideEffects::worstDefault,"LLVMIsAFuncletPadInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACatchPadInst >(*this,lib,"LLVMIsACatchPadInst",SideEffects::worstDefault,"LLVMIsACatchPadInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACleanupPadInst >(*this,lib,"LLVMIsACleanupPadInst",SideEffects::worstDefault,"LLVMIsACleanupPadInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUnaryInstruction >(*this,lib,"LLVMIsAUnaryInstruction",SideEffects::worstDefault,"LLVMIsAUnaryInstruction")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAAllocaInst >(*this,lib,"LLVMIsAAllocaInst",SideEffects::worstDefault,"LLVMIsAAllocaInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACastInst >(*this,lib,"LLVMIsACastInst",SideEffects::worstDefault,"LLVMIsACastInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAAddrSpaceCastInst >(*this,lib,"LLVMIsAAddrSpaceCastInst",SideEffects::worstDefault,"LLVMIsAAddrSpaceCastInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABitCastInst >(*this,lib,"LLVMIsABitCastInst",SideEffects::worstDefault,"LLVMIsABitCastInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFPExtInst >(*this,lib,"LLVMIsAFPExtInst",SideEffects::worstDefault,"LLVMIsAFPExtInst")
		->args({"Val"});
}
}


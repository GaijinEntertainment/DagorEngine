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
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAShuffleVectorInst , SimNode_ExtFuncCall >(lib,"LLVMIsAShuffleVectorInst","LLVMIsAShuffleVectorInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAStoreInst , SimNode_ExtFuncCall >(lib,"LLVMIsAStoreInst","LLVMIsAStoreInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABranchInst , SimNode_ExtFuncCall >(lib,"LLVMIsABranchInst","LLVMIsABranchInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAIndirectBrInst , SimNode_ExtFuncCall >(lib,"LLVMIsAIndirectBrInst","LLVMIsAIndirectBrInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInvokeInst , SimNode_ExtFuncCall >(lib,"LLVMIsAInvokeInst","LLVMIsAInvokeInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAReturnInst , SimNode_ExtFuncCall >(lib,"LLVMIsAReturnInst","LLVMIsAReturnInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsASwitchInst , SimNode_ExtFuncCall >(lib,"LLVMIsASwitchInst","LLVMIsASwitchInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUnreachableInst , SimNode_ExtFuncCall >(lib,"LLVMIsAUnreachableInst","LLVMIsAUnreachableInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAResumeInst , SimNode_ExtFuncCall >(lib,"LLVMIsAResumeInst","LLVMIsAResumeInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACleanupReturnInst , SimNode_ExtFuncCall >(lib,"LLVMIsACleanupReturnInst","LLVMIsACleanupReturnInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACatchReturnInst , SimNode_ExtFuncCall >(lib,"LLVMIsACatchReturnInst","LLVMIsACatchReturnInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACatchSwitchInst , SimNode_ExtFuncCall >(lib,"LLVMIsACatchSwitchInst","LLVMIsACatchSwitchInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACallBrInst , SimNode_ExtFuncCall >(lib,"LLVMIsACallBrInst","LLVMIsACallBrInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFuncletPadInst , SimNode_ExtFuncCall >(lib,"LLVMIsAFuncletPadInst","LLVMIsAFuncletPadInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACatchPadInst , SimNode_ExtFuncCall >(lib,"LLVMIsACatchPadInst","LLVMIsACatchPadInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACleanupPadInst , SimNode_ExtFuncCall >(lib,"LLVMIsACleanupPadInst","LLVMIsACleanupPadInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUnaryInstruction , SimNode_ExtFuncCall >(lib,"LLVMIsAUnaryInstruction","LLVMIsAUnaryInstruction")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAAllocaInst , SimNode_ExtFuncCall >(lib,"LLVMIsAAllocaInst","LLVMIsAAllocaInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACastInst , SimNode_ExtFuncCall >(lib,"LLVMIsACastInst","LLVMIsACastInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAAddrSpaceCastInst , SimNode_ExtFuncCall >(lib,"LLVMIsAAddrSpaceCastInst","LLVMIsAAddrSpaceCastInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


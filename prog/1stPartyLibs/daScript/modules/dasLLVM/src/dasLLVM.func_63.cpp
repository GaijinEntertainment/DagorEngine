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
void Module_dasLLVM::initFunctions_63() {
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:47:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddCFGSimplificationPass , SimNode_ExtFuncCall >(lib,"LLVMAddCFGSimplificationPass","LLVMAddCFGSimplificationPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:50:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddDeadStoreEliminationPass , SimNode_ExtFuncCall >(lib,"LLVMAddDeadStoreEliminationPass","LLVMAddDeadStoreEliminationPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:53:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddScalarizerPass , SimNode_ExtFuncCall >(lib,"LLVMAddScalarizerPass","LLVMAddScalarizerPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:56:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddMergedLoadStoreMotionPass , SimNode_ExtFuncCall >(lib,"LLVMAddMergedLoadStoreMotionPass","LLVMAddMergedLoadStoreMotionPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:59:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddGVNPass , SimNode_ExtFuncCall >(lib,"LLVMAddGVNPass","LLVMAddGVNPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:62:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddNewGVNPass , SimNode_ExtFuncCall >(lib,"LLVMAddNewGVNPass","LLVMAddNewGVNPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:65:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddIndVarSimplifyPass , SimNode_ExtFuncCall >(lib,"LLVMAddIndVarSimplifyPass","LLVMAddIndVarSimplifyPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:68:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddInstructionCombiningPass , SimNode_ExtFuncCall >(lib,"LLVMAddInstructionCombiningPass","LLVMAddInstructionCombiningPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:71:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddInstructionSimplifyPass , SimNode_ExtFuncCall >(lib,"LLVMAddInstructionSimplifyPass","LLVMAddInstructionSimplifyPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:74:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddJumpThreadingPass , SimNode_ExtFuncCall >(lib,"LLVMAddJumpThreadingPass","LLVMAddJumpThreadingPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:77:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLICMPass , SimNode_ExtFuncCall >(lib,"LLVMAddLICMPass","LLVMAddLICMPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:80:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopDeletionPass , SimNode_ExtFuncCall >(lib,"LLVMAddLoopDeletionPass","LLVMAddLoopDeletionPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:83:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopIdiomPass , SimNode_ExtFuncCall >(lib,"LLVMAddLoopIdiomPass","LLVMAddLoopIdiomPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:86:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopRotatePass , SimNode_ExtFuncCall >(lib,"LLVMAddLoopRotatePass","LLVMAddLoopRotatePass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:89:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopRerollPass , SimNode_ExtFuncCall >(lib,"LLVMAddLoopRerollPass","LLVMAddLoopRerollPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:92:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopUnrollPass , SimNode_ExtFuncCall >(lib,"LLVMAddLoopUnrollPass","LLVMAddLoopUnrollPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:95:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopUnrollAndJamPass , SimNode_ExtFuncCall >(lib,"LLVMAddLoopUnrollAndJamPass","LLVMAddLoopUnrollAndJamPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:98:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLowerAtomicPass , SimNode_ExtFuncCall >(lib,"LLVMAddLowerAtomicPass","LLVMAddLowerAtomicPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:101:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddMemCpyOptPass , SimNode_ExtFuncCall >(lib,"LLVMAddMemCpyOptPass","LLVMAddMemCpyOptPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:104:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddPartiallyInlineLibCallsPass , SimNode_ExtFuncCall >(lib,"LLVMAddPartiallyInlineLibCallsPass","LLVMAddPartiallyInlineLibCallsPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


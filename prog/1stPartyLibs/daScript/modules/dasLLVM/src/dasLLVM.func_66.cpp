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
void Module_dasLLVM::initFunctions_66() {
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:62:1
	addExtern< void (*)(LLVMOpaquePassManagerBuilder *,unsigned int) , LLVMPassManagerBuilderUseInlinerWithThreshold >(*this,lib,"LLVMPassManagerBuilderUseInlinerWithThreshold",SideEffects::worstDefault,"LLVMPassManagerBuilderUseInlinerWithThreshold")
		->args({"PMB","Threshold"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:67:1
	addExtern< void (*)(LLVMOpaquePassManagerBuilder *,LLVMOpaquePassManager *) , LLVMPassManagerBuilderPopulateFunctionPassManager >(*this,lib,"LLVMPassManagerBuilderPopulateFunctionPassManager",SideEffects::worstDefault,"LLVMPassManagerBuilderPopulateFunctionPassManager")
		->args({"PMB","PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:72:1
	addExtern< void (*)(LLVMOpaquePassManagerBuilder *,LLVMOpaquePassManager *) , LLVMPassManagerBuilderPopulateModulePassManager >(*this,lib,"LLVMPassManagerBuilderPopulateModulePassManager",SideEffects::worstDefault,"LLVMPassManagerBuilderPopulateModulePassManager")
		->args({"PMB","PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:35:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddAggressiveDCEPass >(*this,lib,"LLVMAddAggressiveDCEPass",SideEffects::worstDefault,"LLVMAddAggressiveDCEPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:38:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddDCEPass >(*this,lib,"LLVMAddDCEPass",SideEffects::worstDefault,"LLVMAddDCEPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:41:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddBitTrackingDCEPass >(*this,lib,"LLVMAddBitTrackingDCEPass",SideEffects::worstDefault,"LLVMAddBitTrackingDCEPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:44:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddAlignmentFromAssumptionsPass >(*this,lib,"LLVMAddAlignmentFromAssumptionsPass",SideEffects::worstDefault,"LLVMAddAlignmentFromAssumptionsPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:47:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddCFGSimplificationPass >(*this,lib,"LLVMAddCFGSimplificationPass",SideEffects::worstDefault,"LLVMAddCFGSimplificationPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:50:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddDeadStoreEliminationPass >(*this,lib,"LLVMAddDeadStoreEliminationPass",SideEffects::worstDefault,"LLVMAddDeadStoreEliminationPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:53:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddScalarizerPass >(*this,lib,"LLVMAddScalarizerPass",SideEffects::worstDefault,"LLVMAddScalarizerPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:56:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddMergedLoadStoreMotionPass >(*this,lib,"LLVMAddMergedLoadStoreMotionPass",SideEffects::worstDefault,"LLVMAddMergedLoadStoreMotionPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:59:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddGVNPass >(*this,lib,"LLVMAddGVNPass",SideEffects::worstDefault,"LLVMAddGVNPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:62:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddNewGVNPass >(*this,lib,"LLVMAddNewGVNPass",SideEffects::worstDefault,"LLVMAddNewGVNPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:65:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddIndVarSimplifyPass >(*this,lib,"LLVMAddIndVarSimplifyPass",SideEffects::worstDefault,"LLVMAddIndVarSimplifyPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:68:6
/*	// BBATKIN: keep this manual for now
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddInstructionCombiningPass >(*this,lib,"LLVMAddInstructionCombiningPass",SideEffects::worstDefault,"LLVMAddInstructionCombiningPass")
		->args({"PM"});
*/
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:71:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddInstructionSimplifyPass >(*this,lib,"LLVMAddInstructionSimplifyPass",SideEffects::worstDefault,"LLVMAddInstructionSimplifyPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:74:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddJumpThreadingPass >(*this,lib,"LLVMAddJumpThreadingPass",SideEffects::worstDefault,"LLVMAddJumpThreadingPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:77:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLICMPass >(*this,lib,"LLVMAddLICMPass",SideEffects::worstDefault,"LLVMAddLICMPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:80:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopDeletionPass >(*this,lib,"LLVMAddLoopDeletionPass",SideEffects::worstDefault,"LLVMAddLoopDeletionPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:83:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopIdiomPass >(*this,lib,"LLVMAddLoopIdiomPass",SideEffects::worstDefault,"LLVMAddLoopIdiomPass")
		->args({"PM"});
}
}


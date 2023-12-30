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
void Module_dasLLVM::initFunctions_67() {
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:86:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopRotatePass >(*this,lib,"LLVMAddLoopRotatePass",SideEffects::worstDefault,"LLVMAddLoopRotatePass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:89:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopRerollPass >(*this,lib,"LLVMAddLoopRerollPass",SideEffects::worstDefault,"LLVMAddLoopRerollPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:92:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopUnrollPass >(*this,lib,"LLVMAddLoopUnrollPass",SideEffects::worstDefault,"LLVMAddLoopUnrollPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:95:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopUnrollAndJamPass >(*this,lib,"LLVMAddLoopUnrollAndJamPass",SideEffects::worstDefault,"LLVMAddLoopUnrollAndJamPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:98:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLowerAtomicPass >(*this,lib,"LLVMAddLowerAtomicPass",SideEffects::worstDefault,"LLVMAddLowerAtomicPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:101:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddMemCpyOptPass >(*this,lib,"LLVMAddMemCpyOptPass",SideEffects::worstDefault,"LLVMAddMemCpyOptPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:104:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddPartiallyInlineLibCallsPass >(*this,lib,"LLVMAddPartiallyInlineLibCallsPass",SideEffects::worstDefault,"LLVMAddPartiallyInlineLibCallsPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:107:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddReassociatePass >(*this,lib,"LLVMAddReassociatePass",SideEffects::worstDefault,"LLVMAddReassociatePass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:110:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddSCCPPass >(*this,lib,"LLVMAddSCCPPass",SideEffects::worstDefault,"LLVMAddSCCPPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:113:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddScalarReplAggregatesPass >(*this,lib,"LLVMAddScalarReplAggregatesPass",SideEffects::worstDefault,"LLVMAddScalarReplAggregatesPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:116:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddScalarReplAggregatesPassSSA >(*this,lib,"LLVMAddScalarReplAggregatesPassSSA",SideEffects::worstDefault,"LLVMAddScalarReplAggregatesPassSSA")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:119:6
	addExtern< void (*)(LLVMOpaquePassManager *,int) , LLVMAddScalarReplAggregatesPassWithThreshold >(*this,lib,"LLVMAddScalarReplAggregatesPassWithThreshold",SideEffects::worstDefault,"LLVMAddScalarReplAggregatesPassWithThreshold")
		->args({"PM","Threshold"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:123:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddSimplifyLibCallsPass >(*this,lib,"LLVMAddSimplifyLibCallsPass",SideEffects::worstDefault,"LLVMAddSimplifyLibCallsPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:126:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddTailCallEliminationPass >(*this,lib,"LLVMAddTailCallEliminationPass",SideEffects::worstDefault,"LLVMAddTailCallEliminationPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:129:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddDemoteMemoryToRegisterPass >(*this,lib,"LLVMAddDemoteMemoryToRegisterPass",SideEffects::worstDefault,"LLVMAddDemoteMemoryToRegisterPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:132:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddVerifierPass >(*this,lib,"LLVMAddVerifierPass",SideEffects::worstDefault,"LLVMAddVerifierPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:135:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddCorrelatedValuePropagationPass >(*this,lib,"LLVMAddCorrelatedValuePropagationPass",SideEffects::worstDefault,"LLVMAddCorrelatedValuePropagationPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:138:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddEarlyCSEPass >(*this,lib,"LLVMAddEarlyCSEPass",SideEffects::worstDefault,"LLVMAddEarlyCSEPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:141:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddEarlyCSEMemSSAPass >(*this,lib,"LLVMAddEarlyCSEMemSSAPass",SideEffects::worstDefault,"LLVMAddEarlyCSEMemSSAPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:144:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLowerExpectIntrinsicPass >(*this,lib,"LLVMAddLowerExpectIntrinsicPass",SideEffects::worstDefault,"LLVMAddLowerExpectIntrinsicPass")
		->args({"PM"});
}
}


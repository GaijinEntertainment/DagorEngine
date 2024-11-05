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
void Module_dasLLVM::initFunctions_64() {
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:107:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddReassociatePass , SimNode_ExtFuncCall >(lib,"LLVMAddReassociatePass","LLVMAddReassociatePass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:110:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddSCCPPass , SimNode_ExtFuncCall >(lib,"LLVMAddSCCPPass","LLVMAddSCCPPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:113:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddScalarReplAggregatesPass , SimNode_ExtFuncCall >(lib,"LLVMAddScalarReplAggregatesPass","LLVMAddScalarReplAggregatesPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:116:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddScalarReplAggregatesPassSSA , SimNode_ExtFuncCall >(lib,"LLVMAddScalarReplAggregatesPassSSA","LLVMAddScalarReplAggregatesPassSSA")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:119:6
	makeExtern< void (*)(LLVMOpaquePassManager *,int) , LLVMAddScalarReplAggregatesPassWithThreshold , SimNode_ExtFuncCall >(lib,"LLVMAddScalarReplAggregatesPassWithThreshold","LLVMAddScalarReplAggregatesPassWithThreshold")
		->args({"PM","Threshold"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:123:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddSimplifyLibCallsPass , SimNode_ExtFuncCall >(lib,"LLVMAddSimplifyLibCallsPass","LLVMAddSimplifyLibCallsPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:126:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddTailCallEliminationPass , SimNode_ExtFuncCall >(lib,"LLVMAddTailCallEliminationPass","LLVMAddTailCallEliminationPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:129:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddDemoteMemoryToRegisterPass , SimNode_ExtFuncCall >(lib,"LLVMAddDemoteMemoryToRegisterPass","LLVMAddDemoteMemoryToRegisterPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:132:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddVerifierPass , SimNode_ExtFuncCall >(lib,"LLVMAddVerifierPass","LLVMAddVerifierPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:135:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddCorrelatedValuePropagationPass , SimNode_ExtFuncCall >(lib,"LLVMAddCorrelatedValuePropagationPass","LLVMAddCorrelatedValuePropagationPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:138:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddEarlyCSEPass , SimNode_ExtFuncCall >(lib,"LLVMAddEarlyCSEPass","LLVMAddEarlyCSEPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:141:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddEarlyCSEMemSSAPass , SimNode_ExtFuncCall >(lib,"LLVMAddEarlyCSEMemSSAPass","LLVMAddEarlyCSEMemSSAPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:144:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLowerExpectIntrinsicPass , SimNode_ExtFuncCall >(lib,"LLVMAddLowerExpectIntrinsicPass","LLVMAddLowerExpectIntrinsicPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:147:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLowerConstantIntrinsicsPass , SimNode_ExtFuncCall >(lib,"LLVMAddLowerConstantIntrinsicsPass","LLVMAddLowerConstantIntrinsicsPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:150:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddTypeBasedAliasAnalysisPass , SimNode_ExtFuncCall >(lib,"LLVMAddTypeBasedAliasAnalysisPass","LLVMAddTypeBasedAliasAnalysisPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:153:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddScopedNoAliasAAPass , SimNode_ExtFuncCall >(lib,"LLVMAddScopedNoAliasAAPass","LLVMAddScopedNoAliasAAPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:156:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddBasicAliasAnalysisPass , SimNode_ExtFuncCall >(lib,"LLVMAddBasicAliasAnalysisPass","LLVMAddBasicAliasAnalysisPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:159:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddUnifyFunctionExitNodesPass , SimNode_ExtFuncCall >(lib,"LLVMAddUnifyFunctionExitNodesPass","LLVMAddUnifyFunctionExitNodesPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Utils.h:35:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLowerSwitchPass , SimNode_ExtFuncCall >(lib,"LLVMAddLowerSwitchPass","LLVMAddLowerSwitchPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Utils.h:38:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddPromoteMemoryToRegisterPass , SimNode_ExtFuncCall >(lib,"LLVMAddPromoteMemoryToRegisterPass","LLVMAddPromoteMemoryToRegisterPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


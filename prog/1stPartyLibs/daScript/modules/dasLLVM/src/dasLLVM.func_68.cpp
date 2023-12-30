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
void Module_dasLLVM::initFunctions_68() {
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:147:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLowerConstantIntrinsicsPass >(*this,lib,"LLVMAddLowerConstantIntrinsicsPass",SideEffects::worstDefault,"LLVMAddLowerConstantIntrinsicsPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:150:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddTypeBasedAliasAnalysisPass >(*this,lib,"LLVMAddTypeBasedAliasAnalysisPass",SideEffects::worstDefault,"LLVMAddTypeBasedAliasAnalysisPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:153:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddScopedNoAliasAAPass >(*this,lib,"LLVMAddScopedNoAliasAAPass",SideEffects::worstDefault,"LLVMAddScopedNoAliasAAPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:156:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddBasicAliasAnalysisPass >(*this,lib,"LLVMAddBasicAliasAnalysisPass",SideEffects::worstDefault,"LLVMAddBasicAliasAnalysisPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:159:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddUnifyFunctionExitNodesPass >(*this,lib,"LLVMAddUnifyFunctionExitNodesPass",SideEffects::worstDefault,"LLVMAddUnifyFunctionExitNodesPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Utils.h:35:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLowerSwitchPass >(*this,lib,"LLVMAddLowerSwitchPass",SideEffects::worstDefault,"LLVMAddLowerSwitchPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Utils.h:38:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddPromoteMemoryToRegisterPass >(*this,lib,"LLVMAddPromoteMemoryToRegisterPass",SideEffects::worstDefault,"LLVMAddPromoteMemoryToRegisterPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Utils.h:41:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddAddDiscriminatorsPass >(*this,lib,"LLVMAddAddDiscriminatorsPass",SideEffects::worstDefault,"LLVMAddAddDiscriminatorsPass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Vectorize.h:36:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopVectorizePass >(*this,lib,"LLVMAddLoopVectorizePass",SideEffects::worstDefault,"LLVMAddLoopVectorizePass")
		->args({"PM"});
// from D:\Work\libclang\include\llvm-c/Transforms/Vectorize.h:39:6
	addExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddSLPVectorizePass >(*this,lib,"LLVMAddSLPVectorizePass",SideEffects::worstDefault,"LLVMAddSLPVectorizePass")
		->args({"PM"});
}
}


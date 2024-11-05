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
void Module_dasLLVM::initFunctions_61() {
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:34:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddMergeFunctionsPass , SimNode_ExtFuncCall >(lib,"LLVMAddMergeFunctionsPass","LLVMAddMergeFunctionsPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:37:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddCalledValuePropagationPass , SimNode_ExtFuncCall >(lib,"LLVMAddCalledValuePropagationPass","LLVMAddCalledValuePropagationPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:40:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddDeadArgEliminationPass , SimNode_ExtFuncCall >(lib,"LLVMAddDeadArgEliminationPass","LLVMAddDeadArgEliminationPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:43:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddFunctionAttrsPass , SimNode_ExtFuncCall >(lib,"LLVMAddFunctionAttrsPass","LLVMAddFunctionAttrsPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:46:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddFunctionInliningPass , SimNode_ExtFuncCall >(lib,"LLVMAddFunctionInliningPass","LLVMAddFunctionInliningPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:49:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddAlwaysInlinerPass , SimNode_ExtFuncCall >(lib,"LLVMAddAlwaysInlinerPass","LLVMAddAlwaysInlinerPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:52:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddGlobalDCEPass , SimNode_ExtFuncCall >(lib,"LLVMAddGlobalDCEPass","LLVMAddGlobalDCEPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:55:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddGlobalOptimizerPass , SimNode_ExtFuncCall >(lib,"LLVMAddGlobalOptimizerPass","LLVMAddGlobalOptimizerPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:58:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddIPSCCPPass , SimNode_ExtFuncCall >(lib,"LLVMAddIPSCCPPass","LLVMAddIPSCCPPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:61:6
	makeExtern< void (*)(LLVMOpaquePassManager *,unsigned int) , LLVMAddInternalizePass , SimNode_ExtFuncCall >(lib,"LLVMAddInternalizePass","LLVMAddInternalizePass")
		->args({"","AllButMain"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:79:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddStripDeadPrototypesPass , SimNode_ExtFuncCall >(lib,"LLVMAddStripDeadPrototypesPass","LLVMAddStripDeadPrototypesPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:82:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddStripSymbolsPass , SimNode_ExtFuncCall >(lib,"LLVMAddStripSymbolsPass","LLVMAddStripSymbolsPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:49:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOpaqueModule *,const char *,LLVMOpaqueTargetMachine *,LLVMOpaquePassBuilderOptions *) , LLVMRunPasses , SimNode_ExtFuncCall >(lib,"LLVMRunPasses","LLVMRunPasses")
		->args({"M","Passes","TM","Options"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:60:27
	makeExtern< LLVMOpaquePassBuilderOptions * (*)() , LLVMCreatePassBuilderOptions , SimNode_ExtFuncCall >(lib,"LLVMCreatePassBuilderOptions","LLVMCreatePassBuilderOptions")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:66:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetVerifyEach , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetVerifyEach","LLVMPassBuilderOptionsSetVerifyEach")
		->args({"Options","VerifyEach"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:72:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetDebugLogging , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetDebugLogging","LLVMPassBuilderOptionsSetDebugLogging")
		->args({"Options","DebugLogging"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:75:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetLoopInterleaving , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetLoopInterleaving","LLVMPassBuilderOptionsSetLoopInterleaving")
		->args({"Options","LoopInterleaving"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:78:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetLoopVectorization , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetLoopVectorization","LLVMPassBuilderOptionsSetLoopVectorization")
		->args({"Options","LoopVectorization"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:81:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetSLPVectorization , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetSLPVectorization","LLVMPassBuilderOptionsSetSLPVectorization")
		->args({"Options","SLPVectorization"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:84:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetLoopUnrolling , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetLoopUnrolling","LLVMPassBuilderOptionsSetLoopUnrolling")
		->args({"Options","LoopUnrolling"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


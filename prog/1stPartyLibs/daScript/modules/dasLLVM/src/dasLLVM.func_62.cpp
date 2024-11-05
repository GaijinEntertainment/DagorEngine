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
void Module_dasLLVM::initFunctions_62() {
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:87:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll","LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll")
		->args({"Options","ForgetAllSCEVInLoopUnroll"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:90:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,unsigned int) , LLVMPassBuilderOptionsSetLicmMssaOptCap , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetLicmMssaOptCap","LLVMPassBuilderOptionsSetLicmMssaOptCap")
		->args({"Options","LicmMssaOptCap"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:93:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,unsigned int) , LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap","LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap")
		->args({"Options","LicmMssaNoAccForPromotionCap"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:96:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetCallGraphProfile , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetCallGraphProfile","LLVMPassBuilderOptionsSetCallGraphProfile")
		->args({"Options","CallGraphProfile"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:99:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetMergeFunctions , SimNode_ExtFuncCall >(lib,"LLVMPassBuilderOptionsSetMergeFunctions","LLVMPassBuilderOptionsSetMergeFunctions")
		->args({"Options","MergeFunctions"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:105:6
	makeExtern< void (*)(LLVMOpaquePassBuilderOptions *) , LLVMDisposePassBuilderOptions , SimNode_ExtFuncCall >(lib,"LLVMDisposePassBuilderOptions","LLVMDisposePassBuilderOptions")
		->args({"Options"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:32:27
	makeExtern< LLVMOpaquePassManagerBuilder * (*)() , LLVMPassManagerBuilderCreate , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderCreate","LLVMPassManagerBuilderCreate")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:33:6
	makeExtern< void (*)(LLVMOpaquePassManagerBuilder *) , LLVMPassManagerBuilderDispose , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderDispose","LLVMPassManagerBuilderDispose")
		->args({"PMB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:37:1
	makeExtern< void (*)(LLVMOpaquePassManagerBuilder *,unsigned int) , LLVMPassManagerBuilderSetOptLevel , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderSetOptLevel","LLVMPassManagerBuilderSetOptLevel")
		->args({"PMB","OptLevel"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:42:1
	makeExtern< void (*)(LLVMOpaquePassManagerBuilder *,unsigned int) , LLVMPassManagerBuilderSetSizeLevel , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderSetSizeLevel","LLVMPassManagerBuilderSetSizeLevel")
		->args({"PMB","SizeLevel"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:47:1
	makeExtern< void (*)(LLVMOpaquePassManagerBuilder *,int) , LLVMPassManagerBuilderSetDisableUnitAtATime , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderSetDisableUnitAtATime","LLVMPassManagerBuilderSetDisableUnitAtATime")
		->args({"PMB","Value"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:52:1
	makeExtern< void (*)(LLVMOpaquePassManagerBuilder *,int) , LLVMPassManagerBuilderSetDisableUnrollLoops , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderSetDisableUnrollLoops","LLVMPassManagerBuilderSetDisableUnrollLoops")
		->args({"PMB","Value"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:57:1
	makeExtern< void (*)(LLVMOpaquePassManagerBuilder *,int) , LLVMPassManagerBuilderSetDisableSimplifyLibCalls , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderSetDisableSimplifyLibCalls","LLVMPassManagerBuilderSetDisableSimplifyLibCalls")
		->args({"PMB","Value"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:62:1
	makeExtern< void (*)(LLVMOpaquePassManagerBuilder *,unsigned int) , LLVMPassManagerBuilderUseInlinerWithThreshold , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderUseInlinerWithThreshold","LLVMPassManagerBuilderUseInlinerWithThreshold")
		->args({"PMB","Threshold"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:67:1
	makeExtern< void (*)(LLVMOpaquePassManagerBuilder *,LLVMOpaquePassManager *) , LLVMPassManagerBuilderPopulateFunctionPassManager , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderPopulateFunctionPassManager","LLVMPassManagerBuilderPopulateFunctionPassManager")
		->args({"PMB","PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:72:1
	makeExtern< void (*)(LLVMOpaquePassManagerBuilder *,LLVMOpaquePassManager *) , LLVMPassManagerBuilderPopulateModulePassManager , SimNode_ExtFuncCall >(lib,"LLVMPassManagerBuilderPopulateModulePassManager","LLVMPassManagerBuilderPopulateModulePassManager")
		->args({"PMB","PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:35:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddAggressiveDCEPass , SimNode_ExtFuncCall >(lib,"LLVMAddAggressiveDCEPass","LLVMAddAggressiveDCEPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:38:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddDCEPass , SimNode_ExtFuncCall >(lib,"LLVMAddDCEPass","LLVMAddDCEPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:41:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddBitTrackingDCEPass , SimNode_ExtFuncCall >(lib,"LLVMAddBitTrackingDCEPass","LLVMAddBitTrackingDCEPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Scalar.h:44:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddAlignmentFromAssumptionsPass , SimNode_ExtFuncCall >(lib,"LLVMAddAlignmentFromAssumptionsPass","LLVMAddAlignmentFromAssumptionsPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


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
void Module_dasLLVM::initFunctions_65() {
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:60:27
	addExtern< LLVMOpaquePassBuilderOptions * (*)() , LLVMCreatePassBuilderOptions >(*this,lib,"LLVMCreatePassBuilderOptions",SideEffects::worstDefault,"LLVMCreatePassBuilderOptions");
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:66:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetVerifyEach >(*this,lib,"LLVMPassBuilderOptionsSetVerifyEach",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetVerifyEach")
		->args({"Options","VerifyEach"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:72:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetDebugLogging >(*this,lib,"LLVMPassBuilderOptionsSetDebugLogging",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetDebugLogging")
		->args({"Options","DebugLogging"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:75:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetLoopInterleaving >(*this,lib,"LLVMPassBuilderOptionsSetLoopInterleaving",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetLoopInterleaving")
		->args({"Options","LoopInterleaving"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:78:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetLoopVectorization >(*this,lib,"LLVMPassBuilderOptionsSetLoopVectorization",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetLoopVectorization")
		->args({"Options","LoopVectorization"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:81:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetSLPVectorization >(*this,lib,"LLVMPassBuilderOptionsSetSLPVectorization",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetSLPVectorization")
		->args({"Options","SLPVectorization"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:84:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetLoopUnrolling >(*this,lib,"LLVMPassBuilderOptionsSetLoopUnrolling",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetLoopUnrolling")
		->args({"Options","LoopUnrolling"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:87:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll >(*this,lib,"LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll")
		->args({"Options","ForgetAllSCEVInLoopUnroll"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:90:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,unsigned int) , LLVMPassBuilderOptionsSetLicmMssaOptCap >(*this,lib,"LLVMPassBuilderOptionsSetLicmMssaOptCap",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetLicmMssaOptCap")
		->args({"Options","LicmMssaOptCap"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:93:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,unsigned int) , LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap >(*this,lib,"LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetLicmMssaNoAccForPromotionCap")
		->args({"Options","LicmMssaNoAccForPromotionCap"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:96:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetCallGraphProfile >(*this,lib,"LLVMPassBuilderOptionsSetCallGraphProfile",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetCallGraphProfile")
		->args({"Options","CallGraphProfile"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:99:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *,int) , LLVMPassBuilderOptionsSetMergeFunctions >(*this,lib,"LLVMPassBuilderOptionsSetMergeFunctions",SideEffects::worstDefault,"LLVMPassBuilderOptionsSetMergeFunctions")
		->args({"Options","MergeFunctions"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassBuilder.h:105:6
	addExtern< void (*)(LLVMOpaquePassBuilderOptions *) , LLVMDisposePassBuilderOptions >(*this,lib,"LLVMDisposePassBuilderOptions",SideEffects::worstDefault,"LLVMDisposePassBuilderOptions")
		->args({"Options"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:32:27
	addExtern< LLVMOpaquePassManagerBuilder * (*)() , LLVMPassManagerBuilderCreate >(*this,lib,"LLVMPassManagerBuilderCreate",SideEffects::worstDefault,"LLVMPassManagerBuilderCreate");
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:33:6
	addExtern< void (*)(LLVMOpaquePassManagerBuilder *) , LLVMPassManagerBuilderDispose >(*this,lib,"LLVMPassManagerBuilderDispose",SideEffects::worstDefault,"LLVMPassManagerBuilderDispose")
		->args({"PMB"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:37:1
	addExtern< void (*)(LLVMOpaquePassManagerBuilder *,unsigned int) , LLVMPassManagerBuilderSetOptLevel >(*this,lib,"LLVMPassManagerBuilderSetOptLevel",SideEffects::worstDefault,"LLVMPassManagerBuilderSetOptLevel")
		->args({"PMB","OptLevel"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:42:1
	addExtern< void (*)(LLVMOpaquePassManagerBuilder *,unsigned int) , LLVMPassManagerBuilderSetSizeLevel >(*this,lib,"LLVMPassManagerBuilderSetSizeLevel",SideEffects::worstDefault,"LLVMPassManagerBuilderSetSizeLevel")
		->args({"PMB","SizeLevel"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:47:1
	addExtern< void (*)(LLVMOpaquePassManagerBuilder *,int) , LLVMPassManagerBuilderSetDisableUnitAtATime >(*this,lib,"LLVMPassManagerBuilderSetDisableUnitAtATime",SideEffects::worstDefault,"LLVMPassManagerBuilderSetDisableUnitAtATime")
		->args({"PMB","Value"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:52:1
	addExtern< void (*)(LLVMOpaquePassManagerBuilder *,int) , LLVMPassManagerBuilderSetDisableUnrollLoops >(*this,lib,"LLVMPassManagerBuilderSetDisableUnrollLoops",SideEffects::worstDefault,"LLVMPassManagerBuilderSetDisableUnrollLoops")
		->args({"PMB","Value"});
// from D:\Work\libclang\include\llvm-c/Transforms/PassManagerBuilder.h:57:1
	addExtern< void (*)(LLVMOpaquePassManagerBuilder *,int) , LLVMPassManagerBuilderSetDisableSimplifyLibCalls >(*this,lib,"LLVMPassManagerBuilderSetDisableSimplifyLibCalls",SideEffects::worstDefault,"LLVMPassManagerBuilderSetDisableSimplifyLibCalls")
		->args({"PMB","Value"});
}
}


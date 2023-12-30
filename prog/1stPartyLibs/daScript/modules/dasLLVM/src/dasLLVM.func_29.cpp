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
void Module_dasLLVM::initFunctions_29() {
// from D:\Work\libclang\include\llvm-c/Core.h:3657:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueBuilder *) , LLVMGetCurrentDebugLocation2 >(*this,lib,"LLVMGetCurrentDebugLocation2",SideEffects::worstDefault,"LLVMGetCurrentDebugLocation2")
		->args({"Builder"});
// from D:\Work\libclang\include\llvm-c/Core.h:3666:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueMetadata *) , LLVMSetCurrentDebugLocation2 >(*this,lib,"LLVMSetCurrentDebugLocation2",SideEffects::worstDefault,"LLVMSetCurrentDebugLocation2")
		->args({"Builder","Loc"});
// from D:\Work\libclang\include\llvm-c/Core.h:3678:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMSetInstDebugLocation >(*this,lib,"LLVMSetInstDebugLocation",SideEffects::worstDefault,"LLVMSetInstDebugLocation")
		->args({"Builder","Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3685:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMAddMetadataToInst >(*this,lib,"LLVMAddMetadataToInst",SideEffects::worstDefault,"LLVMAddMetadataToInst")
		->args({"Builder","Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3692:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueBuilder *) , LLVMBuilderGetDefaultFPMathTag >(*this,lib,"LLVMBuilderGetDefaultFPMathTag",SideEffects::worstDefault,"LLVMBuilderGetDefaultFPMathTag")
		->args({"Builder"});
// from D:\Work\libclang\include\llvm-c/Core.h:3701:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueMetadata *) , LLVMBuilderSetDefaultFPMathTag >(*this,lib,"LLVMBuilderSetDefaultFPMathTag",SideEffects::worstDefault,"LLVMBuilderSetDefaultFPMathTag")
		->args({"Builder","FPMathTag"});
// from D:\Work\libclang\include\llvm-c/Core.h:3708:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMSetCurrentDebugLocation >(*this,lib,"LLVMSetCurrentDebugLocation",SideEffects::worstDefault,"LLVMSetCurrentDebugLocation")
		->args({"Builder","L"});
// from D:\Work\libclang\include\llvm-c/Core.h:3713:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *) , LLVMGetCurrentDebugLocation >(*this,lib,"LLVMGetCurrentDebugLocation",SideEffects::worstDefault,"LLVMGetCurrentDebugLocation")
		->args({"Builder"});
// from D:\Work\libclang\include\llvm-c/Core.h:3716:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *) , LLVMBuildRetVoid >(*this,lib,"LLVMBuildRetVoid",SideEffects::worstDefault,"LLVMBuildRetVoid")
		->args({""});
// from D:\Work\libclang\include\llvm-c/Core.h:3717:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMBuildRet >(*this,lib,"LLVMBuildRet",SideEffects::worstDefault,"LLVMBuildRet")
		->args({"","V"});
// from D:\Work\libclang\include\llvm-c/Core.h:3718:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue **,unsigned int) , LLVMBuildAggregateRet >(*this,lib,"LLVMBuildAggregateRet",SideEffects::worstDefault,"LLVMBuildAggregateRet")
		->args({"","RetVals","N"});
// from D:\Work\libclang\include\llvm-c/Core.h:3720:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueBasicBlock *) , LLVMBuildBr >(*this,lib,"LLVMBuildBr",SideEffects::worstDefault,"LLVMBuildBr")
		->args({"","Dest"});
// from D:\Work\libclang\include\llvm-c/Core.h:3721:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *,LLVMOpaqueBasicBlock *) , LLVMBuildCondBr >(*this,lib,"LLVMBuildCondBr",SideEffects::worstDefault,"LLVMBuildCondBr")
		->args({"","If","Then","Else"});
// from D:\Work\libclang\include\llvm-c/Core.h:3723:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *,unsigned int) , LLVMBuildSwitch >(*this,lib,"LLVMBuildSwitch",SideEffects::worstDefault,"LLVMBuildSwitch")
		->args({"","V","Else","NumCases"});
// from D:\Work\libclang\include\llvm-c/Core.h:3725:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,unsigned int) , LLVMBuildIndirectBr >(*this,lib,"LLVMBuildIndirectBr",SideEffects::worstDefault,"LLVMBuildIndirectBr")
		->args({"B","Addr","NumDests"});
// from D:\Work\libclang\include\llvm-c/Core.h:3728:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,LLVMOpaqueBasicBlock *,LLVMOpaqueBasicBlock *,const char *) , LLVMBuildInvoke >(*this,lib,"LLVMBuildInvoke",SideEffects::worstDefault,"LLVMBuildInvoke")
		->args({"","Fn","Args","NumArgs","Then","Catch","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3733:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,LLVMOpaqueBasicBlock *,LLVMOpaqueBasicBlock *,const char *) , LLVMBuildInvoke2 >(*this,lib,"LLVMBuildInvoke2",SideEffects::worstDefault,"LLVMBuildInvoke2")
		->args({"","Ty","Fn","Args","NumArgs","Then","Catch","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3737:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *) , LLVMBuildUnreachable >(*this,lib,"LLVMBuildUnreachable",SideEffects::worstDefault,"LLVMBuildUnreachable")
		->args({""});
// from D:\Work\libclang\include\llvm-c/Core.h:3740:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMBuildResume >(*this,lib,"LLVMBuildResume",SideEffects::worstDefault,"LLVMBuildResume")
		->args({"B","Exn"});
// from D:\Work\libclang\include\llvm-c/Core.h:3741:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,unsigned int,const char *) , LLVMBuildLandingPad >(*this,lib,"LLVMBuildLandingPad",SideEffects::worstDefault,"LLVMBuildLandingPad")
		->args({"B","Ty","PersFn","NumClauses","Name"});
}
}


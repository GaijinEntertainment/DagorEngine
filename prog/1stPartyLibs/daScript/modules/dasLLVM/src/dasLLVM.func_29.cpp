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
// from D:\Work\libclang\include\llvm-c/Core.h:3683:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMSetInstDebugLocation , SimNode_ExtFuncCall >(lib,"LLVMSetInstDebugLocation","LLVMSetInstDebugLocation")
		->args({"Builder","Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3690:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMAddMetadataToInst , SimNode_ExtFuncCall >(lib,"LLVMAddMetadataToInst","LLVMAddMetadataToInst")
		->args({"Builder","Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3697:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueBuilder *) , LLVMBuilderGetDefaultFPMathTag , SimNode_ExtFuncCall >(lib,"LLVMBuilderGetDefaultFPMathTag","LLVMBuilderGetDefaultFPMathTag")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3706:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueMetadata *) , LLVMBuilderSetDefaultFPMathTag , SimNode_ExtFuncCall >(lib,"LLVMBuilderSetDefaultFPMathTag","LLVMBuilderSetDefaultFPMathTag")
		->args({"Builder","FPMathTag"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3713:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMSetCurrentDebugLocation , SimNode_ExtFuncCall >(lib,"LLVMSetCurrentDebugLocation","LLVMSetCurrentDebugLocation")
		->args({"Builder","L"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3718:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *) , LLVMGetCurrentDebugLocation , SimNode_ExtFuncCall >(lib,"LLVMGetCurrentDebugLocation","LLVMGetCurrentDebugLocation")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3721:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *) , LLVMBuildRetVoid , SimNode_ExtFuncCall >(lib,"LLVMBuildRetVoid","LLVMBuildRetVoid")
		->args({""})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3722:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMBuildRet , SimNode_ExtFuncCall >(lib,"LLVMBuildRet","LLVMBuildRet")
		->args({"","V"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3723:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue **,unsigned int) , LLVMBuildAggregateRet , SimNode_ExtFuncCall >(lib,"LLVMBuildAggregateRet","LLVMBuildAggregateRet")
		->args({"","RetVals","N"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3725:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueBasicBlock *) , LLVMBuildBr , SimNode_ExtFuncCall >(lib,"LLVMBuildBr","LLVMBuildBr")
		->args({"","Dest"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3726:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *,LLVMOpaqueBasicBlock *) , LLVMBuildCondBr , SimNode_ExtFuncCall >(lib,"LLVMBuildCondBr","LLVMBuildCondBr")
		->args({"","If","Then","Else"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3728:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *,unsigned int) , LLVMBuildSwitch , SimNode_ExtFuncCall >(lib,"LLVMBuildSwitch","LLVMBuildSwitch")
		->args({"","V","Else","NumCases"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3730:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,unsigned int) , LLVMBuildIndirectBr , SimNode_ExtFuncCall >(lib,"LLVMBuildIndirectBr","LLVMBuildIndirectBr")
		->args({"B","Addr","NumDests"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3732:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,LLVMOpaqueBasicBlock *,LLVMOpaqueBasicBlock *,const char *) , LLVMBuildInvoke2 , SimNode_ExtFuncCall >(lib,"LLVMBuildInvoke2","LLVMBuildInvoke2")
		->args({"","Ty","Fn","Args","NumArgs","Then","Catch","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3736:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *) , LLVMBuildUnreachable , SimNode_ExtFuncCall >(lib,"LLVMBuildUnreachable","LLVMBuildUnreachable")
		->args({""})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3739:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMBuildResume , SimNode_ExtFuncCall >(lib,"LLVMBuildResume","LLVMBuildResume")
		->args({"B","Exn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3740:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,unsigned int,const char *) , LLVMBuildLandingPad , SimNode_ExtFuncCall >(lib,"LLVMBuildLandingPad","LLVMBuildLandingPad")
		->args({"B","Ty","PersFn","NumClauses","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3743:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMBuildCleanupRet , SimNode_ExtFuncCall >(lib,"LLVMBuildCleanupRet","LLVMBuildCleanupRet")
		->args({"B","CatchPad","BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3745:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMBuildCatchRet , SimNode_ExtFuncCall >(lib,"LLVMBuildCatchRet","LLVMBuildCatchRet")
		->args({"B","CatchPad","BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3747:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildCatchPad , SimNode_ExtFuncCall >(lib,"LLVMBuildCatchPad","LLVMBuildCatchPad")
		->args({"B","ParentPad","Args","NumArgs","Name"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


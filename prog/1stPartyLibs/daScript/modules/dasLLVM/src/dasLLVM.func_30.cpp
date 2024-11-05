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
void Module_dasLLVM::initFunctions_30() {
// from D:\Work\libclang\include\llvm-c/Core.h:3750:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildCleanupPad , SimNode_ExtFuncCall >(lib,"LLVMBuildCleanupPad","LLVMBuildCleanupPad")
		->args({"B","ParentPad","Args","NumArgs","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3753:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *,unsigned int,const char *) , LLVMBuildCatchSwitch , SimNode_ExtFuncCall >(lib,"LLVMBuildCatchSwitch","LLVMBuildCatchSwitch")
		->args({"B","ParentPad","UnwindBB","NumHandlers","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3758:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMAddCase , SimNode_ExtFuncCall >(lib,"LLVMAddCase","LLVMAddCase")
		->args({"Switch","OnVal","Dest"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3762:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMAddDestination , SimNode_ExtFuncCall >(lib,"LLVMAddDestination","LLVMAddDestination")
		->args({"IndirectBr","Dest"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3765:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumClauses , SimNode_ExtFuncCall >(lib,"LLVMGetNumClauses","LLVMGetNumClauses")
		->args({"LandingPad"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3768:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetClause , SimNode_ExtFuncCall >(lib,"LLVMGetClause","LLVMGetClause")
		->args({"LandingPad","Idx"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3771:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMAddClause , SimNode_ExtFuncCall >(lib,"LLVMAddClause","LLVMAddClause")
		->args({"LandingPad","ClauseVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3774:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsCleanup , SimNode_ExtFuncCall >(lib,"LLVMIsCleanup","LLVMIsCleanup")
		->args({"LandingPad"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3777:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetCleanup , SimNode_ExtFuncCall >(lib,"LLVMSetCleanup","LLVMSetCleanup")
		->args({"LandingPad","Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3780:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMAddHandler , SimNode_ExtFuncCall >(lib,"LLVMAddHandler","LLVMAddHandler")
		->args({"CatchSwitch","Dest"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3783:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumHandlers , SimNode_ExtFuncCall >(lib,"LLVMGetNumHandlers","LLVMGetNumHandlers")
		->args({"CatchSwitch"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3796:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock **) , LLVMGetHandlers , SimNode_ExtFuncCall >(lib,"LLVMGetHandlers","LLVMGetHandlers")
		->args({"CatchSwitch","Handlers"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3801:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetArgOperand , SimNode_ExtFuncCall >(lib,"LLVMGetArgOperand","LLVMGetArgOperand")
		->args({"Funclet","i"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3804:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMSetArgOperand , SimNode_ExtFuncCall >(lib,"LLVMSetArgOperand","LLVMSetArgOperand")
		->args({"Funclet","i","value"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3813:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetParentCatchSwitch , SimNode_ExtFuncCall >(lib,"LLVMGetParentCatchSwitch","LLVMGetParentCatchSwitch")
		->args({"CatchPad"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3822:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetParentCatchSwitch , SimNode_ExtFuncCall >(lib,"LLVMSetParentCatchSwitch","LLVMSetParentCatchSwitch")
		->args({"CatchPad","CatchSwitch"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3825:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildAdd , SimNode_ExtFuncCall >(lib,"LLVMBuildAdd","LLVMBuildAdd")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3827:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNSWAdd , SimNode_ExtFuncCall >(lib,"LLVMBuildNSWAdd","LLVMBuildNSWAdd")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3829:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNUWAdd , SimNode_ExtFuncCall >(lib,"LLVMBuildNUWAdd","LLVMBuildNUWAdd")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3831:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFAdd , SimNode_ExtFuncCall >(lib,"LLVMBuildFAdd","LLVMBuildFAdd")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


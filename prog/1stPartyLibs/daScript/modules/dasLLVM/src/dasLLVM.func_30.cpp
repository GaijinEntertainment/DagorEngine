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
// from D:\Work\libclang\include\llvm-c/Core.h:3744:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMBuildCleanupRet >(*this,lib,"LLVMBuildCleanupRet",SideEffects::worstDefault,"LLVMBuildCleanupRet")
		->args({"B","CatchPad","BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3746:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMBuildCatchRet >(*this,lib,"LLVMBuildCatchRet",SideEffects::worstDefault,"LLVMBuildCatchRet")
		->args({"B","CatchPad","BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3748:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildCatchPad >(*this,lib,"LLVMBuildCatchPad",SideEffects::worstDefault,"LLVMBuildCatchPad")
		->args({"B","ParentPad","Args","NumArgs","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3751:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildCleanupPad >(*this,lib,"LLVMBuildCleanupPad",SideEffects::worstDefault,"LLVMBuildCleanupPad")
		->args({"B","ParentPad","Args","NumArgs","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3754:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *,unsigned int,const char *) , LLVMBuildCatchSwitch >(*this,lib,"LLVMBuildCatchSwitch",SideEffects::worstDefault,"LLVMBuildCatchSwitch")
		->args({"B","ParentPad","UnwindBB","NumHandlers","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3759:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMAddCase >(*this,lib,"LLVMAddCase",SideEffects::worstDefault,"LLVMAddCase")
		->args({"Switch","OnVal","Dest"});
// from D:\Work\libclang\include\llvm-c/Core.h:3763:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMAddDestination >(*this,lib,"LLVMAddDestination",SideEffects::worstDefault,"LLVMAddDestination")
		->args({"IndirectBr","Dest"});
// from D:\Work\libclang\include\llvm-c/Core.h:3766:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumClauses >(*this,lib,"LLVMGetNumClauses",SideEffects::worstDefault,"LLVMGetNumClauses")
		->args({"LandingPad"});
// from D:\Work\libclang\include\llvm-c/Core.h:3769:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetClause >(*this,lib,"LLVMGetClause",SideEffects::worstDefault,"LLVMGetClause")
		->args({"LandingPad","Idx"});
// from D:\Work\libclang\include\llvm-c/Core.h:3772:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMAddClause >(*this,lib,"LLVMAddClause",SideEffects::worstDefault,"LLVMAddClause")
		->args({"LandingPad","ClauseVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:3775:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsCleanup >(*this,lib,"LLVMIsCleanup",SideEffects::worstDefault,"LLVMIsCleanup")
		->args({"LandingPad"});
// from D:\Work\libclang\include\llvm-c/Core.h:3778:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetCleanup >(*this,lib,"LLVMSetCleanup",SideEffects::worstDefault,"LLVMSetCleanup")
		->args({"LandingPad","Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:3781:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMAddHandler >(*this,lib,"LLVMAddHandler",SideEffects::worstDefault,"LLVMAddHandler")
		->args({"CatchSwitch","Dest"});
// from D:\Work\libclang\include\llvm-c/Core.h:3784:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumHandlers >(*this,lib,"LLVMGetNumHandlers",SideEffects::worstDefault,"LLVMGetNumHandlers")
		->args({"CatchSwitch"});
// from D:\Work\libclang\include\llvm-c/Core.h:3797:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock **) , LLVMGetHandlers >(*this,lib,"LLVMGetHandlers",SideEffects::worstDefault,"LLVMGetHandlers")
		->args({"CatchSwitch","Handlers"});
// from D:\Work\libclang\include\llvm-c/Core.h:3802:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetArgOperand >(*this,lib,"LLVMGetArgOperand",SideEffects::worstDefault,"LLVMGetArgOperand")
		->args({"Funclet","i"});
// from D:\Work\libclang\include\llvm-c/Core.h:3805:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMSetArgOperand >(*this,lib,"LLVMSetArgOperand",SideEffects::worstDefault,"LLVMSetArgOperand")
		->args({"Funclet","i","value"});
// from D:\Work\libclang\include\llvm-c/Core.h:3814:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetParentCatchSwitch >(*this,lib,"LLVMGetParentCatchSwitch",SideEffects::worstDefault,"LLVMGetParentCatchSwitch")
		->args({"CatchPad"});
// from D:\Work\libclang\include\llvm-c/Core.h:3823:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetParentCatchSwitch >(*this,lib,"LLVMSetParentCatchSwitch",SideEffects::worstDefault,"LLVMSetParentCatchSwitch")
		->args({"CatchPad","CatchSwitch"});
// from D:\Work\libclang\include\llvm-c/Core.h:3826:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildAdd >(*this,lib,"LLVMBuildAdd",SideEffects::worstDefault,"LLVMBuildAdd")
		->args({"","LHS","RHS","Name"});
}
}


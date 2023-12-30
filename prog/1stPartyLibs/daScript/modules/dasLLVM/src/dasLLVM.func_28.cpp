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
void Module_dasLLVM::initFunctions_28() {
// from D:\Work\libclang\include\llvm-c/Core.h:3527:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMGetAllocatedType >(*this,lib,"LLVMGetAllocatedType",SideEffects::worstDefault,"LLVMGetAllocatedType")
		->args({"Alloca"});
// from D:\Work\libclang\include\llvm-c/Core.h:3545:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsInBounds >(*this,lib,"LLVMIsInBounds",SideEffects::worstDefault,"LLVMIsInBounds")
		->args({"GEP"});
// from D:\Work\libclang\include\llvm-c/Core.h:3550:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetIsInBounds >(*this,lib,"LLVMSetIsInBounds",SideEffects::worstDefault,"LLVMSetIsInBounds")
		->args({"GEP","InBounds"});
// from D:\Work\libclang\include\llvm-c/Core.h:3555:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMGetGEPSourceElementType >(*this,lib,"LLVMGetGEPSourceElementType",SideEffects::worstDefault,"LLVMGetGEPSourceElementType")
		->args({"GEP"});
// from D:\Work\libclang\include\llvm-c/Core.h:3573:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue **,LLVMOpaqueBasicBlock **,unsigned int) , LLVMAddIncoming >(*this,lib,"LLVMAddIncoming",SideEffects::worstDefault,"LLVMAddIncoming")
		->args({"PhiNode","IncomingValues","IncomingBlocks","Count"});
// from D:\Work\libclang\include\llvm-c/Core.h:3579:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMCountIncoming >(*this,lib,"LLVMCountIncoming",SideEffects::worstDefault,"LLVMCountIncoming")
		->args({"PhiNode"});
// from D:\Work\libclang\include\llvm-c/Core.h:3584:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetIncomingValue >(*this,lib,"LLVMGetIncomingValue",SideEffects::worstDefault,"LLVMGetIncomingValue")
		->args({"PhiNode","Index"});
// from D:\Work\libclang\include\llvm-c/Core.h:3589:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetIncomingBlock >(*this,lib,"LLVMGetIncomingBlock",SideEffects::worstDefault,"LLVMGetIncomingBlock")
		->args({"PhiNode","Index"});
// from D:\Work\libclang\include\llvm-c/Core.h:3609:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumIndices >(*this,lib,"LLVMGetNumIndices",SideEffects::worstDefault,"LLVMGetNumIndices")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3614:17
	addExtern< const unsigned int * (*)(LLVMOpaqueValue *) , LLVMGetIndices >(*this,lib,"LLVMGetIndices",SideEffects::worstDefault,"LLVMGetIndices")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3637:16
	addExtern< LLVMOpaqueBuilder * (*)(LLVMOpaqueContext *) , LLVMCreateBuilderInContext >(*this,lib,"LLVMCreateBuilderInContext",SideEffects::worstDefault,"LLVMCreateBuilderInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:3638:16
	addExtern< LLVMOpaqueBuilder * (*)() , LLVMCreateBuilder >(*this,lib,"LLVMCreateBuilder",SideEffects::worstDefault,"LLVMCreateBuilder");
// from D:\Work\libclang\include\llvm-c/Core.h:3639:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueBasicBlock *,LLVMOpaqueValue *) , LLVMPositionBuilder >(*this,lib,"LLVMPositionBuilder",SideEffects::worstDefault,"LLVMPositionBuilder")
		->args({"Builder","Block","Instr"});
// from D:\Work\libclang\include\llvm-c/Core.h:3641:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMPositionBuilderBefore >(*this,lib,"LLVMPositionBuilderBefore",SideEffects::worstDefault,"LLVMPositionBuilderBefore")
		->args({"Builder","Instr"});
// from D:\Work\libclang\include\llvm-c/Core.h:3642:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueBasicBlock *) , LLVMPositionBuilderAtEnd >(*this,lib,"LLVMPositionBuilderAtEnd",SideEffects::worstDefault,"LLVMPositionBuilderAtEnd")
		->args({"Builder","Block"});
// from D:\Work\libclang\include\llvm-c/Core.h:3643:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueBuilder *) , LLVMGetInsertBlock >(*this,lib,"LLVMGetInsertBlock",SideEffects::worstDefault,"LLVMGetInsertBlock")
		->args({"Builder"});
// from D:\Work\libclang\include\llvm-c/Core.h:3644:6
	addExtern< void (*)(LLVMOpaqueBuilder *) , LLVMClearInsertionPosition >(*this,lib,"LLVMClearInsertionPosition",SideEffects::worstDefault,"LLVMClearInsertionPosition")
		->args({"Builder"});
// from D:\Work\libclang\include\llvm-c/Core.h:3645:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMInsertIntoBuilder >(*this,lib,"LLVMInsertIntoBuilder",SideEffects::worstDefault,"LLVMInsertIntoBuilder")
		->args({"Builder","Instr"});
// from D:\Work\libclang\include\llvm-c/Core.h:3646:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMInsertIntoBuilderWithName >(*this,lib,"LLVMInsertIntoBuilderWithName",SideEffects::worstDefault,"LLVMInsertIntoBuilderWithName")
		->args({"Builder","Instr","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3648:6
	addExtern< void (*)(LLVMOpaqueBuilder *) , LLVMDisposeBuilder >(*this,lib,"LLVMDisposeBuilder",SideEffects::worstDefault,"LLVMDisposeBuilder")
		->args({"Builder"});
}
}


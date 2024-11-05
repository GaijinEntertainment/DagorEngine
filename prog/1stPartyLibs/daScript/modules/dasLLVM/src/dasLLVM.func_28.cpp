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
// from D:\Work\libclang\include\llvm-c/Core.h:3555:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetIsInBounds , SimNode_ExtFuncCall >(lib,"LLVMSetIsInBounds","LLVMSetIsInBounds")
		->args({"GEP","InBounds"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3560:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMGetGEPSourceElementType , SimNode_ExtFuncCall >(lib,"LLVMGetGEPSourceElementType","LLVMGetGEPSourceElementType")
		->args({"GEP"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3578:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue **,LLVMOpaqueBasicBlock **,unsigned int) , LLVMAddIncoming , SimNode_ExtFuncCall >(lib,"LLVMAddIncoming","LLVMAddIncoming")
		->args({"PhiNode","IncomingValues","IncomingBlocks","Count"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3584:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMCountIncoming , SimNode_ExtFuncCall >(lib,"LLVMCountIncoming","LLVMCountIncoming")
		->args({"PhiNode"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3589:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetIncomingValue , SimNode_ExtFuncCall >(lib,"LLVMGetIncomingValue","LLVMGetIncomingValue")
		->args({"PhiNode","Index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3594:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetIncomingBlock , SimNode_ExtFuncCall >(lib,"LLVMGetIncomingBlock","LLVMGetIncomingBlock")
		->args({"PhiNode","Index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3614:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumIndices , SimNode_ExtFuncCall >(lib,"LLVMGetNumIndices","LLVMGetNumIndices")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3619:17
	makeExtern< const unsigned int * (*)(LLVMOpaqueValue *) , LLVMGetIndices , SimNode_ExtFuncCall >(lib,"LLVMGetIndices","LLVMGetIndices")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3642:16
	makeExtern< LLVMOpaqueBuilder * (*)(LLVMOpaqueContext *) , LLVMCreateBuilderInContext , SimNode_ExtFuncCall >(lib,"LLVMCreateBuilderInContext","LLVMCreateBuilderInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3643:16
	makeExtern< LLVMOpaqueBuilder * (*)() , LLVMCreateBuilder , SimNode_ExtFuncCall >(lib,"LLVMCreateBuilder","LLVMCreateBuilder")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3644:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueBasicBlock *,LLVMOpaqueValue *) , LLVMPositionBuilder , SimNode_ExtFuncCall >(lib,"LLVMPositionBuilder","LLVMPositionBuilder")
		->args({"Builder","Block","Instr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3646:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMPositionBuilderBefore , SimNode_ExtFuncCall >(lib,"LLVMPositionBuilderBefore","LLVMPositionBuilderBefore")
		->args({"Builder","Instr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3647:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueBasicBlock *) , LLVMPositionBuilderAtEnd , SimNode_ExtFuncCall >(lib,"LLVMPositionBuilderAtEnd","LLVMPositionBuilderAtEnd")
		->args({"Builder","Block"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3648:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueBuilder *) , LLVMGetInsertBlock , SimNode_ExtFuncCall >(lib,"LLVMGetInsertBlock","LLVMGetInsertBlock")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3649:6
	makeExtern< void (*)(LLVMOpaqueBuilder *) , LLVMClearInsertionPosition , SimNode_ExtFuncCall >(lib,"LLVMClearInsertionPosition","LLVMClearInsertionPosition")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3650:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMInsertIntoBuilder , SimNode_ExtFuncCall >(lib,"LLVMInsertIntoBuilder","LLVMInsertIntoBuilder")
		->args({"Builder","Instr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3651:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMInsertIntoBuilderWithName , SimNode_ExtFuncCall >(lib,"LLVMInsertIntoBuilderWithName","LLVMInsertIntoBuilderWithName")
		->args({"Builder","Instr","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3653:6
	makeExtern< void (*)(LLVMOpaqueBuilder *) , LLVMDisposeBuilder , SimNode_ExtFuncCall >(lib,"LLVMDisposeBuilder","LLVMDisposeBuilder")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3662:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueBuilder *) , LLVMGetCurrentDebugLocation2 , SimNode_ExtFuncCall >(lib,"LLVMGetCurrentDebugLocation2","LLVMGetCurrentDebugLocation2")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3671:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueMetadata *) , LLVMSetCurrentDebugLocation2 , SimNode_ExtFuncCall >(lib,"LLVMSetCurrentDebugLocation2","LLVMSetCurrentDebugLocation2")
		->args({"Builder","Loc"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


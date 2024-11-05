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
void Module_dasLLVM::initFunctions_27() {
// from D:\Work\libclang\include\llvm-c/Core.h:3360:18
	makeExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueValue *,unsigned int,const char *,unsigned int) , LLVMGetCallSiteStringAttribute , SimNode_ExtFuncCall >(lib,"LLVMGetCallSiteStringAttribute","LLVMGetCallSiteStringAttribute")
		->args({"C","Idx","K","KLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3363:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMRemoveCallSiteEnumAttribute , SimNode_ExtFuncCall >(lib,"LLVMRemoveCallSiteEnumAttribute","LLVMRemoveCallSiteEnumAttribute")
		->args({"C","Idx","KindID"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3365:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,const char *,unsigned int) , LLVMRemoveCallSiteStringAttribute , SimNode_ExtFuncCall >(lib,"LLVMRemoveCallSiteStringAttribute","LLVMRemoveCallSiteStringAttribute")
		->args({"C","Idx","K","KLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3373:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMGetCalledFunctionType , SimNode_ExtFuncCall >(lib,"LLVMGetCalledFunctionType","LLVMGetCalledFunctionType")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3384:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetCalledValue , SimNode_ExtFuncCall >(lib,"LLVMGetCalledValue","LLVMGetCalledValue")
		->args({"Instr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3393:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsTailCall , SimNode_ExtFuncCall >(lib,"LLVMIsTailCall","LLVMIsTailCall")
		->args({"CallInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3402:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetTailCall , SimNode_ExtFuncCall >(lib,"LLVMSetTailCall","LLVMSetTailCall")
		->args({"CallInst","IsTailCall"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3411:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetNormalDest , SimNode_ExtFuncCall >(lib,"LLVMGetNormalDest","LLVMGetNormalDest")
		->args({"InvokeInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3423:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetUnwindDest , SimNode_ExtFuncCall >(lib,"LLVMGetUnwindDest","LLVMGetUnwindDest")
		->args({"InvokeInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3432:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMSetNormalDest , SimNode_ExtFuncCall >(lib,"LLVMSetNormalDest","LLVMSetNormalDest")
		->args({"InvokeInst","B"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3444:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMSetUnwindDest , SimNode_ExtFuncCall >(lib,"LLVMSetUnwindDest","LLVMSetUnwindDest")
		->args({"InvokeInst","B"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3464:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumSuccessors , SimNode_ExtFuncCall >(lib,"LLVMGetNumSuccessors","LLVMGetNumSuccessors")
		->args({"Term"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3471:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetSuccessor , SimNode_ExtFuncCall >(lib,"LLVMGetSuccessor","LLVMGetSuccessor")
		->args({"Term","i"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3478:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueBasicBlock *) , LLVMSetSuccessor , SimNode_ExtFuncCall >(lib,"LLVMSetSuccessor","LLVMSetSuccessor")
		->args({"Term","i","block"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3487:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsConditional , SimNode_ExtFuncCall >(lib,"LLVMIsConditional","LLVMIsConditional")
		->args({"Branch"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3496:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetCondition , SimNode_ExtFuncCall >(lib,"LLVMGetCondition","LLVMGetCondition")
		->args({"Branch"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3505:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetCondition , SimNode_ExtFuncCall >(lib,"LLVMSetCondition","LLVMSetCondition")
		->args({"Branch","Cond"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3514:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetSwitchDefaultDest , SimNode_ExtFuncCall >(lib,"LLVMGetSwitchDefaultDest","LLVMGetSwitchDefaultDest")
		->args({"SwitchInstr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3532:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMGetAllocatedType , SimNode_ExtFuncCall >(lib,"LLVMGetAllocatedType","LLVMGetAllocatedType")
		->args({"Alloca"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3550:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsInBounds , SimNode_ExtFuncCall >(lib,"LLVMIsInBounds","LLVMIsInBounds")
		->args({"GEP"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


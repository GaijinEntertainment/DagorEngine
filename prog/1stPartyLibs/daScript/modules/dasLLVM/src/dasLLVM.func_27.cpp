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
// from D:\Work\libclang\include\llvm-c/Core.h:3350:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueAttributeRef **) , LLVMGetCallSiteAttributes >(*this,lib,"LLVMGetCallSiteAttributes",SideEffects::worstDefault,"LLVMGetCallSiteAttributes")
		->args({"C","Idx","Attrs"});
// from D:\Work\libclang\include\llvm-c/Core.h:3352:18
	addExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMGetCallSiteEnumAttribute >(*this,lib,"LLVMGetCallSiteEnumAttribute",SideEffects::worstDefault,"LLVMGetCallSiteEnumAttribute")
		->args({"C","Idx","KindID"});
// from D:\Work\libclang\include\llvm-c/Core.h:3355:18
	addExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueValue *,unsigned int,const char *,unsigned int) , LLVMGetCallSiteStringAttribute >(*this,lib,"LLVMGetCallSiteStringAttribute",SideEffects::worstDefault,"LLVMGetCallSiteStringAttribute")
		->args({"C","Idx","K","KLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:3358:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMRemoveCallSiteEnumAttribute >(*this,lib,"LLVMRemoveCallSiteEnumAttribute",SideEffects::worstDefault,"LLVMRemoveCallSiteEnumAttribute")
		->args({"C","Idx","KindID"});
// from D:\Work\libclang\include\llvm-c/Core.h:3360:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,const char *,unsigned int) , LLVMRemoveCallSiteStringAttribute >(*this,lib,"LLVMRemoveCallSiteStringAttribute",SideEffects::worstDefault,"LLVMRemoveCallSiteStringAttribute")
		->args({"C","Idx","K","KLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:3368:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMGetCalledFunctionType >(*this,lib,"LLVMGetCalledFunctionType",SideEffects::worstDefault,"LLVMGetCalledFunctionType")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:3379:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetCalledValue >(*this,lib,"LLVMGetCalledValue",SideEffects::worstDefault,"LLVMGetCalledValue")
		->args({"Instr"});
// from D:\Work\libclang\include\llvm-c/Core.h:3388:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsTailCall >(*this,lib,"LLVMIsTailCall",SideEffects::worstDefault,"LLVMIsTailCall")
		->args({"CallInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3397:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetTailCall >(*this,lib,"LLVMSetTailCall",SideEffects::worstDefault,"LLVMSetTailCall")
		->args({"CallInst","IsTailCall"});
// from D:\Work\libclang\include\llvm-c/Core.h:3406:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetNormalDest >(*this,lib,"LLVMGetNormalDest",SideEffects::worstDefault,"LLVMGetNormalDest")
		->args({"InvokeInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3418:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetUnwindDest >(*this,lib,"LLVMGetUnwindDest",SideEffects::worstDefault,"LLVMGetUnwindDest")
		->args({"InvokeInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3427:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMSetNormalDest >(*this,lib,"LLVMSetNormalDest",SideEffects::worstDefault,"LLVMSetNormalDest")
		->args({"InvokeInst","B"});
// from D:\Work\libclang\include\llvm-c/Core.h:3439:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMSetUnwindDest >(*this,lib,"LLVMSetUnwindDest",SideEffects::worstDefault,"LLVMSetUnwindDest")
		->args({"InvokeInst","B"});
// from D:\Work\libclang\include\llvm-c/Core.h:3459:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumSuccessors >(*this,lib,"LLVMGetNumSuccessors",SideEffects::worstDefault,"LLVMGetNumSuccessors")
		->args({"Term"});
// from D:\Work\libclang\include\llvm-c/Core.h:3466:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetSuccessor >(*this,lib,"LLVMGetSuccessor",SideEffects::worstDefault,"LLVMGetSuccessor")
		->args({"Term","i"});
// from D:\Work\libclang\include\llvm-c/Core.h:3473:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueBasicBlock *) , LLVMSetSuccessor >(*this,lib,"LLVMSetSuccessor",SideEffects::worstDefault,"LLVMSetSuccessor")
		->args({"Term","i","block"});
// from D:\Work\libclang\include\llvm-c/Core.h:3482:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsConditional >(*this,lib,"LLVMIsConditional",SideEffects::worstDefault,"LLVMIsConditional")
		->args({"Branch"});
// from D:\Work\libclang\include\llvm-c/Core.h:3491:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetCondition >(*this,lib,"LLVMGetCondition",SideEffects::worstDefault,"LLVMGetCondition")
		->args({"Branch"});
// from D:\Work\libclang\include\llvm-c/Core.h:3500:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetCondition >(*this,lib,"LLVMSetCondition",SideEffects::worstDefault,"LLVMSetCondition")
		->args({"Branch","Cond"});
// from D:\Work\libclang\include\llvm-c/Core.h:3509:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetSwitchDefaultDest >(*this,lib,"LLVMGetSwitchDefaultDest",SideEffects::worstDefault,"LLVMGetSwitchDefaultDest")
		->args({"SwitchInstr"});
}
}


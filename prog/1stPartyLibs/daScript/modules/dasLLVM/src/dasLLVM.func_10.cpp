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
void Module_dasLLVM::initFunctions_10() {
// from D:\Work\libclang\include\llvm-c/Core.h:1579:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,const char *,LLVMOpaqueType **,unsigned int,unsigned int *,unsigned int) , LLVMTargetExtTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMTargetExtTypeInContext","LLVMTargetExtTypeInContext")
		->args({"C","Name","TypeParams","TypeParamCount","IntParams","IntParamCount"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1719:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMTypeOf , SimNode_ExtFuncCall >(lib,"LLVMTypeOf","LLVMTypeOf")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1726:15
	makeExtern< LLVMValueKind (*)(LLVMOpaqueValue *) , LLVMGetValueKind , SimNode_ExtFuncCall >(lib,"LLVMGetValueKind","LLVMGetValueKind")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1733:13
	makeExtern< const char * (*)(LLVMOpaqueValue *,size_t *) , LLVMGetValueName2 , SimNode_ExtFuncCall >(lib,"LLVMGetValueName2","LLVMGetValueName2")
		->args({"Val","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1740:6
	makeExtern< void (*)(LLVMOpaqueValue *,const char *,size_t) , LLVMSetValueName2 , SimNode_ExtFuncCall >(lib,"LLVMSetValueName2","LLVMSetValueName2")
		->args({"Val","Name","NameLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1747:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMDumpValue , SimNode_ExtFuncCall >(lib,"LLVMDumpValue","LLVMDumpValue")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1755:7
	makeExtern< char * (*)(LLVMOpaqueValue *) , LLVMPrintValueToString , SimNode_ExtFuncCall >(lib,"LLVMPrintValueToString","LLVMPrintValueToString")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1762:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMReplaceAllUsesWith , SimNode_ExtFuncCall >(lib,"LLVMReplaceAllUsesWith","LLVMReplaceAllUsesWith")
		->args({"OldVal","NewVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1767:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsConstant , SimNode_ExtFuncCall >(lib,"LLVMIsConstant","LLVMIsConstant")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1772:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsUndef , SimNode_ExtFuncCall >(lib,"LLVMIsUndef","LLVMIsUndef")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1777:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsPoison , SimNode_ExtFuncCall >(lib,"LLVMIsPoison","LLVMIsPoison")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAArgument , SimNode_ExtFuncCall >(lib,"LLVMIsAArgument","LLVMIsAArgument")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABasicBlock , SimNode_ExtFuncCall >(lib,"LLVMIsABasicBlock","LLVMIsABasicBlock")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInlineAsm , SimNode_ExtFuncCall >(lib,"LLVMIsAInlineAsm","LLVMIsAInlineAsm")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUser , SimNode_ExtFuncCall >(lib,"LLVMIsAUser","LLVMIsAUser")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstant , SimNode_ExtFuncCall >(lib,"LLVMIsAConstant","LLVMIsAConstant")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABlockAddress , SimNode_ExtFuncCall >(lib,"LLVMIsABlockAddress","LLVMIsABlockAddress")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantAggregateZero , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantAggregateZero","LLVMIsAConstantAggregateZero")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantArray , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantArray","LLVMIsAConstantArray")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantDataSequential , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantDataSequential","LLVMIsAConstantDataSequential")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


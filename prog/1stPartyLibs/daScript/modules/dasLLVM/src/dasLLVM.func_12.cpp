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
void Module_dasLLVM::initFunctions_12() {
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACallInst , SimNode_ExtFuncCall >(lib,"LLVMIsACallInst","LLVMIsACallInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAIntrinsicInst , SimNode_ExtFuncCall >(lib,"LLVMIsAIntrinsicInst","LLVMIsAIntrinsicInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsADbgInfoIntrinsic , SimNode_ExtFuncCall >(lib,"LLVMIsADbgInfoIntrinsic","LLVMIsADbgInfoIntrinsic")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsADbgVariableIntrinsic , SimNode_ExtFuncCall >(lib,"LLVMIsADbgVariableIntrinsic","LLVMIsADbgVariableIntrinsic")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsADbgDeclareInst , SimNode_ExtFuncCall >(lib,"LLVMIsADbgDeclareInst","LLVMIsADbgDeclareInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsADbgLabelInst , SimNode_ExtFuncCall >(lib,"LLVMIsADbgLabelInst","LLVMIsADbgLabelInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMemIntrinsic , SimNode_ExtFuncCall >(lib,"LLVMIsAMemIntrinsic","LLVMIsAMemIntrinsic")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMemCpyInst , SimNode_ExtFuncCall >(lib,"LLVMIsAMemCpyInst","LLVMIsAMemCpyInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMemMoveInst , SimNode_ExtFuncCall >(lib,"LLVMIsAMemMoveInst","LLVMIsAMemMoveInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMemSetInst , SimNode_ExtFuncCall >(lib,"LLVMIsAMemSetInst","LLVMIsAMemSetInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACmpInst , SimNode_ExtFuncCall >(lib,"LLVMIsACmpInst","LLVMIsACmpInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFCmpInst , SimNode_ExtFuncCall >(lib,"LLVMIsAFCmpInst","LLVMIsAFCmpInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAICmpInst , SimNode_ExtFuncCall >(lib,"LLVMIsAICmpInst","LLVMIsAICmpInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAExtractElementInst , SimNode_ExtFuncCall >(lib,"LLVMIsAExtractElementInst","LLVMIsAExtractElementInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGetElementPtrInst , SimNode_ExtFuncCall >(lib,"LLVMIsAGetElementPtrInst","LLVMIsAGetElementPtrInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInsertElementInst , SimNode_ExtFuncCall >(lib,"LLVMIsAInsertElementInst","LLVMIsAInsertElementInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInsertValueInst , SimNode_ExtFuncCall >(lib,"LLVMIsAInsertValueInst","LLVMIsAInsertValueInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsALandingPadInst , SimNode_ExtFuncCall >(lib,"LLVMIsALandingPadInst","LLVMIsALandingPadInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAPHINode , SimNode_ExtFuncCall >(lib,"LLVMIsAPHINode","LLVMIsAPHINode")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsASelectInst , SimNode_ExtFuncCall >(lib,"LLVMIsASelectInst","LLVMIsASelectInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


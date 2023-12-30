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
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsADbgInfoIntrinsic >(*this,lib,"LLVMIsADbgInfoIntrinsic",SideEffects::worstDefault,"LLVMIsADbgInfoIntrinsic")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsADbgVariableIntrinsic >(*this,lib,"LLVMIsADbgVariableIntrinsic",SideEffects::worstDefault,"LLVMIsADbgVariableIntrinsic")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsADbgDeclareInst >(*this,lib,"LLVMIsADbgDeclareInst",SideEffects::worstDefault,"LLVMIsADbgDeclareInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsADbgLabelInst >(*this,lib,"LLVMIsADbgLabelInst",SideEffects::worstDefault,"LLVMIsADbgLabelInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMemIntrinsic >(*this,lib,"LLVMIsAMemIntrinsic",SideEffects::worstDefault,"LLVMIsAMemIntrinsic")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMemCpyInst >(*this,lib,"LLVMIsAMemCpyInst",SideEffects::worstDefault,"LLVMIsAMemCpyInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMemMoveInst >(*this,lib,"LLVMIsAMemMoveInst",SideEffects::worstDefault,"LLVMIsAMemMoveInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMemSetInst >(*this,lib,"LLVMIsAMemSetInst",SideEffects::worstDefault,"LLVMIsAMemSetInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACmpInst >(*this,lib,"LLVMIsACmpInst",SideEffects::worstDefault,"LLVMIsACmpInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFCmpInst >(*this,lib,"LLVMIsAFCmpInst",SideEffects::worstDefault,"LLVMIsAFCmpInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAICmpInst >(*this,lib,"LLVMIsAICmpInst",SideEffects::worstDefault,"LLVMIsAICmpInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAExtractElementInst >(*this,lib,"LLVMIsAExtractElementInst",SideEffects::worstDefault,"LLVMIsAExtractElementInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGetElementPtrInst >(*this,lib,"LLVMIsAGetElementPtrInst",SideEffects::worstDefault,"LLVMIsAGetElementPtrInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInsertElementInst >(*this,lib,"LLVMIsAInsertElementInst",SideEffects::worstDefault,"LLVMIsAInsertElementInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInsertValueInst >(*this,lib,"LLVMIsAInsertValueInst",SideEffects::worstDefault,"LLVMIsAInsertValueInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsALandingPadInst >(*this,lib,"LLVMIsALandingPadInst",SideEffects::worstDefault,"LLVMIsALandingPadInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAPHINode >(*this,lib,"LLVMIsAPHINode",SideEffects::worstDefault,"LLVMIsAPHINode")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsASelectInst >(*this,lib,"LLVMIsASelectInst",SideEffects::worstDefault,"LLVMIsASelectInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAShuffleVectorInst >(*this,lib,"LLVMIsAShuffleVectorInst",SideEffects::worstDefault,"LLVMIsAShuffleVectorInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAStoreInst >(*this,lib,"LLVMIsAStoreInst",SideEffects::worstDefault,"LLVMIsAStoreInst")
		->args({"Val"});
}
}


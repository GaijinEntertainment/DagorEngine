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
void Module_dasLLVM::initFunctions_11() {
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantExpr >(*this,lib,"LLVMIsAConstantExpr",SideEffects::worstDefault,"LLVMIsAConstantExpr")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantFP >(*this,lib,"LLVMIsAConstantFP",SideEffects::worstDefault,"LLVMIsAConstantFP")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantInt >(*this,lib,"LLVMIsAConstantInt",SideEffects::worstDefault,"LLVMIsAConstantInt")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantPointerNull >(*this,lib,"LLVMIsAConstantPointerNull",SideEffects::worstDefault,"LLVMIsAConstantPointerNull")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantStruct >(*this,lib,"LLVMIsAConstantStruct",SideEffects::worstDefault,"LLVMIsAConstantStruct")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantTokenNone >(*this,lib,"LLVMIsAConstantTokenNone",SideEffects::worstDefault,"LLVMIsAConstantTokenNone")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantVector >(*this,lib,"LLVMIsAConstantVector",SideEffects::worstDefault,"LLVMIsAConstantVector")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalValue >(*this,lib,"LLVMIsAGlobalValue",SideEffects::worstDefault,"LLVMIsAGlobalValue")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalAlias >(*this,lib,"LLVMIsAGlobalAlias",SideEffects::worstDefault,"LLVMIsAGlobalAlias")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalObject >(*this,lib,"LLVMIsAGlobalObject",SideEffects::worstDefault,"LLVMIsAGlobalObject")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFunction >(*this,lib,"LLVMIsAFunction",SideEffects::worstDefault,"LLVMIsAFunction")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalVariable >(*this,lib,"LLVMIsAGlobalVariable",SideEffects::worstDefault,"LLVMIsAGlobalVariable")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalIFunc >(*this,lib,"LLVMIsAGlobalIFunc",SideEffects::worstDefault,"LLVMIsAGlobalIFunc")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUndefValue >(*this,lib,"LLVMIsAUndefValue",SideEffects::worstDefault,"LLVMIsAUndefValue")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAPoisonValue >(*this,lib,"LLVMIsAPoisonValue",SideEffects::worstDefault,"LLVMIsAPoisonValue")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInstruction >(*this,lib,"LLVMIsAInstruction",SideEffects::worstDefault,"LLVMIsAInstruction")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUnaryOperator >(*this,lib,"LLVMIsAUnaryOperator",SideEffects::worstDefault,"LLVMIsAUnaryOperator")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABinaryOperator >(*this,lib,"LLVMIsABinaryOperator",SideEffects::worstDefault,"LLVMIsABinaryOperator")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsACallInst >(*this,lib,"LLVMIsACallInst",SideEffects::worstDefault,"LLVMIsACallInst")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1771:30
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAIntrinsicInst >(*this,lib,"LLVMIsAIntrinsicInst",SideEffects::worstDefault,"LLVMIsAIntrinsicInst")
		->args({"Val"});
}
}


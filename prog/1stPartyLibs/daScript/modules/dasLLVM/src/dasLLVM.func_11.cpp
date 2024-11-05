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
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantDataArray , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantDataArray","LLVMIsAConstantDataArray")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantDataVector , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantDataVector","LLVMIsAConstantDataVector")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantExpr , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantExpr","LLVMIsAConstantExpr")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantFP , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantFP","LLVMIsAConstantFP")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantInt , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantInt","LLVMIsAConstantInt")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantPointerNull , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantPointerNull","LLVMIsAConstantPointerNull")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantStruct , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantStruct","LLVMIsAConstantStruct")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantTokenNone , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantTokenNone","LLVMIsAConstantTokenNone")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAConstantVector , SimNode_ExtFuncCall >(lib,"LLVMIsAConstantVector","LLVMIsAConstantVector")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalValue , SimNode_ExtFuncCall >(lib,"LLVMIsAGlobalValue","LLVMIsAGlobalValue")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalAlias , SimNode_ExtFuncCall >(lib,"LLVMIsAGlobalAlias","LLVMIsAGlobalAlias")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalObject , SimNode_ExtFuncCall >(lib,"LLVMIsAGlobalObject","LLVMIsAGlobalObject")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFunction , SimNode_ExtFuncCall >(lib,"LLVMIsAFunction","LLVMIsAFunction")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalVariable , SimNode_ExtFuncCall >(lib,"LLVMIsAGlobalVariable","LLVMIsAGlobalVariable")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAGlobalIFunc , SimNode_ExtFuncCall >(lib,"LLVMIsAGlobalIFunc","LLVMIsAGlobalIFunc")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUndefValue , SimNode_ExtFuncCall >(lib,"LLVMIsAUndefValue","LLVMIsAUndefValue")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAPoisonValue , SimNode_ExtFuncCall >(lib,"LLVMIsAPoisonValue","LLVMIsAPoisonValue")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAInstruction , SimNode_ExtFuncCall >(lib,"LLVMIsAInstruction","LLVMIsAInstruction")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUnaryOperator , SimNode_ExtFuncCall >(lib,"LLVMIsAUnaryOperator","LLVMIsAUnaryOperator")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABinaryOperator , SimNode_ExtFuncCall >(lib,"LLVMIsABinaryOperator","LLVMIsABinaryOperator")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


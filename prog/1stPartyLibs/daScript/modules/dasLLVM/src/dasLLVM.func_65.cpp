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
void Module_dasLLVM::initFunctions_65() {
// from D:\Work\libclang\include\llvm-c/Transforms/Utils.h:41:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddAddDiscriminatorsPass , SimNode_ExtFuncCall >(lib,"LLVMAddAddDiscriminatorsPass","LLVMAddAddDiscriminatorsPass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Vectorize.h:36:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddLoopVectorizePass , SimNode_ExtFuncCall >(lib,"LLVMAddLoopVectorizePass","LLVMAddLoopVectorizePass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/Vectorize.h:39:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddSLPVectorizePass , SimNode_ExtFuncCall >(lib,"LLVMAddSLPVectorizePass","LLVMAddSLPVectorizePass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


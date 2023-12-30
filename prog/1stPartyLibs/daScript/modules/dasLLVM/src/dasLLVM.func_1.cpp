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
void Module_dasLLVM::initFunctions_1() {
// from D:\Work\libclang\include\llvm-c/Analysis.h:44:10
	addExtern< int (*)(LLVMOpaqueModule *,LLVMVerifierFailureAction,char **) , LLVMVerifyModule >(*this,lib,"LLVMVerifyModule",SideEffects::worstDefault,"LLVMVerifyModule")
		->args({"M","Action","OutMessage"});
// from D:\Work\libclang\include\llvm-c/Analysis.h:49:10
	addExtern< int (*)(LLVMOpaqueValue *,LLVMVerifierFailureAction) , LLVMVerifyFunction >(*this,lib,"LLVMVerifyFunction",SideEffects::worstDefault,"LLVMVerifyFunction")
		->args({"Fn","Action"});
// from D:\Work\libclang\include\llvm-c/Analysis.h:53:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMViewFunctionCFG >(*this,lib,"LLVMViewFunctionCFG",SideEffects::worstDefault,"LLVMViewFunctionCFG")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Analysis.h:54:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMViewFunctionCFGOnly >(*this,lib,"LLVMViewFunctionCFGOnly",SideEffects::worstDefault,"LLVMViewFunctionCFGOnly")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/BitReader.h:39:10
	addExtern< int (*)(LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMParseBitcode >(*this,lib,"LLVMParseBitcode",SideEffects::worstDefault,"LLVMParseBitcode")
		->args({"MemBuf","OutModule","OutMessage"});
// from D:\Work\libclang\include\llvm-c/BitReader.h:44:10
	addExtern< int (*)(LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **) , LLVMParseBitcode2 >(*this,lib,"LLVMParseBitcode2",SideEffects::worstDefault,"LLVMParseBitcode2")
		->args({"MemBuf","OutModule"});
// from D:\Work\libclang\include\llvm-c/BitReader.h:48:10
	addExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMParseBitcodeInContext >(*this,lib,"LLVMParseBitcodeInContext",SideEffects::worstDefault,"LLVMParseBitcodeInContext")
		->args({"ContextRef","MemBuf","OutModule","OutMessage"});
// from D:\Work\libclang\include\llvm-c/BitReader.h:52:10
	addExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **) , LLVMParseBitcodeInContext2 >(*this,lib,"LLVMParseBitcodeInContext2",SideEffects::worstDefault,"LLVMParseBitcodeInContext2")
		->args({"ContextRef","MemBuf","OutModule"});
// from D:\Work\libclang\include\llvm-c/BitReader.h:60:10
	addExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMGetBitcodeModuleInContext >(*this,lib,"LLVMGetBitcodeModuleInContext",SideEffects::worstDefault,"LLVMGetBitcodeModuleInContext")
		->args({"ContextRef","MemBuf","OutM","OutMessage"});
// from D:\Work\libclang\include\llvm-c/BitReader.h:71:10
	addExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **) , LLVMGetBitcodeModuleInContext2 >(*this,lib,"LLVMGetBitcodeModuleInContext2",SideEffects::worstDefault,"LLVMGetBitcodeModuleInContext2")
		->args({"ContextRef","MemBuf","OutM"});
// from D:\Work\libclang\include\llvm-c/BitReader.h:76:10
	addExtern< int (*)(LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMGetBitcodeModule >(*this,lib,"LLVMGetBitcodeModule",SideEffects::worstDefault,"LLVMGetBitcodeModule")
		->args({"MemBuf","OutM","OutMessage"});
// from D:\Work\libclang\include\llvm-c/BitReader.h:79:10
	addExtern< int (*)(LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **) , LLVMGetBitcodeModule2 >(*this,lib,"LLVMGetBitcodeModule2",SideEffects::worstDefault,"LLVMGetBitcodeModule2")
		->args({"MemBuf","OutM"});
// from D:\Work\libclang\include\llvm-c/BitWriter.h:37:5
	addExtern< int (*)(LLVMOpaqueModule *,const char *) , LLVMWriteBitcodeToFile >(*this,lib,"LLVMWriteBitcodeToFile",SideEffects::worstDefault,"LLVMWriteBitcodeToFile")
		->args({"M","Path"});
// from D:\Work\libclang\include\llvm-c/BitWriter.h:40:5
	addExtern< int (*)(LLVMOpaqueModule *,int,int,int) , LLVMWriteBitcodeToFD >(*this,lib,"LLVMWriteBitcodeToFD",SideEffects::worstDefault,"LLVMWriteBitcodeToFD")
		->args({"M","FD","ShouldClose","Unbuffered"});
}
}


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
// from C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\ucrt\inttypes.h:45:27
	makeExtern< intmax_t (*)(intmax_t) , imaxabs , SimNode_ExtFuncCall >(lib,"imaxabs","imaxabs")
		->args({"_Number"})
		->addToModule(*this, SideEffects::worstDefault);
// from C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\ucrt\inttypes.h:50:28
	makeExtern< _Lldiv_t (*)(intmax_t,intmax_t) , imaxdiv , SimNode_ExtFuncCallAndCopyOrMove >(lib,"imaxdiv","imaxdiv")
		->args({"_Numerator","_Denominator"})
		->addToModule(*this, SideEffects::worstDefault);
// from C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\ucrt\inttypes.h:55:27
	makeExtern< intmax_t (*)(const char *,char **,int) , strtoimax , SimNode_ExtFuncCall >(lib,"strtoimax","strtoimax")
		->args({"_String","_EndPtr","_Radix"})
		->addToModule(*this, SideEffects::worstDefault);
// from C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\ucrt\inttypes.h:68:28
	makeExtern< uintmax_t (*)(const char *,char **,int) , strtoumax , SimNode_ExtFuncCall >(lib,"strtoumax","strtoumax")
		->args({"_String","_EndPtr","_Radix"})
		->addToModule(*this, SideEffects::worstDefault);
// from C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\ucrt\inttypes.h:81:27
	makeExtern< intmax_t (*)(const wchar_t *,wchar_t **,int) , wcstoimax , SimNode_ExtFuncCall >(lib,"wcstoimax","wcstoimax")
		->args({"_String","_EndPtr","_Radix"})
		->addToModule(*this, SideEffects::worstDefault);
// from C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\ucrt\inttypes.h:94:28
	makeExtern< uintmax_t (*)(const wchar_t *,wchar_t **,int) , wcstoumax , SimNode_ExtFuncCall >(lib,"wcstoumax","wcstoumax")
		->args({"_String","_EndPtr","_Radix"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Analysis.h:44:10
	makeExtern< int (*)(LLVMOpaqueModule *,LLVMVerifierFailureAction,char **) , LLVMVerifyModule , SimNode_ExtFuncCall >(lib,"LLVMVerifyModule","LLVMVerifyModule")
		->args({"M","Action","OutMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Analysis.h:49:10
	makeExtern< int (*)(LLVMOpaqueValue *,LLVMVerifierFailureAction) , LLVMVerifyFunction , SimNode_ExtFuncCall >(lib,"LLVMVerifyFunction","LLVMVerifyFunction")
		->args({"Fn","Action"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Analysis.h:53:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMViewFunctionCFG , SimNode_ExtFuncCall >(lib,"LLVMViewFunctionCFG","LLVMViewFunctionCFG")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Analysis.h:54:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMViewFunctionCFGOnly , SimNode_ExtFuncCall >(lib,"LLVMViewFunctionCFGOnly","LLVMViewFunctionCFGOnly")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitReader.h:39:10
	makeExtern< int (*)(LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMParseBitcode , SimNode_ExtFuncCall >(lib,"LLVMParseBitcode","LLVMParseBitcode")
		->args({"MemBuf","OutModule","OutMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitReader.h:44:10
	makeExtern< int (*)(LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **) , LLVMParseBitcode2 , SimNode_ExtFuncCall >(lib,"LLVMParseBitcode2","LLVMParseBitcode2")
		->args({"MemBuf","OutModule"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitReader.h:48:10
	makeExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMParseBitcodeInContext , SimNode_ExtFuncCall >(lib,"LLVMParseBitcodeInContext","LLVMParseBitcodeInContext")
		->args({"ContextRef","MemBuf","OutModule","OutMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitReader.h:52:10
	makeExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **) , LLVMParseBitcodeInContext2 , SimNode_ExtFuncCall >(lib,"LLVMParseBitcodeInContext2","LLVMParseBitcodeInContext2")
		->args({"ContextRef","MemBuf","OutModule"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitReader.h:60:10
	makeExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMGetBitcodeModuleInContext , SimNode_ExtFuncCall >(lib,"LLVMGetBitcodeModuleInContext","LLVMGetBitcodeModuleInContext")
		->args({"ContextRef","MemBuf","OutM","OutMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitReader.h:71:10
	makeExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **) , LLVMGetBitcodeModuleInContext2 , SimNode_ExtFuncCall >(lib,"LLVMGetBitcodeModuleInContext2","LLVMGetBitcodeModuleInContext2")
		->args({"ContextRef","MemBuf","OutM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitReader.h:76:10
	makeExtern< int (*)(LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMGetBitcodeModule , SimNode_ExtFuncCall >(lib,"LLVMGetBitcodeModule","LLVMGetBitcodeModule")
		->args({"MemBuf","OutM","OutMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitReader.h:79:10
	makeExtern< int (*)(LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **) , LLVMGetBitcodeModule2 , SimNode_ExtFuncCall >(lib,"LLVMGetBitcodeModule2","LLVMGetBitcodeModule2")
		->args({"MemBuf","OutM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitWriter.h:37:5
	makeExtern< int (*)(LLVMOpaqueModule *,const char *) , LLVMWriteBitcodeToFile , SimNode_ExtFuncCall >(lib,"LLVMWriteBitcodeToFile","LLVMWriteBitcodeToFile")
		->args({"M","Path"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitWriter.h:40:5
	makeExtern< int (*)(LLVMOpaqueModule *,int,int,int) , LLVMWriteBitcodeToFD , SimNode_ExtFuncCall >(lib,"LLVMWriteBitcodeToFD","LLVMWriteBitcodeToFD")
		->args({"M","FD","ShouldClose","Unbuffered"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


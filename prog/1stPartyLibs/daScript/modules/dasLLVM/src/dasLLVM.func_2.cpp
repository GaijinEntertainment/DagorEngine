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
void Module_dasLLVM::initFunctions_2() {
// from D:\Work\libclang\include\llvm-c/BitWriter.h:45:5
	makeExtern< int (*)(LLVMOpaqueModule *,int) , LLVMWriteBitcodeToFileHandle , SimNode_ExtFuncCall >(lib,"LLVMWriteBitcodeToFileHandle","LLVMWriteBitcodeToFileHandle")
		->args({"M","Handle"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/BitWriter.h:48:21
	makeExtern< LLVMOpaqueMemoryBuffer * (*)(LLVMOpaqueModule *) , LLVMWriteBitcodeToMemoryBuffer , SimNode_ExtFuncCall >(lib,"LLVMWriteBitcodeToMemoryBuffer","LLVMWriteBitcodeToMemoryBuffer")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Comdat.h:46:15
	makeExtern< LLVMComdat * (*)(LLVMOpaqueModule *,const char *) , LLVMGetOrInsertComdat , SimNode_ExtFuncCall >(lib,"LLVMGetOrInsertComdat","LLVMGetOrInsertComdat")
		->args({"M","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Comdat.h:53:15
	makeExtern< LLVMComdat * (*)(LLVMOpaqueValue *) , LLVMGetComdat , SimNode_ExtFuncCall >(lib,"LLVMGetComdat","LLVMGetComdat")
		->args({"V"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Comdat.h:60:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMComdat *) , LLVMSetComdat , SimNode_ExtFuncCall >(lib,"LLVMSetComdat","LLVMSetComdat")
		->args({"V","C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Comdat.h:67:25
	makeExtern< LLVMComdatSelectionKind (*)(LLVMComdat *) , LLVMGetComdatSelectionKind , SimNode_ExtFuncCall >(lib,"LLVMGetComdatSelectionKind","LLVMGetComdatSelectionKind")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Comdat.h:74:6
	makeExtern< void (*)(LLVMComdat *,LLVMComdatSelectionKind) , LLVMSetComdatSelectionKind , SimNode_ExtFuncCall >(lib,"LLVMSetComdatSelectionKind","LLVMSetComdatSelectionKind")
		->args({"C","Kind"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ErrorHandling.h:42:6
	makeExtern< void (*)() , LLVMResetFatalErrorHandler , SimNode_ExtFuncCall >(lib,"LLVMResetFatalErrorHandler","LLVMResetFatalErrorHandler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ErrorHandling.h:49:6
	makeExtern< void (*)() , LLVMEnablePrettyStackTrace , SimNode_ExtFuncCall >(lib,"LLVMEnablePrettyStackTrace","LLVMEnablePrettyStackTrace")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:482:6
	makeExtern< void (*)() , LLVMShutdown , SimNode_ExtFuncCall >(lib,"LLVMShutdown","LLVMShutdown")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:492:6
	makeExtern< void (*)(unsigned int *,unsigned int *,unsigned int *) , LLVMGetVersion , SimNode_ExtFuncCall >(lib,"LLVMGetVersion","LLVMGetVersion")
		->args({"Major","Minor","Patch"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:496:7
	makeExtern< char * (*)(const char *) , LLVMCreateMessage , SimNode_ExtFuncCall >(lib,"LLVMCreateMessage","LLVMCreateMessage")
		->args({"Message"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:497:6
	makeExtern< void (*)(char *) , LLVMDisposeMessage , SimNode_ExtFuncCall >(lib,"LLVMDisposeMessage","LLVMDisposeMessage")
		->args({"Message"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:520:16
	makeExtern< LLVMOpaqueContext * (*)() , LLVMContextCreate , SimNode_ExtFuncCall >(lib,"LLVMContextCreate","LLVMContextCreate")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:525:16
	makeExtern< LLVMOpaqueContext * (*)() , LLVMGetGlobalContext , SimNode_ExtFuncCall >(lib,"LLVMGetGlobalContext","LLVMGetGlobalContext")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:542:7
	makeExtern< void * (*)(LLVMOpaqueContext *) , LLVMContextGetDiagnosticContext , SimNode_ExtFuncCall >(lib,"LLVMContextGetDiagnosticContext","LLVMContextGetDiagnosticContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:557:10
	makeExtern< int (*)(LLVMOpaqueContext *) , LLVMContextShouldDiscardValueNames , SimNode_ExtFuncCall >(lib,"LLVMContextShouldDiscardValueNames","LLVMContextShouldDiscardValueNames")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:567:6
	makeExtern< void (*)(LLVMOpaqueContext *,int) , LLVMContextSetDiscardValueNames , SimNode_ExtFuncCall >(lib,"LLVMContextSetDiscardValueNames","LLVMContextSetDiscardValueNames")
		->args({"C","Discard"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:574:6
	makeExtern< void (*)(LLVMOpaqueContext *,int) , LLVMContextSetOpaquePointers , SimNode_ExtFuncCall >(lib,"LLVMContextSetOpaquePointers","LLVMContextSetOpaquePointers")
		->args({"C","OpaquePointers"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:582:6
	makeExtern< void (*)(LLVMOpaqueContext *) , LLVMContextDispose , SimNode_ExtFuncCall >(lib,"LLVMContextDispose","LLVMContextDispose")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


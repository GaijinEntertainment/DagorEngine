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
void Module_dasLLVM::initFunctions_51() {
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:78:10
	addExtern< int (*)(LLVMOpaqueExecutionEngine **,LLVMOpaqueModule *,char **) , LLVMCreateInterpreterForModule >(*this,lib,"LLVMCreateInterpreterForModule",SideEffects::worstDefault,"LLVMCreateInterpreterForModule")
		->args({"OutInterp","M","OutError"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:82:10
	addExtern< int (*)(LLVMOpaqueExecutionEngine **,LLVMOpaqueModule *,unsigned int,char **) , LLVMCreateJITCompilerForModule >(*this,lib,"LLVMCreateJITCompilerForModule",SideEffects::worstDefault,"LLVMCreateJITCompilerForModule")
		->args({"OutJIT","M","OptLevel","OutError"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:87:6
	addExtern< void (*)(LLVMMCJITCompilerOptions *,size_t) , LLVMInitializeMCJITCompilerOptions >(*this,lib,"LLVMInitializeMCJITCompilerOptions",SideEffects::worstDefault,"LLVMInitializeMCJITCompilerOptions")
		->args({"Options","SizeOfOptions"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:107:10
	addExtern< int (*)(LLVMOpaqueExecutionEngine **,LLVMOpaqueModule *,LLVMMCJITCompilerOptions *,size_t,char **) , LLVMCreateMCJITCompilerForModule >(*this,lib,"LLVMCreateMCJITCompilerForModule",SideEffects::worstDefault,"LLVMCreateMCJITCompilerForModule")
		->args({"OutJIT","M","Options","SizeOfOptions","OutError"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:112:6
	addExtern< void (*)(LLVMOpaqueExecutionEngine *) , LLVMDisposeExecutionEngine >(*this,lib,"LLVMDisposeExecutionEngine",SideEffects::worstDefault,"LLVMDisposeExecutionEngine")
		->args({"EE"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:114:6
	addExtern< void (*)(LLVMOpaqueExecutionEngine *) , LLVMRunStaticConstructors >(*this,lib,"LLVMRunStaticConstructors",SideEffects::worstDefault,"LLVMRunStaticConstructors")
		->args({"EE"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:116:6
	addExtern< void (*)(LLVMOpaqueExecutionEngine *) , LLVMRunStaticDestructors >(*this,lib,"LLVMRunStaticDestructors",SideEffects::worstDefault,"LLVMRunStaticDestructors")
		->args({"EE"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:118:5
	addExtern< int (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *,unsigned int,const char *const *,const char *const *) , LLVMRunFunctionAsMain >(*this,lib,"LLVMRunFunctionAsMain",SideEffects::worstDefault,"LLVMRunFunctionAsMain")
		->args({"EE","F","ArgC","ArgV","EnvP"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:122:21
	addExtern< LLVMOpaqueGenericValue * (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *,unsigned int,LLVMOpaqueGenericValue **) , LLVMRunFunction >(*this,lib,"LLVMRunFunction",SideEffects::worstDefault,"LLVMRunFunction")
		->args({"EE","F","NumArgs","Args"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:126:6
	addExtern< void (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *) , LLVMFreeMachineCodeForFunction >(*this,lib,"LLVMFreeMachineCodeForFunction",SideEffects::worstDefault,"LLVMFreeMachineCodeForFunction")
		->args({"EE","F"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:128:6
	addExtern< void (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueModule *) , LLVMAddModule >(*this,lib,"LLVMAddModule",SideEffects::worstDefault,"LLVMAddModule")
		->args({"EE","M"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:130:10
	addExtern< int (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueModule *,LLVMOpaqueModule **,char **) , LLVMRemoveModule >(*this,lib,"LLVMRemoveModule",SideEffects::worstDefault,"LLVMRemoveModule")
		->args({"EE","M","OutMod","OutError"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:133:10
	addExtern< int (*)(LLVMOpaqueExecutionEngine *,const char *,LLVMOpaqueValue **) , LLVMFindFunction >(*this,lib,"LLVMFindFunction",SideEffects::worstDefault,"LLVMFindFunction")
		->args({"EE","Name","OutFn"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:136:7
	addExtern< void * (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *) , LLVMRecompileAndRelinkFunction >(*this,lib,"LLVMRecompileAndRelinkFunction",SideEffects::worstDefault,"LLVMRecompileAndRelinkFunction")
		->args({"EE","Fn"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:139:19
	addExtern< LLVMOpaqueTargetData * (*)(LLVMOpaqueExecutionEngine *) , LLVMGetExecutionEngineTargetData >(*this,lib,"LLVMGetExecutionEngineTargetData",SideEffects::worstDefault,"LLVMGetExecutionEngineTargetData")
		->args({"EE"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:141:1
	addExtern< LLVMOpaqueTargetMachine * (*)(LLVMOpaqueExecutionEngine *) , LLVMGetExecutionEngineTargetMachine >(*this,lib,"LLVMGetExecutionEngineTargetMachine",SideEffects::worstDefault,"LLVMGetExecutionEngineTargetMachine")
		->args({"EE"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:143:6
	addExtern< void (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *,void *) , LLVMAddGlobalMapping >(*this,lib,"LLVMAddGlobalMapping",SideEffects::worstDefault,"LLVMAddGlobalMapping")
		->args({"EE","Global","Addr"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:146:7
	addExtern< void * (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *) , LLVMGetPointerToGlobal >(*this,lib,"LLVMGetPointerToGlobal",SideEffects::worstDefault,"LLVMGetPointerToGlobal")
		->args({"EE","Global"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:148:10
	addExtern< uint64_t (*)(LLVMOpaqueExecutionEngine *,const char *) , LLVMGetGlobalValueAddress >(*this,lib,"LLVMGetGlobalValueAddress",SideEffects::worstDefault,"LLVMGetGlobalValueAddress")
		->args({"EE","Name"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:150:10
	addExtern< uint64_t (*)(LLVMOpaqueExecutionEngine *,const char *) , LLVMGetFunctionAddress >(*this,lib,"LLVMGetFunctionAddress",SideEffects::worstDefault,"LLVMGetFunctionAddress")
		->args({"EE","Name"});
}
}


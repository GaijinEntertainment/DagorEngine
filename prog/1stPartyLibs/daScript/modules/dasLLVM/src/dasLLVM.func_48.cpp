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
void Module_dasLLVM::initFunctions_48() {
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:87:6
	makeExtern< void (*)(LLVMMCJITCompilerOptions *,size_t) , LLVMInitializeMCJITCompilerOptions , SimNode_ExtFuncCall >(lib,"LLVMInitializeMCJITCompilerOptions","LLVMInitializeMCJITCompilerOptions")
		->args({"Options","SizeOfOptions"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:107:10
	makeExtern< int (*)(LLVMOpaqueExecutionEngine **,LLVMOpaqueModule *,LLVMMCJITCompilerOptions *,size_t,char **) , LLVMCreateMCJITCompilerForModule , SimNode_ExtFuncCall >(lib,"LLVMCreateMCJITCompilerForModule","LLVMCreateMCJITCompilerForModule")
		->args({"OutJIT","M","Options","SizeOfOptions","OutError"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:112:6
	makeExtern< void (*)(LLVMOpaqueExecutionEngine *) , LLVMDisposeExecutionEngine , SimNode_ExtFuncCall >(lib,"LLVMDisposeExecutionEngine","LLVMDisposeExecutionEngine")
		->args({"EE"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:114:6
	makeExtern< void (*)(LLVMOpaqueExecutionEngine *) , LLVMRunStaticConstructors , SimNode_ExtFuncCall >(lib,"LLVMRunStaticConstructors","LLVMRunStaticConstructors")
		->args({"EE"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:116:6
	makeExtern< void (*)(LLVMOpaqueExecutionEngine *) , LLVMRunStaticDestructors , SimNode_ExtFuncCall >(lib,"LLVMRunStaticDestructors","LLVMRunStaticDestructors")
		->args({"EE"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:118:5
	makeExtern< int (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *,unsigned int,const char *const *,const char *const *) , LLVMRunFunctionAsMain , SimNode_ExtFuncCall >(lib,"LLVMRunFunctionAsMain","LLVMRunFunctionAsMain")
		->args({"EE","F","ArgC","ArgV","EnvP"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:122:21
	makeExtern< LLVMOpaqueGenericValue * (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *,unsigned int,LLVMOpaqueGenericValue **) , LLVMRunFunction , SimNode_ExtFuncCall >(lib,"LLVMRunFunction","LLVMRunFunction")
		->args({"EE","F","NumArgs","Args"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:126:6
	makeExtern< void (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *) , LLVMFreeMachineCodeForFunction , SimNode_ExtFuncCall >(lib,"LLVMFreeMachineCodeForFunction","LLVMFreeMachineCodeForFunction")
		->args({"EE","F"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:128:6
	makeExtern< void (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueModule *) , LLVMAddModule , SimNode_ExtFuncCall >(lib,"LLVMAddModule","LLVMAddModule")
		->args({"EE","M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:130:10
	makeExtern< int (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueModule *,LLVMOpaqueModule **,char **) , LLVMRemoveModule , SimNode_ExtFuncCall >(lib,"LLVMRemoveModule","LLVMRemoveModule")
		->args({"EE","M","OutMod","OutError"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:133:10
	makeExtern< int (*)(LLVMOpaqueExecutionEngine *,const char *,LLVMOpaqueValue **) , LLVMFindFunction , SimNode_ExtFuncCall >(lib,"LLVMFindFunction","LLVMFindFunction")
		->args({"EE","Name","OutFn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:136:7
	makeExtern< void * (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *) , LLVMRecompileAndRelinkFunction , SimNode_ExtFuncCall >(lib,"LLVMRecompileAndRelinkFunction","LLVMRecompileAndRelinkFunction")
		->args({"EE","Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:139:19
	makeExtern< LLVMOpaqueTargetData * (*)(LLVMOpaqueExecutionEngine *) , LLVMGetExecutionEngineTargetData , SimNode_ExtFuncCall >(lib,"LLVMGetExecutionEngineTargetData","LLVMGetExecutionEngineTargetData")
		->args({"EE"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:141:1
	makeExtern< LLVMOpaqueTargetMachine * (*)(LLVMOpaqueExecutionEngine *) , LLVMGetExecutionEngineTargetMachine , SimNode_ExtFuncCall >(lib,"LLVMGetExecutionEngineTargetMachine","LLVMGetExecutionEngineTargetMachine")
		->args({"EE"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:143:6
	makeExtern< void (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *,void *) , LLVMAddGlobalMapping , SimNode_ExtFuncCall >(lib,"LLVMAddGlobalMapping","LLVMAddGlobalMapping")
		->args({"EE","Global","Addr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:146:7
	makeExtern< void * (*)(LLVMOpaqueExecutionEngine *,LLVMOpaqueValue *) , LLVMGetPointerToGlobal , SimNode_ExtFuncCall >(lib,"LLVMGetPointerToGlobal","LLVMGetPointerToGlobal")
		->args({"EE","Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:148:10
	makeExtern< uint64_t (*)(LLVMOpaqueExecutionEngine *,const char *) , LLVMGetGlobalValueAddress , SimNode_ExtFuncCall >(lib,"LLVMGetGlobalValueAddress","LLVMGetGlobalValueAddress")
		->args({"EE","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:150:10
	makeExtern< uint64_t (*)(LLVMOpaqueExecutionEngine *,const char *) , LLVMGetFunctionAddress , SimNode_ExtFuncCall >(lib,"LLVMGetFunctionAddress","LLVMGetFunctionAddress")
		->args({"EE","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:154:10
	makeExtern< int (*)(LLVMOpaqueExecutionEngine *,char **) , LLVMExecutionEngineGetErrMsg , SimNode_ExtFuncCall >(lib,"LLVMExecutionEngineGetErrMsg","LLVMExecutionEngineGetErrMsg")
		->args({"EE","OutError"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:187:6
	makeExtern< void (*)(LLVMOpaqueMCJITMemoryManager *) , LLVMDisposeMCJITMemoryManager , SimNode_ExtFuncCall >(lib,"LLVMDisposeMCJITMemoryManager","LLVMDisposeMCJITMemoryManager")
		->args({"MM"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


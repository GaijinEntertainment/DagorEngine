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
void Module_dasLLVM::initFunctions_47() {
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:138:10
	makeExtern< int (*)(LLVMOpaqueTargetMachine *,LLVMOpaqueModule *,const char *,LLVMCodeGenFileType,char **) , LLVMTargetMachineEmitToFile , SimNode_ExtFuncCall >(lib,"LLVMTargetMachineEmitToFile","LLVMTargetMachineEmitToFile")
		->args({"T","M","Filename","codegen","ErrorMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:144:10
	makeExtern< int (*)(LLVMOpaqueTargetMachine *,LLVMOpaqueModule *,LLVMCodeGenFileType,char **,LLVMOpaqueMemoryBuffer **) , LLVMTargetMachineEmitToMemoryBuffer , SimNode_ExtFuncCall >(lib,"LLVMTargetMachineEmitToMemoryBuffer","LLVMTargetMachineEmitToMemoryBuffer")
		->args({"T","M","codegen","ErrorMessage","OutMemBuf"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:150:7
	makeExtern< char * (*)() , LLVMGetDefaultTargetTriple , SimNode_ExtFuncCall >(lib,"LLVMGetDefaultTargetTriple","LLVMGetDefaultTargetTriple")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:154:7
	makeExtern< char * (*)(const char *) , LLVMNormalizeTargetTriple , SimNode_ExtFuncCall >(lib,"LLVMNormalizeTargetTriple","LLVMNormalizeTargetTriple")
		->args({"triple"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:158:7
	makeExtern< char * (*)() , LLVMGetHostCPUName , SimNode_ExtFuncCall >(lib,"LLVMGetHostCPUName","LLVMGetHostCPUName")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:162:7
	makeExtern< char * (*)() , LLVMGetHostCPUFeatures , SimNode_ExtFuncCall >(lib,"LLVMGetHostCPUFeatures","LLVMGetHostCPUFeatures")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:165:6
	makeExtern< void (*)(LLVMOpaqueTargetMachine *,LLVMOpaquePassManager *) , LLVMAddAnalysisPasses , SimNode_ExtFuncCall >(lib,"LLVMAddAnalysisPasses","LLVMAddAnalysisPasses")
		->args({"T","PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:36:6
	makeExtern< void (*)() , LLVMLinkInMCJIT , SimNode_ExtFuncCall >(lib,"LLVMLinkInMCJIT","LLVMLinkInMCJIT")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:37:6
	makeExtern< void (*)() , LLVMLinkInInterpreter , SimNode_ExtFuncCall >(lib,"LLVMLinkInInterpreter","LLVMLinkInInterpreter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:53:21
	makeExtern< LLVMOpaqueGenericValue * (*)(LLVMOpaqueType *,unsigned long long,int) , LLVMCreateGenericValueOfInt , SimNode_ExtFuncCall >(lib,"LLVMCreateGenericValueOfInt","LLVMCreateGenericValueOfInt")
		->args({"Ty","N","IsSigned"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:57:21
	makeExtern< LLVMOpaqueGenericValue * (*)(void *) , LLVMCreateGenericValueOfPointer , SimNode_ExtFuncCall >(lib,"LLVMCreateGenericValueOfPointer","LLVMCreateGenericValueOfPointer")
		->args({"P"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:59:21
	makeExtern< LLVMOpaqueGenericValue * (*)(LLVMOpaqueType *,double) , LLVMCreateGenericValueOfFloat , SimNode_ExtFuncCall >(lib,"LLVMCreateGenericValueOfFloat","LLVMCreateGenericValueOfFloat")
		->args({"Ty","N"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:61:10
	makeExtern< unsigned int (*)(LLVMOpaqueGenericValue *) , LLVMGenericValueIntWidth , SimNode_ExtFuncCall >(lib,"LLVMGenericValueIntWidth","LLVMGenericValueIntWidth")
		->args({"GenValRef"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:63:20
	makeExtern< unsigned long long (*)(LLVMOpaqueGenericValue *,int) , LLVMGenericValueToInt , SimNode_ExtFuncCall >(lib,"LLVMGenericValueToInt","LLVMGenericValueToInt")
		->args({"GenVal","IsSigned"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:66:7
	makeExtern< void * (*)(LLVMOpaqueGenericValue *) , LLVMGenericValueToPointer , SimNode_ExtFuncCall >(lib,"LLVMGenericValueToPointer","LLVMGenericValueToPointer")
		->args({"GenVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:68:8
	makeExtern< double (*)(LLVMOpaqueType *,LLVMOpaqueGenericValue *) , LLVMGenericValueToFloat , SimNode_ExtFuncCall >(lib,"LLVMGenericValueToFloat","LLVMGenericValueToFloat")
		->args({"TyRef","GenVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:70:6
	makeExtern< void (*)(LLVMOpaqueGenericValue *) , LLVMDisposeGenericValue , SimNode_ExtFuncCall >(lib,"LLVMDisposeGenericValue","LLVMDisposeGenericValue")
		->args({"GenVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:74:10
	makeExtern< int (*)(LLVMOpaqueExecutionEngine **,LLVMOpaqueModule *,char **) , LLVMCreateExecutionEngineForModule , SimNode_ExtFuncCall >(lib,"LLVMCreateExecutionEngineForModule","LLVMCreateExecutionEngineForModule")
		->args({"OutEE","M","OutError"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:78:10
	makeExtern< int (*)(LLVMOpaqueExecutionEngine **,LLVMOpaqueModule *,char **) , LLVMCreateInterpreterForModule , SimNode_ExtFuncCall >(lib,"LLVMCreateInterpreterForModule","LLVMCreateInterpreterForModule")
		->args({"OutInterp","M","OutError"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:82:10
	makeExtern< int (*)(LLVMOpaqueExecutionEngine **,LLVMOpaqueModule *,unsigned int,char **) , LLVMCreateJITCompilerForModule , SimNode_ExtFuncCall >(lib,"LLVMCreateJITCompilerForModule","LLVMCreateJITCompilerForModule")
		->args({"OutJIT","M","OptLevel","OutError"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


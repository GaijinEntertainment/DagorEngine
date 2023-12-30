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
void Module_dasLLVM::initFunctions_50() {
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:129:19
	addExtern< LLVMOpaqueTargetData * (*)(LLVMOpaqueTargetMachine *) , LLVMCreateTargetDataLayout >(*this,lib,"LLVMCreateTargetDataLayout",SideEffects::worstDefault,"LLVMCreateTargetDataLayout")
		->args({"T"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:132:6
	addExtern< void (*)(LLVMOpaqueTargetMachine *,int) , LLVMSetTargetMachineAsmVerbosity >(*this,lib,"LLVMSetTargetMachineAsmVerbosity",SideEffects::worstDefault,"LLVMSetTargetMachineAsmVerbosity")
		->args({"T","VerboseAsm"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:138:10
	addExtern< int (*)(LLVMOpaqueTargetMachine *,LLVMOpaqueModule *,const char *,LLVMCodeGenFileType,char **) , LLVMTargetMachineEmitToFile >(*this,lib,"LLVMTargetMachineEmitToFile",SideEffects::worstDefault,"LLVMTargetMachineEmitToFile")
		->args({"T","M","Filename","codegen","ErrorMessage"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:144:10
	addExtern< int (*)(LLVMOpaqueTargetMachine *,LLVMOpaqueModule *,LLVMCodeGenFileType,char **,LLVMOpaqueMemoryBuffer **) , LLVMTargetMachineEmitToMemoryBuffer >(*this,lib,"LLVMTargetMachineEmitToMemoryBuffer",SideEffects::worstDefault,"LLVMTargetMachineEmitToMemoryBuffer")
		->args({"T","M","codegen","ErrorMessage","OutMemBuf"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:150:7
	addExtern< char * (*)() , LLVMGetDefaultTargetTriple >(*this,lib,"LLVMGetDefaultTargetTriple",SideEffects::worstDefault,"LLVMGetDefaultTargetTriple");
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:154:7
	addExtern< char * (*)(const char *) , LLVMNormalizeTargetTriple >(*this,lib,"LLVMNormalizeTargetTriple",SideEffects::worstDefault,"LLVMNormalizeTargetTriple")
		->args({"triple"});
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:158:7
	addExtern< char * (*)() , LLVMGetHostCPUName >(*this,lib,"LLVMGetHostCPUName",SideEffects::worstDefault,"LLVMGetHostCPUName");
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:162:7
	addExtern< char * (*)() , LLVMGetHostCPUFeatures >(*this,lib,"LLVMGetHostCPUFeatures",SideEffects::worstDefault,"LLVMGetHostCPUFeatures");
// from D:\Work\libclang\include\llvm-c/TargetMachine.h:165:6
	addExtern< void (*)(LLVMOpaqueTargetMachine *,LLVMOpaquePassManager *) , LLVMAddAnalysisPasses >(*this,lib,"LLVMAddAnalysisPasses",SideEffects::worstDefault,"LLVMAddAnalysisPasses")
		->args({"T","PM"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:36:6
	addExtern< void (*)() , LLVMLinkInMCJIT >(*this,lib,"LLVMLinkInMCJIT",SideEffects::worstDefault,"LLVMLinkInMCJIT");
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:37:6
	addExtern< void (*)() , LLVMLinkInInterpreter >(*this,lib,"LLVMLinkInInterpreter",SideEffects::worstDefault,"LLVMLinkInInterpreter");
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:53:21
	addExtern< LLVMOpaqueGenericValue * (*)(LLVMOpaqueType *,unsigned long long,int) , LLVMCreateGenericValueOfInt >(*this,lib,"LLVMCreateGenericValueOfInt",SideEffects::worstDefault,"LLVMCreateGenericValueOfInt")
		->args({"Ty","N","IsSigned"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:57:21
	addExtern< LLVMOpaqueGenericValue * (*)(void *) , LLVMCreateGenericValueOfPointer >(*this,lib,"LLVMCreateGenericValueOfPointer",SideEffects::worstDefault,"LLVMCreateGenericValueOfPointer")
		->args({"P"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:59:21
	addExtern< LLVMOpaqueGenericValue * (*)(LLVMOpaqueType *,double) , LLVMCreateGenericValueOfFloat >(*this,lib,"LLVMCreateGenericValueOfFloat",SideEffects::worstDefault,"LLVMCreateGenericValueOfFloat")
		->args({"Ty","N"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:61:10
	addExtern< unsigned int (*)(LLVMOpaqueGenericValue *) , LLVMGenericValueIntWidth >(*this,lib,"LLVMGenericValueIntWidth",SideEffects::worstDefault,"LLVMGenericValueIntWidth")
		->args({"GenValRef"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:63:20
	addExtern< unsigned long long (*)(LLVMOpaqueGenericValue *,int) , LLVMGenericValueToInt >(*this,lib,"LLVMGenericValueToInt",SideEffects::worstDefault,"LLVMGenericValueToInt")
		->args({"GenVal","IsSigned"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:66:7
	addExtern< void * (*)(LLVMOpaqueGenericValue *) , LLVMGenericValueToPointer >(*this,lib,"LLVMGenericValueToPointer",SideEffects::worstDefault,"LLVMGenericValueToPointer")
		->args({"GenVal"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:68:8
	addExtern< double (*)(LLVMOpaqueType *,LLVMOpaqueGenericValue *) , LLVMGenericValueToFloat >(*this,lib,"LLVMGenericValueToFloat",SideEffects::worstDefault,"LLVMGenericValueToFloat")
		->args({"TyRef","GenVal"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:70:6
	addExtern< void (*)(LLVMOpaqueGenericValue *) , LLVMDisposeGenericValue >(*this,lib,"LLVMDisposeGenericValue",SideEffects::worstDefault,"LLVMDisposeGenericValue")
		->args({"GenVal"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:74:10
	addExtern< int (*)(LLVMOpaqueExecutionEngine **,LLVMOpaqueModule *,char **) , LLVMCreateExecutionEngineForModule >(*this,lib,"LLVMCreateExecutionEngineForModule",SideEffects::worstDefault,"LLVMCreateExecutionEngineForModule")
		->args({"OutEE","M","OutError"});
}
}


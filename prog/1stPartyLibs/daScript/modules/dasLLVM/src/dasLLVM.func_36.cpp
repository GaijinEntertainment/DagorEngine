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
void Module_dasLLVM::initFunctions_36() {
// from D:\Work\libclang\include\llvm-c/Core.h:4098:1
	makeExtern< LLVMOpaqueModuleProvider * (*)(LLVMOpaqueModule *) , LLVMCreateModuleProviderForExistingModule , SimNode_ExtFuncCall >(lib,"LLVMCreateModuleProviderForExistingModule","LLVMCreateModuleProviderForExistingModule")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4103:6
	makeExtern< void (*)(LLVMOpaqueModuleProvider *) , LLVMDisposeModuleProvider , SimNode_ExtFuncCall >(lib,"LLVMDisposeModuleProvider","LLVMDisposeModuleProvider")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4115:10
	makeExtern< int (*)(const char *,LLVMOpaqueMemoryBuffer **,char **) , LLVMCreateMemoryBufferWithContentsOfFile , SimNode_ExtFuncCall >(lib,"LLVMCreateMemoryBufferWithContentsOfFile","LLVMCreateMemoryBufferWithContentsOfFile")
		->args({"Path","OutMemBuf","OutMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4118:10
	makeExtern< int (*)(LLVMOpaqueMemoryBuffer **,char **) , LLVMCreateMemoryBufferWithSTDIN , SimNode_ExtFuncCall >(lib,"LLVMCreateMemoryBufferWithSTDIN","LLVMCreateMemoryBufferWithSTDIN")
		->args({"OutMemBuf","OutMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4120:21
	makeExtern< LLVMOpaqueMemoryBuffer * (*)(const char *,size_t,const char *,int) , LLVMCreateMemoryBufferWithMemoryRange , SimNode_ExtFuncCall >(lib,"LLVMCreateMemoryBufferWithMemoryRange","LLVMCreateMemoryBufferWithMemoryRange")
		->args({"InputData","InputDataLength","BufferName","RequiresNullTerminator"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4124:21
	makeExtern< LLVMOpaqueMemoryBuffer * (*)(const char *,size_t,const char *) , LLVMCreateMemoryBufferWithMemoryRangeCopy , SimNode_ExtFuncCall >(lib,"LLVMCreateMemoryBufferWithMemoryRangeCopy","LLVMCreateMemoryBufferWithMemoryRangeCopy")
		->args({"InputData","InputDataLength","BufferName"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4127:13
	makeExtern< const char * (*)(LLVMOpaqueMemoryBuffer *) , LLVMGetBufferStart , SimNode_ExtFuncCall >(lib,"LLVMGetBufferStart","LLVMGetBufferStart")
		->args({"MemBuf"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4128:8
	makeExtern< size_t (*)(LLVMOpaqueMemoryBuffer *) , LLVMGetBufferSize , SimNode_ExtFuncCall >(lib,"LLVMGetBufferSize","LLVMGetBufferSize")
		->args({"MemBuf"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4129:6
	makeExtern< void (*)(LLVMOpaqueMemoryBuffer *) , LLVMDisposeMemoryBuffer , SimNode_ExtFuncCall >(lib,"LLVMDisposeMemoryBuffer","LLVMDisposeMemoryBuffer")
		->args({"MemBuf"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4144:21
	makeExtern< LLVMOpaquePassRegistry * (*)() , LLVMGetGlobalPassRegistry , SimNode_ExtFuncCall >(lib,"LLVMGetGlobalPassRegistry","LLVMGetGlobalPassRegistry")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4160:20
	makeExtern< LLVMOpaquePassManager * (*)() , LLVMCreatePassManager , SimNode_ExtFuncCall >(lib,"LLVMCreatePassManager","LLVMCreatePassManager")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4166:20
	makeExtern< LLVMOpaquePassManager * (*)(LLVMOpaqueModule *) , LLVMCreateFunctionPassManagerForModule , SimNode_ExtFuncCall >(lib,"LLVMCreateFunctionPassManagerForModule","LLVMCreateFunctionPassManagerForModule")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4169:20
	makeExtern< LLVMOpaquePassManager * (*)(LLVMOpaqueModuleProvider *) , LLVMCreateFunctionPassManager , SimNode_ExtFuncCall >(lib,"LLVMCreateFunctionPassManager","LLVMCreateFunctionPassManager")
		->args({"MP"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4175:10
	makeExtern< int (*)(LLVMOpaquePassManager *,LLVMOpaqueModule *) , LLVMRunPassManager , SimNode_ExtFuncCall >(lib,"LLVMRunPassManager","LLVMRunPassManager")
		->args({"PM","M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4180:10
	makeExtern< int (*)(LLVMOpaquePassManager *) , LLVMInitializeFunctionPassManager , SimNode_ExtFuncCall >(lib,"LLVMInitializeFunctionPassManager","LLVMInitializeFunctionPassManager")
		->args({"FPM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4186:10
	makeExtern< int (*)(LLVMOpaquePassManager *,LLVMOpaqueValue *) , LLVMRunFunctionPassManager , SimNode_ExtFuncCall >(lib,"LLVMRunFunctionPassManager","LLVMRunFunctionPassManager")
		->args({"FPM","F"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4191:10
	makeExtern< int (*)(LLVMOpaquePassManager *) , LLVMFinalizeFunctionPassManager , SimNode_ExtFuncCall >(lib,"LLVMFinalizeFunctionPassManager","LLVMFinalizeFunctionPassManager")
		->args({"FPM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4196:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMDisposePassManager , SimNode_ExtFuncCall >(lib,"LLVMDisposePassManager","LLVMDisposePassManager")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4213:10
	makeExtern< int (*)() , LLVMStartMultithreaded , SimNode_ExtFuncCall >(lib,"LLVMStartMultithreaded","LLVMStartMultithreaded")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4217:6
	makeExtern< void (*)() , LLVMStopMultithreaded , SimNode_ExtFuncCall >(lib,"LLVMStopMultithreaded","LLVMStopMultithreaded")
		->addToModule(*this, SideEffects::worstDefault);
}
}


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
// from D:\Work\libclang\include\llvm-c/Core.h:4084:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumMaskElements >(*this,lib,"LLVMGetNumMaskElements",SideEffects::worstDefault,"LLVMGetNumMaskElements")
		->args({"ShuffleVectorInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:4090:5
	addExtern< int (*)() , LLVMGetUndefMaskElem >(*this,lib,"LLVMGetUndefMaskElem",SideEffects::worstDefault,"LLVMGetUndefMaskElem");
// from D:\Work\libclang\include\llvm-c/Core.h:4099:5
	addExtern< int (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetMaskValue >(*this,lib,"LLVMGetMaskValue",SideEffects::worstDefault,"LLVMGetMaskValue")
		->args({"ShuffleVectorInst","Elt"});
// from D:\Work\libclang\include\llvm-c/Core.h:4101:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsAtomicSingleThread >(*this,lib,"LLVMIsAtomicSingleThread",SideEffects::worstDefault,"LLVMIsAtomicSingleThread")
		->args({"AtomicInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:4102:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetAtomicSingleThread >(*this,lib,"LLVMSetAtomicSingleThread",SideEffects::worstDefault,"LLVMSetAtomicSingleThread")
		->args({"AtomicInst","SingleThread"});
// from D:\Work\libclang\include\llvm-c/Core.h:4104:20
	addExtern< LLVMAtomicOrdering (*)(LLVMOpaqueValue *) , LLVMGetCmpXchgSuccessOrdering >(*this,lib,"LLVMGetCmpXchgSuccessOrdering",SideEffects::worstDefault,"LLVMGetCmpXchgSuccessOrdering")
		->args({"CmpXchgInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:4105:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMAtomicOrdering) , LLVMSetCmpXchgSuccessOrdering >(*this,lib,"LLVMSetCmpXchgSuccessOrdering",SideEffects::worstDefault,"LLVMSetCmpXchgSuccessOrdering")
		->args({"CmpXchgInst","Ordering"});
// from D:\Work\libclang\include\llvm-c/Core.h:4107:20
	addExtern< LLVMAtomicOrdering (*)(LLVMOpaqueValue *) , LLVMGetCmpXchgFailureOrdering >(*this,lib,"LLVMGetCmpXchgFailureOrdering",SideEffects::worstDefault,"LLVMGetCmpXchgFailureOrdering")
		->args({"CmpXchgInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:4108:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMAtomicOrdering) , LLVMSetCmpXchgFailureOrdering >(*this,lib,"LLVMSetCmpXchgFailureOrdering",SideEffects::worstDefault,"LLVMSetCmpXchgFailureOrdering")
		->args({"CmpXchgInst","Ordering"});
// from D:\Work\libclang\include\llvm-c/Core.h:4126:1
	addExtern< LLVMOpaqueModuleProvider * (*)(LLVMOpaqueModule *) , LLVMCreateModuleProviderForExistingModule >(*this,lib,"LLVMCreateModuleProviderForExistingModule",SideEffects::worstDefault,"LLVMCreateModuleProviderForExistingModule")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:4131:6
	addExtern< void (*)(LLVMOpaqueModuleProvider *) , LLVMDisposeModuleProvider >(*this,lib,"LLVMDisposeModuleProvider",SideEffects::worstDefault,"LLVMDisposeModuleProvider")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:4143:10
	addExtern< int (*)(const char *,LLVMOpaqueMemoryBuffer **,char **) , LLVMCreateMemoryBufferWithContentsOfFile >(*this,lib,"LLVMCreateMemoryBufferWithContentsOfFile",SideEffects::worstDefault,"LLVMCreateMemoryBufferWithContentsOfFile")
		->args({"Path","OutMemBuf","OutMessage"});
// from D:\Work\libclang\include\llvm-c/Core.h:4146:10
	addExtern< int (*)(LLVMOpaqueMemoryBuffer **,char **) , LLVMCreateMemoryBufferWithSTDIN >(*this,lib,"LLVMCreateMemoryBufferWithSTDIN",SideEffects::worstDefault,"LLVMCreateMemoryBufferWithSTDIN")
		->args({"OutMemBuf","OutMessage"});
// from D:\Work\libclang\include\llvm-c/Core.h:4148:21
	addExtern< LLVMOpaqueMemoryBuffer * (*)(const char *,size_t,const char *,int) , LLVMCreateMemoryBufferWithMemoryRange >(*this,lib,"LLVMCreateMemoryBufferWithMemoryRange",SideEffects::worstDefault,"LLVMCreateMemoryBufferWithMemoryRange")
		->args({"InputData","InputDataLength","BufferName","RequiresNullTerminator"});
// from D:\Work\libclang\include\llvm-c/Core.h:4152:21
	addExtern< LLVMOpaqueMemoryBuffer * (*)(const char *,size_t,const char *) , LLVMCreateMemoryBufferWithMemoryRangeCopy >(*this,lib,"LLVMCreateMemoryBufferWithMemoryRangeCopy",SideEffects::worstDefault,"LLVMCreateMemoryBufferWithMemoryRangeCopy")
		->args({"InputData","InputDataLength","BufferName"});
// from D:\Work\libclang\include\llvm-c/Core.h:4155:13
	addExtern< const char * (*)(LLVMOpaqueMemoryBuffer *) , LLVMGetBufferStart >(*this,lib,"LLVMGetBufferStart",SideEffects::worstDefault,"LLVMGetBufferStart")
		->args({"MemBuf"});
// from D:\Work\libclang\include\llvm-c/Core.h:4156:8
	addExtern< size_t (*)(LLVMOpaqueMemoryBuffer *) , LLVMGetBufferSize >(*this,lib,"LLVMGetBufferSize",SideEffects::worstDefault,"LLVMGetBufferSize")
		->args({"MemBuf"});
// from D:\Work\libclang\include\llvm-c/Core.h:4157:6
	addExtern< void (*)(LLVMOpaqueMemoryBuffer *) , LLVMDisposeMemoryBuffer >(*this,lib,"LLVMDisposeMemoryBuffer",SideEffects::worstDefault,"LLVMDisposeMemoryBuffer")
		->args({"MemBuf"});
// from D:\Work\libclang\include\llvm-c/Core.h:4172:21
	addExtern< LLVMOpaquePassRegistry * (*)() , LLVMGetGlobalPassRegistry >(*this,lib,"LLVMGetGlobalPassRegistry",SideEffects::worstDefault,"LLVMGetGlobalPassRegistry");
// from D:\Work\libclang\include\llvm-c/Core.h:4188:20
	addExtern< LLVMOpaquePassManager * (*)() , LLVMCreatePassManager >(*this,lib,"LLVMCreatePassManager",SideEffects::worstDefault,"LLVMCreatePassManager");
}
}


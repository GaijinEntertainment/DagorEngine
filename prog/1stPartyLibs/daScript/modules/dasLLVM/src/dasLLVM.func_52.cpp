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
void Module_dasLLVM::initFunctions_52() {
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:154:10
	addExtern< int (*)(LLVMOpaqueExecutionEngine *,char **) , LLVMExecutionEngineGetErrMsg >(*this,lib,"LLVMExecutionEngineGetErrMsg",SideEffects::worstDefault,"LLVMExecutionEngineGetErrMsg")
		->args({"EE","OutError"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:187:6
	addExtern< void (*)(LLVMOpaqueMCJITMemoryManager *) , LLVMDisposeMCJITMemoryManager >(*this,lib,"LLVMDisposeMCJITMemoryManager",SideEffects::worstDefault,"LLVMDisposeMCJITMemoryManager")
		->args({"MM"});
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:191:25
	addExtern< LLVMOpaqueJITEventListener * (*)() , LLVMCreateGDBRegistrationListener >(*this,lib,"LLVMCreateGDBRegistrationListener",SideEffects::worstDefault,"LLVMCreateGDBRegistrationListener");
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:192:25
	addExtern< LLVMOpaqueJITEventListener * (*)() , LLVMCreateIntelJITEventListener >(*this,lib,"LLVMCreateIntelJITEventListener",SideEffects::worstDefault,"LLVMCreateIntelJITEventListener");
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:193:25
	addExtern< LLVMOpaqueJITEventListener * (*)() , LLVMCreateOProfileJITEventListener >(*this,lib,"LLVMCreateOProfileJITEventListener",SideEffects::worstDefault,"LLVMCreateOProfileJITEventListener");
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:194:25
	addExtern< LLVMOpaqueJITEventListener * (*)() , LLVMCreatePerfJITEventListener >(*this,lib,"LLVMCreatePerfJITEventListener",SideEffects::worstDefault,"LLVMCreatePerfJITEventListener");
// from D:\Work\libclang\include\llvm-c/Initialization.h:34:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeTransformUtils >(*this,lib,"LLVMInitializeTransformUtils",SideEffects::worstDefault,"LLVMInitializeTransformUtils")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:35:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeScalarOpts >(*this,lib,"LLVMInitializeScalarOpts",SideEffects::worstDefault,"LLVMInitializeScalarOpts")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:36:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeObjCARCOpts >(*this,lib,"LLVMInitializeObjCARCOpts",SideEffects::worstDefault,"LLVMInitializeObjCARCOpts")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:37:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeVectorization >(*this,lib,"LLVMInitializeVectorization",SideEffects::worstDefault,"LLVMInitializeVectorization")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:38:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeInstCombine >(*this,lib,"LLVMInitializeInstCombine",SideEffects::worstDefault,"LLVMInitializeInstCombine")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:39:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeAggressiveInstCombiner >(*this,lib,"LLVMInitializeAggressiveInstCombiner",SideEffects::worstDefault,"LLVMInitializeAggressiveInstCombiner")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:40:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeIPO >(*this,lib,"LLVMInitializeIPO",SideEffects::worstDefault,"LLVMInitializeIPO")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:41:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeInstrumentation >(*this,lib,"LLVMInitializeInstrumentation",SideEffects::worstDefault,"LLVMInitializeInstrumentation")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:42:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeAnalysis >(*this,lib,"LLVMInitializeAnalysis",SideEffects::worstDefault,"LLVMInitializeAnalysis")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:43:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeIPA >(*this,lib,"LLVMInitializeIPA",SideEffects::worstDefault,"LLVMInitializeIPA")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:44:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeCodeGen >(*this,lib,"LLVMInitializeCodeGen",SideEffects::worstDefault,"LLVMInitializeCodeGen")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/Initialization.h:45:6
	addExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeTarget >(*this,lib,"LLVMInitializeTarget",SideEffects::worstDefault,"LLVMInitializeTarget")
		->args({"R"});
// from D:\Work\libclang\include\llvm-c/IRReader.h:38:10
	addExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMParseIRInContext >(*this,lib,"LLVMParseIRInContext",SideEffects::worstDefault,"LLVMParseIRInContext")
		->args({"ContextRef","MemBuf","OutM","OutMessage"});
// from D:\Work\libclang\include\llvm-c/Linker.h:41:10
	addExtern< int (*)(LLVMOpaqueModule *,LLVMOpaqueModule *) , LLVMLinkModules2 >(*this,lib,"LLVMLinkModules2",SideEffects::worstDefault,"LLVMLinkModules2")
		->args({"Dest","Src"});
}
}


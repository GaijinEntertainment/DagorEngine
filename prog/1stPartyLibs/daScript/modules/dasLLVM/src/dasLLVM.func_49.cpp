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
void Module_dasLLVM::initFunctions_49() {
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:191:25
	makeExtern< LLVMOpaqueJITEventListener * (*)() , LLVMCreateGDBRegistrationListener , SimNode_ExtFuncCall >(lib,"LLVMCreateGDBRegistrationListener","LLVMCreateGDBRegistrationListener")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:192:25
	makeExtern< LLVMOpaqueJITEventListener * (*)() , LLVMCreateIntelJITEventListener , SimNode_ExtFuncCall >(lib,"LLVMCreateIntelJITEventListener","LLVMCreateIntelJITEventListener")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:193:25
	makeExtern< LLVMOpaqueJITEventListener * (*)() , LLVMCreateOProfileJITEventListener , SimNode_ExtFuncCall >(lib,"LLVMCreateOProfileJITEventListener","LLVMCreateOProfileJITEventListener")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/ExecutionEngine.h:194:25
	makeExtern< LLVMOpaqueJITEventListener * (*)() , LLVMCreatePerfJITEventListener , SimNode_ExtFuncCall >(lib,"LLVMCreatePerfJITEventListener","LLVMCreatePerfJITEventListener")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Initialization.h:34:6
	makeExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeTransformUtils , SimNode_ExtFuncCall >(lib,"LLVMInitializeTransformUtils","LLVMInitializeTransformUtils")
		->args({"R"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Initialization.h:35:6
	makeExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeScalarOpts , SimNode_ExtFuncCall >(lib,"LLVMInitializeScalarOpts","LLVMInitializeScalarOpts")
		->args({"R"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Initialization.h:36:6
	makeExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeVectorization , SimNode_ExtFuncCall >(lib,"LLVMInitializeVectorization","LLVMInitializeVectorization")
		->args({"R"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Initialization.h:37:6
	makeExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeInstCombine , SimNode_ExtFuncCall >(lib,"LLVMInitializeInstCombine","LLVMInitializeInstCombine")
		->args({"R"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Initialization.h:38:6
	makeExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeIPO , SimNode_ExtFuncCall >(lib,"LLVMInitializeIPO","LLVMInitializeIPO")
		->args({"R"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Initialization.h:39:6
	makeExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeAnalysis , SimNode_ExtFuncCall >(lib,"LLVMInitializeAnalysis","LLVMInitializeAnalysis")
		->args({"R"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Initialization.h:40:6
	makeExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeIPA , SimNode_ExtFuncCall >(lib,"LLVMInitializeIPA","LLVMInitializeIPA")
		->args({"R"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Initialization.h:41:6
	makeExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeCodeGen , SimNode_ExtFuncCall >(lib,"LLVMInitializeCodeGen","LLVMInitializeCodeGen")
		->args({"R"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Initialization.h:42:6
	makeExtern< void (*)(LLVMOpaquePassRegistry *) , LLVMInitializeTarget , SimNode_ExtFuncCall >(lib,"LLVMInitializeTarget","LLVMInitializeTarget")
		->args({"R"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/IRReader.h:38:10
	makeExtern< int (*)(LLVMOpaqueContext *,LLVMOpaqueMemoryBuffer *,LLVMOpaqueModule **,char **) , LLVMParseIRInContext , SimNode_ExtFuncCall >(lib,"LLVMParseIRInContext","LLVMParseIRInContext")
		->args({"ContextRef","MemBuf","OutM","OutMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Linker.h:41:10
	makeExtern< int (*)(LLVMOpaqueModule *,LLVMOpaqueModule *) , LLVMLinkModules2 , SimNode_ExtFuncCall >(lib,"LLVMLinkModules2","LLVMLinkModules2")
		->args({"Dest","Src"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:493:1
	makeExtern< LLVMOrcOpaqueSymbolStringPool * (*)(LLVMOrcOpaqueExecutionSession *) , LLVMOrcExecutionSessionGetSymbolStringPool , SimNode_ExtFuncCall >(lib,"LLVMOrcExecutionSessionGetSymbolStringPool","LLVMOrcExecutionSessionGetSymbolStringPool")
		->args({"ES"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:505:6
	makeExtern< void (*)(LLVMOrcOpaqueSymbolStringPool *) , LLVMOrcSymbolStringPoolClearDeadEntries , SimNode_ExtFuncCall >(lib,"LLVMOrcSymbolStringPoolClearDeadEntries","LLVMOrcSymbolStringPoolClearDeadEntries")
		->args({"SSP"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:520:1
	makeExtern< LLVMOrcOpaqueSymbolStringPoolEntry * (*)(LLVMOrcOpaqueExecutionSession *,const char *) , LLVMOrcExecutionSessionIntern , SimNode_ExtFuncCall >(lib,"LLVMOrcExecutionSessionIntern","LLVMOrcExecutionSessionIntern")
		->args({"ES","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:577:6
	makeExtern< void (*)(LLVMOrcOpaqueSymbolStringPoolEntry *) , LLVMOrcRetainSymbolStringPoolEntry , SimNode_ExtFuncCall >(lib,"LLVMOrcRetainSymbolStringPoolEntry","LLVMOrcRetainSymbolStringPoolEntry")
		->args({"S"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:582:6
	makeExtern< void (*)(LLVMOrcOpaqueSymbolStringPoolEntry *) , LLVMOrcReleaseSymbolStringPoolEntry , SimNode_ExtFuncCall >(lib,"LLVMOrcReleaseSymbolStringPoolEntry","LLVMOrcReleaseSymbolStringPoolEntry")
		->args({"S"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


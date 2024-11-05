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
// from D:\Work\libclang\include\llvm-c/Orc.h:1144:1
	makeExtern< LLVMOrcOpaqueJITTargetMachineBuilder * (*)(LLVMOpaqueTargetMachine *) , LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine , SimNode_ExtFuncCall >(lib,"LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine","LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine")
		->args({"TM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1149:6
	makeExtern< void (*)(LLVMOrcOpaqueJITTargetMachineBuilder *) , LLVMOrcDisposeJITTargetMachineBuilder , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeJITTargetMachineBuilder","LLVMOrcDisposeJITTargetMachineBuilder")
		->args({"JTMB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1158:7
	makeExtern< char * (*)(LLVMOrcOpaqueJITTargetMachineBuilder *) , LLVMOrcJITTargetMachineBuilderGetTargetTriple , SimNode_ExtFuncCall >(lib,"LLVMOrcJITTargetMachineBuilderGetTargetTriple","LLVMOrcJITTargetMachineBuilderGetTargetTriple")
		->args({"JTMB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1165:6
	makeExtern< void (*)(LLVMOrcOpaqueJITTargetMachineBuilder *,const char *) , LLVMOrcJITTargetMachineBuilderSetTargetTriple , SimNode_ExtFuncCall >(lib,"LLVMOrcJITTargetMachineBuilderSetTargetTriple","LLVMOrcJITTargetMachineBuilderSetTargetTriple")
		->args({"JTMB","TargetTriple"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1179:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueObjectLayer *,LLVMOrcOpaqueJITDylib *,LLVMOpaqueMemoryBuffer *) , LLVMOrcObjectLayerAddObjectFile , SimNode_ExtFuncCall >(lib,"LLVMOrcObjectLayerAddObjectFile","LLVMOrcObjectLayerAddObjectFile")
		->args({"ObjLayer","JD","ObjBuffer"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1205:6
	makeExtern< void (*)(LLVMOrcOpaqueObjectLayer *,LLVMOrcOpaqueMaterializationResponsibility *,LLVMOpaqueMemoryBuffer *) , LLVMOrcObjectLayerEmit , SimNode_ExtFuncCall >(lib,"LLVMOrcObjectLayerEmit","LLVMOrcObjectLayerEmit")
		->args({"ObjLayer","R","ObjBuffer"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1212:6
	makeExtern< void (*)(LLVMOrcOpaqueObjectLayer *) , LLVMOrcDisposeObjectLayer , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeObjectLayer","LLVMOrcDisposeObjectLayer")
		->args({"ObjLayer"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1214:6
	makeExtern< void (*)(LLVMOrcOpaqueIRTransformLayer *,LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcOpaqueThreadSafeModule *) , LLVMOrcIRTransformLayerEmit , SimNode_ExtFuncCall >(lib,"LLVMOrcIRTransformLayerEmit","LLVMOrcIRTransformLayerEmit")
		->args({"IRTransformLayer","MR","TSM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1240:1
	makeExtern< LLVMOrcOpaqueIndirectStubsManager * (*)(const char *) , LLVMOrcCreateLocalIndirectStubsManager , SimNode_ExtFuncCall >(lib,"LLVMOrcCreateLocalIndirectStubsManager","LLVMOrcCreateLocalIndirectStubsManager")
		->args({"TargetTriple"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1245:6
	makeExtern< void (*)(LLVMOrcOpaqueIndirectStubsManager *) , LLVMOrcDisposeIndirectStubsManager , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeIndirectStubsManager","LLVMOrcDisposeIndirectStubsManager")
		->args({"ISM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1247:14
	makeExtern< LLVMOpaqueError * (*)(const char *,LLVMOrcOpaqueExecutionSession *,LLVMOrcJITTargetAddress,LLVMOrcOpaqueLazyCallThroughManager **) , LLVMOrcCreateLocalLazyCallThroughManager , SimNode_ExtFuncCall >(lib,"LLVMOrcCreateLocalLazyCallThroughManager","LLVMOrcCreateLocalLazyCallThroughManager")
		->args({"TargetTriple","ES","ErrorHandlerAddr","LCTM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1255:6
	makeExtern< void (*)(LLVMOrcOpaqueLazyCallThroughManager *) , LLVMOrcDisposeLazyCallThroughManager , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeLazyCallThroughManager","LLVMOrcDisposeLazyCallThroughManager")
		->args({"LCTM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1272:23
	makeExtern< LLVMOrcOpaqueDumpObjects * (*)(const char *,const char *) , LLVMOrcCreateDumpObjects , SimNode_ExtFuncCall >(lib,"LLVMOrcCreateDumpObjects","LLVMOrcCreateDumpObjects")
		->args({"DumpDir","IdentifierOverride"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1278:6
	makeExtern< void (*)(LLVMOrcOpaqueDumpObjects *) , LLVMOrcDisposeDumpObjects , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeDumpObjects","LLVMOrcDisposeDumpObjects")
		->args({"DumpObjects"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1283:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueDumpObjects *,LLVMOpaqueMemoryBuffer **) , LLVMOrcDumpObjects_CallOperator , SimNode_ExtFuncCall >(lib,"LLVMOrcDumpObjects_CallOperator","LLVMOrcDumpObjects_CallOperator")
		->args({"DumpObjects","ObjBuffer"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:74:24
	makeExtern< LLVMOrcOpaqueLLJITBuilder * (*)() , LLVMOrcCreateLLJITBuilder , SimNode_ExtFuncCall >(lib,"LLVMOrcCreateLLJITBuilder","LLVMOrcCreateLLJITBuilder")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:81:6
	makeExtern< void (*)(LLVMOrcOpaqueLLJITBuilder *) , LLVMOrcDisposeLLJITBuilder , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeLLJITBuilder","LLVMOrcDisposeLLJITBuilder")
		->args({"Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:92:6
	makeExtern< void (*)(LLVMOrcOpaqueLLJITBuilder *,LLVMOrcOpaqueJITTargetMachineBuilder *) , LLVMOrcLLJITBuilderSetJITTargetMachineBuilder , SimNode_ExtFuncCall >(lib,"LLVMOrcLLJITBuilderSetJITTargetMachineBuilder","LLVMOrcLLJITBuilderSetJITTargetMachineBuilder")
		->args({"Builder","JTMB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:116:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueLLJIT **,LLVMOrcOpaqueLLJITBuilder *) , LLVMOrcCreateLLJIT , SimNode_ExtFuncCall >(lib,"LLVMOrcCreateLLJIT","LLVMOrcCreateLLJIT")
		->args({"Result","Builder"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/LLJIT.h:122:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueLLJIT *) , LLVMOrcDisposeLLJIT , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeLLJIT","LLVMOrcDisposeLLJIT")
		->args({"J"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


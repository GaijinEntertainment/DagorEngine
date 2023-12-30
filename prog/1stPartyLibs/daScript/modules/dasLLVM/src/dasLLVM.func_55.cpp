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
void Module_dasLLVM::initFunctions_55() {
// from D:\Work\libclang\include\llvm-c/Orc.h:1087:1
	addExtern< LLVMOpaqueContext * (*)(LLVMOrcOpaqueThreadSafeContext *) , LLVMOrcThreadSafeContextGetContext >(*this,lib,"LLVMOrcThreadSafeContextGetContext",SideEffects::worstDefault,"LLVMOrcThreadSafeContextGetContext")
		->args({"TSCtx"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1092:6
	addExtern< void (*)(LLVMOrcOpaqueThreadSafeContext *) , LLVMOrcDisposeThreadSafeContext >(*this,lib,"LLVMOrcDisposeThreadSafeContext",SideEffects::worstDefault,"LLVMOrcDisposeThreadSafeContext")
		->args({"TSCtx"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1105:1
	addExtern< LLVMOrcOpaqueThreadSafeModule * (*)(LLVMOpaqueModule *,LLVMOrcOpaqueThreadSafeContext *) , LLVMOrcCreateNewThreadSafeModule >(*this,lib,"LLVMOrcCreateNewThreadSafeModule",SideEffects::worstDefault,"LLVMOrcCreateNewThreadSafeModule")
		->args({"M","TSCtx"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1113:6
	addExtern< void (*)(LLVMOrcOpaqueThreadSafeModule *) , LLVMOrcDisposeThreadSafeModule >(*this,lib,"LLVMOrcDisposeThreadSafeModule",SideEffects::worstDefault,"LLVMOrcDisposeThreadSafeModule")
		->args({"TSM"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1131:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueJITTargetMachineBuilder **) , LLVMOrcJITTargetMachineBuilderDetectHost >(*this,lib,"LLVMOrcJITTargetMachineBuilderDetectHost",SideEffects::worstDefault,"LLVMOrcJITTargetMachineBuilderDetectHost")
		->args({"Result"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1144:1
	addExtern< LLVMOrcOpaqueJITTargetMachineBuilder * (*)(LLVMOpaqueTargetMachine *) , LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine >(*this,lib,"LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine",SideEffects::worstDefault,"LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine")
		->args({"TM"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1149:6
	addExtern< void (*)(LLVMOrcOpaqueJITTargetMachineBuilder *) , LLVMOrcDisposeJITTargetMachineBuilder >(*this,lib,"LLVMOrcDisposeJITTargetMachineBuilder",SideEffects::worstDefault,"LLVMOrcDisposeJITTargetMachineBuilder")
		->args({"JTMB"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1158:7
	addExtern< char * (*)(LLVMOrcOpaqueJITTargetMachineBuilder *) , LLVMOrcJITTargetMachineBuilderGetTargetTriple >(*this,lib,"LLVMOrcJITTargetMachineBuilderGetTargetTriple",SideEffects::worstDefault,"LLVMOrcJITTargetMachineBuilderGetTargetTriple")
		->args({"JTMB"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1165:6
	addExtern< void (*)(LLVMOrcOpaqueJITTargetMachineBuilder *,const char *) , LLVMOrcJITTargetMachineBuilderSetTargetTriple >(*this,lib,"LLVMOrcJITTargetMachineBuilderSetTargetTriple",SideEffects::worstDefault,"LLVMOrcJITTargetMachineBuilderSetTargetTriple")
		->args({"JTMB","TargetTriple"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1179:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueObjectLayer *,LLVMOrcOpaqueJITDylib *,LLVMOpaqueMemoryBuffer *) , LLVMOrcObjectLayerAddObjectFile >(*this,lib,"LLVMOrcObjectLayerAddObjectFile",SideEffects::worstDefault,"LLVMOrcObjectLayerAddObjectFile")
		->args({"ObjLayer","JD","ObjBuffer"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1205:6
	addExtern< void (*)(LLVMOrcOpaqueObjectLayer *,LLVMOrcOpaqueMaterializationResponsibility *,LLVMOpaqueMemoryBuffer *) , LLVMOrcObjectLayerEmit >(*this,lib,"LLVMOrcObjectLayerEmit",SideEffects::worstDefault,"LLVMOrcObjectLayerEmit")
		->args({"ObjLayer","R","ObjBuffer"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1212:6
	addExtern< void (*)(LLVMOrcOpaqueObjectLayer *) , LLVMOrcDisposeObjectLayer >(*this,lib,"LLVMOrcDisposeObjectLayer",SideEffects::worstDefault,"LLVMOrcDisposeObjectLayer")
		->args({"ObjLayer"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1214:6
	addExtern< void (*)(LLVMOrcOpaqueIRTransformLayer *,LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcOpaqueThreadSafeModule *) , LLVMOrcIRTransformLayerEmit >(*this,lib,"LLVMOrcIRTransformLayerEmit",SideEffects::worstDefault,"LLVMOrcIRTransformLayerEmit")
		->args({"IRTransformLayer","MR","TSM"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1240:1
	addExtern< LLVMOrcOpaqueIndirectStubsManager * (*)(const char *) , LLVMOrcCreateLocalIndirectStubsManager >(*this,lib,"LLVMOrcCreateLocalIndirectStubsManager",SideEffects::worstDefault,"LLVMOrcCreateLocalIndirectStubsManager")
		->args({"TargetTriple"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1245:6
	addExtern< void (*)(LLVMOrcOpaqueIndirectStubsManager *) , LLVMOrcDisposeIndirectStubsManager >(*this,lib,"LLVMOrcDisposeIndirectStubsManager",SideEffects::worstDefault,"LLVMOrcDisposeIndirectStubsManager")
		->args({"ISM"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1247:14
	addExtern< LLVMOpaqueError * (*)(const char *,LLVMOrcOpaqueExecutionSession *,LLVMOrcJITTargetAddress,LLVMOrcOpaqueLazyCallThroughManager **) , LLVMOrcCreateLocalLazyCallThroughManager >(*this,lib,"LLVMOrcCreateLocalLazyCallThroughManager",SideEffects::worstDefault,"LLVMOrcCreateLocalLazyCallThroughManager")
		->args({"TargetTriple","ES","ErrorHandlerAddr","LCTM"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1255:6
	addExtern< void (*)(LLVMOrcOpaqueLazyCallThroughManager *) , LLVMOrcDisposeLazyCallThroughManager >(*this,lib,"LLVMOrcDisposeLazyCallThroughManager",SideEffects::worstDefault,"LLVMOrcDisposeLazyCallThroughManager")
		->args({"LCTM"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1272:23
	addExtern< LLVMOrcOpaqueDumpObjects * (*)(const char *,const char *) , LLVMOrcCreateDumpObjects >(*this,lib,"LLVMOrcCreateDumpObjects",SideEffects::worstDefault,"LLVMOrcCreateDumpObjects")
		->args({"DumpDir","IdentifierOverride"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1278:6
	addExtern< void (*)(LLVMOrcOpaqueDumpObjects *) , LLVMOrcDisposeDumpObjects >(*this,lib,"LLVMOrcDisposeDumpObjects",SideEffects::worstDefault,"LLVMOrcDisposeDumpObjects")
		->args({"DumpObjects"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1283:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueDumpObjects *,LLVMOpaqueMemoryBuffer **) , LLVMOrcDumpObjects_CallOperator >(*this,lib,"LLVMOrcDumpObjects_CallOperator",SideEffects::worstDefault,"LLVMOrcDumpObjects_CallOperator")
		->args({"DumpObjects","ObjBuffer"});
}
}


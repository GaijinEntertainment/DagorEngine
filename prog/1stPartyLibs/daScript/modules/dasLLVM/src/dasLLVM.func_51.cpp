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
// from D:\Work\libclang\include\llvm-c/Orc.h:856:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcOpaqueMaterializationUnit *) , LLVMOrcMaterializationResponsibilityReplace , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityReplace","LLVMOrcMaterializationResponsibilityReplace")
		->args({"MR","MU"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:868:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcOpaqueSymbolStringPoolEntry **,size_t,LLVMOrcOpaqueMaterializationResponsibility **) , LLVMOrcMaterializationResponsibilityDelegate , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityDelegate","LLVMOrcMaterializationResponsibilityDelegate")
		->args({"MR","Symbols","NumSymbols","Result"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:891:6
	makeExtern< void (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcOpaqueSymbolStringPoolEntry *,LLVMOrcCDependenceMapPair *,size_t) , LLVMOrcMaterializationResponsibilityAddDependencies , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityAddDependencies","LLVMOrcMaterializationResponsibilityAddDependencies")
		->args({"MR","Name","Dependencies","NumPairs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:901:6
	makeExtern< void (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcCDependenceMapPair *,size_t) , LLVMOrcMaterializationResponsibilityAddDependenciesForAll , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityAddDependenciesForAll","LLVMOrcMaterializationResponsibilityAddDependenciesForAll")
		->args({"MR","Dependencies","NumPairs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:915:1
	makeExtern< LLVMOrcOpaqueJITDylib * (*)(LLVMOrcOpaqueExecutionSession *,const char *) , LLVMOrcExecutionSessionCreateBareJITDylib , SimNode_ExtFuncCall >(lib,"LLVMOrcExecutionSessionCreateBareJITDylib","LLVMOrcExecutionSessionCreateBareJITDylib")
		->args({"ES","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:931:1
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueExecutionSession *,LLVMOrcOpaqueJITDylib **,const char *) , LLVMOrcExecutionSessionCreateJITDylib , SimNode_ExtFuncCall >(lib,"LLVMOrcExecutionSessionCreateJITDylib","LLVMOrcExecutionSessionCreateJITDylib")
		->args({"ES","Result","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:940:1
	makeExtern< LLVMOrcOpaqueJITDylib * (*)(LLVMOrcOpaqueExecutionSession *,const char *) , LLVMOrcExecutionSessionGetJITDylibByName , SimNode_ExtFuncCall >(lib,"LLVMOrcExecutionSessionGetJITDylibByName","LLVMOrcExecutionSessionGetJITDylibByName")
		->args({"ES","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:949:1
	makeExtern< LLVMOrcOpaqueResourceTracker * (*)(LLVMOrcOpaqueJITDylib *) , LLVMOrcJITDylibCreateResourceTracker , SimNode_ExtFuncCall >(lib,"LLVMOrcJITDylibCreateResourceTracker","LLVMOrcJITDylibCreateResourceTracker")
		->args({"JD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:957:1
	makeExtern< LLVMOrcOpaqueResourceTracker * (*)(LLVMOrcOpaqueJITDylib *) , LLVMOrcJITDylibGetDefaultResourceTracker , SimNode_ExtFuncCall >(lib,"LLVMOrcJITDylibGetDefaultResourceTracker","LLVMOrcJITDylibGetDefaultResourceTracker")
		->args({"JD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:966:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueJITDylib *,LLVMOrcOpaqueMaterializationUnit *) , LLVMOrcJITDylibDefine , SimNode_ExtFuncCall >(lib,"LLVMOrcJITDylibDefine","LLVMOrcJITDylibDefine")
		->args({"JD","MU"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:973:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueJITDylib *) , LLVMOrcJITDylibClear , SimNode_ExtFuncCall >(lib,"LLVMOrcJITDylibClear","LLVMOrcJITDylibClear")
		->args({"JD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:981:6
	makeExtern< void (*)(LLVMOrcOpaqueJITDylib *,LLVMOrcOpaqueDefinitionGenerator *) , LLVMOrcJITDylibAddGenerator , SimNode_ExtFuncCall >(lib,"LLVMOrcJITDylibAddGenerator","LLVMOrcJITDylibAddGenerator")
		->args({"JD","DG"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1005:6
	makeExtern< void (*)(LLVMOrcOpaqueLookupState *,LLVMOpaqueError *) , LLVMOrcLookupStateContinueLookup , SimNode_ExtFuncCall >(lib,"LLVMOrcLookupStateContinueLookup","LLVMOrcLookupStateContinueLookup")
		->args({"S","Err"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1069:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueDefinitionGenerator **,LLVMOrcOpaqueObjectLayer *,const char *,const char *) , LLVMOrcCreateStaticLibrarySearchGeneratorForPath , SimNode_ExtFuncCall >(lib,"LLVMOrcCreateStaticLibrarySearchGeneratorForPath","LLVMOrcCreateStaticLibrarySearchGeneratorForPath")
		->args({"Result","ObjLayer","FileName","TargetTriple"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1081:29
	makeExtern< LLVMOrcOpaqueThreadSafeContext * (*)() , LLVMOrcCreateNewThreadSafeContext , SimNode_ExtFuncCall >(lib,"LLVMOrcCreateNewThreadSafeContext","LLVMOrcCreateNewThreadSafeContext")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1087:1
	makeExtern< LLVMOpaqueContext * (*)(LLVMOrcOpaqueThreadSafeContext *) , LLVMOrcThreadSafeContextGetContext , SimNode_ExtFuncCall >(lib,"LLVMOrcThreadSafeContextGetContext","LLVMOrcThreadSafeContextGetContext")
		->args({"TSCtx"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1092:6
	makeExtern< void (*)(LLVMOrcOpaqueThreadSafeContext *) , LLVMOrcDisposeThreadSafeContext , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeThreadSafeContext","LLVMOrcDisposeThreadSafeContext")
		->args({"TSCtx"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1105:1
	makeExtern< LLVMOrcOpaqueThreadSafeModule * (*)(LLVMOpaqueModule *,LLVMOrcOpaqueThreadSafeContext *) , LLVMOrcCreateNewThreadSafeModule , SimNode_ExtFuncCall >(lib,"LLVMOrcCreateNewThreadSafeModule","LLVMOrcCreateNewThreadSafeModule")
		->args({"M","TSCtx"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1113:6
	makeExtern< void (*)(LLVMOrcOpaqueThreadSafeModule *) , LLVMOrcDisposeThreadSafeModule , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeThreadSafeModule","LLVMOrcDisposeThreadSafeModule")
		->args({"TSM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:1131:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueJITTargetMachineBuilder **) , LLVMOrcJITTargetMachineBuilderDetectHost , SimNode_ExtFuncCall >(lib,"LLVMOrcJITTargetMachineBuilderDetectHost","LLVMOrcJITTargetMachineBuilderDetectHost")
		->args({"Result"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


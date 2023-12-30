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
void Module_dasLLVM::initFunctions_54() {
// from D:\Work\libclang\include\llvm-c/Orc.h:784:6
	addExtern< void (*)(LLVMOrcOpaqueSymbolStringPoolEntry **) , LLVMOrcDisposeSymbols >(*this,lib,"LLVMOrcDisposeSymbols",SideEffects::worstDefault,"LLVMOrcDisposeSymbols")
		->args({"Symbols"});
// from D:\Work\libclang\include\llvm-c/Orc.h:802:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcCSymbolMapPair *,size_t) , LLVMOrcMaterializationResponsibilityNotifyResolved >(*this,lib,"LLVMOrcMaterializationResponsibilityNotifyResolved",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityNotifyResolved")
		->args({"MR","Symbols","NumPairs"});
// from D:\Work\libclang\include\llvm-c/Orc.h:819:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityNotifyEmitted >(*this,lib,"LLVMOrcMaterializationResponsibilityNotifyEmitted",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityNotifyEmitted")
		->args({"MR"});
// from D:\Work\libclang\include\llvm-c/Orc.h:835:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcCSymbolFlagsMapPair *,size_t) , LLVMOrcMaterializationResponsibilityDefineMaterializing >(*this,lib,"LLVMOrcMaterializationResponsibilityDefineMaterializing",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityDefineMaterializing")
		->args({"MR","Pairs","NumPairs"});
// from D:\Work\libclang\include\llvm-c/Orc.h:846:6
	addExtern< void (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityFailMaterialization >(*this,lib,"LLVMOrcMaterializationResponsibilityFailMaterialization",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityFailMaterialization")
		->args({"MR"});
// from D:\Work\libclang\include\llvm-c/Orc.h:856:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcOpaqueMaterializationUnit *) , LLVMOrcMaterializationResponsibilityReplace >(*this,lib,"LLVMOrcMaterializationResponsibilityReplace",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityReplace")
		->args({"MR","MU"});
// from D:\Work\libclang\include\llvm-c/Orc.h:868:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcOpaqueSymbolStringPoolEntry **,size_t,LLVMOrcOpaqueMaterializationResponsibility **) , LLVMOrcMaterializationResponsibilityDelegate >(*this,lib,"LLVMOrcMaterializationResponsibilityDelegate",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityDelegate")
		->args({"MR","Symbols","NumSymbols","Result"});
// from D:\Work\libclang\include\llvm-c/Orc.h:891:6
	addExtern< void (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcOpaqueSymbolStringPoolEntry *,LLVMOrcCDependenceMapPair *,size_t) , LLVMOrcMaterializationResponsibilityAddDependencies >(*this,lib,"LLVMOrcMaterializationResponsibilityAddDependencies",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityAddDependencies")
		->args({"MR","Name","Dependencies","NumPairs"});
// from D:\Work\libclang\include\llvm-c/Orc.h:901:6
	addExtern< void (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcCDependenceMapPair *,size_t) , LLVMOrcMaterializationResponsibilityAddDependenciesForAll >(*this,lib,"LLVMOrcMaterializationResponsibilityAddDependenciesForAll",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityAddDependenciesForAll")
		->args({"MR","Dependencies","NumPairs"});
// from D:\Work\libclang\include\llvm-c/Orc.h:915:1
	addExtern< LLVMOrcOpaqueJITDylib * (*)(LLVMOrcOpaqueExecutionSession *,const char *) , LLVMOrcExecutionSessionCreateBareJITDylib >(*this,lib,"LLVMOrcExecutionSessionCreateBareJITDylib",SideEffects::worstDefault,"LLVMOrcExecutionSessionCreateBareJITDylib")
		->args({"ES","Name"});
// from D:\Work\libclang\include\llvm-c/Orc.h:931:1
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueExecutionSession *,LLVMOrcOpaqueJITDylib **,const char *) , LLVMOrcExecutionSessionCreateJITDylib >(*this,lib,"LLVMOrcExecutionSessionCreateJITDylib",SideEffects::worstDefault,"LLVMOrcExecutionSessionCreateJITDylib")
		->args({"ES","Result","Name"});
// from D:\Work\libclang\include\llvm-c/Orc.h:940:1
	addExtern< LLVMOrcOpaqueJITDylib * (*)(LLVMOrcOpaqueExecutionSession *,const char *) , LLVMOrcExecutionSessionGetJITDylibByName >(*this,lib,"LLVMOrcExecutionSessionGetJITDylibByName",SideEffects::worstDefault,"LLVMOrcExecutionSessionGetJITDylibByName")
		->args({"ES","Name"});
// from D:\Work\libclang\include\llvm-c/Orc.h:949:1
	addExtern< LLVMOrcOpaqueResourceTracker * (*)(LLVMOrcOpaqueJITDylib *) , LLVMOrcJITDylibCreateResourceTracker >(*this,lib,"LLVMOrcJITDylibCreateResourceTracker",SideEffects::worstDefault,"LLVMOrcJITDylibCreateResourceTracker")
		->args({"JD"});
// from D:\Work\libclang\include\llvm-c/Orc.h:957:1
	addExtern< LLVMOrcOpaqueResourceTracker * (*)(LLVMOrcOpaqueJITDylib *) , LLVMOrcJITDylibGetDefaultResourceTracker >(*this,lib,"LLVMOrcJITDylibGetDefaultResourceTracker",SideEffects::worstDefault,"LLVMOrcJITDylibGetDefaultResourceTracker")
		->args({"JD"});
// from D:\Work\libclang\include\llvm-c/Orc.h:966:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueJITDylib *,LLVMOrcOpaqueMaterializationUnit *) , LLVMOrcJITDylibDefine >(*this,lib,"LLVMOrcJITDylibDefine",SideEffects::worstDefault,"LLVMOrcJITDylibDefine")
		->args({"JD","MU"});
// from D:\Work\libclang\include\llvm-c/Orc.h:973:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueJITDylib *) , LLVMOrcJITDylibClear >(*this,lib,"LLVMOrcJITDylibClear",SideEffects::worstDefault,"LLVMOrcJITDylibClear")
		->args({"JD"});
// from D:\Work\libclang\include\llvm-c/Orc.h:981:6
	addExtern< void (*)(LLVMOrcOpaqueJITDylib *,LLVMOrcOpaqueDefinitionGenerator *) , LLVMOrcJITDylibAddGenerator >(*this,lib,"LLVMOrcJITDylibAddGenerator",SideEffects::worstDefault,"LLVMOrcJITDylibAddGenerator")
		->args({"JD","DG"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1005:6
	addExtern< void (*)(LLVMOrcOpaqueLookupState *,LLVMOpaqueError *) , LLVMOrcLookupStateContinueLookup >(*this,lib,"LLVMOrcLookupStateContinueLookup",SideEffects::worstDefault,"LLVMOrcLookupStateContinueLookup")
		->args({"S","Err"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1069:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueDefinitionGenerator **,LLVMOrcOpaqueObjectLayer *,const char *,const char *) , LLVMOrcCreateStaticLibrarySearchGeneratorForPath >(*this,lib,"LLVMOrcCreateStaticLibrarySearchGeneratorForPath",SideEffects::worstDefault,"LLVMOrcCreateStaticLibrarySearchGeneratorForPath")
		->args({"Result","ObjLayer","FileName","TargetTriple"});
// from D:\Work\libclang\include\llvm-c/Orc.h:1081:29
	addExtern< LLVMOrcOpaqueThreadSafeContext * (*)() , LLVMOrcCreateNewThreadSafeContext >(*this,lib,"LLVMOrcCreateNewThreadSafeContext",SideEffects::worstDefault,"LLVMOrcCreateNewThreadSafeContext");
}
}


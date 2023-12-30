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
void Module_dasLLVM::initFunctions_53() {
// from D:\Work\libclang\include\llvm-c/Orc.h:493:1
	addExtern< LLVMOrcOpaqueSymbolStringPool * (*)(LLVMOrcOpaqueExecutionSession *) , LLVMOrcExecutionSessionGetSymbolStringPool >(*this,lib,"LLVMOrcExecutionSessionGetSymbolStringPool",SideEffects::worstDefault,"LLVMOrcExecutionSessionGetSymbolStringPool")
		->args({"ES"});
// from D:\Work\libclang\include\llvm-c/Orc.h:505:6
	addExtern< void (*)(LLVMOrcOpaqueSymbolStringPool *) , LLVMOrcSymbolStringPoolClearDeadEntries >(*this,lib,"LLVMOrcSymbolStringPoolClearDeadEntries",SideEffects::worstDefault,"LLVMOrcSymbolStringPoolClearDeadEntries")
		->args({"SSP"});
// from D:\Work\libclang\include\llvm-c/Orc.h:520:1
	addExtern< LLVMOrcOpaqueSymbolStringPoolEntry * (*)(LLVMOrcOpaqueExecutionSession *,const char *) , LLVMOrcExecutionSessionIntern >(*this,lib,"LLVMOrcExecutionSessionIntern",SideEffects::worstDefault,"LLVMOrcExecutionSessionIntern")
		->args({"ES","Name"});
// from D:\Work\libclang\include\llvm-c/Orc.h:577:6
	addExtern< void (*)(LLVMOrcOpaqueSymbolStringPoolEntry *) , LLVMOrcRetainSymbolStringPoolEntry >(*this,lib,"LLVMOrcRetainSymbolStringPoolEntry",SideEffects::worstDefault,"LLVMOrcRetainSymbolStringPoolEntry")
		->args({"S"});
// from D:\Work\libclang\include\llvm-c/Orc.h:582:6
	addExtern< void (*)(LLVMOrcOpaqueSymbolStringPoolEntry *) , LLVMOrcReleaseSymbolStringPoolEntry >(*this,lib,"LLVMOrcReleaseSymbolStringPoolEntry",SideEffects::worstDefault,"LLVMOrcReleaseSymbolStringPoolEntry")
		->args({"S"});
// from D:\Work\libclang\include\llvm-c/Orc.h:589:13
	addExtern< const char * (*)(LLVMOrcOpaqueSymbolStringPoolEntry *) , LLVMOrcSymbolStringPoolEntryStr >(*this,lib,"LLVMOrcSymbolStringPoolEntryStr",SideEffects::worstDefault,"LLVMOrcSymbolStringPoolEntryStr")
		->args({"S"});
// from D:\Work\libclang\include\llvm-c/Orc.h:594:6
	addExtern< void (*)(LLVMOrcOpaqueResourceTracker *) , LLVMOrcReleaseResourceTracker >(*this,lib,"LLVMOrcReleaseResourceTracker",SideEffects::worstDefault,"LLVMOrcReleaseResourceTracker")
		->args({"RT"});
// from D:\Work\libclang\include\llvm-c/Orc.h:600:6
	addExtern< void (*)(LLVMOrcOpaqueResourceTracker *,LLVMOrcOpaqueResourceTracker *) , LLVMOrcResourceTrackerTransferTo >(*this,lib,"LLVMOrcResourceTrackerTransferTo",SideEffects::worstDefault,"LLVMOrcResourceTrackerTransferTo")
		->args({"SrcRT","DstRT"});
// from D:\Work\libclang\include\llvm-c/Orc.h:607:14
	addExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueResourceTracker *) , LLVMOrcResourceTrackerRemove >(*this,lib,"LLVMOrcResourceTrackerRemove",SideEffects::worstDefault,"LLVMOrcResourceTrackerRemove")
		->args({"RT"});
// from D:\Work\libclang\include\llvm-c/Orc.h:614:6
	addExtern< void (*)(LLVMOrcOpaqueDefinitionGenerator *) , LLVMOrcDisposeDefinitionGenerator >(*this,lib,"LLVMOrcDisposeDefinitionGenerator",SideEffects::worstDefault,"LLVMOrcDisposeDefinitionGenerator")
		->args({"DG"});
// from D:\Work\libclang\include\llvm-c/Orc.h:619:6
	addExtern< void (*)(LLVMOrcOpaqueMaterializationUnit *) , LLVMOrcDisposeMaterializationUnit >(*this,lib,"LLVMOrcDisposeMaterializationUnit",SideEffects::worstDefault,"LLVMOrcDisposeMaterializationUnit")
		->args({"MU"});
// from D:\Work\libclang\include\llvm-c/Orc.h:683:1
	addExtern< LLVMOrcOpaqueMaterializationUnit * (*)(LLVMOrcCSymbolMapPair *,size_t) , LLVMOrcAbsoluteSymbols >(*this,lib,"LLVMOrcAbsoluteSymbols",SideEffects::worstDefault,"LLVMOrcAbsoluteSymbols")
		->args({"Syms","NumPairs"});
// from D:\Work\libclang\include\llvm-c/Orc.h:706:31
	addExtern< LLVMOrcOpaqueMaterializationUnit * (*)(LLVMOrcOpaqueLazyCallThroughManager *,LLVMOrcOpaqueIndirectStubsManager *,LLVMOrcOpaqueJITDylib *,LLVMOrcCSymbolAliasMapPair *,size_t) , LLVMOrcLazyReexports >(*this,lib,"LLVMOrcLazyReexports",SideEffects::worstDefault,"LLVMOrcLazyReexports")
		->args({"LCTM","ISM","SourceRef","CallableAliases","NumPairs"});
// from D:\Work\libclang\include\llvm-c/Orc.h:721:6
	addExtern< void (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcDisposeMaterializationResponsibility >(*this,lib,"LLVMOrcDisposeMaterializationResponsibility",SideEffects::worstDefault,"LLVMOrcDisposeMaterializationResponsibility")
		->args({"MR"});
// from D:\Work\libclang\include\llvm-c/Orc.h:727:20
	addExtern< LLVMOrcOpaqueJITDylib * (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityGetTargetDylib >(*this,lib,"LLVMOrcMaterializationResponsibilityGetTargetDylib",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityGetTargetDylib")
		->args({"MR"});
// from D:\Work\libclang\include\llvm-c/Orc.h:734:1
	addExtern< LLVMOrcOpaqueExecutionSession * (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityGetExecutionSession >(*this,lib,"LLVMOrcMaterializationResponsibilityGetExecutionSession",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityGetExecutionSession")
		->args({"MR"});
// from D:\Work\libclang\include\llvm-c/Orc.h:747:29
	addExtern< LLVMOrcCSymbolFlagsMapPair * (*)(LLVMOrcOpaqueMaterializationResponsibility *,size_t *) , LLVMOrcMaterializationResponsibilityGetSymbols >(*this,lib,"LLVMOrcMaterializationResponsibilityGetSymbols",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityGetSymbols")
		->args({"MR","NumPairs"});
// from D:\Work\libclang\include\llvm-c/Orc.h:755:6
	addExtern< void (*)(LLVMOrcCSymbolFlagsMapPair *) , LLVMOrcDisposeCSymbolFlagsMap >(*this,lib,"LLVMOrcDisposeCSymbolFlagsMap",SideEffects::worstDefault,"LLVMOrcDisposeCSymbolFlagsMap")
		->args({"Pairs"});
// from D:\Work\libclang\include\llvm-c/Orc.h:766:1
	addExtern< LLVMOrcOpaqueSymbolStringPoolEntry * (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityGetInitializerSymbol >(*this,lib,"LLVMOrcMaterializationResponsibilityGetInitializerSymbol",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityGetInitializerSymbol")
		->args({"MR"});
// from D:\Work\libclang\include\llvm-c/Orc.h:776:1
	addExtern< LLVMOrcOpaqueSymbolStringPoolEntry ** (*)(LLVMOrcOpaqueMaterializationResponsibility *,size_t *) , LLVMOrcMaterializationResponsibilityGetRequestedSymbols >(*this,lib,"LLVMOrcMaterializationResponsibilityGetRequestedSymbols",SideEffects::worstDefault,"LLVMOrcMaterializationResponsibilityGetRequestedSymbols")
		->args({"MR","NumSymbols"});
}
}


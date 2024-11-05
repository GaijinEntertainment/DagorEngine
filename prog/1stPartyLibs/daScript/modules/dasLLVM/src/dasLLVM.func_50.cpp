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
// from D:\Work\libclang\include\llvm-c/Orc.h:589:13
	makeExtern< const char * (*)(LLVMOrcOpaqueSymbolStringPoolEntry *) , LLVMOrcSymbolStringPoolEntryStr , SimNode_ExtFuncCall >(lib,"LLVMOrcSymbolStringPoolEntryStr","LLVMOrcSymbolStringPoolEntryStr")
		->args({"S"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:594:6
	makeExtern< void (*)(LLVMOrcOpaqueResourceTracker *) , LLVMOrcReleaseResourceTracker , SimNode_ExtFuncCall >(lib,"LLVMOrcReleaseResourceTracker","LLVMOrcReleaseResourceTracker")
		->args({"RT"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:600:6
	makeExtern< void (*)(LLVMOrcOpaqueResourceTracker *,LLVMOrcOpaqueResourceTracker *) , LLVMOrcResourceTrackerTransferTo , SimNode_ExtFuncCall >(lib,"LLVMOrcResourceTrackerTransferTo","LLVMOrcResourceTrackerTransferTo")
		->args({"SrcRT","DstRT"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:607:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueResourceTracker *) , LLVMOrcResourceTrackerRemove , SimNode_ExtFuncCall >(lib,"LLVMOrcResourceTrackerRemove","LLVMOrcResourceTrackerRemove")
		->args({"RT"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:614:6
	makeExtern< void (*)(LLVMOrcOpaqueDefinitionGenerator *) , LLVMOrcDisposeDefinitionGenerator , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeDefinitionGenerator","LLVMOrcDisposeDefinitionGenerator")
		->args({"DG"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:619:6
	makeExtern< void (*)(LLVMOrcOpaqueMaterializationUnit *) , LLVMOrcDisposeMaterializationUnit , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeMaterializationUnit","LLVMOrcDisposeMaterializationUnit")
		->args({"MU"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:683:1
	makeExtern< LLVMOrcOpaqueMaterializationUnit * (*)(LLVMOrcCSymbolMapPair *,size_t) , LLVMOrcAbsoluteSymbols , SimNode_ExtFuncCall >(lib,"LLVMOrcAbsoluteSymbols","LLVMOrcAbsoluteSymbols")
		->args({"Syms","NumPairs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:706:31
	makeExtern< LLVMOrcOpaqueMaterializationUnit * (*)(LLVMOrcOpaqueLazyCallThroughManager *,LLVMOrcOpaqueIndirectStubsManager *,LLVMOrcOpaqueJITDylib *,LLVMOrcCSymbolAliasMapPair *,size_t) , LLVMOrcLazyReexports , SimNode_ExtFuncCall >(lib,"LLVMOrcLazyReexports","LLVMOrcLazyReexports")
		->args({"LCTM","ISM","SourceRef","CallableAliases","NumPairs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:721:6
	makeExtern< void (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcDisposeMaterializationResponsibility , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeMaterializationResponsibility","LLVMOrcDisposeMaterializationResponsibility")
		->args({"MR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:727:20
	makeExtern< LLVMOrcOpaqueJITDylib * (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityGetTargetDylib , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityGetTargetDylib","LLVMOrcMaterializationResponsibilityGetTargetDylib")
		->args({"MR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:734:1
	makeExtern< LLVMOrcOpaqueExecutionSession * (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityGetExecutionSession , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityGetExecutionSession","LLVMOrcMaterializationResponsibilityGetExecutionSession")
		->args({"MR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:747:29
	makeExtern< LLVMOrcCSymbolFlagsMapPair * (*)(LLVMOrcOpaqueMaterializationResponsibility *,size_t *) , LLVMOrcMaterializationResponsibilityGetSymbols , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityGetSymbols","LLVMOrcMaterializationResponsibilityGetSymbols")
		->args({"MR","NumPairs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:755:6
	makeExtern< void (*)(LLVMOrcCSymbolFlagsMapPair *) , LLVMOrcDisposeCSymbolFlagsMap , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeCSymbolFlagsMap","LLVMOrcDisposeCSymbolFlagsMap")
		->args({"Pairs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:766:1
	makeExtern< LLVMOrcOpaqueSymbolStringPoolEntry * (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityGetInitializerSymbol , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityGetInitializerSymbol","LLVMOrcMaterializationResponsibilityGetInitializerSymbol")
		->args({"MR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:776:1
	makeExtern< LLVMOrcOpaqueSymbolStringPoolEntry ** (*)(LLVMOrcOpaqueMaterializationResponsibility *,size_t *) , LLVMOrcMaterializationResponsibilityGetRequestedSymbols , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityGetRequestedSymbols","LLVMOrcMaterializationResponsibilityGetRequestedSymbols")
		->args({"MR","NumSymbols"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:784:6
	makeExtern< void (*)(LLVMOrcOpaqueSymbolStringPoolEntry **) , LLVMOrcDisposeSymbols , SimNode_ExtFuncCall >(lib,"LLVMOrcDisposeSymbols","LLVMOrcDisposeSymbols")
		->args({"Symbols"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:802:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcCSymbolMapPair *,size_t) , LLVMOrcMaterializationResponsibilityNotifyResolved , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityNotifyResolved","LLVMOrcMaterializationResponsibilityNotifyResolved")
		->args({"MR","Symbols","NumPairs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:819:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityNotifyEmitted , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityNotifyEmitted","LLVMOrcMaterializationResponsibilityNotifyEmitted")
		->args({"MR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:835:14
	makeExtern< LLVMOpaqueError * (*)(LLVMOrcOpaqueMaterializationResponsibility *,LLVMOrcCSymbolFlagsMapPair *,size_t) , LLVMOrcMaterializationResponsibilityDefineMaterializing , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityDefineMaterializing","LLVMOrcMaterializationResponsibilityDefineMaterializing")
		->args({"MR","Pairs","NumPairs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Orc.h:846:6
	makeExtern< void (*)(LLVMOrcOpaqueMaterializationResponsibility *) , LLVMOrcMaterializationResponsibilityFailMaterialization , SimNode_ExtFuncCall >(lib,"LLVMOrcMaterializationResponsibilityFailMaterialization","LLVMOrcMaterializationResponsibilityFailMaterialization")
		->args({"MR"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


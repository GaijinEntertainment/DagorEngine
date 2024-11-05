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
void Module_dasLLVM::initFunctions_20() {
// from D:\Work\libclang\include\llvm-c/Core.h:2290:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMHasUnnamedAddr , SimNode_ExtFuncCall >(lib,"LLVMHasUnnamedAddr","LLVMHasUnnamedAddr")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2292:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetUnnamedAddr , SimNode_ExtFuncCall >(lib,"LLVMSetUnnamedAddr","LLVMSetUnnamedAddr")
		->args({"Global","HasUnnamedAddr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2310:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetAlignment , SimNode_ExtFuncCall >(lib,"LLVMGetAlignment","LLVMGetAlignment")
		->args({"V"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2321:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMSetAlignment , SimNode_ExtFuncCall >(lib,"LLVMSetAlignment","LLVMSetAlignment")
		->args({"V","Bytes"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2329:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueMetadata *) , LLVMGlobalSetMetadata , SimNode_ExtFuncCall >(lib,"LLVMGlobalSetMetadata","LLVMGlobalSetMetadata")
		->args({"Global","Kind","MD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2337:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMGlobalEraseMetadata , SimNode_ExtFuncCall >(lib,"LLVMGlobalEraseMetadata","LLVMGlobalEraseMetadata")
		->args({"Global","Kind"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2344:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMGlobalClearMetadata , SimNode_ExtFuncCall >(lib,"LLVMGlobalClearMetadata","LLVMGlobalClearMetadata")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2353:25
	makeExtern< LLVMOpaqueValueMetadataEntry * (*)(LLVMOpaqueValue *,size_t *) , LLVMGlobalCopyAllMetadata , SimNode_ExtFuncCall >(lib,"LLVMGlobalCopyAllMetadata","LLVMGlobalCopyAllMetadata")
		->args({"Value","NumEntries"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2359:6
	makeExtern< void (*)(LLVMOpaqueValueMetadataEntry *) , LLVMDisposeValueMetadataEntries , SimNode_ExtFuncCall >(lib,"LLVMDisposeValueMetadataEntries","LLVMDisposeValueMetadataEntries")
		->args({"Entries"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2364:10
	makeExtern< unsigned int (*)(LLVMOpaqueValueMetadataEntry *,unsigned int) , LLVMValueMetadataEntriesGetKind , SimNode_ExtFuncCall >(lib,"LLVMValueMetadataEntriesGetKind","LLVMValueMetadataEntriesGetKind")
		->args({"Entries","Index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2372:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueValueMetadataEntry *,unsigned int) , LLVMValueMetadataEntriesGetMetadata , SimNode_ExtFuncCall >(lib,"LLVMValueMetadataEntriesGetMetadata","LLVMValueMetadataEntriesGetMetadata")
		->args({"Entries","Index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2388:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,LLVMOpaqueType *,const char *) , LLVMAddGlobal , SimNode_ExtFuncCall >(lib,"LLVMAddGlobal","LLVMAddGlobal")
		->args({"M","Ty","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2389:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,LLVMOpaqueType *,const char *,unsigned int) , LLVMAddGlobalInAddressSpace , SimNode_ExtFuncCall >(lib,"LLVMAddGlobalInAddressSpace","LLVMAddGlobalInAddressSpace")
		->args({"M","Ty","Name","AddressSpace"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2392:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *) , LLVMGetNamedGlobal , SimNode_ExtFuncCall >(lib,"LLVMGetNamedGlobal","LLVMGetNamedGlobal")
		->args({"M","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2393:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetFirstGlobal , SimNode_ExtFuncCall >(lib,"LLVMGetFirstGlobal","LLVMGetFirstGlobal")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2394:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetLastGlobal , SimNode_ExtFuncCall >(lib,"LLVMGetLastGlobal","LLVMGetLastGlobal")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2395:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextGlobal , SimNode_ExtFuncCall >(lib,"LLVMGetNextGlobal","LLVMGetNextGlobal")
		->args({"GlobalVar"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2396:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousGlobal , SimNode_ExtFuncCall >(lib,"LLVMGetPreviousGlobal","LLVMGetPreviousGlobal")
		->args({"GlobalVar"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2397:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMDeleteGlobal , SimNode_ExtFuncCall >(lib,"LLVMDeleteGlobal","LLVMDeleteGlobal")
		->args({"GlobalVar"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2398:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetInitializer , SimNode_ExtFuncCall >(lib,"LLVMGetInitializer","LLVMGetInitializer")
		->args({"GlobalVar"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


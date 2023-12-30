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
// from D:\Work\libclang\include\llvm-c/Core.h:2277:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMGlobalGetValueType >(*this,lib,"LLVMGlobalGetValueType",SideEffects::worstDefault,"LLVMGlobalGetValueType")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2280:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMHasUnnamedAddr >(*this,lib,"LLVMHasUnnamedAddr",SideEffects::worstDefault,"LLVMHasUnnamedAddr")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2282:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetUnnamedAddr >(*this,lib,"LLVMSetUnnamedAddr",SideEffects::worstDefault,"LLVMSetUnnamedAddr")
		->args({"Global","HasUnnamedAddr"});
// from D:\Work\libclang\include\llvm-c/Core.h:2300:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetAlignment >(*this,lib,"LLVMGetAlignment",SideEffects::worstDefault,"LLVMGetAlignment")
		->args({"V"});
// from D:\Work\libclang\include\llvm-c/Core.h:2311:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMSetAlignment >(*this,lib,"LLVMSetAlignment",SideEffects::worstDefault,"LLVMSetAlignment")
		->args({"V","Bytes"});
// from D:\Work\libclang\include\llvm-c/Core.h:2319:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueMetadata *) , LLVMGlobalSetMetadata >(*this,lib,"LLVMGlobalSetMetadata",SideEffects::worstDefault,"LLVMGlobalSetMetadata")
		->args({"Global","Kind","MD"});
// from D:\Work\libclang\include\llvm-c/Core.h:2327:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMGlobalEraseMetadata >(*this,lib,"LLVMGlobalEraseMetadata",SideEffects::worstDefault,"LLVMGlobalEraseMetadata")
		->args({"Global","Kind"});
// from D:\Work\libclang\include\llvm-c/Core.h:2334:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMGlobalClearMetadata >(*this,lib,"LLVMGlobalClearMetadata",SideEffects::worstDefault,"LLVMGlobalClearMetadata")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2343:25
	addExtern< LLVMOpaqueValueMetadataEntry * (*)(LLVMOpaqueValue *,size_t *) , LLVMGlobalCopyAllMetadata >(*this,lib,"LLVMGlobalCopyAllMetadata",SideEffects::worstDefault,"LLVMGlobalCopyAllMetadata")
		->args({"Value","NumEntries"});
// from D:\Work\libclang\include\llvm-c/Core.h:2349:6
	addExtern< void (*)(LLVMOpaqueValueMetadataEntry *) , LLVMDisposeValueMetadataEntries >(*this,lib,"LLVMDisposeValueMetadataEntries",SideEffects::worstDefault,"LLVMDisposeValueMetadataEntries")
		->args({"Entries"});
// from D:\Work\libclang\include\llvm-c/Core.h:2354:10
	addExtern< unsigned int (*)(LLVMOpaqueValueMetadataEntry *,unsigned int) , LLVMValueMetadataEntriesGetKind >(*this,lib,"LLVMValueMetadataEntriesGetKind",SideEffects::worstDefault,"LLVMValueMetadataEntriesGetKind")
		->args({"Entries","Index"});
// from D:\Work\libclang\include\llvm-c/Core.h:2362:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueValueMetadataEntry *,unsigned int) , LLVMValueMetadataEntriesGetMetadata >(*this,lib,"LLVMValueMetadataEntriesGetMetadata",SideEffects::worstDefault,"LLVMValueMetadataEntriesGetMetadata")
		->args({"Entries","Index"});
// from D:\Work\libclang\include\llvm-c/Core.h:2378:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,LLVMOpaqueType *,const char *) , LLVMAddGlobal >(*this,lib,"LLVMAddGlobal",SideEffects::worstDefault,"LLVMAddGlobal")
		->args({"M","Ty","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:2379:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,LLVMOpaqueType *,const char *,unsigned int) , LLVMAddGlobalInAddressSpace >(*this,lib,"LLVMAddGlobalInAddressSpace",SideEffects::worstDefault,"LLVMAddGlobalInAddressSpace")
		->args({"M","Ty","Name","AddressSpace"});
// from D:\Work\libclang\include\llvm-c/Core.h:2382:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *) , LLVMGetNamedGlobal >(*this,lib,"LLVMGetNamedGlobal",SideEffects::worstDefault,"LLVMGetNamedGlobal")
		->args({"M","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:2383:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetFirstGlobal >(*this,lib,"LLVMGetFirstGlobal",SideEffects::worstDefault,"LLVMGetFirstGlobal")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:2384:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetLastGlobal >(*this,lib,"LLVMGetLastGlobal",SideEffects::worstDefault,"LLVMGetLastGlobal")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:2385:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextGlobal >(*this,lib,"LLVMGetNextGlobal",SideEffects::worstDefault,"LLVMGetNextGlobal")
		->args({"GlobalVar"});
// from D:\Work\libclang\include\llvm-c/Core.h:2386:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousGlobal >(*this,lib,"LLVMGetPreviousGlobal",SideEffects::worstDefault,"LLVMGetPreviousGlobal")
		->args({"GlobalVar"});
// from D:\Work\libclang\include\llvm-c/Core.h:2387:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMDeleteGlobal >(*this,lib,"LLVMDeleteGlobal",SideEffects::worstDefault,"LLVMDeleteGlobal")
		->args({"GlobalVar"});
}
}


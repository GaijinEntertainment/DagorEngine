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
void Module_dasLLVM::initFunctions_61() {
// from D:\Work\libclang\include\llvm-c/Object.h:133:24
	addExtern< LLVMOpaqueSectionIterator * (*)(LLVMOpaqueBinary *) , LLVMObjectFileCopySectionIterator >(*this,lib,"LLVMObjectFileCopySectionIterator",SideEffects::worstDefault,"LLVMObjectFileCopySectionIterator")
		->args({"BR"});
// from D:\Work\libclang\include\llvm-c/Object.h:140:10
	addExtern< int (*)(LLVMOpaqueBinary *,LLVMOpaqueSectionIterator *) , LLVMObjectFileIsSectionIteratorAtEnd >(*this,lib,"LLVMObjectFileIsSectionIteratorAtEnd",SideEffects::worstDefault,"LLVMObjectFileIsSectionIteratorAtEnd")
		->args({"BR","SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:154:23
	addExtern< LLVMOpaqueSymbolIterator * (*)(LLVMOpaqueBinary *) , LLVMObjectFileCopySymbolIterator >(*this,lib,"LLVMObjectFileCopySymbolIterator",SideEffects::worstDefault,"LLVMObjectFileCopySymbolIterator")
		->args({"BR"});
// from D:\Work\libclang\include\llvm-c/Object.h:161:10
	addExtern< int (*)(LLVMOpaqueBinary *,LLVMOpaqueSymbolIterator *) , LLVMObjectFileIsSymbolIteratorAtEnd >(*this,lib,"LLVMObjectFileIsSymbolIteratorAtEnd",SideEffects::worstDefault,"LLVMObjectFileIsSymbolIteratorAtEnd")
		->args({"BR","SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:164:6
	addExtern< void (*)(LLVMOpaqueSectionIterator *) , LLVMDisposeSectionIterator >(*this,lib,"LLVMDisposeSectionIterator",SideEffects::worstDefault,"LLVMDisposeSectionIterator")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:166:6
	addExtern< void (*)(LLVMOpaqueSectionIterator *) , LLVMMoveToNextSection >(*this,lib,"LLVMMoveToNextSection",SideEffects::worstDefault,"LLVMMoveToNextSection")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:167:6
	addExtern< void (*)(LLVMOpaqueSectionIterator *,LLVMOpaqueSymbolIterator *) , LLVMMoveToContainingSection >(*this,lib,"LLVMMoveToContainingSection",SideEffects::worstDefault,"LLVMMoveToContainingSection")
		->args({"Sect","Sym"});
// from D:\Work\libclang\include\llvm-c/Object.h:171:6
	addExtern< void (*)(LLVMOpaqueSymbolIterator *) , LLVMDisposeSymbolIterator >(*this,lib,"LLVMDisposeSymbolIterator",SideEffects::worstDefault,"LLVMDisposeSymbolIterator")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:172:6
	addExtern< void (*)(LLVMOpaqueSymbolIterator *) , LLVMMoveToNextSymbol >(*this,lib,"LLVMMoveToNextSymbol",SideEffects::worstDefault,"LLVMMoveToNextSymbol")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:175:13
	addExtern< const char * (*)(LLVMOpaqueSectionIterator *) , LLVMGetSectionName >(*this,lib,"LLVMGetSectionName",SideEffects::worstDefault,"LLVMGetSectionName")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:176:10
	addExtern< uint64_t (*)(LLVMOpaqueSectionIterator *) , LLVMGetSectionSize >(*this,lib,"LLVMGetSectionSize",SideEffects::worstDefault,"LLVMGetSectionSize")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:177:13
	addExtern< const char * (*)(LLVMOpaqueSectionIterator *) , LLVMGetSectionContents >(*this,lib,"LLVMGetSectionContents",SideEffects::worstDefault,"LLVMGetSectionContents")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:178:10
	addExtern< uint64_t (*)(LLVMOpaqueSectionIterator *) , LLVMGetSectionAddress >(*this,lib,"LLVMGetSectionAddress",SideEffects::worstDefault,"LLVMGetSectionAddress")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:179:10
	addExtern< int (*)(LLVMOpaqueSectionIterator *,LLVMOpaqueSymbolIterator *) , LLVMGetSectionContainsSymbol >(*this,lib,"LLVMGetSectionContainsSymbol",SideEffects::worstDefault,"LLVMGetSectionContainsSymbol")
		->args({"SI","Sym"});
// from D:\Work\libclang\include\llvm-c/Object.h:183:27
	addExtern< LLVMOpaqueRelocationIterator * (*)(LLVMOpaqueSectionIterator *) , LLVMGetRelocations >(*this,lib,"LLVMGetRelocations",SideEffects::worstDefault,"LLVMGetRelocations")
		->args({"Section"});
// from D:\Work\libclang\include\llvm-c/Object.h:184:6
	addExtern< void (*)(LLVMOpaqueRelocationIterator *) , LLVMDisposeRelocationIterator >(*this,lib,"LLVMDisposeRelocationIterator",SideEffects::worstDefault,"LLVMDisposeRelocationIterator")
		->args({"RI"});
// from D:\Work\libclang\include\llvm-c/Object.h:185:10
	addExtern< int (*)(LLVMOpaqueSectionIterator *,LLVMOpaqueRelocationIterator *) , LLVMIsRelocationIteratorAtEnd >(*this,lib,"LLVMIsRelocationIteratorAtEnd",SideEffects::worstDefault,"LLVMIsRelocationIteratorAtEnd")
		->args({"Section","RI"});
// from D:\Work\libclang\include\llvm-c/Object.h:187:6
	addExtern< void (*)(LLVMOpaqueRelocationIterator *) , LLVMMoveToNextRelocation >(*this,lib,"LLVMMoveToNextRelocation",SideEffects::worstDefault,"LLVMMoveToNextRelocation")
		->args({"RI"});
// from D:\Work\libclang\include\llvm-c/Object.h:191:13
	addExtern< const char * (*)(LLVMOpaqueSymbolIterator *) , LLVMGetSymbolName >(*this,lib,"LLVMGetSymbolName",SideEffects::worstDefault,"LLVMGetSymbolName")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:192:10
	addExtern< uint64_t (*)(LLVMOpaqueSymbolIterator *) , LLVMGetSymbolAddress >(*this,lib,"LLVMGetSymbolAddress",SideEffects::worstDefault,"LLVMGetSymbolAddress")
		->args({"SI"});
}
}


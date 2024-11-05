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
void Module_dasLLVM::initFunctions_58() {
// from D:\Work\libclang\include\llvm-c/Object.h:166:6
	makeExtern< void (*)(LLVMOpaqueSectionIterator *) , LLVMMoveToNextSection , SimNode_ExtFuncCall >(lib,"LLVMMoveToNextSection","LLVMMoveToNextSection")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:167:6
	makeExtern< void (*)(LLVMOpaqueSectionIterator *,LLVMOpaqueSymbolIterator *) , LLVMMoveToContainingSection , SimNode_ExtFuncCall >(lib,"LLVMMoveToContainingSection","LLVMMoveToContainingSection")
		->args({"Sect","Sym"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:171:6
	makeExtern< void (*)(LLVMOpaqueSymbolIterator *) , LLVMDisposeSymbolIterator , SimNode_ExtFuncCall >(lib,"LLVMDisposeSymbolIterator","LLVMDisposeSymbolIterator")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:172:6
	makeExtern< void (*)(LLVMOpaqueSymbolIterator *) , LLVMMoveToNextSymbol , SimNode_ExtFuncCall >(lib,"LLVMMoveToNextSymbol","LLVMMoveToNextSymbol")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:175:13
	makeExtern< const char * (*)(LLVMOpaqueSectionIterator *) , LLVMGetSectionName , SimNode_ExtFuncCall >(lib,"LLVMGetSectionName","LLVMGetSectionName")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:176:10
	makeExtern< uint64_t (*)(LLVMOpaqueSectionIterator *) , LLVMGetSectionSize , SimNode_ExtFuncCall >(lib,"LLVMGetSectionSize","LLVMGetSectionSize")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:177:13
	makeExtern< const char * (*)(LLVMOpaqueSectionIterator *) , LLVMGetSectionContents , SimNode_ExtFuncCall >(lib,"LLVMGetSectionContents","LLVMGetSectionContents")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:178:10
	makeExtern< uint64_t (*)(LLVMOpaqueSectionIterator *) , LLVMGetSectionAddress , SimNode_ExtFuncCall >(lib,"LLVMGetSectionAddress","LLVMGetSectionAddress")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:179:10
	makeExtern< int (*)(LLVMOpaqueSectionIterator *,LLVMOpaqueSymbolIterator *) , LLVMGetSectionContainsSymbol , SimNode_ExtFuncCall >(lib,"LLVMGetSectionContainsSymbol","LLVMGetSectionContainsSymbol")
		->args({"SI","Sym"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:183:27
	makeExtern< LLVMOpaqueRelocationIterator * (*)(LLVMOpaqueSectionIterator *) , LLVMGetRelocations , SimNode_ExtFuncCall >(lib,"LLVMGetRelocations","LLVMGetRelocations")
		->args({"Section"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:184:6
	makeExtern< void (*)(LLVMOpaqueRelocationIterator *) , LLVMDisposeRelocationIterator , SimNode_ExtFuncCall >(lib,"LLVMDisposeRelocationIterator","LLVMDisposeRelocationIterator")
		->args({"RI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:185:10
	makeExtern< int (*)(LLVMOpaqueSectionIterator *,LLVMOpaqueRelocationIterator *) , LLVMIsRelocationIteratorAtEnd , SimNode_ExtFuncCall >(lib,"LLVMIsRelocationIteratorAtEnd","LLVMIsRelocationIteratorAtEnd")
		->args({"Section","RI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:187:6
	makeExtern< void (*)(LLVMOpaqueRelocationIterator *) , LLVMMoveToNextRelocation , SimNode_ExtFuncCall >(lib,"LLVMMoveToNextRelocation","LLVMMoveToNextRelocation")
		->args({"RI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:191:13
	makeExtern< const char * (*)(LLVMOpaqueSymbolIterator *) , LLVMGetSymbolName , SimNode_ExtFuncCall >(lib,"LLVMGetSymbolName","LLVMGetSymbolName")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:192:10
	makeExtern< uint64_t (*)(LLVMOpaqueSymbolIterator *) , LLVMGetSymbolAddress , SimNode_ExtFuncCall >(lib,"LLVMGetSymbolAddress","LLVMGetSymbolAddress")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:193:10
	makeExtern< uint64_t (*)(LLVMOpaqueSymbolIterator *) , LLVMGetSymbolSize , SimNode_ExtFuncCall >(lib,"LLVMGetSymbolSize","LLVMGetSymbolSize")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:196:10
	makeExtern< uint64_t (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationOffset , SimNode_ExtFuncCall >(lib,"LLVMGetRelocationOffset","LLVMGetRelocationOffset")
		->args({"RI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:197:23
	makeExtern< LLVMOpaqueSymbolIterator * (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationSymbol , SimNode_ExtFuncCall >(lib,"LLVMGetRelocationSymbol","LLVMGetRelocationSymbol")
		->args({"RI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:198:10
	makeExtern< uint64_t (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationType , SimNode_ExtFuncCall >(lib,"LLVMGetRelocationType","LLVMGetRelocationType")
		->args({"RI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:201:13
	makeExtern< const char * (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationTypeName , SimNode_ExtFuncCall >(lib,"LLVMGetRelocationTypeName","LLVMGetRelocationTypeName")
		->args({"RI"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


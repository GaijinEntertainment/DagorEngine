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
void Module_dasLLVM::initFunctions_62() {
// from D:\Work\libclang\include\llvm-c/Object.h:193:10
	addExtern< uint64_t (*)(LLVMOpaqueSymbolIterator *) , LLVMGetSymbolSize >(*this,lib,"LLVMGetSymbolSize",SideEffects::worstDefault,"LLVMGetSymbolSize")
		->args({"SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:196:10
	addExtern< uint64_t (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationOffset >(*this,lib,"LLVMGetRelocationOffset",SideEffects::worstDefault,"LLVMGetRelocationOffset")
		->args({"RI"});
// from D:\Work\libclang\include\llvm-c/Object.h:197:23
	addExtern< LLVMOpaqueSymbolIterator * (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationSymbol >(*this,lib,"LLVMGetRelocationSymbol",SideEffects::worstDefault,"LLVMGetRelocationSymbol")
		->args({"RI"});
// from D:\Work\libclang\include\llvm-c/Object.h:198:10
	addExtern< uint64_t (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationType >(*this,lib,"LLVMGetRelocationType",SideEffects::worstDefault,"LLVMGetRelocationType")
		->args({"RI"});
// from D:\Work\libclang\include\llvm-c/Object.h:201:13
	addExtern< const char * (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationTypeName >(*this,lib,"LLVMGetRelocationTypeName",SideEffects::worstDefault,"LLVMGetRelocationTypeName")
		->args({"RI"});
// from D:\Work\libclang\include\llvm-c/Object.h:202:13
	addExtern< const char * (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationValueString >(*this,lib,"LLVMGetRelocationValueString",SideEffects::worstDefault,"LLVMGetRelocationValueString")
		->args({"RI"});
// from D:\Work\libclang\include\llvm-c/Object.h:208:19
	addExtern< LLVMOpaqueObjectFile * (*)(LLVMOpaqueMemoryBuffer *) , LLVMCreateObjectFile >(*this,lib,"LLVMCreateObjectFile",SideEffects::worstDefault,"LLVMCreateObjectFile")
		->args({"MemBuf"});
// from D:\Work\libclang\include\llvm-c/Object.h:211:6
	addExtern< void (*)(LLVMOpaqueObjectFile *) , LLVMDisposeObjectFile >(*this,lib,"LLVMDisposeObjectFile",SideEffects::worstDefault,"LLVMDisposeObjectFile")
		->args({"ObjectFile"});
// from D:\Work\libclang\include\llvm-c/Object.h:214:24
	addExtern< LLVMOpaqueSectionIterator * (*)(LLVMOpaqueObjectFile *) , LLVMGetSections >(*this,lib,"LLVMGetSections",SideEffects::worstDefault,"LLVMGetSections")
		->args({"ObjectFile"});
// from D:\Work\libclang\include\llvm-c/Object.h:217:10
	addExtern< int (*)(LLVMOpaqueObjectFile *,LLVMOpaqueSectionIterator *) , LLVMIsSectionIteratorAtEnd >(*this,lib,"LLVMIsSectionIteratorAtEnd",SideEffects::worstDefault,"LLVMIsSectionIteratorAtEnd")
		->args({"ObjectFile","SI"});
// from D:\Work\libclang\include\llvm-c/Object.h:221:23
	addExtern< LLVMOpaqueSymbolIterator * (*)(LLVMOpaqueObjectFile *) , LLVMGetSymbols >(*this,lib,"LLVMGetSymbols",SideEffects::worstDefault,"LLVMGetSymbols")
		->args({"ObjectFile"});
// from D:\Work\libclang\include\llvm-c/Object.h:224:10
	addExtern< int (*)(LLVMOpaqueObjectFile *,LLVMOpaqueSymbolIterator *) , LLVMIsSymbolIteratorAtEnd >(*this,lib,"LLVMIsSymbolIteratorAtEnd",SideEffects::worstDefault,"LLVMIsSymbolIteratorAtEnd")
		->args({"ObjectFile","SI"});
// from D:\Work\libclang\include\llvm-c/OrcEE.h:47:1
	addExtern< LLVMOrcOpaqueObjectLayer * (*)(LLVMOrcOpaqueExecutionSession *) , LLVMOrcCreateRTDyldObjectLinkingLayerWithSectionMemoryManager >(*this,lib,"LLVMOrcCreateRTDyldObjectLinkingLayerWithSectionMemoryManager",SideEffects::worstDefault,"LLVMOrcCreateRTDyldObjectLinkingLayerWithSectionMemoryManager")
		->args({"ES"});
// from D:\Work\libclang\include\llvm-c/OrcEE.h:56:6
	addExtern< void (*)(LLVMOrcOpaqueObjectLayer *,LLVMOpaqueJITEventListener *) , LLVMOrcRTDyldObjectLinkingLayerRegisterJITEventListener >(*this,lib,"LLVMOrcRTDyldObjectLinkingLayerRegisterJITEventListener",SideEffects::worstDefault,"LLVMOrcRTDyldObjectLinkingLayerRegisterJITEventListener")
		->args({"RTDyldObjLinkingLayer","Listener"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:64:20
	addExtern< const char * (*)(LLVMRemarkOpaqueString *) , LLVMRemarkStringGetData >(*this,lib,"LLVMRemarkStringGetData",SideEffects::worstDefault,"LLVMRemarkStringGetData")
		->args({"String"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:71:17
	addExtern< unsigned int (*)(LLVMRemarkOpaqueString *) , LLVMRemarkStringGetLen >(*this,lib,"LLVMRemarkStringGetLen",SideEffects::worstDefault,"LLVMRemarkStringGetLen")
		->args({"String"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:86:1
	addExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueDebugLoc *) , LLVMRemarkDebugLocGetSourceFilePath >(*this,lib,"LLVMRemarkDebugLocGetSourceFilePath",SideEffects::worstDefault,"LLVMRemarkDebugLocGetSourceFilePath")
		->args({"DL"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:93:17
	addExtern< unsigned int (*)(LLVMRemarkOpaqueDebugLoc *) , LLVMRemarkDebugLocGetSourceLine >(*this,lib,"LLVMRemarkDebugLocGetSourceLine",SideEffects::worstDefault,"LLVMRemarkDebugLocGetSourceLine")
		->args({"DL"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:100:17
	addExtern< unsigned int (*)(LLVMRemarkOpaqueDebugLoc *) , LLVMRemarkDebugLocGetSourceColumn >(*this,lib,"LLVMRemarkDebugLocGetSourceColumn",SideEffects::worstDefault,"LLVMRemarkDebugLocGetSourceColumn")
		->args({"DL"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:117:28
	addExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueArg *) , LLVMRemarkArgGetKey >(*this,lib,"LLVMRemarkArgGetKey",SideEffects::worstDefault,"LLVMRemarkArgGetKey")
		->args({"Arg"});
}
}


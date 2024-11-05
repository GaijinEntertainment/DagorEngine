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
void Module_dasLLVM::initFunctions_59() {
// from D:\Work\libclang\include\llvm-c/Object.h:202:13
	makeExtern< const char * (*)(LLVMOpaqueRelocationIterator *) , LLVMGetRelocationValueString , SimNode_ExtFuncCall >(lib,"LLVMGetRelocationValueString","LLVMGetRelocationValueString")
		->args({"RI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:208:19
	makeExtern< LLVMOpaqueObjectFile * (*)(LLVMOpaqueMemoryBuffer *) , LLVMCreateObjectFile , SimNode_ExtFuncCall >(lib,"LLVMCreateObjectFile","LLVMCreateObjectFile")
		->args({"MemBuf"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:211:6
	makeExtern< void (*)(LLVMOpaqueObjectFile *) , LLVMDisposeObjectFile , SimNode_ExtFuncCall >(lib,"LLVMDisposeObjectFile","LLVMDisposeObjectFile")
		->args({"ObjectFile"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:214:24
	makeExtern< LLVMOpaqueSectionIterator * (*)(LLVMOpaqueObjectFile *) , LLVMGetSections , SimNode_ExtFuncCall >(lib,"LLVMGetSections","LLVMGetSections")
		->args({"ObjectFile"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:217:10
	makeExtern< int (*)(LLVMOpaqueObjectFile *,LLVMOpaqueSectionIterator *) , LLVMIsSectionIteratorAtEnd , SimNode_ExtFuncCall >(lib,"LLVMIsSectionIteratorAtEnd","LLVMIsSectionIteratorAtEnd")
		->args({"ObjectFile","SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:221:23
	makeExtern< LLVMOpaqueSymbolIterator * (*)(LLVMOpaqueObjectFile *) , LLVMGetSymbols , SimNode_ExtFuncCall >(lib,"LLVMGetSymbols","LLVMGetSymbols")
		->args({"ObjectFile"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:224:10
	makeExtern< int (*)(LLVMOpaqueObjectFile *,LLVMOpaqueSymbolIterator *) , LLVMIsSymbolIteratorAtEnd , SimNode_ExtFuncCall >(lib,"LLVMIsSymbolIteratorAtEnd","LLVMIsSymbolIteratorAtEnd")
		->args({"ObjectFile","SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/OrcEE.h:50:1
	makeExtern< LLVMOrcOpaqueObjectLayer * (*)(LLVMOrcOpaqueExecutionSession *) , LLVMOrcCreateRTDyldObjectLinkingLayerWithSectionMemoryManager , SimNode_ExtFuncCall >(lib,"LLVMOrcCreateRTDyldObjectLinkingLayerWithSectionMemoryManager","LLVMOrcCreateRTDyldObjectLinkingLayerWithSectionMemoryManager")
		->args({"ES"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/OrcEE.h:93:6
	makeExtern< void (*)(LLVMOrcOpaqueObjectLayer *,LLVMOpaqueJITEventListener *) , LLVMOrcRTDyldObjectLinkingLayerRegisterJITEventListener , SimNode_ExtFuncCall >(lib,"LLVMOrcRTDyldObjectLinkingLayerRegisterJITEventListener","LLVMOrcRTDyldObjectLinkingLayerRegisterJITEventListener")
		->args({"RTDyldObjLinkingLayer","Listener"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:64:20
	makeExtern< const char * (*)(LLVMRemarkOpaqueString *) , LLVMRemarkStringGetData , SimNode_ExtFuncCall >(lib,"LLVMRemarkStringGetData","LLVMRemarkStringGetData")
		->args({"String"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:71:17
	makeExtern< unsigned int (*)(LLVMRemarkOpaqueString *) , LLVMRemarkStringGetLen , SimNode_ExtFuncCall >(lib,"LLVMRemarkStringGetLen","LLVMRemarkStringGetLen")
		->args({"String"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:86:1
	makeExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueDebugLoc *) , LLVMRemarkDebugLocGetSourceFilePath , SimNode_ExtFuncCall >(lib,"LLVMRemarkDebugLocGetSourceFilePath","LLVMRemarkDebugLocGetSourceFilePath")
		->args({"DL"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:93:17
	makeExtern< unsigned int (*)(LLVMRemarkOpaqueDebugLoc *) , LLVMRemarkDebugLocGetSourceLine , SimNode_ExtFuncCall >(lib,"LLVMRemarkDebugLocGetSourceLine","LLVMRemarkDebugLocGetSourceLine")
		->args({"DL"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:100:17
	makeExtern< unsigned int (*)(LLVMRemarkOpaqueDebugLoc *) , LLVMRemarkDebugLocGetSourceColumn , SimNode_ExtFuncCall >(lib,"LLVMRemarkDebugLocGetSourceColumn","LLVMRemarkDebugLocGetSourceColumn")
		->args({"DL"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:117:28
	makeExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueArg *) , LLVMRemarkArgGetKey , SimNode_ExtFuncCall >(lib,"LLVMRemarkArgGetKey","LLVMRemarkArgGetKey")
		->args({"Arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:124:28
	makeExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueArg *) , LLVMRemarkArgGetValue , SimNode_ExtFuncCall >(lib,"LLVMRemarkArgGetValue","LLVMRemarkArgGetValue")
		->args({"Arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:133:30
	makeExtern< LLVMRemarkOpaqueDebugLoc * (*)(LLVMRemarkOpaqueArg *) , LLVMRemarkArgGetDebugLoc , SimNode_ExtFuncCall >(lib,"LLVMRemarkArgGetDebugLoc","LLVMRemarkArgGetDebugLoc")
		->args({"Arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:147:13
	makeExtern< void (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryDispose , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryDispose","LLVMRemarkEntryDispose")
		->args({"Remark"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:155:28
	makeExtern< LLVMRemarkType (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetType , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryGetType","LLVMRemarkEntryGetType")
		->args({"Remark"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:163:1
	makeExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetPassName , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryGetPassName","LLVMRemarkEntryGetPassName")
		->args({"Remark"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


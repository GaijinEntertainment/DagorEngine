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
void Module_dasLLVM::initFunctions_63() {
// from D:\Work\libclang\include\llvm-c/Remarks.h:124:28
	addExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueArg *) , LLVMRemarkArgGetValue >(*this,lib,"LLVMRemarkArgGetValue",SideEffects::worstDefault,"LLVMRemarkArgGetValue")
		->args({"Arg"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:133:30
	addExtern< LLVMRemarkOpaqueDebugLoc * (*)(LLVMRemarkOpaqueArg *) , LLVMRemarkArgGetDebugLoc >(*this,lib,"LLVMRemarkArgGetDebugLoc",SideEffects::worstDefault,"LLVMRemarkArgGetDebugLoc")
		->args({"Arg"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:147:13
	addExtern< void (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryDispose >(*this,lib,"LLVMRemarkEntryDispose",SideEffects::worstDefault,"LLVMRemarkEntryDispose")
		->args({"Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:155:28
	addExtern< LLVMRemarkType (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetType >(*this,lib,"LLVMRemarkEntryGetType",SideEffects::worstDefault,"LLVMRemarkEntryGetType")
		->args({"Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:163:1
	addExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetPassName >(*this,lib,"LLVMRemarkEntryGetPassName",SideEffects::worstDefault,"LLVMRemarkEntryGetPassName")
		->args({"Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:171:1
	addExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetRemarkName >(*this,lib,"LLVMRemarkEntryGetRemarkName",SideEffects::worstDefault,"LLVMRemarkEntryGetRemarkName")
		->args({"Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:179:1
	addExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetFunctionName >(*this,lib,"LLVMRemarkEntryGetFunctionName",SideEffects::worstDefault,"LLVMRemarkEntryGetFunctionName")
		->args({"Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:189:1
	addExtern< LLVMRemarkOpaqueDebugLoc * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetDebugLoc >(*this,lib,"LLVMRemarkEntryGetDebugLoc",SideEffects::worstDefault,"LLVMRemarkEntryGetDebugLoc")
		->args({"Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:198:17
	addExtern< uint64_t (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetHotness >(*this,lib,"LLVMRemarkEntryGetHotness",SideEffects::worstDefault,"LLVMRemarkEntryGetHotness")
		->args({"Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:205:17
	addExtern< unsigned int (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetNumArgs >(*this,lib,"LLVMRemarkEntryGetNumArgs",SideEffects::worstDefault,"LLVMRemarkEntryGetNumArgs")
		->args({"Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:216:25
	addExtern< LLVMRemarkOpaqueArg * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetFirstArg >(*this,lib,"LLVMRemarkEntryGetFirstArg",SideEffects::worstDefault,"LLVMRemarkEntryGetFirstArg")
		->args({"Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:227:25
	addExtern< LLVMRemarkOpaqueArg * (*)(LLVMRemarkOpaqueArg *,LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetNextArg >(*this,lib,"LLVMRemarkEntryGetNextArg",SideEffects::worstDefault,"LLVMRemarkEntryGetNextArg")
		->args({"It","Remark"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:243:28
	addExtern< LLVMRemarkOpaqueParser * (*)(const void *,uint64_t) , LLVMRemarkParserCreateYAML >(*this,lib,"LLVMRemarkParserCreateYAML",SideEffects::worstDefault,"LLVMRemarkParserCreateYAML")
		->args({"Buf","Size"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:257:28
	addExtern< LLVMRemarkOpaqueParser * (*)(const void *,uint64_t) , LLVMRemarkParserCreateBitstream >(*this,lib,"LLVMRemarkParserCreateBitstream",SideEffects::worstDefault,"LLVMRemarkParserCreateBitstream")
		->args({"Buf","Size"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:302:27
	addExtern< LLVMRemarkOpaqueEntry * (*)(LLVMRemarkOpaqueParser *) , LLVMRemarkParserGetNext >(*this,lib,"LLVMRemarkParserGetNext",SideEffects::worstDefault,"LLVMRemarkParserGetNext")
		->args({"Parser"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:309:17
	addExtern< int (*)(LLVMRemarkOpaqueParser *) , LLVMRemarkParserHasError >(*this,lib,"LLVMRemarkParserHasError",SideEffects::worstDefault,"LLVMRemarkParserHasError")
		->args({"Parser"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:322:20
	addExtern< const char * (*)(LLVMRemarkOpaqueParser *) , LLVMRemarkParserGetErrorMessage >(*this,lib,"LLVMRemarkParserGetErrorMessage",SideEffects::worstDefault,"LLVMRemarkParserGetErrorMessage")
		->args({"Parser"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:329:13
	addExtern< void (*)(LLVMRemarkOpaqueParser *) , LLVMRemarkParserDispose >(*this,lib,"LLVMRemarkParserDispose",SideEffects::worstDefault,"LLVMRemarkParserDispose")
		->args({"Parser"});
// from D:\Work\libclang\include\llvm-c/Remarks.h:336:17
	addExtern< unsigned int (*)() , LLVMRemarkVersion >(*this,lib,"LLVMRemarkVersion",SideEffects::worstDefault,"LLVMRemarkVersion");
// from D:\Work\libclang\include\llvm-c/Support.h:35:10
	addExtern< int (*)(const char *) , LLVMLoadLibraryPermanently >(*this,lib,"LLVMLoadLibraryPermanently",SideEffects::worstDefault,"LLVMLoadLibraryPermanently")
		->args({"Filename"});
}
}


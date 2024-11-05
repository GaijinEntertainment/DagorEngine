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
void Module_dasLLVM::initFunctions_60() {
// from D:\Work\libclang\include\llvm-c/Remarks.h:171:1
	makeExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetRemarkName , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryGetRemarkName","LLVMRemarkEntryGetRemarkName")
		->args({"Remark"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:179:1
	makeExtern< LLVMRemarkOpaqueString * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetFunctionName , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryGetFunctionName","LLVMRemarkEntryGetFunctionName")
		->args({"Remark"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:189:1
	makeExtern< LLVMRemarkOpaqueDebugLoc * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetDebugLoc , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryGetDebugLoc","LLVMRemarkEntryGetDebugLoc")
		->args({"Remark"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:198:17
	makeExtern< uint64_t (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetHotness , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryGetHotness","LLVMRemarkEntryGetHotness")
		->args({"Remark"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:205:17
	makeExtern< unsigned int (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetNumArgs , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryGetNumArgs","LLVMRemarkEntryGetNumArgs")
		->args({"Remark"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:216:25
	makeExtern< LLVMRemarkOpaqueArg * (*)(LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetFirstArg , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryGetFirstArg","LLVMRemarkEntryGetFirstArg")
		->args({"Remark"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:227:25
	makeExtern< LLVMRemarkOpaqueArg * (*)(LLVMRemarkOpaqueArg *,LLVMRemarkOpaqueEntry *) , LLVMRemarkEntryGetNextArg , SimNode_ExtFuncCall >(lib,"LLVMRemarkEntryGetNextArg","LLVMRemarkEntryGetNextArg")
		->args({"It","Remark"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:243:28
	makeExtern< LLVMRemarkOpaqueParser * (*)(const void *,uint64_t) , LLVMRemarkParserCreateYAML , SimNode_ExtFuncCall >(lib,"LLVMRemarkParserCreateYAML","LLVMRemarkParserCreateYAML")
		->args({"Buf","Size"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:257:28
	makeExtern< LLVMRemarkOpaqueParser * (*)(const void *,uint64_t) , LLVMRemarkParserCreateBitstream , SimNode_ExtFuncCall >(lib,"LLVMRemarkParserCreateBitstream","LLVMRemarkParserCreateBitstream")
		->args({"Buf","Size"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:302:27
	makeExtern< LLVMRemarkOpaqueEntry * (*)(LLVMRemarkOpaqueParser *) , LLVMRemarkParserGetNext , SimNode_ExtFuncCall >(lib,"LLVMRemarkParserGetNext","LLVMRemarkParserGetNext")
		->args({"Parser"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:309:17
	makeExtern< int (*)(LLVMRemarkOpaqueParser *) , LLVMRemarkParserHasError , SimNode_ExtFuncCall >(lib,"LLVMRemarkParserHasError","LLVMRemarkParserHasError")
		->args({"Parser"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:322:20
	makeExtern< const char * (*)(LLVMRemarkOpaqueParser *) , LLVMRemarkParserGetErrorMessage , SimNode_ExtFuncCall >(lib,"LLVMRemarkParserGetErrorMessage","LLVMRemarkParserGetErrorMessage")
		->args({"Parser"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:329:13
	makeExtern< void (*)(LLVMRemarkOpaqueParser *) , LLVMRemarkParserDispose , SimNode_ExtFuncCall >(lib,"LLVMRemarkParserDispose","LLVMRemarkParserDispose")
		->args({"Parser"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Remarks.h:336:17
	makeExtern< unsigned int (*)() , LLVMRemarkVersion , SimNode_ExtFuncCall >(lib,"LLVMRemarkVersion","LLVMRemarkVersion")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Support.h:35:10
	makeExtern< int (*)(const char *) , LLVMLoadLibraryPermanently , SimNode_ExtFuncCall >(lib,"LLVMLoadLibraryPermanently","LLVMLoadLibraryPermanently")
		->args({"Filename"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Support.h:45:6
	makeExtern< void (*)(int,const char *const *,const char *) , LLVMParseCommandLineOptions , SimNode_ExtFuncCall >(lib,"LLVMParseCommandLineOptions","LLVMParseCommandLineOptions")
		->args({"argc","argv","Overview"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Support.h:55:7
	makeExtern< void * (*)(const char *) , LLVMSearchForAddressOfSymbol , SimNode_ExtFuncCall >(lib,"LLVMSearchForAddressOfSymbol","LLVMSearchForAddressOfSymbol")
		->args({"symbolName"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Support.h:64:6
	makeExtern< void (*)(const char *,void *) , LLVMAddSymbol , SimNode_ExtFuncCall >(lib,"LLVMAddSymbol","LLVMAddSymbol")
		->args({"symbolName","symbolValue"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Transforms/IPO.h:31:6
	makeExtern< void (*)(LLVMOpaquePassManager *) , LLVMAddConstantMergePass , SimNode_ExtFuncCall >(lib,"LLVMAddConstantMergePass","LLVMAddConstantMergePass")
		->args({"PM"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


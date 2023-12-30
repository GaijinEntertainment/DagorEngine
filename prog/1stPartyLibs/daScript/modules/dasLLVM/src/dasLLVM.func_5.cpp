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
void Module_dasLLVM::initFunctions_5() {
// from D:\Work\libclang\include\llvm-c/Core.h:869:13
	addExtern< const char * (*)(LLVMOpaqueModule *,size_t *) , LLVMGetModuleInlineAsm >(*this,lib,"LLVMGetModuleInlineAsm",SideEffects::worstDefault,"LLVMGetModuleInlineAsm")
		->args({"M","Len"});
// from D:\Work\libclang\include\llvm-c/Core.h:876:6
	addExtern< void (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMSetModuleInlineAsm2 >(*this,lib,"LLVMSetModuleInlineAsm2",SideEffects::worstDefault,"LLVMSetModuleInlineAsm2")
		->args({"M","Asm","Len"});
// from D:\Work\libclang\include\llvm-c/Core.h:883:6
	addExtern< void (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMAppendModuleInlineAsm >(*this,lib,"LLVMAppendModuleInlineAsm",SideEffects::worstDefault,"LLVMAppendModuleInlineAsm")
		->args({"M","Asm","Len"});
// from D:\Work\libclang\include\llvm-c/Core.h:890:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,char *,size_t,char *,size_t,int,int,LLVMInlineAsmDialect,int) , LLVMGetInlineAsm >(*this,lib,"LLVMGetInlineAsm",SideEffects::worstDefault,"LLVMGetInlineAsm")
		->args({"Ty","AsmString","AsmStringSize","Constraints","ConstraintsSize","HasSideEffects","IsAlignStack","Dialect","CanThrow"});
// from D:\Work\libclang\include\llvm-c/Core.h:901:16
	addExtern< LLVMOpaqueContext * (*)(LLVMOpaqueModule *) , LLVMGetModuleContext >(*this,lib,"LLVMGetModuleContext",SideEffects::worstDefault,"LLVMGetModuleContext")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:904:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueModule *,const char *) , LLVMGetTypeByName >(*this,lib,"LLVMGetTypeByName",SideEffects::worstDefault,"LLVMGetTypeByName")
		->args({"M","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:911:20
	addExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueModule *) , LLVMGetFirstNamedMetadata >(*this,lib,"LLVMGetFirstNamedMetadata",SideEffects::worstDefault,"LLVMGetFirstNamedMetadata")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:918:20
	addExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueModule *) , LLVMGetLastNamedMetadata >(*this,lib,"LLVMGetLastNamedMetadata",SideEffects::worstDefault,"LLVMGetLastNamedMetadata")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:926:20
	addExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueNamedMDNode *) , LLVMGetNextNamedMetadata >(*this,lib,"LLVMGetNextNamedMetadata",SideEffects::worstDefault,"LLVMGetNextNamedMetadata")
		->args({"NamedMDNode"});
// from D:\Work\libclang\include\llvm-c/Core.h:934:20
	addExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueNamedMDNode *) , LLVMGetPreviousNamedMetadata >(*this,lib,"LLVMGetPreviousNamedMetadata",SideEffects::worstDefault,"LLVMGetPreviousNamedMetadata")
		->args({"NamedMDNode"});
// from D:\Work\libclang\include\llvm-c/Core.h:942:20
	addExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetNamedMetadata >(*this,lib,"LLVMGetNamedMetadata",SideEffects::worstDefault,"LLVMGetNamedMetadata")
		->args({"M","Name","NameLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:951:20
	addExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetOrInsertNamedMetadata >(*this,lib,"LLVMGetOrInsertNamedMetadata",SideEffects::worstDefault,"LLVMGetOrInsertNamedMetadata")
		->args({"M","Name","NameLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:960:13
	addExtern< const char * (*)(LLVMOpaqueNamedMDNode *,size_t *) , LLVMGetNamedMetadataName >(*this,lib,"LLVMGetNamedMetadataName",SideEffects::worstDefault,"LLVMGetNamedMetadataName")
		->args({"NamedMD","NameLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:968:10
	addExtern< unsigned int (*)(LLVMOpaqueModule *,const char *) , LLVMGetNamedMetadataNumOperands >(*this,lib,"LLVMGetNamedMetadataNumOperands",SideEffects::worstDefault,"LLVMGetNamedMetadataNumOperands")
		->args({"M","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:981:6
	addExtern< void (*)(LLVMOpaqueModule *,const char *,LLVMOpaqueValue **) , LLVMGetNamedMetadataOperands >(*this,lib,"LLVMGetNamedMetadataOperands",SideEffects::worstDefault,"LLVMGetNamedMetadataOperands")
		->args({"M","Name","Dest"});
// from D:\Work\libclang\include\llvm-c/Core.h:990:6
	addExtern< void (*)(LLVMOpaqueModule *,const char *,LLVMOpaqueValue *) , LLVMAddNamedMetadataOperand >(*this,lib,"LLVMAddNamedMetadataOperand",SideEffects::worstDefault,"LLVMAddNamedMetadataOperand")
		->args({"M","Name","Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1001:13
	addExtern< const char * (*)(LLVMOpaqueValue *,unsigned int *) , LLVMGetDebugLocDirectory >(*this,lib,"LLVMGetDebugLocDirectory",SideEffects::worstDefault,"LLVMGetDebugLocDirectory")
		->args({"Val","Length"});
// from D:\Work\libclang\include\llvm-c/Core.h:1011:13
	addExtern< const char * (*)(LLVMOpaqueValue *,unsigned int *) , LLVMGetDebugLocFilename >(*this,lib,"LLVMGetDebugLocFilename",SideEffects::worstDefault,"LLVMGetDebugLocFilename")
		->args({"Val","Length"});
// from D:\Work\libclang\include\llvm-c/Core.h:1021:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetDebugLocLine >(*this,lib,"LLVMGetDebugLocLine",SideEffects::worstDefault,"LLVMGetDebugLocLine")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1029:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetDebugLocColumn >(*this,lib,"LLVMGetDebugLocColumn",SideEffects::worstDefault,"LLVMGetDebugLocColumn")
		->args({"Val"});
}
}


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
void Module_dasLLVM::initFunctions_4() {
// from D:\Work\libclang\include\llvm-c/Core.h:706:6
	addExtern< void (*)(LLVMOpaqueModule *) , LLVMDisposeModule >(*this,lib,"LLVMDisposeModule",SideEffects::worstDefault,"LLVMDisposeModule")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:716:13
	addExtern< const char * (*)(LLVMOpaqueModule *,size_t *) , LLVMGetModuleIdentifier >(*this,lib,"LLVMGetModuleIdentifier",SideEffects::worstDefault,"LLVMGetModuleIdentifier")
		->args({"M","Len"});
// from D:\Work\libclang\include\llvm-c/Core.h:726:6
	addExtern< void (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMSetModuleIdentifier >(*this,lib,"LLVMSetModuleIdentifier",SideEffects::worstDefault,"LLVMSetModuleIdentifier")
		->args({"M","Ident","Len"});
// from D:\Work\libclang\include\llvm-c/Core.h:736:13
	addExtern< const char * (*)(LLVMOpaqueModule *,size_t *) , LLVMGetSourceFileName >(*this,lib,"LLVMGetSourceFileName",SideEffects::worstDefault,"LLVMGetSourceFileName")
		->args({"M","Len"});
// from D:\Work\libclang\include\llvm-c/Core.h:747:6
	addExtern< void (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMSetSourceFileName >(*this,lib,"LLVMSetSourceFileName",SideEffects::worstDefault,"LLVMSetSourceFileName")
		->args({"M","Name","Len"});
// from D:\Work\libclang\include\llvm-c/Core.h:758:13
	addExtern< const char * (*)(LLVMOpaqueModule *) , LLVMGetDataLayoutStr >(*this,lib,"LLVMGetDataLayoutStr",SideEffects::worstDefault,"LLVMGetDataLayoutStr")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:759:13
	addExtern< const char * (*)(LLVMOpaqueModule *) , LLVMGetDataLayout >(*this,lib,"LLVMGetDataLayout",SideEffects::worstDefault,"LLVMGetDataLayout")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:766:6
	addExtern< void (*)(LLVMOpaqueModule *,const char *) , LLVMSetDataLayout >(*this,lib,"LLVMSetDataLayout",SideEffects::worstDefault,"LLVMSetDataLayout")
		->args({"M","DataLayoutStr"});
// from D:\Work\libclang\include\llvm-c/Core.h:773:13
	addExtern< const char * (*)(LLVMOpaqueModule *) , LLVMGetTarget >(*this,lib,"LLVMGetTarget",SideEffects::worstDefault,"LLVMGetTarget")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:780:6
	addExtern< void (*)(LLVMOpaqueModule *,const char *) , LLVMSetTarget >(*this,lib,"LLVMSetTarget",SideEffects::worstDefault,"LLVMSetTarget")
		->args({"M","Triple"});
// from D:\Work\libclang\include\llvm-c/Core.h:789:22
	addExtern< LLVMOpaqueModuleFlagEntry * (*)(LLVMOpaqueModule *,size_t *) , LLVMCopyModuleFlagsMetadata >(*this,lib,"LLVMCopyModuleFlagsMetadata",SideEffects::worstDefault,"LLVMCopyModuleFlagsMetadata")
		->args({"M","Len"});
// from D:\Work\libclang\include\llvm-c/Core.h:794:6
	addExtern< void (*)(LLVMOpaqueModuleFlagEntry *) , LLVMDisposeModuleFlagsMetadata >(*this,lib,"LLVMDisposeModuleFlagsMetadata",SideEffects::worstDefault,"LLVMDisposeModuleFlagsMetadata")
		->args({"Entries"});
// from D:\Work\libclang\include\llvm-c/Core.h:802:1
	addExtern< LLVMModuleFlagBehavior (*)(LLVMOpaqueModuleFlagEntry *,unsigned int) , LLVMModuleFlagEntriesGetFlagBehavior >(*this,lib,"LLVMModuleFlagEntriesGetFlagBehavior",SideEffects::worstDefault,"LLVMModuleFlagEntriesGetFlagBehavior")
		->args({"Entries","Index"});
// from D:\Work\libclang\include\llvm-c/Core.h:810:13
	addExtern< const char * (*)(LLVMOpaqueModuleFlagEntry *,unsigned int,size_t *) , LLVMModuleFlagEntriesGetKey >(*this,lib,"LLVMModuleFlagEntriesGetKey",SideEffects::worstDefault,"LLVMModuleFlagEntriesGetKey")
		->args({"Entries","Index","Len"});
// from D:\Work\libclang\include\llvm-c/Core.h:818:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueModuleFlagEntry *,unsigned int) , LLVMModuleFlagEntriesGetMetadata >(*this,lib,"LLVMModuleFlagEntriesGetMetadata",SideEffects::worstDefault,"LLVMModuleFlagEntriesGetMetadata")
		->args({"Entries","Index"});
// from D:\Work\libclang\include\llvm-c/Core.h:827:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetModuleFlag >(*this,lib,"LLVMGetModuleFlag",SideEffects::worstDefault,"LLVMGetModuleFlag")
		->args({"M","Key","KeyLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:836:6
	addExtern< void (*)(LLVMOpaqueModule *,LLVMModuleFlagBehavior,const char *,size_t,LLVMOpaqueMetadata *) , LLVMAddModuleFlag >(*this,lib,"LLVMAddModuleFlag",SideEffects::worstDefault,"LLVMAddModuleFlag")
		->args({"M","Behavior","Key","KeyLen","Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:845:6
	addExtern< void (*)(LLVMOpaqueModule *) , LLVMDumpModule >(*this,lib,"LLVMDumpModule",SideEffects::worstDefault,"LLVMDumpModule")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Core.h:853:10
	addExtern< int (*)(LLVMOpaqueModule *,const char *,char **) , LLVMPrintModuleToFile >(*this,lib,"LLVMPrintModuleToFile",SideEffects::worstDefault,"LLVMPrintModuleToFile")
		->args({"M","Filename","ErrorMessage"});
// from D:\Work\libclang\include\llvm-c/Core.h:862:7
	addExtern< char * (*)(LLVMOpaqueModule *) , LLVMPrintModuleToString >(*this,lib,"LLVMPrintModuleToString",SideEffects::worstDefault,"LLVMPrintModuleToString")
		->args({"M"});
}
}


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
// from D:\Work\libclang\include\llvm-c/Core.h:710:15
	makeExtern< LLVMOpaqueModule * (*)(LLVMOpaqueModule *) , LLVMCloneModule , SimNode_ExtFuncCall >(lib,"LLVMCloneModule","LLVMCloneModule")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:718:6
	makeExtern< void (*)(LLVMOpaqueModule *) , LLVMDisposeModule , SimNode_ExtFuncCall >(lib,"LLVMDisposeModule","LLVMDisposeModule")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:728:13
	makeExtern< const char * (*)(LLVMOpaqueModule *,size_t *) , LLVMGetModuleIdentifier , SimNode_ExtFuncCall >(lib,"LLVMGetModuleIdentifier","LLVMGetModuleIdentifier")
		->args({"M","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:738:6
	makeExtern< void (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMSetModuleIdentifier , SimNode_ExtFuncCall >(lib,"LLVMSetModuleIdentifier","LLVMSetModuleIdentifier")
		->args({"M","Ident","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:748:13
	makeExtern< const char * (*)(LLVMOpaqueModule *,size_t *) , LLVMGetSourceFileName , SimNode_ExtFuncCall >(lib,"LLVMGetSourceFileName","LLVMGetSourceFileName")
		->args({"M","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:759:6
	makeExtern< void (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMSetSourceFileName , SimNode_ExtFuncCall >(lib,"LLVMSetSourceFileName","LLVMSetSourceFileName")
		->args({"M","Name","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:770:13
	makeExtern< const char * (*)(LLVMOpaqueModule *) , LLVMGetDataLayoutStr , SimNode_ExtFuncCall >(lib,"LLVMGetDataLayoutStr","LLVMGetDataLayoutStr")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:771:13
	makeExtern< const char * (*)(LLVMOpaqueModule *) , LLVMGetDataLayout , SimNode_ExtFuncCall >(lib,"LLVMGetDataLayout","LLVMGetDataLayout")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:778:6
	makeExtern< void (*)(LLVMOpaqueModule *,const char *) , LLVMSetDataLayout , SimNode_ExtFuncCall >(lib,"LLVMSetDataLayout","LLVMSetDataLayout")
		->args({"M","DataLayoutStr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:785:13
	makeExtern< const char * (*)(LLVMOpaqueModule *) , LLVMGetTarget , SimNode_ExtFuncCall >(lib,"LLVMGetTarget","LLVMGetTarget")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:792:6
	makeExtern< void (*)(LLVMOpaqueModule *,const char *) , LLVMSetTarget , SimNode_ExtFuncCall >(lib,"LLVMSetTarget","LLVMSetTarget")
		->args({"M","Triple"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:801:22
	makeExtern< LLVMOpaqueModuleFlagEntry * (*)(LLVMOpaqueModule *,size_t *) , LLVMCopyModuleFlagsMetadata , SimNode_ExtFuncCall >(lib,"LLVMCopyModuleFlagsMetadata","LLVMCopyModuleFlagsMetadata")
		->args({"M","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:806:6
	makeExtern< void (*)(LLVMOpaqueModuleFlagEntry *) , LLVMDisposeModuleFlagsMetadata , SimNode_ExtFuncCall >(lib,"LLVMDisposeModuleFlagsMetadata","LLVMDisposeModuleFlagsMetadata")
		->args({"Entries"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:814:1
	makeExtern< LLVMModuleFlagBehavior (*)(LLVMOpaqueModuleFlagEntry *,unsigned int) , LLVMModuleFlagEntriesGetFlagBehavior , SimNode_ExtFuncCall >(lib,"LLVMModuleFlagEntriesGetFlagBehavior","LLVMModuleFlagEntriesGetFlagBehavior")
		->args({"Entries","Index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:822:13
	makeExtern< const char * (*)(LLVMOpaqueModuleFlagEntry *,unsigned int,size_t *) , LLVMModuleFlagEntriesGetKey , SimNode_ExtFuncCall >(lib,"LLVMModuleFlagEntriesGetKey","LLVMModuleFlagEntriesGetKey")
		->args({"Entries","Index","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:830:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueModuleFlagEntry *,unsigned int) , LLVMModuleFlagEntriesGetMetadata , SimNode_ExtFuncCall >(lib,"LLVMModuleFlagEntriesGetMetadata","LLVMModuleFlagEntriesGetMetadata")
		->args({"Entries","Index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:839:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetModuleFlag , SimNode_ExtFuncCall >(lib,"LLVMGetModuleFlag","LLVMGetModuleFlag")
		->args({"M","Key","KeyLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:848:6
	makeExtern< void (*)(LLVMOpaqueModule *,LLVMModuleFlagBehavior,const char *,size_t,LLVMOpaqueMetadata *) , LLVMAddModuleFlag , SimNode_ExtFuncCall >(lib,"LLVMAddModuleFlag","LLVMAddModuleFlag")
		->args({"M","Behavior","Key","KeyLen","Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:857:6
	makeExtern< void (*)(LLVMOpaqueModule *) , LLVMDumpModule , SimNode_ExtFuncCall >(lib,"LLVMDumpModule","LLVMDumpModule")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:865:10
	makeExtern< int (*)(LLVMOpaqueModule *,const char *,char **) , LLVMPrintModuleToFile , SimNode_ExtFuncCall >(lib,"LLVMPrintModuleToFile","LLVMPrintModuleToFile")
		->args({"M","Filename","ErrorMessage"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


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
// from D:\Work\libclang\include\llvm-c/Core.h:874:7
	makeExtern< char * (*)(LLVMOpaqueModule *) , LLVMPrintModuleToString , SimNode_ExtFuncCall >(lib,"LLVMPrintModuleToString","LLVMPrintModuleToString")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:881:13
	makeExtern< const char * (*)(LLVMOpaqueModule *,size_t *) , LLVMGetModuleInlineAsm , SimNode_ExtFuncCall >(lib,"LLVMGetModuleInlineAsm","LLVMGetModuleInlineAsm")
		->args({"M","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:888:6
	makeExtern< void (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMSetModuleInlineAsm2 , SimNode_ExtFuncCall >(lib,"LLVMSetModuleInlineAsm2","LLVMSetModuleInlineAsm2")
		->args({"M","Asm","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:895:6
	makeExtern< void (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMAppendModuleInlineAsm , SimNode_ExtFuncCall >(lib,"LLVMAppendModuleInlineAsm","LLVMAppendModuleInlineAsm")
		->args({"M","Asm","Len"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:902:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,char *,size_t,char *,size_t,int,int,LLVMInlineAsmDialect,int) , LLVMGetInlineAsm , SimNode_ExtFuncCall >(lib,"LLVMGetInlineAsm","LLVMGetInlineAsm")
		->args({"Ty","AsmString","AsmStringSize","Constraints","ConstraintsSize","HasSideEffects","IsAlignStack","Dialect","CanThrow"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:913:16
	makeExtern< LLVMOpaqueContext * (*)(LLVMOpaqueModule *) , LLVMGetModuleContext , SimNode_ExtFuncCall >(lib,"LLVMGetModuleContext","LLVMGetModuleContext")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:916:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueModule *,const char *) , LLVMGetTypeByName , SimNode_ExtFuncCall >(lib,"LLVMGetTypeByName","LLVMGetTypeByName")
		->args({"M","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:923:20
	makeExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueModule *) , LLVMGetFirstNamedMetadata , SimNode_ExtFuncCall >(lib,"LLVMGetFirstNamedMetadata","LLVMGetFirstNamedMetadata")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:930:20
	makeExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueModule *) , LLVMGetLastNamedMetadata , SimNode_ExtFuncCall >(lib,"LLVMGetLastNamedMetadata","LLVMGetLastNamedMetadata")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:938:20
	makeExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueNamedMDNode *) , LLVMGetNextNamedMetadata , SimNode_ExtFuncCall >(lib,"LLVMGetNextNamedMetadata","LLVMGetNextNamedMetadata")
		->args({"NamedMDNode"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:946:20
	makeExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueNamedMDNode *) , LLVMGetPreviousNamedMetadata , SimNode_ExtFuncCall >(lib,"LLVMGetPreviousNamedMetadata","LLVMGetPreviousNamedMetadata")
		->args({"NamedMDNode"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:954:20
	makeExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetNamedMetadata , SimNode_ExtFuncCall >(lib,"LLVMGetNamedMetadata","LLVMGetNamedMetadata")
		->args({"M","Name","NameLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:963:20
	makeExtern< LLVMOpaqueNamedMDNode * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetOrInsertNamedMetadata , SimNode_ExtFuncCall >(lib,"LLVMGetOrInsertNamedMetadata","LLVMGetOrInsertNamedMetadata")
		->args({"M","Name","NameLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:972:13
	makeExtern< const char * (*)(LLVMOpaqueNamedMDNode *,size_t *) , LLVMGetNamedMetadataName , SimNode_ExtFuncCall >(lib,"LLVMGetNamedMetadataName","LLVMGetNamedMetadataName")
		->args({"NamedMD","NameLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:980:10
	makeExtern< unsigned int (*)(LLVMOpaqueModule *,const char *) , LLVMGetNamedMetadataNumOperands , SimNode_ExtFuncCall >(lib,"LLVMGetNamedMetadataNumOperands","LLVMGetNamedMetadataNumOperands")
		->args({"M","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:993:6
	makeExtern< void (*)(LLVMOpaqueModule *,const char *,LLVMOpaqueValue **) , LLVMGetNamedMetadataOperands , SimNode_ExtFuncCall >(lib,"LLVMGetNamedMetadataOperands","LLVMGetNamedMetadataOperands")
		->args({"M","Name","Dest"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1002:6
	makeExtern< void (*)(LLVMOpaqueModule *,const char *,LLVMOpaqueValue *) , LLVMAddNamedMetadataOperand , SimNode_ExtFuncCall >(lib,"LLVMAddNamedMetadataOperand","LLVMAddNamedMetadataOperand")
		->args({"M","Name","Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1013:13
	makeExtern< const char * (*)(LLVMOpaqueValue *,unsigned int *) , LLVMGetDebugLocDirectory , SimNode_ExtFuncCall >(lib,"LLVMGetDebugLocDirectory","LLVMGetDebugLocDirectory")
		->args({"Val","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1023:13
	makeExtern< const char * (*)(LLVMOpaqueValue *,unsigned int *) , LLVMGetDebugLocFilename , SimNode_ExtFuncCall >(lib,"LLVMGetDebugLocFilename","LLVMGetDebugLocFilename")
		->args({"Val","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1033:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetDebugLocLine , SimNode_ExtFuncCall >(lib,"LLVMGetDebugLocLine","LLVMGetDebugLocLine")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


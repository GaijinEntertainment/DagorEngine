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
void Module_dasLLVM::initFunctions_3() {
// from D:\Work\libclang\include\llvm-c/Core.h:585:24
	addExtern< LLVMDiagnosticSeverity (*)(LLVMOpaqueDiagnosticInfo *) , LLVMGetDiagInfoSeverity >(*this,lib,"LLVMGetDiagInfoSeverity",SideEffects::worstDefault,"LLVMGetDiagInfoSeverity")
		->args({"DI"});
// from D:\Work\libclang\include\llvm-c/Core.h:587:10
	addExtern< unsigned int (*)(LLVMOpaqueContext *,const char *,unsigned int) , LLVMGetMDKindIDInContext >(*this,lib,"LLVMGetMDKindIDInContext",SideEffects::worstDefault,"LLVMGetMDKindIDInContext")
		->args({"C","Name","SLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:589:10
	addExtern< unsigned int (*)(const char *,unsigned int) , LLVMGetMDKindID >(*this,lib,"LLVMGetMDKindID",SideEffects::worstDefault,"LLVMGetMDKindID")
		->args({"Name","SLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:602:10
	addExtern< unsigned int (*)(const char *,size_t) , LLVMGetEnumAttributeKindForName >(*this,lib,"LLVMGetEnumAttributeKindForName",SideEffects::worstDefault,"LLVMGetEnumAttributeKindForName")
		->args({"Name","SLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:603:10
	addExtern< unsigned int (*)() , LLVMGetLastEnumAttributeKind >(*this,lib,"LLVMGetLastEnumAttributeKind",SideEffects::worstDefault,"LLVMGetLastEnumAttributeKind");
// from D:\Work\libclang\include\llvm-c/Core.h:608:18
	addExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueContext *,unsigned int,uint64_t) , LLVMCreateEnumAttribute >(*this,lib,"LLVMCreateEnumAttribute",SideEffects::worstDefault,"LLVMCreateEnumAttribute")
		->args({"C","KindID","Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:615:10
	addExtern< unsigned int (*)(LLVMOpaqueAttributeRef *) , LLVMGetEnumAttributeKind >(*this,lib,"LLVMGetEnumAttributeKind",SideEffects::worstDefault,"LLVMGetEnumAttributeKind")
		->args({"A"});
// from D:\Work\libclang\include\llvm-c/Core.h:620:10
	addExtern< uint64_t (*)(LLVMOpaqueAttributeRef *) , LLVMGetEnumAttributeValue >(*this,lib,"LLVMGetEnumAttributeValue",SideEffects::worstDefault,"LLVMGetEnumAttributeValue")
		->args({"A"});
// from D:\Work\libclang\include\llvm-c/Core.h:625:18
	addExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueContext *,unsigned int,LLVMOpaqueType *) , LLVMCreateTypeAttribute >(*this,lib,"LLVMCreateTypeAttribute",SideEffects::worstDefault,"LLVMCreateTypeAttribute")
		->args({"C","KindID","type_ref"});
// from D:\Work\libclang\include\llvm-c/Core.h:631:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueAttributeRef *) , LLVMGetTypeAttributeValue >(*this,lib,"LLVMGetTypeAttributeValue",SideEffects::worstDefault,"LLVMGetTypeAttributeValue")
		->args({"A"});
// from D:\Work\libclang\include\llvm-c/Core.h:636:18
	addExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueContext *,const char *,unsigned int,const char *,unsigned int) , LLVMCreateStringAttribute >(*this,lib,"LLVMCreateStringAttribute",SideEffects::worstDefault,"LLVMCreateStringAttribute")
		->args({"C","K","KLength","V","VLength"});
// from D:\Work\libclang\include\llvm-c/Core.h:643:13
	addExtern< const char * (*)(LLVMOpaqueAttributeRef *,unsigned int *) , LLVMGetStringAttributeKind >(*this,lib,"LLVMGetStringAttributeKind",SideEffects::worstDefault,"LLVMGetStringAttributeKind")
		->args({"A","Length"});
// from D:\Work\libclang\include\llvm-c/Core.h:648:13
	addExtern< const char * (*)(LLVMOpaqueAttributeRef *,unsigned int *) , LLVMGetStringAttributeValue >(*this,lib,"LLVMGetStringAttributeValue",SideEffects::worstDefault,"LLVMGetStringAttributeValue")
		->args({"A","Length"});
// from D:\Work\libclang\include\llvm-c/Core.h:653:10
	addExtern< int (*)(LLVMOpaqueAttributeRef *) , LLVMIsEnumAttribute >(*this,lib,"LLVMIsEnumAttribute",SideEffects::worstDefault,"LLVMIsEnumAttribute")
		->args({"A"});
// from D:\Work\libclang\include\llvm-c/Core.h:654:10
	addExtern< int (*)(LLVMOpaqueAttributeRef *) , LLVMIsStringAttribute >(*this,lib,"LLVMIsStringAttribute",SideEffects::worstDefault,"LLVMIsStringAttribute")
		->args({"A"});
// from D:\Work\libclang\include\llvm-c/Core.h:655:10
	addExtern< int (*)(LLVMOpaqueAttributeRef *) , LLVMIsTypeAttribute >(*this,lib,"LLVMIsTypeAttribute",SideEffects::worstDefault,"LLVMIsTypeAttribute")
		->args({"A"});
// from D:\Work\libclang\include\llvm-c/Core.h:660:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,const char *) , LLVMGetTypeByName2 >(*this,lib,"LLVMGetTypeByName2",SideEffects::worstDefault,"LLVMGetTypeByName2")
		->args({"C","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:685:15
	addExtern< LLVMOpaqueModule * (*)(const char *) , LLVMModuleCreateWithName >(*this,lib,"LLVMModuleCreateWithName",SideEffects::worstDefault,"LLVMModuleCreateWithName")
		->args({"ModuleID"});
// from D:\Work\libclang\include\llvm-c/Core.h:693:15
	addExtern< LLVMOpaqueModule * (*)(const char *,LLVMOpaqueContext *) , LLVMModuleCreateWithNameInContext >(*this,lib,"LLVMModuleCreateWithNameInContext",SideEffects::worstDefault,"LLVMModuleCreateWithNameInContext")
		->args({"ModuleID","C"});
// from D:\Work\libclang\include\llvm-c/Core.h:698:15
	addExtern< LLVMOpaqueModule * (*)(LLVMOpaqueModule *) , LLVMCloneModule >(*this,lib,"LLVMCloneModule",SideEffects::worstDefault,"LLVMCloneModule")
		->args({"M"});
}
}


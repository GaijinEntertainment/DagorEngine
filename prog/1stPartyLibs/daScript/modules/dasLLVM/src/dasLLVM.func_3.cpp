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
// from D:\Work\libclang\include\llvm-c/Core.h:590:7
	makeExtern< char * (*)(LLVMOpaqueDiagnosticInfo *) , LLVMGetDiagInfoDescription , SimNode_ExtFuncCall >(lib,"LLVMGetDiagInfoDescription","LLVMGetDiagInfoDescription")
		->args({"DI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:597:24
	makeExtern< LLVMDiagnosticSeverity (*)(LLVMOpaqueDiagnosticInfo *) , LLVMGetDiagInfoSeverity , SimNode_ExtFuncCall >(lib,"LLVMGetDiagInfoSeverity","LLVMGetDiagInfoSeverity")
		->args({"DI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:599:10
	makeExtern< unsigned int (*)(LLVMOpaqueContext *,const char *,unsigned int) , LLVMGetMDKindIDInContext , SimNode_ExtFuncCall >(lib,"LLVMGetMDKindIDInContext","LLVMGetMDKindIDInContext")
		->args({"C","Name","SLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:601:10
	makeExtern< unsigned int (*)(const char *,unsigned int) , LLVMGetMDKindID , SimNode_ExtFuncCall >(lib,"LLVMGetMDKindID","LLVMGetMDKindID")
		->args({"Name","SLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:614:10
	makeExtern< unsigned int (*)(const char *,size_t) , LLVMGetEnumAttributeKindForName , SimNode_ExtFuncCall >(lib,"LLVMGetEnumAttributeKindForName","LLVMGetEnumAttributeKindForName")
		->args({"Name","SLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:615:10
	makeExtern< unsigned int (*)() , LLVMGetLastEnumAttributeKind , SimNode_ExtFuncCall >(lib,"LLVMGetLastEnumAttributeKind","LLVMGetLastEnumAttributeKind")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:620:18
	makeExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueContext *,unsigned int,uint64_t) , LLVMCreateEnumAttribute , SimNode_ExtFuncCall >(lib,"LLVMCreateEnumAttribute","LLVMCreateEnumAttribute")
		->args({"C","KindID","Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:627:10
	makeExtern< unsigned int (*)(LLVMOpaqueAttributeRef *) , LLVMGetEnumAttributeKind , SimNode_ExtFuncCall >(lib,"LLVMGetEnumAttributeKind","LLVMGetEnumAttributeKind")
		->args({"A"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:632:10
	makeExtern< uint64_t (*)(LLVMOpaqueAttributeRef *) , LLVMGetEnumAttributeValue , SimNode_ExtFuncCall >(lib,"LLVMGetEnumAttributeValue","LLVMGetEnumAttributeValue")
		->args({"A"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:637:18
	makeExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueContext *,unsigned int,LLVMOpaqueType *) , LLVMCreateTypeAttribute , SimNode_ExtFuncCall >(lib,"LLVMCreateTypeAttribute","LLVMCreateTypeAttribute")
		->args({"C","KindID","type_ref"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:643:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueAttributeRef *) , LLVMGetTypeAttributeValue , SimNode_ExtFuncCall >(lib,"LLVMGetTypeAttributeValue","LLVMGetTypeAttributeValue")
		->args({"A"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:648:18
	makeExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueContext *,const char *,unsigned int,const char *,unsigned int) , LLVMCreateStringAttribute , SimNode_ExtFuncCall >(lib,"LLVMCreateStringAttribute","LLVMCreateStringAttribute")
		->args({"C","K","KLength","V","VLength"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:655:13
	makeExtern< const char * (*)(LLVMOpaqueAttributeRef *,unsigned int *) , LLVMGetStringAttributeKind , SimNode_ExtFuncCall >(lib,"LLVMGetStringAttributeKind","LLVMGetStringAttributeKind")
		->args({"A","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:660:13
	makeExtern< const char * (*)(LLVMOpaqueAttributeRef *,unsigned int *) , LLVMGetStringAttributeValue , SimNode_ExtFuncCall >(lib,"LLVMGetStringAttributeValue","LLVMGetStringAttributeValue")
		->args({"A","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:665:10
	makeExtern< int (*)(LLVMOpaqueAttributeRef *) , LLVMIsEnumAttribute , SimNode_ExtFuncCall >(lib,"LLVMIsEnumAttribute","LLVMIsEnumAttribute")
		->args({"A"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:666:10
	makeExtern< int (*)(LLVMOpaqueAttributeRef *) , LLVMIsStringAttribute , SimNode_ExtFuncCall >(lib,"LLVMIsStringAttribute","LLVMIsStringAttribute")
		->args({"A"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:667:10
	makeExtern< int (*)(LLVMOpaqueAttributeRef *) , LLVMIsTypeAttribute , SimNode_ExtFuncCall >(lib,"LLVMIsTypeAttribute","LLVMIsTypeAttribute")
		->args({"A"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:672:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,const char *) , LLVMGetTypeByName2 , SimNode_ExtFuncCall >(lib,"LLVMGetTypeByName2","LLVMGetTypeByName2")
		->args({"C","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:697:15
	makeExtern< LLVMOpaqueModule * (*)(const char *) , LLVMModuleCreateWithName , SimNode_ExtFuncCall >(lib,"LLVMModuleCreateWithName","LLVMModuleCreateWithName")
		->args({"ModuleID"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:705:15
	makeExtern< LLVMOpaqueModule * (*)(const char *,LLVMOpaqueContext *) , LLVMModuleCreateWithNameInContext , SimNode_ExtFuncCall >(lib,"LLVMModuleCreateWithNameInContext","LLVMModuleCreateWithNameInContext")
		->args({"ModuleID","C"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


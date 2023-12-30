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
void Module_dasLLVM::initFunctions_22() {
// from D:\Work\libclang\include\llvm-c/Core.h:2504:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMHasPersonalityFn >(*this,lib,"LLVMHasPersonalityFn",SideEffects::worstDefault,"LLVMHasPersonalityFn")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:2511:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPersonalityFn >(*this,lib,"LLVMGetPersonalityFn",SideEffects::worstDefault,"LLVMGetPersonalityFn")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:2518:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetPersonalityFn >(*this,lib,"LLVMSetPersonalityFn",SideEffects::worstDefault,"LLVMSetPersonalityFn")
		->args({"Fn","PersonalityFn"});
// from D:\Work\libclang\include\llvm-c/Core.h:2525:10
	addExtern< unsigned int (*)(const char *,size_t) , LLVMLookupIntrinsicID >(*this,lib,"LLVMLookupIntrinsicID",SideEffects::worstDefault,"LLVMLookupIntrinsicID")
		->args({"Name","NameLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:2532:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetIntrinsicID >(*this,lib,"LLVMGetIntrinsicID",SideEffects::worstDefault,"LLVMGetIntrinsicID")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:2540:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,unsigned int,LLVMOpaqueType **,size_t) , LLVMGetIntrinsicDeclaration >(*this,lib,"LLVMGetIntrinsicDeclaration",SideEffects::worstDefault,"LLVMGetIntrinsicDeclaration")
		->args({"Mod","ID","ParamTypes","ParamCount"});
// from D:\Work\libclang\include\llvm-c/Core.h:2551:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,unsigned int,LLVMOpaqueType **,size_t) , LLVMIntrinsicGetType >(*this,lib,"LLVMIntrinsicGetType",SideEffects::worstDefault,"LLVMIntrinsicGetType")
		->args({"Ctx","ID","ParamTypes","ParamCount"});
// from D:\Work\libclang\include\llvm-c/Core.h:2559:13
	addExtern< const char * (*)(unsigned int,size_t *) , LLVMIntrinsicGetName >(*this,lib,"LLVMIntrinsicGetName",SideEffects::worstDefault,"LLVMIntrinsicGetName")
		->args({"ID","NameLength"});
// from D:\Work\libclang\include\llvm-c/Core.h:2562:13
	addExtern< const char * (*)(unsigned int,LLVMOpaqueType **,size_t,size_t *) , LLVMIntrinsicCopyOverloadedName >(*this,lib,"LLVMIntrinsicCopyOverloadedName",SideEffects::worstDefault,"LLVMIntrinsicCopyOverloadedName")
		->args({"ID","ParamTypes","ParamCount","NameLength"});
// from D:\Work\libclang\include\llvm-c/Core.h:2578:13
	addExtern< const char * (*)(LLVMOpaqueModule *,unsigned int,LLVMOpaqueType **,size_t,size_t *) , LLVMIntrinsicCopyOverloadedName2 >(*this,lib,"LLVMIntrinsicCopyOverloadedName2",SideEffects::worstDefault,"LLVMIntrinsicCopyOverloadedName2")
		->args({"Mod","ID","ParamTypes","ParamCount","NameLength"});
// from D:\Work\libclang\include\llvm-c/Core.h:2588:10
	addExtern< int (*)(unsigned int) , LLVMIntrinsicIsOverloaded >(*this,lib,"LLVMIntrinsicIsOverloaded",SideEffects::worstDefault,"LLVMIntrinsicIsOverloaded")
		->args({"ID"});
// from D:\Work\libclang\include\llvm-c/Core.h:2597:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetFunctionCallConv >(*this,lib,"LLVMGetFunctionCallConv",SideEffects::worstDefault,"LLVMGetFunctionCallConv")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:2607:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMSetFunctionCallConv >(*this,lib,"LLVMSetFunctionCallConv",SideEffects::worstDefault,"LLVMSetFunctionCallConv")
		->args({"Fn","CC"});
// from D:\Work\libclang\include\llvm-c/Core.h:2615:13
	addExtern< const char * (*)(LLVMOpaqueValue *) , LLVMGetGC >(*this,lib,"LLVMGetGC",SideEffects::worstDefault,"LLVMGetGC")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:2622:6
	addExtern< void (*)(LLVMOpaqueValue *,const char *) , LLVMSetGC >(*this,lib,"LLVMSetGC",SideEffects::worstDefault,"LLVMSetGC")
		->args({"Fn","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:2629:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueAttributeRef *) , LLVMAddAttributeAtIndex >(*this,lib,"LLVMAddAttributeAtIndex",SideEffects::worstDefault,"LLVMAddAttributeAtIndex")
		->args({"F","Idx","A"});
// from D:\Work\libclang\include\llvm-c/Core.h:2631:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetAttributeCountAtIndex >(*this,lib,"LLVMGetAttributeCountAtIndex",SideEffects::worstDefault,"LLVMGetAttributeCountAtIndex")
		->args({"F","Idx"});
// from D:\Work\libclang\include\llvm-c/Core.h:2632:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueAttributeRef **) , LLVMGetAttributesAtIndex >(*this,lib,"LLVMGetAttributesAtIndex",SideEffects::worstDefault,"LLVMGetAttributesAtIndex")
		->args({"F","Idx","Attrs"});
// from D:\Work\libclang\include\llvm-c/Core.h:2634:18
	addExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMGetEnumAttributeAtIndex >(*this,lib,"LLVMGetEnumAttributeAtIndex",SideEffects::worstDefault,"LLVMGetEnumAttributeAtIndex")
		->args({"F","Idx","KindID"});
// from D:\Work\libclang\include\llvm-c/Core.h:2637:18
	addExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueValue *,unsigned int,const char *,unsigned int) , LLVMGetStringAttributeAtIndex >(*this,lib,"LLVMGetStringAttributeAtIndex",SideEffects::worstDefault,"LLVMGetStringAttributeAtIndex")
		->args({"F","Idx","K","KLen"});
}
}


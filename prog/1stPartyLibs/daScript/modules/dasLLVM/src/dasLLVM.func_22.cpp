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
// from D:\Work\libclang\include\llvm-c/Core.h:2523:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetPersonalityFn , SimNode_ExtFuncCall >(lib,"LLVMSetPersonalityFn","LLVMSetPersonalityFn")
		->args({"Fn","PersonalityFn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2530:10
	makeExtern< unsigned int (*)(const char *,size_t) , LLVMLookupIntrinsicID , SimNode_ExtFuncCall >(lib,"LLVMLookupIntrinsicID","LLVMLookupIntrinsicID")
		->args({"Name","NameLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2537:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetIntrinsicID , SimNode_ExtFuncCall >(lib,"LLVMGetIntrinsicID","LLVMGetIntrinsicID")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2545:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,unsigned int,LLVMOpaqueType **,size_t) , LLVMGetIntrinsicDeclaration , SimNode_ExtFuncCall >(lib,"LLVMGetIntrinsicDeclaration","LLVMGetIntrinsicDeclaration")
		->args({"Mod","ID","ParamTypes","ParamCount"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2556:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,unsigned int,LLVMOpaqueType **,size_t) , LLVMIntrinsicGetType , SimNode_ExtFuncCall >(lib,"LLVMIntrinsicGetType","LLVMIntrinsicGetType")
		->args({"Ctx","ID","ParamTypes","ParamCount"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2564:13
	makeExtern< const char * (*)(unsigned int,size_t *) , LLVMIntrinsicGetName , SimNode_ExtFuncCall >(lib,"LLVMIntrinsicGetName","LLVMIntrinsicGetName")
		->args({"ID","NameLength"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2567:13
	makeExtern< const char * (*)(unsigned int,LLVMOpaqueType **,size_t,size_t *) , LLVMIntrinsicCopyOverloadedName , SimNode_ExtFuncCall >(lib,"LLVMIntrinsicCopyOverloadedName","LLVMIntrinsicCopyOverloadedName")
		->args({"ID","ParamTypes","ParamCount","NameLength"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2583:13
	makeExtern< const char * (*)(LLVMOpaqueModule *,unsigned int,LLVMOpaqueType **,size_t,size_t *) , LLVMIntrinsicCopyOverloadedName2 , SimNode_ExtFuncCall >(lib,"LLVMIntrinsicCopyOverloadedName2","LLVMIntrinsicCopyOverloadedName2")
		->args({"Mod","ID","ParamTypes","ParamCount","NameLength"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2593:10
	makeExtern< int (*)(unsigned int) , LLVMIntrinsicIsOverloaded , SimNode_ExtFuncCall >(lib,"LLVMIntrinsicIsOverloaded","LLVMIntrinsicIsOverloaded")
		->args({"ID"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2602:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetFunctionCallConv , SimNode_ExtFuncCall >(lib,"LLVMGetFunctionCallConv","LLVMGetFunctionCallConv")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2612:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMSetFunctionCallConv , SimNode_ExtFuncCall >(lib,"LLVMSetFunctionCallConv","LLVMSetFunctionCallConv")
		->args({"Fn","CC"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2620:13
	makeExtern< const char * (*)(LLVMOpaqueValue *) , LLVMGetGC , SimNode_ExtFuncCall >(lib,"LLVMGetGC","LLVMGetGC")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2627:6
	makeExtern< void (*)(LLVMOpaqueValue *,const char *) , LLVMSetGC , SimNode_ExtFuncCall >(lib,"LLVMSetGC","LLVMSetGC")
		->args({"Fn","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2634:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueAttributeRef *) , LLVMAddAttributeAtIndex , SimNode_ExtFuncCall >(lib,"LLVMAddAttributeAtIndex","LLVMAddAttributeAtIndex")
		->args({"F","Idx","A"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2636:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetAttributeCountAtIndex , SimNode_ExtFuncCall >(lib,"LLVMGetAttributeCountAtIndex","LLVMGetAttributeCountAtIndex")
		->args({"F","Idx"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2637:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueAttributeRef **) , LLVMGetAttributesAtIndex , SimNode_ExtFuncCall >(lib,"LLVMGetAttributesAtIndex","LLVMGetAttributesAtIndex")
		->args({"F","Idx","Attrs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2639:18
	makeExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMGetEnumAttributeAtIndex , SimNode_ExtFuncCall >(lib,"LLVMGetEnumAttributeAtIndex","LLVMGetEnumAttributeAtIndex")
		->args({"F","Idx","KindID"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2642:18
	makeExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueValue *,unsigned int,const char *,unsigned int) , LLVMGetStringAttributeAtIndex , SimNode_ExtFuncCall >(lib,"LLVMGetStringAttributeAtIndex","LLVMGetStringAttributeAtIndex")
		->args({"F","Idx","K","KLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2645:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMRemoveEnumAttributeAtIndex , SimNode_ExtFuncCall >(lib,"LLVMRemoveEnumAttributeAtIndex","LLVMRemoveEnumAttributeAtIndex")
		->args({"F","Idx","KindID"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2647:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,const char *,unsigned int) , LLVMRemoveStringAttributeAtIndex , SimNode_ExtFuncCall >(lib,"LLVMRemoveStringAttributeAtIndex","LLVMRemoveStringAttributeAtIndex")
		->args({"F","Idx","K","KLen"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


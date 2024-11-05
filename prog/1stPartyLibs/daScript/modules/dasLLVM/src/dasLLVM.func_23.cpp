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
void Module_dasLLVM::initFunctions_23() {
// from D:\Work\libclang\include\llvm-c/Core.h:2654:6
	makeExtern< void (*)(LLVMOpaqueValue *,const char *,const char *) , LLVMAddTargetDependentFunctionAttr , SimNode_ExtFuncCall >(lib,"LLVMAddTargetDependentFunctionAttr","LLVMAddTargetDependentFunctionAttr")
		->args({"Fn","A","V"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2673:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMCountParams , SimNode_ExtFuncCall >(lib,"LLVMCountParams","LLVMCountParams")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2686:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue **) , LLVMGetParams , SimNode_ExtFuncCall >(lib,"LLVMGetParams","LLVMGetParams")
		->args({"Fn","Params"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2695:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetParam , SimNode_ExtFuncCall >(lib,"LLVMGetParam","LLVMGetParam")
		->args({"Fn","Index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2706:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetParamParent , SimNode_ExtFuncCall >(lib,"LLVMGetParamParent","LLVMGetParamParent")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2713:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetFirstParam , SimNode_ExtFuncCall >(lib,"LLVMGetFirstParam","LLVMGetFirstParam")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2720:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetLastParam , SimNode_ExtFuncCall >(lib,"LLVMGetLastParam","LLVMGetLastParam")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2729:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextParam , SimNode_ExtFuncCall >(lib,"LLVMGetNextParam","LLVMGetNextParam")
		->args({"Arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2736:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousParam , SimNode_ExtFuncCall >(lib,"LLVMGetPreviousParam","LLVMGetPreviousParam")
		->args({"Arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2744:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMSetParamAlignment , SimNode_ExtFuncCall >(lib,"LLVMSetParamAlignment","LLVMSetParamAlignment")
		->args({"Arg","Align"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2766:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *,size_t,LLVMOpaqueType *,unsigned int,LLVMOpaqueValue *) , LLVMAddGlobalIFunc , SimNode_ExtFuncCall >(lib,"LLVMAddGlobalIFunc","LLVMAddGlobalIFunc")
		->args({"M","Name","NameLen","Ty","AddrSpace","Resolver"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2778:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *,const char *,size_t) , LLVMGetNamedGlobalIFunc , SimNode_ExtFuncCall >(lib,"LLVMGetNamedGlobalIFunc","LLVMGetNamedGlobalIFunc")
		->args({"M","Name","NameLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2786:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetFirstGlobalIFunc , SimNode_ExtFuncCall >(lib,"LLVMGetFirstGlobalIFunc","LLVMGetFirstGlobalIFunc")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2793:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueModule *) , LLVMGetLastGlobalIFunc , SimNode_ExtFuncCall >(lib,"LLVMGetLastGlobalIFunc","LLVMGetLastGlobalIFunc")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2801:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextGlobalIFunc , SimNode_ExtFuncCall >(lib,"LLVMGetNextGlobalIFunc","LLVMGetNextGlobalIFunc")
		->args({"IFunc"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2809:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousGlobalIFunc , SimNode_ExtFuncCall >(lib,"LLVMGetPreviousGlobalIFunc","LLVMGetPreviousGlobalIFunc")
		->args({"IFunc"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2817:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetGlobalIFuncResolver , SimNode_ExtFuncCall >(lib,"LLVMGetGlobalIFuncResolver","LLVMGetGlobalIFuncResolver")
		->args({"IFunc"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2824:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMSetGlobalIFuncResolver , SimNode_ExtFuncCall >(lib,"LLVMSetGlobalIFuncResolver","LLVMSetGlobalIFuncResolver")
		->args({"IFunc","Resolver"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2831:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMEraseGlobalIFunc , SimNode_ExtFuncCall >(lib,"LLVMEraseGlobalIFunc","LLVMEraseGlobalIFunc")
		->args({"IFunc"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2841:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMRemoveGlobalIFunc , SimNode_ExtFuncCall >(lib,"LLVMRemoveGlobalIFunc","LLVMRemoveGlobalIFunc")
		->args({"IFunc"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


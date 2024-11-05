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
void Module_dasLLVM::initFunctions_14() {
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsABitCastInst , SimNode_ExtFuncCall >(lib,"LLVMIsABitCastInst","LLVMIsABitCastInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFPExtInst , SimNode_ExtFuncCall >(lib,"LLVMIsAFPExtInst","LLVMIsAFPExtInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFPToSIInst , SimNode_ExtFuncCall >(lib,"LLVMIsAFPToSIInst","LLVMIsAFPToSIInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFPToUIInst , SimNode_ExtFuncCall >(lib,"LLVMIsAFPToUIInst","LLVMIsAFPToUIInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFPTruncInst , SimNode_ExtFuncCall >(lib,"LLVMIsAFPTruncInst","LLVMIsAFPTruncInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAIntToPtrInst , SimNode_ExtFuncCall >(lib,"LLVMIsAIntToPtrInst","LLVMIsAIntToPtrInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAPtrToIntInst , SimNode_ExtFuncCall >(lib,"LLVMIsAPtrToIntInst","LLVMIsAPtrToIntInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsASExtInst , SimNode_ExtFuncCall >(lib,"LLVMIsASExtInst","LLVMIsASExtInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsASIToFPInst , SimNode_ExtFuncCall >(lib,"LLVMIsASIToFPInst","LLVMIsASIToFPInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsATruncInst , SimNode_ExtFuncCall >(lib,"LLVMIsATruncInst","LLVMIsATruncInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAUIToFPInst , SimNode_ExtFuncCall >(lib,"LLVMIsAUIToFPInst","LLVMIsAUIToFPInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAZExtInst , SimNode_ExtFuncCall >(lib,"LLVMIsAZExtInst","LLVMIsAZExtInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAExtractValueInst , SimNode_ExtFuncCall >(lib,"LLVMIsAExtractValueInst","LLVMIsAExtractValueInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsALoadInst , SimNode_ExtFuncCall >(lib,"LLVMIsALoadInst","LLVMIsALoadInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAVAArgInst , SimNode_ExtFuncCall >(lib,"LLVMIsAVAArgInst","LLVMIsAVAArgInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFreezeInst , SimNode_ExtFuncCall >(lib,"LLVMIsAFreezeInst","LLVMIsAFreezeInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAAtomicCmpXchgInst , SimNode_ExtFuncCall >(lib,"LLVMIsAAtomicCmpXchgInst","LLVMIsAAtomicCmpXchgInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAAtomicRMWInst , SimNode_ExtFuncCall >(lib,"LLVMIsAAtomicRMWInst","LLVMIsAAtomicRMWInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1792:30
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAFenceInst , SimNode_ExtFuncCall >(lib,"LLVMIsAFenceInst","LLVMIsAFenceInst")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1794:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMDNode , SimNode_ExtFuncCall >(lib,"LLVMIsAMDNode","LLVMIsAMDNode")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


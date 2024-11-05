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
void Module_dasLLVM::initFunctions_35() {
// from D:\Work\libclang\include\llvm-c/Core.h:4020:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildInsertElement , SimNode_ExtFuncCall >(lib,"LLVMBuildInsertElement","LLVMBuildInsertElement")
		->args({"","VecVal","EltVal","Index","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4023:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildShuffleVector , SimNode_ExtFuncCall >(lib,"LLVMBuildShuffleVector","LLVMBuildShuffleVector")
		->args({"","V1","V2","Mask","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4026:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,unsigned int,const char *) , LLVMBuildExtractValue , SimNode_ExtFuncCall >(lib,"LLVMBuildExtractValue","LLVMBuildExtractValue")
		->args({"","AggVal","Index","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4028:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,unsigned int,const char *) , LLVMBuildInsertValue , SimNode_ExtFuncCall >(lib,"LLVMBuildInsertValue","LLVMBuildInsertValue")
		->args({"","AggVal","EltVal","Index","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4031:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildFreeze , SimNode_ExtFuncCall >(lib,"LLVMBuildFreeze","LLVMBuildFreeze")
		->args({"","Val","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4034:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildIsNull , SimNode_ExtFuncCall >(lib,"LLVMBuildIsNull","LLVMBuildIsNull")
		->args({"","Val","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4036:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildIsNotNull , SimNode_ExtFuncCall >(lib,"LLVMBuildIsNotNull","LLVMBuildIsNotNull")
		->args({"","Val","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4038:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildPtrDiff2 , SimNode_ExtFuncCall >(lib,"LLVMBuildPtrDiff2","LLVMBuildPtrDiff2")
		->args({"","ElemTy","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4041:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMAtomicOrdering,int,const char *) , LLVMBuildFence , SimNode_ExtFuncCall >(lib,"LLVMBuildFence","LLVMBuildFence")
		->args({"B","ordering","singleThread","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4043:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMAtomicRMWBinOp,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMAtomicOrdering,int) , LLVMBuildAtomicRMW , SimNode_ExtFuncCall >(lib,"LLVMBuildAtomicRMW","LLVMBuildAtomicRMW")
		->args({"B","op","PTR","Val","ordering","singleThread"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4047:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMAtomicOrdering,LLVMAtomicOrdering,int) , LLVMBuildAtomicCmpXchg , SimNode_ExtFuncCall >(lib,"LLVMBuildAtomicCmpXchg","LLVMBuildAtomicCmpXchg")
		->args({"B","Ptr","Cmp","New","SuccessOrdering","FailureOrdering","SingleThread"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4056:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumMaskElements , SimNode_ExtFuncCall >(lib,"LLVMGetNumMaskElements","LLVMGetNumMaskElements")
		->args({"ShuffleVectorInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4062:5
	makeExtern< int (*)() , LLVMGetUndefMaskElem , SimNode_ExtFuncCall >(lib,"LLVMGetUndefMaskElem","LLVMGetUndefMaskElem")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4071:5
	makeExtern< int (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetMaskValue , SimNode_ExtFuncCall >(lib,"LLVMGetMaskValue","LLVMGetMaskValue")
		->args({"ShuffleVectorInst","Elt"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4073:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsAtomicSingleThread , SimNode_ExtFuncCall >(lib,"LLVMIsAtomicSingleThread","LLVMIsAtomicSingleThread")
		->args({"AtomicInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4074:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetAtomicSingleThread , SimNode_ExtFuncCall >(lib,"LLVMSetAtomicSingleThread","LLVMSetAtomicSingleThread")
		->args({"AtomicInst","SingleThread"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4076:20
	makeExtern< LLVMAtomicOrdering (*)(LLVMOpaqueValue *) , LLVMGetCmpXchgSuccessOrdering , SimNode_ExtFuncCall >(lib,"LLVMGetCmpXchgSuccessOrdering","LLVMGetCmpXchgSuccessOrdering")
		->args({"CmpXchgInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4077:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMAtomicOrdering) , LLVMSetCmpXchgSuccessOrdering , SimNode_ExtFuncCall >(lib,"LLVMSetCmpXchgSuccessOrdering","LLVMSetCmpXchgSuccessOrdering")
		->args({"CmpXchgInst","Ordering"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4079:20
	makeExtern< LLVMAtomicOrdering (*)(LLVMOpaqueValue *) , LLVMGetCmpXchgFailureOrdering , SimNode_ExtFuncCall >(lib,"LLVMGetCmpXchgFailureOrdering","LLVMGetCmpXchgFailureOrdering")
		->args({"CmpXchgInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4080:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMAtomicOrdering) , LLVMSetCmpXchgFailureOrdering , SimNode_ExtFuncCall >(lib,"LLVMSetCmpXchgFailureOrdering","LLVMSetCmpXchgFailureOrdering")
		->args({"CmpXchgInst","Ordering"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


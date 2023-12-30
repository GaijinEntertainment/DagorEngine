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
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMIntPredicate,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildICmp >(*this,lib,"LLVMBuildICmp",SideEffects::worstDefault,"LLVMBuildICmp")
		->args({"","Op","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4023:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMRealPredicate,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFCmp >(*this,lib,"LLVMBuildFCmp",SideEffects::worstDefault,"LLVMBuildFCmp")
		->args({"","Op","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4028:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,const char *) , LLVMBuildPhi >(*this,lib,"LLVMBuildPhi",SideEffects::worstDefault,"LLVMBuildPhi")
		->args({"","Ty","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4030:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildCall >(*this,lib,"LLVMBuildCall",SideEffects::worstDefault,"LLVMBuildCall")
		->args({"","Fn","Args","NumArgs","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4034:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildCall2 >(*this,lib,"LLVMBuildCall2",SideEffects::worstDefault,"LLVMBuildCall2")
		->args({"","","Fn","Args","NumArgs","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4037:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildSelect >(*this,lib,"LLVMBuildSelect",SideEffects::worstDefault,"LLVMBuildSelect")
		->args({"","If","Then","Else","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4040:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildVAArg >(*this,lib,"LLVMBuildVAArg",SideEffects::worstDefault,"LLVMBuildVAArg")
		->args({"","List","Ty","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4042:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildExtractElement >(*this,lib,"LLVMBuildExtractElement",SideEffects::worstDefault,"LLVMBuildExtractElement")
		->args({"","VecVal","Index","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4044:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildInsertElement >(*this,lib,"LLVMBuildInsertElement",SideEffects::worstDefault,"LLVMBuildInsertElement")
		->args({"","VecVal","EltVal","Index","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4047:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildShuffleVector >(*this,lib,"LLVMBuildShuffleVector",SideEffects::worstDefault,"LLVMBuildShuffleVector")
		->args({"","V1","V2","Mask","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4050:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,unsigned int,const char *) , LLVMBuildExtractValue >(*this,lib,"LLVMBuildExtractValue",SideEffects::worstDefault,"LLVMBuildExtractValue")
		->args({"","AggVal","Index","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4052:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,unsigned int,const char *) , LLVMBuildInsertValue >(*this,lib,"LLVMBuildInsertValue",SideEffects::worstDefault,"LLVMBuildInsertValue")
		->args({"","AggVal","EltVal","Index","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4055:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildFreeze >(*this,lib,"LLVMBuildFreeze",SideEffects::worstDefault,"LLVMBuildFreeze")
		->args({"","Val","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4058:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildIsNull >(*this,lib,"LLVMBuildIsNull",SideEffects::worstDefault,"LLVMBuildIsNull")
		->args({"","Val","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4060:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildIsNotNull >(*this,lib,"LLVMBuildIsNotNull",SideEffects::worstDefault,"LLVMBuildIsNotNull")
		->args({"","Val","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4063:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildPtrDiff >(*this,lib,"LLVMBuildPtrDiff",SideEffects::worstDefault,"LLVMBuildPtrDiff")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4066:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildPtrDiff2 >(*this,lib,"LLVMBuildPtrDiff2",SideEffects::worstDefault,"LLVMBuildPtrDiff2")
		->args({"","ElemTy","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4069:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMAtomicOrdering,int,const char *) , LLVMBuildFence >(*this,lib,"LLVMBuildFence",SideEffects::worstDefault,"LLVMBuildFence")
		->args({"B","ordering","singleThread","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4071:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMAtomicRMWBinOp,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMAtomicOrdering,int) , LLVMBuildAtomicRMW >(*this,lib,"LLVMBuildAtomicRMW",SideEffects::worstDefault,"LLVMBuildAtomicRMW")
		->args({"B","op","PTR","Val","ordering","singleThread"});
// from D:\Work\libclang\include\llvm-c/Core.h:4075:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMAtomicOrdering,LLVMAtomicOrdering,int) , LLVMBuildAtomicCmpXchg >(*this,lib,"LLVMBuildAtomicCmpXchg",SideEffects::worstDefault,"LLVMBuildAtomicCmpXchg")
		->args({"B","Ptr","Cmp","New","SuccessOrdering","FailureOrdering","SingleThread"});
}
}


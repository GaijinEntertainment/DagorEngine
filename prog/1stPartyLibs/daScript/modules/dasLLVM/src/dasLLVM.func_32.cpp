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
void Module_dasLLVM::initFunctions_32() {
// from D:\Work\libclang\include\llvm-c/Core.h:3873:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildOr , SimNode_ExtFuncCall >(lib,"LLVMBuildOr","LLVMBuildOr")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3875:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildXor , SimNode_ExtFuncCall >(lib,"LLVMBuildXor","LLVMBuildXor")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3877:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpcode,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildBinOp , SimNode_ExtFuncCall >(lib,"LLVMBuildBinOp","LLVMBuildBinOp")
		->args({"B","Op","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3880:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildNeg , SimNode_ExtFuncCall >(lib,"LLVMBuildNeg","LLVMBuildNeg")
		->args({"","V","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3881:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildNSWNeg , SimNode_ExtFuncCall >(lib,"LLVMBuildNSWNeg","LLVMBuildNSWNeg")
		->args({"B","V","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3883:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildNUWNeg , SimNode_ExtFuncCall >(lib,"LLVMBuildNUWNeg","LLVMBuildNUWNeg")
		->args({"B","V","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3885:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildFNeg , SimNode_ExtFuncCall >(lib,"LLVMBuildFNeg","LLVMBuildFNeg")
		->args({"","V","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3886:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildNot , SimNode_ExtFuncCall >(lib,"LLVMBuildNot","LLVMBuildNot")
		->args({"","V","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3889:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,const char *) , LLVMBuildMalloc , SimNode_ExtFuncCall >(lib,"LLVMBuildMalloc","LLVMBuildMalloc")
		->args({"","Ty","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3890:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,const char *) , LLVMBuildArrayMalloc , SimNode_ExtFuncCall >(lib,"LLVMBuildArrayMalloc","LLVMBuildArrayMalloc")
		->args({"","Ty","Val","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3899:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,unsigned int) , LLVMBuildMemSet , SimNode_ExtFuncCall >(lib,"LLVMBuildMemSet","LLVMBuildMemSet")
		->args({"B","Ptr","Val","Len","Align"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3907:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMBuildMemCpy , SimNode_ExtFuncCall >(lib,"LLVMBuildMemCpy","LLVMBuildMemCpy")
		->args({"B","Dst","DstAlign","Src","SrcAlign","Size"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3916:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMBuildMemMove , SimNode_ExtFuncCall >(lib,"LLVMBuildMemMove","LLVMBuildMemMove")
		->args({"B","Dst","DstAlign","Src","SrcAlign","Size"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3921:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,const char *) , LLVMBuildAlloca , SimNode_ExtFuncCall >(lib,"LLVMBuildAlloca","LLVMBuildAlloca")
		->args({"","Ty","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3922:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,const char *) , LLVMBuildArrayAlloca , SimNode_ExtFuncCall >(lib,"LLVMBuildArrayAlloca","LLVMBuildArrayAlloca")
		->args({"","Ty","Val","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3924:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMBuildFree , SimNode_ExtFuncCall >(lib,"LLVMBuildFree","LLVMBuildFree")
		->args({"","PointerVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3925:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,const char *) , LLVMBuildLoad2 , SimNode_ExtFuncCall >(lib,"LLVMBuildLoad2","LLVMBuildLoad2")
		->args({"","Ty","PointerVal","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3927:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMBuildStore , SimNode_ExtFuncCall >(lib,"LLVMBuildStore","LLVMBuildStore")
		->args({"","Val","Ptr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3928:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildGEP2 , SimNode_ExtFuncCall >(lib,"LLVMBuildGEP2","LLVMBuildGEP2")
		->args({"B","Ty","Pointer","Indices","NumIndices","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3931:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildInBoundsGEP2 , SimNode_ExtFuncCall >(lib,"LLVMBuildInBoundsGEP2","LLVMBuildInBoundsGEP2")
		->args({"B","Ty","Pointer","Indices","NumIndices","Name"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


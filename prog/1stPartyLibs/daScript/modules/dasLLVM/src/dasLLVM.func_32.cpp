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
// from D:\Work\libclang\include\llvm-c/Core.h:3868:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildLShr >(*this,lib,"LLVMBuildLShr",SideEffects::worstDefault,"LLVMBuildLShr")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3870:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildAShr >(*this,lib,"LLVMBuildAShr",SideEffects::worstDefault,"LLVMBuildAShr")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3872:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildAnd >(*this,lib,"LLVMBuildAnd",SideEffects::worstDefault,"LLVMBuildAnd")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3874:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildOr >(*this,lib,"LLVMBuildOr",SideEffects::worstDefault,"LLVMBuildOr")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3876:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildXor >(*this,lib,"LLVMBuildXor",SideEffects::worstDefault,"LLVMBuildXor")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3878:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpcode,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildBinOp >(*this,lib,"LLVMBuildBinOp",SideEffects::worstDefault,"LLVMBuildBinOp")
		->args({"B","Op","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3881:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildNeg >(*this,lib,"LLVMBuildNeg",SideEffects::worstDefault,"LLVMBuildNeg")
		->args({"","V","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3882:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildNSWNeg >(*this,lib,"LLVMBuildNSWNeg",SideEffects::worstDefault,"LLVMBuildNSWNeg")
		->args({"B","V","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3884:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildNUWNeg >(*this,lib,"LLVMBuildNUWNeg",SideEffects::worstDefault,"LLVMBuildNUWNeg")
		->args({"B","V","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3886:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildFNeg >(*this,lib,"LLVMBuildFNeg",SideEffects::worstDefault,"LLVMBuildFNeg")
		->args({"","V","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3887:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildNot >(*this,lib,"LLVMBuildNot",SideEffects::worstDefault,"LLVMBuildNot")
		->args({"","V","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3890:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,const char *) , LLVMBuildMalloc >(*this,lib,"LLVMBuildMalloc",SideEffects::worstDefault,"LLVMBuildMalloc")
		->args({"","Ty","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3891:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,const char *) , LLVMBuildArrayMalloc >(*this,lib,"LLVMBuildArrayMalloc",SideEffects::worstDefault,"LLVMBuildArrayMalloc")
		->args({"","Ty","Val","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3900:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,unsigned int) , LLVMBuildMemSet >(*this,lib,"LLVMBuildMemSet",SideEffects::worstDefault,"LLVMBuildMemSet")
		->args({"B","Ptr","Val","Len","Align"});
// from D:\Work\libclang\include\llvm-c/Core.h:3908:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMBuildMemCpy >(*this,lib,"LLVMBuildMemCpy",SideEffects::worstDefault,"LLVMBuildMemCpy")
		->args({"B","Dst","DstAlign","Src","SrcAlign","Size"});
// from D:\Work\libclang\include\llvm-c/Core.h:3917:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMBuildMemMove >(*this,lib,"LLVMBuildMemMove",SideEffects::worstDefault,"LLVMBuildMemMove")
		->args({"B","Dst","DstAlign","Src","SrcAlign","Size"});
// from D:\Work\libclang\include\llvm-c/Core.h:3922:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,const char *) , LLVMBuildAlloca >(*this,lib,"LLVMBuildAlloca",SideEffects::worstDefault,"LLVMBuildAlloca")
		->args({"","Ty","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3923:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,const char *) , LLVMBuildArrayAlloca >(*this,lib,"LLVMBuildArrayAlloca",SideEffects::worstDefault,"LLVMBuildArrayAlloca")
		->args({"","Ty","Val","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3925:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *) , LLVMBuildFree >(*this,lib,"LLVMBuildFree",SideEffects::worstDefault,"LLVMBuildFree")
		->args({"","PointerVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:3927:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,const char *) , LLVMBuildLoad >(*this,lib,"LLVMBuildLoad",SideEffects::worstDefault,"LLVMBuildLoad")
		->args({"","PointerVal","Name"});
}
}


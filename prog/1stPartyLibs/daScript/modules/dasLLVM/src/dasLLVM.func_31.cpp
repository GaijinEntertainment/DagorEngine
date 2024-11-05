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
void Module_dasLLVM::initFunctions_31() {
// from D:\Work\libclang\include\llvm-c/Core.h:3833:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildSub , SimNode_ExtFuncCall >(lib,"LLVMBuildSub","LLVMBuildSub")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3835:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNSWSub , SimNode_ExtFuncCall >(lib,"LLVMBuildNSWSub","LLVMBuildNSWSub")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3837:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNUWSub , SimNode_ExtFuncCall >(lib,"LLVMBuildNUWSub","LLVMBuildNUWSub")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3839:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFSub , SimNode_ExtFuncCall >(lib,"LLVMBuildFSub","LLVMBuildFSub")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3841:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildMul , SimNode_ExtFuncCall >(lib,"LLVMBuildMul","LLVMBuildMul")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3843:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNSWMul , SimNode_ExtFuncCall >(lib,"LLVMBuildNSWMul","LLVMBuildNSWMul")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3845:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNUWMul , SimNode_ExtFuncCall >(lib,"LLVMBuildNUWMul","LLVMBuildNUWMul")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3847:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFMul , SimNode_ExtFuncCall >(lib,"LLVMBuildFMul","LLVMBuildFMul")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3849:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildUDiv , SimNode_ExtFuncCall >(lib,"LLVMBuildUDiv","LLVMBuildUDiv")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3851:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildExactUDiv , SimNode_ExtFuncCall >(lib,"LLVMBuildExactUDiv","LLVMBuildExactUDiv")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3853:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildSDiv , SimNode_ExtFuncCall >(lib,"LLVMBuildSDiv","LLVMBuildSDiv")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3855:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildExactSDiv , SimNode_ExtFuncCall >(lib,"LLVMBuildExactSDiv","LLVMBuildExactSDiv")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3857:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFDiv , SimNode_ExtFuncCall >(lib,"LLVMBuildFDiv","LLVMBuildFDiv")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3859:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildURem , SimNode_ExtFuncCall >(lib,"LLVMBuildURem","LLVMBuildURem")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3861:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildSRem , SimNode_ExtFuncCall >(lib,"LLVMBuildSRem","LLVMBuildSRem")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3863:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFRem , SimNode_ExtFuncCall >(lib,"LLVMBuildFRem","LLVMBuildFRem")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3865:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildShl , SimNode_ExtFuncCall >(lib,"LLVMBuildShl","LLVMBuildShl")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3867:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildLShr , SimNode_ExtFuncCall >(lib,"LLVMBuildLShr","LLVMBuildLShr")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3869:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildAShr , SimNode_ExtFuncCall >(lib,"LLVMBuildAShr","LLVMBuildAShr")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3871:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildAnd , SimNode_ExtFuncCall >(lib,"LLVMBuildAnd","LLVMBuildAnd")
		->args({"","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


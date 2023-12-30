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
// from D:\Work\libclang\include\llvm-c/Core.h:3828:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNSWAdd >(*this,lib,"LLVMBuildNSWAdd",SideEffects::worstDefault,"LLVMBuildNSWAdd")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3830:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNUWAdd >(*this,lib,"LLVMBuildNUWAdd",SideEffects::worstDefault,"LLVMBuildNUWAdd")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3832:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFAdd >(*this,lib,"LLVMBuildFAdd",SideEffects::worstDefault,"LLVMBuildFAdd")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3834:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildSub >(*this,lib,"LLVMBuildSub",SideEffects::worstDefault,"LLVMBuildSub")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3836:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNSWSub >(*this,lib,"LLVMBuildNSWSub",SideEffects::worstDefault,"LLVMBuildNSWSub")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3838:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNUWSub >(*this,lib,"LLVMBuildNUWSub",SideEffects::worstDefault,"LLVMBuildNUWSub")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3840:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFSub >(*this,lib,"LLVMBuildFSub",SideEffects::worstDefault,"LLVMBuildFSub")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3842:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildMul >(*this,lib,"LLVMBuildMul",SideEffects::worstDefault,"LLVMBuildMul")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3844:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNSWMul >(*this,lib,"LLVMBuildNSWMul",SideEffects::worstDefault,"LLVMBuildNSWMul")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3846:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildNUWMul >(*this,lib,"LLVMBuildNUWMul",SideEffects::worstDefault,"LLVMBuildNUWMul")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3848:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFMul >(*this,lib,"LLVMBuildFMul",SideEffects::worstDefault,"LLVMBuildFMul")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3850:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildUDiv >(*this,lib,"LLVMBuildUDiv",SideEffects::worstDefault,"LLVMBuildUDiv")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3852:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildExactUDiv >(*this,lib,"LLVMBuildExactUDiv",SideEffects::worstDefault,"LLVMBuildExactUDiv")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3854:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildSDiv >(*this,lib,"LLVMBuildSDiv",SideEffects::worstDefault,"LLVMBuildSDiv")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3856:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildExactSDiv >(*this,lib,"LLVMBuildExactSDiv",SideEffects::worstDefault,"LLVMBuildExactSDiv")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3858:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFDiv >(*this,lib,"LLVMBuildFDiv",SideEffects::worstDefault,"LLVMBuildFDiv")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3860:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildURem >(*this,lib,"LLVMBuildURem",SideEffects::worstDefault,"LLVMBuildURem")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3862:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildSRem >(*this,lib,"LLVMBuildSRem",SideEffects::worstDefault,"LLVMBuildSRem")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3864:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFRem >(*this,lib,"LLVMBuildFRem",SideEffects::worstDefault,"LLVMBuildFRem")
		->args({"","LHS","RHS","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3866:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildShl >(*this,lib,"LLVMBuildShl",SideEffects::worstDefault,"LLVMBuildShl")
		->args({"","LHS","RHS","Name"});
}
}


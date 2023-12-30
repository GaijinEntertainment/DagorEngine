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
void Module_dasLLVM::initFunctions_19() {
// from D:\Work\libclang\include\llvm-c/Core.h:2222:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *,int) , LLVMConstIntCast >(*this,lib,"LLVMConstIntCast",SideEffects::worstDefault,"LLVMConstIntCast")
		->args({"ConstantVal","ToType","isSigned"});
// from D:\Work\libclang\include\llvm-c/Core.h:2224:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPCast >(*this,lib,"LLVMConstFPCast",SideEffects::worstDefault,"LLVMConstFPCast")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2225:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstSelect >(*this,lib,"LLVMConstSelect",SideEffects::worstDefault,"LLVMConstSelect")
		->args({"ConstantCondition","ConstantIfTrue","ConstantIfFalse"});
// from D:\Work\libclang\include\llvm-c/Core.h:2228:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstExtractElement >(*this,lib,"LLVMConstExtractElement",SideEffects::worstDefault,"LLVMConstExtractElement")
		->args({"VectorConstant","IndexConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2230:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstInsertElement >(*this,lib,"LLVMConstInsertElement",SideEffects::worstDefault,"LLVMConstInsertElement")
		->args({"VectorConstant","ElementValueConstant","IndexConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2233:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstShuffleVector >(*this,lib,"LLVMConstShuffleVector",SideEffects::worstDefault,"LLVMConstShuffleVector")
		->args({"VectorAConstant","VectorBConstant","MaskConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2236:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMBlockAddress >(*this,lib,"LLVMBlockAddress",SideEffects::worstDefault,"LLVMBlockAddress")
		->args({"F","BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:2239:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *,const char *,int,int) , LLVMConstInlineAsm >(*this,lib,"LLVMConstInlineAsm",SideEffects::worstDefault,"LLVMConstInlineAsm")
		->args({"Ty","AsmString","Constraints","HasSideEffects","IsAlignStack"});
// from D:\Work\libclang\include\llvm-c/Core.h:2258:15
	addExtern< LLVMOpaqueModule * (*)(LLVMOpaqueValue *) , LLVMGetGlobalParent >(*this,lib,"LLVMGetGlobalParent",SideEffects::worstDefault,"LLVMGetGlobalParent")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2259:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsDeclaration >(*this,lib,"LLVMIsDeclaration",SideEffects::worstDefault,"LLVMIsDeclaration")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2260:13
	addExtern< LLVMLinkage (*)(LLVMOpaqueValue *) , LLVMGetLinkage >(*this,lib,"LLVMGetLinkage",SideEffects::worstDefault,"LLVMGetLinkage")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2261:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMLinkage) , LLVMSetLinkage >(*this,lib,"LLVMSetLinkage",SideEffects::worstDefault,"LLVMSetLinkage")
		->args({"Global","Linkage"});
// from D:\Work\libclang\include\llvm-c/Core.h:2262:13
	addExtern< const char * (*)(LLVMOpaqueValue *) , LLVMGetSection >(*this,lib,"LLVMGetSection",SideEffects::worstDefault,"LLVMGetSection")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2263:6
	addExtern< void (*)(LLVMOpaqueValue *,const char *) , LLVMSetSection >(*this,lib,"LLVMSetSection",SideEffects::worstDefault,"LLVMSetSection")
		->args({"Global","Section"});
// from D:\Work\libclang\include\llvm-c/Core.h:2264:16
	addExtern< LLVMVisibility (*)(LLVMOpaqueValue *) , LLVMGetVisibility >(*this,lib,"LLVMGetVisibility",SideEffects::worstDefault,"LLVMGetVisibility")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2265:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMVisibility) , LLVMSetVisibility >(*this,lib,"LLVMSetVisibility",SideEffects::worstDefault,"LLVMSetVisibility")
		->args({"Global","Viz"});
// from D:\Work\libclang\include\llvm-c/Core.h:2266:21
	addExtern< LLVMDLLStorageClass (*)(LLVMOpaqueValue *) , LLVMGetDLLStorageClass >(*this,lib,"LLVMGetDLLStorageClass",SideEffects::worstDefault,"LLVMGetDLLStorageClass")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2267:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMDLLStorageClass) , LLVMSetDLLStorageClass >(*this,lib,"LLVMSetDLLStorageClass",SideEffects::worstDefault,"LLVMSetDLLStorageClass")
		->args({"Global","Class"});
// from D:\Work\libclang\include\llvm-c/Core.h:2268:17
	addExtern< LLVMUnnamedAddr (*)(LLVMOpaqueValue *) , LLVMGetUnnamedAddress >(*this,lib,"LLVMGetUnnamedAddress",SideEffects::worstDefault,"LLVMGetUnnamedAddress")
		->args({"Global"});
// from D:\Work\libclang\include\llvm-c/Core.h:2269:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMUnnamedAddr) , LLVMSetUnnamedAddress >(*this,lib,"LLVMSetUnnamedAddress",SideEffects::worstDefault,"LLVMSetUnnamedAddress")
		->args({"Global","UnnamedAddr"});
}
}


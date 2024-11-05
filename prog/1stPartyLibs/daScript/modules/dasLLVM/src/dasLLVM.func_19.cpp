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
// from D:\Work\libclang\include\llvm-c/Core.h:2234:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPCast , SimNode_ExtFuncCall >(lib,"LLVMConstFPCast","LLVMConstFPCast")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2235:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstSelect , SimNode_ExtFuncCall >(lib,"LLVMConstSelect","LLVMConstSelect")
		->args({"ConstantCondition","ConstantIfTrue","ConstantIfFalse"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2238:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstExtractElement , SimNode_ExtFuncCall >(lib,"LLVMConstExtractElement","LLVMConstExtractElement")
		->args({"VectorConstant","IndexConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2240:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstInsertElement , SimNode_ExtFuncCall >(lib,"LLVMConstInsertElement","LLVMConstInsertElement")
		->args({"VectorConstant","ElementValueConstant","IndexConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2243:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstShuffleVector , SimNode_ExtFuncCall >(lib,"LLVMConstShuffleVector","LLVMConstShuffleVector")
		->args({"VectorAConstant","VectorBConstant","MaskConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2246:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMBlockAddress , SimNode_ExtFuncCall >(lib,"LLVMBlockAddress","LLVMBlockAddress")
		->args({"F","BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2249:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *,const char *,int,int) , LLVMConstInlineAsm , SimNode_ExtFuncCall >(lib,"LLVMConstInlineAsm","LLVMConstInlineAsm")
		->args({"Ty","AsmString","Constraints","HasSideEffects","IsAlignStack"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2268:15
	makeExtern< LLVMOpaqueModule * (*)(LLVMOpaqueValue *) , LLVMGetGlobalParent , SimNode_ExtFuncCall >(lib,"LLVMGetGlobalParent","LLVMGetGlobalParent")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2269:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsDeclaration , SimNode_ExtFuncCall >(lib,"LLVMIsDeclaration","LLVMIsDeclaration")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2270:13
	makeExtern< LLVMLinkage (*)(LLVMOpaqueValue *) , LLVMGetLinkage , SimNode_ExtFuncCall >(lib,"LLVMGetLinkage","LLVMGetLinkage")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2271:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMLinkage) , LLVMSetLinkage , SimNode_ExtFuncCall >(lib,"LLVMSetLinkage","LLVMSetLinkage")
		->args({"Global","Linkage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2272:13
	makeExtern< const char * (*)(LLVMOpaqueValue *) , LLVMGetSection , SimNode_ExtFuncCall >(lib,"LLVMGetSection","LLVMGetSection")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2273:6
	makeExtern< void (*)(LLVMOpaqueValue *,const char *) , LLVMSetSection , SimNode_ExtFuncCall >(lib,"LLVMSetSection","LLVMSetSection")
		->args({"Global","Section"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2274:16
	makeExtern< LLVMVisibility (*)(LLVMOpaqueValue *) , LLVMGetVisibility , SimNode_ExtFuncCall >(lib,"LLVMGetVisibility","LLVMGetVisibility")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2275:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMVisibility) , LLVMSetVisibility , SimNode_ExtFuncCall >(lib,"LLVMSetVisibility","LLVMSetVisibility")
		->args({"Global","Viz"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2276:21
	makeExtern< LLVMDLLStorageClass (*)(LLVMOpaqueValue *) , LLVMGetDLLStorageClass , SimNode_ExtFuncCall >(lib,"LLVMGetDLLStorageClass","LLVMGetDLLStorageClass")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2277:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMDLLStorageClass) , LLVMSetDLLStorageClass , SimNode_ExtFuncCall >(lib,"LLVMSetDLLStorageClass","LLVMSetDLLStorageClass")
		->args({"Global","Class"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2278:17
	makeExtern< LLVMUnnamedAddr (*)(LLVMOpaqueValue *) , LLVMGetUnnamedAddress , SimNode_ExtFuncCall >(lib,"LLVMGetUnnamedAddress","LLVMGetUnnamedAddress")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2279:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMUnnamedAddr) , LLVMSetUnnamedAddress , SimNode_ExtFuncCall >(lib,"LLVMSetUnnamedAddress","LLVMSetUnnamedAddress")
		->args({"Global","UnnamedAddr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2287:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMGlobalGetValueType , SimNode_ExtFuncCall >(lib,"LLVMGlobalGetValueType","LLVMGlobalGetValueType")
		->args({"Global"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


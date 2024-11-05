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
void Module_dasLLVM::initFunctions_16() {
// from D:\Work\libclang\include\llvm-c/Core.h:2014:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *,unsigned int,unsigned char) , LLVMConstIntOfStringAndSize , SimNode_ExtFuncCall >(lib,"LLVMConstIntOfStringAndSize","LLVMConstIntOfStringAndSize")
		->args({"IntTy","Text","SLen","Radix"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2020:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,double) , LLVMConstReal , SimNode_ExtFuncCall >(lib,"LLVMConstReal","LLVMConstReal")
		->args({"RealTy","N"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2028:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *) , LLVMConstRealOfString , SimNode_ExtFuncCall >(lib,"LLVMConstRealOfString","LLVMConstRealOfString")
		->args({"RealTy","Text"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2033:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *,unsigned int) , LLVMConstRealOfStringAndSize , SimNode_ExtFuncCall >(lib,"LLVMConstRealOfStringAndSize","LLVMConstRealOfStringAndSize")
		->args({"RealTy","Text","SLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2041:20
	makeExtern< unsigned long long (*)(LLVMOpaqueValue *) , LLVMConstIntGetZExtValue , SimNode_ExtFuncCall >(lib,"LLVMConstIntGetZExtValue","LLVMConstIntGetZExtValue")
		->args({"ConstantVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2048:11
	makeExtern< long long (*)(LLVMOpaqueValue *) , LLVMConstIntGetSExtValue , SimNode_ExtFuncCall >(lib,"LLVMConstIntGetSExtValue","LLVMConstIntGetSExtValue")
		->args({"ConstantVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2056:8
	makeExtern< double (*)(LLVMOpaqueValue *,int *) , LLVMConstRealGetDouble , SimNode_ExtFuncCall >(lib,"LLVMConstRealGetDouble","LLVMConstRealGetDouble")
		->args({"ConstantVal","losesInfo"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2075:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,const char *,unsigned int,int) , LLVMConstStringInContext , SimNode_ExtFuncCall >(lib,"LLVMConstStringInContext","LLVMConstStringInContext")
		->args({"C","Str","Length","DontNullTerminate"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2087:14
	makeExtern< LLVMOpaqueValue * (*)(const char *,unsigned int,int) , LLVMConstString , SimNode_ExtFuncCall >(lib,"LLVMConstString","LLVMConstString")
		->args({"Str","Length","DontNullTerminate"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2095:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsConstantString , SimNode_ExtFuncCall >(lib,"LLVMIsConstantString","LLVMIsConstantString")
		->args({"c"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2102:13
	makeExtern< const char * (*)(LLVMOpaqueValue *,size_t *) , LLVMGetAsString , SimNode_ExtFuncCall >(lib,"LLVMGetAsString","LLVMGetAsString")
		->args({"c","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2109:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,LLVMOpaqueValue **,unsigned int,int) , LLVMConstStructInContext , SimNode_ExtFuncCall >(lib,"LLVMConstStructInContext","LLVMConstStructInContext")
		->args({"C","ConstantVals","Count","Packed"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2121:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue **,unsigned int,int) , LLVMConstStruct , SimNode_ExtFuncCall >(lib,"LLVMConstStruct","LLVMConstStruct")
		->args({"ConstantVals","Count","Packed"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2129:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,LLVMOpaqueValue **,unsigned int) , LLVMConstArray , SimNode_ExtFuncCall >(lib,"LLVMConstArray","LLVMConstArray")
		->args({"ElementTy","ConstantVals","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2137:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,LLVMOpaqueValue **,unsigned int) , LLVMConstNamedStruct , SimNode_ExtFuncCall >(lib,"LLVMConstNamedStruct","LLVMConstNamedStruct")
		->args({"StructTy","ConstantVals","Count"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2149:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetAggregateElement , SimNode_ExtFuncCall >(lib,"LLVMGetAggregateElement","LLVMGetAggregateElement")
		->args({"C","Idx"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2157:18
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetElementAsConstant , SimNode_ExtFuncCall >(lib,"LLVMGetElementAsConstant","LLVMGetElementAsConstant")
		->args({"C","idx"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2165:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue **,unsigned int) , LLVMConstVector , SimNode_ExtFuncCall >(lib,"LLVMConstVector","LLVMConstVector")
		->args({"ScalarConstantVals","Size"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2180:12
	makeExtern< LLVMOpcode (*)(LLVMOpaqueValue *) , LLVMGetConstOpcode , SimNode_ExtFuncCall >(lib,"LLVMGetConstOpcode","LLVMGetConstOpcode")
		->args({"ConstantVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2181:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMAlignOf , SimNode_ExtFuncCall >(lib,"LLVMAlignOf","LLVMAlignOf")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


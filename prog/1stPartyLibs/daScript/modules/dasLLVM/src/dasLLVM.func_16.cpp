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
// from D:\Work\libclang\include\llvm-c/Core.h:2007:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *) , LLVMConstRealOfString >(*this,lib,"LLVMConstRealOfString",SideEffects::worstDefault,"LLVMConstRealOfString")
		->args({"RealTy","Text"});
// from D:\Work\libclang\include\llvm-c/Core.h:2012:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *,unsigned int) , LLVMConstRealOfStringAndSize >(*this,lib,"LLVMConstRealOfStringAndSize",SideEffects::worstDefault,"LLVMConstRealOfStringAndSize")
		->args({"RealTy","Text","SLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:2020:20
	addExtern< unsigned long long (*)(LLVMOpaqueValue *) , LLVMConstIntGetZExtValue >(*this,lib,"LLVMConstIntGetZExtValue",SideEffects::worstDefault,"LLVMConstIntGetZExtValue")
		->args({"ConstantVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:2027:11
	addExtern< long long (*)(LLVMOpaqueValue *) , LLVMConstIntGetSExtValue >(*this,lib,"LLVMConstIntGetSExtValue",SideEffects::worstDefault,"LLVMConstIntGetSExtValue")
		->args({"ConstantVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:2035:8
	addExtern< double (*)(LLVMOpaqueValue *,int *) , LLVMConstRealGetDouble >(*this,lib,"LLVMConstRealGetDouble",SideEffects::worstDefault,"LLVMConstRealGetDouble")
		->args({"ConstantVal","losesInfo"});
// from D:\Work\libclang\include\llvm-c/Core.h:2054:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,const char *,unsigned int,int) , LLVMConstStringInContext >(*this,lib,"LLVMConstStringInContext",SideEffects::worstDefault,"LLVMConstStringInContext")
		->args({"C","Str","Length","DontNullTerminate"});
// from D:\Work\libclang\include\llvm-c/Core.h:2066:14
	addExtern< LLVMOpaqueValue * (*)(const char *,unsigned int,int) , LLVMConstString >(*this,lib,"LLVMConstString",SideEffects::worstDefault,"LLVMConstString")
		->args({"Str","Length","DontNullTerminate"});
// from D:\Work\libclang\include\llvm-c/Core.h:2074:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsConstantString >(*this,lib,"LLVMIsConstantString",SideEffects::worstDefault,"LLVMIsConstantString")
		->args({"c"});
// from D:\Work\libclang\include\llvm-c/Core.h:2081:13
	addExtern< const char * (*)(LLVMOpaqueValue *,size_t *) , LLVMGetAsString >(*this,lib,"LLVMGetAsString",SideEffects::worstDefault,"LLVMGetAsString")
		->args({"c","Length"});
// from D:\Work\libclang\include\llvm-c/Core.h:2088:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,LLVMOpaqueValue **,unsigned int,int) , LLVMConstStructInContext >(*this,lib,"LLVMConstStructInContext",SideEffects::worstDefault,"LLVMConstStructInContext")
		->args({"C","ConstantVals","Count","Packed"});
// from D:\Work\libclang\include\llvm-c/Core.h:2100:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue **,unsigned int,int) , LLVMConstStruct >(*this,lib,"LLVMConstStruct",SideEffects::worstDefault,"LLVMConstStruct")
		->args({"ConstantVals","Count","Packed"});
// from D:\Work\libclang\include\llvm-c/Core.h:2108:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,LLVMOpaqueValue **,unsigned int) , LLVMConstArray >(*this,lib,"LLVMConstArray",SideEffects::worstDefault,"LLVMConstArray")
		->args({"ElementTy","ConstantVals","Length"});
// from D:\Work\libclang\include\llvm-c/Core.h:2116:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,LLVMOpaqueValue **,unsigned int) , LLVMConstNamedStruct >(*this,lib,"LLVMConstNamedStruct",SideEffects::worstDefault,"LLVMConstNamedStruct")
		->args({"StructTy","ConstantVals","Count"});
// from D:\Work\libclang\include\llvm-c/Core.h:2128:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetAggregateElement >(*this,lib,"LLVMGetAggregateElement",SideEffects::worstDefault,"LLVMGetAggregateElement")
		->args({"C","Idx"});
// from D:\Work\libclang\include\llvm-c/Core.h:2136:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetElementAsConstant >(*this,lib,"LLVMGetElementAsConstant",SideEffects::worstDefault,"LLVMGetElementAsConstant")
		->args({"C","idx"});
// from D:\Work\libclang\include\llvm-c/Core.h:2144:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue **,unsigned int) , LLVMConstVector >(*this,lib,"LLVMConstVector",SideEffects::worstDefault,"LLVMConstVector")
		->args({"ScalarConstantVals","Size"});
// from D:\Work\libclang\include\llvm-c/Core.h:2159:12
	addExtern< LLVMOpcode (*)(LLVMOpaqueValue *) , LLVMGetConstOpcode >(*this,lib,"LLVMGetConstOpcode",SideEffects::worstDefault,"LLVMGetConstOpcode")
		->args({"ConstantVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:2160:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMAlignOf >(*this,lib,"LLVMAlignOf",SideEffects::worstDefault,"LLVMAlignOf")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:2161:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMSizeOf >(*this,lib,"LLVMSizeOf",SideEffects::worstDefault,"LLVMSizeOf")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:2162:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMConstNeg >(*this,lib,"LLVMConstNeg",SideEffects::worstDefault,"LLVMConstNeg")
		->args({"ConstantVal"});
}
}


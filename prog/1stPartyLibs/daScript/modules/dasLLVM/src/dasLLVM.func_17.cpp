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
void Module_dasLLVM::initFunctions_17() {
// from D:\Work\libclang\include\llvm-c/Core.h:2163:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMConstNSWNeg >(*this,lib,"LLVMConstNSWNeg",SideEffects::worstDefault,"LLVMConstNSWNeg")
		->args({"ConstantVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:2164:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMConstNUWNeg >(*this,lib,"LLVMConstNUWNeg",SideEffects::worstDefault,"LLVMConstNUWNeg")
		->args({"ConstantVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:2165:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMConstFNeg >(*this,lib,"LLVMConstFNeg",SideEffects::worstDefault,"LLVMConstFNeg")
		->args({"ConstantVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:2166:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMConstNot >(*this,lib,"LLVMConstNot",SideEffects::worstDefault,"LLVMConstNot")
		->args({"ConstantVal"});
// from D:\Work\libclang\include\llvm-c/Core.h:2167:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstAdd >(*this,lib,"LLVMConstAdd",SideEffects::worstDefault,"LLVMConstAdd")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2168:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNSWAdd >(*this,lib,"LLVMConstNSWAdd",SideEffects::worstDefault,"LLVMConstNSWAdd")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2169:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNUWAdd >(*this,lib,"LLVMConstNUWAdd",SideEffects::worstDefault,"LLVMConstNUWAdd")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2170:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstSub >(*this,lib,"LLVMConstSub",SideEffects::worstDefault,"LLVMConstSub")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2171:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNSWSub >(*this,lib,"LLVMConstNSWSub",SideEffects::worstDefault,"LLVMConstNSWSub")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2172:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNUWSub >(*this,lib,"LLVMConstNUWSub",SideEffects::worstDefault,"LLVMConstNUWSub")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2173:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstMul >(*this,lib,"LLVMConstMul",SideEffects::worstDefault,"LLVMConstMul")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2174:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNSWMul >(*this,lib,"LLVMConstNSWMul",SideEffects::worstDefault,"LLVMConstNSWMul")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2175:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNUWMul >(*this,lib,"LLVMConstNUWMul",SideEffects::worstDefault,"LLVMConstNUWMul")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2176:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstAnd >(*this,lib,"LLVMConstAnd",SideEffects::worstDefault,"LLVMConstAnd")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2177:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstOr >(*this,lib,"LLVMConstOr",SideEffects::worstDefault,"LLVMConstOr")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2178:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstXor >(*this,lib,"LLVMConstXor",SideEffects::worstDefault,"LLVMConstXor")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2179:14
	addExtern< LLVMOpaqueValue * (*)(LLVMIntPredicate,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstICmp >(*this,lib,"LLVMConstICmp",SideEffects::worstDefault,"LLVMConstICmp")
		->args({"Predicate","LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2181:14
	addExtern< LLVMOpaqueValue * (*)(LLVMRealPredicate,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstFCmp >(*this,lib,"LLVMConstFCmp",SideEffects::worstDefault,"LLVMConstFCmp")
		->args({"Predicate","LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2183:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstShl >(*this,lib,"LLVMConstShl",SideEffects::worstDefault,"LLVMConstShl")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2184:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstLShr >(*this,lib,"LLVMConstLShr",SideEffects::worstDefault,"LLVMConstLShr")
		->args({"LHSConstant","RHSConstant"});
}
}


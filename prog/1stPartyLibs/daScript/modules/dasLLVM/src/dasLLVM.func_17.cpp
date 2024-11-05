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
// from D:\Work\libclang\include\llvm-c/Core.h:2182:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMSizeOf , SimNode_ExtFuncCall >(lib,"LLVMSizeOf","LLVMSizeOf")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2183:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMConstNeg , SimNode_ExtFuncCall >(lib,"LLVMConstNeg","LLVMConstNeg")
		->args({"ConstantVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2184:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMConstNSWNeg , SimNode_ExtFuncCall >(lib,"LLVMConstNSWNeg","LLVMConstNSWNeg")
		->args({"ConstantVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2185:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMConstNUWNeg , SimNode_ExtFuncCall >(lib,"LLVMConstNUWNeg","LLVMConstNUWNeg")
		->args({"ConstantVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2186:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMConstNot , SimNode_ExtFuncCall >(lib,"LLVMConstNot","LLVMConstNot")
		->args({"ConstantVal"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2187:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstAdd , SimNode_ExtFuncCall >(lib,"LLVMConstAdd","LLVMConstAdd")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2188:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNSWAdd , SimNode_ExtFuncCall >(lib,"LLVMConstNSWAdd","LLVMConstNSWAdd")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2189:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNUWAdd , SimNode_ExtFuncCall >(lib,"LLVMConstNUWAdd","LLVMConstNUWAdd")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2190:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstSub , SimNode_ExtFuncCall >(lib,"LLVMConstSub","LLVMConstSub")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2191:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNSWSub , SimNode_ExtFuncCall >(lib,"LLVMConstNSWSub","LLVMConstNSWSub")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2192:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNUWSub , SimNode_ExtFuncCall >(lib,"LLVMConstNUWSub","LLVMConstNUWSub")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2193:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstMul , SimNode_ExtFuncCall >(lib,"LLVMConstMul","LLVMConstMul")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2194:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNSWMul , SimNode_ExtFuncCall >(lib,"LLVMConstNSWMul","LLVMConstNSWMul")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2195:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstNUWMul , SimNode_ExtFuncCall >(lib,"LLVMConstNUWMul","LLVMConstNUWMul")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2196:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstAnd , SimNode_ExtFuncCall >(lib,"LLVMConstAnd","LLVMConstAnd")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2197:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstOr , SimNode_ExtFuncCall >(lib,"LLVMConstOr","LLVMConstOr")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2198:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstXor , SimNode_ExtFuncCall >(lib,"LLVMConstXor","LLVMConstXor")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2199:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMIntPredicate,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstICmp , SimNode_ExtFuncCall >(lib,"LLVMConstICmp","LLVMConstICmp")
		->args({"Predicate","LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2201:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMRealPredicate,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstFCmp , SimNode_ExtFuncCall >(lib,"LLVMConstFCmp","LLVMConstFCmp")
		->args({"Predicate","LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2203:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstShl , SimNode_ExtFuncCall >(lib,"LLVMConstShl","LLVMConstShl")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


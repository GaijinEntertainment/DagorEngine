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
void Module_dasLLVM::initFunctions_18() {
// from D:\Work\libclang\include\llvm-c/Core.h:2204:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstLShr , SimNode_ExtFuncCall >(lib,"LLVMConstLShr","LLVMConstLShr")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2205:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstAShr , SimNode_ExtFuncCall >(lib,"LLVMConstAShr","LLVMConstAShr")
		->args({"LHSConstant","RHSConstant"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2211:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstTrunc , SimNode_ExtFuncCall >(lib,"LLVMConstTrunc","LLVMConstTrunc")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2212:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstSExt , SimNode_ExtFuncCall >(lib,"LLVMConstSExt","LLVMConstSExt")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2213:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstZExt , SimNode_ExtFuncCall >(lib,"LLVMConstZExt","LLVMConstZExt")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2214:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPTrunc , SimNode_ExtFuncCall >(lib,"LLVMConstFPTrunc","LLVMConstFPTrunc")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2215:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPExt , SimNode_ExtFuncCall >(lib,"LLVMConstFPExt","LLVMConstFPExt")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2216:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstUIToFP , SimNode_ExtFuncCall >(lib,"LLVMConstUIToFP","LLVMConstUIToFP")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2217:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstSIToFP , SimNode_ExtFuncCall >(lib,"LLVMConstSIToFP","LLVMConstSIToFP")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2218:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPToUI , SimNode_ExtFuncCall >(lib,"LLVMConstFPToUI","LLVMConstFPToUI")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2219:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPToSI , SimNode_ExtFuncCall >(lib,"LLVMConstFPToSI","LLVMConstFPToSI")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2220:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstPtrToInt , SimNode_ExtFuncCall >(lib,"LLVMConstPtrToInt","LLVMConstPtrToInt")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2221:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstIntToPtr , SimNode_ExtFuncCall >(lib,"LLVMConstIntToPtr","LLVMConstIntToPtr")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2222:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstBitCast , SimNode_ExtFuncCall >(lib,"LLVMConstBitCast","LLVMConstBitCast")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2223:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstAddrSpaceCast , SimNode_ExtFuncCall >(lib,"LLVMConstAddrSpaceCast","LLVMConstAddrSpaceCast")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2224:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstZExtOrBitCast , SimNode_ExtFuncCall >(lib,"LLVMConstZExtOrBitCast","LLVMConstZExtOrBitCast")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2226:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstSExtOrBitCast , SimNode_ExtFuncCall >(lib,"LLVMConstSExtOrBitCast","LLVMConstSExtOrBitCast")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2228:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstTruncOrBitCast , SimNode_ExtFuncCall >(lib,"LLVMConstTruncOrBitCast","LLVMConstTruncOrBitCast")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2230:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstPointerCast , SimNode_ExtFuncCall >(lib,"LLVMConstPointerCast","LLVMConstPointerCast")
		->args({"ConstantVal","ToType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2232:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *,int) , LLVMConstIntCast , SimNode_ExtFuncCall >(lib,"LLVMConstIntCast","LLVMConstIntCast")
		->args({"ConstantVal","ToType","isSigned"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


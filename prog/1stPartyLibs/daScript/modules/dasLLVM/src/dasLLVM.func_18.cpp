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
// from D:\Work\libclang\include\llvm-c/Core.h:2185:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMConstAShr >(*this,lib,"LLVMConstAShr",SideEffects::worstDefault,"LLVMConstAShr")
		->args({"LHSConstant","RHSConstant"});
// from D:\Work\libclang\include\llvm-c/Core.h:2187:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int) , LLVMConstGEP >(*this,lib,"LLVMConstGEP",SideEffects::worstDefault,"LLVMConstGEP")
		->args({"ConstantVal","ConstantIndices","NumIndices"});
// from D:\Work\libclang\include\llvm-c/Core.h:2194:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int) , LLVMConstInBoundsGEP >(*this,lib,"LLVMConstInBoundsGEP",SideEffects::worstDefault,"LLVMConstInBoundsGEP")
		->args({"ConstantVal","ConstantIndices","NumIndices"});
// from D:\Work\libclang\include\llvm-c/Core.h:2201:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstTrunc >(*this,lib,"LLVMConstTrunc",SideEffects::worstDefault,"LLVMConstTrunc")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2202:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstSExt >(*this,lib,"LLVMConstSExt",SideEffects::worstDefault,"LLVMConstSExt")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2203:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstZExt >(*this,lib,"LLVMConstZExt",SideEffects::worstDefault,"LLVMConstZExt")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2204:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPTrunc >(*this,lib,"LLVMConstFPTrunc",SideEffects::worstDefault,"LLVMConstFPTrunc")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2205:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPExt >(*this,lib,"LLVMConstFPExt",SideEffects::worstDefault,"LLVMConstFPExt")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2206:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstUIToFP >(*this,lib,"LLVMConstUIToFP",SideEffects::worstDefault,"LLVMConstUIToFP")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2207:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstSIToFP >(*this,lib,"LLVMConstSIToFP",SideEffects::worstDefault,"LLVMConstSIToFP")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2208:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPToUI >(*this,lib,"LLVMConstFPToUI",SideEffects::worstDefault,"LLVMConstFPToUI")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2209:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstFPToSI >(*this,lib,"LLVMConstFPToSI",SideEffects::worstDefault,"LLVMConstFPToSI")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2210:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstPtrToInt >(*this,lib,"LLVMConstPtrToInt",SideEffects::worstDefault,"LLVMConstPtrToInt")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2211:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstIntToPtr >(*this,lib,"LLVMConstIntToPtr",SideEffects::worstDefault,"LLVMConstIntToPtr")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2212:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstBitCast >(*this,lib,"LLVMConstBitCast",SideEffects::worstDefault,"LLVMConstBitCast")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2213:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstAddrSpaceCast >(*this,lib,"LLVMConstAddrSpaceCast",SideEffects::worstDefault,"LLVMConstAddrSpaceCast")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2214:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstZExtOrBitCast >(*this,lib,"LLVMConstZExtOrBitCast",SideEffects::worstDefault,"LLVMConstZExtOrBitCast")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2216:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstSExtOrBitCast >(*this,lib,"LLVMConstSExtOrBitCast",SideEffects::worstDefault,"LLVMConstSExtOrBitCast")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2218:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstTruncOrBitCast >(*this,lib,"LLVMConstTruncOrBitCast",SideEffects::worstDefault,"LLVMConstTruncOrBitCast")
		->args({"ConstantVal","ToType"});
// from D:\Work\libclang\include\llvm-c/Core.h:2220:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,LLVMOpaqueType *) , LLVMConstPointerCast >(*this,lib,"LLVMConstPointerCast",SideEffects::worstDefault,"LLVMConstPointerCast")
		->args({"ConstantVal","ToType"});
}
}


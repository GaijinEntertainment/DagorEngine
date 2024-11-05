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
void Module_dasLLVM::initFunctions_15() {
// from D:\Work\libclang\include\llvm-c/Core.h:1795:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsAMDString , SimNode_ExtFuncCall >(lib,"LLVMIsAMDString","LLVMIsAMDString")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1798:13
	makeExtern< const char * (*)(LLVMOpaqueValue *) , LLVMGetValueName , SimNode_ExtFuncCall >(lib,"LLVMGetValueName","LLVMGetValueName")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1800:6
	makeExtern< void (*)(LLVMOpaqueValue *,const char *) , LLVMSetValueName , SimNode_ExtFuncCall >(lib,"LLVMSetValueName","LLVMSetValueName")
		->args({"Val","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1829:12
	makeExtern< LLVMOpaqueUse * (*)(LLVMOpaqueValue *) , LLVMGetFirstUse , SimNode_ExtFuncCall >(lib,"LLVMGetFirstUse","LLVMGetFirstUse")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1837:12
	makeExtern< LLVMOpaqueUse * (*)(LLVMOpaqueUse *) , LLVMGetNextUse , SimNode_ExtFuncCall >(lib,"LLVMGetNextUse","LLVMGetNextUse")
		->args({"U"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1846:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueUse *) , LLVMGetUser , SimNode_ExtFuncCall >(lib,"LLVMGetUser","LLVMGetUser")
		->args({"U"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1853:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueUse *) , LLVMGetUsedValue , SimNode_ExtFuncCall >(lib,"LLVMGetUsedValue","LLVMGetUsedValue")
		->args({"U"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1874:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetOperand , SimNode_ExtFuncCall >(lib,"LLVMGetOperand","LLVMGetOperand")
		->args({"Val","Index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1881:12
	makeExtern< LLVMOpaqueUse * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetOperandUse , SimNode_ExtFuncCall >(lib,"LLVMGetOperandUse","LLVMGetOperandUse")
		->args({"Val","Index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1888:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMSetOperand , SimNode_ExtFuncCall >(lib,"LLVMSetOperand","LLVMSetOperand")
		->args({"User","Index","Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1895:5
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMGetNumOperands , SimNode_ExtFuncCall >(lib,"LLVMGetNumOperands","LLVMGetNumOperands")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1918:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMConstNull , SimNode_ExtFuncCall >(lib,"LLVMConstNull","LLVMConstNull")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1928:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMConstAllOnes , SimNode_ExtFuncCall >(lib,"LLVMConstAllOnes","LLVMConstAllOnes")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1935:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMGetUndef , SimNode_ExtFuncCall >(lib,"LLVMGetUndef","LLVMGetUndef")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1942:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMGetPoison , SimNode_ExtFuncCall >(lib,"LLVMGetPoison","LLVMGetPoison")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1949:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMIsNull , SimNode_ExtFuncCall >(lib,"LLVMIsNull","LLVMIsNull")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1955:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMConstPointerNull , SimNode_ExtFuncCall >(lib,"LLVMConstPointerNull","LLVMConstPointerNull")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1984:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,unsigned long long,int) , LLVMConstInt , SimNode_ExtFuncCall >(lib,"LLVMConstInt","LLVMConstInt")
		->args({"IntTy","N","SignExtend"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1992:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,unsigned int,const unsigned long long[]) , LLVMConstIntOfArbitraryPrecision , SimNode_ExtFuncCall >(lib,"LLVMConstIntOfArbitraryPrecision","LLVMConstIntOfArbitraryPrecision")
		->args({"IntTy","NumWords","Words"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2005:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *,unsigned char) , LLVMConstIntOfString , SimNode_ExtFuncCall >(lib,"LLVMConstIntOfString","LLVMConstIntOfString")
		->args({"IntTy","Text","Radix"})
		->addToModule(*this, SideEffects::worstDefault);
}
}


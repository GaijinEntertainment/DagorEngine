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
// from D:\Work\libclang\include\llvm-c/Core.h:1779:6
	addExtern< void (*)(LLVMOpaqueValue *,const char *) , LLVMSetValueName >(*this,lib,"LLVMSetValueName",SideEffects::worstDefault,"LLVMSetValueName")
		->args({"Val","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:1808:12
	addExtern< LLVMOpaqueUse * (*)(LLVMOpaqueValue *) , LLVMGetFirstUse >(*this,lib,"LLVMGetFirstUse",SideEffects::worstDefault,"LLVMGetFirstUse")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1816:12
	addExtern< LLVMOpaqueUse * (*)(LLVMOpaqueUse *) , LLVMGetNextUse >(*this,lib,"LLVMGetNextUse",SideEffects::worstDefault,"LLVMGetNextUse")
		->args({"U"});
// from D:\Work\libclang\include\llvm-c/Core.h:1825:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueUse *) , LLVMGetUser >(*this,lib,"LLVMGetUser",SideEffects::worstDefault,"LLVMGetUser")
		->args({"U"});
// from D:\Work\libclang\include\llvm-c/Core.h:1832:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueUse *) , LLVMGetUsedValue >(*this,lib,"LLVMGetUsedValue",SideEffects::worstDefault,"LLVMGetUsedValue")
		->args({"U"});
// from D:\Work\libclang\include\llvm-c/Core.h:1853:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetOperand >(*this,lib,"LLVMGetOperand",SideEffects::worstDefault,"LLVMGetOperand")
		->args({"Val","Index"});
// from D:\Work\libclang\include\llvm-c/Core.h:1860:12
	addExtern< LLVMOpaqueUse * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetOperandUse >(*this,lib,"LLVMGetOperandUse",SideEffects::worstDefault,"LLVMGetOperandUse")
		->args({"Val","Index"});
// from D:\Work\libclang\include\llvm-c/Core.h:1867:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMSetOperand >(*this,lib,"LLVMSetOperand",SideEffects::worstDefault,"LLVMSetOperand")
		->args({"User","Index","Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1874:5
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMGetNumOperands >(*this,lib,"LLVMGetNumOperands",SideEffects::worstDefault,"LLVMGetNumOperands")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1897:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMConstNull >(*this,lib,"LLVMConstNull",SideEffects::worstDefault,"LLVMConstNull")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1907:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMConstAllOnes >(*this,lib,"LLVMConstAllOnes",SideEffects::worstDefault,"LLVMConstAllOnes")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1914:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMGetUndef >(*this,lib,"LLVMGetUndef",SideEffects::worstDefault,"LLVMGetUndef")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1921:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMGetPoison >(*this,lib,"LLVMGetPoison",SideEffects::worstDefault,"LLVMGetPoison")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1928:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMIsNull >(*this,lib,"LLVMIsNull",SideEffects::worstDefault,"LLVMIsNull")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:1934:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *) , LLVMConstPointerNull >(*this,lib,"LLVMConstPointerNull",SideEffects::worstDefault,"LLVMConstPointerNull")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1963:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,unsigned long long,int) , LLVMConstInt >(*this,lib,"LLVMConstInt",SideEffects::worstDefault,"LLVMConstInt")
		->args({"IntTy","N","SignExtend"});
// from D:\Work\libclang\include\llvm-c/Core.h:1971:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,unsigned int,const unsigned long long[]) , LLVMConstIntOfArbitraryPrecision >(*this,lib,"LLVMConstIntOfArbitraryPrecision",SideEffects::worstDefault,"LLVMConstIntOfArbitraryPrecision")
		->args({"IntTy","NumWords","Words"});
// from D:\Work\libclang\include\llvm-c/Core.h:1984:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *,unsigned char) , LLVMConstIntOfString >(*this,lib,"LLVMConstIntOfString",SideEffects::worstDefault,"LLVMConstIntOfString")
		->args({"IntTy","Text","Radix"});
// from D:\Work\libclang\include\llvm-c/Core.h:1993:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,const char *,unsigned int,unsigned char) , LLVMConstIntOfStringAndSize >(*this,lib,"LLVMConstIntOfStringAndSize",SideEffects::worstDefault,"LLVMConstIntOfStringAndSize")
		->args({"IntTy","Text","SLen","Radix"});
// from D:\Work\libclang\include\llvm-c/Core.h:1999:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueType *,double) , LLVMConstReal >(*this,lib,"LLVMConstReal",SideEffects::worstDefault,"LLVMConstReal")
		->args({"RealTy","N"});
}
}


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
void Module_dasLLVM::initFunctions_9() {
// from D:\Work\libclang\include\llvm-c/Core.h:1437:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMArrayType >(*this,lib,"LLVMArrayType",SideEffects::worstDefault,"LLVMArrayType")
		->args({"ElementType","ElementCount"});
// from D:\Work\libclang\include\llvm-c/Core.h:1446:10
	addExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetArrayLength >(*this,lib,"LLVMGetArrayLength",SideEffects::worstDefault,"LLVMGetArrayLength")
		->args({"ArrayTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1456:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMPointerType >(*this,lib,"LLVMPointerType",SideEffects::worstDefault,"LLVMPointerType")
		->args({"ElementType","AddressSpace"});
// from D:\Work\libclang\include\llvm-c/Core.h:1465:10
	addExtern< int (*)(LLVMOpaqueType *) , LLVMPointerTypeIsOpaque >(*this,lib,"LLVMPointerTypeIsOpaque",SideEffects::worstDefault,"LLVMPointerTypeIsOpaque")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1472:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,unsigned int) , LLVMPointerTypeInContext >(*this,lib,"LLVMPointerTypeInContext",SideEffects::worstDefault,"LLVMPointerTypeInContext")
		->args({"C","AddressSpace"});
// from D:\Work\libclang\include\llvm-c/Core.h:1481:10
	addExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetPointerAddressSpace >(*this,lib,"LLVMGetPointerAddressSpace",SideEffects::worstDefault,"LLVMGetPointerAddressSpace")
		->args({"PointerTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1492:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMVectorType >(*this,lib,"LLVMVectorType",SideEffects::worstDefault,"LLVMVectorType")
		->args({"ElementType","ElementCount"});
// from D:\Work\libclang\include\llvm-c/Core.h:1503:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMScalableVectorType >(*this,lib,"LLVMScalableVectorType",SideEffects::worstDefault,"LLVMScalableVectorType")
		->args({"ElementType","ElementCount"});
// from D:\Work\libclang\include\llvm-c/Core.h:1513:10
	addExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetVectorSize >(*this,lib,"LLVMGetVectorSize",SideEffects::worstDefault,"LLVMGetVectorSize")
		->args({"VectorTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1528:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMVoidTypeInContext >(*this,lib,"LLVMVoidTypeInContext",SideEffects::worstDefault,"LLVMVoidTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1533:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMLabelTypeInContext >(*this,lib,"LLVMLabelTypeInContext",SideEffects::worstDefault,"LLVMLabelTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1538:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMX86MMXTypeInContext >(*this,lib,"LLVMX86MMXTypeInContext",SideEffects::worstDefault,"LLVMX86MMXTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1543:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMX86AMXTypeInContext >(*this,lib,"LLVMX86AMXTypeInContext",SideEffects::worstDefault,"LLVMX86AMXTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1548:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMTokenTypeInContext >(*this,lib,"LLVMTokenTypeInContext",SideEffects::worstDefault,"LLVMTokenTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1553:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMMetadataTypeInContext >(*this,lib,"LLVMMetadataTypeInContext",SideEffects::worstDefault,"LLVMMetadataTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1559:13
	addExtern< LLVMOpaqueType * (*)() , LLVMVoidType >(*this,lib,"LLVMVoidType",SideEffects::worstDefault,"LLVMVoidType");
// from D:\Work\libclang\include\llvm-c/Core.h:1560:13
	addExtern< LLVMOpaqueType * (*)() , LLVMLabelType >(*this,lib,"LLVMLabelType",SideEffects::worstDefault,"LLVMLabelType");
// from D:\Work\libclang\include\llvm-c/Core.h:1561:13
	addExtern< LLVMOpaqueType * (*)() , LLVMX86MMXType >(*this,lib,"LLVMX86MMXType",SideEffects::worstDefault,"LLVMX86MMXType");
// from D:\Work\libclang\include\llvm-c/Core.h:1562:13
	addExtern< LLVMOpaqueType * (*)() , LLVMX86AMXType >(*this,lib,"LLVMX86AMXType",SideEffects::worstDefault,"LLVMX86AMXType");
// from D:\Work\libclang\include\llvm-c/Core.h:1698:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueValue *) , LLVMTypeOf >(*this,lib,"LLVMTypeOf",SideEffects::worstDefault,"LLVMTypeOf")
		->args({"Val"});
}
}


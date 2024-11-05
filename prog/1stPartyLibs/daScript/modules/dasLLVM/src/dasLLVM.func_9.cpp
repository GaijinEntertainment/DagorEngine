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
// from D:\Work\libclang\include\llvm-c/Core.h:1439:10
	makeExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetNumContainedTypes , SimNode_ExtFuncCall >(lib,"LLVMGetNumContainedTypes","LLVMGetNumContainedTypes")
		->args({"Tp"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1449:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMArrayType , SimNode_ExtFuncCall >(lib,"LLVMArrayType","LLVMArrayType")
		->args({"ElementType","ElementCount"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1458:10
	makeExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetArrayLength , SimNode_ExtFuncCall >(lib,"LLVMGetArrayLength","LLVMGetArrayLength")
		->args({"ArrayTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1468:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMPointerType , SimNode_ExtFuncCall >(lib,"LLVMPointerType","LLVMPointerType")
		->args({"ElementType","AddressSpace"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1477:10
	makeExtern< int (*)(LLVMOpaqueType *) , LLVMPointerTypeIsOpaque , SimNode_ExtFuncCall >(lib,"LLVMPointerTypeIsOpaque","LLVMPointerTypeIsOpaque")
		->args({"Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1484:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,unsigned int) , LLVMPointerTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMPointerTypeInContext","LLVMPointerTypeInContext")
		->args({"C","AddressSpace"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1493:10
	makeExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetPointerAddressSpace , SimNode_ExtFuncCall >(lib,"LLVMGetPointerAddressSpace","LLVMGetPointerAddressSpace")
		->args({"PointerTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1504:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMVectorType , SimNode_ExtFuncCall >(lib,"LLVMVectorType","LLVMVectorType")
		->args({"ElementType","ElementCount"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1515:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMScalableVectorType , SimNode_ExtFuncCall >(lib,"LLVMScalableVectorType","LLVMScalableVectorType")
		->args({"ElementType","ElementCount"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1525:10
	makeExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetVectorSize , SimNode_ExtFuncCall >(lib,"LLVMGetVectorSize","LLVMGetVectorSize")
		->args({"VectorTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1540:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMVoidTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMVoidTypeInContext","LLVMVoidTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1545:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMLabelTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMLabelTypeInContext","LLVMLabelTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1550:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMX86MMXTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMX86MMXTypeInContext","LLVMX86MMXTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1555:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMX86AMXTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMX86AMXTypeInContext","LLVMX86AMXTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1560:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMTokenTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMTokenTypeInContext","LLVMTokenTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1565:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMMetadataTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMMetadataTypeInContext","LLVMMetadataTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1571:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMVoidType , SimNode_ExtFuncCall >(lib,"LLVMVoidType","LLVMVoidType")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1572:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMLabelType , SimNode_ExtFuncCall >(lib,"LLVMLabelType","LLVMLabelType")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1573:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMX86MMXType , SimNode_ExtFuncCall >(lib,"LLVMX86MMXType","LLVMX86MMXType")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1574:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMX86AMXType , SimNode_ExtFuncCall >(lib,"LLVMX86AMXType","LLVMX86AMXType")
		->addToModule(*this, SideEffects::worstDefault);
}
}


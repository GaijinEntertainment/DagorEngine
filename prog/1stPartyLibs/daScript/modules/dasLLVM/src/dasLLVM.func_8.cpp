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
void Module_dasLLVM::initFunctions_8() {
// from D:\Work\libclang\include\llvm-c/Core.h:1241:13
	addExtern< LLVMOpaqueType * (*)() , LLVMPPCFP128Type >(*this,lib,"LLVMPPCFP128Type",SideEffects::worstDefault,"LLVMPPCFP128Type");
// from D:\Work\libclang\include\llvm-c/Core.h:1259:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,LLVMOpaqueType **,unsigned int,int) , LLVMFunctionType >(*this,lib,"LLVMFunctionType",SideEffects::worstDefault,"LLVMFunctionType")
		->args({"ReturnType","ParamTypes","ParamCount","IsVarArg"});
// from D:\Work\libclang\include\llvm-c/Core.h:1266:10
	addExtern< int (*)(LLVMOpaqueType *) , LLVMIsFunctionVarArg >(*this,lib,"LLVMIsFunctionVarArg",SideEffects::worstDefault,"LLVMIsFunctionVarArg")
		->args({"FunctionTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1271:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *) , LLVMGetReturnType >(*this,lib,"LLVMGetReturnType",SideEffects::worstDefault,"LLVMGetReturnType")
		->args({"FunctionTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1276:10
	addExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMCountParamTypes >(*this,lib,"LLVMCountParamTypes",SideEffects::worstDefault,"LLVMCountParamTypes")
		->args({"FunctionTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1289:6
	addExtern< void (*)(LLVMOpaqueType *,LLVMOpaqueType **) , LLVMGetParamTypes >(*this,lib,"LLVMGetParamTypes",SideEffects::worstDefault,"LLVMGetParamTypes")
		->args({"FunctionTy","Dest"});
// from D:\Work\libclang\include\llvm-c/Core.h:1313:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,LLVMOpaqueType **,unsigned int,int) , LLVMStructTypeInContext >(*this,lib,"LLVMStructTypeInContext",SideEffects::worstDefault,"LLVMStructTypeInContext")
		->args({"C","ElementTypes","ElementCount","Packed"});
// from D:\Work\libclang\include\llvm-c/Core.h:1321:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueType **,unsigned int,int) , LLVMStructType >(*this,lib,"LLVMStructType",SideEffects::worstDefault,"LLVMStructType")
		->args({"ElementTypes","ElementCount","Packed"});
// from D:\Work\libclang\include\llvm-c/Core.h:1329:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,const char *) , LLVMStructCreateNamed >(*this,lib,"LLVMStructCreateNamed",SideEffects::worstDefault,"LLVMStructCreateNamed")
		->args({"C","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:1336:13
	addExtern< const char * (*)(LLVMOpaqueType *) , LLVMGetStructName >(*this,lib,"LLVMGetStructName",SideEffects::worstDefault,"LLVMGetStructName")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1343:6
	addExtern< void (*)(LLVMOpaqueType *,LLVMOpaqueType **,unsigned int,int) , LLVMStructSetBody >(*this,lib,"LLVMStructSetBody",SideEffects::worstDefault,"LLVMStructSetBody")
		->args({"StructTy","ElementTypes","ElementCount","Packed"});
// from D:\Work\libclang\include\llvm-c/Core.h:1351:10
	addExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMCountStructElementTypes >(*this,lib,"LLVMCountStructElementTypes",SideEffects::worstDefault,"LLVMCountStructElementTypes")
		->args({"StructTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1363:6
	addExtern< void (*)(LLVMOpaqueType *,LLVMOpaqueType **) , LLVMGetStructElementTypes >(*this,lib,"LLVMGetStructElementTypes",SideEffects::worstDefault,"LLVMGetStructElementTypes")
		->args({"StructTy","Dest"});
// from D:\Work\libclang\include\llvm-c/Core.h:1370:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *,unsigned int) , LLVMStructGetTypeAtIndex >(*this,lib,"LLVMStructGetTypeAtIndex",SideEffects::worstDefault,"LLVMStructGetTypeAtIndex")
		->args({"StructTy","i"});
// from D:\Work\libclang\include\llvm-c/Core.h:1377:10
	addExtern< int (*)(LLVMOpaqueType *) , LLVMIsPackedStruct >(*this,lib,"LLVMIsPackedStruct",SideEffects::worstDefault,"LLVMIsPackedStruct")
		->args({"StructTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1384:10
	addExtern< int (*)(LLVMOpaqueType *) , LLVMIsOpaqueStruct >(*this,lib,"LLVMIsOpaqueStruct",SideEffects::worstDefault,"LLVMIsOpaqueStruct")
		->args({"StructTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1391:10
	addExtern< int (*)(LLVMOpaqueType *) , LLVMIsLiteralStruct >(*this,lib,"LLVMIsLiteralStruct",SideEffects::worstDefault,"LLVMIsLiteralStruct")
		->args({"StructTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1413:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueType *) , LLVMGetElementType >(*this,lib,"LLVMGetElementType",SideEffects::worstDefault,"LLVMGetElementType")
		->args({"Ty"});
// from D:\Work\libclang\include\llvm-c/Core.h:1420:6
	addExtern< void (*)(LLVMOpaqueType *,LLVMOpaqueType **) , LLVMGetSubtypes >(*this,lib,"LLVMGetSubtypes",SideEffects::worstDefault,"LLVMGetSubtypes")
		->args({"Tp","Arr"});
// from D:\Work\libclang\include\llvm-c/Core.h:1427:10
	addExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetNumContainedTypes >(*this,lib,"LLVMGetNumContainedTypes",SideEffects::worstDefault,"LLVMGetNumContainedTypes")
		->args({"Tp"});
}
}

